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

#include "VPE.h"

typedef enum {
	eH264DEC_COLOR_YUV422,
	eH264DEC_COLOR_YUV420P,	
}E_H264DEC_COLOR;

static pthread_mutex_t *s_ptH264CodecMutex = NULL;

typedef struct {
	int32_t i32CodecInst;
	bool bInit;
	E_H264DEC_COLOR eDecColor;
	uint8_t *pu8BSBuf;
	uint32_t u32BSBufLen;
	uint32_t u32VPETransBufSize;
	uint8_t *pu8VPETransBuf;
	uint8_t *pu8VPETransSrcBuf;
	uint64_t u64PrevFrameTime;
	
} S_H264DEC_RES;

static int32_t
NM_H264Dec_Init(
	S_NM_VIDEO_CTX *psSrcCtx,
	S_NM_VIDEO_CTX *psDestCtx,
	S_H264DEC_RES *psH264DecRes
)
{
	FAVC_DEC_PARAM      sDecParam;
	int32_t i32Ret = 0;
	uint8_t *pu8TempBSBuf = NULL;
	uint8_t *pu8TempVPEBuf = NULL;
	
	if(psH264DecRes->i32CodecInst < 0)
		return -1;

	memset(&sDecParam, 0, sizeof(FAVC_DEC_PARAM));
	sDecParam.u32API_version = H264VER;
	sDecParam.u32MaxWidth = (uint32_t)-1;
	sDecParam.u32MaxHeight = (uint32_t)-1;

	sDecParam.u32FrameBufferWidth = (uint32_t)-1;
	sDecParam.u32FrameBufferHeight = (uint32_t)-1;
	
	if((psSrcCtx->u32Width == psDestCtx->u32Width) &&
		(psSrcCtx->u32Height == psDestCtx->u32Height))
		psH264DecRes->eDecColor = eH264DEC_COLOR_YUV422;
	else
		psH264DecRes->eDecColor = eH264DEC_COLOR_YUV420P;

	if(psH264DecRes->eDecColor == eH264DEC_COLOR_YUV422)
		sDecParam.u32OutputFmt = 1; // 1->Packet YUV422 format, 0-> planar YUV420 foramt
	else
		sDecParam.u32OutputFmt = 0; // 1->Packet YUV422 format, 0-> planar YUV420 foramt

	if(H264_ioctl_ex(psH264DecRes->i32CodecInst, FAVC_IOCTL_DECODE_INIT,&sDecParam) < 0){
		NMLOG_ERROR("FAVC_IOCTL_DECODE_INIT: memory allocation failed \n");
		return -2;
	}
	
	uint32_t u32SrcImageSize = psSrcCtx->u32Width * psSrcCtx->u32Height * 2;
	
	//Allocate aligned 32 bitstream buffer
	pu8TempBSBuf = nv_malloc(u32SrcImageSize / 2, 32);
	psH264DecRes->u32BSBufLen = u32SrcImageSize / 2;
	
	if(pu8TempBSBuf == NULL){
		i32Ret = -3;
		goto NM_H264Dec_Init_fail;
	}
	
	if(psH264DecRes->eDecColor == eH264DEC_COLOR_YUV420P){
		//Allocate VPE transfer buffer
		if(psH264DecRes->u32VPETransBufSize < (u32SrcImageSize * 2)){
			pu8TempVPEBuf = malloc(u32SrcImageSize * 2);
			if(pu8TempVPEBuf == NULL){
				i32Ret = -4;
				goto NM_H264Dec_Init_fail;
			}
			
			psH264DecRes->pu8VPETransBuf = pu8TempVPEBuf;
			psH264DecRes->u32VPETransBufSize = u32SrcImageSize * 2;
		}

		if(VPE_Init()!= ERR_VPE_SUCCESS){
				i32Ret = -5;
				goto NM_H264Dec_Init_fail;
		}
	}
		
	psH264DecRes->pu8BSBuf = pu8TempBSBuf;
	psH264DecRes->bInit = true;
	return 0;

NM_H264Dec_Init_fail:	
	if(pu8TempBSBuf){
		nv_free(pu8TempBSBuf);
	}

	if(pu8TempVPEBuf)
		free(pu8TempVPEBuf);
	
	return i32Ret;
}

static ERRCODE
TriggerVPETrans(
	S_NM_VIDEO_CTX *psSrcCtx,
	S_NM_VIDEO_CTX *psDestCtx,
	uint8_t *pu8SrcBufAddr,
	uint8_t *pu8DstBufAddr
)
{
	S_VPE_TRANS_ATTR sVPETransAttr;
	S_VPE_TRANS_PARAM sVPETransParam;

	memset(&sVPETransParam, 0x00, sizeof(S_VPE_TRANS_PARAM));
	sVPETransParam.eVPEOP = VPE_OP_NORMAL;
	sVPETransParam.eVPEScaleMode = VPE_SCALE_DDA;//VPE_SCALE_BILINEAR
	sVPETransParam.eSrcFmt = VPE_SRC_PLANAR_YUV420;
	sVPETransParam.eDestFmt = VPE_DST_PACKET_YUV422;
	sVPETransParam.u32SrcImgWidth = psSrcCtx->u32Width;
	sVPETransParam.u32SrcImgHeight = psSrcCtx->u32Height;
	sVPETransParam.u32DestImgWidth = psDestCtx->u32Width;
	sVPETransParam.u32DestImgHeight = psDestCtx->u32Height;

	sVPETransAttr.psTransParam = &sVPETransParam;

	sVPETransAttr.pu8SrcBuf = pu8SrcBufAddr;
	sVPETransAttr.pu8DestBuf = pu8DstBufAddr;
	sVPETransAttr.u32SrcBufPhyAdr = (uint32_t)pu8SrcBufAddr;
	sVPETransAttr.u32DestBufPhyAdr = (uint32_t)pu8DstBufAddr;

	return VPE_Trans(&sVPETransAttr);
}


///////////////////////////////////////////////////////////////////////////////////////
static E_NM_ERRNO
NM_H264Dec_Open(
	S_NM_VIDEO_CTX *psDestCtx,
	void **ppCodecRes
)
{
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;
	S_H264DEC_RES *psH264DecRes = NULL;

	if (ppCodecRes == NULL) {
		return eNM_ERRNO_NULL_PTR;
	}

	*ppCodecRes = NULL;

	if(s_ptH264CodecMutex == NULL){
		s_ptH264CodecMutex = calloc(1, sizeof(pthread_mutex_t));
		if(s_ptH264CodecMutex == NULL)
			return eNM_ERRNO_MALLOC;
		if(pthread_mutex_init(s_ptH264CodecMutex, NULL) != 0){
			return eNM_ERRNO_OS;
		}
	}
	
	pthread_mutex_lock(s_ptH264CodecMutex);
	

	psH264DecRes = calloc(1, sizeof(S_H264DEC_RES));
	if(psH264DecRes == NULL){
		eRet = eNM_ERRNO_MALLOC;
		goto NM_H264Dec_Open_fail;
	}
	
	psH264DecRes->i32CodecInst = H264Dec_Open(); 

	if(psH264DecRes->i32CodecInst < 0){
		NMLOG_ERROR("Unable open H264 hardware decoder \n");
		eRet = eNM_ERRNO_CODEC_OPEN;
		goto NM_H264Dec_Open_fail;
	}
	
	pthread_mutex_unlock(s_ptH264CodecMutex);	


	*ppCodecRes = psH264DecRes;
	return eRet;

NM_H264Dec_Open_fail:
	
	if(psH264DecRes)
		free(psH264DecRes);
	
	pthread_mutex_unlock(s_ptH264CodecMutex);		
	return eRet;
}

static E_NM_ERRNO
NM_H264Dec_Close(
	void	**ppCodecRes
)
{
	if((ppCodecRes == NULL) || (*ppCodecRes == NULL))
		return eNM_ERRNO_NULL_PTR;
		
	S_H264DEC_RES *psH264DecRes = (S_H264DEC_RES *) *ppCodecRes;

	pthread_mutex_lock(s_ptH264CodecMutex);


	if(psH264DecRes->pu8VPETransBuf)
		free(psH264DecRes->pu8VPETransBuf);

	if(psH264DecRes->pu8BSBuf)
		nv_free(psH264DecRes->pu8BSBuf);
	
	if(psH264DecRes->i32CodecInst >= 0){
		H264Dec_Close_ex(psH264DecRes->i32CodecInst);
	}
	
	pthread_mutex_unlock(s_ptH264CodecMutex);

	free(psH264DecRes);
	*ppCodecRes = NULL;

	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
NM_H264Dec_Decode(
	S_NM_VIDEO_CTX *psSrcCtx,
	S_NM_VIDEO_CTX *psDestCtx,
	uint32_t *pu32RemainDataSize,
	void *pvCodecRes
)
{
	S_H264DEC_RES *psH264DecRes = (S_H264DEC_RES *)pvCodecRes;
	uint32_t u32DestCtxSize;
	uint32_t u32SrcImagePixel;
	ERRCODE eVPERet;
	BOOL bVPETrigger;
	FAVC_DEC_PARAM      sDecParam;	
	int32_t i32DecoderRet;
	
	if (psH264DecRes == NULL)
		return eNM_ERRNO_NULL_PTR;

	if(psH264DecRes->i32CodecInst < 0)
		return eNM_ERRNO_CODEC_OPEN;

	pthread_mutex_lock(s_ptH264CodecMutex);

	psDestCtx->u32DataSize = 0;
	*pu32RemainDataSize = 0;

	if(psH264DecRes->bInit == false){
		if(NM_H264Dec_Init(psSrcCtx, psDestCtx, psH264DecRes) != 0){
			pthread_mutex_unlock(s_ptH264CodecMutex);
			return eNM_ERRNO_IO;
		}
	}
	
	u32DestCtxSize = psDestCtx->u32Width * psDestCtx->u32Height *2 ;
	u32SrcImagePixel = psSrcCtx->u32Width * psSrcCtx->u32Height;
	
	if(psDestCtx->u32DataLimit != u32DestCtxSize){
		*pu32RemainDataSize = u32DestCtxSize;
		pthread_mutex_unlock(s_ptH264CodecMutex);
		return eNM_ERRNO_NONE;
	}

	//VPE transfer if H.264 decoded color is YUV420P. Scaler and color transfer to YUV422 
	bVPETrigger = FALSE;
	if(psH264DecRes->eDecColor == eH264DEC_COLOR_YUV420P){
		if(psH264DecRes->pu8VPETransSrcBuf){
			eVPERet = TriggerVPETrans(psSrcCtx, psDestCtx, psH264DecRes->pu8VPETransSrcBuf, psDestCtx->pu8DataBuf);
			if(eVPERet != ERR_VPE_SUCCESS){
				NMLOG_ERROR("Unalbe trigger VPE transfer Failed.ret=%x\n",eVPERet);
				psH264DecRes->pu8VPETransSrcBuf = NULL;
			}
			else{
				bVPETrigger = TRUE;
			}
		}
	}

	//Decode H.264 bitstram
	//here pu8VPETransSrcBuf will be used to pointer decoder destination buffer address
	if(psH264DecRes->eDecColor == eH264DEC_COLOR_YUV420P){
		//switch ping pong buffer
		if(psH264DecRes->pu8VPETransSrcBuf == psH264DecRes->pu8VPETransBuf){
			psH264DecRes->pu8VPETransSrcBuf = psH264DecRes->pu8VPETransBuf + (u32SrcImagePixel * 2);
		}
		else{
			psH264DecRes->pu8VPETransSrcBuf = psH264DecRes->pu8VPETransBuf;
		}
	}
	else{
		psH264DecRes->pu8VPETransSrcBuf = psDestCtx->pu8DataBuf;
	}

	//start decode
	// Set decoded raw image data address. [1][2] for YUV420 planar u, v
	sDecParam.pu8Display_addr[0] = (unsigned int)(psH264DecRes->pu8VPETransSrcBuf);
	sDecParam.pu8Display_addr[1] = (unsigned int)(psH264DecRes->pu8VPETransSrcBuf + u32SrcImagePixel);
	sDecParam.pu8Display_addr[2] = (unsigned int)(psH264DecRes->pu8VPETransSrcBuf + u32SrcImagePixel + (u32SrcImagePixel / 4));

	sDecParam.u32Pkt_size =	(unsigned int)psSrcCtx->u32DataSize;

	if(psH264DecRes->u32BSBufLen < sDecParam.u32Pkt_size)
		NMLOG_ERROR("Bit stream buffer < packet size \n");
		
	memcpy(psH264DecRes->pu8BSBuf, psSrcCtx->pu8DataBuf, psSrcCtx->u32DataSize);
	//			sDecParam.pu8Pkt_buf = (UINT8 *)((UINT32)psVideoPacket->pu8DataPtr | CACHE_BIT31);
	sDecParam.pu8Pkt_buf = (UINT8 *)((UINT32)psH264DecRes->pu8BSBuf | CACHE_BIT31);

	i32DecoderRet = H264_ioctl_ex(psH264DecRes->i32CodecInst, FAVC_IOCTL_DECODE_FRAME, &sDecParam);		

	if(i32DecoderRet != 0){
		NMLOG_ERROR("FAVC_IOCTL_DECODE_FRAME: Failed.ret=%x\n",i32DecoderRet);
		psH264DecRes->pu8VPETransSrcBuf = NULL;
	}

	if(sDecParam.got_picture == 0){
		NMLOG_ERROR("Decode video frame error \n");
		psH264DecRes->pu8VPETransSrcBuf = NULL;
	}
	
	//Wait VPE transfer done 
	if(psH264DecRes->eDecColor == eH264DEC_COLOR_YUV420P){
		if(bVPETrigger){
			VPE_WaitTransDone();
			psDestCtx->u64DataTime = psH264DecRes->u64PrevFrameTime;
			psDestCtx->u32DataSize = u32DestCtxSize;			
		}
		psH264DecRes->u64PrevFrameTime = psSrcCtx->u64DataTime;
	}
	else{
		if(psH264DecRes->pu8VPETransSrcBuf){
			psDestCtx->u64DataTime = psSrcCtx->u64DataTime;
			psDestCtx->u32DataSize = u32DestCtxSize;			
		}
	}

	pthread_mutex_unlock(s_ptH264CodecMutex);
	return eNM_ERRNO_NONE;	
}


S_NM_CODECDEC_VIDEO_IF g_sH264DecIF = {
	.pfnOpenCodec = NM_H264Dec_Open,
	.pfnCloseCodec = NM_H264Dec_Close,

	.pfnDecodeVideo = NM_H264Dec_Decode,
};

