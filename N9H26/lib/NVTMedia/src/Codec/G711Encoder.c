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

#define MILLISEC_PRE_BLOCK	100			//milli seconds pre each block

typedef struct {
	uint8_t *pu8TmpBuf;
	uint32_t u32TmpBufSize;
	uint64_t u64TmpDataTime;
} S_G711ENC_RES;

/////////////////////////////////////////////////////////////////////////////////
//			Alaw codec
/////////////////////////////////////////////////////////////////////////////////

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

static short seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF,
			    0x1FF, 0x3FF, 0x7FF, 0xFFF};


static short
search(
	short		val,
	short		*table,
	short		size)
{
	short		i;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

/*
 * AlawEncode() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * AlawEncode() accepts an 16-bit integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */

static unsigned char
AlawEncode(
	short		pcm_val)	/* 2's complement (16-bit range) */
{
	short		mask;
	short		seg;
	unsigned char	aval;

	pcm_val = pcm_val >> 3;

	if (pcm_val >= 0) {
		mask = 0xD5;		/* sign (7th) bit = 1 */
	} else {
		mask = 0x55;		/* sign bit = 0 */
		pcm_val = -pcm_val - 1;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_aend, 8);

	/* Combine the sign, segment, and quantization bits. */

	if (seg >= 8)		/* out of range, return maximum value. */
		return (unsigned char) (0x7F ^ mask);
	else {
		aval = (unsigned char) seg << SEG_SHIFT;
		if (seg < 2)
			aval |= (pcm_val >> 1) & QUANT_MASK;
		else
			aval |= (pcm_val >> seg) & QUANT_MASK;
		return (aval ^ mask);
	}
}

////////////////////////////////////////////////////////////////////////////////////
//	Ulaw codec
////////////////////////////////////////////////////////////////////////////////////

#define	BIAS		(0x84)		// Bias for linear code.
#define CLIP            8159

static short seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF,
							0x3FF, 0x7FF, 0xFFF, 0x1FFF
						   };

static unsigned char
UlawEncode(
	short		pcm_val) {	/* 2's complement (16-bit range) */
	short		mask;
	short		seg;
	unsigned char	uval;
	/* Get the sign and the magnitude of the value. */
	pcm_val = pcm_val >> 2;

	if (pcm_val < 0) {
		pcm_val = -pcm_val;
		mask = 0x7F;
	}
	else {
		mask = 0xFF;
	}

	if (pcm_val > CLIP) pcm_val = CLIP;		/* clip the magnitude */

	pcm_val += (BIAS >> 2);
	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_uend, 8);

	/*
	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	if (seg >= 8)		/* out of range, return maximum value. */
		return (unsigned char)(0x7F ^ mask);
	else {
		uval = (unsigned char)(seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
		return (uval ^ mask);
	}
}

///////////////////////////////////////////////////////////////////////////////////////

static E_NM_ERRNO
G711Enc_Open(
	S_NM_AUDIO_CTX *psDestCtx,
	void **ppCodecRes
)
{
	S_G711ENC_RES *psG711Res;

	if (ppCodecRes == NULL) {
		return eNM_ERRNO_NULL_PTR;
	}

	*ppCodecRes = NULL;
	psG711Res = calloc(1, sizeof(S_G711ENC_RES));

	if (psG711Res == NULL)
		return eNM_ERRNO_MALLOC;

	*ppCodecRes = psG711Res;

	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
G711Enc_Close(
	void	**ppCodecRes
)
{
	if((ppCodecRes == NULL) || (*ppCodecRes == NULL))
		return eNM_ERRNO_NULL_PTR;
		
	S_G711ENC_RES *psG711Res = (S_G711ENC_RES *) *ppCodecRes;

	if(psG711Res->pu8TmpBuf)
		free(psG711Res->pu8TmpBuf);
	
	free(psG711Res);
	*ppCodecRes = NULL;
	return eNM_ERRNO_NONE;

}

E_NM_ERRNO
G711Enc_Encode(
	S_NM_AUDIO_CTX *psSrcCtx,
	S_NM_AUDIO_CTX *psDestCtx,
	uint32_t *pu32RemainDataSize,
	void *pvCodecRes
)
{
	unsigned char	*pbyG711DataPtr = NULL;
	unsigned int	u32G711DataSize;
	uint32_t 		u32PCMDataSize;
	short			*pi16PCMDataPtr = NULL;
	S_G711ENC_RES *psRes = (S_G711ENC_RES *)pvCodecRes;

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


	pi16PCMDataPtr = (short*)(psSrcCtx->pu8DataBuf);
	u32PCMDataSize = psSrcCtx->u32DataSize;
	pbyG711DataPtr = (unsigned char*)(psDestCtx->pu8DataBuf);
	u32G711DataSize = u32PCMDataSize / 2;
	*pu32RemainDataSize = 0;
	psDestCtx->u32DataSize = 0;
	
	if (psDestCtx->u32DataLimit < u32G711DataSize) {
		if (u32G711DataSize > psRes->u32TmpBufSize) {
			if (psRes->pu8TmpBuf) {
				free(psRes->pu8TmpBuf);
				psRes->u32TmpBufSize = 0;
			}

			psRes->pu8TmpBuf = malloc(u32G711DataSize);

			if (psRes->pu8TmpBuf == NULL) {
				NMLOG_ERROR("G711Codec_ulawDecode no memory \n");
				return eNM_ERRNO_MALLOC;
			}

			psRes->u32TmpBufSize = u32G711DataSize;
			psRes->u64TmpDataTime = psSrcCtx->u64DataTime;
		}

		psDestCtx->u32DataSize = 0;
		*pu32RemainDataSize = u32G711DataSize;
		pbyG711DataPtr = (unsigned char *)psRes->pu8TmpBuf;
	}
	else {
		psDestCtx->u32DataSize = u32G711DataSize;
		psDestCtx->u64DataTime = psSrcCtx->u64DataTime;
	}
	
	while (u32G711DataSize--) {
		if(psDestCtx->eAudioType == eNM_CTX_AUDIO_ALAW)
			*pbyG711DataPtr++ = AlawEncode(*pi16PCMDataPtr++);
		else
			*pbyG711DataPtr++ = UlawEncode(*pi16PCMDataPtr++);
	}	
	
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
G711Enc_CodecAttrGet(
	S_NM_AUDIO_CTX *psSrcCtx,
	S_NM_AUDIO_CTX *psDestCtx
)
{
	
	psDestCtx->u32SamplePerBlock = psSrcCtx->u32SampleRate * MILLISEC_PRE_BLOCK / 1000;
	psSrcCtx->u32SamplePerBlock = psDestCtx->u32SamplePerBlock;

	return eNM_ERRNO_NONE;
}

S_NM_CODECENC_AUDIO_IF g_sG711EncIF = {
	.pfnOpenCodec = G711Enc_Open,
	.pfnCloseCodec = G711Enc_Close,

	.pfnEncodeAudio = G711Enc_Encode,
	.pfnCodecAttrGet = G711Enc_CodecAttrGet,
};
