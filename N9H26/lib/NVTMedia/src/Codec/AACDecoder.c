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

#include "NVTMedia.h"
#include "NVTMedia_SAL_OS.h"

#include "AACDecoder.h"

typedef struct {
	uint8_t *pu8TmpBuf;
	uint32_t u32TmpBufSize;
	uint64_t u64TmpDataTime;
} S_AACDEC_RES;

static pthread_mutex_t *s_ptAACCodecMutex = NULL;

//AAC codec buffer
#if defined (__GNUC__) && !(__CC_ARM)
__attribute__ ((aligned (32))) int g_i32InBuf[2048];
__attribute__ ((aligned (32))) int g_i32OutBuf[2048];
#else
__align(32) int g_i32InBuf[2048];
__align(32) int g_i32OutBuf[2048];
#endif

// ==================================================
// External function declaration
// ==================================================

extern bool								// [out]true / false: Success / Failed
AACDec_Initialize2(
	unsigned int	u32ObjectType,		// [out]Object type
	unsigned int	u32SampleRate,		// [out]Sample rate in Hz
	unsigned int	u32ChannelNum,		// [out]Channel number
	bool			bUseHWDec			// [in] Use H/W decoder if existed
);

extern bool								// [out]true/false: Successful/Failed
AACDec_DecodeFrame2(
	S_AACDEC_FRAME_INFO	*psFrameInfo,	// [in] Frame information
	unsigned char		*pu8Buf,		// [in] AAC bitstream
	unsigned int		u32BufSize		// [in] Byte size
);


///////////////////////////////////////////////////////////////////////////////////////
static E_NM_ERRNO
AACDec_Open(
	S_NM_AUDIO_CTX *psDestCtx,
	void **ppCodecRes
)
{
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;
	S_AACDEC_RES *psAACDecRes = NULL;
	E_NM_AAC_PROFILE eAACProfile = eNM_AAC_LC;

	*ppCodecRes = NULL;
	
	if(s_ptAACCodecMutex == NULL){
		s_ptAACCodecMutex = calloc(1, sizeof(pthread_mutex_t));
		if(s_ptAACCodecMutex == NULL)
			return eNM_ERRNO_MALLOC;
		if(pthread_mutex_init(s_ptAACCodecMutex, NULL) != 0){
			NMLOG_ERROR("AAC decode: Profile not support \n");
			return eNM_ERRNO_OS;
		}
	}
	
	if(psDestCtx->pvParamSet){
		S_NM_AAC_CTX_PARAM *psAACtxParm = (S_NM_AAC_CTX_PARAM *)psDestCtx->pvParamSet; 
		eAACProfile = psAACtxParm->eProfile;
	}
	
	if(eAACProfile != eNM_AAC_LC)
		return eNM_ERRNO_CODEC_TYPE;
	
	// Allocate AAC decoder resource
	psAACDecRes = (S_AACDEC_RES *)calloc(1, sizeof(S_AACDEC_RES));

	if (psAACDecRes == NULL) {
		return eNM_ERRNO_MALLOC;
	} // if	
	
	pthread_mutex_lock(s_ptAACCodecMutex);

	AACDec_CodecBuffer(&g_i32InBuf[0], &g_i32OutBuf[0]);
	
	// Initialize libnaacdec
	if (!AACDec_Initialize2(eAACProfile, psDestCtx->u32SampleRate,
							psDestCtx->u32Channel, true)) {
			NMLOG_ERROR("libnaacdec initialize failed: ObjectType %u, %u Hz, %u ch \n",
						  eAACProfile, psDestCtx->u32SampleRate, psDestCtx->u32Channel);


		eRet = eNM_ERRNO_CODEC_OPEN;
		goto AACDec_Open_fail;
	} // if
	
	psDestCtx->u32SamplePerBlock = 1024;
	
	pthread_mutex_unlock(s_ptAACCodecMutex);
	
	*ppCodecRes = psAACDecRes;
	
	return eNM_ERRNO_NONE;

AACDec_Open_fail:

	pthread_mutex_unlock(s_ptAACCodecMutex);

	
	if(psAACDecRes)
		free(psAACDecRes);
	
	return eRet;
}

static E_NM_ERRNO
AACDec_Close(
	void	**ppCodecRes
)
{
	if((ppCodecRes == NULL) || (*ppCodecRes == NULL))
		return eNM_ERRNO_NULL_PTR;

	S_AACDEC_RES *psAACDecRes = (S_AACDEC_RES *) *ppCodecRes;

	pthread_mutex_lock(s_ptAACCodecMutex);

	// Finalize libnaacdec
	AACDec_Finalize();	
	
	pthread_mutex_unlock(s_ptAACCodecMutex);
	
	free(psAACDecRes);
	
	*ppCodecRes = NULL;
	
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
AACDec_Decode(
	S_NM_AUDIO_CTX *psSrcCtx,
	S_NM_AUDIO_CTX *psDestCtx,
	uint32_t *pu32RemainDataSize,
	void *pvCodecRes
)
{
	S_AACDEC_RES *psAACDecRes = (S_AACDEC_RES *)pvCodecRes;
	uint32_t u32DecodeDataSize;
	uint8_t *pu8DecodeDataBuf;
	
	if (psAACDecRes == NULL)
		return eNM_ERRNO_NULL_PTR;
	
	psDestCtx->u32DataSize = 0;
	*pu32RemainDataSize = 0;
	
	if (psSrcCtx == NULL) {
		if (psAACDecRes->u32TmpBufSize > psDestCtx->u32DataLimit) {
			psDestCtx->u32DataSize = 0;
			*pu32RemainDataSize = psAACDecRes->u32TmpBufSize;
		}
		else {
			if (psAACDecRes->u32TmpBufSize) {
				memcpy((void *)psDestCtx->pu8DataBuf, psAACDecRes->pu8TmpBuf, psAACDecRes->u32TmpBufSize);
				psDestCtx->u32DataSize = psAACDecRes->u32TmpBufSize;
				psDestCtx->u64DataTime = psAACDecRes->u64TmpDataTime;
				free(psAACDecRes->pu8TmpBuf);
				psAACDecRes->pu8TmpBuf = NULL;
				psAACDecRes->u32TmpBufSize = 0;
				*pu32RemainDataSize = 0;
			}
		}
		return eNM_ERRNO_NONE;
	}

	pthread_mutex_lock(s_ptAACCodecMutex);
	
	// Decode a block of AAC data
	S_AACDEC_FRAME_INFO	sFrameInfo;

	if (!AACDec_DecodeFrame2(&sFrameInfo, (unsigned char *)(psSrcCtx->pu8DataBuf), psSrcCtx->u32DataSize)) {
		NMLOG_ERROR("libnaacdec error code 0x%08X \n", sFrameInfo.m_eError);
		pthread_mutex_unlock(s_ptAACCodecMutex);
		return eNM_ERRNO_DECODE;
	} // if
	
	pu8DecodeDataBuf = (uint8_t *)sFrameInfo.m_pcSample_buffer;
	u32DecodeDataSize = 2 * sFrameInfo.m_u32Samples * sFrameInfo.m_u32Channels;
	
	if (psDestCtx->u32DataLimit < u32DecodeDataSize) {
		if (u32DecodeDataSize > psAACDecRes->u32TmpBufSize) {
			if (psAACDecRes->pu8TmpBuf) {
				free(psAACDecRes->pu8TmpBuf);
				psAACDecRes->u32TmpBufSize = 0;
			}

			psAACDecRes->pu8TmpBuf = malloc(u32DecodeDataSize);

			if (psAACDecRes->pu8TmpBuf == NULL) {
				NMLOG_ERROR("AACDec_Decode no memory \n");
				pthread_mutex_unlock(s_ptAACCodecMutex);
				return eNM_ERRNO_MALLOC;
			}

			psAACDecRes->u32TmpBufSize = u32DecodeDataSize;
			psAACDecRes->u64TmpDataTime = psSrcCtx->u64DataTime;
		}

		psDestCtx->u32DataSize = 0;
		*pu32RemainDataSize = psAACDecRes->u32TmpBufSize;
		memcpy(psAACDecRes->pu8TmpBuf, pu8DecodeDataBuf, u32DecodeDataSize);
	}
	else {
		psDestCtx->u32DataSize = u32DecodeDataSize;
		psDestCtx->u64DataTime = psSrcCtx->u64DataTime;
		memcpy(psDestCtx->pu8DataBuf, pu8DecodeDataBuf, u32DecodeDataSize);
	}
		
	pthread_mutex_unlock(s_ptAACCodecMutex);
	
	return eNM_ERRNO_NONE;
}


S_NM_CODECDEC_AUDIO_IF g_sAACDecIF = {
	.pfnOpenCodec = AACDec_Open,
	.pfnCloseCodec = AACDec_Close,

	.pfnDecodeAudio = AACDec_Decode,
};
