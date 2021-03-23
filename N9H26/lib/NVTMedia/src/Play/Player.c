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

#include "PlayEngine.h"

typedef struct{
	E_NM_MEDIA_FORMAT eMediaFormat;
	int32_t 	i32MediaHandle;
	S_NM_MEDIAREAD_IF *psMediaIF;
	void *pvMediaRes;
	void *pvAudioCtxParam;
}S_NM_PLAY_OEPN_RES;

/*
typedef enum{
	eNM_CTX_AUDIO_UNKNOWN = -1,
	eNM_CTX_AUDIO_NONE,
	eNM_CTX_AUDIO_ALAW,
	eNM_CTX_AUDIO_ULAW,
	eNM_CTX_AUDIO_AAC,	
	eNM_CTX_AUDIO_MP3,	
	eNM_CTX_AUDIO_PCM_L16,		//Raw data format
	eNM_CTX_AUDIO_ADPCM,
	eNM_CTX_AUDIO_G726,	
	eNM_CTX_AUDIO_END,	
}E_NM_CTX_AUDIO_TYPE;
*/

static S_NM_CODECDEC_AUDIO_IF *s_apsAudioDecCodecList[eNM_CTX_AUDIO_END] = {
	NULL,
	&g_sG711DecIF,
	&g_sG711DecIF,
	&g_sAACDecIF,
	NULL,
	NULL,
	NULL,
	NULL,	
};

/*
typedef enum{
	eNM_CTX_VIDEO_UNKNOWN = -1,
	eNM_CTX_VIDEO_NONE,
	eNM_CTX_VIDEO_MJPG,
	eNM_CTX_VIDEO_H264,
	eNM_CTX_VIDEO_YUV422,		//Raw data format
	eNM_CTX_VIDEO_YUV422P,		
	eNM_CTX_VIDEO_END,		
}E_NM_CTX_VIDEO_TYPE;
*/

static S_NM_CODECDEC_VIDEO_IF *s_apsVideoDecCodecList[eNM_CTX_VIDEO_END] = {
	NULL,
	NULL,
	&g_sH264DecIF,
	NULL,
	NULL,
};


static E_NM_ERRNO
OpenMP4ReadMedia(
	S_NM_MP4_MEDIA_ATTR *psMP4Attr,
	S_NM_PLAY_IF *psPlayIF,
	S_NM_PLAY_CONTEXT *psPlayCtx,
	S_NM_PLAY_INFO *psPlayInfo,
	S_NM_PLAY_OEPN_RES *psOpenRes
)
{
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;

	eRet = g_sMP4ReaderIF.pfnOpenMedia(&psPlayCtx->sMediaVideoCtx, &psPlayCtx->sMediaAudioCtx, psMP4Attr, &psPlayIF->pvMediaRes);

	if(eRet != eNM_ERRNO_NONE)
		return eRet;

	psPlayIF->psMediaIF = &g_sMP4ReaderIF;	

	if(psPlayCtx->sMediaVideoCtx.eVideoType > eNM_CTX_VIDEO_NONE)
		psPlayIF->psVideoCodecIF = s_apsVideoDecCodecList[psPlayCtx->sMediaVideoCtx.eVideoType];
	if(psPlayCtx->sMediaAudioCtx.eAudioType > eNM_CTX_AUDIO_NONE)
		psPlayIF->psAudioCodecIF = s_apsAudioDecCodecList[psPlayCtx->sMediaAudioCtx.eAudioType];

	if(psOpenRes){
		psOpenRes->eMediaFormat = eNM_MEDIA_FORMAT_FILE;
		psOpenRes->i32MediaHandle = psMP4Attr->i32FD;		
		psOpenRes->psMediaIF = psPlayIF->psMediaIF; 
		psOpenRes->pvMediaRes = psPlayIF->pvMediaRes; 
		
		if(psPlayCtx->sMediaAudioCtx.eAudioType == eNM_CTX_AUDIO_AAC){
			if(psMP4Attr->eAACProfile != eNM_AAC_UNKNOWN){

				S_NM_AAC_CTX_PARAM *psCtxParam = calloc(1, sizeof(S_NM_AAC_CTX_PARAM));
				if(psCtxParam == NULL){
					eRet = eNM_ERRNO_MALLOC;
					goto OpenMP4ReadMedia_fail;
				}
				
				psCtxParam->eProfile = psMP4Attr->eAACProfile;
				psPlayCtx->sMediaAudioCtx.pvParamSet = psCtxParam;
				psOpenRes->pvAudioCtxParam = psCtxParam;
			}
		}
	}

	if(psPlayInfo){
		psPlayInfo->eMediaFormat = eNM_MEDIA_FORMAT_FILE;
		psPlayInfo->i32MediaHandle = psMP4Attr->i32FD;		
		psPlayInfo->u32Duration = (uint32_t)psMP4Attr->u64Duration;
		psPlayInfo->u32VideoChunks = psMP4Attr->u32VideoChunks;
		psPlayInfo->u32AudioChunks = psMP4Attr->u32AudioChunks;
		psPlayInfo->eVideoType = psPlayCtx->sMediaVideoCtx.eVideoType;
		psPlayInfo->eAudioType = psPlayCtx->sMediaAudioCtx.eAudioType;

	}
	
	return eNM_ERRNO_NONE;
	
OpenMP4ReadMedia_fail:
	
	if(psOpenRes){
		if(psOpenRes->pvAudioCtxParam)
			free(psOpenRes->pvAudioCtxParam);
	}

	g_sMP4ReaderIF.pfnCloseMedia(&psPlayIF->pvMediaRes);

	return eRet;
}


static E_NM_ERRNO
OpenAVIReadMedia(
	S_NM_AVI_MEDIA_ATTR *psAVIAttr,
	S_NM_PLAY_IF *psPlayIF,
	S_NM_PLAY_CONTEXT *psPlayCtx,
	S_NM_PLAY_INFO *psPlayInfo,
	S_NM_PLAY_OEPN_RES *psOpenRes
)
{
	E_NM_ERRNO eRet = eNM_ERRNO_NONE;

	eRet = g_sAVIReaderIF.pfnOpenMedia(&psPlayCtx->sMediaVideoCtx, &psPlayCtx->sMediaAudioCtx, psAVIAttr, &psPlayIF->pvMediaRes);

	if(eRet != eNM_ERRNO_NONE)
		return eRet;
	
	psPlayIF->psMediaIF = &g_sAVIReaderIF;	

	if(psPlayCtx->sMediaVideoCtx.eVideoType > eNM_CTX_VIDEO_NONE)
		psPlayIF->psVideoCodecIF = s_apsVideoDecCodecList[psPlayCtx->sMediaVideoCtx.eVideoType];
	if(psPlayCtx->sMediaAudioCtx.eAudioType > eNM_CTX_AUDIO_NONE)
		psPlayIF->psAudioCodecIF = s_apsAudioDecCodecList[psPlayCtx->sMediaAudioCtx.eAudioType];

	if(psOpenRes){
		psOpenRes->eMediaFormat = eNM_MEDIA_FORMAT_FILE;
		psOpenRes->i32MediaHandle = psAVIAttr->i32FD;		
		psOpenRes->psMediaIF = psPlayIF->psMediaIF; 
		psOpenRes->pvMediaRes = psPlayIF->pvMediaRes; 
		
		if(psPlayCtx->sMediaAudioCtx.eAudioType == eNM_CTX_AUDIO_AAC){
			if(psAVIAttr->eAACProfile != eNM_AAC_UNKNOWN){
				S_NM_AAC_CTX_PARAM *psCtxParam = calloc(1, sizeof(S_NM_AAC_CTX_PARAM));
				if(psCtxParam == NULL){
					eRet = eNM_ERRNO_MALLOC;
					goto OpenAVIReadMedia_fail;
				}
			
				psCtxParam->eProfile = psAVIAttr->eAACProfile;
				psPlayCtx->sMediaAudioCtx.pvParamSet = psCtxParam;
				psOpenRes->pvAudioCtxParam = psCtxParam;
			}
		}

	}
	
	if(psPlayInfo){
		psPlayInfo->eMediaFormat = eNM_MEDIA_FORMAT_FILE;
		psPlayInfo->i32MediaHandle = psAVIAttr->i32FD;		
		psPlayInfo->u32Duration = psAVIAttr->u32Duration;
		psPlayInfo->u32VideoChunks = psAVIAttr->u32VideoChunks;
		psPlayInfo->u32AudioChunks = psAVIAttr->u32AudioChunks;
		psPlayInfo->eVideoType = psPlayCtx->sMediaVideoCtx.eVideoType;
		psPlayInfo->eAudioType = psPlayCtx->sMediaAudioCtx.eAudioType;

	}
	
	return eNM_ERRNO_NONE;

OpenAVIReadMedia_fail:
	
	if(psOpenRes){
		if(psOpenRes->pvAudioCtxParam)
			free(psOpenRes->pvAudioCtxParam);
	}

	g_sAVIReaderIF.pfnCloseMedia(&psPlayIF->pvMediaRes);

	return eRet;
}


////////////////////////////////////////////////////////////////////////////////////////////////

/**	@brief	Open file/url for playback
 *
 *	@param	szPath[in]		file or url path for playback
 *	@param	psPlayIF[out]	give suggestion on media and codec interface 
 *	@param	psPlayCtx[out]	report media video/audio context
 *	@param	psPlayInfo[out]	report media information
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMPlay_Open(
	char *szPath,
	S_NM_PLAY_IF *psPlayIF,
	S_NM_PLAY_CONTEXT *psPlayCtx,
	S_NM_PLAY_INFO *psPlayInfo,
	void **ppvNMOpenRes
)
{
	E_NM_ERRNO eNMRet;
	int hFile = 0;
	struct stat sFileState;
	S_NM_PLAY_OEPN_RES *psOpenRes = NULL;
	
	*ppvNMOpenRes = NULL;
	
	if((szPath == NULL) || (psPlayIF == NULL) || (psPlayCtx == NULL)){
		return eNM_ERRNO_NULL_PTR;
	}

	psOpenRes = calloc(1, sizeof(S_NM_PLAY_OEPN_RES));
	
	memset(psPlayIF, 0, sizeof(S_NM_PLAY_IF));
	memset(psPlayCtx, 0, sizeof(S_NM_PLAY_CONTEXT));
	
	if(psPlayInfo){
		memset(psPlayInfo, 0, sizeof(S_NM_PLAY_INFO));
	}

	if(stat(szPath, &sFileState) < 0){
			NMLOG_ERROR("Unable stat media file %s \n", szPath);
			eNMRet  = eNM_ERRNO_MEDIA_OPEN;
			goto NMPlay_Open_fail;
	}
	
	if(strstr(szPath, ".avi") != NULL){
		S_NM_AVI_MEDIA_ATTR sAVIAttr;

		hFile = open(szPath, O_RDONLY);
		if(hFile < 0){
			NMLOG_ERROR("Unable open AVI file %s \n", szPath);
			eNMRet  = eNM_ERRNO_MEDIA_OPEN;
			goto NMPlay_Open_fail;
		}

		memset(&sAVIAttr, 0 , sizeof(S_NM_AVI_MEDIA_ATTR));
		sAVIAttr.i32FD = hFile;
		sAVIAttr.u32MediaSize = (uint32_t)sFileState.st_size;
		
		eNMRet = OpenAVIReadMedia(&sAVIAttr, psPlayIF, psPlayCtx, psPlayInfo, psOpenRes);

		if(eNMRet != eNM_ERRNO_NONE)
			goto NMPlay_Open_fail;
	}
	else if(strstr(szPath, ".mp4") != NULL){
		S_NM_MP4_MEDIA_ATTR sMP4Attr;

		hFile = open(szPath, O_RDONLY);
		if(hFile < 0){
			NMLOG_ERROR("Unable open MP4 file %s \n", szPath);
			eNMRet  = eNM_ERRNO_MEDIA_OPEN;
			goto NMPlay_Open_fail;
		}

		memset(&sMP4Attr, 0 , sizeof(S_NM_MP4_MEDIA_ATTR));
		sMP4Attr.i32FD = hFile;
		sMP4Attr.u32MediaSize = (uint32_t)sFileState.st_size;
		
		eNMRet = OpenMP4ReadMedia(&sMP4Attr, psPlayIF, psPlayCtx, psPlayInfo, psOpenRes);

		if(eNMRet != eNM_ERRNO_NONE)
			goto NMPlay_Open_fail;
	}
	
		
	*ppvNMOpenRes = psOpenRes;

	return eNM_ERRNO_NONE;

NMPlay_Open_fail:

	if(psOpenRes)
		free(psOpenRes);

	if(hFile > 0)
		close(hFile);
	
	return eNMRet;
}


/**	@brief	Open file/url for playback
 *	@param	phPlay [in/out] Play engine handle
 *	@param	psPlayIF[in]	give suggestion on media and codec interface 
 *	@param	psPlayCtx[out]	report media video/audio context
 *  @usage  For start play, *phPlay must equal eNM_INVALID_HANDLE, psPlayIF and psPlayCtx required
 *  @usage  For continue play, *phPlay != eNM_INVALID_HANDLE, psPlayIF and psPlayCtx must NULL
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMPlay_Play(
	HPLAY *phPlay,
	S_NM_PLAY_IF *psPlayIF,
	S_NM_PLAY_CONTEXT *psPlayCtx,
	bool bWait
)
{
	bool bCreatePlayEngine = false;
	
	if(phPlay == NULL)
		return eNM_ERRNO_NULL_PTR;

	if(*phPlay == (HPLAY)eNM_INVALID_HANDLE){
		if((psPlayIF == NULL) || (psPlayCtx == NULL))
			return eNM_ERRNO_NULL_PTR;
		bCreatePlayEngine = true;
	}

	//Start play
	if(bCreatePlayEngine){
		return PlayEngine_Create(phPlay, psPlayIF, psPlayCtx);
	}
	
	//Continue play
	return PlayEngine_Play(*phPlay, bWait);
}

E_NM_ERRNO
NMPlay_Close(
	HPLAY hPlay,
	void **ppvNMOpenRes
)
{
	S_NM_PLAY_OEPN_RES *psOpenRes = NULL;

	if(hPlay != (HPLAY)eNM_INVALID_HANDLE){
		PlayEngine_Destroy(hPlay);
	}
	
	if((ppvNMOpenRes == NULL) || (*ppvNMOpenRes == NULL))
		return eNM_ERRNO_NONE;
		
	psOpenRes = (S_NM_PLAY_OEPN_RES *)*ppvNMOpenRes;
	
	//release context param
	if(psOpenRes->pvAudioCtxParam)
		free(psOpenRes->pvAudioCtxParam);

	//close media
	if(psOpenRes->pvMediaRes){
		psOpenRes->psMediaIF->pfnCloseMedia(&psOpenRes->pvMediaRes);
	}

	//Add S_NM_PLAY_OEPN_RES argument to close file/stream
	if((psOpenRes->i32MediaHandle) && (psOpenRes->eMediaFormat == eNM_MEDIA_FORMAT_FILE)){
		close(psOpenRes->i32MediaHandle);
	}
	
	free(psOpenRes);
	*ppvNMOpenRes = NULL;
	
	return eNM_ERRNO_NONE;
}

E_NM_ERRNO
NMPlay_Pause(
	HPLAY hPlay,
	bool bWait
)
{
	if(hPlay == (HPLAY)eNM_INVALID_HANDLE)
		return eNM_ERRNO_NONE;

	return PlayEngine_Pause(hPlay, bWait);
}

E_NM_ERRNO
NMPlay_Fastforward(
	HPLAY hPlay,
	E_NM_PLAY_FASTFORWARD_SPEED eSpeed,
	bool bWait
)
{
	if(hPlay == (HPLAY)eNM_INVALID_HANDLE)
		return eNM_ERRNO_NONE;

	return PlayEngine_Fastforward(hPlay, eSpeed, bWait);
}

E_NM_ERRNO
NMPlay_Seek(
	HPLAY hPlay,
	uint32_t u32MilliSec,
	uint32_t u32TotalVideoChunks,
	uint32_t u32TotalAudioChunks,
	bool bWait
)
{
	if(hPlay == (HPLAY)eNM_INVALID_HANDLE)
		return eNM_ERRNO_NONE;

	return PlayEngine_Seek(hPlay, u32MilliSec, u32TotalVideoChunks, u32TotalAudioChunks, bWait);
}

/**	@brief	Get player status
 *	@param	hPlay [in] Play engine handle
 *	@return	E_NM_PLAY_STATUS
 *
 */

E_NM_PLAY_STATUS
NMPlay_Status(
	HPLAY hPlay
)
{
	if(hPlay == (HPLAY)eNM_INVALID_HANDLE)
		return eNM_PLAY_STATUS_ERROR;

	return PlayEngine_Status(hPlay);
}

