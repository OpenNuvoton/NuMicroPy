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

#ifndef __AVI_UTILITY_H__
#define __AVI_UTILITY_H__
#include <stdint.h>
#include "AVIFMT.h"

// Defined Error Code in here
#ifndef ERRCODE
#define ERRCODE int32_t
#endif

#define AVIUTIL_ERR						0x80080000							

#define ERR_AVIUTIL_SUCCESS			0
#define ERR_AVIUTIL_FILE_HANDLE		(AVIUTIL_ERR | 0x1)
#define ERR_AVIUTIL_CODEC_FMT		(AVIUTIL_ERR | 0x2)
#define ERR_AVIUTIL_CRETAE_TMP_FILE	(AVIUTIL_ERR | 0x3)
#define ERR_AVIUTIL_INVALID_PARAM	(AVIUTIL_ERR | 0x4)
#define ERR_AVIUTIL_MALLOC			(AVIUTIL_ERR | 0x5)
#define ERR_AVIUTIL_IDX1_HEADER    	(AVIUTIL_ERR | 0x6)
#define ERR_AVIUTIL_LIST_MOVI_CHUNK		(AVIUTIL_ERR | 0x7)
#define ERR_AVIUTIL_AUDIO_STREAM_HEADER	(AVIUTIL_ERR | 0x8)
#define ERR_AVIUTIL_JUNK_HEADER		(AVIUTIL_ERR | 0x9)
#define ERR_AVIUTIL_VIDEO_STREAM_HEADER	(AVIUTIL_ERR | 0xa)
#define ERR_AVIUTIL_MAIN_AVI_HEADER	(AVIUTIL_ERR | 0xb)

#define MJPEG_FOURCC 	mmioFOURCC('M','J','P','G')
#define JPEG_FOURCC		mmioFOURCC('J','P','E','G')
#define YUYV_FOURCC		mmioFOURCC('Y','U','Y','V')
#define H264_FOURCC		mmioFOURCC('H','2','6','4')

#define PCM_FMT_TAG			0x01
#define	ADPCM_FMT_TAG		0x02
#define IMA_ADPCM_FMT_TAG	0x11
#define ULAW_FMT_TAG		0x07
#define MP3_FMT_TAG			0x55
#define ALAW_FMT_TAG		0x06
#define AAC_FMT_TAG			0xFF
#define G726_FMT_TAG		0x45

#define TEMP_FILE_DIR	"/tmp/"

#ifndef BOOL
#define BOOL int32_t
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

typedef struct{
	int32_t					i32FileHandle;
	S_AVIFMT_AVI_CONTENT	sAVICtx;
	uint32_t 				u32MBResvSize;
	uint32_t				u32AudioFrame;
	uint32_t				u32FixedNo;
	uint32_t				u32TempPos;
	int32_t					i32TempFileHandle;
	int32_t					i32AnoFileHandle;
	uint32_t				u32AnoPos;
	uint32_t				u32CurrentPos;
	uint32_t				u32VideoSuggSize;
	uint32_t				u32AudioSuggSize;
	BOOL					bIdxUseFile;
	uint64_t				u64FirstFrameTime;
	uint64_t				u64LastFrameTime;	
	char					*szFullFileName;
	uint8_t					*pu8IdxTableMem;
}S_AVIUTIL_AVIHANDLE;

// AVI file attribute
typedef struct
{
	uint32_t	u32FrameWidth;
	uint32_t	u32FrameHeight;
	uint32_t	u32FrameRate;
	uint32_t	u32SamplingRate;
	uint32_t	u32VideoFcc;
	uint32_t	u32AudioFmtTag;
	uint32_t	u32AudioChannel;
	uint32_t	u32AudioFrameSize;
	uint32_t	u32AudioSamplesPerFrame;
	uint32_t 	u32AudioBitRate;
	uint32_t	u32Reserved;
}S_AVIUTIL_FILEATTR;

ERRCODE
AVIUtility_CreateAVIFile(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	S_AVIUTIL_FILEATTR *psAVIFileAttr,
	char *szTempFilePath		//Used if psAVIHandle->bIdxUseFile == TRUE
);

uint32_t						// add audio frame to AVI file
AVIUtility_AddAudioFrame(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	uint8_t * pbyAudioBuf,		// audio buffer [in], should reserve 8 bits in the front.
	uint32_t u32AudioSize		// audio size
);

uint32_t
AVIUtility_AddVideoFrame(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
    uint8_t* pbyJPEGBuf,
    uint32_t u32JPEGSize
);

uint32_t
AVIUtility_WriteMemoryNeed(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	uint32_t u32RecTimeSec
);

uint32_t *
AVIUtility_WriteConfigMemory(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	uint32_t	*pu32MemBuf
);


void
AVIUtility_FinalizeAVIFile(
	S_AVIUTIL_AVIHANDLE *psAVIHandle,
	char *szTempFilePath		//Used if psAVIHandle->bIdxUseFile == TRUE
);


//for AVI reader
typedef struct{
	uint32_t			u32FourCC;		///< Video payload type
	uint32_t 			u32Profile;			///< H.264 profile
	uint32_t 			u32Level;			///< H.264 level	
	uint32_t 			u32MaxChunkSize;
	uint32_t			u32FrameRate;
	uint32_t 			u32BitRate;
	uint32_t			u32Height;
	uint32_t			u32Width;
	uint32_t			u32ChunkNum;	//total frames
	uint32_t			u32VideoScale; //Copy to S_AVIREAD_RES
	uint32_t 			u32VideoRate; //Copy to S_AVIREAD_RES
}S_AVIREAD_VIDEO_ATTR;

typedef struct{
	uint8_t	 u8FmtTag;		///< Audio payload type
	uint32_t u32SampleRate;
	uint32_t u32ChannelNum;
	uint32_t u32BitRate;
	uint32_t u32MaxChunkSize;
	uint32_t u32SmplNumPerChunk;
	uint32_t u32AvgBytesPerSec;	//copy to S_AVIREAD_RES
	uint32_t u32AudioScale;		//Copy to S_AVIREAD_RES
	uint32_t u32AudioRate;		//Copy to S_AVIREAD_RES
	uint32_t u32Profile;	//for AAC
}S_AVIREAD_AUDIO_ATTR;

typedef struct _S_AVIREAD_MEDIA_ATTR {
	//input attr
	int32_t i32FileHandle;
	uint32_t u32MediaSize;

	//output attr
	uint32_t u32Duration;
	S_AVIREAD_VIDEO_ATTR	sVideoAttr;
	S_AVIREAD_AUDIO_ATTR	sAudioAttr;
}S_AVIREAD_MEDIA_ATTR;

ERRCODE
AVIUtility_AVIParse(
	S_AVIREAD_MEDIA_ATTR *psMediaAttr,
	S_AVIFMT_AVI_CONTENT	*psAVIFileContent
);

#endif

