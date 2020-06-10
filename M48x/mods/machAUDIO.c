/**************************************************************************//**
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of Nuvoton Technology Corp. nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Change Logs:
* Date            Author           Notes
* 2020-5-1        CHChen        First version
*
******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/mpthread.h"
#include "py/binary.h"

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "hal/M48x_I2C.h"
#include "hal/M48x_I2S.h"
#include "hal/NAU88L25.h"

#include "machAUDIO.h"
//libMAD includes
#include "mad.h"

//shine includes
#include "shine/src/lib/layer3.h"


#if MICROPY_PY_MACHINE_AUDIO

#define PCM_BUFFER_SIZE        2304		//MP3 sample per block size is 384, 576, 1152
#define FILE_IO_BUFFER_SIZE    4096		//File IO buffer for MP3 file read


#define MP3_TASK_PRIORITY        (configMAX_PRIORITIES - 1)
#define MP3_TASK_STACK_SIZE      (8 * 1024)
#define MP3_TASK_STACK_LEN       (MP3_TASK_STACK_SIZE / sizeof(StackType_t))

STATIC StaticTask_t mp3task_tcb;
STATIC StackType_t mp3task_stack[MP3_TASK_STACK_LEN] __attribute__((aligned (8)));

typedef enum{
	eAUDIO_STATUS_STOP,
	eAUDIO_STATUS_TO_RECORD,
	eAUDIO_STATUS_RECORDING,
	eAUDIO_STATUS_TO_PLAY,
	eAUDIO_STATUS_PLAYING,
	eAUDIO_STATUS_TO_STOP,
}E_AUDIO_STATUS;

struct mp3Header
{
    unsigned int sync : 11;
    unsigned int version : 2;
    unsigned int layer : 2;
    unsigned int protect : 1;
    unsigned int bitrate : 4;
    unsigned int samfreq : 2;
    unsigned int padding : 1;
    unsigned int private : 1;
    unsigned int channel : 2;
    unsigned int mode : 2;
    unsigned int copy : 1;
    unsigned int original : 1;
    unsigned int emphasis : 2;
};

struct AudioInfoObject
{
	unsigned int FileSize;
//	unsigned int mp3FileEndFlag;
	unsigned int SampleRate;
	unsigned int BitRate;
	unsigned int Channel;
	unsigned int PlayTime;
//	unsigned int Playing;
};


typedef struct{
    mp_obj_base_t base;
	E_AUDIO_STATUS eStatus;
    bool initialized;
	I2C_T *i2c_base;
	i2s_t i2sObj;
	FATFS *fs;

	char *szFile;
	signed int *pi32PCMBuff;
	struct AudioInfoObject audioInfo;
	uint8_t au8PCMBuffer_Full[2];

//for MP3 playback
	unsigned char *pu8MADInputBuff;
	uint8_t u8PCMBufferPlayIdx;

//for MP3 record
	uint8_t u8PCMBufferRecIdx;
	uint32_t u32StorFreeSizeKB;

}mach_audio_obj_t;


STATIC mach_audio_obj_t mach_audio_obj = {
	.base = {&mach_audio_type},
	.eStatus = eAUDIO_STATUS_STOP,
	.initialized = FALSE
};


STATIC int32_t
acodec_open(
	mach_audio_obj_t *self
)
{
	//check i2c pin
	const pin_af_obj_t *af_scl = pin_find_af_by_fn_type(MICROPY_NAU88L25_I2C_SCL, AF_FN_I2C, AF_PIN_TYPE_I2C_SCL);
	const pin_af_obj_t *af_sda = pin_find_af_by_fn_type(MICROPY_NAU88L25_I2C_SDA, AF_FN_I2C, AF_PIN_TYPE_I2C_SDA);

	//check i2s pin
	const pin_af_obj_t *af_i2s_lrck = pin_find_af_by_fn_type(MICROPY_NAU88L25_I2S_LRCK, AF_FN_I2S, AF_PIN_TYPE_I2S_LRCK);
	const pin_af_obj_t *af_i2s_do = pin_find_af_by_fn_type(MICROPY_NAU88L25_I2S_DO, AF_FN_I2S, AF_PIN_TYPE_I2S_DO);
	const pin_af_obj_t *af_i2s_di = pin_find_af_by_fn_type(MICROPY_NAU88L25_I2S_DI, AF_FN_I2S, AF_PIN_TYPE_I2S_DI);
	const pin_af_obj_t *af_i2s_mclk = pin_find_af_by_fn_type(MICROPY_NAU88L25_I2S_MCLK, AF_FN_I2S, AF_PIN_TYPE_I2S_MCLK);
	const pin_af_obj_t *af_i2s_bclk = pin_find_af_by_fn_type(MICROPY_NAU88L25_I2S_BCLK, AF_FN_I2S, AF_PIN_TYPE_I2S_BCLK);

	if((af_scl == NULL) || (af_sda == NULL)){
       nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Invalid I2C SCL and SDA pin"));
	}

	if((af_i2s_lrck == NULL) || (af_i2s_do == NULL) || (af_i2s_di == NULL) || (af_i2s_mclk == NULL) || (af_i2s_bclk == NULL)){
       nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Invalid I2S pins"));
	}


	self->i2c_base = (I2C_T *)af_scl->reg;
	self->i2sObj.i2s = (I2S_T *)af_i2s_lrck->reg;

	int i2c_unit;
	int i2s_unit;

    if (self->i2c_base == I2C0) {
        i2c_unit = 0;
    } else if (self->i2c_base == I2C1) {
        i2c_unit = 1;
    } else if (self->i2c_base == I2C2) {
        i2c_unit = 2;
    } else {
        // I2C does not exist for this board (shouldn't get here, should be checked by caller)
        printf("machAUDIO chekc i2c pin select failed\n");
        return -1;
    }

    if (self->i2sObj.i2s == I2S0) {
        i2s_unit = 0;
	} else {
		// I2S does not exist for this board (shouldn't get here, should be checked by caller)
		printf("machAUDIO chekc i2s pin select failed\n");
		return -1;
	}


   // init the pin to I2C mode
    uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;
    mp_hal_pin_config_alt(MICROPY_NAU88L25_I2C_SCL, mode, AF_FN_I2C, i2c_unit);
    mp_hal_pin_config_alt(MICROPY_NAU88L25_I2C_SDA, mode, AF_FN_I2C, i2c_unit);

   // init the pin to I2S mode
    mp_hal_pin_config_alt(MICROPY_NAU88L25_I2S_LRCK, mode, AF_FN_I2S, i2s_unit);
    mp_hal_pin_config_alt(MICROPY_NAU88L25_I2S_DO, mode, AF_FN_I2S, i2s_unit);
    mp_hal_pin_config_alt(MICROPY_NAU88L25_I2S_DI, mode, AF_FN_I2S, i2s_unit);
    mp_hal_pin_config_alt(MICROPY_NAU88L25_I2S_MCLK, mode, AF_FN_I2S, i2s_unit);
    mp_hal_pin_config_alt(MICROPY_NAU88L25_I2S_BCLK, mode, AF_FN_I2S, i2s_unit);


	I2C_InitTypeDef init;

	init.Mode = I2C_MODE_MASTER;	//Master/Slave mode: I2C_MODE_MASTER/I2C_MODE_SLAVE
	init.BaudRate = 100000;			//Baud rate
	init.GeneralCallMode = 0;		//Support general call mode: I2C_GCMODE_ENABLE/I2C_GCMODE_DISABLE
	init.OwnAddress = 0;			//Slave own address 

    // init the I2C device
    if (I2C_Init(self->i2c_base, &init) != 0) {
        // init error
        // TODO should raise an exception, but this function is not necessarily going to be
        // called via Python, so may not be properly wrapped in an NLR handler
        printf("OSError: HAL_I2C_Init failed\n");
        return -2;
    }

	S_NAU88L25_CONFIG sNAU88L25Config;
	
	sNAU88L25Config.pin_phonejack_en = MICROPY_NAU88L25_JK_EN;
	sNAU88L25Config.pin_phonejack_det = MICROPY_NAU88L25_JK_DET;
	
	mp_hal_pin_output(MICROPY_NAU88L25_JK_EN);
	mp_hal_pin_input(MICROPY_NAU88L25_JK_DET);

	//Audio Codec reset and init
	NAU88L25_Reset(self->i2c_base);
	NAU88L25_Init(self->i2c_base, &sNAU88L25Config);

	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MP3 playback function
//////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC void MP3_DECODE_HEADER(unsigned char *pBytes, struct mp3Header *hdr)
{
    (hdr)->sync     = (pBytes)[0];
    (hdr)->sync     = (hdr)->sync << 3;
    (hdr)->sync     |= ((pBytes)[1] & 0xE0) >> 5;
    (hdr)->version  = ((pBytes)[1] & 0x18) >> 3;
    (hdr)->layer    = ((pBytes)[1] & 0x06) >> 1;
    (hdr)->protect  = ((pBytes)[1] & 0x01);
    (hdr)->bitrate  = ((pBytes)[2] & 0xF0) >> 4;
    (hdr)->samfreq  = ((pBytes)[2] & 0x0C) >> 2;
    (hdr)->padding  = ((pBytes)[2] & 0x02) >> 1;
    (hdr)->private  = ((pBytes)[2] & 0x01);
    (hdr)->channel  = ((pBytes)[3] & 0xC0) >> 6;
    (hdr)->mode     = ((pBytes)[3] & 0x30) >> 4;
    (hdr)->copy     = ((pBytes)[3] & 0x08) >> 3;
    (hdr)->original = ((pBytes)[3] & 0x04) >> 2;
    (hdr)->emphasis = ((pBytes)[3] & 0x03);
}

STATIC int MP3_IS_VALID_HEADER(struct mp3Header *hdr)
{
    return( (((hdr)->sync == 0x7FF)
             && ((hdr)->bitrate != 0x0f)
             && ((hdr)->version != 0x01)
             && ((hdr)->layer != 0x00)
             && ((hdr)->samfreq != 0x03)
             && ((hdr)->emphasis != 0x02) ) ? 1:0);
}

#if 0
STATIC int MP3_IS_V1L3_HEADER(struct mp3Header *hdr)
{
    return( ((hdr)->layer == 0x01)
            && (((hdr)->version == 0x03) || ((hdr)->version == 0x02)) ? 1:0);
}
#endif

STATIC int mp3GetFrameLength(struct mp3Header *pHdr)
{
    int pL1Rates[] =   {   0,  32,  64,  96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448,  -1 };
    int pL2Rates[] =   {   0,  32,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384,  -1 };
    int pV1L3Rates[] = {   0,  32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320,  -1 };
    int pV2L1Rates[] = {   0,  32,  48,  56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256,  -1 };
    int pV2L3Rates[] = {   0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160,  -1 };

    int pRate[4][4] =
    {
        { 11025, 12000, 8000, -1 }, // 2.5
        { -1, -1, -1, -1 }, // reserved
        { 22050, 24000, 16000, -1 }, // 2
        { 44100, 48000, 32000, -1 } // 1
    };

    int bitrate;
    int freq;

    int base = 144;

    if (pHdr->layer == 0x01)
    {
        if (pHdr->version == 0x03)
            bitrate = pV1L3Rates[pHdr->bitrate];
        else
            bitrate = pV2L3Rates[pHdr->bitrate];
    }
    else if (pHdr->layer == 0x02)
    {
        if (pHdr->version == 0x03)
            bitrate = pL2Rates[pHdr->bitrate];
        else
            bitrate = pV2L3Rates[pHdr->bitrate];
    }
    else
    {
        if (pHdr->version == 0x03)
            bitrate = pL1Rates[pHdr->bitrate];
        else
            bitrate = pV2L1Rates[pHdr->bitrate];
    }

    if (pHdr->layer == 3)   /* Layer 1 */
    {
        base = 12;
    }

    freq = pRate[pHdr->version][pHdr->samfreq];

    return ((base * 1000 * bitrate) / freq) + pHdr->padding;
}

STATIC void mp3PrintHeader(
	struct mp3Header *pHdr, 
	struct AudioInfoObject *pAudioInfo
)
{
    int pL1Rates[] =   {   0,  32,  64,  96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448,  -1 };
    int pL2Rates[] =   {   0,  32,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384,  -1 };
    int pV1L3Rates[] = {   0,  32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320,  -1 };
    int pV2L1Rates[] = {   0,  32,  48,  56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256,  -1 };
    int pV2L3Rates[] = {   0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160,  -1 };

    int pRate[4][4] =
    {
        { 11025, 12000, 8000, -1 }, // 2.5
        { -1, -1, -1, -1 }, // reserved
        { 22050, 24000, 16000, -1 }, // 2
        { 44100, 48000, 32000, -1 } // 1
    };

    int bitrate;

    if (pHdr->layer == 0x01)
    {
        if (pHdr->version == 0x03)
            bitrate = pV1L3Rates[pHdr->bitrate];
        else
            bitrate = pV2L3Rates[pHdr->bitrate];
    }
    else if (pHdr->layer == 0x02)
    {
        if (pHdr->version == 0x03)
            bitrate = pL2Rates[pHdr->bitrate];
        else
            bitrate = pV2L3Rates[pHdr->bitrate];
    }
    else
    {
        if (pHdr->version == 0x03)
            bitrate = pL1Rates[pHdr->bitrate];
        else
            bitrate = pV2L1Rates[pHdr->bitrate];
    }

    pAudioInfo->SampleRate = pRate[pHdr->version][pHdr->samfreq];
    pAudioInfo->BitRate = bitrate * 1000;
    pAudioInfo->Channel = (pHdr->channel == 0x03 ? 1:2);
    pAudioInfo->PlayTime = ((unsigned int)pAudioInfo->FileSize / (unsigned int)pAudioInfo->BitRate * (unsigned int)8) * 1000;

}


STATIC int mp3CountV1L3Headers(
	unsigned char *pBytes, 
	size_t size,
	struct AudioInfoObject *pAudioInfo
)
{
    int             offset              = 0;
    int             result              = 0;
    struct mp3Header       header;

    while (size >= 4)
    {
        if ((pBytes[0] == 0xFF)
                && ((pBytes[1] & 0xE0) == 0xE0)
           )
        {
            MP3_DECODE_HEADER(pBytes, &header);

            if (MP3_IS_VALID_HEADER(&header))
            {
                int framelength = mp3GetFrameLength(&header);

                if ((framelength > 0) && (size > framelength + 4))
                {
                    MP3_DECODE_HEADER(pBytes + framelength, &header);

                    if (MP3_IS_VALID_HEADER(&header))
                    {
                        offset = 0;
                        mp3PrintHeader(&header, pAudioInfo);
                        result++;
                    }
                }
            }
        }

        offset++;
        pBytes++;
        size--;
    }

    return result;
}

// Parse MP3 header and get some informations

STATIC void MP3_ParseHeaderInfo(
	mach_audio_obj_t *self,
	char *pFileName
)
{
    FRESULT res;
    FIL fp;
    FILINFO Finfo;
	size_t	ReturnSize;

	memset(&self->audioInfo, 0, sizeof(struct AudioInfoObject));
    res = f_open(self->fs, &fp, (void *)pFileName, FA_OPEN_EXISTING | FA_READ);
    if (res == FR_OK)
    {
        printf("file is opened!!\r\n");
        f_stat(self->fs, (void *)pFileName, &Finfo);
        self->audioInfo.FileSize = Finfo.fsize;

        res = f_read(&fp, (char *)self->pu8MADInputBuff, FILE_IO_BUFFER_SIZE, &ReturnSize);

        //parsing MP3 header
        mp3CountV1L3Headers((unsigned char *)self->pu8MADInputBuff, ReturnSize, &self->audioInfo);
    }
    else
    {
        printf("Open File Error\r\n");
        return;
    }
    f_close(&fp);

    printf("====[MP3 Info]======\r\n");
    printf("FileSize = %d\r\n", self->audioInfo.FileSize);
    printf("SampleRate = %d\r\n", self->audioInfo.SampleRate);
    printf("BitRate = %d\r\n", self->audioInfo.BitRate);
    printf("Channel = %d\r\n", self->audioInfo.Channel);
    printf("PlayTime = %d\r\n", self->audioInfo.PlayTime);
    printf("=====================\r\n");

	if(self->audioInfo.SampleRate == 0){
		self->audioInfo.SampleRate = 44100;
	}

	if(self->audioInfo.Channel == 0){
		self->audioInfo.Channel = 2;
	}
}

STATIC void I2S_TX_PDMA_IRQ(
	void *psObj
)
{
	i2s_t *pI2SObj = (i2s_t *)psObj;
	mach_audio_obj_t *self = (mach_audio_obj_t *)pI2SObj->pvPriv;

	if(pI2SObj->event == I2S_EVENT_DMA_TX_DONE){
		if(self->au8PCMBuffer_Full[self->u8PCMBufferPlayIdx ^ 1] != 1)
			printf("underflow!!\n");
		self->au8PCMBuffer_Full[self->u8PCMBufferPlayIdx] = 0;       //set empty flag
		self->u8PCMBufferPlayIdx ^= 1;
	}
}

typedef struct dma_desc_t
{
    uint32_t ctl;
    uint32_t src;
    uint32_t dest;
    uint32_t offset;
} DMA_DESC_T;


STATIC void StartTxPDMATrans(
	mach_audio_obj_t *self
)
{
	I2S_TransParam sTransParam;

	sTransParam.tx = self->pi32PCMBuff;
	sTransParam.tx_length = PCM_BUFFER_SIZE;
	sTransParam.tx1 = (self->pi32PCMBuff + (PCM_BUFFER_SIZE));
	sTransParam.tx1_length = PCM_BUFFER_SIZE;
	
	sTransParam.rx = NULL;
	sTransParam.rx_length = 0;
	sTransParam.rx1 = NULL;
	sTransParam.rx1_length = 0;
	self->i2sObj.pvPriv = self;
	
	I2S_StartTransfer(&self->i2sObj, &sTransParam, I2S_TX_PDMA_IRQ, I2S_EVENT_ALL, DMA_USAGE_ALWAYS);
}

STATIC void StopTxPDMATrans(
	mach_audio_obj_t *self
)
{
	I2S_StopTransfer(&self->i2sObj);
}

STATIC	struct mad_stream   Stream;
STATIC	struct mad_frame    Frame;
STATIC	struct mad_synth    Synth;

STATIC void mp3_play_task(void *pvParameter) {
	
	mach_audio_obj_t *self = (mach_audio_obj_t *)pvParameter;

    uint8_t *ReadStart;
    uint8_t *GuardPtr;
	size_t Remaining;
	size_t ReadSize;
	size_t ReturnSize;
    FRESULT res;
    volatile unsigned int Mp3FileOffset=0;
    volatile uint8_t u8PCMBufferTargetIdx = 0;

	bool bStartPDMATransfer = FALSE;
    uint16_t sampleL, sampleR;
    volatile uint32_t pcmbuf_idx;
	int i;

	pcmbuf_idx = 0;
	self->au8PCMBuffer_Full[0] = 0;
	self->au8PCMBuffer_Full[1] = 0;
	self->u8PCMBufferPlayIdx = 0;
	
	/* Parse MP3 header */
	MP3_ParseHeaderInfo(self, self->szFile);

	/* First the structures used by libmad must be initialized. */
	mad_stream_init(&Stream);
	mad_frame_init(&Frame);
	mad_synth_init(&Synth);

	//Open I2S0 interface and set to slave mode, I2S format 
	I2S_InitTypeDef I2SInit;
	
	I2SInit.Mode = I2S_MODE_SLAVE;
	I2SInit.SampleRate = self->audioInfo.SampleRate;
	I2SInit.WordWidth = I2S_DATABIT_16;
	I2SInit.DataFormat = I2S_FORMAT_I2S;

	if(self->audioInfo.Channel == 1)
		I2SInit.MonoData = I2S_ENABLE_MONO;
	else
		I2SInit.MonoData = I2S_DISABLE_MONO;

	
	if(I2S_Init(&self->i2sObj, &I2SInit) != 0){
		goto play_stop;
	}


	NAU88L25_MixerCtrl(self->i2c_base, NAUL8825_MIXER_MUTE, 0);

    /* Configure NAU88L25 to specific sample rate */
    NAU88L25_DSPConfig(self->i2c_base, self->audioInfo.SampleRate, self->audioInfo.Channel, 32);

	self->eStatus = eAUDIO_STATUS_PLAYING;

    FIL mp3_fp;

    /* Open MP3 file */
    f_open(self->fs, &mp3_fp, self->szFile, FA_OPEN_EXISTING | FA_READ);

	while(self->eStatus == eAUDIO_STATUS_PLAYING){
 
		//Read MP3 file
        if(Stream.buffer==NULL || Stream.error==MAD_ERROR_BUFLEN)
        {
            if(Stream.next_frame != NULL)
            {
                /* Get the remaining frame */
                Remaining = Stream.bufend - Stream.next_frame;
                memmove(self->pu8MADInputBuff, Stream.next_frame, Remaining);
                ReadStart = self->pu8MADInputBuff + Remaining;
                ReadSize = FILE_IO_BUFFER_SIZE - Remaining;
            }
            else
            {
                ReadSize = FILE_IO_BUFFER_SIZE,
                ReadStart = self->pu8MADInputBuff,
                Remaining = 0;
            }

            /* read the file */
            res = f_read(&mp3_fp, ReadStart, ReadSize, &ReturnSize);
            if(res != FR_OK)
            {
                printf("Stop !(%x)\n", res);
                break;
            }

            if(f_eof(&mp3_fp))
            {
                printf("End Stop !\n");
                if(ReturnSize == 0)
                    break;
            }

            /* if the file is over */
            if (ReadSize > ReturnSize)
            {
                GuardPtr = ReadStart + ReadSize;
                memset(GuardPtr, 0, MAD_BUFFER_GUARD);
                ReadSize += MAD_BUFFER_GUARD;
            }

            Mp3FileOffset = Mp3FileOffset + ReturnSize;
            /* Pipe the new buffer content to libmad's stream decoder
                     * facility.
            */
            mad_stream_buffer(&Stream,self->pu8MADInputBuff, ReadSize+Remaining);
            Stream.error=(enum  mad_error)0;
        }


        /* decode a frame from the mp3 stream data */
        if(mad_frame_decode(&Frame,& Stream))
        {
            if(MAD_RECOVERABLE(Stream.error))
            {
                /*if(Stream.error!=MAD_ERROR_LOSTSYNC ||
                   Stream.this_frame!=GuardPtr)
                {
                }*/
                continue;
            }
            else
            {
                /* the current frame is not full, need to read the remaining part */
                if(Stream.error==MAD_ERROR_BUFLEN)
                {
                    continue;
                }
                else
                {
                    printf("Something error!!\n");

                    /* play the next file */
//                    audioInfo.mp3FileEndFlag = 1;
                    break;
                }
            }
        }

        /* Once decoded the frame is synthesized to PCM samples. No errors
        * are reported by mad_synth_frame();
        */
        mad_synth_frame(&Synth, &Frame);

		//
		// decode finished, try to copy pcm data to audio buffer
		//

		if(bStartPDMATransfer == TRUE){
			//if next buffer is still full (playing), wait until it's empty
            if(self->au8PCMBuffer_Full[u8PCMBufferTargetIdx] == 1){
//				printf("Wait PDMA trans done Idx is %d, %d \n", u8PCMBufferTargetIdx, self->u8PCMBuffer_Playing);
                while(self->au8PCMBuffer_Full[u8PCMBufferTargetIdx]){
//					vTaskDelay(1);
				}
//				printf("Exit PDMA trans done ---0\n");
			}
		}
		else{
            if((self->au8PCMBuffer_Full[0] == 1) && (self->au8PCMBuffer_Full[1] == 1 ))         //all buffers are full, wait
            {
                StartTxPDMATrans(self);
				bStartPDMATransfer = TRUE;
            }
		}

		signed int *piPCMBufAdr = self->pi32PCMBuff + (u8PCMBufferTargetIdx * PCM_BUFFER_SIZE) + pcmbuf_idx;

        for(i=0; i<(int)Synth.pcm.length; i++)
        {
            /* Get the left/right samples */
            sampleL = Synth.pcm.samples[0][i];
            sampleR = Synth.pcm.samples[1][i];

            /* Fill PCM data to I2S(PDMA) buffer */
			*piPCMBufAdr = sampleR | (sampleL << 16);
			piPCMBufAdr ++;			
			pcmbuf_idx++;

            /* Need change buffer ? */
            if(pcmbuf_idx == PCM_BUFFER_SIZE)
            {
                self->au8PCMBuffer_Full[u8PCMBufferTargetIdx] = 1;      //set full flag
                u8PCMBufferTargetIdx ^= 1;

                pcmbuf_idx = 0;
                //printf("change to ==>%d ..\n", u8PCMBufferTargetIdx);
                /* if next buffer is still full (playing), wait until it's empty */
                if((self->au8PCMBuffer_Full[u8PCMBufferTargetIdx] == 1) && (bStartPDMATransfer)){
//					printf("Wait PDMA trans done ---1\n");
                    while(self->au8PCMBuffer_Full[u8PCMBufferTargetIdx]){
//						vTaskDelay(1);
					}
//				printf("Exit PDMA trans done %d ---1\n", Synth.pcm.length);
				}
            }
        }
	}

    f_close(&mp3_fp);
	I2S_Final(&self->i2sObj);
	StopTxPDMATrans(self);


play_stop:

	//Mute and phonejack disable
	NAU88L25_MixerCtrl(self->i2c_base, NAUL8825_MIXER_MUTE, 1);

    mad_synth_finish(&Synth);
    mad_frame_finish(&Frame);
    mad_stream_finish(&Stream);

	free(self->szFile);
	free(self->pi32PCMBuff);
	free(self->pu8MADInputBuff);

	self->eStatus = eAUDIO_STATUS_STOP;
	vTaskDelete(NULL);
}

#define DEF_RESERVED_STOR_SPACE_KB	10 


STATIC void I2S_RX_PDMA_IRQ(
	void *psObj
)
{
	i2s_t *pI2SObj = (i2s_t *)psObj;
	mach_audio_obj_t *self = (mach_audio_obj_t *)pI2SObj->pvPriv;

	if(pI2SObj->event == I2S_EVENT_DMA_RX_DONE){
		if(self->au8PCMBuffer_Full[self->u8PCMBufferRecIdx ^ 1] != 0)
			printf("overflow!!\n");
		self->au8PCMBuffer_Full[self->u8PCMBufferRecIdx] = 1;       //set full flag
		self->u8PCMBufferRecIdx ^= 1;
	}
}

STATIC void StartRxPDMATrans(
	mach_audio_obj_t *self
)
{
	I2S_TransParam sTransParam;

	sTransParam.rx = self->pi32PCMBuff;
	sTransParam.rx_length = PCM_BUFFER_SIZE;
	sTransParam.rx1 = (self->pi32PCMBuff + (PCM_BUFFER_SIZE));
	sTransParam.rx1_length = PCM_BUFFER_SIZE;
	
	sTransParam.tx = NULL;
	sTransParam.tx_length = 0;
	sTransParam.tx1 = NULL;
	sTransParam.tx1_length = 0;
	self->i2sObj.pvPriv = self;
	
	I2S_StartTransfer(&self->i2sObj, &sTransParam, I2S_RX_PDMA_IRQ, I2S_EVENT_ALL, DMA_USAGE_ALWAYS);
}

STATIC void StopRxPDMATrans(
	mach_audio_obj_t *self
)
{
	I2S_StopTransfer(&self->i2sObj);
}



STATIC void mp3_rec_task(void *pvParameter) {
	
	mach_audio_obj_t *self = (mach_audio_obj_t *)pvParameter;

	shine_config_t sShineConfig;
	shine_t psShine = NULL;
	
	uint32_t u32SamplePerBlock;
	volatile uint8_t u8PCMBuffEncIdx = 0;
	volatile uint32_t pcmbuf_idx;
	uint8_t *pu8EncodedData;
	int16_t *pi6PCMBufAdr;
	int i32EncodedLen;
	UINT i32FileWriteLen;
	uint32_t u32TotalFileWriteB;

	u32TotalFileWriteB = 0;
	self->au8PCMBuffer_Full[0] = 0;
	self->au8PCMBuffer_Full[1] = 0;
	self->u8PCMBufferRecIdx = 0;
	pcmbuf_idx = 0;
	
	
	shine_set_config_mpeg_defaults(&sShineConfig.mpeg);
	sShineConfig.mpeg.bitr = self->audioInfo.BitRate / 1000;

	if(self->audioInfo.Channel > 1){
		sShineConfig.wave.channels = PCM_STEREO;	
		sShineConfig.mpeg.mode = STEREO;
	}
	else{
		sShineConfig.wave.channels = PCM_MONO;	
		sShineConfig.mpeg.mode = MONO;
	}

	sShineConfig.wave.samplerate = self->audioInfo.SampleRate;

	//open shine mp3 encoder
	psShine = shine_initialise(&sShineConfig);
	if(psShine == NULL)
		goto record_stop;

	u32SamplePerBlock = shine_samples_per_pass(psShine);

	if(self->audioInfo.Channel == 1){
		u32SamplePerBlock /= 2;
	}

	if(PCM_BUFFER_SIZE % u32SamplePerBlock){
		printf("PCM_BUFFER_SIZE is not multiple of %ld \n", u32SamplePerBlock);
	}

	//setup audio codec
	//Open I2S0 interface and set to slave mode, I2S format 
	I2S_InitTypeDef I2SInit;
	
	I2SInit.Mode = I2S_MODE_SLAVE;
	I2SInit.SampleRate = self->audioInfo.SampleRate;
	I2SInit.WordWidth = I2S_DATABIT_16;
	I2SInit.DataFormat = I2S_FORMAT_I2S;

	if(self->audioInfo.Channel == 1)
		I2SInit.MonoData = I2S_ENABLE_MONO;
	else
		I2SInit.MonoData = I2S_DISABLE_MONO;

	
	if(I2S_Init(&self->i2sObj, &I2SInit) != 0){
		goto record_stop;
	}

	NAU88L25_MixerCtrl(self->i2c_base, NAUL8825_MIXER_MUTE, 0);

    /* Configure NAU88L25 to specific sample rate */
    NAU88L25_DSPConfig(self->i2c_base, self->audioInfo.SampleRate, self->audioInfo.Channel, 32);

	//Open record file
    FIL mp3_fp;
	FRESULT res;

    /* Open MP3 file */
	res = f_open(self->fs, &mp3_fp, self->szFile, FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK){
		goto record_stop;
	}

	//Start PDMA
	StartRxPDMATrans(self);
		
	self->eStatus = eAUDIO_STATUS_RECORDING;
	
	
	while(self->eStatus == eAUDIO_STATUS_RECORDING){

		if(self->au8PCMBuffer_Full[u8PCMBuffEncIdx] == 0){
			//Wait PCM data ready
			while(self->au8PCMBuffer_Full[u8PCMBuffEncIdx] == 0){
//				vTaskDelay(1);
			}
		}
		
		//Encode and fwrite
		if((PCM_BUFFER_SIZE - pcmbuf_idx) > u32SamplePerBlock){
			pi6PCMBufAdr = (int16_t *)(self->pi32PCMBuff + (u8PCMBuffEncIdx * PCM_BUFFER_SIZE) + pcmbuf_idx);
			pu8EncodedData = shine_encode_buffer_interleaved(psShine, pi6PCMBufAdr, &i32EncodedLen);
		
			//Write file
			if(pu8EncodedData){
				res = f_write(&mp3_fp, pu8EncodedData, i32EncodedLen, &i32FileWriteLen);
				if(res != FR_OK)
				{
					printf("MP3 file write stop! (%x)\n", res);
					break;
				}
				u32TotalFileWriteB += i32FileWriteLen;
			}
			
			if((self->u32StorFreeSizeKB - (u32TotalFileWriteB / 1024)) <= DEF_RESERVED_STOR_SPACE_KB){
				printf("MP3 file write stop! Storage free space not enough\n");
				break;
			}
			
			//Update index
			pcmbuf_idx += u32SamplePerBlock;
		}
		else {
			//Wait next buffer
			printf("Discard some audio data. Because PCM_BUFFER_SIZE is not multiple of samples per block \n");
			pcmbuf_idx = PCM_BUFFER_SIZE;
		}
		
		if(pcmbuf_idx >= PCM_BUFFER_SIZE){
			self->au8PCMBuffer_Full[u8PCMBuffEncIdx] = 0;
			u8PCMBuffEncIdx ^= 1;			
			pcmbuf_idx -= PCM_BUFFER_SIZE;
		}
	
	}

	//Stop PDMA
	StopRxPDMATrans(self);

	//Close record file
	f_close(&mp3_fp);

	/* Flush and write remaining data. */
	int i32DataLen;
	shine_flush(psShine, &i32DataLen);


record_stop:

	I2S_Final(&self->i2sObj);

	//Mute and phonejack disable
	NAU88L25_MixerCtrl(self->i2c_base, NAUL8825_MIXER_MUTE, 1);
	
	if(psShine){
		/* Close encoder. */
		shine_close(psShine);
	}

	free(self->szFile);
	free(self->pi32PCMBuff);

	self->eStatus = eAUDIO_STATUS_STOP;
	vTaskDelete(NULL);

}

STATIC FATFS *lookup_path(const char **path) {
    mp_vfs_mount_t *fs = mp_vfs_lookup_path(*path, path);
    if (fs == MP_VFS_NONE || fs == MP_VFS_ROOT) {
        return NULL;
    }
    // here we assume that the mounted device is FATFS
    return &((fs_user_mount_t*)MP_OBJ_TO_PTR(fs->obj))->fatfs;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	MicroPython binding function
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t mach_audio_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    mach_audio_obj_t *self = &mach_audio_obj;

	if(!self->initialized){
		//initiate audio codec //nau88l25 open I2c and write configure
		if(acodec_open(self) != 0){
		   nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Unable open audio codec"));
		}
		self->initialized = TRUE;
	}

    // return object
	return self;
}


STATIC mp_obj_t mach_audio_mp3_play(mp_obj_t self_in, mp_obj_t file_name) {

	bool bPlayed = FALSE;

	mach_audio_obj_t *self = MP_OBJ_TO_PTR(self_in);

	if(self->eStatus != eAUDIO_STATUS_STOP){
		mp_printf(&mp_plat_print, "Audio is busy. Unable play");
		return mp_obj_new_bool(bPlayed);
	}

	const char *szFileName = mp_obj_str_get_str(file_name);

	
	//look up file system
	self->fs = lookup_path(&szFileName);
	
    if (self->fs == NULL) {
		mp_printf(&mp_plat_print, "file system not found");
		return mp_obj_new_bool(bPlayed);

    }

	//Check file exist or not
    FILINFO fno;
	FRESULT res;
    
    res = f_stat(self->fs, szFileName, &fno);

	if(res != FR_OK){
		mp_printf(&mp_plat_print, "No such file");
		return mp_obj_new_bool(bPlayed);
	}
	
	//copy file name
	self->szFile = strdup(szFileName);

	if(self->szFile == NULL){
		mp_printf(&mp_plat_print, "strdup failed");
		return mp_obj_new_bool(bPlayed);
	}
	
	//allocate PCM buffer
	self->pi32PCMBuff = malloc(2 * PCM_BUFFER_SIZE * sizeof(signed int));

	if(self->pi32PCMBuff == NULL){
		free(self->szFile);
		mp_printf(&mp_plat_print, "allocate PCM buffer failed");
		return mp_obj_new_bool(bPlayed);
	}

	//allocate file IO buffer
	self->pu8MADInputBuff = malloc(FILE_IO_BUFFER_SIZE + MAD_BUFFER_GUARD);

	if(self->pu8MADInputBuff == NULL){
		free(self->szFile);
		free(self->pi32PCMBuff);
		mp_printf(&mp_plat_print, "allocate file IO buffer failed");
		return mp_obj_new_bool(bPlayed);
	}
		
	//Create MP3 play thread 
	TaskHandle_t MP3PlayTaskHandle;

    MP3PlayTaskHandle = xTaskCreateStatic(mp3_play_task, "MP3_play",
        MP3_TASK_STACK_LEN, self, MP3_TASK_PRIORITY, mp3task_stack, &mp3task_tcb);

    if (MP3PlayTaskHandle == NULL){
		free(self->szFile);
		free(self->pi32PCMBuff);
		free(self->pu8MADInputBuff);
		mp_printf(&mp_plat_print, "unable crate mp3 play thread");
		return mp_obj_new_bool(bPlayed);
	}
	
	self->eStatus = eAUDIO_STATUS_TO_PLAY;
	bPlayed = TRUE;
	return mp_obj_new_bool(bPlayed);
}

MP_DEFINE_CONST_FUN_OBJ_2(mach_audio_mp3_play_obj, mach_audio_mp3_play);


STATIC mp_obj_t mach_audio_mp3_stop(mp_obj_t self_in) {

	mach_audio_obj_t *self = MP_OBJ_TO_PTR(self_in);

	if(self->eStatus == eAUDIO_STATUS_STOP){
		return mp_const_none;
	}

	self->eStatus = eAUDIO_STATUS_TO_STOP;

	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mach_audio_mp3_stop_obj, mach_audio_mp3_stop);



STATIC mp_obj_t mach_audio_mp3_status(mp_obj_t self_in) {

	mach_audio_obj_t *self = MP_OBJ_TO_PTR(self_in);

	return MP_OBJ_NEW_SMALL_INT(self->eStatus);
}

MP_DEFINE_CONST_FUN_OBJ_1(mach_audio_mp3_status_obj, mach_audio_mp3_status);

#define DEF_REC_FILE_NAME "/sd/nvt001.mp3"

STATIC mp_obj_t mach_audio_mp3_rec(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_file, ARG_samplerate, ARG_channel, ARG_bitrate};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_file, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_samplerate, MP_ARG_INT, {.u_int = 16000} },
        { MP_QSTR_channel, MP_ARG_INT, {.u_int = 2} },
        { MP_QSTR_bitrate, MP_ARG_INT, {.u_int = 128000} },
    };

    mach_audio_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	if(self->eStatus != eAUDIO_STATUS_STOP){
		mp_printf(&mp_plat_print, "Audio is busy. Unable record");
		return mp_const_none;
	}

	const char *szFileName;
	
	if(args[ARG_file].u_obj != mp_const_none){
		szFileName = mp_obj_str_get_str(args[ARG_file].u_obj);
	}
	else{
		szFileName = DEF_REC_FILE_NAME;
	}

	if(shine_check_config(args[ARG_samplerate].u_int, (args[ARG_bitrate].u_int) / 1000)< 0){
		mp_printf(&mp_plat_print, "sample rate and bit rate is not supported");
		return mp_const_none;
	}

	//copy file name
	self->szFile = strdup(szFileName);

	if(self->szFile == NULL){
		mp_printf(&mp_plat_print, "strdup failed");
		return mp_const_none;
	}

	//look up file system
	self->fs = lookup_path(&szFileName);
	
    if (self->fs == NULL) {
		mp_printf(&mp_plat_print, "file system not found");
		return mp_const_none;
    }

	//Get free storage size;
	f_getfree(self->fs, &self->u32StorFreeSizeKB);
	self->u32StorFreeSizeKB = (self->u32StorFreeSizeKB * self->fs->csize) / (1024 / 512);
	printf("Free stoage space %d(KB) \n", self->u32StorFreeSizeKB);

	if(self->u32StorFreeSizeKB <= DEF_RESERVED_STOR_SPACE_KB){
		mp_printf(&mp_plat_print, "storage free space is not enough");
		return mp_const_none;
	}

	//allocate PCM buffer
	self->pi32PCMBuff = malloc(2 * PCM_BUFFER_SIZE * sizeof(signed int));

	if(self->pi32PCMBuff == NULL){
		free(self->szFile);
		mp_printf(&mp_plat_print, "allocate PCM buffer failed");
		return mp_const_none;
	}

	//Create MP3 record thread 
	TaskHandle_t MP3RecTaskHandle;

    MP3RecTaskHandle = xTaskCreateStatic(mp3_rec_task, "MP3_rec",
        MP3_TASK_STACK_LEN, self, MP3_TASK_PRIORITY, mp3task_stack, &mp3task_tcb);

    if (MP3RecTaskHandle == NULL){
		free(self->szFile);
		free(self->pi32PCMBuff);
		mp_printf(&mp_plat_print, "unable crate mp3 record thread");
		return mp_const_none;
	}

	self->audioInfo.SampleRate = args[ARG_samplerate].u_int;
	self->audioInfo.Channel = args[ARG_channel].u_int;	
	self->audioInfo.BitRate = args[ARG_bitrate].u_int;	
	self->eStatus = eAUDIO_STATUS_TO_RECORD;

	return mp_obj_new_str(self->szFile, strlen(self->szFile));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mach_audio_mp3_rec_obj, 1, mach_audio_mp3_rec);


STATIC const mp_rom_map_elem_t mach_audio_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_mp3_play), MP_ROM_PTR(&mach_audio_mp3_play_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mp3_stop), MP_ROM_PTR(&mach_audio_mp3_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mp3_status), MP_ROM_PTR(&mach_audio_mp3_status_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mp3_record), MP_ROM_PTR(&mach_audio_mp3_rec_obj) },

	// constants for AUDIO status
    { MP_ROM_QSTR(MP_QSTR_STATUS_STOP), MP_ROM_INT(eAUDIO_STATUS_STOP) },
    { MP_ROM_QSTR(MP_QSTR_STATUS_PLAYING), MP_ROM_INT(eAUDIO_STATUS_PLAYING) },
    { MP_ROM_QSTR(MP_QSTR_STATUS_RECORDING), MP_ROM_INT(eAUDIO_STATUS_RECORDING) },
    { MP_ROM_QSTR(MP_QSTR_STATUS_TO_PLAY), MP_ROM_INT(eAUDIO_STATUS_TO_PLAY) },
    { MP_ROM_QSTR(MP_QSTR_STATUS_TO_RECORD), MP_ROM_INT(eAUDIO_STATUS_TO_RECORD) },
    { MP_ROM_QSTR(MP_QSTR_STATUS_TO_STOP), MP_ROM_INT(eAUDIO_STATUS_TO_STOP) },	
};

STATIC MP_DEFINE_CONST_DICT(mach_audio_locals_dict, mach_audio_locals_dict_table);


const mp_obj_type_t mach_audio_type = {
    { &mp_type_type },
    .name = MP_QSTR_AUDIO,
    .make_new = mach_audio_make_new,
    .locals_dict = (mp_obj_dict_t*)&mach_audio_locals_dict,
};

#endif
