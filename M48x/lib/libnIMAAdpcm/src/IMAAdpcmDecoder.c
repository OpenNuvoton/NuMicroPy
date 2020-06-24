/***************************************************************************//**
 * @file     IMAAdpcmDecoder.c
 * @brief    IMA ADPCM decoder
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "IMAAdpcmCodec.h"
#include "IMAAdpcmTable.h"


static
char*									// [out]New indata pointer
IMAAdpcm_Decode(
	signed char			*indata,		// [in] Input buffer pointer
	short				*outdata,		// [in] Output buffer pointer
	uint16_t				len,
	S_IMAADPCM_DECSTATE	*state,
	uint8_t				u8ChannelNextDataWordOffset,
	uint8_t				u8ChannelIndex
) {
	_S_IMAADPCM_DECSTATE	*pstate = &(state->m_asPrivateState[u8ChannelIndex]);
    int						step = StepTable[pstate->m_i8StepIndex]; /* Stepsize */
    int 					valpred = pstate->m_i16PrevValue;	/* Predicted value */
  
    for ( ; len > 0 ; len-- ) 
	{
	    int	sign;		/* Current adpcm sign bit */
    	int delta;		/* Current adpcm output value */
	    int vpdiff;		/* Current change to pstate->m_i16PrevValue */

		/* Step 1 - get the delta value */
		if ( pstate->m_u8BufferStep ){
			if(state->m_eFormat == eIMAADPCM_IMA4)
				delta = (pstate->m_i16InputBuffer >> 4) & 0x0f;
			else
				delta = pstate->m_i16InputBuffer & 0xf;
		}
		else
		{
			pstate->m_i16InputBuffer = *indata++;
			if ( ((++pstate->m_u16ReadBytes) % IMAADPCM_CHANNEL_DATA_SIZE) == 0 )
				indata += u8ChannelNextDataWordOffset;
			if(state->m_eFormat == eIMAADPCM_IMA4)
				delta = pstate->m_i16InputBuffer & 0xf;
			else
				delta = (pstate->m_i16InputBuffer >> 4) & 0x0f;			
		} // else
		pstate->m_u8BufferStep = !pstate->m_u8BufferStep;
		
		/* Step 2 - Find new index value (for later) */
		pstate->m_i8StepIndex += IndexTable[delta];
		if ( pstate->m_i8StepIndex < 0 ) pstate->m_i8StepIndex = 0;
		if ( pstate->m_i8StepIndex > 88 ) pstate->m_i8StepIndex = 88;

		/* Step 3 - Separate sign and magnitude */
		sign = delta & 8;
		delta = delta & 7;

		/* Step 4 - Compute difference and new predicted value */
		/*
		** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
		** in adpcm_coder.
		*/
		vpdiff = step >> 3;
		if ( delta & 4 ) vpdiff += step;
		if ( delta & 2 ) vpdiff += step>>1;
		if ( delta & 1 ) vpdiff += step>>2;

		if ( sign )
	      valpred -= vpdiff;
		else
    	  valpred += vpdiff;

		/* Step 5 - clamp output value */
		if ( valpred > 32767 )
		  valpred = 32767;
		else if ( valpred < -32768 )
		  valpred = -32768;

		/* Step 6 - Update step value */
		step = StepTable[pstate->m_i8StepIndex];

		/* Step 7 - Output value */
		*outdata = valpred;
		outdata += state->m_u8Channels;
    } // for

	pstate->m_i16PrevValue = valpred;
    return (char *)indata;
} // IMAAdpcm_Decode()


// Decode a block of data
BOOL 
IMAAdpcm_DecodeBlock(
	char				*pbyInData,			// [in] Input encoded data
	int16_t				*pi16OutData, 		// [in] Output decoded samples
	uint16_t			u16SampleCount,		// [in] Sample count of pbyInData
	S_IMAADPCM_DECSTATE	*psDecState			// [in/out] IMA ADPCM decoder state
) {
	char	*pbyChannelHeader = pbyInData,
			*pbySampleData = pbyInData + psDecState->m_u8Channels * IMAADPCM_HEADER_SIZE;
	uint8_t	u8ChannelIndex,
			u8ChannelNextDataWordOffset
				= (psDecState->m_u8Channels - 1) * IMAADPCM_CHANNEL_DATA_SIZE;

	for (u8ChannelIndex = 0; u8ChannelIndex < psDecState->m_u8Channels
		; u8ChannelIndex++, pbyChannelHeader += IMAADPCM_HEADER_SIZE
			, pbySampleData += IMAADPCM_CHANNEL_DATA_SIZE) {
		_S_IMAADPCM_DECSTATE	*psPrivateState
									= &(psDecState->m_asPrivateState[u8ChannelIndex]);
		uint16_t u16Temp;

		if (psDecState->m_eFormat == eIMAADPCM_IMA4) {
			// Parse block header
			u16Temp = (uint16_t)pbyChannelHeader[0]
								| ((uint16_t)pbyChannelHeader[1] << 8);

			pi16OutData[u8ChannelIndex] = (int16_t)u16Temp;

			memset(psPrivateState, 0, sizeof(_S_IMAADPCM_DECSTATE));
			psPrivateState->m_i16PrevValue = (int16_t)u16Temp;
			psPrivateState->m_i8StepIndex = pbyChannelHeader[2];

			// Decode block data
			IMAAdpcm_Decode((signed char *)pbySampleData
				, (pi16OutData + psDecState->m_u8Channels) + u8ChannelIndex
				, u16SampleCount - 1, psDecState, u8ChannelNextDataWordOffset
				, u8ChannelIndex);
		}
		else { // DVI4
			// Parse block header
			u16Temp = (uint16_t)pbyChannelHeader[1]
								| ((uint16_t)pbyChannelHeader[0] << 8);

			memset(psPrivateState, 0, sizeof(_S_IMAADPCM_DECSTATE));
			psPrivateState->m_i16PrevValue = (int16_t)u16Temp;
			psPrivateState->m_i8StepIndex = pbyChannelHeader[2];

			// Decode block data
			IMAAdpcm_Decode((signed char*)pbySampleData
				, pi16OutData + u8ChannelIndex
				, u16SampleCount, psDecState, u8ChannelNextDataWordOffset
				, u8ChannelIndex);
		}
	} // for

	return TRUE;
} // IMAAdpcm_DecodeBlock()


// Decode any length of data
uint32_t									// [out]Byte count of decoded data
IMAAdpcm_DecodeBuffer(
	char				*pbyInBuf,			// [in] Input encoded buffer
	int16_t				*pi16OutBuf,		// [in] Output decoded buffer
	uint16_t			u16InBufBytes,		// [in] Byte count of pbyInBuf
											//		(multiple of 4 * channel_number)
	S_IMAADPCM_DECSTATE	*psDecState			// [in/out] IMA ADPCM decoder state
) {
	const uint32_t	u32MaxSamplesPerChannel = psDecState->m_u16SamplesPerBlock
												/ psDecState->m_u8Channels,
					u32MaxSamplesToDecodePerChannel = u32MaxSamplesPerChannel - 1,
					u32MaxReadBytesPerChannel = u32MaxSamplesToDecodePerChannel / 2;	// 1 sample has 0.5 byte
	const uint8_t	u8ChannelNextDataWordOffset = IMAADPCM_CHANNEL_DATA_SIZE
													* (psDecState->m_u8Channels - 1);
	uint32_t		u32DecodedBytes = 0;
	uint8_t			u8ChannelIndex;

	if (u16InBufBytes % psDecState->m_u8Channels)
		return 0;

	u16InBufBytes /= psDecState->m_u8Channels;
	if (u16InBufBytes & 0x03)
		return 0;				// Encoded data should be word-aligned

	for (u8ChannelIndex = 0; u8ChannelIndex < psDecState->m_u8Channels; u8ChannelIndex++) {
		_S_IMAADPCM_DECSTATE	*psPrivateState
									= &(psDecState->m_asPrivateState[u8ChannelIndex]);
		char		*pbyInBufData = pbyInBuf + IMAADPCM_CHANNEL_DATA_SIZE * u8ChannelIndex;
		int16_t		*pi16OutBufData = pi16OutBuf + u8ChannelIndex;
		uint32_t	u32InBufByteCount = u16InBufBytes;

		while (u32InBufByteCount) {
			uint32_t	u32SamplesToDecodeInBlock,
						u32MaxSamplesToDecodeInBlock;

			if (psPrivateState->m_u16ReadBytes == 0) {
				// Parse block header
				const char 	*pbyChannelHeader = pbyInBufData;
				uint16_t	u16Temp;

				if (psDecState->m_eFormat == eIMAADPCM_IMA4) {
					u16Temp = (uint16_t)pbyChannelHeader[0]
							| ((uint16_t)pbyChannelHeader[1] << 8);

					psPrivateState->m_i16PrevValue = *pi16OutBufData = (int16_t)u16Temp;
				}
				else {
					u16Temp = (uint16_t)pbyChannelHeader[1]
							| ((uint16_t)pbyChannelHeader[0] << 8);

					psPrivateState->m_i16PrevValue = (int16_t)u16Temp;
				}

				psPrivateState->m_i16InputBuffer = 0;
				psPrivateState->m_u8BufferStep = 0;
				psPrivateState->m_i8StepIndex = pbyChannelHeader[2];

				if (psDecState->m_eFormat == eIMAADPCM_IMA4)
					u32DecodedBytes += 2;
				
				if (u32InBufByteCount < IMAADPCM_HEADER_SIZE)
					break;
				u32InBufByteCount -= IMAADPCM_HEADER_SIZE;

				// Decode block data
				pbyInBufData += (IMAADPCM_HEADER_SIZE + u8ChannelNextDataWordOffset);
				
				if (psDecState->m_eFormat == eIMAADPCM_IMA4)
					pi16OutBufData += psDecState->m_u8Channels;
			} // if

			// 1 byte decodes 2 samples
			u32SamplesToDecodeInBlock = 2 * u32InBufByteCount;
			u32MaxSamplesToDecodeInBlock
					= u32MaxSamplesToDecodePerChannel
						- 2 * psPrivateState->m_u16ReadBytes
						- psPrivateState->m_u8BufferStep;
			if (u32SamplesToDecodeInBlock > u32MaxSamplesToDecodeInBlock)
				u32SamplesToDecodeInBlock = u32MaxSamplesToDecodeInBlock;

			pbyInBufData = IMAAdpcm_Decode((signed char *)pbyInBufData, pi16OutBufData
					, u32SamplesToDecodeInBlock, psDecState
					, u8ChannelNextDataWordOffset, u8ChannelIndex);

			u32InBufByteCount -= u32SamplesToDecodeInBlock / 2;
			pi16OutBufData += u32SamplesToDecodeInBlock * psDecState->m_u8Channels;
			u32DecodedBytes += 2 * u32SamplesToDecodeInBlock;
			
			if (psPrivateState->m_u16ReadBytes >= u32MaxReadBytesPerChannel)
				psPrivateState->m_u16ReadBytes = 0;
		} // while u32InBufByteCount > 0
	} // for each channel

	return u32DecodedBytes;
} // IMAAdpcm_DecodeBuffer()
