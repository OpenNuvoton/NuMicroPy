/***************************************************************************//**
 * @file     WavFileReader.c
 * @brief    WAV file reader
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
 
#include "WavFileUtil.h"

#include <unistd.h>
#include <string.h>


#define _SIGNATURE_TO_UINT32(abyData)	\
	((UINT32)abyData[3] | ((UINT32)abyData[2] << 8) |	\
	 ((UINT32)abyData[1] << 16) | ((UINT32)abyData[0] << 24))
#define _BYTE_TO_UINT32(abyData)	\
	((UINT32)abyData[0] | ((UINT32)abyData[1] << 8) |	\
	 ((UINT32)abyData[2] << 16) | ((UINT32)abyData[3] << 24))


//-----------------------------------------------------------------------------
//Description
//	read riff header and point to fmt and data location
//Parameter
//	psInfo[in]
//		pointer point to wav file content
//-----------------------------------------------------------------------------
static BOOL
WavFileUtil_Read_ReadHeader(
	S_WavFileReadInfo *psInfo
)
{
	BYTE	abySignature[4];
	UINT32	u32BytesRead;
	FRESULT eRes;

	u32BytesRead = 0;
	f_read(psInfo->pFile, abySignature, 4, &u32BytesRead);

	if (4 != u32BytesRead)
		return FALSE;

	if (0x52494646 != _SIGNATURE_TO_UINT32(abySignature))
		return FALSE;	// "RIFF"

	u32BytesRead = 0;
	f_read(psInfo->pFile, &psInfo->u32RiffDataLen, 4, &u32BytesRead);

	if (4 != u32BytesRead)
		return FALSE;

	u32BytesRead = 0;
	f_read(psInfo->pFile, abySignature, 4, &u32BytesRead);

	if (4 != u32BytesRead)
		return FALSE;

	if (0x57415645 != _SIGNATURE_TO_UINT32(abySignature))
		return FALSE;	// "WAVE"

	UINT32	u32FilePos = 12;
	UINT32	u32RestDataLen = psInfo->u32RiffDataLen - 4;

	while (u32RestDataLen > 8) {

		u32BytesRead = 0;
		f_read(psInfo->pFile, abySignature, 4, &u32BytesRead);

		if (4 != u32BytesRead)
			return FALSE;

		UINT32	u32ChunkLen;

		u32BytesRead = 0;
		f_read(psInfo->pFile, &u32ChunkLen, 4, &u32BytesRead);

		if (4 != u32BytesRead)
			return FALSE;

		u32FilePos += 8;
		u32RestDataLen -= 8;

		switch (_SIGNATURE_TO_UINT32(abySignature)) {
		case 0x666D7420:	// "fmt "
			psInfo->u32FormatFilePos = u32FilePos;
			psInfo->u32FormatDataLen = u32ChunkLen;
			break;

		case 0x64617461:	// "data"
		case 0x44415441:	// "DATA"
			psInfo->u32DataFilePos = u32FilePos;
			psInfo->u32DataLen = u32ChunkLen;
			break;
		}

		u32FilePos += u32ChunkLen;

		if (u32RestDataLen < u32ChunkLen)
			break;

		u32RestDataLen -= u32ChunkLen;

		eRes = f_lseek(psInfo->pFile, u32FilePos);

		if (eRes != FR_OK)
			return FALSE;

//		if (u32FilePos != lseek(psInfo->i32FileHandle, u32FilePos, SEEK_SET))
//			return FALSE;
	} // while u32RestDataLen > 8

	return TRUE;
} // WavFileUtil_Read_ReadHeader()


//-----------------------------------------------------------------------------
//Description
//	read the fmt chunk and fill with buffer
//Parameter
//	psInfo[in]
//		pointer point to wav file content
//	pbyWavFormat[out]
//		pointer point to data buffer
//	u32WavFormatDataLen[in]
//		read data size
//-----------------------------------------------------------------------------
static BOOL
WavFileUtil_Read_GetFormatInternal(
	S_WavFileReadInfo	*psInfo,
	BYTE				*pbyWavFormat,
	UINT32				u32WavFormatDataLen
)
{
	if (!psInfo || (psInfo->pFile == NULL))
		return FALSE;

	FRESULT eRes;

	eRes = f_lseek(psInfo->pFile, psInfo->u32FormatFilePos);

	if (eRes != FR_OK)
		return FALSE;
		
//	if (psInfo->u32FormatFilePos != lseek(psInfo->i32FileHandle, psInfo->u32FormatFilePos, SEEK_SET))
//		return FALSE;

	UINT32 u32BytesRead;

	if (u32WavFormatDataLen >= psInfo->u32FormatDataLen) {
		u32BytesRead = 0;
		f_read(psInfo->pFile, pbyWavFormat, psInfo->u32FormatDataLen, &u32BytesRead);

		if (psInfo->u32FormatDataLen != u32BytesRead)
			return FALSE;

		//for (u32Idx = psInfo->u32FormatDataLen; u32Idx < u32WavFormatDataLen; u32Idx++)
		//	pbyWavFormat[u32Idx] = 0;
		memset(pbyWavFormat + psInfo->u32FormatDataLen, 0, u32WavFormatDataLen - psInfo->u32FormatDataLen);
	}
	else {
		u32BytesRead = 0;
		f_read(psInfo->pFile, pbyWavFormat, u32WavFormatDataLen, &u32BytesRead);

		if (u32WavFormatDataLen != u32BytesRead)
			return FALSE;
	}

	return TRUE;
} // WavFileUtil_Read_GetFormatInternal()


//----------------------------------------------------------------------------
/*
BOOL
WavFileUtil_Read_GetFormat(
	S_WavFileReadInfo*	psInfo,
	S_WavFormat*		psWavFormat
	)
{
	return WavFileUtil_Read_GetFormatInternal(psInfo, (BYTE*)psWavFormat, WAV_FORMAT_LEN);
} // WavFileUtil_Read_GetFormat()
*/
//----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//Description
//
//Parameter
//	psInfo[in]
//		pointer point to wav file content
//	psWavFormatEx[out]
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_GetFormatEx(
	S_WavFileReadInfo	*psInfo,
	S_WavFormatEx		*psWavFormatEx
)
{
	if (NULL == psInfo)
		return FALSE;

	if (!WavFileUtil_Read_GetFormatInternal(psInfo, (BYTE *)psWavFormatEx, WAV_FORMAT_EX_LEN))
		return FALSE;

	psInfo->u16SamplesPerBlock = 0;
	psInfo->u16BytesPerBlock = 0;

	if (eWAVE_FORMAT_IMA_ADPCM == psWavFormatEx->u16FormatTag) {
		if (!WavFileUtil_Read_GetFormatBlockInfo(psInfo, &psInfo->u16SamplesPerBlock, &psInfo->u16BytesPerBlock))
			return FALSE;
	}

	return TRUE;
} // WavFileUtil_Read_GetFormatEx()


//-----------------------------------------------------------------------------
//Description
//
//Parameter
//	psInfo[in]
//		pointer point to wav file content
//	psIMAWavFormat[out]
//
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_GetIMAFormat(
	S_WavFileReadInfo	*psInfo,
	S_IMAWavFormat		*psIMAWavFormat
)
{
	return WavFileUtil_Read_GetFormatInternal(psInfo, (BYTE *)psIMAWavFormat, IMA_WAV_FORMAT_LEN);
} // WavFileUtil_Read_GetIMAFormat()


//----------------------------------------------------------------------------
//							 Public functions
//----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//Description
// Get version inforamtion.
//Parameter
//	None.
//-----------------------------------------------------------------------------
UINT32
WavFileUtil_GetVersion(VOID)
{
	return WAVFILEUTIL_VERSION;
} // WavFileUtil_GetVersion()


//-----------------------------------------------------------------------------
//Description
// initialize structure and fill with data
//Parameter
//	psInfo[in]
//		pointer point to wav file content
//	pszInFileName[in]
//		current file name
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_Initialize(
	S_WavFileReadInfo	*psInfo,
	FIL *	pFile
)
{
	if (NULL == psInfo)
		return FALSE;

	memset(psInfo, 0, sizeof(S_WavFileReadInfo));
	psInfo->pFile = pFile;

	if (psInfo->pFile == NULL)
		return FALSE;

	if (!WavFileUtil_Read_ReadHeader(psInfo)) {
		psInfo->pFile = NULL;
		return FALSE;
	}

	psInfo->sWavFormatEx.u16FormatTag = eWAVE_FORMAT_UNKNOWN;
	return TRUE;
} // WavFileUtil_Read_Initialize()


//-----------------------------------------------------------------------------
//Description
//	get the detail information
//Parameter
//	psInfo[in]
//		pointer point to wav file content
//	peWavFormatTag[out]
//		file format
//	pu16Channels[out]
//		channel number
//	pu32SamplingRate[out]
//		sample rate
//	pu16BitsPerSample[out]
//		bit per sample
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_GetFormat(
	S_WavFileReadInfo	*psInfo,
	E_WavFormatTag		*peWavFormatTag,
	PUINT16				pu16Channels,
	PUINT32				pu32SamplingRate,
	PUINT16				pu16BitsPerSample
)
{
	if (NULL == psInfo)
		return FALSE;

	S_WavFormatEx *psWavFormatEx = &psInfo->sWavFormatEx;

	if (eWAVE_FORMAT_UNKNOWN == psWavFormatEx->u16FormatTag) {
		if (!WavFileUtil_Read_GetFormatEx(psInfo, psWavFormatEx))
			return FALSE;
	}

	*peWavFormatTag = (E_WavFormatTag)psWavFormatEx->u16FormatTag;
	*pu16Channels = psWavFormatEx->u16Channels;
	*pu32SamplingRate = psWavFormatEx->u32SamplesPerSec;
	*pu16BitsPerSample = psWavFormatEx->u16BitsPerSample;
	return TRUE;
} // WavFileUtil_Read_GetFormat()


//----------------------------------------------------------------------------
UINT32
WavFileUtil_Read_GetTimeLength(
	S_WavFileReadInfo	*psInfo
)
{
	if (NULL == psInfo)
		return 0;

	S_WavFormatEx *psWavFormatEx = &psInfo->sWavFormatEx;

	if (eWAVE_FORMAT_UNKNOWN == psWavFormatEx->u16FormatTag) {
		if (!WavFileUtil_Read_GetFormatEx(psInfo, psWavFormatEx))
			return 0;
	}

	UINT32 u32SampleNum;

	switch (psWavFormatEx->u16FormatTag) {
	// Kenny,2014/1/6
	case eWAVE_FORMAT_G711_ALAW:
	case eWAVE_FORMAT_G711_ULAW:
	case eWAVE_FORMAT_G726_ADPCM:
		u32SampleNum = 8 * psInfo->u32DataLen / psWavFormatEx->u16BitsPerSample;
		break;

	// -- END --

	case eWAVE_FORMAT_IMA_ADPCM:
		u32SampleNum = (psInfo->u32DataLen / (UINT32)psInfo->u16BytesPerBlock) * psInfo->u16SamplesPerBlock;
		// Kenny,2013/5/29
		u32SampleNum *= psWavFormatEx->u16Channels;
		// -- END --
		break;

	case eWAVE_FORMAT_PCM:
		switch (psWavFormatEx->u16BitsPerSample) {
		case 16:
			u32SampleNum = psInfo->u32DataLen >> 1;
			break;

		case 8:
			u32SampleNum = psInfo->u32DataLen;
			break;

		default:
			return 0;
		}

		break;

	default:
		return 0;
	} // switch psWavFormatEx->u16FormatTag

	long long u64Temp1;

	u64Temp1 = ((long long)u32SampleNum / psWavFormatEx->u16Channels) * 1000 / psWavFormatEx->u32SamplesPerSec;
	return (UINT32)u64Temp1;
} // WavFileUtil_Read_GetTimeLength()


//----------------------------------------------------------------------------
UINT32
WavFileUtil_Read_GetTimePosition(
	S_WavFileReadInfo	*psInfo
)
{
	if (NULL == psInfo)
		return 0;

	S_WavFormatEx	*psWavFormatEx;
	UINT32			u32SampleNum;

	psWavFormatEx = &psInfo->sWavFormatEx;

	switch (psWavFormatEx->u16FormatTag) {
	// Kenny,2014/1/6
	case eWAVE_FORMAT_G711_ALAW:
	case eWAVE_FORMAT_G711_ULAW:
	case eWAVE_FORMAT_G726_ADPCM:
		u32SampleNum = 8 * psInfo->u32DataPos / psWavFormatEx->u16BitsPerSample;
		break;

	// -- END --

	case eWAVE_FORMAT_IMA_ADPCM:
		u32SampleNum = (psInfo->u32DataPos / (UINT32)psInfo->u16BytesPerBlock) * psInfo->u16SamplesPerBlock;
		break;

	case eWAVE_FORMAT_PCM:
		switch (psWavFormatEx->u16BitsPerSample) {
		case 16:
			u32SampleNum = psInfo->u32DataPos >> 1;
			break;

		case 8:
			u32SampleNum = psInfo->u32DataPos;
			break;

		default:
			return 0;
		}

		break;

	default:
		return 0;
	} // switch psWavFormatEx->u16FormatTag

	//bz u32SampleNum/psWavFormatEx->u16Channels)*1000 will cause 32bit overflow
	long long	u64Temp1;

	u64Temp1 = ((long long)u32SampleNum / psWavFormatEx->u16Channels) * 1000 / psWavFormatEx->u32SamplesPerSec;
	return (UINT32)u64Temp1;
} // WavFileUtil_Read_GetTimePosition()


//-----------------------------------------------------------------------------
//Description
//	get format block information
//Parameter
//	psInfo[in]
//		pointer point to wav file content
//	pu16SamplesPerBlock[out]
//		sample per block
//	pu16BytesPerBlock[out]
//		bytes per block
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_GetFormatBlockInfo(
	S_WavFileReadInfo	*psInfo,
	PUINT16				pu16SamplesPerBlock,
	PUINT16				pu16BytesPerBlock
)
{
	if (!psInfo || !pu16SamplesPerBlock || !pu16BytesPerBlock)
		return FALSE;

	if (0 == psInfo->u16SamplesPerBlock) {
		S_IMAWavFormat sIMAWavFormat;

		if (!WavFileUtil_Read_GetIMAFormat(psInfo, &sIMAWavFormat))
			return FALSE;

		*pu16SamplesPerBlock = sIMAWavFormat.u16SamplesPerBlock;
		*pu16BytesPerBlock = sIMAWavFormat.u16BlockAlign;
	}
	else {
		*pu16SamplesPerBlock = psInfo->u16SamplesPerBlock;
		*pu16BytesPerBlock = psInfo->u16BytesPerBlock;
	}

	return TRUE;
} // WavFileUtil_Read_GetFormatBlockInfo()


//-----------------------------------------------------------------------------
//Description
//	seek to data position to prepare to read
//Parameter
//	psInfo[in]
//		pointer point to wav file content
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_ReadDataStart(
	S_WavFileReadInfo	*psInfo
)
{
	if (!psInfo || (psInfo->pFile == NULL))
		return FALSE;

	FRESULT eRes;

	eRes = f_lseek(psInfo->pFile, psInfo->u32DataFilePos);
	
	if(eRes != FR_OK)
		return FALSE;
	
//	if (psInfo->u32DataFilePos != lseek(psInfo->i32FileHandle, psInfo->u32DataFilePos, SEEK_SET))
//		return FALSE;

	psInfo->u32DataPos = 0;
	return TRUE;
} // WavFileUtil_Read_ReadDataStart()


//-----------------------------------------------------------------------------
//Description
//	read data from file and fill with buffer
//Parameter
//	psInfo[in]
//		pointer point to wav file content
//	pbyData[out]
//		pointer point to data buffer
//	u32DataSize[in]
//		read data size
//-----------------------------------------------------------------------------
UINT32
WavFileUtil_Read_ReadData(
	S_WavFileReadInfo	*psInfo,
	BYTE				*pbyData,
	UINT32				u32DataSize
)
{
	if (!psInfo || (psInfo->pFile == NULL))
		return 0;

	if ((psInfo->u32DataPos + u32DataSize) >= psInfo->u32DataLen)
		u32DataSize = (psInfo->u32DataLen - psInfo->u32DataPos);

	UINT32 u32BytesRead = 0;

	if (u32DataSize > 0) {
		f_read(psInfo->pFile, pbyData, u32DataSize, &u32BytesRead);

		if (0 == u32BytesRead)
			return 0;

		psInfo->u32DataPos += u32DataSize;
	}

	return u32BytesRead;
} // WavFileUtil_Read_ReadData()


//-----------------------------------------------------------------------------
//Description
//	Clean up WAV file reader variables and close file
//Parameter
//	psInfo[in]
//		pointer point to wav file content
//-----------------------------------------------------------------------------
BOOL
WavFileUtil_Read_Finish(
	S_WavFileReadInfo	*psInfo
)
{
	if (!psInfo || (psInfo->pFile))
		return FALSE;


	psInfo->pFile = NULL;
	return TRUE;
} // WavFileUtil_Read_Finish()
