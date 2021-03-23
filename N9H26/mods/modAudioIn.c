/***************************************************************************//**
 * @file     modAudioIn.c
 * @brief    AudioIn python module function
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/binary.h"
#include "py/objarray.h"

#if MICROPY_HW_ENABLE_AUDIOIN

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "wblib.h"
#include "N9H26_AUR.h"
#include "hal/N9H26_HAL_EDMA.h"

#define AIN_TASK_PRIORITY        (configMAX_PRIORITIES - 1)
#define AIN_CONFIG_FRAGMENT_DURATION                (100)
#define RAISE_OS_EXCEPTION(msg)     nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, msg))

#define __ENABLE_CACHE__

#ifdef __ENABLE_CACHE__
    #define E_NONCACHE_BIT  0x80000000
#else
    #define E_NONCACHE_BIT  0x00000000
#endif


typedef struct
{
    uint32_t u32SampleRate;
    uint32_t u32Channel;
} S_AIN_INFO;

typedef enum
{
	eAIN_THREAD_RUN,
	eAIN_THREAD_TO_STOP,
	eAIN_THREAD_STOP,	
}E_AIN_THREAD_SATAE;

typedef struct
{
    S_AIN_INFO sAinInfo;
    uint32_t u32PCMFragBufSize;

    uint32_t u32PCMEDMABufSize;
    uint8_t *pu8PCMEDMABuf;

    uint8_t *pu8PCMBuf;
    uint32_t u32PCMBufSize;
    uint32_t u32PCMInIdx;
    uint32_t u32PCMOutIdx;

    SemaphoreHandle_t tAinISRSem;
    xSemaphoreHandle tPCMIndexMutex;

    uint32_t u32EDMAChannel;
    uint64_t u64LastFrameTimestamp;

} S_AIN_CTRL;


static S_AIN_CTRL s_sAinCtrl = {
	.u32PCMFragBufSize = 0,
	.u32PCMEDMABufSize = 0,
	.pu8PCMEDMABuf = NULL,
	.pu8PCMBuf = NULL,
	.u32PCMBufSize = 0,
	.u32PCMInIdx = 0,
	.u32PCMOutIdx = 0,
	.tAinISRSem = NULL,
	.tPCMIndexMutex = NULL,
};

static mp_obj_t g_audio_callback = mp_const_none;
static mp_obj_array_t *g_pcmbuf = NULL;

static portBASE_TYPE xHigherPrioTaskWoken = pdFALSE;
static volatile BOOL s_bIsEDMABufferDone = 0;
static volatile E_AIN_THREAD_SATAE s_eAIN_ThreadState;

//////////////////////////////////////////////////////////////////////////////
// A read-only buffer that contains a C pointer
// Used to communicate function pointers to Micropython bindings
///////////////////////////////////////////////////////////////////////////////

typedef struct mp_ptr_t
{
    mp_obj_base_t base;
    void *ptr;
} mp_ptr_t;

STATIC mp_int_t mp_ptr_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags)
{
    mp_ptr_t *self = MP_OBJ_TO_PTR(self_in);

    if (flags & MP_BUFFER_WRITE) {
        // read-only ptr
        return 1;
    }

    bufinfo->buf = &self->ptr;
    bufinfo->len = sizeof(self->ptr);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}

#define PTR_OBJ(ptr_global) ptr_global ## _obj

#define DEFINE_PTR_OBJ_TYPE(ptr_obj_type, ptr_type_qstr)\
STATIC const mp_obj_type_t ptr_obj_type = {\
    { &mp_type_type },\
    .name = ptr_type_qstr,\
    .buffer_p = { .get_buffer = mp_ptr_get_buffer }\
}

#define DEFINE_PTR_OBJ(ptr_global)\
DEFINE_PTR_OBJ_TYPE(ptr_global ## _type, MP_QSTR_ ## ptr_global);\
STATIC mp_ptr_t PTR_OBJ(ptr_global) = {\
    { &ptr_global ## _type },\
    &ptr_global\
}

#define NEW_PTR_OBJ(name, value)\
({\
    DEFINE_PTR_OBJ_TYPE(ptr_obj_type, MP_QSTR_ ## name);\
    mp_ptr_t *self = m_new_obj(mp_ptr_t);\
    self->base.type = &ptr_obj_type;\
    self->ptr = value;\
    MP_OBJ_FROM_PTR(self);\
})
//////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
// EDMA request
/////////////////////////////////////////////////////////////////////////////////////////////
int initADC_EDMA(
    uint32_t *u32EDMAChanel,
    int8_t  *pi8PCMBufAddr,
    uint32_t u32PCMBufSize,
    nu_edma_cb_handler_t pfnEDMACallback
)
{
	int32_t i32Channel;
	int i32Ret;
	i32Channel = nu_edma_channel_allocate(EDMA_ADC_RX);

	if(i32Channel < 0){
		printf("Unable alloc EDMA channel\n");
		return -1;
	}

	/* Register ISR callback function */
	nu_edma_callback_register(i32Channel, pfnEDMACallback, NULL, NU_PDMA_EVENT_WRA_EMPTY | NU_PDMA_EVENT_WRA_HALF);
	
	i32Ret = nu_edma_transfer(i32Channel, 
								32,
								(uint32_t)(0xB800E010),
								(uint32_t)pi8PCMBufAddr,
								u32PCMBufSize / sizeof(uint32_t),
								eDRVEDMA_WRAPAROUND_EMPTY | eDRVEDMA_WRAPAROUND_HALF); 

	if(i32Ret != 0){
		printf("Unable trigger EDMA channel");
		nu_edma_channel_free(i32Channel);
		return i32Ret;
	}

	*u32EDMAChanel = i32Channel;
	return 0;
}

void releaseADC_EDMA(uint32_t u32EDMAChannel)
{
	nu_edma_channel_terminate(u32EDMAChannel);
	nu_edma_channel_free(u32EDMAChannel);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//Audio record callback
static void AudioRecordSampleDone(void)
{


}

//EDMA callback
static void edmaCallback(void *pvUserData, uint32_t u32Events)
{
   //UINT32 u32Period, u32Attack, u32Recovery, u32Hold;
    if (u32Events & NU_PDMA_EVENT_WRA_EMPTY)
    {
        s_bIsEDMABufferDone = 1;
        //printf("1 edmaCallback\n");
        xSemaphoreGiveFromISR(s_sAinCtrl.tAinISRSem, &xHigherPrioTaskWoken);
        if (xHigherPrioTaskWoken == pdTRUE)
        {
            //SWITCH CONTEXT;
            portEXIT_SWITCHING_ISR(xHigherPrioTaskWoken);
        }
    }
    else if (u32Events & NU_PDMA_EVENT_WRA_HALF)
    {

        s_bIsEDMABufferDone = 2;
        //printf("2 edmaCallback\n");
        xSemaphoreGiveFromISR(s_sAinCtrl.tAinISRSem, &xHigherPrioTaskWoken);
        if (xHigherPrioTaskWoken == pdTRUE)
        {
            //SWITCH CONTEXT
            portEXIT_SWITCHING_ISR(xHigherPrioTaskWoken);
        }
    }
    //sysprintf(" ints = %d,  0x%x\n", u32Count, u32WrapStatus);
}

static int
OpenAinDev(
    E_AUR_MIC_SEL eMicType,
    S_AIN_CTRL *psAinCtrl
)
{
    S_AIN_INFO *psAinInfo;

    psAinInfo = &psAinCtrl->sAinInfo;

    PFN_AUR_CALLBACK pfnOldCallback;
    int32_t i32Idx;

	//Sample rate support list
    uint32_t au32ArraySampleRate[] =
    {
        eAUR_SPS_48000,
        eAUR_SPS_44100, eAUR_SPS_32000, eAUR_SPS_24000, eAUR_SPS_22050,
        eAUR_SPS_16000, eAUR_SPS_12000, eAUR_SPS_11025, eAUR_SPS_8000,
        eAUR_SPS_96000,
        eAUR_SPS_192000
    };

    uint32_t u32MaxSupport = sizeof(au32ArraySampleRate) / sizeof(au32ArraySampleRate[0]);

	//Checking sample rate in support list or not
    for (i32Idx = 0; i32Idx < u32MaxSupport ; i32Idx = i32Idx + 1)
    {
        if (psAinInfo->u32SampleRate == au32ArraySampleRate[i32Idx])
            break;
        if (i32Idx == u32MaxSupport)
        {
            printf("Not in support audio sample rate list\n")  ;
            return -1;
        }
    }

	//Open audio record device
    DrvAUR_Open(eMicType, TRUE);
    DrvAUR_InstallCallback(AudioRecordSampleDone, &pfnOldCallback);

    if (eMicType == eAUR_MONO_MIC_IN)
    {
        DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)0x22, 0x1E);           /* Pregain */
        DrvAUR_AudioI2cWrite((E_AUR_ADC_ADDR)0x23, 0x0E);           /* No any amplifier */
        DrvAUR_DisableInt();
        DrvAUR_SetSampleRate((E_AUR_SPS)au32ArraySampleRate[i32Idx]);
        DrvAUR_AutoGainTiming(1, 1, 1);
        DrvAUR_AutoGainCtrl(TRUE, TRUE, eAUR_OTL_N12P6);
    }
    else if ((eMicType == eAUR_MONO_DIGITAL_MIC_IN) || (eMicType == eAUR_STEREO_DIGITAL_MIC_IN))
    {
        DrvAUR_SetDigiMicGain(TRUE, eAUR_DIGI_MIC_GAIN_P19P2);
        DrvAUR_DisableInt();
        DrvAUR_SetSampleRate((E_AUR_SPS)au32ArraySampleRate[i32Idx]);
    }

    DrvAUR_SetDataOrder(eAUR_ORDER_MONO_16BITS);

    outp32(REG_AR_CON, inp32(REG_AR_CON) | AR_EDMA);
    mp_hal_delay_ms(30);      /* Delay 30ms for OTL stable */

	s_bIsEDMABufferDone = 0;
    return initADC_EDMA(&psAinCtrl->u32EDMAChannel, (int8_t *)psAinCtrl->pu8PCMEDMABuf, psAinCtrl->u32PCMEDMABufSize, edmaCallback);
}

static void CloseAinDev(S_AIN_CTRL *psAinCtrl)
{
	releaseADC_EDMA(psAinCtrl->u32EDMAChannel);
	DrvAUR_Close();
}

//Audio-In task.
static void AinFrameLoop(
    S_AIN_CTRL *psAinCtrl
)
{
    uint32_t u32Temp;

	mp_hal_delay_ms(30);
    //Combine with EDMA, only mode 1 can be set.
    DrvAUR_StartRecord(eAUR_MODE_1);

    while (s_eAIN_ThreadState == eAIN_THREAD_RUN)
    {
 		//Wait audio-in ISR semaphore
		xSemaphoreTake(psAinCtrl->tAinISRSem, portMAX_DELAY);
		//lock PCM index mutex
        xSemaphoreTake(psAinCtrl->tPCMIndexMutex, portMAX_DELAY);

        psAinCtrl->u64LastFrameTimestamp = mp_hal_ticks_ms();

//      NMLOG_INFO("Audio In time stamp %d \n", (uint32_t)psAinCtrl->u64LastFrameTimestamp);
        if (psAinCtrl->u32PCMOutIdx)
        {
            u32Temp = psAinCtrl->u32PCMInIdx - psAinCtrl->u32PCMOutIdx;
            memmove(psAinCtrl->pu8PCMBuf, psAinCtrl->pu8PCMBuf + psAinCtrl->u32PCMOutIdx, u32Temp);
            psAinCtrl->u32PCMInIdx = psAinCtrl->u32PCMInIdx - psAinCtrl->u32PCMOutIdx;
            psAinCtrl->u32PCMOutIdx = 0;
        }

        if ((psAinCtrl->u32PCMInIdx + psAinCtrl->u32PCMFragBufSize) >= psAinCtrl->u32PCMBufSize)
        {
            //Cut oldest audio data, if PCM buffer data over buffer size
            memmove(psAinCtrl->pu8PCMBuf, psAinCtrl->pu8PCMBuf + psAinCtrl->u32PCMFragBufSize, psAinCtrl->u32PCMFragBufSize);
            psAinCtrl->u32PCMInIdx = psAinCtrl->u32PCMInIdx - psAinCtrl->u32PCMFragBufSize;
        }

        if (s_bIsEDMABufferDone == 1)
        {
            memcpy(psAinCtrl->pu8PCMBuf + psAinCtrl->u32PCMInIdx, (int8_t *)((int32_t)(psAinCtrl->pu8PCMEDMABuf + psAinCtrl->u32PCMFragBufSize) | E_NONCACHE_BIT), psAinCtrl->u32PCMFragBufSize);
        }
        else if (s_bIsEDMABufferDone == 2)
        {
            memcpy(psAinCtrl->pu8PCMBuf + psAinCtrl->u32PCMInIdx, (int8_t *)((int32_t)(psAinCtrl->pu8PCMEDMABuf) | E_NONCACHE_BIT), psAinCtrl->u32PCMFragBufSize);
        }

		if(g_pcmbuf){
			g_pcmbuf->len = psAinCtrl->u32PCMFragBufSize;
			g_pcmbuf->items = psAinCtrl->pu8PCMBuf + psAinCtrl->u32PCMInIdx;
		}

		if(g_audio_callback != mp_const_none){
			mp_call_function_1(g_audio_callback, MP_OBJ_FROM_PTR(g_pcmbuf));
		}

        psAinCtrl->u32PCMInIdx = psAinCtrl->u32PCMInIdx + psAinCtrl->u32PCMFragBufSize;
        s_bIsEDMABufferDone = 0;

		//unlock PCM index mutex
        xSemaphoreGive(psAinCtrl->tPCMIndexMutex);
    }

	DrvAUR_StopRecord();
}

static void AudioIn_TaskCreate(
    void *pvParameters
)
{
	s_eAIN_ThreadState = eAIN_THREAD_RUN;
    AinFrameLoop(&s_sAinCtrl);
	s_eAIN_ThreadState = eAIN_THREAD_STOP;
    vTaskDelete(NULL);
}

////////////////////////////////////////////////////////////////////////////////////
// micropython binding
/////////////////////////////////////////////////////////////////////////////////////
static mp_obj_t py_audioin_init(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	uint32_t u32FragDuration;
    S_AIN_CTRL *psAinCtrl = &s_sAinCtrl;
	S_AIN_INFO *psAinInfo = &psAinCtrl->sAinInfo;
 
	enum{
		ARG_Channel,
		ARG_Freq,
	};

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_channels, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 2}},
        { MP_QSTR_frequency, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 16000}},
    };

	mp_arg_val_t args_val[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args_val);

	psAinInfo->u32Channel = args_val[ARG_Channel].u_int;
	psAinInfo->u32SampleRate = args_val[ARG_Freq].u_int;

    u32FragDuration = AIN_CONFIG_FRAGMENT_DURATION;

    psAinCtrl->u32PCMFragBufSize = ((psAinInfo->u32SampleRate * u32FragDuration) / 1000 * psAinInfo->u32Channel * 2);
    psAinCtrl->u32PCMEDMABufSize = psAinCtrl->u32PCMFragBufSize * 2;
    psAinCtrl->u32PCMBufSize = psAinInfo->u32SampleRate * psAinInfo->u32Channel * 2;
	
    if(psAinCtrl->tAinISRSem == NULL) {
		psAinCtrl->tAinISRSem = xSemaphoreCreateBinary();

		if (psAinCtrl->tAinISRSem == NULL)
		{
			RAISE_OS_EXCEPTION("unable create AudioIn semaphore");
		}
	}

	if(psAinCtrl->tPCMIndexMutex == NULL)
	{
		psAinCtrl->tPCMIndexMutex = xSemaphoreCreateMutex();
		if (psAinCtrl->tPCMIndexMutex == NULL)
		{
			RAISE_OS_EXCEPTION("unable create AudioIn mutex");
		}
	}

    psAinCtrl->u32PCMInIdx = 0;
    psAinCtrl->u32PCMOutIdx = 0;

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_audioin_init_obj, 0, py_audioin_init);

static mp_obj_t py_audioin_start_streaming(mp_obj_t callback_obj)
{
    S_AIN_CTRL *psAinCtrl = &s_sAinCtrl;
	S_AIN_INFO *psAinInfo = &psAinCtrl->sAinInfo;
	int i32Ret = 0;

	//TODO: if callback_obj is null, we will use ptr obj for application to receive audio data
    g_audio_callback = callback_obj;

    if (!mp_obj_is_callable(g_audio_callback)) {
        g_audio_callback = mp_const_none;
		i32Ret = -1;
		goto statrt_stream_fail;
    }

    psAinCtrl->pu8PCMEDMABuf = malloc(psAinCtrl->u32PCMEDMABufSize + 4096);
    if (psAinCtrl->pu8PCMEDMABuf == NULL){
		i32Ret = -2;
		goto statrt_stream_fail;
    }
	
    psAinCtrl->pu8PCMBuf = malloc(psAinCtrl->u32PCMBufSize);
    if (psAinCtrl->pu8PCMBuf == NULL)
    {
		i32Ret = -3;
		goto statrt_stream_fail;
    }

    // Allocate global PCM buffer object
    g_pcmbuf = mp_obj_new_bytearray_by_ref(
            psAinCtrl->u32PCMFragBufSize,
            psAinCtrl->pu8PCMEDMABuf);

    psAinCtrl->u32PCMInIdx = 0;
    psAinCtrl->u32PCMOutIdx = 0;

	//Open AudioIn devcie
    if(OpenAinDev(eAUR_MONO_MIC_IN, psAinCtrl) < 0){
		i32Ret = -4;
		goto statrt_stream_fail;
	}

	//Create thread to receive audio dat
	xTaskCreate(AudioIn_TaskCreate, "AudioIn", configMINIMAL_STACK_SIZE, NULL, AIN_TASK_PRIORITY, NULL);

	return mp_const_none;

statrt_stream_fail:
	g_audio_callback = mp_const_none;

	if(g_pcmbuf){
		m_del_obj(mp_obj_array_t, g_pcmbuf);
		g_pcmbuf = NULL;
	}

	if(psAinCtrl->pu8PCMEDMABuf){
		free(psAinCtrl->pu8PCMEDMABuf);
		psAinCtrl->pu8PCMEDMABuf = NULL;
	}

	if(psAinCtrl->pu8PCMBuf){
		free(psAinCtrl->pu8PCMBuf);
		psAinCtrl->pu8PCMBuf = NULL;
	}

	if(i32Ret == -1){
        RAISE_OS_EXCEPTION("Invalid callback object!");
	}
	else if(i32Ret == -2){
        RAISE_OS_EXCEPTION("Unable allocate audio pdma buffer");
	}
	else if(i32Ret == -3){
        RAISE_OS_EXCEPTION("Unable allocate audio pcm buffer");
	}
	else if(i32Ret == -4){
        RAISE_OS_EXCEPTION("Unable open AudioIn device");
	}

	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_audioin_start_streaming_obj, py_audioin_start_streaming);

static mp_obj_t py_audioin_stop_streaming()
{
    S_AIN_CTRL *psAinCtrl = &s_sAinCtrl;

	if(s_eAIN_ThreadState == eAIN_THREAD_RUN){
		s_eAIN_ThreadState = eAIN_THREAD_TO_STOP;
		
		while(s_eAIN_ThreadState != eAIN_THREAD_STOP){
			mp_hal_delay_ms(10);
		}
	}

	releaseADC_EDMA(psAinCtrl->u32EDMAChannel);
	DrvAUR_Close();

	if(psAinCtrl->pu8PCMEDMABuf){
		free(psAinCtrl->pu8PCMEDMABuf);
		psAinCtrl->pu8PCMEDMABuf = NULL;
	}

	if(psAinCtrl->pu8PCMBuf){
		free(psAinCtrl->pu8PCMBuf);
		psAinCtrl->pu8PCMBuf = NULL;
	}

	m_del_obj(mp_obj_array_t, g_pcmbuf);
	g_pcmbuf = NULL;
	g_audio_callback = mp_const_none;

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_audioin_stop_streaming_obj, py_audioin_stop_streaming);

bool
py_audioin_read_frame(
    uint32_t u32ReadSamples,
    uint8_t *pu8FrameData,
    uint64_t *pu64FrameTime
)
{
    uint32_t u32SamplesInBuf;
    S_AIN_INFO *psAinInfo;
    uint32_t u32ReadSize;

    psAinInfo = &s_sAinCtrl.sAinInfo;
    u32ReadSize = u32ReadSamples * psAinInfo->u32Channel * 2;

    xSemaphoreTake(s_sAinCtrl.tPCMIndexMutex, portMAX_DELAY);

    u32SamplesInBuf = (s_sAinCtrl.u32PCMInIdx - s_sAinCtrl.u32PCMOutIdx) / psAinInfo->u32Channel / 2;

    if (u32SamplesInBuf < u32ReadSamples)
    {
        xSemaphoreGive(s_sAinCtrl.tPCMIndexMutex);
        return FALSE;
    }

    memcpy(pu8FrameData, s_sAinCtrl.pu8PCMBuf + s_sAinCtrl.u32PCMOutIdx, u32ReadSize);
    s_sAinCtrl.u32PCMOutIdx += u32ReadSize;
//  printf("s_sAinCtrl.u64LastFrameTimestamp %d \n", (uint32_t)s_sAinCtrl.u64LastFrameTimestamp);
    *pu64FrameTime = s_sAinCtrl.u64LastFrameTimestamp - ((1000 * u32SamplesInBuf) / psAinInfo->u32SampleRate);

    xSemaphoreGive(s_sAinCtrl.tPCMIndexMutex);
    return TRUE;
}
DEFINE_PTR_OBJ(py_audioin_read_frame);


static const mp_rom_map_elem_t AudioIn_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_ROM_QSTR(MP_QSTR_AudioIn)               },
    { MP_ROM_QSTR(MP_QSTR_init),            MP_ROM_PTR(&py_audioin_init_obj)           },
    { MP_ROM_QSTR(MP_QSTR_start_streaming), MP_ROM_PTR(&py_audioin_start_streaming_obj)},
    { MP_ROM_QSTR(MP_QSTR_stop_streaming),  MP_ROM_PTR(&py_audioin_stop_streaming_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_frame),		MP_ROM_PTR(&PTR_OBJ(py_audioin_read_frame)) },
};

STATIC MP_DEFINE_CONST_DICT(AudioIn_module_globals, AudioIn_globals_table);

const mp_obj_module_t mp_module_AudioIn = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&AudioIn_module_globals,
};

#endif
