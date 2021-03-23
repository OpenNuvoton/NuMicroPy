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

#ifndef __NVTMEDIA_CODEC_IF_H__
#define __NVTMEDIA_CODEC_IF_H__
// ==================================================
// Codec interface
// ==================================================

/**
 *	- video encoder/decoder open 
 *
 * 	@param[in\out]		psDestCtx	video destination context
 * 	@param[out]	ppCodecRes		Pointer to codec resource
 * 								- NULL if failed
 *
 * 	@return	Error code
 *
 *
 */


typedef	E_NM_ERRNO
(*PFN_NM_CODEC_VIDEO_OPEN)(
	S_NM_VIDEO_CTX *psDestCtx,
	void **ppCodecRes
);


/**
 *	- audio encoder/decoder open 
 *
 * 	@param[in\out]		psDestCtx	audio destination context
 * 	@param[out]	ppCodecRes		Pointer to codec resource
 * 								- NULL if failed
 *
 * 	@return	Error code
 *
 *
 */


typedef	E_NM_ERRNO
(*PFN_NM_CODEC_AUDIO_OPEN)(
	S_NM_AUDIO_CTX *psDestCtx,
	void **ppCodecRes
);

/**
 *	- video encode 
 *
 * 	@param[in]		psSrcCtx		video source context
 * 	@param[out]		psDestCtx		video destination context
 * 	@param[in]		pvCodecRes		Pointer to codec resource
 *
 * 	@return	Error code
 *
 *
 */

typedef	E_NM_ERRNO
(*PFN_NM_CODEC_VIDEO_ENCODE)(
	S_NM_VIDEO_CTX *psSrcCtx,
	S_NM_VIDEO_CTX *psDestCtx,
	uint32_t *pu32RemainDataSize,
	void *pvCodecRes
);


/**
 *	- get video codec attribute, used before record media open  
 *
 * 	@param[in]		psSrcCtx		video source context
 * 	@param[out]		psDestCtx		video destination context
 *
 * 	@return	Error code
 *
 *
 */

typedef	E_NM_ERRNO
(*PFN_NM_CODEC_VIDEO_CODEC_ATTR_GET)(
	S_NM_VIDEO_CTX *psSrcCtx,
	S_NM_VIDEO_CTX *psDestCtx
);


/**
 *	- audio encode 
 *
 * 	@param[in]		psAudioSrcCtx	audio source context
 * 	@param[out]		psAudioDestCtx	audio destination context
 * 	@param[in]		pvCodecRes		Pointer to codec resource
 *
 * 	@return	Error code
 *
 *
 */

typedef	E_NM_ERRNO
(*PFN_NM_CODEC_AUDIO_ENCODE)(
	S_NM_AUDIO_CTX *psSrcCtx,
	S_NM_AUDIO_CTX *psDestCtx,
	uint32_t *pu32RemainDataSize,
	void *pvCodecRes
);

/**
 *	- get audio codec attribute, used before record media open  
 *
 * 	@param[in]		psSrcCtx		video source context
 * 	@param[out]		psDestCtx		video destination context
 *
 * 	@return	Error code
 *
 *
 */

typedef	E_NM_ERRNO
(*PFN_NM_CODEC_AUDIO_CODEC_ATTR_GET)(
	S_NM_AUDIO_CTX *psSrcCtx,
	S_NM_AUDIO_CTX *psDestCtx
);


/**
 *	- video decode 
 *
 * 	@param[in]		psSrcCtx		video source context
 * 	@param[out]		psDestCtx		video destination context
 * 	@param[in]		pvCodecRes		Pointer to codec resource
 *
 * 	@return	Error code
 *
 *
 */

typedef	E_NM_ERRNO
(*PFN_NM_CODEC_VIDEO_DECODE)(
	S_NM_VIDEO_CTX *psSrcCtx,
	S_NM_VIDEO_CTX *psDestCtx,
	uint32_t *pu32RemainDataSize,
	void *pvCodecRes
);


/**
 *	- audio decode 
 *
 * 	@param[in]		psAudioSrcCtx	audio source context
 * 	@param[out]		psAudioDestCtx	audio destination context
 * 	@param[in]		pvCodecRes		Pointer to codec resource
 *
 * 	@return	Error code
 *
 *
 */

typedef	E_NM_ERRNO
(*PFN_NM_CODEC_AUDIO_DECODE)(
	S_NM_AUDIO_CTX *psSrcCtx,
	S_NM_AUDIO_CTX *psDestCtx,
	uint32_t *pu32RemainDataSize,
	void *pvCodecRes
);


/**	@brief	Close codec
 *
 * 	@param[in]	ppCodecRes		Pointer to codec resource
 * 	@param[out]	ppCodecRes		*ppMediaRes = NULL if successful to close the codec
 *
 * 	@return	Error code
 *
 *	@note	This function will free codec resource.
 *
 */

typedef	E_NM_ERRNO
(*PFN_NM_CODEC_CLOSE)(
	void	**ppCodecRes
);

// ==================================================
// Codec encode interface
// ==================================================

typedef struct S_NM_CODECENC_VIDEO_IF {

	PFN_NM_CODEC_VIDEO_OPEN             pfnOpenCodec;
	PFN_NM_CODEC_CLOSE                  pfnCloseCodec;

	PFN_NM_CODEC_VIDEO_ENCODE           pfnEncodeVideo;
	PFN_NM_CODEC_VIDEO_CODEC_ATTR_GET		pfnCodecAttrGet;
} S_NM_CODECENC_VIDEO_IF;


typedef struct S_NM_CODECENC_AUDIO_IF {

	PFN_NM_CODEC_AUDIO_OPEN             pfnOpenCodec;
	PFN_NM_CODEC_CLOSE                  pfnCloseCodec;

	PFN_NM_CODEC_AUDIO_ENCODE           pfnEncodeAudio;
	PFN_NM_CODEC_AUDIO_CODEC_ATTR_GET		pfnCodecAttrGet;
} S_NM_CODECENC_AUDIO_IF;

// ==================================================
// Codec decode interface
// ==================================================

typedef struct S_NM_CODECDEC_VIDEO_IF {

	PFN_NM_CODEC_VIDEO_OPEN             pfnOpenCodec;
	PFN_NM_CODEC_CLOSE                  pfnCloseCodec;

	PFN_NM_CODEC_VIDEO_DECODE           pfnDecodeVideo;
} S_NM_CODECDEC_VIDEO_IF;


typedef struct S_NM_CODECDEC_AUDIO_IF {

	PFN_NM_CODEC_AUDIO_OPEN             pfnOpenCodec;
	PFN_NM_CODEC_CLOSE                  pfnCloseCodec;

	PFN_NM_CODEC_AUDIO_DECODE           pfnDecodeAudio;
} S_NM_CODECDEC_AUDIO_IF;

extern S_NM_CODECDEC_VIDEO_IF g_sH264DecIF;

extern S_NM_CODECDEC_AUDIO_IF g_sG711DecIF;
extern S_NM_CODECDEC_AUDIO_IF g_sAACDecIF;

extern S_NM_CODECENC_VIDEO_IF g_sH264EncIF;

extern S_NM_CODECENC_AUDIO_IF g_sG711EncIF;
extern S_NM_CODECENC_AUDIO_IF g_sAACEncIF;
#endif
