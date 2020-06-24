/***************************************************************************//**
 * @file     IMAAdpcmCodec.h
 * @brief    IMA ADPCM codec
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#ifndef __IMAADPCMCODEC_H__
#define __IMAADPCMCODEC_H__

#include <stdint.h>

//#include "Platform.h"
//#include "System/SysInfra.h"

//-----------------------------------------------------------------------------
// Version information
//	   -----------------------------------------------------------------
//     |      31:24       |     23:16    |     15:8     |     7:0      | 
//     -----------------------------------------------------------------
//     | Should all be 0s | Major number | Minor number | Build number |
//     -----------------------------------------------------------------
//     Major number: Module+"_"+MAJOR+"_"+NUM
//     Minor number: Module+"_"+MIMOR+"_"+NUM
//     Build number: Module+"_"+BUILD+"_"+NUM
//-----------------------------------------------------------------------------
#define IMAADPCM_MAJOR_NUM			3
#define IMAADPCM_MINOR_NUM			61
#define IMAADPCM_BUILD_NUM			2

// Version defined with SysInfra
//_SYSINFRA_VERSION_DEF(IMAADPCM, IMAADPCM_MAJOR_NUM, IMAADPCM_MINOR_NUM, IMAADPCM_BUILD_NUM)

#define IMAADPCM_HEADER_SIZE		4
#define IMAADPCM_CHANNEL_DATA_SIZE	4
#define IMAADPCM_MAX_CHANNEL		2		// Max supported channel number

#ifndef BOOL
#define BOOL int32_t
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

typedef enum
{
	eIMAADPCM_IMA4,
	eIMAADPCM_DVI4
}E_IMAADPCM_FORMAT;

typedef struct
{
	int16_t	m_i16InputBuffer;
	int16_t	m_i16PrevValue;
	uint16_t	m_u16ReadBytes;
	uint8_t	m_u8BufferStep;					// Toggle between inputbuffer/input
	int8_t	m_i8StepIndex;
} _S_IMAADPCM_DECSTATE;

typedef struct
{
	int16_t	m_i16OutputBuffer;
	int16_t	m_i16PrevValue;
	uint16_t	m_u16Count;
	uint8_t	m_u8BufferStep;					// Toggle between outputbuffer/output
	int8_t	m_i8StepIndex;

	uint8_t	m_abyOutputCache[IMAADPCM_CHANNEL_DATA_SIZE - 1];
	uint8_t	m_bWritenHeader;				// Flag of write block header
} _S_IMAADPCM_ENCSTATE;

typedef struct
{
	// Only support 4 bits per encoded sample
	uint8_t   m_u8Channels;
	uint16_t	m_u16SamplesPerBlock;
	uint16_t	m_u16BytesPerBlock;

	E_IMAADPCM_FORMAT m_eFormat;
	_S_IMAADPCM_DECSTATE	m_asPrivateState[IMAADPCM_MAX_CHANNEL];
} S_IMAADPCM_DECSTATE;

typedef struct
{
	// Only support 4 bits per encoded sample
	uint8_t   m_u8Channels;
	uint16_t  m_u16SamplesPerBlock;
	uint16_t  m_u16BytesPerBlock;

	E_IMAADPCM_FORMAT m_eFormat;
	_S_IMAADPCM_ENCSTATE	m_asPrivateState[IMAADPCM_MAX_CHANNEL];
} S_IMAADPCM_ENCSTATE;


#ifdef	__cplusplus
extern "C"
{
#endif

BOOL	
IMAAdpcm_DecodeInitialize(
	S_IMAADPCM_DECSTATE		*psDecState,		// [in/out] IMA ADPCM decoder state
	uint16_t				u16Channels,		// [in] Channel number up to 2
	uint32_t				u32SamplingRate
);

BOOL 
IMAAdpcm_DecodeInitializeEx(
	S_IMAADPCM_DECSTATE	*psDecState,		// [in/out] IMA ADPCM decoder state
	uint16_t				u16Channels,		// [in] Channel number up to 2
	uint32_t				u32SamplesPerBlock,
	uint32_t				u32BytesPerBlock	
);

BOOL	
IMAAdpcm_EncodeInitialize(
	S_IMAADPCM_ENCSTATE	*psEncState,		// [in/out] IMA ADPCM encoder state
	uint16_t				u16Channels,		// [in] Channel number up to 2
	uint32_t				u32SamplingRate
);

// Decode a block of data
BOOL	
IMAAdpcm_DecodeBlock(
	char				*pbyInData,			// [in] Input encoded data
	int16_t				*pi16OutData, 		// [in] Output decoded samples
	uint16_t			u16SampleCount,		// [in] Sample count of pbyInData
	S_IMAADPCM_DECSTATE	*psDecState			// [in/out] IMA ADPCM decoder state
);

// Encode a block of samples
BOOL	
IMAAdpcm_EncodeBlock(
	int16_t				*pi16InData,			// [in] Input decoded samples
	char				*pbyOutData,		// [in] Output encoded data
	uint16_t			u16SampleCount,		// [in] Sample count of pi16InData
	S_IMAADPCM_ENCSTATE	*psEncState			// [in/out] IMA ADPCM encoder state
);

uint32_t
IMAAdpcm_GetVersion(void);


// Decode any length of data
uint32_t										// [out]Byte count of decoded data
IMAAdpcm_DecodeBuffer(
	char				*pbyInBuf,			// [in] Input encoded buffer
	int16_t				*pi16OutBuf,			// [in] Output decoded buffer
	uint16_t			u16InBufBytes,		// [in] Byte count of pbyInBuf
											//		(multiple of 4 * channel_number)
	S_IMAADPCM_DECSTATE	*psDecState			// [in/out] IMA ADPCM decoder state
);

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
);

BOOL
IMAAdpcm_EncodeSetFromat(
	E_IMAADPCM_FORMAT eFormat,				// [in] IMA_ADPCM format
	S_IMAADPCM_ENCSTATE	*psEncState			// [in/out] IMA ADPCM encoder state
);

BOOL
IMAAdpcm_DecodeSetFromat(
	E_IMAADPCM_FORMAT eFormat,				// [in] IMA_ADPCM format
	S_IMAADPCM_DECSTATE	*psDecState			// [in/out] IMA ADPCM decoder state
);


#ifdef	__cplusplus
}
#endif

#endif //__IMAADPCMCODEC_H__
