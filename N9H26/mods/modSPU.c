//////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/binary.h"
#include "py/objarray.h"

#if MICROPY_HW_ENABLE_SPU
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "wblib.h"
#include "N9H26_SPU.h"

#define PCM_TASK_PRIORITY        (configMAX_PRIORITIES - 1)
#define RAISE_OS_EXCEPTION(msg)     nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, msg))

typedef struct
{
    uint8_t     *pu8Buf;
    uint32_t    u32Size;
    uint32_t    u32WriteIdx;
    uint32_t    u32ReadIdx;
    SemaphoreHandle_t tBufMutex;
    volatile BOOL bAccessISR;
} S_PCM_BUF_MGR;

typedef enum
{
	ePUNCHER_THREAD_RUN,
	ePUNCHER_THREAD_TO_STOP,
	ePUNCHER_THREAD_STOP,	
}E_PUNCHER_THREAD_SATAE;

typedef struct
{
	uint32_t u32Channel;
	uint32_t u32SampleRate;
	uint32_t u32FragmentSize;
	int32_t i32Volume;
}S_SPU_CTRL;

static uint32_t s_u32EmptyAudioCount = 0;
static uint64_t s_u64EmptyAudioTotalTime = 0;
static uint64_t s_u64EmptyAudioStartTime = 0;
static xTaskHandle s_hPunckerTaskHandle = 0;

static uint8_t *s_pu8ISRPcmBuf;
static SemaphoreHandle_t s_tPCMPuncherSem = NULL;

static mp_obj_t g_pcm_callback = mp_const_none;
static mp_obj_array_t *g_pcmbuf = NULL;

static BOOL s_bPAEnabled = FALSE;

static volatile E_PUNCHER_THREAD_SATAE s_ePuncherThreadState;

static S_SPU_CTRL s_sSPUCtrl =
{
	.u32Channel = 2,
	.u32FragmentSize = 1024,
	.u32SampleRate = 16000,
	.i32Volume = 70,
};

static S_PCM_BUF_MGR s_sPCMBufMgr =
{
	.pu8Buf = NULL,
	.u32Size = 0,
	.u32WriteIdx = 0,
	.u32ReadIdx = 0,
	.tBufMutex = NULL,
	.bAccessISR = FALSE,
};

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

//Called by SPU end address and threshold address inturrept. Copy half fragment PCM data to pu8Data
static int
DACIsrCallback(
    uint8_t *pu8Data
)
{
    BaseType_t xHigherPriorityTaskWoken;
    int32_t i32HalfFrag = s_sSPUCtrl.u32FragmentSize / 2;

    if (s_pu8ISRPcmBuf == NULL)
    {
        memset(pu8Data, 0, i32HalfFrag);
        return 0;
    }

    memcpy(pu8Data, s_pu8ISRPcmBuf, i32HalfFrag);
    memset(s_pu8ISRPcmBuf, 0, i32HalfFrag);

    xSemaphoreGiveFromISR(s_tPCMPuncherSem, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        portEXIT_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }

    return 0;
}

static uint32_t 
GetFragmentSize(
    int i32SampleRate,
    int i32Channel
)
{
    uint32_t u32FragmentSize = 0;

	//SPU fragment size ~250ms 
    u32FragmentSize = i32SampleRate / 4 * i32Channel;
	//SPU fragment size alignment to 4096
    u32FragmentSize = u32FragmentSize & (~(4096 - 1));
    if (u32FragmentSize == 0)
        u32FragmentSize = 4096;

	//Set SPU fragment size
    spuIoctl(SPU_IOCTL_SET_FRAG_SIZE, (uint32_t) &u32FragmentSize, 0);

    spuIoctl(SPU_IOCTL_GET_FRAG_SIZE, (uint32_t) &u32FragmentSize, 0);

	return u32FragmentSize;
}

static void
OpenDAC(
    int i32SampleRate,
    int i32Channel,
    uint32_t u32FragmentSize
)
{
    outp32(REG_AHBCLK, inp32(REG_AHBCLK) | ADO_CKE | SPU_CKE | HCLK4_CKE);
    spuOpen(i32SampleRate);
    if (i32Channel == 1)
    {
        spuIoctl(SPU_IOCTL_SET_MONO, 0, 0);
    }
    else
    {
        spuIoctl(SPU_IOCTL_SET_STEREO, 0,0);
    }

	//Set SPU fragment size
    spuIoctl(SPU_IOCTL_SET_FRAG_SIZE, (uint32_t) &u32FragmentSize, 0);

    s_u32EmptyAudioCount = 0;
    s_u64EmptyAudioTotalTime = 0;
    s_u64EmptyAudioStartTime = 0;
}

static void
CloseDAC(void)
{
    s_sSPUCtrl.i32Volume = 0;
    spuIoctl(SPU_IOCTL_SET_VOLUME, s_sSPUCtrl.i32Volume, s_sSPUCtrl.i32Volume);

#if defined(MICROPY_HW_SPU_PA)
//	mp_hal_pin_low(MICROPY_HW_SPU_PA);	//Set losw
//	s_bPAEnabled = FALSE;
#endif

    spuStopPlay();
    spuClose();
}

static int
StartPlayDAC(
    int32_t i32Volume
)
{
    uint8_t *pu8SilentPCM;

    pu8SilentPCM = malloc(s_sSPUCtrl.u32FragmentSize);

    if (!pu8SilentPCM)
        return -1;

    spuDacOn(2);

    vTaskDelay(1 / portTICK_RATE_MS);
    spuSetDacSlaveMode();

    memset(pu8SilentPCM, 0x0, s_sSPUCtrl.u32FragmentSize);

	//SPU enable and pass a silent PCM data to SPU
    spuStartPlay(DACIsrCallback, pu8SilentPCM);

#if defined(MICROPY_HW_SPU_PA)
    //Set multi function pin
    if (s_bPAEnabled == FALSE)
    {
		mp_hal_pin_config(MICROPY_HW_SPU_PA, GPIO_MODE_OUTPUT, 0);
		mp_hal_pin_high(MICROPY_HW_SPU_PA);	//Set losw

        s_bPAEnabled = TRUE;
    }
#endif

    free(pu8SilentPCM);

    return 0;
}

static void
PCMBuf_Send(
    uint8_t *pu8DataBuf,
    uint32_t *pu8SendLen
)
{
    uint32_t u32SendLen = *pu8SendLen;
    S_PCM_BUF_MGR *psPCMBufMgr = &s_sPCMBufMgr;
    int32_t i32HalfFrag = s_sSPUCtrl.u32FragmentSize / 2;
    uint32_t u32CopyLen;
	uint32_t u32TotalSend = 0;

    //Copy data to PCM buffer. lock the buffer

	//printf("Send len %d, half frame %d  \n", u32SendLen,i32HalfFrag );
    while (u32SendLen >= i32HalfFrag)
    {

        if (psPCMBufMgr->u32ReadIdx)
        {
            //move PCM buffer data to the front end
            memmove(psPCMBufMgr->pu8Buf, psPCMBufMgr->pu8Buf + psPCMBufMgr->u32ReadIdx, psPCMBufMgr->u32WriteIdx -  psPCMBufMgr->u32ReadIdx);
            psPCMBufMgr->u32WriteIdx = (psPCMBufMgr->u32WriteIdx - psPCMBufMgr->u32ReadIdx);
            psPCMBufMgr->u32ReadIdx = 0;
        }

        u32CopyLen = i32HalfFrag;

        if ((psPCMBufMgr->u32WriteIdx + u32CopyLen) <= psPCMBufMgr->u32Size)
        {
#if 1
//          INFO("Copy data to PCM buffer  \n");
            memcpy(psPCMBufMgr->pu8Buf + psPCMBufMgr->u32WriteIdx, pu8DataBuf, u32CopyLen);
#else
            int i;
            uint8_t *pu8PCMData_0;
            uint8_t *pu8PCMData_1;

            for (i = 0; i < u32CopyLen; i = i + 2)
            {
                pu8PCMData_0 = psPCMBufMgr->pu8Buf + psPCMBufMgr->u32WriteIdx + i;
                pu8PCMData_1 = psPCMBufMgr->pu8Buf + psPCMBufMgr->u32WriteIdx + i + 1;
                *pu8PCMData_0 = pu8RecvBuf[i + 1];
                *pu8PCMData_1 = pu8RecvBuf[i];
            }
#endif
            psPCMBufMgr->u32WriteIdx += u32CopyLen;
        }
        else
        {
			printf("PCM playback buffer full. Discard audio packet \n");
            memcpy(psPCMBufMgr->pu8Buf, pu8DataBuf, u32CopyLen);
            psPCMBufMgr->u32WriteIdx = u32CopyLen;
            psPCMBufMgr->u32ReadIdx = 0;
        }

        memmove(pu8DataBuf, pu8DataBuf + u32CopyLen, u32SendLen -  u32CopyLen);
        u32SendLen -= u32CopyLen;
        u32TotalSend += u32CopyLen;
    }

    //unlock the buffer
    *pu8SendLen = u32TotalSend;
}

//Punch PCM data to PCM ISR buffer
static void PCMPuncherTask(
    void *pvArg
)
{
    S_PCM_BUF_MGR *psPCMBufMgr = (S_PCM_BUF_MGR *) pvArg;
    int32_t i32HalfFrag = s_sSPUCtrl.u32FragmentSize / 2;
    BOOL bEmpty = FALSE;
    mp_buffer_info_t sBufinfo;

	printf("SPU PCMPuncherTask start \n");

    spuIoctl(SPU_IOCTL_SET_VOLUME, s_sSPUCtrl.i32Volume, s_sSPUCtrl.i32Volume);
    while (s_ePuncherThreadState == ePUNCHER_THREAD_RUN)
    {
		//Wait DAC ISR semphore
        if (xSemaphoreTake(s_tPCMPuncherSem, portMAX_DELAY) == pdFALSE)
        {
            break;
        }

        xSemaphoreTake(psPCMBufMgr->tBufMutex, portMAX_DELAY);

		if(g_pcm_callback != mp_const_none){
			mp_call_function_1(g_pcm_callback, MP_OBJ_FROM_PTR(g_pcmbuf));

			if (mp_get_buffer(g_pcmbuf, &sBufinfo, MP_BUFFER_READ)) {
				if(sBufinfo.len > 0){
					PCMBuf_Send(sBufinfo.buf, (uint32_t *)&sBufinfo.len);
				}
			}
		}

        if (bEmpty == TRUE)
        {
            if ((psPCMBufMgr->u32WriteIdx - psPCMBufMgr->u32ReadIdx) < i32HalfFrag)
            {
				//PCM data still not enough
                xSemaphoreGive(psPCMBufMgr->tBufMutex);
                vTaskDelay(5 / portTICK_RATE_MS);
                continue;
            }
            else
            {
				//Calcute PCM buffer empty status
                uint32_t u32AverageEmptyTime;

                spuIoctl(SPU_IOCTL_SET_VOLUME, s_sSPUCtrl.i32Volume, s_sSPUCtrl.i32Volume);
                s_u32EmptyAudioCount ++;
                s_u64EmptyAudioTotalTime += mp_hal_ticks_ms() - s_u64EmptyAudioStartTime;
                u32AverageEmptyTime = s_u64EmptyAudioTotalTime / s_u32EmptyAudioCount;
				printf("AAAAAAAAAAAAAA Audio empty count %ld, average time %ld \n", s_u32EmptyAudioCount, u32AverageEmptyTime);
            }
        }
        else
        {
			//Detect PCM data empty or not
            if ((psPCMBufMgr->u32WriteIdx - psPCMBufMgr->u32ReadIdx) < i32HalfFrag)
            {
                xSemaphoreGive(psPCMBufMgr->tBufMutex);
                printf("PCM buffer is empty\n");
                bEmpty = TRUE;
                s_u64EmptyAudioStartTime = mp_hal_ticks_ms();
                spuIoctl(SPU_IOCTL_SET_VOLUME, 0, 0);
                vTaskDelay(5 / portTICK_RATE_MS);
                continue;
            }
        }

		//PCM data is enough to put PCM ISR buffer
        bEmpty = FALSE;
        memcpy(s_pu8ISRPcmBuf, psPCMBufMgr->pu8Buf + psPCMBufMgr->u32ReadIdx, i32HalfFrag);
        psPCMBufMgr->u32ReadIdx += i32HalfFrag;

        xSemaphoreGive(psPCMBufMgr->tBufMutex);

    }

	printf("SPU PCMPuncherTask stop \n");
	s_ePuncherThreadState = ePUNCHER_THREAD_STOP;
    vTaskDelete(NULL);
}

////////////////////////////////////////////////////////////////////////////////////
// micropython binding
/////////////////////////////////////////////////////////////////////////////////////
static mp_obj_t py_SPU_init(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
	enum{
		ARG_Channel,
		ARG_Freq,
		ARG_Volume,
	};

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_channels, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 2}},
        { MP_QSTR_frequency, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 16000}},
        { MP_QSTR_volume, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 70}},
    };

	mp_arg_val_t args_val[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args_val);
	
	s_sSPUCtrl.u32Channel = args_val[ARG_Channel].u_int;
	s_sSPUCtrl.u32SampleRate = args_val[ARG_Freq].u_int;
	s_sSPUCtrl.i32Volume = args_val[ARG_Volume].u_int;

	//Create puncher semaphore
	if(s_tPCMPuncherSem == NULL){
		s_tPCMPuncherSem = xSemaphoreCreateBinary();
		
		if (s_tPCMPuncherSem == NULL){
			RAISE_OS_EXCEPTION("unable create SPU ISR semaphore");
		}
	}

	if(s_sPCMBufMgr.tBufMutex == NULL){
	    s_sPCMBufMgr.tBufMutex = xSemaphoreCreateMutex();
		
		if(s_sPCMBufMgr.tBufMutex == NULL){
			RAISE_OS_EXCEPTION("unable create SPU PCM mutex");
		}
	}

//	s_sSPUCtrl.u32FragmentSize = OpenDAC(s_sSPUCtrl.u32SampleRate, s_sSPUCtrl.u32Channel);
	s_sSPUCtrl.u32FragmentSize = GetFragmentSize(s_sSPUCtrl.u32SampleRate, s_sSPUCtrl.u32Channel);

	//Allocate PCM buffer for PCM buffer manage
    s_sPCMBufMgr.u32Size = s_sSPUCtrl.u32FragmentSize  * 4;
    s_sPCMBufMgr.u32WriteIdx = 0;
    s_sPCMBufMgr.u32ReadIdx = 0;
	
	return MP_OBJ_NEW_SMALL_INT(s_sSPUCtrl.u32FragmentSize);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_SPU_init_obj, 0, py_SPU_init);

static mp_obj_t py_spu_start_play(mp_obj_t callback_obj)
{
	int i32Ret;
	
	//If callback_obj is null, we will use ptr obj for application to receive audio data
    g_pcm_callback = callback_obj;

    if ((g_pcm_callback !=mp_const_none) && (!mp_obj_is_callable(g_pcm_callback))) {
        g_pcm_callback = mp_const_none;
		i32Ret = -1;
		goto spu_start_play_fail;
    }

    //Allocate ISR PCM buffer
    s_pu8ISRPcmBuf = malloc(s_sSPUCtrl.u32FragmentSize / 2);
    if (s_pu8ISRPcmBuf == NULL)
    {
        i32Ret = -2;
        goto spu_start_play_fail;
    }

    memset(s_pu8ISRPcmBuf, 0, (s_sSPUCtrl.u32FragmentSize / 2));

	//Allocate mgr buffer
	s_sPCMBufMgr.pu8Buf = malloc(s_sPCMBufMgr.u32Size);
	if (s_sPCMBufMgr.pu8Buf == NULL)
	{
		i32Ret = -3;
		goto spu_start_play_fail;
	}

	OpenDAC(s_sSPUCtrl.u32SampleRate, s_sSPUCtrl.u32Channel, s_sSPUCtrl.u32FragmentSize);

    // Allocate global PCM buffer object
	g_pcmbuf = mp_obj_new_bytearray_by_ref(
			s_sSPUCtrl.u32FragmentSize,
			m_new(int8_t, s_sSPUCtrl.u32FragmentSize));

	s_sPCMBufMgr.u32WriteIdx = 0;
	s_sPCMBufMgr.u32ReadIdx = 0;

	//Create PCM task
	s_ePuncherThreadState = ePUNCHER_THREAD_RUN;
	xTaskCreate(PCMPuncherTask, "pcm_puncher", configMINIMAL_STACK_SIZE, &s_sPCMBufMgr, PCM_TASK_PRIORITY,  &s_hPunckerTaskHandle);

	if (s_hPunckerTaskHandle == NULL)
	{
		i32Ret = -4;
		goto spu_start_play_fail;
	}

	StartPlayDAC(s_sSPUCtrl.i32Volume);

	return mp_const_none;

spu_start_play_fail:

	g_pcm_callback = mp_const_none;

	if(g_pcmbuf){
		if(g_pcmbuf->items){
			m_del(int8_t, g_pcmbuf->items, g_pcmbuf->len);
		}

		m_del_obj(mp_obj_array_t, g_pcmbuf);
		g_pcmbuf = NULL;
	}

	if(s_sPCMBufMgr.pu8Buf){
		free(s_sPCMBufMgr.pu8Buf);
		s_sPCMBufMgr.pu8Buf = NULL;
	}
	
	if(s_pu8ISRPcmBuf){
		free(s_pu8ISRPcmBuf);
		s_pu8ISRPcmBuf = NULL;
	}
	
	if(i32Ret == -1){
        RAISE_OS_EXCEPTION("Invalid callback object!");
	}
	else if(i32Ret == -2){
        RAISE_OS_EXCEPTION("Unable allocate SPU ISR PCM buffer");
	}
	else if(i32Ret == -3){
        RAISE_OS_EXCEPTION("Unable allocate SPU PCM buffer");
	}
	else if(i32Ret == -4){
        RAISE_OS_EXCEPTION("Unable create PCM punckeher");
	}

	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_spu_start_play_obj, py_spu_start_play);

static mp_obj_t py_spu_stop_play()
{
	if (s_hPunckerTaskHandle != NULL){
		s_ePuncherThreadState = ePUNCHER_THREAD_TO_STOP;
		
		while(s_ePuncherThreadState != ePUNCHER_THREAD_STOP){
			mp_hal_delay_ms(10);
		}		
		s_hPunckerTaskHandle = NULL;
	}

    CloseDAC();

	g_pcm_callback = mp_const_none;

	if(g_pcmbuf){
		if(g_pcmbuf->items){
			m_del(int8_t, g_pcmbuf->items, g_pcmbuf->len);
		}

		m_del_obj(mp_obj_array_t, g_pcmbuf);
		g_pcmbuf = NULL;
	}

	if(s_sPCMBufMgr.pu8Buf){
		free(s_sPCMBufMgr.pu8Buf);
		s_sPCMBufMgr.pu8Buf = NULL;
	}
	
	if(s_pu8ISRPcmBuf){
		free(s_pu8ISRPcmBuf);
		s_pu8ISRPcmBuf = NULL;
	}

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_spu_stop_play_obj, py_spu_stop_play);

void
py_spu_write_frame(
    uint8_t *pu8DataBuf,
    uint32_t *pu8SendLen
)
{
    S_PCM_BUF_MGR *psPCMBufMgr = &s_sPCMBufMgr;

    //Copy data to PCM buffer. lock the buffer
    xSemaphoreTake(psPCMBufMgr->tBufMutex, portMAX_DELAY);

	PCMBuf_Send(pu8DataBuf, (uint32_t *)pu8SendLen);

    //unlock the buffer
    xSemaphoreGive(psPCMBufMgr->tBufMutex);
}
DEFINE_PTR_OBJ(py_spu_write_frame);

static mp_obj_t py_spu_put_frame(mp_obj_t frame_obj)
{
    mp_buffer_info_t sBufinfo;
    S_PCM_BUF_MGR *psPCMBufMgr = &s_sPCMBufMgr;
	
	if(frame_obj == mp_const_none)
		return mp_const_none;

	if (mp_get_buffer(frame_obj, &sBufinfo, MP_BUFFER_READ)) {
		if(sBufinfo.len <= 0){
			return mp_const_none;
		}
	}

#if 1
    //Copy data to PCM buffer. lock the buffer
    xSemaphoreTake(psPCMBufMgr->tBufMutex, portMAX_DELAY);

	PCMBuf_Send(sBufinfo.buf, (uint32_t *)&sBufinfo.len);

    //unlock the buffer
    xSemaphoreGive(psPCMBufMgr->tBufMutex);

	return MP_OBJ_NEW_SMALL_INT(sBufinfo.len);
#else
	return MP_OBJ_NEW_SMALL_INT(sBufinfo.len);
#endif
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_spu_put_frame_obj, py_spu_put_frame);

static const mp_rom_map_elem_t SPU_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_ROM_QSTR(MP_QSTR_SPU)               },
    { MP_ROM_QSTR(MP_QSTR_init),            MP_ROM_PTR(&py_SPU_init_obj)           },
    { MP_ROM_QSTR(MP_QSTR_start_play), 		MP_ROM_PTR(&py_spu_start_play_obj)},
    { MP_ROM_QSTR(MP_QSTR_stop_play),  		MP_ROM_PTR(&py_spu_stop_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_put_frame),  		MP_ROM_PTR(&py_spu_put_frame_obj) },

    { MP_ROM_QSTR(MP_QSTR_write_frame),		MP_ROM_PTR(&PTR_OBJ(py_spu_write_frame)) },
};

STATIC MP_DEFINE_CONST_DICT(SPU_module_globals, SPU_globals_table);

const mp_obj_module_t mp_module_SPU = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&SPU_module_globals,
};

#endif
