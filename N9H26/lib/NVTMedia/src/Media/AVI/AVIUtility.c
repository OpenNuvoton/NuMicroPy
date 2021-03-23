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
#include "NVTMedia_SAL_FS.h"

#include "Media/AVI/AVIUtility.h"


#define ENCODE_END   0xCFFFFFFF   // 4GB - 4M 4G is 4294967296

#define MOVIE_CHUNK_START_POS 0x2800

#define MAX_AACSRI	12

//From NVTFAT package
//extern int fsSetFileSize(int hFile, char *suFileName, char *szAsciiName, int64_t n64NewSize);
//extern int  fsDeleteFile(char *suFileName, char *szAsciiName);
//extern int  fsAsciiToUnicode(void *pvASCII, void *pvUniStr, BOOL bIsNullTerm);

static const int32_t s_aiAACSRI[MAX_AACSRI] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025,  8000
};


//Get AAC sample rate index
static BOOL
GetAACSRI(
	int32_t i32SR,
	int32_t *piSRI
)
{
	int32_t i;
	 
	for(i = 0; i < MAX_AACSRI; i ++){
		if(s_aiAACSRI[i] == i32SR){
			break;
		}
	}

	if(i < MAX_AACSRI){
		if(piSRI){
			*piSRI = i;
		}
		return TRUE;
	}
	return FALSE;
}

static uint32_t
AVIUtility_WriteFourCC(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	char *pchszFourCC
)
{
	uint32_t dwFourCC;
	int32_t i32Size = 0;
	
	dwFourCC = mmioFOURCC(
	               pchszFourCC[0],
	               pchszFourCC[1],
	               pchszFourCC[2],
	               pchszFourCC[3]);

	lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);
	i32Size = write(psAVIHandle->i32FileHandle, (uint8_t *)&dwFourCC, 4);
	psAVIHandle->u32CurrentPos = lseek(psAVIHandle->i32FileHandle, 0, SEEK_CUR);

	return i32Size;
}

static uint32_t
AVIUtility_WriteDWORD(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
    uint32_t u32Dw
)
{
	int32_t	i32Size = 0;

	lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);
	i32Size = write(psAVIHandle->i32FileHandle, (uint8_t *)&u32Dw, 4);
	psAVIHandle->u32CurrentPos = lseek(psAVIHandle->i32FileHandle, 0, SEEK_CUR);
	
	return i32Size;
}

static uint32_t
AVIUtility_ReadDWORD(
	int32_t			i32FileHandle,
	uint32_t		*pu32Dw,
	S_AVIFMT_AVI_CONTENT	*psAVIFileContent
)
{
	int32_t i32Size;

	if (lseek(i32FileHandle, psAVIFileContent->u32AVICurrentPos, SEEK_SET) < 0)
		return 0;

	i32Size = read(i32FileHandle, (uint8_t *)pu32Dw, 4);
	psAVIFileContent->u32AVICurrentPos += i32Size;
	return i32Size;
}

static uint32_t
AVIUtility_WriteVOID(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	void* pV,
	uint32_t u32DwSize
)
{
	int32_t i32Size = 0;
	if(u32DwSize == 0)
		return 0;
	i32Size = write(psAVIHandle->i32FileHandle, pV, u32DwSize);
	psAVIHandle->u32CurrentPos = lseek(psAVIHandle->i32FileHandle, 0, SEEK_CUR);
	return i32Size;
}

static uint32_t
AVIUtility_ReadVOID(
	int32_t			i32FileHandle,
	void			*pV,
	uint32_t		u32Size,
	S_AVIFMT_AVI_CONTENT	*psAVIFileContent
)
{
	int32_t i32RetSize;

	if (lseek(i32FileHandle, psAVIFileContent->u32AVICurrentPos, SEEK_SET) < 0)
		return 0;

	i32RetSize = read(i32FileHandle, pV, u32Size);
	psAVIFileContent->u32AVICurrentPos += i32RetSize;
	return i32RetSize;
}

static uint32_t
AVIUtility_WriteTempVOID(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
    void* pV,
    uint32_t u32DwSize
)
{
	int32_t i32Size = 0;
	i32Size = write(psAVIHandle->i32TempFileHandle, pV, u32DwSize);
	psAVIHandle->u32TempPos = lseek(psAVIHandle->i32TempFileHandle, 0, SEEK_CUR);	
	return i32Size;
}

static uint32_t
AVIUtility_ReadTempVOID(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
    void* pV,
    uint32_t u32DwSize)
{
	int32_t i32Size = 0;
	i32Size = read(psAVIHandle->i32TempFileHandle, pV, u32DwSize);
	psAVIHandle->u32TempPos = lseek(psAVIHandle->i32TempFileHandle, 0, SEEK_CUR);	
	return i32Size;
}

static uint32_t
AVIUtility_WriteRIFF(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
    uint32_t u32Size,
    char *pchszParam
)
{
	uint32_t dwPos;
	psAVIHandle-> u32CurrentPos= 0;
	dwPos = 0;

	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"RIFF");
	dwPos += AVIUtility_WriteDWORD(psAVIHandle, u32Size);
	dwPos += AVIUtility_WriteFourCC(psAVIHandle, pchszParam);
	return dwPos;
}

static uint32_t
AVIUtility_WriteLIST(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
    uint32_t u32Size,
    char * pchszParam
)
{
	uint32_t dwPos = 0;
	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"LIST");
	dwPos += AVIUtility_WriteDWORD(psAVIHandle, u32Size);
	dwPos += AVIUtility_WriteFourCC(psAVIHandle,pchszParam);
	return dwPos;
}

static uint32_t
AVIUtility_GenJunkChunk(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
    uint32_t u32DwSizeJunk
)
{
	uint32_t dwPos, i;
	uint8_t *pcbJunk;
	char achDummyBuf[512];

	dwPos = 0;

	pcbJunk = (uint8_t *)achDummyBuf;
	for (i = 0; i < 512; i++)
		pcbJunk[i] = 0;
	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"JUNK");
	dwPos += AVIUtility_WriteDWORD(psAVIHandle, u32DwSizeJunk);

	while (u32DwSizeJunk > 0)
	{
		if (u32DwSizeJunk > 512)
		{
			dwPos += AVIUtility_WriteVOID(psAVIHandle, pcbJunk, 512);
			u32DwSizeJunk -= 512;
		}
		else
		{
			dwPos += AVIUtility_WriteVOID(psAVIHandle, pcbJunk, u32DwSizeJunk);
			u32DwSizeJunk = 0;
		}
	}
	return dwPos;
}

static uint32_t
AVIUtility_GenAVI20Chunk(
	S_AVIUTIL_AVIHANDLE *psAVIHandle
)
{
	uint32_t dwPos, dwSizeJunk, i;
	uint8_t *pcbJunk;
	char achDummyBuf[512];

	dwSizeJunk = 0xF8;
	dwPos = 0;
	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"LIST");
	dwPos += AVIUtility_WriteDWORD(psAVIHandle, 0x104);
	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"odml");
	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"dmlh");
	dwPos += AVIUtility_WriteDWORD(psAVIHandle, dwSizeJunk);

	pcbJunk = (uint8_t *)achDummyBuf;
	for (i = 0; i < 512; i++)
		pcbJunk[i] = 0;

	while (dwSizeJunk > 0)
	{
		if (dwSizeJunk > 512)
		{
			dwPos += AVIUtility_WriteVOID(psAVIHandle, pcbJunk, 512);
			dwSizeJunk -= 512;
		}
		else
		{
			dwPos += AVIUtility_WriteVOID(psAVIHandle, pcbJunk, dwSizeJunk);
			dwSizeJunk = 0;
		}
	}

	return dwPos;
}


static uint32_t
AVIUtility_GenListMoviChunkHeader(
	S_AVIUTIL_AVIHANDLE *psAVIHandle
)
{

	uint32_t dwSizeMOVI, dwPos;

	psAVIHandle->u32CurrentPos = MOVIE_CHUNK_START_POS;
	dwSizeMOVI = psAVIHandle->sAVICtx.u32SizeMOVI;
	dwPos = AVIUtility_WriteLIST(psAVIHandle, dwSizeMOVI, (char*)"movi");
	return dwPos;
}

static uint32_t
AVIUtility_GenMainAviHeader(
	S_AVIUTIL_AVIHANDLE *psAVIHandle
)
{
	S_AVIFMT_MAINAVIHEADER sMainAVIHeader;
	uint32_t u32SizeRIFF;
	uint32_t u32SizeHDRL; // alternative 0x166 if audio available;
	// RIFF(u32SizeRIFF)AVI
	//		LIST(00DA)hdrl
	//			avih(0038)<MainAVIHeader>
	uint32_t u32Pos, i;

	sMainAVIHeader.u32MicroSecPerFrame = (uint32_t)1000000 / (psAVIHandle->sAVICtx.u32FrameRate) ;
	sMainAVIHeader.u32MaxBytesPerSec = 0;
	sMainAVIHeader.u32PaddingGranularity = 0x0;
	sMainAVIHeader.u32Flags = AVIF_WASCAPTUREFILE | AVIF_HASINDEX | AVIF_MUSTUSEINDEX; //0x110;	// AVIF_HASINDEX | AVIF_TRUSTCKTYPE;
	sMainAVIHeader.u32TotalFrames = psAVIHandle->sAVICtx.u32TotalFrames;
	sMainAVIHeader.u32InitialFrames = 0;
	sMainAVIHeader.u32Streams = psAVIHandle->sAVICtx.u32Streams;
	sMainAVIHeader.u32SuggestedBufferSize = psAVIHandle->sAVICtx.u32SuggestedBufferSize;
	sMainAVIHeader.u32Width = (uint32_t)(psAVIHandle->sAVICtx.i32Width);
	sMainAVIHeader.u32Height = (uint32_t)(psAVIHandle->sAVICtx.i32Height);
	for (i = 0; i < 4; i++)
		sMainAVIHeader.u32Reserved[i] = 0;

	u32SizeRIFF = psAVIHandle->sAVICtx.u32SizeRIFF;
	u32SizeHDRL = psAVIHandle->sAVICtx.u32SizeHDRL;
	// RIFF(u32SizeRIFF)AVI
	//		LIST(00DA)hdrl
	//			avih(0038)<MainAVIHeader>

	u32Pos = 0;

	u32Pos += AVIUtility_WriteRIFF(psAVIHandle, u32SizeRIFF, (char*)"AVI ");
	u32Pos += AVIUtility_WriteLIST(psAVIHandle, u32SizeHDRL, (char*)"hdrl");
	u32Pos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"avih");
	u32Pos += AVIUtility_WriteDWORD(psAVIHandle, 0x0038);
	u32Pos += AVIUtility_WriteVOID(psAVIHandle, (void*) & sMainAVIHeader, sizeof(S_AVIFMT_MAINAVIHEADER));

	return u32Pos;
}

static uint32_t
AVIUtility_GenVideoStreamHeaderChunk(
	S_AVIUTIL_AVIHANDLE *psAVIHandle
)
{
	uint32_t dwPos;
	S_AVIFMT_AVISTREAMHEADER aStreamHeader;
	S_AVIFMT_BITMAPINFOHEADER bmpInfoHeader;

	// LIST(008E)strl
	//	strh(0038)	<AVIStreamHeader>
	//	strf(0028)	<BITMAPINFOHEADER>
	//	strn(0012)	<streamName>

	//<1> LIST(008E)strl

	dwPos = 0;
	dwPos += AVIUtility_WriteLIST(psAVIHandle, 0x1094, (char*)"strl"); //SIZE_STRL, "strl" );
    memset( &aStreamHeader, 0, sizeof(S_AVIFMT_AVISTREAMHEADER) );

	aStreamHeader.fccType = mmioFOURCC('v', 'i', 'd', 's');
	aStreamHeader.fccHandler = psAVIHandle->sAVICtx.u32VideoFcc;

	aStreamHeader.u32Scale = (uint32_t)(psAVIHandle->u64LastFrameTime - psAVIHandle->u64FirstFrameTime);
	aStreamHeader.u32Rate = 1000 * (psAVIHandle->sAVICtx.u32TotalFrames);
	aStreamHeader.u32Length = psAVIHandle->sAVICtx.u32TotalFrames;
	aStreamHeader.u32SuggestedBufferSize = psAVIHandle->u32VideoSuggSize;
	aStreamHeader.u16Right_X = psAVIHandle->sAVICtx.i32Width;
	aStreamHeader.u16Bottom_Y = psAVIHandle->sAVICtx.i32Height;
	
	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"strh");
	dwPos += AVIUtility_WriteDWORD(psAVIHandle, 0x0038); //SIZE_STRH );
	dwPos += AVIUtility_WriteVOID(psAVIHandle, &aStreamHeader, 0x0038); //SIZE_STRH );

	//<3> strf(0028)<BITMAPINFOHEADER>
	bmpInfoHeader.biSize = 0x28;
	bmpInfoHeader.biCompression = psAVIHandle->sAVICtx.u32VideoFcc;
	//KC bmpInfoHeader.biBitCount = 12;
	bmpInfoHeader.biBitCount = 24;
	bmpInfoHeader.biPlanes = 1;
	bmpInfoHeader.biHeight = psAVIHandle->sAVICtx.i32Height;
	bmpInfoHeader.biWidth = psAVIHandle->sAVICtx.i32Width;
	bmpInfoHeader.biSizeImage = (psAVIHandle->sAVICtx.i32Width) * (psAVIHandle->sAVICtx.i32Height) * 3 / 2;
	bmpInfoHeader.biClrUsed = 0;
	bmpInfoHeader.biClrImportant = 0;
	bmpInfoHeader.biXPelsPerMeter = 0;
	bmpInfoHeader.biYPelsPerMeter = 0;
	//
	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"strf");
	dwPos += AVIUtility_WriteDWORD(psAVIHandle, 0x0028); //SIZE_STRF );
	dwPos += AVIUtility_WriteVOID(psAVIHandle, &bmpInfoHeader, 0x0028); //SIZE_STRF );

	dwPos += AVIUtility_GenJunkChunk(psAVIHandle, 0x1018);
	return dwPos;
}

static uint32_t
AVIUtility_GenAudioStreamHeaderChunk(
	S_AVIUTIL_AVIHANDLE *psAVIHandle
)
{
	uint32_t dwPos;
	S_AVIFMT_AVISTREAMHEADER aStreamHeader;
	S_AVIFMT_WAVEFORMATEX    audInfoHeader;
	S_AVIFMT_MP3WAVEFORMAT	audMP3InfoHeader;
	S_AVIFMT_IMAADPCMWAVEFORMAT audIMAAdpmcInfoHeader;
	S_AVIFMT_AACWAVEFORMAT	audAACInfoHeader;

	dwPos = 0;
	dwPos += AVIUtility_WriteLIST(psAVIHandle, 0x1080, (char*)"strl"); //SIZE_STRL, "strl" );

	//<2> strh(0038)<AVIStreamHeader>
	aStreamHeader.fccType = mmioFOURCC('a', 'u', 'd', 's');
	aStreamHeader.fccHandler = 1;
	aStreamHeader.u32Flags = 0;
	aStreamHeader.u16Priority = 0;
	aStreamHeader.u16Language = 0;
	aStreamHeader.u32InitialFrames = 0;
	aStreamHeader.u32SuggestedBufferSize = psAVIHandle->u32AudioSuggSize;

	if(psAVIHandle->sAVICtx.u32AudioFmtTag == ULAW_FMT_TAG){
		audInfoHeader.u16FormatTag = psAVIHandle->sAVICtx.u32AudioFmtTag;
		audInfoHeader.u16Channels = psAVIHandle->sAVICtx.u32Channel;
		audInfoHeader.u32SamplesPerSec = psAVIHandle->sAVICtx.u32SamplingRate;
		audInfoHeader.u16BlockAlign = psAVIHandle->sAVICtx.u32AudioFrameSize;
		audInfoHeader.u16BitsPerSample = 8;
		audInfoHeader.u32AvgBytesPerSec =  audInfoHeader.u32SamplesPerSec * (audInfoHeader.u16BitsPerSample/8) * audInfoHeader.u16Channels;
		audInfoHeader.u16cbSize = 0;

		aStreamHeader.u32Scale = audInfoHeader.u16BlockAlign;
		aStreamHeader.u32Rate = audInfoHeader.u32AvgBytesPerSec;
		aStreamHeader.u32SampleSize = audInfoHeader.u16BlockAlign;
	}
	else if(psAVIHandle->sAVICtx.u32AudioFmtTag == ALAW_FMT_TAG){
		audInfoHeader.u16FormatTag = psAVIHandle->sAVICtx.u32AudioFmtTag;
		audInfoHeader.u16Channels = psAVIHandle->sAVICtx.u32Channel;
		audInfoHeader.u32SamplesPerSec = psAVIHandle->sAVICtx.u32SamplingRate;
		audInfoHeader.u16BlockAlign = psAVIHandle->sAVICtx.u32AudioFrameSize;
		audInfoHeader.u16BitsPerSample = 8;
		audInfoHeader.u32AvgBytesPerSec =  audInfoHeader.u32SamplesPerSec * (audInfoHeader.u16BitsPerSample/8) * audInfoHeader.u16Channels;
		audInfoHeader.u16cbSize = 0;

		aStreamHeader.u32Scale = audInfoHeader.u16BlockAlign;
		aStreamHeader.u32Rate = audInfoHeader.u32AvgBytesPerSec;
		aStreamHeader.u32SampleSize = audInfoHeader.u16BlockAlign;
	}
	else if(psAVIHandle->sAVICtx.u32AudioFmtTag == PCM_FMT_TAG){
		audInfoHeader.u16FormatTag = psAVIHandle->sAVICtx.u32AudioFmtTag;
		audInfoHeader.u16Channels = psAVIHandle->sAVICtx.u32Channel;
		audInfoHeader.u32SamplesPerSec = psAVIHandle->sAVICtx.u32SamplingRate;
		audInfoHeader.u16BlockAlign = 2;

		audInfoHeader.u16BitsPerSample = 16;;
		audInfoHeader.u32AvgBytesPerSec =  audInfoHeader.u32SamplesPerSec * (audInfoHeader.u16BitsPerSample/8) * audInfoHeader.u16Channels;
		audInfoHeader.u16cbSize = 0;

		aStreamHeader.u32Scale = audInfoHeader.u16BlockAlign;
		aStreamHeader.u32Rate = audInfoHeader.u32AvgBytesPerSec;
		aStreamHeader.u32SampleSize = audInfoHeader.u16BlockAlign;
	}
	else if(psAVIHandle->sAVICtx.u32AudioFmtTag == MP3_FMT_TAG){
		audMP3InfoHeader.sWFX.u16FormatTag = psAVIHandle->sAVICtx.u32AudioFmtTag;
		audMP3InfoHeader.sWFX.u16Channels = psAVIHandle->sAVICtx.u32Channel;
		audMP3InfoHeader.sWFX.u32SamplesPerSec = psAVIHandle->sAVICtx.u32SamplingRate;
		audMP3InfoHeader.sWFX.u16BlockAlign = psAVIHandle->sAVICtx.u32AudioFrameSize;

		audMP3InfoHeader.sWFX.u16BitsPerSample = 0;
//		audMP3InfoHeader.sWFX.u32AvgBytesPerSec =  (psAVIHandle->sAVICtx.u32SamplingRate * psAVIHandle->sAVICtx.u32AudioFrameSize) / psAVIHandle->sAVICtx.u32AudioSamplesPerFrame;
		audMP3InfoHeader.sWFX.u32AvgBytesPerSec = psAVIHandle->sAVICtx.u32AudioBitRate / 8;
		audMP3InfoHeader.sWFX.u16cbSize = 12;
		audMP3InfoHeader.u16WId = 0x0001;
		audMP3InfoHeader.u32Flags = 0x00000002;
		audMP3InfoHeader.u16BlockSize = psAVIHandle->sAVICtx.u32AudioFrameSize;
		audMP3InfoHeader.u16FramesPerBlock = 0x0001;
		audMP3InfoHeader.u16CodecDelay = 0;
		
		aStreamHeader.u32Scale = audMP3InfoHeader.sWFX.u16BlockAlign;
		aStreamHeader.u32Rate = audMP3InfoHeader.sWFX.u32AvgBytesPerSec;
		aStreamHeader.u32SampleSize = audMP3InfoHeader.sWFX.u16BlockAlign;
	}
	else if(psAVIHandle->sAVICtx.u32AudioFmtTag == IMA_ADPCM_FMT_TAG){
		audIMAAdpmcInfoHeader.sWFX.u16FormatTag = psAVIHandle->sAVICtx.u32AudioFmtTag;
		audIMAAdpmcInfoHeader.sWFX.u16Channels = psAVIHandle->sAVICtx.u32Channel;
		audIMAAdpmcInfoHeader.sWFX.u32SamplesPerSec = psAVIHandle->sAVICtx.u32SamplingRate;
		audIMAAdpmcInfoHeader.sWFX.u16BlockAlign = psAVIHandle->sAVICtx.u32AudioFrameSize;

		audIMAAdpmcInfoHeader.sWFX.u16BitsPerSample = 4;
//		audIMAAdpmcInfoHeader.sWFX.u32AvgBytesPerSec = (psAVIHandle->sAVICtx.u32SamplingRate * psAVIHandle->sAVICtx.u32AudioFrameSize)/psAVIHandle->sAVICtx.u32AudioSamplesPerFrame;
		audMP3InfoHeader.sWFX.u32AvgBytesPerSec = psAVIHandle->sAVICtx.u32AudioBitRate / 8;
		audIMAAdpmcInfoHeader.sWFX.u16cbSize = 0x2;
		audIMAAdpmcInfoHeader.u16SamplePerBlock = (uint16_t)psAVIHandle->sAVICtx.u32AudioSamplesPerFrame;
		
		aStreamHeader.u32Scale = audIMAAdpmcInfoHeader.sWFX.u16BlockAlign;
		aStreamHeader.u32Rate = audIMAAdpmcInfoHeader.sWFX.u32AvgBytesPerSec;
		aStreamHeader.u32SampleSize = audIMAAdpmcInfoHeader.sWFX.u16BlockAlign;
	}
	else if (psAVIHandle->sAVICtx.u32AudioFmtTag == AAC_FMT_TAG) {
		int32_t i32SRI;
		int32_t i32Profile;
		int32_t i32DSRI;
		int32_t i32SyncExtType;
		
		audAACInfoHeader.sWFX.u16FormatTag = psAVIHandle->sAVICtx.u32AudioFmtTag;
		audAACInfoHeader.sWFX.u16Channels = psAVIHandle->sAVICtx.u32Channel;
		audAACInfoHeader.sWFX.u32SamplesPerSec = psAVIHandle->sAVICtx.u32SamplingRate;
		audAACInfoHeader.sWFX.u16BitsPerSample = 0;
		audAACInfoHeader.sWFX.u32AvgBytesPerSec = psAVIHandle->sAVICtx.u32AudioBitRate / 8;
		audAACInfoHeader.sWFX.u16BlockAlign = 4096; // ???, I don't know.
		audAACInfoHeader.sWFX.u16cbSize = 5;

		GetAACSRI(psAVIHandle->sAVICtx.u32SamplingRate, &i32SRI);
		i32Profile = 2;  //psAVIHandle->u32AACProfile; //Profile, 1 = main, 2 = LC, 3 = SSR, 4 = LTP
		GetAACSRI(psAVIHandle->sAVICtx.u32SamplingRate * 2, &i32DSRI);
		i32SyncExtType = 0x2B7;
		
		audAACInfoHeader.u8Profile = (uint8_t)((i32Profile) << 3 | (i32SRI >> 1));
		audAACInfoHeader.u8SRI = (uint8_t)(((i32SRI & 0x1) << 7) | (1 << 3));
		audAACInfoHeader.u8SyncExtType_H = (uint8_t)(i32SyncExtType >> 3) & 0xFF;
		audAACInfoHeader.u8SyncExtType_L = (uint8_t)(((i32SyncExtType & 0x7) << 5) | 5);
		audAACInfoHeader.u8DSRI = (uint8_t)((1 & 0x1) << 7) | (i32DSRI << 3);

		aStreamHeader.u32Scale = psAVIHandle->sAVICtx.u32AudioSamplesPerFrame;	//AAC default ecode samples per frame
		aStreamHeader.u32Rate = psAVIHandle->sAVICtx.u32SamplingRate;
		aStreamHeader.u32SampleSize = 0;
	}
	else{

	}

	aStreamHeader.u32Start = 0;
	////
	aStreamHeader.u32Length = psAVIHandle->u32AudioFrame;
	////
	aStreamHeader.u32Quality = 0xFFFFFFFF;
	aStreamHeader.u16Left_X = 0;
	aStreamHeader.u16Top_Y = 0;
	aStreamHeader.u16Right_X = 0;
	aStreamHeader.u16Bottom_Y = 0;
	//UINT32 dwTemp = sizeof(RECT);
	//
	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"strh");
	dwPos += AVIUtility_WriteDWORD(psAVIHandle, 0x0038); //SIZE_STRH );
	dwPos += AVIUtility_WriteVOID(psAVIHandle, &aStreamHeader, 0x0038); //SIZE_STRH );

	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"strf");
	
	uint32_t u32JunkChunkSize = 0x102c;

	if(psAVIHandle->sAVICtx.u32AudioFmtTag == MP3_FMT_TAG){
		dwPos += AVIUtility_WriteDWORD(psAVIHandle, sizeof(S_AVIFMT_MP3WAVEFORMAT)); //SIZE_STRF );
		dwPos += AVIUtility_WriteVOID(psAVIHandle, &audMP3InfoHeader, sizeof(S_AVIFMT_MP3WAVEFORMAT)); //SIZE_STRF );
		dwPos += AVIUtility_GenJunkChunk(psAVIHandle, (u32JunkChunkSize - sizeof(S_AVIFMT_MP3WAVEFORMAT)));
	}
	else if(psAVIHandle->sAVICtx.u32AudioFmtTag == IMA_ADPCM_FMT_TAG){
		dwPos += AVIUtility_WriteDWORD(psAVIHandle, sizeof(S_AVIFMT_IMAADPCMWAVEFORMAT)); //SIZE_STRF );
		dwPos += AVIUtility_WriteVOID(psAVIHandle, &audIMAAdpmcInfoHeader, sizeof(S_AVIFMT_IMAADPCMWAVEFORMAT)); //SIZE_STRF );
		dwPos += AVIUtility_GenJunkChunk(psAVIHandle, (u32JunkChunkSize - sizeof(S_AVIFMT_IMAADPCMWAVEFORMAT)));
	}
	else if (psAVIHandle->sAVICtx.u32AudioFmtTag == AAC_FMT_TAG) {
		dwPos += AVIUtility_WriteDWORD(psAVIHandle, sizeof(S_AVIFMT_AACWAVEFORMAT)); //SIZE_STRF );
		dwPos += AVIUtility_WriteVOID(psAVIHandle, &audAACInfoHeader, sizeof(S_AVIFMT_AACWAVEFORMAT)); //SIZE_STRF );
		dwPos += AVIUtility_GenJunkChunk(psAVIHandle, (u32JunkChunkSize - sizeof(S_AVIFMT_AACWAVEFORMAT)));
	}
	else{
		dwPos += AVIUtility_WriteDWORD(psAVIHandle, sizeof(S_AVIFMT_WAVEFORMATEX)); //SIZE_STRF );
		dwPos += AVIUtility_WriteVOID(psAVIHandle, &audInfoHeader, sizeof(S_AVIFMT_WAVEFORMATEX)); //SIZE_STRF );
		dwPos += AVIUtility_GenJunkChunk(psAVIHandle, (u32JunkChunkSize - sizeof(S_AVIFMT_WAVEFORMATEX)));
	}

	return dwPos;
}



ERRCODE
AVIUtility_CreateAVIFile(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	S_AVIUTIL_FILEATTR *psAVIFileAttr, 
	char *szTempFilePath		//Used if psAVIHandle->bIdxUseFile == TRUE
)
{
//	char achBuf[512];
//	int32_t i;
	uint32_t dwPos;
	int32_t i32FileWriteSize;


	if(psAVIHandle->i32FileHandle < 0)
		return ERR_AVIUTIL_FILE_HANDLE;

	if((psAVIFileAttr->u32VideoFcc == 0) && (psAVIFileAttr->u32AudioFmtTag == 0))
		return ERR_AVIUTIL_CODEC_FMT;
		
	if((psAVIHandle->bIdxUseFile == TRUE) && (szTempFilePath == NULL)){
		return ERR_AVIUTIL_INVALID_PARAM;
	}

	if(psAVIHandle->u32MBResvSize){
		i32FileWriteSize = write(psAVIHandle->i32FileHandle,"aaaa",4);
		ftruncate(psAVIHandle->i32FileHandle, 1024*1024*psAVIHandle->u32MBResvSize); // Reserved SD Card space 
		lseek(psAVIHandle->i32FileHandle, 0, SEEK_SET);		
	}

	memset(&(psAVIHandle->sAVICtx), 0x0, sizeof(psAVIHandle->sAVICtx));	
	psAVIHandle->sAVICtx.i32Width = psAVIFileAttr->u32FrameWidth;
	psAVIHandle->sAVICtx.i32Height = psAVIFileAttr->u32FrameHeight;
	psAVIHandle->sAVICtx.u32FrameRate = psAVIFileAttr->u32FrameRate;
	psAVIHandle->sAVICtx.u32SamplingRate = psAVIFileAttr->u32SamplingRate;
	psAVIHandle->sAVICtx.u32Channel = psAVIFileAttr->u32AudioChannel;	
	psAVIHandle->sAVICtx.u32AudioFrameSize = psAVIFileAttr->u32AudioFrameSize;	
	psAVIHandle->sAVICtx.u32AudioSamplesPerFrame = psAVIFileAttr->u32AudioSamplesPerFrame;
	psAVIHandle->sAVICtx.u32AudioBitRate = psAVIFileAttr->u32AudioBitRate;
	psAVIHandle->sAVICtx.u32SizeHDRL = 0x2274;

	psAVIHandle->u32AudioFrame = 0;
	psAVIHandle->u32FixedNo = 0;
	psAVIHandle->u32TempPos = 0;

	if(psAVIHandle->bIdxUseFile == TRUE){
		char *szTempFileName = malloc(strlen(szTempFilePath) + 14);
//		char szUnicodeFileName[MAX_FILE_NAME_LEN];

		if(szTempFileName == NULL)
			return ERR_AVIUTIL_MALLOC;

		sprintf(szTempFileName, "%s\\%x.tmp", szTempFilePath, psAVIHandle->i32FileHandle);
//		fsAsciiToUnicode(szTempFileName, szUnicodeFileName, TRUE); 

//		if((psAVIHandle->i32TempFileHandle = fsOpenFile(szUnicodeFileName, NULL, O_CREATE|O_RDWR)) < 0){
		if((psAVIHandle->i32TempFileHandle = open(szTempFileName, O_CREATE|O_RDWR)) < 0){
			free(szTempFileName);
			return ERR_AVIUTIL_CRETAE_TMP_FILE;
		}
	
		if(psAVIHandle->u32MBResvSize){
			i32FileWriteSize = write(psAVIHandle->i32TempFileHandle,"aaaa",4);
			ftruncate(psAVIHandle->i32TempFileHandle, 1024);
		}
		lseek(psAVIHandle->i32TempFileHandle, 0, SEEK_SET);

		free(szTempFileName);
	}


	psAVIHandle->sAVICtx.u32Streams = 0;
	psAVIHandle->sAVICtx.u32VideoFcc = psAVIFileAttr->u32VideoFcc;
	psAVIHandle->sAVICtx.u32AudioFmtTag = psAVIFileAttr->u32AudioFmtTag;

	psAVIHandle->u32CurrentPos = 0;
	psAVIHandle->u32VideoSuggSize = 0;
	psAVIHandle->u32AudioSuggSize = 0;

	if(psAVIFileAttr->u32VideoFcc != 0){
		psAVIHandle->sAVICtx.u32Streams ++;
	}
	if(psAVIFileAttr->u32AudioFmtTag != 0){
		psAVIHandle->sAVICtx.u32Streams ++;
	}
	
	dwPos = 0;
	dwPos += AVIUtility_GenMainAviHeader(psAVIHandle);

	if(psAVIFileAttr->u32VideoFcc != 0)
		dwPos += AVIUtility_GenVideoStreamHeaderChunk(psAVIHandle);
	if(psAVIFileAttr->u32AudioFmtTag != 0)
		dwPos += AVIUtility_GenAudioStreamHeaderChunk(psAVIHandle);
	
	dwPos += AVIUtility_GenAVI20Chunk(psAVIHandle);
	psAVIHandle->sAVICtx.u32SizeHDRL = dwPos - 0x14;
	
	dwPos += AVIUtility_GenJunkChunk(psAVIHandle, MOVIE_CHUNK_START_POS - dwPos - 8); //8:"JUNKxxxxxxxx"

	if (dwPos != MOVIE_CHUNK_START_POS) {
		NMLOG_ERROR("Please check AVI header \n");
	}

	dwPos += AVIUtility_GenListMoviChunkHeader(psAVIHandle);
	return ERR_AVIUTIL_SUCCESS;
}

uint32_t						// add audio frame to AVI file
AVIUtility_AddAudioFrame(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	uint8_t * pbyAudioBuf,		// audio buffer [in], should reserve 8 bits in the front.
	uint32_t u32AudioSize		// audio size
)
{
	uint32_t retval; //, dwAudioFrame;
	uint32_t u32TotalWriteSize=0;
	char *sz00wb = "00wb";
	char *sz01wb = "01wb";
	char *szFourCC = NULL;
	uint32_t u32FourCC;
	uint32_t u32RealAudioSize;
	char achPadding[4] = {0,0,0,0};
	uint32_t u32PaddingBytes = 0;

	if (psAVIHandle->sAVICtx.u32AudioFmtTag == AAC_FMT_TAG){
		//There is ADTS header in bitstream, remove it!
		if((pbyAudioBuf[0] == 0xFF) && ((pbyAudioBuf[1] & 0xF0) == 0xF0)){
			if(pbyAudioBuf[1] & 0x01){
				//without CRC
				pbyAudioBuf = pbyAudioBuf + 7;
				u32AudioSize = u32AudioSize - 7;
			}
			else{
				//with CRC
				pbyAudioBuf = pbyAudioBuf + 9;
				u32AudioSize = u32AudioSize - 9;
			}
		}
	}


	u32RealAudioSize = u32AudioSize;

	if(u32RealAudioSize & 1){	//align 2
		u32PaddingBytes ++;
	}

	psAVIHandle->u32AudioFrame ++;

	if (psAVIHandle->sAVICtx.u32Streams == 2) {
		szFourCC = sz01wb;
		u32FourCC = mmioFOURCC('0', '1', 'w', 'b');
	}
	else {
		szFourCC = sz00wb;
		u32FourCC = mmioFOURCC('0', '0', 'w', 'b');
	}

	
	if(psAVIHandle->bIdxUseFile == TRUE){
		psAVIHandle->sAVICtx.psaAVIdxChunkInfo[psAVIHandle->u32FixedNo].ckid = u32FourCC;
		psAVIHandle->sAVICtx.psaAVIdxChunkInfo[psAVIHandle->u32FixedNo].dwChunkLength = u32RealAudioSize;
		psAVIHandle->u32FixedNo++;
	}
	else{
		psAVIHandle->sAVICtx.psaAVIdxChunkInfo[psAVIHandle->sAVICtx.u32arrCounter].ckid = u32FourCC;
		psAVIHandle->sAVICtx.psaAVIdxChunkInfo[psAVIHandle->sAVICtx.u32arrCounter].dwChunkLength = u32RealAudioSize;
	}

	psAVIHandle->sAVICtx.u32arrCounter++;

	if(psAVIHandle->u32AudioSuggSize < (u32RealAudioSize + u32PaddingBytes))
		psAVIHandle->u32AudioSuggSize = u32RealAudioSize + u32PaddingBytes;
	
	if((retval = AVIUtility_WriteFourCC(psAVIHandle, (char*)szFourCC)) != 4){
		psAVIHandle->u32CurrentPos -= retval;
		lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);

		if(psAVIHandle->bIdxUseFile == TRUE)
			psAVIHandle->u32FixedNo --;

		psAVIHandle->sAVICtx.u32arrCounter--;
		psAVIHandle->u32AudioFrame --;
		NMLOG_ERROR("Write audio fourCC fail \n");
		return 0;
	}

	u32TotalWriteSize+=4;

	if((retval = AVIUtility_WriteDWORD(psAVIHandle, u32RealAudioSize + u32PaddingBytes)) != 4 ){
		psAVIHandle->u32CurrentPos -= (retval+4);
		lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);

		if(psAVIHandle->bIdxUseFile == TRUE)
			psAVIHandle->u32FixedNo --;

		psAVIHandle->sAVICtx.u32arrCounter--;
		psAVIHandle->u32AudioFrame --;
		NMLOG_ERROR("Write audio size fail \n");
		return 0;
	}
	u32TotalWriteSize+=4;

	if((retval = AVIUtility_WriteVOID(psAVIHandle, pbyAudioBuf, u32AudioSize)) != u32AudioSize){
		psAVIHandle->u32CurrentPos -= (retval+8);
		lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);

		if(psAVIHandle->bIdxUseFile == TRUE)
			psAVIHandle->u32FixedNo --;

		psAVIHandle->sAVICtx.u32arrCounter--;
		psAVIHandle->u32AudioFrame --;
		NMLOG_ERROR("Write audio data fail");
		return 0;
	}

	u32TotalWriteSize+=u32AudioSize;

	
	// Write padding byte
	if(u32PaddingBytes){
		if ((retval = AVIUtility_WriteVOID(psAVIHandle, achPadding, u32PaddingBytes)) != u32PaddingBytes) {
			psAVIHandle->u32CurrentPos -= (retval + u32TotalWriteSize);
			lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);

			if (psAVIHandle->bIdxUseFile == TRUE)
				psAVIHandle->u32FixedNo --;

			psAVIHandle->sAVICtx.u32arrCounter--;
			return 0;
		}
	}

	u32TotalWriteSize += u32PaddingBytes;

	
	if(psAVIHandle->bIdxUseFile == TRUE){
		if(psAVIHandle->u32FixedNo >= 2048){
			if((retval = AVIUtility_WriteTempVOID(psAVIHandle, (uint8_t *)(&psAVIHandle->sAVICtx.psaAVIdxChunkInfo[0]), sizeof(S_AVIFMT_AVIINDEXENTRY) * psAVIHandle->u32FixedNo)) != (sizeof(S_AVIFMT_AVIINDEXENTRY) * psAVIHandle->u32FixedNo)){
				psAVIHandle->u32TempPos -= retval;
				lseek(psAVIHandle->i32TempFileHandle, psAVIHandle->u32TempPos, SEEK_SET);
				NMLOG_ERROR("Write chunk info fail");
				return 0;
			}
			psAVIHandle->u32FixedNo = 0;
		}
	}

	if(u32AudioSize > psAVIHandle->sAVICtx.u32AudioFrameSize)
		psAVIHandle->sAVICtx.u32AudioFrameSize = u32AudioSize;

	return u32TotalWriteSize;
}

uint32_t
AVIUtility_AddVideoFrame(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
    uint8_t* pbyFrameBuf,
    uint32_t u32FrameSize
)
{
	uint32_t retval;
	uint32_t u32TotalWriteSize = 0;
	uint32_t u32RealVideoSize;
	char achPadding[4] = {0,0,0,0};
	uint32_t u32PaddingBytes = 0;

	u32RealVideoSize = u32FrameSize;

	if (u32RealVideoSize & 1) {
		u32PaddingBytes++;
	}

	if (psAVIHandle->bIdxUseFile == TRUE) {
		psAVIHandle->sAVICtx.psaAVIdxChunkInfo[psAVIHandle->u32FixedNo].ckid = mmioFOURCC('0', '0', 'd', 'b');
		psAVIHandle->sAVICtx.psaAVIdxChunkInfo[psAVIHandle->u32FixedNo].dwChunkLength = u32RealVideoSize;
		psAVIHandle->u32FixedNo++;
	}
	else {
		psAVIHandle->sAVICtx.psaAVIdxChunkInfo[psAVIHandle->sAVICtx.u32arrCounter].ckid = mmioFOURCC('0', '0', 'd', 'b');
		psAVIHandle->sAVICtx.psaAVIdxChunkInfo[psAVIHandle->sAVICtx.u32arrCounter].dwChunkLength = u32RealVideoSize;
	}

	psAVIHandle->sAVICtx.u32arrCounter++;

	if (psAVIHandle->u32VideoSuggSize < (u32RealVideoSize + u32PaddingBytes))
		psAVIHandle->u32VideoSuggSize = u32RealVideoSize + u32PaddingBytes;

	// Write video chunk start code "00db"
	if ((retval = AVIUtility_WriteFourCC(psAVIHandle, (char*)"00db")) != 4) {
		psAVIHandle->u32CurrentPos -= retval;
		lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);

		if (psAVIHandle->bIdxUseFile == TRUE)
			psAVIHandle->u32FixedNo --;

		psAVIHandle->sAVICtx.u32arrCounter--;
		return 0;
	}

	// Write video chunk size
	if ((retval = AVIUtility_WriteDWORD(psAVIHandle, u32RealVideoSize + u32PaddingBytes )) != 4) {
		psAVIHandle->u32CurrentPos -= (retval + u32TotalWriteSize);
		lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);

		if (psAVIHandle->bIdxUseFile == TRUE)
			psAVIHandle->u32FixedNo --;

		psAVIHandle->sAVICtx.u32arrCounter--;
		return 0;
	}
	
	u32TotalWriteSize += 4;
	
	// Write frame data
	if ((retval = AVIUtility_WriteVOID(psAVIHandle, pbyFrameBuf, u32FrameSize)) != u32FrameSize) {
		psAVIHandle->u32CurrentPos -= (retval + u32TotalWriteSize);
		lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);

		if (psAVIHandle->bIdxUseFile == TRUE)
			psAVIHandle->u32FixedNo --;

		psAVIHandle->sAVICtx.u32arrCounter--;
		return 0;
	}

	u32TotalWriteSize += u32FrameSize;

	// Write padding byte
	if(u32PaddingBytes){
		if ((retval = AVIUtility_WriteVOID(psAVIHandle, achPadding, u32PaddingBytes)) != u32PaddingBytes) {
			psAVIHandle->u32CurrentPos -= (retval + u32TotalWriteSize);
			lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);

			if (psAVIHandle->bIdxUseFile == TRUE)
				psAVIHandle->u32FixedNo --;

			psAVIHandle->sAVICtx.u32arrCounter--;
			return 0;
		}
	}

	u32TotalWriteSize += u32PaddingBytes;

	if (psAVIHandle->u32CurrentPos > ENCODE_END) {  // almost near 4 GB
		psAVIHandle->u32CurrentPos -= (retval + u32TotalWriteSize);
		lseek(psAVIHandle->i32FileHandle, psAVIHandle->u32CurrentPos, SEEK_SET);

		if (psAVIHandle->bIdxUseFile == TRUE)
			psAVIHandle->u32FixedNo --;

		psAVIHandle->sAVICtx.u32arrCounter--;
		return 0;
	}

	if(psAVIHandle->sAVICtx.u32TotalFrames){
		psAVIHandle->u64LastFrameTime = NMUtil_GetTimeMilliSec();
	}
	else{
		psAVIHandle->u64FirstFrameTime = NMUtil_GetTimeMilliSec();
	}

	psAVIHandle->sAVICtx.u32TotalFrames ++;

	if ((psAVIHandle->sAVICtx.u32AudioFmtTag == 0) && (psAVIHandle->bIdxUseFile == TRUE)) {
		if (psAVIHandle->u32FixedNo >= 2048) {

			if ((retval = AVIUtility_WriteTempVOID(psAVIHandle, (uint8_t *)(&psAVIHandle->sAVICtx.psaAVIdxChunkInfo[0]), sizeof(S_AVIFMT_AVIINDEXENTRY) * psAVIHandle->u32FixedNo)) != (sizeof(S_AVIFMT_AVIINDEXENTRY) * psAVIHandle->u32FixedNo)) {
				psAVIHandle->u32TempPos -= retval;
				lseek(psAVIHandle->i32TempFileHandle, psAVIHandle->u32TempPos, SEEK_SET);
				NMLOG_ERROR("Write chunk info fail");
				return 0;
			}

			psAVIHandle->u32FixedNo = 0;
		}
	}

	return u32TotalWriteSize;
}

uint32_t
AVIUtility_WriteMemoryNeed(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	uint32_t u32RecTimeSec
)
{
	uint32_t u32VidChunkEnt;
	uint32_t u32AudChunkEnt = 0;

	if(psAVIHandle->bIdxUseFile == TRUE){
		return ((sizeof(S_AVIFMT_AVIINDEXENTRY) * 4096) + 3); 
	}

	u32VidChunkEnt = psAVIHandle->sAVICtx.u32FrameRate *u32RecTimeSec;

	if(psAVIHandle->sAVICtx.u32AudioFmtTag == ULAW_FMT_TAG){
		u32AudChunkEnt = ((psAVIHandle->sAVICtx.u32SamplingRate)/(psAVIHandle->sAVICtx.u32AudioSamplesPerFrame)) * u32RecTimeSec;
	}
	else if(psAVIHandle->sAVICtx.u32AudioFmtTag == PCM_FMT_TAG){
		u32AudChunkEnt = ((psAVIHandle->sAVICtx.u32SamplingRate)/(psAVIHandle->sAVICtx.u32AudioSamplesPerFrame)) * u32RecTimeSec;
	}
	else if(psAVIHandle->sAVICtx.u32AudioFmtTag == MP3_FMT_TAG){
		u32AudChunkEnt = ((psAVIHandle->sAVICtx.u32SamplingRate)/(psAVIHandle->sAVICtx.u32AudioSamplesPerFrame)) * u32RecTimeSec;
	}
	else if(psAVIHandle->sAVICtx.u32AudioFmtTag == IMA_ADPCM_FMT_TAG){
		u32AudChunkEnt = ((psAVIHandle->sAVICtx.u32SamplingRate)/(psAVIHandle->sAVICtx.u32AudioSamplesPerFrame)) * u32RecTimeSec;
	}
	else{
	}

	return ((sizeof(S_AVIFMT_AVIINDEXENTRY) * (u32VidChunkEnt + u32AudChunkEnt) *2) +3);
}

uint32_t *
AVIUtility_WriteConfigMemory(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	uint32_t	*pu32MemBuf
)
{
	uint32_t *pu32TempBuf = (uint32_t*)( ((uint32_t)pu32MemBuf+3)&~0x3 );
	psAVIHandle->sAVICtx.psaAVIdxChunkInfo = (S_AVIFMT_AVIINDEXENTRY *)pu32TempBuf;
	return pu32TempBuf;
}


void
AVIUtility_FinalizeAVIFile(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	char *szTempFilePath		//Used if psAVIHandle->bIdxUseFile == TRUE
)
{
	uint32_t dwSizeIdx1, dwSizeMOVI, dwMaxFrameSize;
	S_AVIFMT_AVIINDEXENTRY aviIndexEntry;
	uint32_t dwOffset, dwPos, dw;
	uint32_t dwLength;
//	uint32_t dw00db, dw01wb;

	if((psAVIHandle->bIdxUseFile == TRUE) && (szTempFilePath == NULL)){
		NMLOG_ERROR("AVI temp file path is NULL \n");
		return;
	}

//	dw00db = mmioFOURCC('0', '0', 'd', 'b');
//	dw01wb = mmioFOURCC('0', '1', 'w', 'b');

	aviIndexEntry.dwFlags = AVIIF_KEYFRAME;

	if(psAVIHandle->bIdxUseFile == TRUE){
		if (psAVIHandle->u32FixedNo != 0)    //512 bytes full
		{
			AVIUtility_WriteTempVOID(psAVIHandle, (uint8_t *)(&psAVIHandle->sAVICtx.psaAVIdxChunkInfo[0]), psAVIHandle->u32FixedNo * sizeof(S_AVIFMT_AVIINDEXENTRY));
		}
		psAVIHandle->u32TempPos = 0;
		psAVIHandle->u32FixedNo = 0;
		lseek(psAVIHandle->i32TempFileHandle, psAVIHandle->u32TempPos, SEEK_SET);
	}

	dwLength = psAVIHandle->sAVICtx.u32arrCounter * sizeof(S_AVIFMT_AVIINDEXENTRY);
	dwOffset = 4;
	dwPos = 0;
	dwPos += AVIUtility_WriteFourCC(psAVIHandle, (char*)"idx1");
	dwPos += AVIUtility_WriteDWORD(psAVIHandle, dwLength);
	dwMaxFrameSize = 0;

	for (dw = 0; dw < psAVIHandle->sAVICtx.u32arrCounter; dw++)
	{
		if(psAVIHandle->bIdxUseFile == TRUE){
			if (psAVIHandle->u32FixedNo == 0)
			{
				AVIUtility_ReadTempVOID(psAVIHandle, (uint8_t *)(&psAVIHandle->sAVICtx.psaAVIdxChunkInfo[0]), sizeof(S_AVIFMT_AVIINDEXENTRY) * 2048);
			}
			dwLength = psAVIHandle->sAVICtx.psaAVIdxChunkInfo[psAVIHandle->u32FixedNo].dwChunkLength;
			aviIndexEntry.ckid = psAVIHandle->sAVICtx.psaAVIdxChunkInfo[psAVIHandle->u32FixedNo].ckid;
			psAVIHandle->u32FixedNo++;
			if (psAVIHandle->u32FixedNo == 2048)
				psAVIHandle->u32FixedNo = 0;
		}
		else{
			dwLength = psAVIHandle->sAVICtx.psaAVIdxChunkInfo[dw].dwChunkLength;
			aviIndexEntry.ckid = psAVIHandle->sAVICtx.psaAVIdxChunkInfo[dw].ckid;
		}
		aviIndexEntry.dwChunkOffset = dwOffset;
		aviIndexEntry.dwChunkLength = dwLength;
		dwOffset += dwLength + (dwLength & 1) + 8;
		if (dwLength > dwMaxFrameSize)
			dwMaxFrameSize = dwLength;
		dwPos += AVIUtility_WriteVOID(psAVIHandle, &aviIndexEntry, sizeof(S_AVIFMT_AVIINDEXENTRY));
	}

	dwSizeIdx1 = dwPos;
	dwSizeMOVI = dwOffset;

	psAVIHandle->sAVICtx.u32SizeMOVI = dwSizeMOVI;
	psAVIHandle->sAVICtx.u32SizeRIFF = (MOVIE_CHUNK_START_POS - 8) + (dwSizeMOVI + 8) + dwSizeIdx1;
	psAVIHandle->sAVICtx.u32MaxBytesPerSec = (dwMaxFrameSize + 512) * psAVIHandle->sAVICtx.u32FrameRate;

	dw = psAVIHandle->u32CurrentPos;
	AVIUtility_GenMainAviHeader(psAVIHandle);
	AVIUtility_GenVideoStreamHeaderChunk(psAVIHandle);

	if(psAVIHandle->sAVICtx.u32AudioFmtTag != 0)
		AVIUtility_GenAudioStreamHeaderChunk(psAVIHandle);

	AVIUtility_GenListMoviChunkHeader(psAVIHandle);
	
//	ftruncate(psAVIHandle->i32FileHandle,dw);
	lseek(psAVIHandle->i32FileHandle, dw, SEEK_SET);

	if(psAVIHandle->bIdxUseFile == TRUE){
		char *szTempFileName = malloc(strlen(szTempFilePath) + 14);
//		char szUnicodeFileName[512];

		close(psAVIHandle->i32TempFileHandle);

		if(szTempFileName == NULL){
			NMLOG_ERROR("Alloc temp file path buffer failed \n");
			return;
		}

		sprintf(szTempFileName, "%s\\%x.tmp", szTempFilePath, psAVIHandle->i32FileHandle);
//		fsAsciiToUnicode(szTempFileName, szUnicodeFileName, TRUE); 
//		fsDeleteFile(szUnicodeFileName, NULL);
		unlink(szTempFileName);
	}	
}


int32_t
AVIUtility_ParseIdx1Chunk(
	S_AVIREAD_MEDIA_ATTR *psMediaAttr,
	S_AVIFMT_AVI_CONTENT	*psAVIFileContent
)
{
	const uint32_t 		u32AVIEntryNum = 32;
	int32_t				i32FileNo = psMediaAttr->i32FileHandle;
	uint32_t			u32Pos, u32Total, u32SizeIDX1, u32FourCC;
	long				nOffset;
	S_AVIFMT_AVIINDEXENTRY	*aviIndexEntry;
	S_AVIFMT_AVIINDEXENTRY	asAviEntryBuff[u32AVIEntryNum];

	uint32_t			u32MaxJPEGSize;
	uint32_t			u32SizeEntry;

	//unsigned int u32Entry;
	int32_t			i32Size;
	int i32ErrNo = ERR_AVIUTIL_SUCCESS;

	//----------------------------------------------------------
	//	RIFF(u32SizeRIFF)AVI
	//		LIST(u32SizeHDRL)hdrl
	//			avih(0038)<MainAVIHeader>
	//			LIST(008E)strl
	//				strh(0038) <StreamHeader>
	//				strf(0028) <BITMAPINFOHEADER>
	//				strn(0012) <streamName>
	//			LIST....for audio (optional)
	//		JUNK(sizeJunk) <junk>
	//		LIST(u32SizeMOVI)movi
	//			00db(sizeJPG_0) <JPGData_0>
	//			00db(sizeJPG_1) <JPGData_1>
	//					...
	//			00db(sizeJPG_n) <JPGData_n>
	//---------------------------------------<<< parse_begin )))
	//		idx1(sizeIDX1) <idx>
	//---------------------------------------<<< parse_end )))

	for (u32Total = 0; u32Total < u32AVIEntryNum; u32Total++) {
		asAviEntryBuff[u32Total].ckid = 0;
		asAviEntryBuff[u32Total].dwChunkLength = 0;
	}

	{
		if (psAVIFileContent->u32SizeJUNK > 0)
			u32Pos = 12 + (psAVIFileContent->u32SizeHDRL + 8) + (psAVIFileContent->u32SizeJUNK + 8) +
					 (psAVIFileContent->u32SizeMOVI + 8) + psAVIFileContent->u32LISTChunkSize;
		else
			u32Pos = 12 + (psAVIFileContent->u32SizeHDRL + 8) + (psAVIFileContent->u32SizeMOVI + 8) +
					 psAVIFileContent->u32LISTChunkSize;

		//		u32Total = s_AVIFileContent.u32SizeRIFF + 8;
		u32Total = psMediaAttr->u32MediaSize;
		nOffset = (long)(u32Total - u32Pos);
		psAVIFileContent->u32AVICurrentPos = psMediaAttr->u32MediaSize - nOffset;
	}

	u32SizeIDX1 = 0;

	//printf("AVIRead_AVI_ParseIdx1Chunk u32Total %d\n",u32Total);
	if (AVIUtility_ReadDWORD(i32FileNo, &u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_IDX1_HEADER;
		goto End_ParseIdx1;
	}

	//printf("AVIRead_AVI_ParseIdx1Chunk u32FourCC %x\n",u32FourCC);

	if (AVIUtility_ReadDWORD(i32FileNo, &u32SizeIDX1, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_IDX1_HEADER;
		goto End_ParseIdx1;
	}

	//printf("AVIRead_AVI_ParseIdx1Chunk u32SizeIDX1 %d\n",u32SizeIDX1);

	if (u32FourCC != 0x31786469) {	//'i','d','x','1'
		i32ErrNo = ERR_AVIUTIL_IDX1_HEADER;
		goto End_ParseIdx1;
	}

	//printf("AVIRead_AVI_ParseIdx1Chunk u32FourCC %x\n",u32FourCC);

	u32MaxJPEGSize = 0;
	u32SizeEntry = sizeof(S_AVIFMT_AVIINDEXENTRY);
	psAVIFileContent->u32arrCounter = u32SizeIDX1 / u32SizeEntry;//number of entry
	psAVIFileContent->u32posIndexEntry = psAVIFileContent->u32AVICurrentPos;

	aviIndexEntry = (S_AVIFMT_AVIINDEXENTRY *)psAVIFileContent->u32posIndexEntry;

	//printf("AVIRead_AVI_ParseIdx1Chunk psAVIFileContent->u32arrCounter %d\n",psAVIFileContent->u32arrCounter);

	if (lseek(i32FileNo, (unsigned int)aviIndexEntry, SEEK_SET) < 0) {
		i32ErrNo = ERR_AVIUTIL_IDX1_HEADER;
		goto End_ParseIdx1;
	}

	i32Size = read(i32FileNo, (unsigned char *)asAviEntryBuff, sizeof(S_AVIFMT_AVIINDEXENTRY));

	if (i32Size != sizeof(S_AVIFMT_AVIINDEXENTRY)) {
		i32ErrNo = ERR_AVIUTIL_IDX1_HEADER;
		goto End_ParseIdx1;
	}

	//printf("AVIRead_AVI_ParseIdx1Chunk asAviEntryBuff[0].ckid %x\n",asAviEntryBuff[0].ckid);

	while (asAviEntryBuff[0].ckid != 0x62643030 &&
			asAviEntryBuff[0].ckid != 0x63643030 &&
			asAviEntryBuff[0].ckid != 0x62773130 &&
			asAviEntryBuff[0].ckid != 0x62643130 &&
			asAviEntryBuff[0].ckid != 0x63643130 &&
			asAviEntryBuff[0].ckid != 0x62773030 &&
			psAVIFileContent->u32arrCounter > 0) {
		aviIndexEntry += 1;
		i32Size = read(i32FileNo, (unsigned char *)asAviEntryBuff, sizeof(S_AVIFMT_AVIINDEXENTRY));

		if (i32Size != sizeof(S_AVIFMT_AVIINDEXENTRY)) {
			i32ErrNo = ERR_AVIUTIL_IDX1_HEADER;
			goto End_ParseIdx1;
		}

		//printf("AVIRead_AVI_ParseIdx1Chunk asAviEntryBuff[0].ckid %x\n",asAviEntryBuff[0].ckid);

		psAVIFileContent->u32arrCounter--;
	}

	psAVIFileContent->u32posIndexEntry = (unsigned int)aviIndexEntry;
	i32Size = read(i32FileNo, (unsigned char *)&asAviEntryBuff[1], (sizeof(S_AVIFMT_AVIINDEXENTRY) * (u32AVIEntryNum - 1)));

	if (i32Size < 0) {
		i32ErrNo = ERR_AVIUTIL_IDX1_HEADER;
	}

	psAVIFileContent->u32SuggestedBufferSize = u32MaxJPEGSize;

End_ParseIdx1:

	if (i32ErrNo != ERR_AVIUTIL_SUCCESS)
		printf("ParseIdx1Header ERROR \n");

	return i32ErrNo;
}

int
AVIUtility_ParseListMoviChunkHeader(
	S_AVIREAD_MEDIA_ATTR *psMediaAttr,
	S_AVIFMT_AVI_CONTENT	*psAVIFileContent
)
{
	int				i32FileNo = psMediaAttr->i32FileHandle;
	unsigned int	u32SizeMOVI;
	unsigned int	u32FourCC;
	unsigned int	u32FourCC2;
	int i32ErrNo = ERR_AVIUTIL_SUCCESS;

	//----------------------------------------------------------
	//	RIFF(u32SizeRIFF)AVI
	//		LIST(u32SizeHDRL)hdrl
	//			avih(0038)<MainAVIHeader>
	//			LIST(008E)strl
	//				strh(0038) <StreamHeader>
	//				strf(0028) <BITMAPINFOHEADER>
	//				strn(0012) <streamName>
	//			LIST....for audio (optional)
	//		JUNK(sizeJunk) <junk>
	//---------------------------------------<<< parse_begin )))
	//		LIST(u32SizeMOVI)movi
	//---------------------------------------<<< parse_end )))
	//			00db(sizeJPG_0) <JPGData_0>
	//			00db(sizeJPG_1) <JPGData_1>
	//					...
	//			00db(sizeJPG_n) <JPGData_n>
	//		idx1(sizeIDX1) <idx>
	//----------------------------------------------------------

	u32SizeMOVI = 0;

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_LIST_MOVI_CHUNK;
		goto End_ParseListMovi;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32SizeMOVI, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_LIST_MOVI_CHUNK;
		goto End_ParseListMovi;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC2, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_LIST_MOVI_CHUNK;
		goto End_ParseListMovi;
	}

	if (u32FourCC != 0x5453494c) {	//'L','I','S','T'
		i32ErrNo = ERR_AVIUTIL_LIST_MOVI_CHUNK;
		goto End_ParseListMovi;
	}

	if (u32FourCC2 != 0x69766f6d) {	//'m','o','v','i'
		i32ErrNo = ERR_AVIUTIL_LIST_MOVI_CHUNK;
		goto End_ParseListMovi;
	}

	psAVIFileContent->u32SizeMOVI = u32SizeMOVI;
	psAVIFileContent->u32PosMOVI = psAVIFileContent->u32AVICurrentPos;

End_ParseListMovi:
	if (i32ErrNo != ERR_AVIUTIL_SUCCESS)
		printf("ParseListMoviHeader ERROR \n");

	return i32ErrNo;
}

int
AVIUtility_ParseAudioStreamHeaderChunk(
	S_AVIREAD_MEDIA_ATTR *psMediaAttr,
	S_AVIFMT_AVI_CONTENT	*psAVIFileContent
)
{
	int							i32FileNo = psMediaAttr->i32FileHandle;
	S_AVIFMT_AVISTREAMHEADER 	aStreamHeader;
	S_AVIFMT_WAVEFORMATEX		*psAudioInfoHeader = NULL;
	unsigned int				u32FourCC;
	unsigned int				u32FourCC2;
	unsigned int				u32SizeSTRH;
	unsigned int				u32SizeSTRF;
	unsigned int				u32SizeSTRL;
	unsigned int				u32orgpos;
	int i32ErrNo = ERR_AVIUTIL_SUCCESS;

	if (lseek(i32FileNo, psAVIFileContent->u32AVICurrentPos, SEEK_SET) < 0) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	u32orgpos = psAVIFileContent->u32AVICurrentPos;

	//----------------------------------------------------------
	//	RIFF(u32SizeRIFF)AVI
	//		LIST(u32SizeHDRL)hdrl
	//			avih(0038)<MainAVIHeader>
	//			LIST(008E)strl
	//				strh(0038) <StreamHeader>
	//				strf(0028) <BITMAPINFOHEADER>
	//				strn(0012) <streamName>
	//---------------------------------------<<< parse_begin )))
	//			LIST(xxxx)strl
	//				strh(0038) <StreamHeader>
	//				strf(0014) <WAVEFORMATEX>
	//				Dummy
	//---------------------------------------<<< parse_end )))
	//
	//		JUNK(sizeJunk) <junk>
	//		LIST(u32SizeMOVI)movi
	//			00db(sizeJPG_0) <JPGData_0>
	//			00db(sizeJPG_1) <JPGData_1>
	//					...
	//			00db(sizeJPG_n) <JPGData_n>
	//		idx1(sizeIDX1) <idx>
	//----------------------------------------------------------
	//----------------------------------------------------------

	//<1> LIST(u32SizeSTRL)strl

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	if (u32FourCC != 0x5453494c) {	//'L','I','S','T'
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32SizeSTRL, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC2, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	if (u32FourCC2 != 0x6c727473) {	//'s','t','r','l'
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	//----------------------------------------------------------

	//<2>	strh(0038)<StreamHeader>

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32SizeSTRH, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	if (u32FourCC != 0x68727473) {	//'s','t','r','h'
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	if (sizeof(S_AVIFMT_AVISTREAMHEADER) != u32SizeSTRH) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	if (AVIUtility_ReadVOID(i32FileNo, (void *)&aStreamHeader, u32SizeSTRH, psAVIFileContent) != u32SizeSTRH) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	// checking AVIStreamHeader

	if (aStreamHeader.fccType != 0x73647561 /*mmioFOURCC('a','u','d','s')*/) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	psMediaAttr->sAudioAttr.u32MaxChunkSize = aStreamHeader.u32SuggestedBufferSize;

	//----------------------------------------------------------

	//<3>	strf(0014)<WAVEFORMATEX>

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32SizeSTRF, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	if (u32FourCC != 0x66727473) {	//'s','t','r','f'
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	psAudioInfoHeader = malloc(u32SizeSTRF);

	if (AVIUtility_ReadVOID(i32FileNo, (void *)psAudioInfoHeader, u32SizeSTRF, psAVIFileContent) != u32SizeSTRF) {
		i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
		goto End_ParseAudioStreamHeaderChunk;
	}

	psAVIFileContent->u32SamplingRate = psAudioInfoHeader->u32SamplesPerSec;
	psAVIFileContent->u16BitsPerSample = psAudioInfoHeader->u16BitsPerSample;
	psMediaAttr->sAudioAttr.u32AvgBytesPerSec = psAudioInfoHeader->u32AvgBytesPerSec;
	psMediaAttr->sAudioAttr.u32AudioScale = aStreamHeader.u32Scale;
	psMediaAttr->sAudioAttr.u32AudioRate = aStreamHeader.u32Rate;
	psMediaAttr->sAudioAttr.u32SampleRate = psAudioInfoHeader->u32SamplesPerSec;

	// IMAADPCM format rule

	/*	if (sAudioInfoHeader.u16Channels == 2)
		{
			s_i32AviAudioSamplePerBlock = sAudioInfoHeader.u16BlockAlign - 8 + 1;
			s_AVIFileContent.u32Channel = 2;
		}
		else if (sAudioInfoHeader.u16Channels == 1)
		{
			s_i32AviAudioSamplePerBlock = (sAudioInfoHeader.u16BlockAlign - 4) * 2 + 1;
			s_AVIFileContent.u32Channel = 1;
		}*/

	psMediaAttr->sAudioAttr.u32ChannelNum = psAudioInfoHeader->u16Channels;

	//	g_AVIUtility_i32AviAudioBlockSize = sAudioInfoHeader.u16BlockAlign;

	if (psAudioInfoHeader->u16FormatTag == ULAW_FMT_TAG) {
		psMediaAttr->sAudioAttr.u8FmtTag = ULAW_FMT_TAG;//eAVIFMT_AUDIO_ULAW;
		psMediaAttr->sAudioAttr.u32AvgBytesPerSec = psAudioInfoHeader->u32SamplesPerSec * psAudioInfoHeader->u16Channels;
		//		if((psAudioInfoHeader->u16Channels == 1) && psAudioInfoHeader->u16BlockAlign==(psAudioInfoHeader->u32SamplesPerSec>>1))
		//			psMediaInfo->sAudioInfo.u32SmplNumPerChunk = psAudioInfoHeader->u16BlockAlign*8/psAudioInfoHeader->u16BitsPerSample;
		//		else if((psAudioInfoHeader->u16Channels == 2) && psAudioInfoHeader->u16BlockAlign==psAudioInfoHeader->u32SamplesPerSec)
		//			psMediaInfo->sAudioInfo.u32SmplNumPerChunk = (psAudioInfoHeader->u16BlockAlign*8/psAudioInfoHeader->u16BitsPerSample)>>1;
		//printf("u32SmplNumPerChunk %d u16BlockAlign % u16BitsPerSample %d \n",psMediaInfo->sAudioInfo.u32SmplNumPerChunk,sAudioInfoHeader.u16BlockAlign,sAudioInfoHeader.u16BitsPerSample);
	}
	else if (psAudioInfoHeader->u16FormatTag == ALAW_FMT_TAG) {
		psMediaAttr->sAudioAttr.u8FmtTag = ALAW_FMT_TAG;
		psMediaAttr->sAudioAttr.u32AvgBytesPerSec = psAudioInfoHeader->u32SamplesPerSec * psAudioInfoHeader->u16Channels;
		//		if((psAudioInfoHeader->u16Channels == 1) && aStreamHeader.u32SuggestedBufferSize==psAudioInfoHeader->u32SamplesPerSec)
		//			psMediaInfo->sAudioInfo.u32SmplNumPerChunk = aStreamHeader.u32SuggestedBufferSize*8/psAudioInfoHeader->u16BitsPerSample;
		//		else if((psAudioInfoHeader->u16Channels == 2) && aStreamHeader.u32SuggestedBufferSize==(psAudioInfoHeader->u32SamplesPerSec*2))
		//			psMediaInfo->sAudioInfo.u32SmplNumPerChunk = (aStreamHeader.u32SuggestedBufferSize*8/psAudioInfoHeader->u16BitsPerSample)>>1;
	}
	else if (psAudioInfoHeader->u16FormatTag == PCM_FMT_TAG) {
		psMediaAttr->sAudioAttr.u8FmtTag = PCM_FMT_TAG;//eNM_AUDIO_PCM, every chunksize diff size;
		psMediaAttr->sAudioAttr.u32AvgBytesPerSec = psAudioInfoHeader->u32SamplesPerSec * psAudioInfoHeader->u16Channels * 2;
		//		if((psAudioInfoHeader->u16Channels == 1) && aStreamHeader.u32SuggestedBufferSize==psAudioInfoHeader->u32SamplesPerSec)
		//			psMediaInfo->sAudioInfo.u32SmplNumPerChunk = aStreamHeader.u32SuggestedBufferSize*8/psAudioInfoHeader->u16BitsPerSample;
		//		else if((psAudioInfoHeader->u16Channels == 2) && aStreamHeader.u32SuggestedBufferSize==(psAudioInfoHeader->u32SamplesPerSec*2))
		//			psMediaInfo->sAudioInfo.u32SmplNumPerChunk = (aStreamHeader.u32SuggestedBufferSize*8/psAudioInfoHeader->u16BitsPerSample)>>1;
	}
	else if (psAudioInfoHeader->u16FormatTag == ADPCM_FMT_TAG) {
		psMediaAttr->sAudioAttr.u8FmtTag = ADPCM_FMT_TAG;//eNM_AUDIO_ADPCM;

		if (psAVIFileContent->u16BitsPerSample != 4) {
			printf("AVIRead_AVI_ParseAudioStreamHeaderChunk u16BitsPerSample %d error\n", psAVIFileContent->u16BitsPerSample);
			i32ErrNo = ERR_AVIUTIL_AUDIO_STREAM_HEADER;
			goto End_ParseAudioStreamHeaderChunk;
		}

		if(psAudioInfoHeader->u16Channels == 2)
			psMediaAttr->sAudioAttr.u32SmplNumPerChunk = (psAudioInfoHeader->u16BlockAlign - 8 + 1);
		else if (psAudioInfoHeader->u16Channels == 1)
			psMediaAttr->sAudioAttr.u32SmplNumPerChunk = (psAudioInfoHeader->u16BlockAlign - 4) * 2 + 1;

		uint32_t u32AvgBytesPerSec = (psAudioInfoHeader->u32SamplesPerSec * psMediaAttr->sAudioAttr.u32MaxChunkSize) / psMediaAttr->sAudioAttr.u32SmplNumPerChunk;

		if((psMediaAttr->sAudioAttr.u32AvgBytesPerSec > u32AvgBytesPerSec +10) || (psMediaAttr->sAudioAttr.u32AvgBytesPerSec +10 < u32AvgBytesPerSec)){
			printf("AvgBytesPerSec need around %d , but it is %d error\n", u32AvgBytesPerSec, psMediaAttr->sAudioAttr.u32AvgBytesPerSec);
			psMediaAttr->sAudioAttr.u32AvgBytesPerSec = u32AvgBytesPerSec;
			u32AvgBytesPerSec = psAudioInfoHeader->u32SamplesPerSec * psAudioInfoHeader->u16Channels / 2;
			if((psMediaAttr->sAudioAttr.u32AvgBytesPerSec > u32AvgBytesPerSec +1000) || (psMediaAttr->sAudioAttr.u32AvgBytesPerSec +1000 < u32AvgBytesPerSec)){
				printf("AvgBytesPerSec need around %d , but it is %d error\n", u32AvgBytesPerSec, psMediaAttr->sAudioAttr.u32AvgBytesPerSec);
				psMediaAttr->sAudioAttr.u32AvgBytesPerSec =0;
			}
		}

		printf("AvgBytesPerSec is %d\n", psMediaAttr->sAudioAttr.u32AvgBytesPerSec);
	}
	else if (psAudioInfoHeader->u16FormatTag == MP3_FMT_TAG) {
		psMediaAttr->sAudioAttr.u8FmtTag = MP3_FMT_TAG;//eNM_AUDIO_MP3;
		//		if(sAudioInfoHeader.u16Channels == 2)
		//			psMediaInfo->sAudioInfo.u32SmplNumPerChunk = (sAudioInfoHeader.u16BlockAlign - 8 + 1);
		//		else if (sAudioInfoHeader.u16Channels == 1)
		//			psMediaInfo->sAudioInfo.u32SmplNumPerChunk = (sAudioInfoHeader.u16BlockAlign - 4) * 2 + 1;
	}
	else if (psAudioInfoHeader->u16FormatTag == AAC_FMT_TAG) {
		S_AVIFMT_AACWAVEFORMAT *psAACInfoHeader = (S_AVIFMT_AACWAVEFORMAT *)psAudioInfoHeader;
		psMediaAttr->sAudioAttr.u8FmtTag = AAC_FMT_TAG;//eNM_AUDIO_AAC;
		psMediaAttr->sAudioAttr.u32Profile = (psAACInfoHeader->u8Profile >> 3) & 0xff;
		psMediaAttr->sAudioAttr.u32SmplNumPerChunk = 1024; // AAC block encode. Each block is 1024 samples
	}
	else if (psAudioInfoHeader->u16FormatTag == G726_FMT_TAG) {
		psMediaAttr->sAudioAttr.u8FmtTag = G726_FMT_TAG;//eNM_AUDIO_G726;
	}

	psMediaAttr->sAudioAttr.u32BitRate = psMediaAttr->sAudioAttr.u32AvgBytesPerSec * 8;
	printf(" Audio BitRate %d \n", psMediaAttr->sAudioAttr.u32BitRate);

End_ParseAudioStreamHeaderChunk:

	//----------------------------------------------------------
	//<4>	continue to read off the dummy data

	if (i32ErrNo != ERR_AVIUTIL_SUCCESS)
		printf("ParseAudioStreamHeader ERROR \n");

	free(psAudioInfoHeader);
	return i32ErrNo;
}

unsigned int
AVIUtility_ParseJunkChunk(
	S_AVIREAD_MEDIA_ATTR *psMediaAttr,
	S_AVIFMT_AVI_CONTENT	*psAVIFileContent
)
{
	int				i32FileNo = psMediaAttr->i32FileHandle;
	unsigned int	u32FourCC;
	unsigned int	u32SizeJUNK, u32SizeLIST, u32TmpSizeJUNK = 0;
	unsigned int	u32SizeHDRL;
	unsigned int	u32Pos;
	int i32ErrNo = ERR_AVIUTIL_SUCCESS;

	//----------------------------------------------------------
	//	RIFF(u32SizeRIFF)AVI
	//		LIST(u32SizeHDRL)hdrl
	//			avih(0038)<MainAVIHeader>
	//			LIST(008E)strl
	//				strh(0038) <StreamHeader>
	//				strf(0028) <BITMAPINFOHEADER>
	//				strn(0012) <streamName>
	//			LIST....for audio (optional)
	//
	//---------------------------------------<<< parse_begin )))
	//		JUNK(sizeJunk) <junk>
	//---------------------------------------<<< parse_end )))
	//		LIST(u32SizeMOVI)movi
	//			00db(sizeJPG_0) <JPGData_0>
	//			00db(sizeJPG_1) <JPGData_1>
	//					...
	//			00db(sizeJPG_n) <JPGData_n>
	//		idx1(sizeIDX1) <idx>
	//----------------------------------------------------------

	u32SizeHDRL = psAVIFileContent->u32SizeHDRL;
	u32Pos = 12 + 8 + u32SizeHDRL;

LIST:
	psAVIFileContent->u32AVICurrentPos = u32Pos;

	if (lseek(i32FileNo, psAVIFileContent->u32AVICurrentPos, SEEK_SET) < 0) {
		i32ErrNo = ERR_AVIUTIL_JUNK_HEADER;
		goto End_ParseJunk;
	}

	u32SizeLIST = 0;

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_JUNK_HEADER;
		goto End_ParseJunk;
	}

	if (u32FourCC == 0x5453494c) { // 'L', 'I', 'S', 'T' for ffmpeg encoder avi
		if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32SizeLIST, psAVIFileContent) < 4) {
			i32ErrNo = ERR_AVIUTIL_JUNK_HEADER;
			goto End_ParseJunk;
		}

		psAVIFileContent->u32LISTChunkSize = u32SizeLIST + 8;
		u32Pos += psAVIFileContent->u32LISTChunkSize;
		goto LIST;
	}

	u32SizeJUNK = 0;

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32SizeJUNK, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_JUNK_HEADER;
		goto End_ParseJunk;
	}

	if (u32FourCC == 0x4b4e554a /* 'J', 'U', 'N', 'K'*/ && u32SizeJUNK != 0)
		u32Pos += u32SizeJUNK + 8;
	// do nothing but move the FilePointer
	psAVIFileContent->u32AVICurrentPos = u32Pos;

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_JUNK_HEADER;
		goto End_ParseJunk;
	}

	if (u32FourCC == 0x4b4e554a /* 'J', 'U', 'N', 'K'*/) {
		u32TmpSizeJUNK += (u32SizeJUNK + 8);
		goto LIST;
	}
	else {
		psAVIFileContent->u32AVICurrentPos -= 4;
		u32TmpSizeJUNK += u32SizeJUNK;
	}

	psAVIFileContent->u32SizeJUNK = u32TmpSizeJUNK;

End_ParseJunk:

	if (i32ErrNo != ERR_AVIUTIL_SUCCESS)
		printf("ParseJunkHeader ERROR \n");
	return i32ErrNo;
}

unsigned int
AVIUtility_ParseVideoStreamHeaderChunk(
	S_AVIREAD_MEDIA_ATTR *psMediaAttr,
	S_AVIFMT_AVI_CONTENT	*psAVIFileContent
)
{
	int							i32FileNo = psMediaAttr->i32FileHandle;
	S_AVIFMT_AVISTREAMHEADER	aStreamHeader;
	S_AVIFMT_BITMAPINFOHEADER	bmpInfoHeader;
	unsigned int				u32FourCC;
	unsigned int				u32FourCC2;
	unsigned int				u32Size;
	unsigned int				u32SizeSTRL;
	unsigned int				u32TmpPos = psAVIFileContent->u32AVICurrentPos;
	int i32ErrNo = ERR_AVIUTIL_SUCCESS;
	double dFrameRate = 0;

	if (lseek(i32FileNo, psAVIFileContent->u32AVICurrentPos, SEEK_SET) < 0) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	//----------------------------------------------------------
	//	RIFF(u32SizeRIFF)AVI
	//		LIST(u32SizeHDRL)hdrl
	//			avih(0038)<MainAVIHeader>
	//---------------------------------------<<< parse_begin )))
	//			LIST(008E)strl
	//				strh(0038) <StreamHeader>
	//				strf(0028) <BITMAPINFOHEADER>
	//				strn(0012) <streamName>
	//---------------------------------------<<< parse_end )))
	//			LIST....for audio (optional)
	//
	//		JUNK(sizeJunk) <junk>
	//		LIST(u32SizeMOVI)movi
	//			00db(sizeJPG_0) <JPGData_0>
	//			00db(sizeJPG_1) <JPGData_1>
	//					...
	//			00db(sizeJPG_n) <JPGData_n>
	//		idx1(sizeIDX1) <idx>
	//----------------------------------------------------------
	//----------------------------------------------------------

	//<1> LIST(u32SizeSTRL)strl

	if ((AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4)) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	if ((AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32SizeSTRL, psAVIFileContent) < 4)) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	if ((AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC2, psAVIFileContent) < 4)) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	//'s','t','r','l'

	if ((u32FourCC != 0x5453494c) || (u32FourCC2 != 0x6c727473)) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	//----------------------------------------------------------

	//<2>	strh(0038)<StreamHeader>

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32Size, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	//'s','t','r','h'

	if ((u32FourCC != 0x68727473) || (sizeof(S_AVIFMT_AVISTREAMHEADER) != u32Size)) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	if (AVIUtility_ReadVOID(i32FileNo, (void *)&aStreamHeader, u32Size, psAVIFileContent) < u32Size) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	// checking AVIStreamHeader
	if (aStreamHeader.fccType != 0x73646976 /*mmioFOURCC('v','i','d','s')*/) {
		//printf("aStreamHeader.fccType %x\n",aStreamHeader.fccType);
		psAVIFileContent->u32AVICurrentPos = u32TmpPos;
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	// checking AVIStreamHeader, wivi or WiVi or MJPG or mjpg
	//	if (
	//			aStreamHeader.fccHandler != 0x47504a4d &&		//MJPG
	//	        aStreamHeader.fccHandler != 0x67706a6d  //&&	//mjpg
	//	        aStreamHeader.fccHandler != 0x69766977  &&	//wivi
	//	        aStreamHeader.fccHandler != 0x69566957 /*mmioFOURCC('W','i','V','i')*/
	//	)
	//	{
	//		return eNM_ERROR_TYPE;
	//	}

	//printf("aStreamHeader.fccHandler %x\n",aStreamHeader.fccHandler);
	psMediaAttr->sVideoAttr.u32MaxChunkSize = aStreamHeader.u32SuggestedBufferSize;


	if (aStreamHeader.u32Scale){
		dFrameRate = (double)aStreamHeader.u32Rate / (double)aStreamHeader.u32Scale;
		psMediaAttr->sVideoAttr.u32FrameRate = (uint32_t)dFrameRate;
	}

	if (dFrameRate != 0){
		double dTemp = (double) aStreamHeader.u32Length / dFrameRate * 1000;
		psMediaAttr->u32Duration =(uint32_t) dTemp;
	}

	psMediaAttr->sVideoAttr.u32VideoScale = aStreamHeader.u32Scale;
	psMediaAttr->sVideoAttr.u32VideoRate = aStreamHeader.u32Rate;
	psAVIFileContent->u32FrameRate = psMediaAttr->sVideoAttr.u32FrameRate;

	//printf("psMediaInfo->sCommonInfo.u32Duration %d\n",psMediaInfo->sCommonInfo.u32Duration);

	//----------------------------------------------------------
	//<3>	strf(0028)<BITMAPINFOHEADER>
	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32Size, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	//'s','t','r','f'

	if ((u32FourCC != 0x66727473)) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	if (AVIUtility_ReadVOID(i32FileNo, (void *)&bmpInfoHeader, u32Size, psAVIFileContent) < u32Size) {
		i32ErrNo = ERR_AVIUTIL_VIDEO_STREAM_HEADER;
		goto End_ParseVideoStream;
	}

	// checking BITMAPINFOHEADER
	//	if (
	//		bmpInfoHeader.biCompression != 0x47504a4d  //&&	//MJPG
	//		bmpInfoHeader.biCompression != 0x69566957 /*mmioFOURCC('W','i','V','i')*/
	//	)
	//	{
	//		return eNM_ERROR_VIDEO_STREAM_HEADER;
	//	}

	//MJPG

	if (bmpInfoHeader.biCompression == 0x47504a4d)  //mmioFOURCC('M','J','P','G')
		psMediaAttr->sVideoAttr.u32FourCC = MJPEG_FOURCC;
	else if (bmpInfoHeader.biCompression == 0x34363248)	//mmioFOURCC('H','2','6','4')
		psMediaAttr->sVideoAttr.u32FourCC = H264_FOURCC;

	//printf("psMediaInfo->sVideoInfo.eVideoType %d\n",psMediaInfo->sVideoInfo.eVideoType);
	//----------------------------------------------------------

	//<4>	continue to read off the dummy data

	u32Size = u32SizeSTRL - (4 + 0x0038 + 8 + u32Size + 8);
	psAVIFileContent->u32AVICurrentPos += u32Size;

End_ParseVideoStream:

	if (i32ErrNo != ERR_AVIUTIL_SUCCESS)
		printf("ParseVideoStreamHeader ERROR \n");

	return i32ErrNo;
}

//=================================================================================================

// Implementation of the private functions ( AviExtractor, layer II )
int
AVIUtility_ParseMainAviHeader(
	S_AVIREAD_MEDIA_ATTR *psMediaAttr,
	S_AVIFMT_AVI_CONTENT	*psAVIFileContent
)
{
	int						i32FileNo = psMediaAttr->i32FileHandle;
	unsigned int			u32FourCC;
	unsigned int			u32FourCC2;
	unsigned int			u32Size;
	unsigned int			u32SizeRIFF;
	unsigned int			u32SizeHDRL;
	S_AVIFMT_MAINAVIHEADER	aMainAviHeader;
	int i32ErrNo = ERR_AVIUTIL_SUCCESS;

	//---------------------------------------<<< parse_begin )))
	//	RIFF(u32SizeRIFF)AVI
	//		LIST(u32SizeHDRL)hdrl
	//			avih(0038)<MainAVIHeader>
	//---------------------------------------<<< parse_end )))
	//			LIST(008E)strl
	//				strh(0038) <StreamHeader>
	//				strf(0028) <BITMAPINFOHEADER>
	//				strn(0012) <streamName>
	//		JUNK(sizeJunk) <junk>
	//		LIST(u32SizeMOVI)movi
	//			00db(sizeJPG_0) <JPGData_0>
	//			00db(sizeJPG_1) <JPGData_1>
	//					...
	//			00db(sizeJPG_n) <JPGData_n>
	//		idx1(sizeIDX1) <idx>
	//----------------------------------------------------------

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32SizeRIFF, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC2, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (u32FourCC != 0x46464952) {	//'R','I','F','F' ?
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (u32FourCC2 != 0x20495641) {	//'A','V','I',' '
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	//<2>	LIST(u32SizeHDRL)hdrl
	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32SizeHDRL, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC2, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (u32FourCC != 0x5453494c) {	//'L','I','S','T'
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (u32FourCC2 != 0x6c726468) {	//'h','d','r','l'
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	//<3>		avih(0038)<MainAVIHeader>
	//memset(&aMainAviHeader,0,sizeof(aMainAviHeader));

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32FourCC, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (AVIUtility_ReadDWORD(i32FileNo, (uint32_t *)&u32Size, psAVIFileContent) < 4) {
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (u32FourCC != 0x68697661) {	//'a','v','i','h'
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (sizeof(aMainAviHeader) != u32Size) {
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	if (AVIUtility_ReadVOID(i32FileNo, (void *)&aMainAviHeader, u32Size, psAVIFileContent) < u32Size) {
		i32ErrNo = ERR_AVIUTIL_MAIN_AVI_HEADER;
		goto End_ParseMainAvi;
	}

	//printf("aMainAviHeader.u32MicroSecPerFrame %d\n",aMainAviHeader.u32MicroSecPerFrame);
	// other fields
	if (aMainAviHeader.u32TotalFrames && aMainAviHeader.u32MicroSecPerFrame != 0) {
		psMediaAttr->sVideoAttr.u32BitRate = aMainAviHeader.u32MaxBytesPerSec * 8;
		psAVIFileContent->i32Width = (int)aMainAviHeader.u32Width;
		psMediaAttr->sVideoAttr.u32Width = aMainAviHeader.u32Width;
		psAVIFileContent->i32Height = (int)aMainAviHeader.u32Height;
		psMediaAttr->sVideoAttr.u32Height = aMainAviHeader.u32Height;
	}

	psAVIFileContent->u32MaxBytesPerSec = aMainAviHeader.u32MaxBytesPerSec;
	psAVIFileContent->u32TotalFrames = aMainAviHeader.u32TotalFrames;
	psMediaAttr->sVideoAttr.u32ChunkNum = aMainAviHeader.u32TotalFrames;
	psAVIFileContent->u32SuggestedBufferSize = aMainAviHeader.u32SuggestedBufferSize;
	psAVIFileContent->u32SizeRIFF = u32SizeRIFF;
	psAVIFileContent->u32SizeHDRL = u32SizeHDRL;
	psAVIFileContent->u32Streams  = (int)aMainAviHeader.u32Streams;

	//	if (s_AVIFileContent.u32FrameRate > 12)
	//		return 0;
	// return the offset value of File Pointer
	//
	//	(12)		RIFF(u32SizeRIFF)AVI
	//	(12)			LIST(u32SizeHDRL)hdrl
	//	(8+0x38)			avih(0038)<MainAVIHeader>
	//
	//u32Pos = 12 + 12 + 8 + sizeof(S_AVICodec_MainAVIHeader);
	//return u32Pos;

End_ParseMainAvi:

	if (i32ErrNo != ERR_AVIUTIL_SUCCESS)
		printf("ParseMainAviHeader ERROR \n");
	return i32ErrNo;
}

//-------------------Calling 2----------------------------

//Parse AVI file

ERRCODE
AVIUtility_AVIParse(
	S_AVIREAD_MEDIA_ATTR *psMediaAttr,
	S_AVIFMT_AVI_CONTENT	*psAVIFileContent
)
{
	ERRCODE	errRet;

	if ((errRet = AVIUtility_ParseMainAviHeader(psMediaAttr, psAVIFileContent)) != ERR_AVIUTIL_SUCCESS)
		return errRet;

	if ((errRet = AVIUtility_ParseVideoStreamHeaderChunk(psMediaAttr, psAVIFileContent)) != ERR_AVIUTIL_SUCCESS)
		psMediaAttr->sVideoAttr.u32FourCC = 0;

	// Audio Stream header
	if ((errRet = AVIUtility_ParseAudioStreamHeaderChunk(psMediaAttr, psAVIFileContent)) != ERR_AVIUTIL_SUCCESS)
		psMediaAttr->sAudioAttr.u8FmtTag = 0;

	if ((errRet = AVIUtility_ParseJunkChunk(psMediaAttr, psAVIFileContent)) != ERR_AVIUTIL_SUCCESS)
		return errRet;

	if ((errRet = AVIUtility_ParseListMoviChunkHeader(psMediaAttr, psAVIFileContent)) != ERR_AVIUTIL_SUCCESS)
		return errRet;

	if ((errRet = AVIUtility_ParseIdx1Chunk(psMediaAttr, psAVIFileContent)) != ERR_AVIUTIL_SUCCESS)
		return errRet;

	return ERR_AVIUTIL_SUCCESS;
}







