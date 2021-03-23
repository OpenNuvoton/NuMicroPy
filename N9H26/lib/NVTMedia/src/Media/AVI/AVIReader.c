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

#include "NVTMedia_SAL_OS.h"

#define PARSE_FRAME_LEN 512

typedef struct _S_AVIREAD_RES {
	uint32_t				u32PosMOVI;				//record AVI MOVI position
	uint32_t				u32VideoChunkNum;
	uint32_t				u32AudioChunkNum;
	uint32_t				u32AudioScale;
	uint32_t				u32AudioRate;
	uint32_t				u32VideoScale;
	uint32_t				u32VideoRate;
	uint16_t   				u16BitsPerSample;
	uint32_t				u32AvgBytesPerSec;
	S_AVIFMT_AVIINDEXENTRY	*psVideoChunkTable;
	S_AVIFMT_AVIINDEXENTRY	*psAudioChunkTable;
	uint64_t				*pu64AudioDataTime;
	uint64_t				*pu64VideoDataTime;
	uint8_t					*pu8H264FrameType;
	E_NM_CTX_VIDEO_TYPE		eVideoType;
	int32_t					i32FileNo;
}S_MIF_AVIREAD_RES;

//AVIRead_AVI_ParseIFrame
static bool
FillFrameInfo(
	S_NM_AVI_MEDIA_ATTR	*psMediaAttr,
	S_NM_UTIL_H264_FRAME_INFO	*psFrameInfo,
	uint8_t					*pu8BitStream,
	int32_t					i32ChunkID,
	S_MIF_AVIREAD_RES		*psAVIReadRes
)
{
	if (psFrameInfo->eNALType & eNM_UTIL_H264_NAL_SPS) {
		psAVIReadRes->pu8H264FrameType[i32ChunkID] |= eNM_UTIL_H264_NAL_SPS;
		
		if(psMediaAttr){
			psMediaAttr->eH264Profile = (E_NM_H264_PROFILE)(pu8BitStream[psFrameInfo->u32SPSOffset +
					psFrameInfo->u32SPSStartCodeLen + 1]);

			psMediaAttr->u32H264Level = pu8BitStream[psFrameInfo->u32SPSOffset +
					psFrameInfo->u32SPSStartCodeLen + 3];
		}
	}

	if (psFrameInfo->eNALType & eNM_UTIL_H264_NAL_PPS) {
		psAVIReadRes->pu8H264FrameType[i32ChunkID] |= eNM_UTIL_H264_NAL_PPS;
	}

	if (psFrameInfo->eNALType & eNM_UTIL_H264_NAL_P) {
		psAVIReadRes->pu8H264FrameType[i32ChunkID] |= eNM_UTIL_H264_NAL_P;
		return false;
	}

	if (psFrameInfo->eNALType & eNM_UTIL_H264_NAL_I) {
		psAVIReadRes->pu8H264FrameType[i32ChunkID] |= eNM_UTIL_H264_NAL_I;
		return true;
	}
	
	return false;
}

static E_NM_ERRNO
AVIRead_Open(
	S_NM_VIDEO_CTX *psVideoCtx,
	S_NM_AUDIO_CTX *psAudioCtx,
	void *pvMediaAttr,	
	void **ppMediaRes
)
{
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;
	S_MIF_AVIREAD_RES *psAVIReadRes = NULL;
	S_NM_AVI_MEDIA_ATTR *psMediaAttr = (S_NM_AVI_MEDIA_ATTR *) pvMediaAttr;	
	ERRCODE eAVIUtiltyRet;
	
	S_AVIFMT_AVIINDEXENTRY	sIndexEntry;
	int32_t				i32Size;
	uint32_t			u32IndexOffset;
	uint32_t			u32Index = 0;
	uint32_t u32AudioSize = 0;
	
	*ppMediaRes = NULL;

	//check parameter
	if((psMediaAttr->i32FD <= 0) || (psMediaAttr->u32MediaSize == 0)){
		return eNM_ERRNO_MEDIA_OPEN;
	}

	if(lseek(psMediaAttr->i32FD, 0, SEEK_SET) < 0) {
		return eNM_ERRNO_IO;
	}
	
	//Initialize
	S_AVIFMT_AVI_CONTENT sAVIFileContent;
	S_AVIREAD_MEDIA_ATTR sAVIAttr;

	memset(&sAVIFileContent, 0 , sizeof(S_AVIFMT_AVI_CONTENT));
	memset(&sAVIAttr, 0 , sizeof(S_AVIREAD_MEDIA_ATTR));
	sAVIAttr.i32FileHandle = psMediaAttr->i32FD;
	sAVIAttr.u32MediaSize =	psMediaAttr->u32MediaSize;
	psAVIReadRes = calloc(1, sizeof(S_MIF_AVIREAD_RES));

	if (psAVIReadRes == NULL) {
		eRet = eNM_ERRNO_MALLOC;
		goto AVIRead_Open_fail;
	}

	//Parse media and fill media information	
	eAVIUtiltyRet = AVIUtility_AVIParse(&sAVIAttr, &sAVIFileContent);
	
	if(eAVIUtiltyRet != ERR_AVIUTIL_SUCCESS){
		eRet = eNM_ERRNO_MEDIA_OPEN;
		goto AVIRead_Open_fail;		
	}

	//Fill informations
	psAVIReadRes->u32PosMOVI = sAVIFileContent.u32PosMOVI;
	psAVIReadRes->u16BitsPerSample = sAVIFileContent.u16BitsPerSample;
	psAVIReadRes->u32VideoScale = sAVIAttr.sVideoAttr.u32VideoScale;
	psAVIReadRes->u32VideoRate = sAVIAttr.sVideoAttr.u32VideoRate;
	psAVIReadRes->u32AvgBytesPerSec = sAVIAttr.sAudioAttr.u32AvgBytesPerSec;
	psAVIReadRes->u32AudioScale = sAVIAttr.sAudioAttr.u32AudioScale;
	psAVIReadRes->u32AudioRate = sAVIAttr.sAudioAttr.u32AudioRate;
	
	//Fill context
	if(sAVIAttr.sVideoAttr.u32FourCC == 0){
		psVideoCtx->eVideoType = eNM_CTX_VIDEO_NONE;
	}
	else if(sAVIAttr.sVideoAttr.u32FourCC == MJPEG_FOURCC){
		psVideoCtx->eVideoType = eNM_CTX_VIDEO_MJPG;
	}
	else if(sAVIAttr.sVideoAttr.u32FourCC == H264_FOURCC){
		psVideoCtx->eVideoType = eNM_CTX_VIDEO_H264;
	}
	else if(sAVIAttr.sVideoAttr.u32FourCC == YUYV_FOURCC){
		psVideoCtx->eVideoType = eNM_CTX_VIDEO_YUV422;
	}
	else {
		psVideoCtx->eVideoType = eNM_CTX_VIDEO_UNKNOWN;
	}
	
	psVideoCtx->u32Width = sAVIAttr.sVideoAttr.u32Width;
	psVideoCtx->u32Height = sAVIAttr.sVideoAttr.u32Height;
	psVideoCtx->u32BitRate = sAVIAttr.sVideoAttr.u32BitRate;

	if(sAVIAttr.sAudioAttr.u8FmtTag == 0){
		psAudioCtx->eAudioType = eNM_CTX_AUDIO_NONE;	
	}		
	else if(sAVIAttr.sAudioAttr.u8FmtTag == ALAW_FMT_TAG){
		psAudioCtx->eAudioType = eNM_CTX_AUDIO_ALAW;	
	}		
	else if(sAVIAttr.sAudioAttr.u8FmtTag == ULAW_FMT_TAG){
		psAudioCtx->eAudioType = eNM_CTX_AUDIO_ULAW;	
	}		
	else if(sAVIAttr.sAudioAttr.u8FmtTag == AAC_FMT_TAG){
		psAudioCtx->eAudioType = eNM_CTX_AUDIO_AAC;	
		psMediaAttr->eAACProfile = (E_NM_AAC_PROFILE)sAVIAttr.sAudioAttr.u32Profile; 
	}		
	else if(sAVIAttr.sAudioAttr.u8FmtTag == MP3_FMT_TAG){
		psAudioCtx->eAudioType = eNM_CTX_AUDIO_MP3;	
	}		
	else if(sAVIAttr.sAudioAttr.u8FmtTag == PCM_FMT_TAG){
		psAudioCtx->eAudioType = eNM_CTX_AUDIO_PCM_L16;	
	}		
	else if(sAVIAttr.sAudioAttr.u8FmtTag == ADPCM_FMT_TAG){
		psAudioCtx->eAudioType = eNM_CTX_AUDIO_ADPCM;	
	}		
	else if(sAVIAttr.sAudioAttr.u8FmtTag == G726_FMT_TAG){
		psAudioCtx->eAudioType = eNM_CTX_AUDIO_G726;	
	}
	else{
		psAudioCtx->eAudioType = eNM_CTX_AUDIO_UNKNOWN;	
	}

	psAudioCtx->u32SampleRate = sAVIAttr.sAudioAttr.u32SampleRate;
	psAudioCtx->u32Channel = sAVIAttr.sAudioAttr.u32ChannelNum;
	psAudioCtx->u32BitRate = sAVIAttr.sAudioAttr.u32BitRate;
	psAudioCtx->u32SamplePerBlock = sAVIAttr.sAudioAttr.u32SmplNumPerChunk;
	
	//Fill media attribute
	psMediaAttr->u32Duration = sAVIAttr.u32Duration;
	psVideoCtx->u32FrameRate = sAVIAttr.sVideoAttr.u32FrameRate;
//	psMediaAttr->u32VideoMaxChunkSize =  sAVIAttr.sVideoAttr.u32MaxChunkSize;
//	psMediaAttr->u32AudioMaxChunkSize =  sAVIAttr.sAudioAttr.u32MaxChunkSize;
	
	//Allocate A/V chunk table memory
	if (sAVIFileContent.u32arrCounter) {
		if(sAVIFileContent.u32TotalFrames) {
			//allocate video chunk table memory
			psAVIReadRes->psVideoChunkTable = malloc(sAVIFileContent.u32TotalFrames * sizeof(S_AVIFMT_AVIINDEXENTRY));

			if(psAVIReadRes->psVideoChunkTable == NULL) {
				eRet = eNM_ERRNO_MALLOC;
				goto AVIRead_Open_fail;				
			}

			psAVIReadRes->pu64VideoDataTime = malloc(sAVIFileContent.u32TotalFrames * sizeof(uint64_t));

			if(psAVIReadRes->pu64VideoDataTime == NULL) {
				eRet = eNM_ERRNO_MALLOC;
				goto AVIRead_Open_fail;				
			}

			psAVIReadRes->pu8H264FrameType = calloc(1, sAVIFileContent.u32TotalFrames * sizeof(uint8_t));

			if(psAVIReadRes->pu8H264FrameType == NULL) {
				eRet = eNM_ERRNO_MALLOC;
				goto AVIRead_Open_fail;				
			}
		}
		else{
			printf("no video chunk, change to eMIF_VIDEO_NONE type\n");
			psVideoCtx->eVideoType = eNM_CTX_VIDEO_NONE;
		}

		if(sAVIFileContent.u32arrCounter - sAVIFileContent.u32TotalFrames) {
			psAVIReadRes->psAudioChunkTable = malloc((sAVIFileContent.u32arrCounter - sAVIFileContent.u32TotalFrames) * sizeof(S_AVIFMT_AVIINDEXENTRY));

			if(psAVIReadRes->psAudioChunkTable == NULL) {
				eRet = eNM_ERRNO_MALLOC;
				goto AVIRead_Open_fail;				
			}

			psAVIReadRes->pu64AudioDataTime = malloc((sAVIFileContent.u32arrCounter - sAVIFileContent.u32TotalFrames) * sizeof(uint64_t));

			if(psAVIReadRes->pu64AudioDataTime == NULL) {
				eRet = eNM_ERRNO_MALLOC;
				goto AVIRead_Open_fail;				
			}
		}
		else{
			printf("no audio chunk, change to eNM_AUDIO_NONE type\n");
			psAudioCtx->eAudioType = eNM_CTX_AUDIO_NONE;
		}
	
	}
	else{
		//without any A/V chunk index table
		printf("no audio & video chunk\n");
		eRet = eNM_ERRNO_CHUNK;
		goto AVIRead_Open_fail;				
	}
	
	//Fill chunk table information
	for (u32Index = 0; u32Index < sAVIFileContent.u32arrCounter; u32Index++) {
		u32IndexOffset = u32Index * sizeof(S_AVIFMT_AVIINDEXENTRY) + sAVIFileContent.u32posIndexEntry;

		if (u32IndexOffset >= psMediaAttr->u32MediaSize) {
			eRet = eNM_ERRNO_EOM;
			goto AVIRead_Open_fail;				
		}

		if(lseek(psMediaAttr->i32FD, u32IndexOffset, SEEK_SET) < 0) {
			eRet = eNM_ERRNO_IO;
			goto AVIRead_Open_fail;
		}


		i32Size = read(psMediaAttr->i32FD, (uint8_t *) & sIndexEntry, sizeof(S_AVIFMT_AVIINDEXENTRY));
		if(i32Size != sizeof(S_AVIFMT_AVIINDEXENTRY)){
			eRet = eNM_ERRNO_IO;
			goto AVIRead_Open_fail;
		}

		if (sIndexEntry.ckid == 0x62643030 || sIndexEntry.ckid == 0x63643030 || sIndexEntry.ckid == 0x62643130 || sIndexEntry.ckid == 0x63643130) {
			((S_AVIFMT_AVIINDEXENTRY *)(psAVIReadRes->psVideoChunkTable))[psAVIReadRes->u32VideoChunkNum] = sIndexEntry;

			if (psAVIReadRes->u32VideoRate)
				psAVIReadRes->pu64VideoDataTime[psAVIReadRes->u32VideoChunkNum] = (uint64_t)psAVIReadRes->u32VideoChunkNum * (uint64_t)1000 * (uint64_t)psAVIReadRes->u32VideoScale / (uint64_t)psAVIReadRes->u32VideoRate;

			psAVIReadRes->u32VideoChunkNum++;
		}
		else if (sIndexEntry.ckid == 0x62773130 || sIndexEntry.ckid == 0x62773030) {
			((S_AVIFMT_AVIINDEXENTRY *)(psAVIReadRes->psAudioChunkTable))[psAVIReadRes->u32AudioChunkNum] = sIndexEntry;

			//TODO: check if video duration is zero
//			if(psMediaAttr->u32Duration == 0){
				if (psAVIReadRes->u32AvgBytesPerSec && psAudioCtx->eAudioType != eNM_CTX_AUDIO_AAC) {
					psAVIReadRes->pu64AudioDataTime[psAVIReadRes->u32AudioChunkNum] = (uint64_t)u32AudioSize * (uint64_t)1000 / (uint64_t)psAVIReadRes->u32AvgBytesPerSec; //sMediaInfo.sAudioInfo.u32ChannelNum;
					//printf(" u32AudioSize %d, psAVIRes->u32AvgBytesPerSec %d,sMediaInfo.sAudioInfo.u32ChannelNum %d, psAVIRes->pu64AudioDataTime %"PRId64" \n",u32AudioSize,psAVIRes->u32AvgBytesPerSec,sMediaInfo.sAudioInfo.u32ChannelNum,psAVIRes->pu64AudioDataTime[psAVIRes->u32AudioChunkNum-1]);
				}
				else if(psAudioCtx->u32SamplePerBlock && psAudioCtx->eAudioType != eNM_CTX_AUDIO_AAC){
					psAVIReadRes->pu64AudioDataTime[psAVIReadRes->u32AudioChunkNum] = (uint64_t)psAVIReadRes->u32AudioChunkNum * (uint64_t)psAudioCtx->u32SamplePerBlock * (uint64_t)1000 / (uint64_t)psAudioCtx->u32SampleRate;
				}
				else {
					if(psAVIReadRes->u32AudioRate)
						psAVIReadRes->pu64AudioDataTime[psAVIReadRes->u32AudioChunkNum] = (uint64_t)psAVIReadRes->u32AudioChunkNum * (uint64_t)1000 * (uint64_t)psAVIReadRes->u32AudioScale / (uint64_t)psAVIReadRes->u32AudioRate;
				}
//			}
			
			psAVIReadRes->u32AudioChunkNum++;
			u32AudioSize += sIndexEntry.dwChunkLength;
		}
	}
	
	printf("AudioChunkNum %d VideoChunkNum %d\n",psAVIReadRes->u32AudioChunkNum, psAVIReadRes->u32VideoChunkNum);

	psMediaAttr->u32VideoChunks = psAVIReadRes->u32VideoChunkNum;
	psMediaAttr->u32AudioChunks = psAVIReadRes->u32AudioChunkNum;
	
	if((psAVIReadRes->u32AudioChunkNum) && (psMediaAttr->u32Duration == 0))
		psMediaAttr->u32Duration = psAVIReadRes->pu64AudioDataTime[psAVIReadRes->u32AudioChunkNum - 1];
	
#if 0
	if(psAVIReadRes->u32AudioChunkNum && psMediaAttr->u32Duration){
		uint32_t u32AudioTotalSize = u32AudioSize;
		u32AudioSize=0;
		for(u32Index=0 ; u32Index < psAVIReadRes->u32AudioChunkNum ; u32Index++){
			S_AVIFMT_AVIINDEXENTRY	*psIndexEntry = &psAVIReadRes->psAudioChunkTable[u32Index];
			psAVIReadRes->pu64AudioDataTime[u32Index] = (uint64_t)psMediaAttr->u32Duration * (uint64_t)u32AudioSize / (uint64_t)u32AudioTotalSize;
			u32AudioSize += psIndexEntry->dwChunkLength;
		}
	}
#endif

	//Fill H264 frame type
	if(psAVIReadRes->u32VideoChunkNum && psVideoCtx->eVideoType == eNM_CTX_VIDEO_H264) {
		S_AVIFMT_AVIINDEXENTRY	*psIndexEntry = &psAVIReadRes->psVideoChunkTable[0];
		uint8_t	FrameHeader[PARSE_FRAME_LEN];
		S_NM_UTIL_H264_FRAME_INFO sFrameInfo;
		uint32_t u32ChunkOffset = psIndexEntry->dwChunkOffset + psAVIReadRes->u32PosMOVI + 4;

		if(u32ChunkOffset >= psMediaAttr->u32MediaSize){
			eRet = eNM_ERRNO_EOM;
			goto AVIRead_Open_fail;
		}

		if(lseek(psMediaAttr->i32FD, u32ChunkOffset, SEEK_SET) < 0){
			eRet = eNM_ERRNO_IO;
			goto AVIRead_Open_fail;
		}

		i32Size = read(psMediaAttr->i32FD, (uint8_t *)FrameHeader, PARSE_FRAME_LEN);
		
		if (i32Size != PARSE_FRAME_LEN){
			eRet = eNM_ERRNO_IO;
			goto AVIRead_Open_fail;
		}

		NMUtil_ParseH264Frame(FrameHeader, PARSE_FRAME_LEN, &sFrameInfo);
		FillFrameInfo(psMediaAttr, &sFrameInfo, FrameHeader, 0, psAVIReadRes);	
	}
	
	psAVIReadRes->eVideoType = psVideoCtx->eVideoType;	
	psAVIReadRes->i32FileNo = psMediaAttr->i32FD;
	*ppMediaRes = psAVIReadRes;
	return eNM_ERRNO_NONE;
	
AVIRead_Open_fail:

	if(psAVIReadRes){
		if(psAVIReadRes->psVideoChunkTable)
			free(psAVIReadRes->psVideoChunkTable);

		if(psAVIReadRes->pu64VideoDataTime)
			free(psAVIReadRes->pu64VideoDataTime);

		if(psAVIReadRes->pu8H264FrameType)
			free(psAVIReadRes->pu8H264FrameType);

		if(psAVIReadRes->psAudioChunkTable)
			free(psAVIReadRes->psAudioChunkTable);

		if(psAVIReadRes->pu64AudioDataTime)
			free(psAVIReadRes->pu64AudioDataTime);

		free(psAVIReadRes);
	}
	
	return eRet;
}

static E_NM_ERRNO
AVIRead_Close(
	void	**ppMediaRes
)
{
	S_MIF_AVIREAD_RES	*psAVIReadRes;
	
	if (!ppMediaRes || !*ppMediaRes)
		return eNM_ERRNO_NULL_PTR;

	psAVIReadRes = (S_MIF_AVIREAD_RES *)*ppMediaRes;

	if(psAVIReadRes->psVideoChunkTable)
		free(psAVIReadRes->psVideoChunkTable);

	if(psAVIReadRes->pu64VideoDataTime)
		free(psAVIReadRes->pu64VideoDataTime);

	if(psAVIReadRes->pu8H264FrameType)
		free(psAVIReadRes->pu8H264FrameType);

	if(psAVIReadRes->psAudioChunkTable)
		free(psAVIReadRes->psAudioChunkTable);

	if(psAVIReadRes->pu64AudioDataTime)
		free(psAVIReadRes->pu64AudioDataTime);

	free(psAVIReadRes);
	*ppMediaRes = NULL;

	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
AVIRead_GetAudioChunkInfo(
	uint32_t			u32ChunkID,
	uint32_t			*pu32ChunkSize,
	uint64_t			*pu64ChunkTime,
	bool				*pbSeekable,
	void				*pMediaRes
)
{
	S_MIF_AVIREAD_RES *psAVIReadRes = (S_MIF_AVIREAD_RES *)pMediaRes;

	if(psAVIReadRes == NULL){
		return eNM_ERRNO_NULL_RES;
	}
	
	if (u32ChunkID >= psAVIReadRes->u32AudioChunkNum)
		return eNM_ERRNO_EOM;

	S_AVIFMT_AVIINDEXENTRY	*psIndexEntry = &psAVIReadRes->psAudioChunkTable[u32ChunkID];

	if(pu32ChunkSize)
		*pu32ChunkSize = psIndexEntry->dwChunkLength;

	if(pu64ChunkTime)
		*pu64ChunkTime = psAVIReadRes->pu64AudioDataTime[u32ChunkID];

	if(pbSeekable)
		*pbSeekable = true;

	return eNM_ERRNO_NONE;
}


static E_NM_ERRNO
AVIRead_GetVideoChunkInfo(
	uint32_t			u32ChunkID,
	uint32_t			*pu32ChunkSize,
	uint64_t			*pu64ChunkTime,
	bool				*pbSeekable,
	void				*pMediaRes
)
{
	S_MIF_AVIREAD_RES *psAVIReadRes = (S_MIF_AVIREAD_RES *)pMediaRes;

	if(psAVIReadRes == NULL){
		return eNM_ERRNO_NULL_RES;
	}

	if (u32ChunkID >= psAVIReadRes->u32VideoChunkNum)
		return eNM_ERRNO_EOM;

	S_AVIFMT_AVIINDEXENTRY	*psIndexEntry = &psAVIReadRes->psVideoChunkTable[u32ChunkID];

	if(pu32ChunkSize)
		*pu32ChunkSize = psIndexEntry->dwChunkLength;

	if(pu64ChunkTime)
		*pu64ChunkTime = psAVIReadRes->pu64VideoDataTime[u32ChunkID];

	if(pbSeekable) {
		if(psAVIReadRes->eVideoType == eNM_CTX_VIDEO_H264){
			if (psAVIReadRes->pu8H264FrameType[u32ChunkID] == 0)	{
				uint8_t	FrameHeader[PARSE_FRAME_LEN];

				const uint32_t	u32ChunkOffset = psIndexEntry->dwChunkOffset + psAVIReadRes->u32PosMOVI + 4;
				int32_t				i32Size = 0;
				int32_t				i32FileNo = psAVIReadRes->i32FileNo;

				if (lseek(i32FileNo, u32ChunkOffset, SEEK_SET) < 0)
					return eNM_ERRNO_IO;

				i32Size = read(i32FileNo, (uint8_t *)FrameHeader, PARSE_FRAME_LEN);

				if (i32Size != PARSE_FRAME_LEN)
					return eNM_ERRNO_IO;

				S_NM_UTIL_H264_FRAME_INFO sFrameInfo;
				NMUtil_ParseH264Frame(FrameHeader, PARSE_FRAME_LEN, &sFrameInfo);
				FillFrameInfo(NULL, &sFrameInfo, FrameHeader, u32ChunkID, psAVIReadRes);				
			}

			if (psAVIReadRes->pu8H264FrameType[u32ChunkID] & eNM_UTIL_H264_NAL_SPS || psAVIReadRes->pu8H264FrameType[u32ChunkID] & eNM_UTIL_H264_NAL_I)
				*pbSeekable = true;
			else
				*pbSeekable = false;
			
		}
		else{
			*pbSeekable = true;
		}
	}

	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
AVIRead_ReadAudio(
	uint32_t			u32ChunkID,
	S_NM_AUDIO_CTX	*psCtx,
	void				*pMediaRes
)
{
	S_MIF_AVIREAD_RES *psAVIReadRes = (S_MIF_AVIREAD_RES *)pMediaRes;

	if(psAVIReadRes == NULL){
		return eNM_ERRNO_NULL_RES;
	}

	if (psCtx->pu8DataBuf == NULL)
		return eNM_ERRNO_NULL_PTR;

	if (u32ChunkID >= psAVIReadRes->u32AudioChunkNum)
		return eNM_ERRNO_EOM;

	int32_t				i32Size = 0;
	int32_t				i32FileNo = psAVIReadRes->i32FileNo;

	S_AVIFMT_AVIINDEXENTRY	*psIndexEntry = &psAVIReadRes->psAudioChunkTable[u32ChunkID];

	//----------------Read Chunk------------------
	// +4 is for skip the 4-byte size field

	const uint32_t	u32ChunkOffset = psIndexEntry->dwChunkOffset + psAVIReadRes->u32PosMOVI + 4;

	if (psCtx->u32DataLimit < psIndexEntry->dwChunkLength)
		return eNM_ERRNO_CTX;

	if (lseek(i32FileNo, u32ChunkOffset, SEEK_SET) < 0)
		return eNM_ERRNO_IO;

	i32Size = read(i32FileNo, psCtx->pu8DataBuf, psIndexEntry->dwChunkLength);

	if (i32Size != psIndexEntry->dwChunkLength)
		return eNM_ERRNO_IO;

	//update datatime
	psCtx->u32DataSize = psIndexEntry->dwChunkLength;
	psCtx->u64DataTime = psAVIReadRes->pu64AudioDataTime[u32ChunkID];
	psCtx->bKeyFrame = true;

	return eNM_ERRNO_NONE;
}


static E_NM_ERRNO
AVIRead_ReadVideo(
	uint32_t			u32ChunkID,
	S_NM_VIDEO_CTX	*psCtx,
	void				*pMediaRes
)
{
	S_MIF_AVIREAD_RES *psAVIReadRes = (S_MIF_AVIREAD_RES *)pMediaRes;

	if(psAVIReadRes == NULL){
		return eNM_ERRNO_NULL_RES;
	}

	if (psCtx->pu8DataBuf == NULL)
		return eNM_ERRNO_NULL_PTR;

	if (u32ChunkID >= psAVIReadRes->u32VideoChunkNum)
		return eNM_ERRNO_EOM;

	int32_t				i32Size = 0;
	int32_t				i32FileNo = psAVIReadRes->i32FileNo;

	S_AVIFMT_AVIINDEXENTRY	*psIndexEntry = &psAVIReadRes->psVideoChunkTable[u32ChunkID];

	//----------------Read Chunk------------------
	// +4 is for skip the 4-byte size field

	const uint32_t	u32ChunkOffset = psIndexEntry->dwChunkOffset + psAVIReadRes->u32PosMOVI + 4;

	if (psCtx->u32DataLimit < psIndexEntry->dwChunkLength)
		return eNM_ERRNO_CTX;

	if (lseek(i32FileNo, u32ChunkOffset, SEEK_SET) < 0)
		return eNM_ERRNO_IO;

	i32Size = read(i32FileNo, psCtx->pu8DataBuf, psIndexEntry->dwChunkLength);

	if (i32Size != psIndexEntry->dwChunkLength)
		return eNM_ERRNO_IO;

	//update datatime
	psCtx->u32DataSize = psIndexEntry->dwChunkLength;
	psCtx->u64DataTime = psAVIReadRes->pu64VideoDataTime[u32ChunkID];
	
	if(psCtx->eVideoType == eNM_CTX_VIDEO_H264){
		if(psAVIReadRes->pu8H264FrameType[u32ChunkID] == eNM_UTIL_H264_NAL_UNKNOWN){
			S_NM_UTIL_H264_FRAME_INFO sFrameInfo;
			NMUtil_ParseH264Frame(psCtx->pu8DataBuf, psCtx->u32DataSize, &sFrameInfo);
			FillFrameInfo(NULL, &sFrameInfo, psCtx->pu8DataBuf, u32ChunkID, psAVIReadRes);							
		}

		if (psAVIReadRes->pu8H264FrameType[u32ChunkID] & eNM_UTIL_H264_NAL_SPS || psAVIReadRes->pu8H264FrameType[u32ChunkID] & eNM_UTIL_H264_NAL_I)
			psCtx->bKeyFrame = true;
		else
			psCtx->bKeyFrame = false;		
	}
	else{
		psCtx->bKeyFrame = true;
	}
	return eNM_ERRNO_NONE;
}





S_NM_MEDIAREAD_IF g_sAVIReaderIF = {
	.eMediaType = eNM_MEDIA_AVI,

	.pfnOpenMedia = AVIRead_Open,
	.pfnCloseMedia = AVIRead_Close,

	.pfnReadAudioChunk = AVIRead_ReadAudio,
	.pfnReadVideoChunk = AVIRead_ReadVideo,

	.pfnGetAudioChunkInfo = AVIRead_GetAudioChunkInfo,
	.pfnGetVideoChunkInfo = AVIRead_GetVideoChunkInfo,
};
