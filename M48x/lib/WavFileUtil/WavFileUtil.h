/***************************************************************************//**
 * @file     WavFileUtil.h
 * @brief    WAV file utility
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
 
#ifndef	__WAVFILEUTIL_H__
#define	__WAVFILEUTIL_H__

#include "lib/oofatfs/ff.h"

//-----------------------------------------------------------------------------
// Constant definition
//-----------------------------------------------------------------------------
#define WAVFILEUTIL_VERSION	0x0352		// v3.52

#define WAV_FORMAT_LEN		14
#define	WAV_FORMAT_EX_LEN	18
#define IMA_WAV_FORMAT_LEN	20

//#define FALSE				0
//#define TRUE				1


//-----------------------------------------------------------------------------
// Type declaration
//-----------------------------------------------------------------------------
typedef signed int			BOOL;
typedef	unsigned char		BYTE;
typedef signed int			INT32;
typedef const char			*PCSTR;
typedef unsigned char		*PUINT8;
typedef unsigned short		*PUINT16;
typedef unsigned int		*PUINT32;
typedef unsigned short		UINT16;
typedef unsigned int		UINT32;
typedef void				VOID;


typedef struct tagWavFormat {
	UINT16	u16FormatTag;
	UINT16  u16Channels;
	UINT32	u32SamplesPerSec;
	UINT32	u32AvgBytesPerSec;
	UINT16	u16BlockAlign;
} S_WavFormat;


typedef struct tagWavFormatEx {
	UINT16	u16FormatTag;
	UINT16  u16Channels;
	UINT32	u32SamplesPerSec; 		//sample rate
	UINT32	u32AvgBytesPerSec; 		//byte rate
	UINT16	u16BlockAlign;

	UINT16	u16BitsPerSample;
	UINT16  u16CbSize;
} S_WavFormatEx;


typedef struct	tagIMAWavFormat {
	UINT16	u16FormatTag;
	UINT16  u16Channels;
	UINT32	u32SamplesPerSec;
	UINT32	u32AvgBytesPerSec;
	UINT16	u16BlockAlign;

	UINT16	u16BitsPerSample;
	UINT16  u16CbSize;

	UINT16	u16SamplesPerBlock;
} S_IMAWavFormat;


typedef struct tagWavFileWriteInfo {
//	INT32	i32FileHandle;
	FIL *	pFile;
	UINT32	u32RiffDataLenFilePos;
	UINT32	u32DataChunkLenFilePos;
} S_WavFileWriteInfo;


typedef struct tagWavFileReadInfo {
//	INT32			i32FileHandle;
	FIL *			pFile;
	UINT32			u32RiffDataLen;
	UINT32			u32FormatFilePos;
	UINT32			u32FormatDataLen;
	UINT32			u32DataFilePos;
	UINT32			u32DataLen;
	UINT32			u32DataPos;
	UINT16			u16SamplesPerBlock;
	UINT16			u16BytesPerBlock;
	S_WavFormatEx	sWavFormatEx;
} S_WavFileReadInfo;


typedef enum tagWavFormatTag {
	eWAVE_FORMAT_UNKNOWN	=	0x0000,
	eWAVE_FORMAT_PCM		=	0x0001,
	eWAVE_FORMAT_IMA_ADPCM	=	0x0011,
	eWAVE_FORMAT_DVI_ADPCM	=	0x0011,
	eWAVE_FORMAT_G726_ADPCM	=	0x0045,
	eWAVE_FORMAT_G711_ALAW	=	0x0006,
	eWAVE_FORMAT_G711_ULAW	=	0x0007
} E_WavFormatTag;


#ifdef	__cplusplus
extern "C"
{
#endif

//-----------------------------------------------------------------------------
// Get version inforamtion.
//-----------------------------------------------------------------------------
UINT32
WavFileUtil_GetVersion(VOID);

// Write file functions
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Write_Initialize(
	S_WavFileWriteInfo	*psInfo,
	FIL *	pFile
//	PCSTR				pszOutFileName
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Write_SetFormat(
	S_WavFileWriteInfo	*psInfo,
	E_WavFormatTag		eWavFormatTag,
	UINT16				u16Channels,
	UINT32				u32SamplingRate,
	UINT16				u16BitsPerSample
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Write_WriteData(
	S_WavFileWriteInfo	*psInfo,
	const BYTE			*pbyData,
	UINT32				u32DataSize
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Write_Finish(
	S_WavFileWriteInfo	*psInfo
);

// Read file functions
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_Initialize(
	S_WavFileReadInfo	*psInfo,
	FIL *	pFile
//	PCSTR				pszInFileName
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_GetFormat(
	S_WavFileReadInfo	*psInfo,
	E_WavFormatTag		*peWavFormatTag,
	PUINT16				pu16Channels,
	PUINT32				pu32SamplingRate,
	PUINT16				pu16BitsPerSample
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_GetFormatBlockInfo(
	S_WavFileReadInfo	*psInfo,
	PUINT16				pu16SamplesPerBlock,
	PUINT16				pu16BytesPerBlock
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
UINT32
WavFileUtil_Read_GetTimeLength(
	S_WavFileReadInfo	*psInfo
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
UINT32
WavFileUtil_Read_GetTimePosition(
	S_WavFileReadInfo	*psInfo
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_ReadDataStart(
	S_WavFileReadInfo	*psInfo
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
UINT32
WavFileUtil_Read_ReadData(
	S_WavFileReadInfo	*psInfo,
	BYTE				*pbyData,
	UINT32				u32DataSize
);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_Finish(
	S_WavFileReadInfo	*psInfo
);


#ifdef	__cplusplus
}
#endif

#endif // __WAVFILEUTIL_H__
