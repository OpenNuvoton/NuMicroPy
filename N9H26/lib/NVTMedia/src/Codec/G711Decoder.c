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

typedef struct {
	uint8_t *pu8TmpBuf;
	uint32_t u32TmpBufSize;
	uint64_t u64TmpDataTime;
} S_G711DEC_RES;

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */
#define	BIAS		(0x84)		// Bias for linear code.

static short
UlawDecode(
	unsigned char	u_val
) {
	short	t;
	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;
	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;
	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

/*
 * AlawDecode() - Convert an A-law value to 16-bit linear PCM
 *
 */
static short
AlawDecode(
	unsigned char	a_val)
{
	short		t;
	short		seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}


///////////////////////////////////////////////////////////////////////////////////////
static E_NM_ERRNO
G711Dec_Open(
	S_NM_AUDIO_CTX *psDestCtx,
	void **ppCodecRes
)
{
	S_G711DEC_RES *psG711Res;

	if (ppCodecRes == NULL) {
		return eNM_ERRNO_NULL_PTR;
	}

	*ppCodecRes = NULL;
	psG711Res = calloc(1, sizeof(S_G711DEC_RES));

	if (psG711Res == NULL)
		return eNM_ERRNO_MALLOC;

	*ppCodecRes = psG711Res;
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
G711Dec_Close(
	void	**ppCodecRes
)
{
	if((ppCodecRes == NULL) || (*ppCodecRes == NULL))
		return eNM_ERRNO_NULL_PTR;
		
	S_G711DEC_RES *psG711Res = (S_G711DEC_RES *) *ppCodecRes;

	if(psG711Res->pu8TmpBuf)
		free(psG711Res->pu8TmpBuf);
	
	free(psG711Res);
	*ppCodecRes = NULL;
	return eNM_ERRNO_NONE;
}


static E_NM_ERRNO
G711Dec_Decode(
	S_NM_AUDIO_CTX *psSrcCtx,
	S_NM_AUDIO_CTX *psDestCtx,
	uint32_t *pu32RemainDataSize,
	void *pvCodecRes
)
{
	unsigned char	*pbyG711DataPtr = NULL;
	unsigned int	u32G711DataSize;
	uint32_t 		u32PCMDataSize;
	short			*pi16PCMDataPtr = (short*)(psDestCtx->pu8DataBuf);
	S_G711DEC_RES *psRes = (S_G711DEC_RES *)pvCodecRes;

	if (psRes == NULL)
		return eNM_ERRNO_NULL_PTR;

	if (psSrcCtx == NULL) {
		if (psRes->u32TmpBufSize > psDestCtx->u32DataLimit) {
			psDestCtx->u32DataSize = 0;
			*pu32RemainDataSize = psRes->u32TmpBufSize;
		}
		else {
			if (psRes->u32TmpBufSize) {
				memcpy((void *)psDestCtx->pu8DataBuf, psRes->pu8TmpBuf, psRes->u32TmpBufSize);
				psDestCtx->u32DataSize = psRes->u32TmpBufSize;
				psDestCtx->u64DataTime = psRes->u64TmpDataTime;
				free(psRes->pu8TmpBuf);
				psRes->pu8TmpBuf = NULL;
				psRes->u32TmpBufSize = 0;
				*pu32RemainDataSize = 0;
			}
		}

		return eNM_ERRNO_NONE;
	}

	pbyG711DataPtr = (unsigned char*)(psSrcCtx->pu8DataBuf);
	u32G711DataSize = psSrcCtx->u32DataSize;
	u32PCMDataSize = u32G711DataSize * 2;
	*pu32RemainDataSize = 0;
	psDestCtx->u32DataSize = 0;

	if (psDestCtx->u32DataLimit < u32PCMDataSize) {
		if (u32PCMDataSize > psRes->u32TmpBufSize) {
			if (psRes->pu8TmpBuf) {
				free(psRes->pu8TmpBuf);
				psRes->u32TmpBufSize = 0;
			}

			psRes->pu8TmpBuf = malloc(u32PCMDataSize);

			if (psRes->pu8TmpBuf == NULL) {
				NMLOG_ERROR("G711Codec no memory \n");
				return eNM_ERRNO_MALLOC;
			}

			psRes->u32TmpBufSize = u32PCMDataSize;
			psRes->u64TmpDataTime = psSrcCtx->u64DataTime;
		}

		psDestCtx->u32DataSize = 0;
		*pu32RemainDataSize = u32PCMDataSize;
		pi16PCMDataPtr = (short*)psRes->pu8TmpBuf;
	}
	else {
		psDestCtx->u32DataSize = u32PCMDataSize;
		psDestCtx->u64DataTime = psSrcCtx->u64DataTime;
	}
	
	while (u32G711DataSize--) {
		if(psSrcCtx->eAudioType == eNM_CTX_AUDIO_ALAW)
			*pi16PCMDataPtr++ = AlawDecode(*pbyG711DataPtr++);
		else
			*pi16PCMDataPtr++ = UlawDecode(*pbyG711DataPtr++);
	}

	return eNM_ERRNO_NONE;
}


S_NM_CODECDEC_AUDIO_IF g_sG711DecIF = {
	.pfnOpenCodec = G711Dec_Open,
	.pfnCloseCodec = G711Dec_Close,

	.pfnDecodeAudio = G711Dec_Decode,
};
