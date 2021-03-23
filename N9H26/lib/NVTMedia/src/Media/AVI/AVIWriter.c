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

#include "AVIUtility.h"

//AVI media writer resource
typedef struct{
	S_AVIUTIL_AVIHANDLE sAVIHandle;
	uint8_t *pu8AVIIdxTableMem;	
	char *szIdxFilePath;
}S_MIF_AVIWRITE_RES;




static E_NM_ERRNO
AVIWrite_Open(
	S_NM_VIDEO_CTX *psVideoCtx,
	S_NM_AUDIO_CTX *psAudioCtx,
	void *pvMediaAttr,			
	void **ppMediaRes
)
{
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;
	S_MIF_AVIWRITE_RES *psMediaRes = NULL;
	S_NM_AVI_MEDIA_ATTR *psMediaAttr = (S_NM_AVI_MEDIA_ATTR *) pvMediaAttr;	
	S_AVIUTIL_FILEATTR sAVIFileAttr;
	uint32_t u32AudioFrameSize = 0;
	ERRCODE eAVIUtilRet = ERR_AVIUTIL_SUCCESS; 
	uint32_t u32AVIIdxTableMemNeed;
	uint8_t *pu8AVIIdxTableMem = NULL;
	
	*ppMediaRes = NULL;
	
	//Allocate media resource
	psMediaRes = malloc(sizeof(S_MIF_AVIWRITE_RES));
	if(psMediaRes == NULL){
		return eNM_ERRNO_MALLOC;
	}
	
	memset(psMediaRes, 0, sizeof(S_MIF_AVIWRITE_RES));
	memset(&sAVIFileAttr, 0, sizeof(S_AVIUTIL_FILEATTR));

	psMediaRes->sAVIHandle.i32FileHandle = psMediaAttr->i32FD;

	if(psMediaAttr->u32Duration == eNM_UNLIMIT_TIME)
		psMediaRes->sAVIHandle.bIdxUseFile = TRUE;
	else
		psMediaRes->sAVIHandle.bIdxUseFile = FALSE;
	
	if((psMediaRes->sAVIHandle.bIdxUseFile == TRUE) && (psMediaAttr->szIdxFilePath == NULL)){
		NMLOG_ERROR("AVI must specify index file path \n");
		eRet = eNM_ERRNO_NULL_PTR;
		goto AVIWrite_Open_fail;
	}

	if(psVideoCtx->u32FrameRate == 0){
		NMLOG_ERROR("AVI unknow video frame rate \n");
		eRet = eNM_ERRNO_SIZE;
		goto AVIWrite_Open_fail;
	}

	if((psAudioCtx->eAudioType != eNM_CTX_AUDIO_NONE) && (psAudioCtx->u32SamplePerBlock == 0)){
		NMLOG_ERROR("AVI unknow audio samples per frame \n");
		eRet = eNM_ERRNO_SIZE;
		goto AVIWrite_Open_fail;
	}
		
	if(psVideoCtx->eVideoType == eNM_CTX_VIDEO_MJPG){
		sAVIFileAttr.u32VideoFcc = MJPEG_FOURCC;
	}
	else if(psVideoCtx->eVideoType == eNM_CTX_VIDEO_H264){
		sAVIFileAttr.u32VideoFcc = H264_FOURCC;
	}
	else if(psVideoCtx->eVideoType != eNM_CTX_VIDEO_NONE){
		NMLOG_ERROR("AVI unknow video type \n");
		eRet = eNM_ERRNO_CODEC_TYPE;
		goto AVIWrite_Open_fail;
	}

	sAVIFileAttr.u32AudioFmtTag = 0;
	
	sAVIFileAttr.u32AudioBitRate = psAudioCtx->u32BitRate;
	
	if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_ULAW) {
		sAVIFileAttr.u32AudioFmtTag = ULAW_FMT_TAG;
		u32AudioFrameSize = psAudioCtx->u32SamplePerBlock * psAudioCtx->u32Channel;
	}
	else if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_ALAW){
		sAVIFileAttr.u32AudioFmtTag = ALAW_FMT_TAG;
		u32AudioFrameSize = psAudioCtx->u32SamplePerBlock * psAudioCtx->u32Channel;
	}
	else if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_PCM_L16){
		sAVIFileAttr.u32AudioFmtTag = PCM_FMT_TAG;
		u32AudioFrameSize = psAudioCtx->u32SamplePerBlock * psAudioCtx->u32Channel *2;
	}
	else if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_MP3){
		sAVIFileAttr.u32AudioFmtTag = MP3_FMT_TAG;
		sAVIFileAttr.u32AudioBitRate = psAudioCtx->u32BitRate;
	}
	else if(psAudioCtx->eAudioType == eNM_CTX_AUDIO_AAC){
		sAVIFileAttr.u32AudioFmtTag = AAC_FMT_TAG;

		if(psAudioCtx->u32BitRate == 0){
			sAVIFileAttr.u32AudioBitRate = 64000;
		}
	}
	else if(psAudioCtx->eAudioType != eNM_CTX_AUDIO_NONE){
		NMLOG_ERROR("AVI unknow audio type \n");
		eRet = eNM_ERRNO_CODEC_TYPE;
		goto AVIWrite_Open_fail;
	}


	//Get this info from video context
	sAVIFileAttr.u32FrameWidth = psVideoCtx->u32Width;
	sAVIFileAttr.u32FrameHeight = psVideoCtx->u32Height;
	sAVIFileAttr.u32FrameRate = psVideoCtx->u32FrameRate;

	//Get this info from audio context
	sAVIFileAttr.u32SamplingRate = psAudioCtx->u32SampleRate;
	sAVIFileAttr.u32AudioChannel = psAudioCtx->u32Channel;
	sAVIFileAttr.u32AudioFrameSize = u32AudioFrameSize;
	sAVIFileAttr.u32AudioSamplesPerFrame = psAudioCtx->u32SamplePerBlock;
	sAVIFileAttr.u32Reserved = 0;
	
	eAVIUtilRet = AVIUtility_CreateAVIFile(&psMediaRes->sAVIHandle, &sAVIFileAttr, psMediaAttr->szIdxFilePath);
	if(eAVIUtilRet != ERR_AVIUTIL_SUCCESS){
		NMLOG_ERROR("AVI unknow create AVI media, error code %d\n", eAVIUtilRet);
		eRet = eNM_ERRNO_MEDIA_OPEN;
		goto AVIWrite_Open_fail;
	}
	

	if(psMediaRes->sAVIHandle.bIdxUseFile == FALSE){
		//Allocate AVI index table memory
		u32AVIIdxTableMemNeed = AVIUtility_WriteMemoryNeed(&psMediaRes->sAVIHandle, ((psMediaAttr->u32Duration + 999) / 1000));
		pu8AVIIdxTableMem = malloc(u32AVIIdxTableMemNeed);

		if(pu8AVIIdxTableMem == NULL){
			NMLOG_ERROR("AVI unable allocate AVI index memory \n");
			eRet = eNM_ERRNO_MALLOC;
			goto AVIWrite_Open_fail;
		}

		AVIUtility_WriteConfigMemory(&psMediaRes->sAVIHandle, (uint32_t *)pu8AVIIdxTableMem);
	}
	else{
		pu8AVIIdxTableMem = malloc((sizeof(S_AVIFMT_AVIINDEXENTRY) * 4096) + 3);

		if(pu8AVIIdxTableMem == NULL){
			NMLOG_ERROR("AVI unable allocate AVI index memory \n");
			eRet = eNM_ERRNO_MALLOC;
			goto AVIWrite_Open_fail;
		}

		AVIUtility_WriteConfigMemory(&psMediaRes->sAVIHandle, (uint32_t *)pu8AVIIdxTableMem);

		psMediaRes->szIdxFilePath = NMUtil_strdup(psMediaAttr->szIdxFilePath);
		if(psMediaRes->szIdxFilePath == NULL){
			NMLOG_ERROR("AVI unable allocate index file path memory \n");
			eRet = eNM_ERRNO_MALLOC;
			goto AVIWrite_Open_fail;
		}			
	}

	psMediaRes->pu8AVIIdxTableMem = pu8AVIIdxTableMem;
	*ppMediaRes = psMediaRes;
	return eNM_ERRNO_NONE;

AVIWrite_Open_fail:

	if(pu8AVIIdxTableMem){
		free(pu8AVIIdxTableMem);
	}
	
	if(psMediaRes->szIdxFilePath){
		free(psMediaRes->szIdxFilePath);
	}
	
	if(psMediaRes){
		free(psMediaRes);
	}
	return eRet;
}

static E_NM_ERRNO
AVIWrite_Close(
	void	**ppMediaRes
)
{
	S_MIF_AVIWRITE_RES	*psMediaRes;
	
	if (!ppMediaRes || !*ppMediaRes)
		return eNM_ERRNO_NULL_PTR;

	psMediaRes = (S_MIF_AVIWRITE_RES *)*ppMediaRes;

	AVIUtility_FinalizeAVIFile(&psMediaRes->sAVIHandle, psMediaRes->szIdxFilePath);

	if(psMediaRes->pu8AVIIdxTableMem)
		free(psMediaRes->pu8AVIIdxTableMem);
	
	if(psMediaRes->szIdxFilePath)
		free(psMediaRes->szIdxFilePath);
	
	free(psMediaRes);
	
	*ppMediaRes = NULL;
	
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
AVIWrite_WriteVideo(
	uint32_t			u32ChunkID,
	const S_NM_VIDEO_CTX	*psCtx,
	void				*pMediaRes
)
{
	S_MIF_AVIWRITE_RES	*psMediaRes;
	S_AVIUTIL_AVIHANDLE *psAVIHandle = NULL;
	
	if (!psCtx || !pMediaRes)
		return eNM_ERRNO_NULL_PTR;

	if (psCtx->u32DataSize > 0){
		if (psCtx->pu8DataBuf == NULL)
			return eNM_ERRNO_NULL_PTR;
	}

	psMediaRes = (S_MIF_AVIWRITE_RES *)pMediaRes;
	psAVIHandle = &psMediaRes->sAVIHandle;
	
	if(psAVIHandle->sAVICtx.u32AudioFmtTag == 0){
		if(psCtx->u32DataSize == 0)
			return eNM_ERRNO_NONE;
	}

	if(AVIUtility_AddVideoFrame(psAVIHandle, psCtx->pu8DataBuf, psCtx->u32DataSize) == 0)
		return eNM_ERRNO_IO;

	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
AVIWrite_WriteAudio(
	uint32_t			u32ChunkID,
	const S_NM_AUDIO_CTX	*psCtx,
	void				*pMediaRes
)
{
	S_MIF_AVIWRITE_RES	*psMediaRes;
	S_AVIUTIL_AVIHANDLE *psAVIHandle = NULL;
	
	if (!psCtx || !pMediaRes)
		return eNM_ERRNO_NULL_PTR;

	if (psCtx->u32DataSize > 0){
		if (psCtx->pu8DataBuf == NULL)
			return eNM_ERRNO_NULL_PTR;
	}

	psMediaRes = (S_MIF_AVIWRITE_RES *)pMediaRes;
	psAVIHandle = &psMediaRes->sAVIHandle;

	if(AVIUtility_AddAudioFrame(psAVIHandle, psCtx->pu8DataBuf, psCtx->u32DataSize) == 0)
		return eNM_ERRNO_IO;

	return eNM_ERRNO_NONE;
}


S_NM_MEDIAWRITE_IF g_sAVIWriterIF = {
	.eMediaType = eNM_MEDIA_AVI,

	.pfnOpenMedia = AVIWrite_Open,
	.pfnCloseMedia = AVIWrite_Close,

	.pfnWriteAudioChunk = AVIWrite_WriteAudio,
	.pfnWriteVideoChunk = AVIWrite_WriteVideo,
};
