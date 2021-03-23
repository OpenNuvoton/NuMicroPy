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

#include "AACEncoder.h"
// ==================================================
// Constant definition
// ==================================================

//#define NM_AACENC_MULTIOPEN				// Define it to allow multiple open
#define DEF_MDCT_BUF_SRAM

enum {
	eNM_AACENC_DEFAULT_QUALITY	= 90,	// 1 ~ 999
	eNM_AACENC_FRAME_SIZE		= 1024,	// Samples per channel in frame
}; // enum

// ==================================================
// Type declaration
// ==================================================

// AAC encoder resource
typedef struct S_AACENC_RES {

	// Private resource
	uint32_t	u32Channel;			// Channel number of dest context
	uint32_t	u32SampleRate;			// Sample rate of dest context
} S_AACENC_RES;

static pthread_mutex_t *s_ptAACEncCodecMutex = NULL;

#ifdef NM_AACENC_MULTIOPEN
static uint32_t s_u32ReferCnt = 0;
#endif

#if !defined (DEF_MDCT_BUF_SRAM)
#if defined (__GNUC__) && !(__CC_ARM)
static __attribute__ ((aligned (32))) int s_ai32MDCTBuf[2048];
#else
static __align(32) int s_ai32MDCTBuf[2048];
#endif
#else
static int *s_ai32MDCTBuf = (int *)0xFF000000; 
#endif

///////////////////////////////////////////////////////////////////////////////////////

static E_NM_ERRNO
AACEnc_Close(
	void	**ppCodecRes
)
{
	// Check parameter
	if (!ppCodecRes || !*ppCodecRes)
		return eNM_ERRNO_NULL_PTR;

//	S_AACENC_RES	*psAACEncRes = *ppCodecRes;

	// Finalize libnaacenc
#ifdef NM_AACENC_MULTIOPEN
	pthread_mutex_lock(s_ptAACEncCodecMutex);

	if (s_u32ReferCnt == 1)
#endif
	{
		AACEnc_Finalize();
		NMLOG_INFO("libnaacenc finalized \n");
	}

#ifdef NM_AACENC_MULTIOPEN
	s_u32ReferCnt--;

	if (s_u32ReferCnt > 0){
		NMLOG_INFO("Done, ReferCnt %u \n", s_u32ReferCnt);
	}
	else
#endif
		NMLOG_INFO("Done \n");

	pthread_mutex_unlock(s_ptAACEncCodecMutex);

	// Free AAC encoder resource
	free(*ppCodecRes);
	*ppCodecRes = NULL;
	return eNM_ERRNO_NONE;
} // AACEnc_Close()



static E_NM_ERRNO
AACEnc_Open(
	S_NM_AUDIO_CTX *psDestCtx,
	void **ppCodecRes
)
{
	S_AACENC_RES	*psAACEncRes = NULL;
	
	if (!psDestCtx || !ppCodecRes)
		return eNM_ERRNO_NULL_PTR;
	
	*ppCodecRes = NULL;
	
	if (!psDestCtx->u32BitRate || !psDestCtx->u32Channel ||
			!psDestCtx->u32SampleRate) {
			NMLOG_ERROR("Bit rate, channel number, and sample rate cannot be zero.");
		return eNM_ERRNO_CTX;
	} // if

	if(s_ptAACEncCodecMutex == NULL){
		s_ptAACEncCodecMutex = calloc(1, sizeof(pthread_mutex_t));
		if(s_ptAACEncCodecMutex == NULL)
			return eNM_ERRNO_MALLOC;
		if(pthread_mutex_init(s_ptAACEncCodecMutex, NULL) != 0){
			return eNM_ERRNO_OS;
		}
	}	

#ifndef NM_AACENC_MULTIOPEN
	// Non-blocked check AACEnc_Open re-entry because libnaacenc is not thread-safe
	if (pthread_mutex_trylock(s_ptAACEncCodecMutex) != 0) {
		NMLOG_ERROR("Not support multiple open");
		return eNM_ERRNO_BUSY;
	} // if
#endif
	
	// Allocate AAC encoder resource
	psAACEncRes = (S_AACENC_RES *)calloc(1, sizeof(S_AACENC_RES));
	
	if(psAACEncRes == NULL){
#ifndef NM_AACENC_MULTIOPEN
		pthread_mutex_unlock(s_ptAACEncCodecMutex);
#endif
		return eNM_ERRNO_MALLOC;
	}
	
	// Initialize AAC encoder resource
	psAACEncRes->u32Channel = psDestCtx->u32Channel;
	psAACEncRes->u32SampleRate = psDestCtx->u32SampleRate;
	*ppCodecRes = psAACEncRes;
	
#ifdef NM_AACENC_MULTIOPEN
	pthread_mutex_lock(s_ptAACEncCodecMutex);

	if (s_u32ReferCnt > 0) {
		s_u32ReferCnt++;
		pthread_mutex_unlock(s_ptAACEncCodecMutex);
		*ppCodecRes = psAACEncRes;
		NMLOG_INFO("Done, ReferCnt %u \n", s_u32ReferCnt);
		return eNM_ERRNO_NONE;
	} // if
#endif	
	
	// Initialize libnaacenc
	S_AACENC	sEncoder;

	sEncoder.m_u32SampleRate = psDestCtx->u32SampleRate;
	sEncoder.m_u32ChannelNum = psDestCtx->u32Channel;
	sEncoder.m_u32BitRate = psDestCtx->u32BitRate * sEncoder.m_u32ChannelNum;
	sEncoder.m_u32Quality = eNM_AACENC_DEFAULT_QUALITY;
	sEncoder.m_bUseAdts = false;
	sEncoder.m_bUseMidSide = false;
	sEncoder.m_bUseTns = false;	
	
	const E_AACENC_ERROR	eEncError = AACEnc_Initialize(&sEncoder, true);

	if (eEncError != eAACENC_ERROR_NONE) {
		NMLOG_ERROR("libnaacenc initialize failed: Error code 0x%08x", eEncError);
		
		// Finalize libnaacenc and free AAC encoder resource
#ifdef NM_AACENC_MULTIOPEN
		pthread_mutex_unlock(s_ptAACEncCodecMutex);
#endif
		return AACEnc_Close(ppCodecRes);
	} // if

	AACEnc_CodecBuffer(s_ai32MDCTBuf);

#ifdef NM_AACENC_MULTIOPEN
	s_u32ReferCnt++;
	pthread_mutex_unlock(s_ptAACEncCodecMutex);
#endif
	
	return eNM_ERRNO_NONE;
}


static E_NM_ERRNO
AACEnc_CodecAttrGet(
	S_NM_AUDIO_CTX *psSrcCtx,
	S_NM_AUDIO_CTX *psDestCtx
)
{
	S_NM_AAC_CTX_PARAM *psCtxParam = NULL;
	
	psDestCtx->u32SamplePerBlock = eNM_AACENC_FRAME_SIZE;
	psSrcCtx->u32SamplePerBlock = psDestCtx->u32SamplePerBlock;

	if(psDestCtx->pvParamSet){
		psCtxParam = (S_NM_AAC_CTX_PARAM *)psDestCtx->pvParamSet;
		psCtxParam->eProfile = eNM_AAC_LC;
	}
		
	return eNM_ERRNO_NONE;
}


E_NM_ERRNO
AACEnc_Encode(
	S_NM_AUDIO_CTX *psSrcCtx,
	S_NM_AUDIO_CTX *psDestCtx,
	uint32_t *pu32RemainDataSize,
	void *pvCodecRes
)
{
	S_AACENC_RES *psRes = (S_AACENC_RES *)pvCodecRes;

	if (psRes == NULL)
		return eNM_ERRNO_NULL_PTR;

	const E_AACENC_ERROR	eEncError = AACEnc_EncodeFrame((short	*)psSrcCtx->pu8DataBuf,
										(char *)psDestCtx->pu8DataBuf, psDestCtx->u32DataLimit,
										(int *)(&psDestCtx->u32DataSize));

	if (eEncError != eAACENC_ERROR_NONE) {
		NMLOG_ERROR("libnaacenc error code 0x%08X", eEncError);
		psDestCtx->u32DataSize = 0;
		return eNM_ERRNO_ENCODE;
	} // if

	if (pu32RemainDataSize)
		*pu32RemainDataSize = 0;

	psDestCtx->u64DataTime = psSrcCtx->u64DataTime;
	// Update AAC encoder attribute
	return eNM_ERRNO_NONE;
}

S_NM_CODECENC_AUDIO_IF g_sAACEncIF = {
	.pfnOpenCodec = AACEnc_Open,
	.pfnCloseCodec = AACEnc_Close,

	.pfnEncodeAudio = AACEnc_Encode,
	.pfnCodecAttrGet = AACEnc_CodecAttrGet,
};
