/***************************************************************************//**
 * @file     IMAAdpcmEncoder.c
 * @brief    IMA ADPCM encoder
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IMAAdpcmCodec.h"
#include "IMAAdpcmTable.h"


static
char *									// [out]New outdata pointer
IMAAdpcm_Encode(
	int16_t				*indata,
	char 				*outdata,
	uint16_t			len,
	S_IMAADPCM_ENCSTATE	*state,
	uint8_t				u8ChannelIndex,
	BOOL				bFreshBlock		// [in] TRUE if force to fresh block
) {
	_S_IMAADPCM_ENCSTATE	*pstate = &(state->m_asPrivateState[u8ChannelIndex]);
    int PredSamp = pstate->m_i16PrevValue;			/* Predicted output value */
    int	step = StepTable[pstate->m_i8StepIndex];	/* Stepsize */

    for ( ; len > 0; len--) {
	    int SampX;		/* Current input sample value */
    	int sign;		/* Current adpcm sign bit */
	    int delta;		/* Current adpcm output value */
    	int Diff;		/* Difference between SampX and PredSamp */
	    int vpdiff;		/* Current change to PredSamp */
 
		//SampX = *indata++; // Axl
		SampX = *indata;
		indata += state->m_u8Channels;

		/* Step 1 - compute difference with previous value */
		Diff = SampX - PredSamp;
		// Set the sign bit
		if (Diff < 0) {
			sign = 8;
			Diff = (-Diff);
		} // if
		else
			sign = 0;

		/* Step 2 - Divide and clamp */
		/* Note:
		** This code *approximately* computes:
		**    delta = Diff*4/step;
		**    vpdiff = (delta+0.5)*step/4;
		** but in shift step bits are dropped. The net result of this is
		** that even if you have fast mul/div hardware you cannot put it to
		** good use since the fixup would be too expensive.
		*/
		// Set the rest of the code
		delta = 0;
		vpdiff = (step >> 3);
		
		if (Diff >= step) {
			delta = 4;
			Diff -= step;
			vpdiff += step;
		} // if

		step >>= 1;
		if (Diff >= step) {
			delta |= 2;
			Diff -= step;
			vpdiff += step;
		} // if
		
		step >>= 1;
		if (Diff >= step) {
			delta |= 1;
			vpdiff += step;
		} // if

		/* Step 3 - Update previous value */
		if (sign)
		  PredSamp -= vpdiff;
		else
		  PredSamp += vpdiff;

		/* Step 4 - Clamp previous value to 16 bits */
		if (PredSamp > 32767)
		  PredSamp = 32767;
		else if (PredSamp < -32768)
		  PredSamp = -32768;

		/* Step 5 - Assemble value, update index and step values */
		delta |= sign;
		
		pstate->m_i8StepIndex += IndexTable[delta];
		if (pstate->m_i8StepIndex < 0)
			pstate->m_i8StepIndex = 0;
		if (pstate->m_i8StepIndex > 88)
			pstate->m_i8StepIndex = 88;
		step = StepTable[pstate->m_i8StepIndex];

		/* Step 6 - Output value */			
		if (pstate->m_u8BufferStep) {
			if(state->m_eFormat == eIMAADPCM_IMA4)
				*outdata = ((delta << 4) & 0xf0) | pstate->m_i16OutputBuffer;
			else
				*outdata = pstate->m_i16OutputBuffer | (delta & 0x0f);
			
			if ((++pstate->m_u16Count) % IMAADPCM_CHANNEL_DATA_SIZE == 0) // Axl
				outdata += (state->m_u8Channels - 1) * IMAADPCM_CHANNEL_DATA_SIZE + 1;
			else
				outdata++;
		} // if
		else {
			if(state->m_eFormat == eIMAADPCM_IMA4)
				pstate->m_i16OutputBuffer = (delta & 0x0f);
			else
				pstate->m_i16OutputBuffer = ((delta << 4) & 0xf0);
		}
		
		pstate->m_u8BufferStep = !pstate->m_u8BufferStep;
    } // for

    /* Output last step, if needed */
	if (bFreshBlock)
		if (pstate->m_u8BufferStep)
			*outdata++ = pstate->m_i16OutputBuffer;

	pstate->m_i16PrevValue = PredSamp;
	return outdata;
} // IMAAdpcm_Encode()


// Encode a block of samples
BOOL 
IMAAdpcm_EncodeBlock(
	int16_t				*pi16InData,			// [in] Input decoded samples
	char				*pbyOutData,		// [in] Output encoded data
	uint16_t				u16SampleCount,		// [in] Sample count of pi16InData
	S_IMAADPCM_ENCSTATE	*psEncState			// [in/out] IMA ADPCM encoder state
) {
	uint8_t	u8ChannelIndex;

	for (u8ChannelIndex = 0; u8ChannelIndex < psEncState->m_u8Channels; u8ChannelIndex++) {
		_S_IMAADPCM_ENCSTATE	*psPrivateState
									= &(psEncState->m_asPrivateState[u8ChannelIndex]);

		// Generate block header
		uint16_t		u16Temp = (uint16_t)pi16InData[u8ChannelIndex];
		const int8_t	i8PrevStepIndex = psPrivateState->m_i8StepIndex;

		if (psEncState->m_eFormat == eIMAADPCM_IMA4) {
			// IMA4 format
			pbyOutData[IMAADPCM_HEADER_SIZE * u8ChannelIndex]	  = (char)(u16Temp & 0xFF);
			pbyOutData[IMAADPCM_HEADER_SIZE * u8ChannelIndex + 1] = (char)(u16Temp >> 8);
			pbyOutData[IMAADPCM_HEADER_SIZE * u8ChannelIndex + 2] = i8PrevStepIndex;
			pbyOutData[IMAADPCM_HEADER_SIZE * u8ChannelIndex + 3] = 0;	// reserved;
		
			// Encode block data
			memset(psPrivateState, 0, sizeof(_S_IMAADPCM_ENCSTATE));
			psPrivateState->m_i16PrevValue = pi16InData[u8ChannelIndex];
			psPrivateState->m_i8StepIndex = i8PrevStepIndex;

			IMAAdpcm_Encode(&pi16InData[psEncState->m_u8Channels + u8ChannelIndex]
					, &pbyOutData[(psEncState->m_u8Channels + u8ChannelIndex) * IMAADPCM_HEADER_SIZE]
					, u16SampleCount - 1, psEncState, u8ChannelIndex, TRUE);
		}
		else {
			// DVI4 format
			u16Temp = psPrivateState->m_i16PrevValue;
			pbyOutData[IMAADPCM_HEADER_SIZE * u8ChannelIndex]	  = (char)(u16Temp >> 8);
			pbyOutData[IMAADPCM_HEADER_SIZE * u8ChannelIndex + 1] = (char)(u16Temp & 0xFF);
			pbyOutData[IMAADPCM_HEADER_SIZE * u8ChannelIndex + 2] = i8PrevStepIndex;
			pbyOutData[IMAADPCM_HEADER_SIZE * u8ChannelIndex + 3] = 0;	// reserved;

			// Encode block data
			memset(psPrivateState, 0, sizeof(_S_IMAADPCM_ENCSTATE));
			psPrivateState->m_i16PrevValue = u16Temp;
			psPrivateState->m_i8StepIndex = i8PrevStepIndex;

			IMAAdpcm_Encode(&pi16InData[u8ChannelIndex]
					, &pbyOutData[(psEncState->m_u8Channels + u8ChannelIndex) * IMAADPCM_HEADER_SIZE]
					, u16SampleCount, psEncState, u8ChannelIndex, TRUE);
		}
	} // for

	return TRUE;
} // IMAAdpcm_EncodeBlock()


// Encode any length of samples
uint32_t										// [out]Byte count of encoded data
IMAAdpcm_EncodeBuffer(
	int16_t				*pi16InBuf,			// [in] Input decoded buffer
	char				*pbyOutBuf,			// [in] Output encoded buffer
	uint16_t			u16InBufBytes,		// [in] Byte count of pi16InBuf
											//		(multiple of 4 * channel_number)
	S_IMAADPCM_ENCSTATE	*psEncState,		// [in/out] IMA ADPCM encoder state
	char				**ppby1stBlockHeader// [in/out] Pointer to channel 0
											//		1st block header in encoded data
											//		(NULL: No header)
) {
	const uint32_t	u32MaxSamplesPerChannel = psEncState->m_u16SamplesPerBlock
												/ psEncState->m_u8Channels,
					u32MaxSamplesToEncodePerChannel = u32MaxSamplesPerChannel - 1,
					// 1 sample has 0.5 byte
					u32MaxCountPerChannel = u32MaxSamplesToEncodePerChannel / 2;
	const uint8_t	u8ChannelDataWordOffset = IMAADPCM_HEADER_SIZE
												* psEncState->m_u8Channels;
	uint32_t		u32EncodedBytes = 0;
	uint8_t			u8ChannelIndex;

	if (u16InBufBytes % psEncState->m_u8Channels)
		return 0;

	u16InBufBytes /= psEncState->m_u8Channels;
	// 2-byte sample encodes to 0.5 byte
	if (u16InBufBytes & 0x03)
		return 0;

	if (ppby1stBlockHeader)
		*ppby1stBlockHeader = NULL;


	for (u8ChannelIndex = 0; u8ChannelIndex < psEncState->m_u8Channels
			; u8ChannelIndex++) {
		_S_IMAADPCM_ENCSTATE	*psPrivateState
									= &(psEncState->m_asPrivateState[u8ChannelIndex]);
		int16_t		*pi16InBufData = pi16InBuf + u8ChannelIndex;
		char		*pbyOutBufData = pbyOutBuf + IMAADPCM_CHANNEL_DATA_SIZE
										* u8ChannelIndex;
		uint32_t	u32InBufByteCount = u16InBufBytes,
					u32CachedBytes;

		// Output cached un-word-aligned data of previous buffer
		u32CachedBytes = psPrivateState->m_u16Count % 4;
		memcpy(pbyOutBufData, psPrivateState->m_abyOutputCache, u32CachedBytes);
		pbyOutBufData += u32CachedBytes;
		u32EncodedBytes += u32CachedBytes;

		while (u32InBufByteCount)
		{
			uint32_t	u32SamplesToEncodeInBlock,
						u32MaxSamplesToEncodeInBlock,
						u32SavedCount;
			char		bFreshBlock;

			if (!psPrivateState->m_bWritenHeader && psPrivateState->m_u16Count == 0) {
				uint16_t	u16Temp = *pi16InBufData;
				char 		*pbyChannelHeader = pbyOutBufData;

				// Generate block header
				psPrivateState->m_i16OutputBuffer = 0;
				psPrivateState->m_u8BufferStep = 0;
				
				if (psEncState->m_eFormat == eIMAADPCM_IMA4) {
					pbyChannelHeader[0] = (char)(u16Temp & 0xFF);
					pbyChannelHeader[1] = (char)(u16Temp >> 8);
				}
				else {
					u16Temp = psPrivateState->m_i16PrevValue;
					pbyChannelHeader[0]	= (char)(u16Temp >> 8);
					pbyChannelHeader[1] = (char)(u16Temp & 0xFF);
				}
				pbyChannelHeader[2] = (char)psPrivateState->m_i8StepIndex;
				pbyChannelHeader[3] = 0;	// reserved;

				psPrivateState->m_bWritenHeader = TRUE;
				if (ppby1stBlockHeader && *ppby1stBlockHeader == NULL
						&& u8ChannelIndex == 0)
					*ppby1stBlockHeader = pbyChannelHeader;

				psPrivateState->m_i16PrevValue = u16Temp;
				u32EncodedBytes += IMAADPCM_HEADER_SIZE;

				// Consume one sample in block begin
				if (psEncState->m_eFormat == eIMAADPCM_IMA4)
					u32InBufByteCount -= 2;
				
				if (u32InBufByteCount == 0)
					break;

				// Encode block data
				if (psEncState->m_eFormat == eIMAADPCM_IMA4) {
					pi16InBufData += psEncState->m_u8Channels;
				}
				pbyOutBufData = pbyChannelHeader + u8ChannelDataWordOffset;
			} // if

			// 1 encoded sample has 0.5 byte
			u32SamplesToEncodeInBlock = u32InBufByteCount / 2;
			u32MaxSamplesToEncodeInBlock
					= u32MaxSamplesToEncodePerChannel
						- 2 * psPrivateState->m_u16Count
						- psPrivateState->m_u8BufferStep;
			if (u32SamplesToEncodeInBlock >= u32MaxSamplesToEncodeInBlock) {
				u32SamplesToEncodeInBlock = u32MaxSamplesToEncodeInBlock;
				// IMAAdpcm_Encode() will fresh m_i16OutputBuffer because meet end of block
				bFreshBlock = TRUE;
			} // if
			else
				bFreshBlock = FALSE;

			u32SavedCount = psPrivateState->m_u16Count;
			pbyOutBufData = IMAAdpcm_Encode(pi16InBufData, pbyOutBufData
					, u32SamplesToEncodeInBlock, psEncState, u8ChannelIndex
					, bFreshBlock);

			u32InBufByteCount -= 2 * u32SamplesToEncodeInBlock;
			pi16InBufData += u32SamplesToEncodeInBlock * psEncState->m_u8Channels;
			u32EncodedBytes += (psPrivateState->m_u16Count - u32SavedCount);

			if (psPrivateState->m_u16Count >= u32MaxCountPerChannel) {
				psPrivateState->m_u16Count = 0;
				psPrivateState->m_bWritenHeader = FALSE;
			} // if
		} // while u32InBufByteCount > 0

		// Cache un-word-aligned output data of this buffer
		u32CachedBytes = psPrivateState->m_u16Count % 4;
		memcpy(psPrivateState->m_abyOutputCache, pbyOutBufData - u32CachedBytes
				, u32CachedBytes);
		u32EncodedBytes -= u32CachedBytes;
	} // for each channel

	return u32EncodedBytes;
} // IMAAdpcm_EncodeBuffer()
