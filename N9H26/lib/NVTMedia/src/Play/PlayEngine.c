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

#include "PlayEngine.h"
#include "PlayDecode.h"
#include "PlayDeMux.h"

#include "Util/NMVideoList.h"
#include "Util/NMAudioList.h"

#define _MAX_VIDEO_LIST_DURATION_		3000		//3sec
#define _MAX_AUDIO_LIST_DURATION_		3000		//3sec


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
E_NM_ERRNO
PlayEngine_Create(
	HPLAY *phPlay,
	S_NM_PLAY_IF *psPlayIF,
	S_NM_PLAY_CONTEXT *psPlayCtx
)
{
	S_PLAY_ENGINE_RES *psPlayEngineRes = NULL;
	E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
	bool bDecodeResInit = false;
	bool bDeMuxResInit = false;
	void *pvVideoList = NULL;
	void *pvAudioList = NULL;
	
	psPlayEngineRes = calloc(sizeof(S_PLAY_ENGINE_RES), 1);
	if(psPlayEngineRes == NULL)
		return eNM_ERRNO_MALLOC;
	
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes = &psPlayEngineRes->sDecodeRes.sVideoDecodeRes;
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes = &psPlayEngineRes->sDecodeRes.sAudioDecodeRes;
	
	//Fill codec and context flush interface
	psVideoDecodeRes->psVideoCodecIF = psPlayIF->psVideoCodecIF;
	psAudioDecodeRes->psAudioCodecIF = psPlayIF->psAudioCodecIF;
	psVideoDecodeRes->pfnVideoFlush = psPlayIF->pfnVideoFlush;
	psAudioDecodeRes->pfnAudioFlush = psPlayIF->pfnAudioFlush;

	//Copy contexts
	memcpy(&psVideoDecodeRes->sFlushVideoCtx, &psPlayCtx->sFlushVideoCtx, sizeof(S_NM_VIDEO_CTX));
	memcpy(&psAudioDecodeRes->sFlushAudioCtx, &psPlayCtx->sFlushAudioCtx, sizeof(S_NM_AUDIO_CTX));
	memcpy(&psVideoDecodeRes->sMediaVideoCtx, &psPlayCtx->sMediaVideoCtx, sizeof(S_NM_VIDEO_CTX));
	memcpy(&psAudioDecodeRes->sMediaAudioCtx, &psPlayCtx->sMediaAudioCtx, sizeof(S_NM_AUDIO_CTX));
	
	//Deocde engine init
	if((eNMRet = PlayDecode_Init(&psPlayEngineRes->sDecodeRes)) == eNM_ERRNO_NONE){
		bDecodeResInit = true;
	}
	else{
		goto PlayEngine_Create_fail;
	}
	
	//Fill media interface
	psPlayEngineRes->sDeMuxRes.psMediaIF = psPlayIF->psMediaIF;
	psPlayEngineRes->sDeMuxRes.pvMediaRes = psPlayIF->pvMediaRes;

	//Copy contexts
	memcpy(&psPlayEngineRes->sDeMuxRes.sMediaVideoCtx, &psPlayCtx->sMediaVideoCtx, sizeof(S_NM_VIDEO_CTX));
	memcpy(&psPlayEngineRes->sDeMuxRes.sMediaAudioCtx, &psPlayCtx->sMediaAudioCtx, sizeof(S_NM_AUDIO_CTX));

	//DeMux engine init
	if((eNMRet = PlayDeMux_Init(&psPlayEngineRes->sDeMuxRes)) == eNM_ERRNO_NONE){
		bDeMuxResInit = true;
	}
	else{
		goto PlayEngine_Create_fail;
	}
	
	//Create video list
	if(psVideoDecodeRes->psVideoCodecIF){
		pvVideoList = NMVideoList_Create(psPlayCtx->sMediaVideoCtx.eVideoType, _MAX_VIDEO_LIST_DURATION_); 
		if(pvVideoList == NULL){
			eNMRet = eNM_ERRNO_NULL_RES;
			goto PlayEngine_Create_fail;
		}
	}
	
	//Create audio list
	if(psAudioDecodeRes->psAudioCodecIF){
		pvAudioList = NMAudioList_Create(psPlayCtx->sMediaAudioCtx.eAudioType, _MAX_AUDIO_LIST_DURATION_); 
		if(pvAudioList == NULL){
			eNMRet = eNM_ERRNO_NULL_RES;
			goto PlayEngine_Create_fail;
		}
	}

	psPlayEngineRes->pvVideoList = pvVideoList;
	psPlayEngineRes->pvAudioList = pvAudioList;
	
	psPlayEngineRes->sDeMuxRes.psPlayEngRes = psPlayEngineRes;
	psPlayEngineRes->sDecodeRes.psPlayEngRes = psPlayEngineRes;
	
	//Create decode thread
	eNMRet = PlayDecode_ThreadCreate(&psPlayEngineRes->sDecodeRes);
	if(eNMRet != eNM_ERRNO_NONE){
		goto PlayEngine_Create_fail;
	}
	
	//create deMux thread
	eNMRet = PlayDeMux_ThreadCreate(&psPlayEngineRes->sDeMuxRes);
	if(eNMRet != eNM_ERRNO_NONE){
		goto PlayEngine_Create_fail;
	}


	PlayDecode_Run(&psPlayEngineRes->sDecodeRes);
	PlayDeMux_Play(&psPlayEngineRes->sDeMuxRes, true);
	

	*phPlay = (HPLAY)psPlayEngineRes;
	return eNM_ERRNO_NONE;

PlayEngine_Create_fail:

	PlayDeMux_ThreadDestroy(&psPlayEngineRes->sDeMuxRes);

	PlayDecode_ThreadDestroy(&psPlayEngineRes->sDecodeRes);
	
	if(pvVideoList)
		NMVideoList_Destroy(&pvVideoList);
	
	if(pvAudioList)
		NMAudioList_Destroy(&pvAudioList);
	
	if(bDeMuxResInit == true)
		PlayDeMux_Final(&psPlayEngineRes->sDeMuxRes);
	
	if(bDecodeResInit == true)
		PlayDecode_Final(&psPlayEngineRes->sDecodeRes);
	
	
	if(psPlayEngineRes)
		free(psPlayEngineRes);
	
	return eNMRet;
}

E_NM_ERRNO
PlayEngine_Destroy(
	HPLAY hPlay
)
{
	S_PLAY_ENGINE_RES *psPlayEngineRes = (S_PLAY_ENGINE_RES *)hPlay;

	if(psPlayEngineRes == NULL)
		return eNM_ERRNO_NULL_RES;

	PlayDeMux_Pause(&psPlayEngineRes->sDeMuxRes, true);
	
	PlayDecode_Stop(&psPlayEngineRes->sDecodeRes);

	//Destroy decode thread
	PlayDecode_ThreadDestroy(&psPlayEngineRes->sDecodeRes);
	
	//Destroy demux thread
	PlayDeMux_ThreadDestroy(&psPlayEngineRes->sDeMuxRes);
	
	// Final deMux thread
	PlayDeMux_Final(&psPlayEngineRes->sDeMuxRes);

	// Final decode thrad
	PlayDecode_Final(&psPlayEngineRes->sDecodeRes);

	//Free resource
	if(psPlayEngineRes->pvVideoList)
		NMVideoList_Destroy(&psPlayEngineRes->pvVideoList);
	
	if(psPlayEngineRes->pvAudioList)
		NMAudioList_Destroy(&psPlayEngineRes->pvAudioList);
	
	free(psPlayEngineRes);
	return eNM_ERRNO_NONE;
}

E_NM_ERRNO
PlayEngine_Play(
	HPLAY hPlay,
	bool bWait
)
{
	S_PLAY_ENGINE_RES *psPlayEngineRes = (S_PLAY_ENGINE_RES *)hPlay;

	if(psPlayEngineRes == NULL)
		return eNM_ERRNO_NULL_RES;

	return PlayDeMux_Play(&psPlayEngineRes->sDeMuxRes, bWait);
}

E_NM_ERRNO
PlayEngine_Pause(
	HPLAY hPlay,
	bool bWait
)
{
	S_PLAY_ENGINE_RES *psPlayEngineRes = (S_PLAY_ENGINE_RES *)hPlay;

	if(psPlayEngineRes == NULL)
		return eNM_ERRNO_NULL_RES;

	return PlayDeMux_Pause(&psPlayEngineRes->sDeMuxRes, bWait);
}

E_NM_ERRNO
PlayEngine_Fastforward(
	HPLAY hPlay,
	E_NM_PLAY_FASTFORWARD_SPEED eSpeed,
	bool bWait
)
{
	S_PLAY_ENGINE_RES *psPlayEngineRes = (S_PLAY_ENGINE_RES *)hPlay;

	if(psPlayEngineRes == NULL)
		return eNM_ERRNO_NULL_RES;

	return PlayDeMux_Fastforward(&psPlayEngineRes->sDeMuxRes, eSpeed, bWait);
}

E_NM_ERRNO
PlayEngine_Seek(
	HPLAY hPlay,
	uint32_t u32MilliSec,
	uint32_t u32TotalVideoChunks,
	uint32_t u32TotalAudioChunks,
	bool bWait
)
{
	S_PLAY_ENGINE_RES *psPlayEngineRes = (S_PLAY_ENGINE_RES *)hPlay;

	if(psPlayEngineRes == NULL)
		return eNM_ERRNO_NULL_RES;

	return PlayDeMux_Seek(&psPlayEngineRes->sDeMuxRes, u32MilliSec, u32TotalVideoChunks, u32TotalAudioChunks, bWait);
}


E_NM_PLAY_STATUS
PlayEngine_Status(
	HPLAY hPlay
)
{
	S_PLAY_ENGINE_RES *psPlayEngineRes = (S_PLAY_ENGINE_RES *)hPlay;

	if(psPlayEngineRes == NULL)
		return eNM_PLAY_STATUS_ERROR;

	return PlayDeMux_Status(&psPlayEngineRes->sDeMuxRes);
}

