/* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
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
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "N9H26_avcodec.h"
#include "N9H26_version.h"
#include "NVTMedia.h"
#include "NVTMedia_SAL_OS.h"

#include "H264Ratecontrol.h"

typedef struct {
	uint8_t *pu8TmpBuf;
	uint32_t u32TmpBufSize;
	uint64_t u64TmpDataTime;

	bool bInit;
	int32_t i32CodecInst;
	FAVC_ENC_PARAM sH264EncParam;
	H264RateControl sH264RateCtrl;
	S_NM_H264_CTX_PARAM	sCtxParam;
	uint32_t u32NextQuant;

	uint32_t u32EncBitStreamBufSize;
	uint8_t *pu8EncBitStreamBuf;
	uint32_t u32EncBitStreamLen;
	uint64_t u64EncBitStreamDataTime;
	
} S_H264ENC_RES;


static pthread_mutex_t *s_ptH264EncCodecMutex = NULL;
static S_NM_H264_CTX_PARAM s_sDefaultCtxParam = 
{
	.eProfile = eNM_H264_BASELINE,
	.u32FixQuality = 0, //Fixed quality(dynamic bitrate) (1(Better) ~ 52, Dynamic: 0 (According to bitrate))
	.u32FixBitrate = 1024,	//Fixed bitrate kbps (dynamic quality) (default value is 1024 (1Mbps))
	.u32FixGOP = 30,			//GOP (0: According to dest context frame rate, 1:All I frame, > 1: Fixed GOP)
	
	.u32MaxQuality = 50,	//Max quality for fixed bitrate  (1(Better) ~ 51). 0:default 50
	.u32MinQuality = 25,	//Min quality for fixed bitrate  (1(Better) ~ 51). 0:default 25
};

static E_NM_ERRNO
CheckingCtxParam(
	S_NM_VIDEO_CTX *psDestCtx,
	S_NM_H264_CTX_PARAM *psCtxParam
)
{
	if(psCtxParam->eProfile != eNM_H264_BASELINE)
		return eNM_ERRNO_BAD_PARAM;
	
	if(psCtxParam-> u32FixQuality > 52)
		return eNM_ERRNO_BAD_PARAM;
		
	if(psCtxParam->u32FixBitrate == 0)
		psCtxParam->u32FixBitrate = s_sDefaultCtxParam.u32FixBitrate;
	
	if(psCtxParam->u32FixGOP == 0){
		if(psDestCtx->u32FrameRate)
			psCtxParam->u32FixGOP = psDestCtx->u32FrameRate;
		else
			psCtxParam->u32FixGOP = s_sDefaultCtxParam.u32FixGOP;
	}
			
	if((psCtxParam->u32MaxQuality == 0) || (psCtxParam->u32MaxQuality > s_sDefaultCtxParam.u32MaxQuality))
		psCtxParam->u32MaxQuality = s_sDefaultCtxParam.u32MaxQuality;
		
	if((psCtxParam->u32MinQuality == 0) || (psCtxParam->u32MinQuality > s_sDefaultCtxParam.u32MinQuality))
		psCtxParam->u32MinQuality = s_sDefaultCtxParam.u32MinQuality;

	if(psCtxParam->u32MinQuality >= psCtxParam->u32MaxQuality)
		return eNM_ERRNO_BAD_PARAM;
	
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
NM_H264Enc_Init(
	S_NM_VIDEO_CTX *psSrcCtx,
	S_NM_VIDEO_CTX *psDestCtx,
	S_H264ENC_RES *psH264EncRes
)
{

	FAVC_ENC_PARAM *psH264EncParam = &psH264EncRes->sH264EncParam;
	S_NM_H264_CTX_PARAM *psCtxParam = &psH264EncRes->sCtxParam;

	if(psH264EncRes->pu8EncBitStreamBuf == NULL){
		psH264EncRes->u32EncBitStreamBufSize = (psSrcCtx->u32Width * psSrcCtx->u32Height) / 3;
		psH264EncRes->pu8EncBitStreamBuf = (uint8_t *)malloc(psH264EncRes->u32EncBitStreamBufSize);
		
		if(psH264EncRes->pu8EncBitStreamBuf == NULL){
			NMLOG_ERROR("Unable allocate H264 bit stream buffer \n");
			return eNM_ERRNO_MALLOC;
		}
	}

	memset(psH264EncParam, 0x00, sizeof(FAVC_ENC_PARAM));	
	psH264EncParam->u32API_version = H264VER;
	psH264EncParam->u32FrameWidth = psSrcCtx->u32Width;
	psH264EncParam->u32FrameHeight = psSrcCtx->u32Height;
	
	psH264EncParam->fFrameRate = psSrcCtx->u32FrameRate;
	psH264EncParam->u32IPInterval = psCtxParam->u32FixGOP; //IPPPP.... I, next I frame interval
	
	psH264EncParam->u32MaxQuant = rc_MAX_QUANT;
	psH264EncParam->u32MinQuant = rc_MIN_QUANT;
	psH264EncParam->u32Quant = (psH264EncParam->u32MaxQuant + psH264EncParam->u32MinQuant) / 2;
	psH264EncParam->u32BitRate = psCtxParam->u32FixBitrate * 1000;         

	psH264EncParam->ssp_output = 0; //1: Force output sps and pps. 0: Force output sps and pps on I slice frame. -1: output sps and pps only on the first IDR frame
	psH264EncParam->intra = -1; //1: Force the encoder to output I slice, 0: Force the encoder not to output I slice, -1: Follow the IP interval rule

	psH264EncParam->bROIEnable = 0;
	psH264EncParam->u32ROIX = 0;
	psH264EncParam->u32ROIY = 0;
	psH264EncParam->u32ROIWidth = 0;
	psH264EncParam->u32ROIHeight = 0;

	memset(&psH264EncRes->sH264RateCtrl, 0x00, sizeof(H264RateControl));

	H264RateControlInit(&psH264EncRes->sH264RateCtrl, psH264EncParam->u32BitRate,
			RC_DELAY_FACTOR,RC_AVERAGING_PERIOD, RC_BUFFER_SIZE_BITRATE, 
			(int)psH264EncParam->fFrameRate,
			rc_MAX_QUANT, 
			rc_MIN_QUANT,
			(unsigned int)psH264EncParam->u32Quant, 
			psH264EncParam->u32IPInterval);

	if (H264_ioctl_ex(psH264EncRes->i32CodecInst, FAVC_IOCTL_ENCODE_INIT, psH264EncParam) < 0){
		NMLOG_ERROR("Unable init H264 encode param \n");
		return eNM_ERRNO_IO;
	}

	psH264EncRes->u32NextQuant = psH264EncParam->u32Quant;
	psH264EncRes->bInit = true;

	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
NM_H264Enc_Enc(
	S_NM_VIDEO_CTX *psSrcCtx,
	uint8_t *pu8BSBuf,
	S_H264ENC_RES *psH264EncRes
)
{
	uint32_t u32ImgYSize;
	FAVC_ENC_PARAM *psH264EncParam = &psH264EncRes->sH264EncParam;
	
	u32ImgYSize = psSrcCtx->u32Width * psSrcCtx->u32Height;
	psH264EncParam->pu8YFrameBaseAddr = (uint8_t *)psSrcCtx->pu8DataBuf;
	psH264EncParam->pu8UFrameBaseAddr = psH264EncParam->pu8YFrameBaseAddr + u32ImgYSize;
	psH264EncParam->pu8VFrameBaseAddr = psH264EncParam->pu8UFrameBaseAddr + (u32ImgYSize >> 2);
	
	psH264EncParam->bitstream = pu8BSBuf;

	if(psH264EncRes->sCtxParam.u32FixQuality){
		psH264EncParam->u32Quant = psH264EncRes->sCtxParam.u32FixQuality;
	}
	else{
		psH264EncParam->u32Quant = psH264EncRes->u32NextQuant;
		
		if(psH264EncParam->u32Quant > psH264EncRes->sCtxParam.u32MaxQuality)
			psH264EncParam->u32Quant = psH264EncRes->sCtxParam.u32MaxQuality;

		if(psH264EncParam->u32Quant < psH264EncRes->sCtxParam.u32MinQuality)
			psH264EncParam->u32Quant = psH264EncRes->sCtxParam.u32MinQuality;
	}
	
	psH264EncParam->bitstream_size = 0;

	if (H264_ioctl_ex(psH264EncRes->i32CodecInst, FAVC_IOCTL_ENCODE_FRAME, psH264EncParam) < 0){
		NMLOG_ERROR("Trigger encode H264 frame failed \n");
		return eNM_ERRNO_ENCODE;
	}

	if (psH264EncRes->sCtxParam.u32FixQuality == 0){
		if (psH264EncParam->keyframe == 0){
			H264RateControlUpdate(&psH264EncRes->sH264RateCtrl, psH264EncParam->u32Quant, psH264EncParam->bitstream_size, 0);
		}
		else {
			H264RateControlUpdate(&psH264EncRes->sH264RateCtrl, psH264EncParam->u32Quant, psH264EncParam->bitstream_size, 1);
		}
		psH264EncRes->u32NextQuant = psH264EncRes->sH264RateCtrl.rtn_quant;
	}
		
	return eNM_ERRNO_NONE;
}


////////////////////////////////////////////////////////////


static E_NM_ERRNO
NM_H264Enc_Open(
	S_NM_VIDEO_CTX *psDestCtx,
	void **ppCodecRes
)
{
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;	
	S_H264ENC_RES *psH264EncRes = NULL;

	if (ppCodecRes == NULL) {
		return eNM_ERRNO_NULL_PTR;
	}

	*ppCodecRes = NULL;

	if(s_ptH264EncCodecMutex == NULL){
		s_ptH264EncCodecMutex = calloc(1, sizeof(pthread_mutex_t));
		if(s_ptH264EncCodecMutex == NULL)
			return eNM_ERRNO_MALLOC;
		if(pthread_mutex_init(s_ptH264EncCodecMutex, NULL) != 0){
			return eNM_ERRNO_OS;
		}
	}
	
	pthread_mutex_lock(s_ptH264EncCodecMutex);
	
	psH264EncRes = calloc(1, sizeof(S_H264ENC_RES));

	if (psH264EncRes == NULL){
		eRet = eNM_ERRNO_MALLOC;
		goto NM_H264Enc_Open_fail;
	}

	if(psDestCtx->pvParamSet){
		memcpy(&psH264EncRes->sCtxParam, psDestCtx->pvParamSet, sizeof(S_NM_H264_CTX_PARAM));
	}
	else{
		memcpy(&psH264EncRes->sCtxParam, &s_sDefaultCtxParam, sizeof(S_NM_H264_CTX_PARAM));
	}

	eRet = CheckingCtxParam(psDestCtx, &psH264EncRes->sCtxParam);
	if(eRet != eNM_ERRNO_NONE){
		goto NM_H264Enc_Open_fail;
	}
	
	psH264EncRes->i32CodecInst = H264Enc_Open(); 

	if(psH264EncRes->i32CodecInst < 0){
		NMLOG_ERROR("Unable open H264 hardware encoder \n");
		eRet = eNM_ERRNO_CODEC_OPEN;
		goto NM_H264Enc_Open_fail;
	}
	
	pthread_mutex_unlock(s_ptH264EncCodecMutex);	
	
	*ppCodecRes = psH264EncRes;

	return eNM_ERRNO_NONE;
	
NM_H264Enc_Open_fail:

	pthread_mutex_unlock(s_ptH264EncCodecMutex);	
		
	if(psH264EncRes)
		free(psH264EncRes);
	
	return eRet;
}

E_NM_ERRNO
NM_H264Enc_Close(
	void	**ppCodecRes
)
{
	if((ppCodecRes == NULL) || (*ppCodecRes == NULL))
		return eNM_ERRNO_NULL_PTR;
		
	S_H264ENC_RES *psH264EncRes = (S_H264ENC_RES *) *ppCodecRes;

	pthread_mutex_lock(s_ptH264EncCodecMutex);
	
	if(psH264EncRes->pu8TmpBuf)
		free(psH264EncRes->pu8TmpBuf);

	if(psH264EncRes->pu8EncBitStreamBuf)
		free(psH264EncRes->pu8EncBitStreamBuf);
	
	if(psH264EncRes->i32CodecInst >= 0){
		H264Enc_Close_ex(psH264EncRes->i32CodecInst);
	}
		
	pthread_mutex_unlock(s_ptH264EncCodecMutex);
	
	free(psH264EncRes);
	*ppCodecRes = NULL;
	
	return eNM_ERRNO_NONE;
}

E_NM_ERRNO
NM_H264Enc_Encode(
	S_NM_VIDEO_CTX *psSrcCtx,
	S_NM_VIDEO_CTX *psDestCtx,
	uint32_t *pu32RemainDataSize,
	void *pvCodecRes
)
{
	S_H264ENC_RES *psH264EncRes = (S_H264ENC_RES *)pvCodecRes;
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;	
	uint8_t *pu8EncBSBuf = NULL;
	uint32_t u32EncDataLen = 0;
	
	if (psH264EncRes == NULL)
		return eNM_ERRNO_NULL_PTR;

	if(psH264EncRes->i32CodecInst < 0)
		return eNM_ERRNO_CODEC_OPEN;

	psDestCtx->u32DataSize = 0;
	*pu32RemainDataSize = 0;
	
	if (psSrcCtx == NULL) {
		if (psH264EncRes->u32EncBitStreamLen > psDestCtx->u32DataLimit) {
			psDestCtx->u32DataSize = 0;
			*pu32RemainDataSize = psH264EncRes->u32EncBitStreamLen;
		}
		else {
			if (psH264EncRes->u32EncBitStreamLen) {
				memcpy((void *)psDestCtx->pu8DataBuf, psH264EncRes->pu8EncBitStreamBuf, psH264EncRes->u32EncBitStreamLen);
				psDestCtx->u32DataSize = psH264EncRes->u32EncBitStreamLen;
				psDestCtx->u64DataTime = psH264EncRes->u64EncBitStreamDataTime;
				psH264EncRes->u32EncBitStreamLen = 0;
				*pu32RemainDataSize = 0;
			}
		}

		return eNM_ERRNO_NONE;
	}
		
	pthread_mutex_lock(s_ptH264EncCodecMutex);

	if(psH264EncRes->bInit == false){
		eRet = NM_H264Enc_Init(psSrcCtx, psDestCtx, psH264EncRes);

		if(eRet != eNM_ERRNO_NONE){
			pthread_mutex_unlock(s_ptH264EncCodecMutex);
			return eRet;
		}
	}

	if(psDestCtx->u32DataLimit > psH264EncRes->u32EncBitStreamBufSize)
		pu8EncBSBuf = psDestCtx->pu8DataBuf;
	else
		pu8EncBSBuf = psH264EncRes->pu8EncBitStreamBuf;

	eRet = NM_H264Enc_Enc(psSrcCtx, pu8EncBSBuf, psH264EncRes);

	if(eRet != eNM_ERRNO_NONE){
		pthread_mutex_unlock(s_ptH264EncCodecMutex);
		return eRet;
	}

	u32EncDataLen = psH264EncRes->sH264EncParam.bitstream_size;
	
	if(pu8EncBSBuf == psH264EncRes->pu8EncBitStreamBuf){
			if(u32EncDataLen <= psDestCtx->u32DataLimit){
				memcpy(psDestCtx->pu8DataBuf, pu8EncBSBuf, u32EncDataLen);
			}
			else{
				psH264EncRes->u32EncBitStreamLen = u32EncDataLen;
				psH264EncRes->u64EncBitStreamDataTime = psSrcCtx->u64DataTime;

				*pu32RemainDataSize = u32EncDataLen;
				pthread_mutex_unlock(s_ptH264EncCodecMutex);
				return eNM_ERRNO_NONE;
			}
	}
		
	psDestCtx->u32DataSize = u32EncDataLen;
	psDestCtx->u64DataTime = psSrcCtx->u64DataTime;
	
	pthread_mutex_unlock(s_ptH264EncCodecMutex);
	
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
NM_H264Enc_CodecAttrGet(
	S_NM_VIDEO_CTX *psSrcCtx,
	S_NM_VIDEO_CTX *psDestCtx
)
{
	S_NM_H264_CTX_PARAM *psCtxParam = NULL;
	
	if(psDestCtx->pvParamSet){
		psCtxParam = (S_NM_H264_CTX_PARAM *)psDestCtx->pvParamSet;
		psCtxParam->eProfile = eNM_H264_BASELINE;
	}

	return eNM_ERRNO_NONE;
}

S_NM_CODECENC_VIDEO_IF g_sH264EncIF = {
	.pfnOpenCodec = NM_H264Enc_Open,
	.pfnCloseCodec = NM_H264Enc_Close,

	.pfnEncodeVideo = NM_H264Enc_Encode,
	.pfnCodecAttrGet = NM_H264Enc_CodecAttrGet,
};

