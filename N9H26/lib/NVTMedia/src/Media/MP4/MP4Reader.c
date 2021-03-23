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

#include "NVTMedia_SAL_OS.h"

#include "minimp4_demux.h"

#define INVALID_TRACK_INDEX		-1

typedef struct _S_MP4READ_RES {
	int32_t					i32FileNo;
	MP4D_demux_t 		tMP4;
	int32_t i32VideoTrackIndex;
	int32_t i32AudioTrackIndex;
	
	int32_t i32VideoTrackScale;
	int32_t i32AudioTrackScale;

	int32_t i32VideoChunks;
	int32_t i32AudioChunks;
	E_NM_CTX_VIDEO_TYPE	eVideoType;
	
	uint32_t		u32SPSSize;			// SPS size
	uint8_t			*pu8SPS;			// Pointer to SPS
	uint32_t		u32PPSSize;			// PPS size
	uint8_t			*pu8PPS;			// Pointer to PPS
	
}S_MP4READ_RES;

typedef	union {
	struct {
		uint16_t	u3Dummy			: 3;	// GASpecificConfig
		uint16_t	u4ChannelNum	: 4;
		uint16_t	u4SampleRateIdx	: 4;
		uint16_t	u5ObjectType	: 5;

		uint16_t	u5AudioObjType	: 5;
		uint16_t	u11SyncExt		: 11;
	} sDecConfig;
	uint8_t	au8Byte[4];
} U_AACDecConfig;



//
// Decodes ISO/IEC 14496 MP4 object type to context type
//
static E_NM_CTX_VIDEO_TYPE GetVideoCtxType(
	int i32ObjTypeIndicate
)
{
	switch (i32ObjTypeIndicate)
	{
		case MP4_OBJECT_TYPE_AVC:
			return eNM_CTX_VIDEO_H264;
		default:
			return eNM_CTX_VIDEO_UNKNOWN;
	}
}

static E_NM_CTX_AUDIO_TYPE GetAudioCtxType(
	int i32ObjTypeIndicate
)
{
	switch (i32ObjTypeIndicate)
	{
		case MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_MAIN_PROFILE:
		case MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_LC_PROFILE:
		case MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_SSR_PROFILE:
		case MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3:
			return eNM_CTX_AUDIO_AAC;
		case MP4_OBJECT_TYPE_AUDIO_ISO_IEC_11172_3:
			return eNM_CTX_AUDIO_MP3;
		default:
			return eNM_CTX_AUDIO_UNKNOWN;
	}
}


static int
FillVideoInfo(
	MP4D_track_t *psTrack,
	S_NM_VIDEO_CTX *psVideoCtx,
	S_NM_MP4_MEDIA_ATTR *psMediaAttr
)
{
	psVideoCtx->eVideoType = GetVideoCtxType(psTrack->object_type_indication);
	psVideoCtx->u32Width = psTrack->SampleDescription.video.width;
	psVideoCtx->u32Height = psTrack->SampleDescription.video.height;
	
	return 0;
}

static int
FillAudioInfo(
	MP4D_track_t *psTrack,
	S_NM_AUDIO_CTX *psAudioCtx,
	S_NM_MP4_MEDIA_ATTR *psMediaAttr
)
{
	
	psAudioCtx->eAudioType = GetAudioCtxType(psTrack->object_type_indication);	
	psAudioCtx->u32Channel = psTrack->SampleDescription.audio.channelcount;
	psAudioCtx->u32SampleRate = psTrack->SampleDescription.audio.samplerate_hz;
	
	if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_AAC){
		psMediaAttr->eAACProfile =  eNM_AAC_UNKNOWN;
		
		if(psTrack->object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_MAIN_PROFILE){
			psMediaAttr->eAACProfile = eNM_AAC_MAIN;
		}
		else if(psTrack->object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_LC_PROFILE){
			psMediaAttr->eAACProfile = eNM_AAC_LC;
		}
		else if(psTrack->object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_SSR_PROFILE){
			psMediaAttr->eAACProfile = eNM_AAC_SSR;
		}
	}
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

static E_NM_ERRNO
MP4Read_Open(
	S_NM_VIDEO_CTX *psVideoCtx,
	S_NM_AUDIO_CTX *psAudioCtx,
	void *pvMediaAttr,	
	void **ppMediaRes
)
{
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;
	S_MP4READ_RES *psMP4ReadRes = NULL;
	S_NM_MP4_MEDIA_ATTR *psMediaAttr = (S_NM_MP4_MEDIA_ATTR *) pvMediaAttr;	
	bool bMP4Opened = false;
	uint64_t u64Duration;
	int i;
	MP4D_track_t *psTrack;
	
	*ppMediaRes = NULL;

	//check parameter
	if((psMediaAttr->i32FD <= 0) || (psMediaAttr->u32MediaSize == 0)){
		return eNM_ERRNO_MEDIA_OPEN;
	}

	//seek to file begin
	if(lseek(psMediaAttr->i32FD, 0, SEEK_SET) < 0) {
		return eNM_ERRNO_IO;
	}

	//Alloc media resource
	psMP4ReadRes = calloc(1, sizeof(S_MP4READ_RES));

	if(psMP4ReadRes == NULL)
		return eNM_ERRNO_MALLOC;

	if(MP4D__open(&(psMP4ReadRes->tMP4), psMediaAttr->i32FD) == false){
		NMLOG_ERROR("Unable open MP4 demux\n");
		eRet = eNM_ERRNO_MEDIA_OPEN;
		goto MP4Read_Open_fail;
	}
	
//	MP4D__printf_info(&psMP4ReadRes->tMP4);
	
	u64Duration = (uint64_t)psMP4ReadRes->tMP4.duration_hi;
	u64Duration =(u64Duration * 4294967296) + psMP4ReadRes->tMP4.duration_lo;
	u64Duration = (u64Duration * 1000) / psMP4ReadRes->tMP4.timescale;
	
	psMediaAttr->u64Duration = u64Duration;
	
	psMP4ReadRes->i32VideoTrackIndex = INVALID_TRACK_INDEX;
	psMP4ReadRes->i32AudioTrackIndex = INVALID_TRACK_INDEX;
	
	for (i = 0; i < psMP4ReadRes->tMP4.track_count; i++)
	{
		psTrack = psMP4ReadRes->tMP4.track + i;

		if(psTrack->handler_type == MP4D_HANDLER_TYPE_VIDE){
			//Visual stream
			FillVideoInfo(psTrack, psVideoCtx, psMediaAttr);
			
			if(psVideoCtx->eVideoType == eNM_CTX_VIDEO_H264){
				char *achSPS = NULL;
				int i32SPSBytes;
				char *achPPS = NULL;
				int i32PPSBytes;

				achSPS = (char *)MP4D__read_sps(&psMP4ReadRes->tMP4, i, 0, &i32SPSBytes);
				
				if(achSPS){
					psMediaAttr->eH264Profile = (E_NM_H264_PROFILE)achSPS[1];
					psMediaAttr->u32H264Level = achSPS[3];					

					psMP4ReadRes->pu8SPS = malloc(i32SPSBytes);
					if(psMP4ReadRes->pu8SPS){
						memcpy(psMP4ReadRes->pu8SPS, achSPS, i32SPSBytes);
						psMP4ReadRes->u32SPSSize = i32SPSBytes;
					}
					else{
						eRet = eNM_ERRNO_MALLOC;
						goto MP4Read_Open_fail;
					}
			}

				achPPS = (char *)MP4D__read_pps(&psMP4ReadRes->tMP4, i, 0, &i32PPSBytes);		

				if(achPPS){
					psMP4ReadRes->pu8PPS = malloc(i32PPSBytes);
					if(psMP4ReadRes->pu8PPS){
						memcpy(psMP4ReadRes->pu8PPS, achPPS, i32PPSBytes);
						psMP4ReadRes->u32PPSSize = i32PPSBytes;
					}
				}
				else{
					eRet = eNM_ERRNO_MALLOC;
					goto MP4Read_Open_fail;
				}
			}
			
			psMediaAttr->u32VideoChunks = psTrack->sample_count;
			psMP4ReadRes->i32VideoTrackIndex = i;
			psMP4ReadRes->i32VideoTrackScale = psTrack->timescale;
			psMP4ReadRes->i32VideoChunks = psMediaAttr->u32VideoChunks;
			psMP4ReadRes->eVideoType = psVideoCtx->eVideoType;
		}
		
		if(psTrack->handler_type == MP4D_HANDLER_TYPE_SOUN){
			//Audio stream
			FillAudioInfo(psTrack, psAudioCtx, psMediaAttr);
			if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_AAC){
				unsigned char *pchDSI = psTrack->dsi;

				if(pchDSI){
					//retrieve AAC profile from decode config
					U_AACDecConfig uAACDecConfig;
					
					uAACDecConfig.au8Byte[0] = pchDSI[1];
					uAACDecConfig.au8Byte[1] = pchDSI[0];
					uAACDecConfig.au8Byte[2] = pchDSI[3];
					uAACDecConfig.au8Byte[3] = pchDSI[2];
					
					psMediaAttr->eAACProfile = (E_NM_AAC_PROFILE)uAACDecConfig.sDecConfig.u5ObjectType;
				}
			}
			
			psMediaAttr->u32AudioChunks = psTrack->sample_count;
			psMP4ReadRes->i32AudioTrackIndex = i;
			psMP4ReadRes->i32AudioTrackScale = psTrack->timescale;
			psMP4ReadRes->i32AudioChunks = psMediaAttr->u32AudioChunks;
		}
	}
		
	//video chunks and audio chunks
	
	bMP4Opened = true;
	psMP4ReadRes->i32FileNo = psMediaAttr->i32FD;
	*ppMediaRes = psMP4ReadRes;

	return eNM_ERRNO_NONE;

MP4Read_Open_fail:

	if(bMP4Opened == true){
		MP4D__close(&psMP4ReadRes->tMP4);
	}
	
	if(psMP4ReadRes){
		if(psMP4ReadRes->pu8SPS)
			free(psMP4ReadRes->pu8SPS);

		if(psMP4ReadRes->pu8PPS)
			free(psMP4ReadRes->pu8PPS);

		free(psMP4ReadRes);
	}

	return eRet;
}

static E_NM_ERRNO
MP4Read_Close(
	void	**ppMediaRes
)
{
	S_MP4READ_RES	*psMP4ReadRes;
	
	if (!ppMediaRes || !*ppMediaRes)
		return eNM_ERRNO_NULL_PTR;

	psMP4ReadRes = (S_MP4READ_RES *)*ppMediaRes;

	MP4D__close(&psMP4ReadRes->tMP4);
	
	free(psMP4ReadRes);
	*ppMediaRes = NULL;
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
MP4Read_GetAudioChunkInfo(
	uint32_t			u32ChunkID,
	uint32_t			*pu32ChunkSize,
	uint64_t			*pu64ChunkTime,
	bool				*pbSeekable,
	void				*pMediaRes
)
{
	S_MP4READ_RES *psMP4ReadRes = (S_MP4READ_RES *)pMediaRes;

	if(psMP4ReadRes == NULL){
		return eNM_ERRNO_NULL_RES;
	}
	
	if(psMP4ReadRes->i32AudioTrackIndex == INVALID_TRACK_INDEX)
		return eNM_ERRNO_CHUNK;
	
	if (u32ChunkID >= psMP4ReadRes->i32AudioChunks)
		return eNM_ERRNO_EOM;
	
	uint32_t u32FrameSize;
	uint32_t u32Timestamp;
	uint32_t u32Duration;

	MP4D__frame_offset(&psMP4ReadRes->tMP4, psMP4ReadRes->i32AudioTrackIndex, u32ChunkID, (unsigned int *)&u32FrameSize, (unsigned int *)&u32Timestamp, (unsigned int *)&u32Duration);

	*pu32ChunkSize = u32FrameSize;	
	uint64_t u64Timestamp = (uint64_t)u32Timestamp;
	*pu64ChunkTime = (u64Timestamp * 1000) / (psMP4ReadRes->i32AudioTrackScale);
	
	if(pbSeekable)
		*pbSeekable = true;

	return eNM_ERRNO_NONE;
}


static E_NM_ERRNO
MP4Read_GetVideoChunkInfo(
	uint32_t			u32ChunkID,
	uint32_t			*pu32ChunkSize,
	uint64_t			*pu64ChunkTime,
	bool				*pbSeekable,
	void				*pMediaRes
)
{
	S_MP4READ_RES *psMP4ReadRes = (S_MP4READ_RES *)pMediaRes;

	if(psMP4ReadRes == NULL){
		return eNM_ERRNO_NULL_RES;
	}
	
	if(psMP4ReadRes->i32VideoTrackIndex == INVALID_TRACK_INDEX)
		return eNM_ERRNO_CHUNK;
	
	if (u32ChunkID >= psMP4ReadRes->i32VideoChunks)
		return eNM_ERRNO_EOM;

	uint32_t u32FrameSize;
	uint32_t u32Timestamp;
	uint32_t u32Duration;
	MP4D_file_offset_t tFileOffset; 
	
	tFileOffset = MP4D__frame_offset(&psMP4ReadRes->tMP4, psMP4ReadRes->i32VideoTrackIndex, u32ChunkID, (unsigned int *)&u32FrameSize, (unsigned int *)&u32Timestamp, (unsigned int *)&u32Duration);

	*pu32ChunkSize = u32FrameSize;	
	uint64_t u64Timestamp = (uint64_t)u32Timestamp;
	*pu64ChunkTime = (u64Timestamp * 1000) / (psMP4ReadRes->i32VideoTrackScale);
	
	if(psMP4ReadRes->eVideoType == eNM_CTX_VIDEO_H264){
		//Maybe insert SPS/PPS information in MP4Read_ReadAudio(), so enlarge chunk size
		*pu32ChunkSize = u32FrameSize + (4 * 2 + psMP4ReadRes->u32SPSSize + psMP4ReadRes->u32PPSSize);
	}
	
	if(pbSeekable){
		if(psMP4ReadRes->eVideoType == eNM_CTX_VIDEO_H264){
			uint8_t *pu8H264FrameBuf = NULL;
			const uint32_t	u32NALUStartCode = 0x01000000;	// Big-endian
			int32_t i32ReadLen;
			
			pu8H264FrameBuf = (uint8_t *)malloc(u32FrameSize);
			if(pu8H264FrameBuf == NULL)
				return eNM_ERRNO_MALLOC;
			
			lseek(psMP4ReadRes->i32FileNo, tFileOffset,SEEK_SET);
			i32ReadLen = read(psMP4ReadRes->i32FileNo, pu8H264FrameBuf, u32FrameSize);
			
			if(i32ReadLen != u32FrameSize){
				free(pu8H264FrameBuf);
				return eNM_ERRNO_IO;
			}

			//overwrite frame szie feild, using NALU start code instead 
			uint32_t u32Temp = 0;
			uint32_t u32NALSize;
			while(u32Temp < u32FrameSize){
				u32NALSize = (pu8H264FrameBuf[u32Temp + 3])|(pu8H264FrameBuf[u32Temp + 2]  << 8)|(pu8H264FrameBuf[u32Temp + 1] << 16)|(pu8H264FrameBuf[u32Temp] << 24); 
				memcpy(pu8H264FrameBuf + u32Temp, &u32NALUStartCode, 4);
				u32Temp += (u32NALSize + 4);
			}

			S_NM_UTIL_H264_FRAME_INFO sFrameInfo;
			NMUtil_ParseH264Frame(pu8H264FrameBuf, u32FrameSize, &sFrameInfo);

			if (sFrameInfo.eNALType & eNM_UTIL_H264_NAL_SPS || sFrameInfo.eNALType & eNM_UTIL_H264_NAL_I){
				*pbSeekable = true;
			}
			else{
				*pbSeekable = false;
				*pu32ChunkSize = u32FrameSize;
			}
					
			free(pu8H264FrameBuf);
		}
		else{
			*pbSeekable = true;
		}
	}
	
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
MP4Read_ReadAudio(
	uint32_t			u32ChunkID,
	S_NM_AUDIO_CTX	*psCtx,
	void				*pMediaRes
)
{
	S_MP4READ_RES *psMP4ReadRes = (S_MP4READ_RES *)pMediaRes;

	psCtx->u32DataSize = 0;
	
	if(psMP4ReadRes == NULL){
		return eNM_ERRNO_NULL_RES;
	}
	
	if(psMP4ReadRes->i32AudioTrackIndex == INVALID_TRACK_INDEX)
		return eNM_ERRNO_CHUNK;
	
	if (u32ChunkID >= psMP4ReadRes->i32AudioChunks)
		return eNM_ERRNO_EOM;

	uint32_t u32FrameSize;
	uint32_t u32Timestamp;
	uint32_t u32Duration;
	MP4D_file_offset_t tFileOffset;
	int32_t i32Size;
	
	tFileOffset = MP4D__frame_offset(&psMP4ReadRes->tMP4, psMP4ReadRes->i32AudioTrackIndex, u32ChunkID, (unsigned int *)&u32FrameSize, (unsigned int *)&u32Timestamp, (unsigned int *)&u32Duration);

	if (psCtx->u32DataLimit < u32FrameSize)
		return eNM_ERRNO_CTX;

	if (lseek(psMP4ReadRes->i32FileNo, tFileOffset, SEEK_SET) < 0)
		return eNM_ERRNO_IO;

	i32Size = read(psMP4ReadRes->i32FileNo, psCtx->pu8DataBuf, u32FrameSize);

	if (i32Size != u32FrameSize)
		return eNM_ERRNO_IO;
	
	//update datatime
	psCtx->u32DataSize = u32FrameSize;	
	uint64_t u64Timestamp = (uint64_t)u32Timestamp;
	psCtx->u64DataTime = (u64Timestamp * 1000) / (psMP4ReadRes->i32AudioTrackScale);
	
	return eNM_ERRNO_NONE;
}


static E_NM_ERRNO
MP4Read_ReadVideo(
	uint32_t			u32ChunkID,
	S_NM_VIDEO_CTX	*psCtx,
	void				*pMediaRes
)
{
	S_MP4READ_RES *psMP4ReadRes = (S_MP4READ_RES *)pMediaRes;

	psCtx->u32DataSize = 0;
	
	if(psMP4ReadRes == NULL){
		return eNM_ERRNO_NULL_RES;
	}
	
	if(psMP4ReadRes->i32VideoTrackIndex == INVALID_TRACK_INDEX)
		return eNM_ERRNO_CHUNK;
	
	if (u32ChunkID >= psMP4ReadRes->i32VideoChunks)
		return eNM_ERRNO_EOM;

	uint32_t u32FrameSize;
	uint32_t u32Timestamp;
	uint32_t u32Duration;
	MP4D_file_offset_t tFileOffset; 
	uint8_t *pu8DataBuf;
	int32_t i32Size;
	int32_t i32ExtraSize = 0;
	const uint32_t	u32NALUStartCode = 0x01000000;	// Big-endian
	
	tFileOffset = MP4D__frame_offset(&psMP4ReadRes->tMP4, psMP4ReadRes->i32VideoTrackIndex, u32ChunkID, (unsigned int *)&u32FrameSize, (unsigned int *)&u32Timestamp, (unsigned int *)&u32Duration);
		
	if (psCtx->u32DataLimit < u32FrameSize)
		return eNM_ERRNO_CTX;

	if (lseek(psMP4ReadRes->i32FileNo, tFileOffset, SEEK_SET) < 0)
		return eNM_ERRNO_IO;
	
	pu8DataBuf = psCtx->pu8DataBuf;
		
	i32Size = read(psMP4ReadRes->i32FileNo, pu8DataBuf, u32FrameSize);

	if (i32Size != u32FrameSize)
		return eNM_ERRNO_IO;

	psCtx->bKeyFrame = true;
		
	if(psMP4ReadRes->eVideoType == eNM_CTX_VIDEO_H264){
		S_NM_UTIL_H264_FRAME_INFO sFrameInfo;

		//overwrite frame szie feild, using NALU start code instead 
		uint32_t u32Temp = 0;
		uint32_t u32NALSize;
		while(u32Temp < u32FrameSize){
			u32NALSize = (pu8DataBuf[u32Temp + 3])|(pu8DataBuf[u32Temp + 2]  << 8)|(pu8DataBuf[u32Temp + 1] << 16)|(pu8DataBuf[u32Temp] << 24); 
			memcpy(pu8DataBuf + u32Temp, &u32NALUStartCode, 4);
			u32Temp += (u32NALSize + 4);
		}

		memcpy(pu8DataBuf, &u32NALUStartCode , 4);

		NMUtil_ParseH264Frame(pu8DataBuf, u32FrameSize, &sFrameInfo);

		if(sFrameInfo.eNALType & eNM_UTIL_H264_NAL_I){
			i32ExtraSize = (4 * 2 + psMP4ReadRes->u32SPSSize + psMP4ReadRes->u32PPSSize);

			if(psCtx->u32DataLimit < (u32FrameSize + i32ExtraSize)){
				return eNM_ERRNO_CTX;
			}

			memmove(pu8DataBuf + i32ExtraSize, pu8DataBuf, u32FrameSize);
			memcpy(pu8DataBuf, &u32NALUStartCode , 4);
			memcpy(pu8DataBuf + 4, psMP4ReadRes->pu8SPS, psMP4ReadRes->u32SPSSize);
			memcpy(pu8DataBuf + 4 + psMP4ReadRes->u32SPSSize, &u32NALUStartCode , 4);
			memcpy(pu8DataBuf + 8 + psMP4ReadRes->u32SPSSize, psMP4ReadRes->pu8PPS, psMP4ReadRes->u32PPSSize);
			
			psCtx->bKeyFrame = true;
		}
		else{
			psCtx->bKeyFrame = false;
		}
	}
	
	psCtx->u32DataSize = u32FrameSize + i32ExtraSize;	
	//update datatime
	uint64_t u64Timestamp = (uint64_t)u32Timestamp;
	psCtx->u64DataTime = (u64Timestamp * 1000) / (psMP4ReadRes->i32VideoTrackScale);
		
	return eNM_ERRNO_NONE;
}

S_NM_MEDIAREAD_IF g_sMP4ReaderIF = {
	.eMediaType = eNM_MEDIA_MP4,
	.pfnOpenMedia = MP4Read_Open,
	.pfnCloseMedia = MP4Read_Close,

	.pfnGetAudioChunkInfo = MP4Read_GetAudioChunkInfo,
	.pfnGetVideoChunkInfo = MP4Read_GetVideoChunkInfo,

	.pfnReadAudioChunk = MP4Read_ReadAudio,
	.pfnReadVideoChunk = MP4Read_ReadVideo,

};






