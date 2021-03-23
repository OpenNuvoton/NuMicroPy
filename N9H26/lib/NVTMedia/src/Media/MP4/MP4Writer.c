/* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
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

#include "minimp4_mux.h"

typedef struct _S_MP4WRITE_RES {
	int32_t					i32FileNo;
	MP4E_mux_t  		*ptMP4Mux;
	mp4_h26x_writer_t	sMP4H264Writer;

	int32_t i32AudioTrackID;
	
	uint64_t u64LastVideoDataTime;
	uint64_t u64LastAudioDataTime;

	uint64_t u64StartAVDataTime;
	
	bool bWriteSPSPPS;
}S_MP4WRITE_RES;

#define MP4E_SEQ_MODE (0)			//mp4 mux use sequential mode
#define MP4E_FRAG_MODE (0)		//mp4 mux use fragmentation mode

#define MAX_AACSRI	12

static const int32_t s_aiAACSRI[MAX_AACSRI] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025,  8000
};

//Get AAC sample rate index
static int
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
		return 0;
	}
	return -1;
}

static int 
MP4E_WriteCB(
	int64_t i64Offset, 
	const void *pvBuffer, 
	size_t i32Size, 
	void *pvToken
)
{
	int i32FileNo = (int)pvToken;

	lseek(i32FileNo, i64Offset, SEEK_SET);
    
	return write(i32FileNo, pvBuffer, i32Size) != i32Size;
}

static int
AddVideoTrack(
	S_NM_VIDEO_CTX *psVideoCtx,
	S_MP4WRITE_RES *psMediaRes
)
{
	int i32Ret;
	
	if(psVideoCtx->eVideoType == eNM_CTX_VIDEO_H264){
		i32Ret = mp4_h26x_write_init(&psMediaRes->sMP4H264Writer, psMediaRes->ptMP4Mux, psVideoCtx->u32Width, psVideoCtx->u32Height, 0);
		if(i32Ret != MP4E_STATUS_OK)
			return -1;
	}
	
	return 0;
}

static int
AddAudioTrack(
	S_NM_AUDIO_CTX *psAudioCtx,
	S_MP4WRITE_RES *psMediaRes
)
{
  MP4E_track_t sAudioTrack;
	
	if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_NONE){
		return 0;
	}

	sAudioTrack.track_media_kind = e_audio;
	sAudioTrack.language[0] = 'u';
	sAudioTrack.language[1] = 'n';
	sAudioTrack.language[2] = 'd';
	sAudioTrack.language[3] = 0;

	if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_AAC)
		sAudioTrack.object_type_indication = MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3;
	else if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_MP3)
		sAudioTrack.object_type_indication = MP4_OBJECT_TYPE_AUDIO_ISO_IEC_11172_3;
		
	sAudioTrack.time_scale = psAudioCtx->u32SampleRate;
	sAudioTrack.default_duration = 0;
	sAudioTrack.u.a.channelcount = psAudioCtx->u32Channel;
	
	psMediaRes->i32AudioTrackID =  MP4E_add_track(psMediaRes->ptMP4Mux, &sAudioTrack);
	if(psMediaRes->i32AudioTrackID < 0){
		//ERROR to add track
		return psMediaRes->i32AudioTrackID;
	}
	
	if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_AAC){
		uint8_t au8AACDecConf[5];
		int32_t i32SRI = 0;
		uint16_t u16SyncType;

		GetAACSRI(psAudioCtx->u32SampleRate, &i32SRI);

		au8AACDecConf[0] = 2 << 3;					//profile id: 2 (AAC-LC)
		au8AACDecConf[0] |= (i32SRI & 0x0000000F) >> 1;	//sample rate index
		au8AACDecConf[1] = (i32SRI & 0x0000000F) << 7;		//sample rate index
		au8AACDecConf[1] |= (psAudioCtx->u32Channel & 0x0000000F) << 3;			//channels

		u16SyncType = 0x02b7 << 5 | 5;		//5: audio object type SBR
		
		au8AACDecConf[2] = (uint8_t)((u16SyncType & 0xFF00) >> 8);
		au8AACDecConf[3] = (uint8_t)(u16SyncType & 0x00FF);
		au8AACDecConf[4] = 0;
		
		MP4E_set_dsi(psMediaRes->ptMP4Mux, psMediaRes->i32AudioTrackID, au8AACDecConf, 5);
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////

static E_NM_ERRNO
MP4Write_Open(
	S_NM_VIDEO_CTX *psVideoCtx,
	S_NM_AUDIO_CTX *psAudioCtx,
	void *pvMediaAttr,			
	void **ppMediaRes
)
{
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;
	S_NM_MP4_MEDIA_ATTR *psMediaAttr;
	S_MP4WRITE_RES *psMediaRes;
	
	*ppMediaRes = NULL;

	//check parameter
	if((psVideoCtx->eVideoType != eNM_CTX_VIDEO_H264) && (psVideoCtx->eVideoType != eNM_CTX_VIDEO_NONE))
		return eNM_ERRNO_CODEC_TYPE;
	
	if((psAudioCtx->eAudioType != eNM_CTX_AUDIO_AAC) && (psAudioCtx->eAudioType != eNM_CTX_AUDIO_NONE))
		return eNM_ERRNO_CODEC_TYPE;
	
	if((psVideoCtx->eVideoType == eNM_CTX_VIDEO_NONE) && (psAudioCtx->eAudioType == eNM_CTX_AUDIO_NONE))
		return eNM_ERRNO_CODEC_TYPE;

	if(pvMediaAttr == NULL)
		return eNM_ERRNO_BAD_PARAM;

	psMediaAttr = (S_NM_MP4_MEDIA_ATTR *)pvMediaAttr;
	
	//Allocate media resource
	psMediaRes = calloc(1, sizeof(S_MP4WRITE_RES));
	if(psMediaRes == NULL){
		return eNM_ERRNO_MALLOC;
	}
	
	psMediaRes->ptMP4Mux = MP4E_open(MP4E_SEQ_MODE, MP4E_FRAG_MODE, (void *)psMediaAttr->i32FD, MP4E_WriteCB);
	
	if(psMediaRes->ptMP4Mux == NULL){
		eRet = eNM_ERRNO_MEDIA_OPEN;
		goto MP4Write_Open_fail;		
	}
	
	if(AddVideoTrack(psVideoCtx, psMediaRes) != 0){
		eRet = eNM_ERRNO_MEDIA_OPEN;
		goto MP4Write_Open_fail;		
	}
	
	if(AddAudioTrack(psAudioCtx, psMediaRes) != 0){
		eRet = eNM_ERRNO_MEDIA_OPEN;
		goto MP4Write_Open_fail;		
	}
	
	psMediaRes->i32FileNo = psMediaAttr->i32FD;
	*ppMediaRes = psMediaRes;
	return eNM_ERRNO_NONE;
	
MP4Write_Open_fail:

	if(psMediaRes->ptMP4Mux){
			MP4E_close(psMediaRes->ptMP4Mux);
	}

	if(psMediaRes)
		free(psMediaRes);
	
	return eRet;
}

static E_NM_ERRNO
MP4Write_Close(
	void	**ppMediaRes
)
{
	S_MP4WRITE_RES	*psMediaRes;
	
	if (!ppMediaRes || !*ppMediaRes)
		return eNM_ERRNO_NULL_PTR;

	psMediaRes = (S_MP4WRITE_RES *)*ppMediaRes;

	
	MP4E_close(psMediaRes->ptMP4Mux);
	mp4_h26x_write_close(&psMediaRes->sMP4H264Writer);
	
	free(psMediaRes);	
	*ppMediaRes = NULL;
	return eNM_ERRNO_NONE;
}

#define DEF_MP4_VIDEO_TIMESCALE 	90000	// 90kHz tick

static E_NM_ERRNO
MP4Write_WriteVideo(
	uint32_t			u32ChunkID,
	const S_NM_VIDEO_CTX	*psCtx,
	void				*pMediaRes
)
{
	S_MP4WRITE_RES	*psMediaRes;
	int32_t i32Ret;
	
	if (!psCtx || !pMediaRes)
		return eNM_ERRNO_NULL_PTR;

	if (psCtx->u32DataSize ==  0){
			return eNM_ERRNO_NONE;
	}

	psMediaRes = (S_MP4WRITE_RES *)pMediaRes;
	
	uint8_t			*pu8ChunkData = psCtx->pu8DataBuf;
	uint32_t		u32ChunkSize = psCtx->u32DataSize;
	uint64_t	u64Duration = 0;

	if(psCtx->eVideoType == eNM_CTX_VIDEO_H264){
		S_NM_UTIL_H264_FRAME_INFO	sH264FrameInfo;

		NMUtil_ParseH264Frame(pu8ChunkData, u32ChunkSize, &sH264FrameInfo);

		if(psMediaRes->u64LastVideoDataTime == 0){
			if(!(sH264FrameInfo.eNALType & eNM_UTIL_H264_NAL_I)){
				// First video chunk must I frame
				return eNM_ERRNO_NONE;
			}

			if(psMediaRes->u64StartAVDataTime == 0){
				psMediaRes->u64StartAVDataTime = psCtx->u64DataTime;
			}
			else if(psCtx->u64DataTime < psMediaRes->u64StartAVDataTime){
				psMediaRes->u64StartAVDataTime = psCtx->u64DataTime;
			}
			
			u64Duration = (psCtx->u64DataTime - psMediaRes->u64StartAVDataTime) * DEF_MP4_VIDEO_TIMESCALE / 1000; 
		}
		else{
			u64Duration = (psCtx->u64DataTime - psMediaRes->u64LastVideoDataTime) * DEF_MP4_VIDEO_TIMESCALE / 1000;
		}
		
		if(psMediaRes->bWriteSPSPPS == false){
			if(sH264FrameInfo.eNALType & eNM_UTIL_H264_NAL_SPS){
				i32Ret = mp4_h26x_write_nal(&psMediaRes->sMP4H264Writer, pu8ChunkData + sH264FrameInfo.u32SPSOffset, sH264FrameInfo.u32SPSLen, (unsigned)u64Duration);
				if(i32Ret != MP4E_STATUS_OK)
					return eNM_ERRNO_IO;
			}

			if(sH264FrameInfo.eNALType & eNM_UTIL_H264_NAL_PPS){
				i32Ret = mp4_h26x_write_nal(&psMediaRes->sMP4H264Writer, pu8ChunkData + sH264FrameInfo.u32PPSOffset, sH264FrameInfo.u32PPSLen, (unsigned)u64Duration);
				if(i32Ret != MP4E_STATUS_OK)
					return eNM_ERRNO_IO;
			}
			psMediaRes->bWriteSPSPPS = true;
		}
		
		if(sH264FrameInfo.eNALType & (eNM_UTIL_H264_NAL_I | eNM_UTIL_H264_NAL_P)){
			i32Ret = mp4_h26x_write_nal(&psMediaRes->sMP4H264Writer, pu8ChunkData + sH264FrameInfo.u32IPOffset, sH264FrameInfo.u32IPLen, (unsigned)u64Duration);
			if(i32Ret != MP4E_STATUS_OK)
				return eNM_ERRNO_IO;
		}
				
		psMediaRes->u64LastVideoDataTime = psCtx->u64DataTime; 
	}

	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
MP4Write_WriteAudio(
	uint32_t			u32ChunkID,
	const S_NM_AUDIO_CTX	*psCtx,
	void				*pMediaRes
)
{
	S_MP4WRITE_RES	*psMediaRes;
	int32_t i32Ret;
	uint64_t	u64Duration = 0;
	
	if (!psCtx || !pMediaRes)
		return eNM_ERRNO_NULL_PTR;

	if (psCtx->u32DataSize && !psCtx->pu8DataBuf)
		return eNM_ERRNO_NULL_PTR;

	psMediaRes = (S_MP4WRITE_RES *)pMediaRes;

	if(psMediaRes->u64LastAudioDataTime == 0){

		if(psMediaRes->u64StartAVDataTime == 0){
			psMediaRes->u64StartAVDataTime = psCtx->u64DataTime;
		}
		else if(psCtx->u64DataTime < psMediaRes->u64StartAVDataTime){
			psMediaRes->u64StartAVDataTime = psCtx->u64DataTime;
		}

		u64Duration = (psCtx->u64DataTime - psMediaRes->u64StartAVDataTime) *  psCtx->u32SampleRate / 1000;
	}
	else{
		u64Duration = (psCtx->u64DataTime - psMediaRes->u64LastAudioDataTime) * psCtx->u32SampleRate / 1000;
	}
		
	i32Ret = MP4E_put_sample(psMediaRes->ptMP4Mux, psMediaRes->i32AudioTrackID, psCtx->pu8DataBuf, psCtx->u32DataSize, (int)u64Duration, MP4E_SAMPLE_RANDOM_ACCESS);

	if(i32Ret != MP4E_STATUS_OK)
		return eNM_ERRNO_IO;

	psMediaRes->u64LastAudioDataTime = psCtx->u64DataTime; 

	return eNM_ERRNO_NONE;
}

S_NM_MEDIAWRITE_IF g_sMP4WriterIF = {
	.eMediaType = eNM_MEDIA_MP4,

	.pfnOpenMedia = MP4Write_Open,
	.pfnCloseMedia = MP4Write_Close,

	.pfnWriteVideoChunk = MP4Write_WriteVideo,
	.pfnWriteAudioChunk = MP4Write_WriteAudio,
};
