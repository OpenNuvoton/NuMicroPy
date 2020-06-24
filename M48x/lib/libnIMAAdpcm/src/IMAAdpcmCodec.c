/***************************************************************************//**
 * @file     IMAAdpcmCodec.c
 * @brief    IMA ADPCM codec
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "IMAAdpcmCodec.h"


//-----------------------------------------------------------------------------
//Description
//	Get version information.
//Parameter 
//	None.
//-----------------------------------------------------------------------------
uint32_t 
IMAAdpcm_GetVersion(void)
{
	return ( (IMAADPCM_MAJOR_NUM << 16) | (IMAADPCM_MINOR_NUM << 8) | IMAADPCM_BUILD_NUM );
} // IMAAdpcm_GetVersion()


BOOL 
IMAAdpcm_DecodeInitialize(
	S_IMAADPCM_DECSTATE	*psDecState,		// [in/out] IMA ADPCM decoder state
	uint16_t				u16Channels,		// [in] Channel number up to 2
	uint32_t				u32SamplingRate
)
{
	uint16_t u16N;

	if (!psDecState || u16Channels > IMAADPCM_MAX_CHANNEL)
		return FALSE;

	if (u32SamplingRate < 22050)
		u16N = 63;
	else if (u32SamplingRate < 44100)
	{
		// 3-bit:
		//	u16N = 126;
		// 4-bit:
			u16N = 127;
	} // if
	else
		u16N = 255;

	memset(psDecState, 0, sizeof(S_IMAADPCM_DECSTATE));

	psDecState->m_u8Channels = (uint8_t)u16Channels;
	psDecState->m_u16BytesPerBlock = (u16N + 1) * 4 * u16Channels;
	// 3-bit:
	//	((N * 4 * 8) / 3) + 1
	//	psDecState->u16SamplesPerBlock = (u16N * 32) / 3 + 1;
	// 4-bit:
	//	((N * 4 * 8) / 4) + 1
		psDecState->m_u16SamplesPerBlock = ((u16N * 8) + 1) * u16Channels;
	psDecState->m_eFormat = eIMAADPCM_IMA4;

//		psDecState->m_u16SamplesPerBlock = ((u16N * 8)) * u16Channels;
//	psDecState->m_eFormat = eIMAADPCM_DVI4;

	return TRUE;
} // IMAAdpcm_DecodeInitialize()


BOOL 
IMAAdpcm_DecodeInitializeEx(
	S_IMAADPCM_DECSTATE	*psDecState,		// [in/out] IMA ADPCM decoder state
	uint16_t				u16Channels,		// [in] Channel number up to 2
	uint32_t				u32SamplesPerBlock,
	uint32_t				u32BytesPerBlock	
)
{
	if (!psDecState || u16Channels > IMAADPCM_MAX_CHANNEL)
		return FALSE;

	memset(psDecState, 0, sizeof(S_IMAADPCM_DECSTATE));

	psDecState->m_u8Channels = (uint8_t)u16Channels;
	psDecState->m_u16BytesPerBlock = u32BytesPerBlock * u16Channels;
	// 3-bit:
	//	((N * 4 * 8) / 3) + 1
	//	psDecState->u16SamplesPerBlock = (u16N * 32) / 3 + 1;
	// 4-bit:
	//	((N * 4 * 8) / 4) + 1
		psDecState->m_u16SamplesPerBlock = u32SamplesPerBlock * u16Channels;
	psDecState->m_eFormat = eIMAADPCM_IMA4;


	return TRUE;
} // IMAAdpcm_DecodeInitialize()



BOOL 
IMAAdpcm_EncodeInitialize(
	S_IMAADPCM_ENCSTATE	*psEncState,		// [in/out] IMA ADPCM encoder state
	uint16_t				u16Channels,		// [in] Channel number up to 2
	uint32_t				u32SamplingRate
)
{
	uint16_t u16N;

	if (!psEncState || u16Channels > IMAADPCM_MAX_CHANNEL)
		return FALSE;

	if (u32SamplingRate < 22050)
		u16N = 63;
	else if (u32SamplingRate < 44100)
	{
		// 3-bit:
		//	u16N = 126;
		// 4-bit:
			u16N = 127;
	} // if
	else
		u16N = 255;

	memset(psEncState, 0, sizeof(S_IMAADPCM_ENCSTATE));

	psEncState->m_u8Channels = (uint8_t)u16Channels;
	psEncState->m_u16BytesPerBlock = (u16N + 1) * 4 * u16Channels;
	// 3-bit:
	//	((N * 4 * 8) / 3) + 1
	//	psEncState->u16SamplesPerBlock = (u16N * 32) / 3 + 1;
	// 4-bit:
	//	((N * 4 * 8) / 4) + 1
		psEncState->m_u16SamplesPerBlock = ((u16N * 8) + 1) * u16Channels;
	psEncState->m_eFormat = eIMAADPCM_IMA4;

//		psEncState->m_u16SamplesPerBlock = ((u16N * 8)) * u16Channels;
//	psEncState->m_eFormat = eIMAADPCM_DVI4;

	return TRUE;
} // IMAAdpcm_EncodeInitialize()


BOOL
IMAAdpcm_EncodeSetFromat(
	E_IMAADPCM_FORMAT eFormat,				// [in] IMA_ADPCM format
	S_IMAADPCM_ENCSTATE	*psEncState			// [in/out] IMA ADPCM encoder state
)
{
	uint16_t u16N;

	if (!psEncState)
		return FALSE;

	u16N = (psEncState->m_u16BytesPerBlock / 4 / psEncState->m_u8Channels) - 1;

	if(eFormat == eIMAADPCM_IMA4){
		psEncState->m_u16SamplesPerBlock = ((u16N * 8) + 1) * psEncState->m_u8Channels;
	}
	else if(eFormat == eIMAADPCM_DVI4){
			psEncState->m_u16SamplesPerBlock = (u16N * 8) * psEncState->m_u8Channels;
	}
	else{
		return FALSE;
	}

	psEncState->m_eFormat = eFormat;

	return TRUE;
}

BOOL
IMAAdpcm_DecodeSetFromat(
	E_IMAADPCM_FORMAT eFormat,				// [in] IMA_ADPCM format
	S_IMAADPCM_DECSTATE	*psDecState			// [in/out] IMA ADPCM decoder state
)
{

	if (!psDecState)
		return FALSE;
//	Don't modify m_u16BytesPerBlock. It should follow reader audio information
//	u16N = (psDecState->m_u16BytesPerBlock / 4 / psDecState->m_u8Channels) - 1;

	if(eFormat == eIMAADPCM_IMA4){
//			psDecState->m_u16SamplesPerBlock = ((u16N * 8) + 1) * psDecState->m_u8Channels;
	}
	else if(eFormat == eIMAADPCM_DVI4){
//			psDecState->m_u16SamplesPerBlock = (u16N * 8) * psDecState->m_u8Channels;
	}
	else{
		return FALSE;
	}

	psDecState->m_eFormat = eFormat;

	return TRUE;
}


