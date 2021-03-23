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

#include "RecordEngine.h"
#include "RecordMux.h"
#include "RecordEncode.h"

#include "Util/NMVideoList.h"
#include "Util/NMAudioList.h"

#define _MAX_VIDEO_LIST_DURATION_		3000		//3sec
#define _MAX_AUDIO_LIST_DURATION_		3000		//3sec


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

E_NM_ERRNO
RecordEngine_Create(
	HRECORD *phRecord,
	uint32_t u32Duration,
	S_NM_RECORD_IF *psRecordIF,
	S_NM_RECORD_CONTEXT *psRecordCtx,
	PFN_NM_RECORD_STATUSCB pfnStatusCB,
	void *pvStatusCBPriv
)
{

	S_RECORD_ENGINE_RES *psRecordEngineRes = NULL;
	E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
	bool bMuxResInit = false;
	bool bEncodeResInit = false;
	void *pvVideoList = NULL;
	void *pvAudioList = NULL;
	
	psRecordEngineRes = calloc(1, sizeof(S_RECORD_ENGINE_RES));
	if(psRecordEngineRes == NULL)
		return eNM_ERRNO_MALLOC;
	
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes = &psRecordEngineRes->sEncodeRes.sVideoEncodeRes;
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes = &psRecordEngineRes->sEncodeRes.sAudioEncodeRes;
	
	//Fill media interface
	psRecordEngineRes->sMuxRes.psMediaIF = psRecordIF->psMediaIF;
	psRecordEngineRes->sMuxRes.pvMediaRes = psRecordIF->pvMediaRes;

	//Copy contexts
	memcpy(&psRecordEngineRes->sMuxRes.sMediaVideoCtx, &psRecordCtx->sMediaVideoCtx, sizeof(S_NM_VIDEO_CTX));
	memcpy(&psRecordEngineRes->sMuxRes.sMediaAudioCtx, &psRecordCtx->sMediaAudioCtx, sizeof(S_NM_AUDIO_CTX));

	//Set status callback and duration
	psRecordEngineRes->sMuxRes.pfnStatusCB = pfnStatusCB;
	psRecordEngineRes->sMuxRes.pvStatusCBPriv = pvStatusCBPriv;
	psRecordEngineRes->sMuxRes.u32RecordTimeLimit = u32Duration;	
	
	//Mux engine init
	if((eNMRet = RecordMux_Init(&psRecordEngineRes->sMuxRes)) == eNM_ERRNO_NONE){
		bMuxResInit = true;
	}
	else{
		goto RecordEngine_Create_fail;
	}

	//Fill codec and context flush interface
	psVideoEncodeRes->psVideoCodecIF = psRecordIF->psVideoCodecIF;
	psAudioEncodeRes->psAudioCodecIF = psRecordIF->psAudioCodecIF;
	psVideoEncodeRes->pfnVideoFill = psRecordIF->pfnVideoFill;
	psAudioEncodeRes->pfnAudioFill = psRecordIF->pfnAudioFill;

	//Copy contexts
	memcpy(&psVideoEncodeRes->sFillVideoCtx, &psRecordCtx->sFillVideoCtx, sizeof(S_NM_VIDEO_CTX));
	memcpy(&psAudioEncodeRes->sFillAudioCtx, &psRecordCtx->sFillAudioCtx, sizeof(S_NM_AUDIO_CTX));
	memcpy(&psVideoEncodeRes->sMediaVideoCtx, &psRecordCtx->sMediaVideoCtx, sizeof(S_NM_VIDEO_CTX));
	memcpy(&psAudioEncodeRes->sMediaAudioCtx, &psRecordCtx->sMediaAudioCtx, sizeof(S_NM_AUDIO_CTX));

	//Encode engine init
	if((eNMRet = RecordEncode_Init(&psRecordEngineRes->sEncodeRes)) == eNM_ERRNO_NONE){
		bEncodeResInit = true;
	}
	else{
		goto RecordEngine_Create_fail;
	}

		//Create video list
	if(psVideoEncodeRes->psVideoCodecIF){
		pvVideoList = NMVideoList_Create(psRecordCtx->sMediaVideoCtx.eVideoType, _MAX_VIDEO_LIST_DURATION_); 
		if(pvVideoList == NULL){
			eNMRet = eNM_ERRNO_NULL_RES;
			goto RecordEngine_Create_fail;
		}
	}
	
	//Create audio list
	if(psAudioEncodeRes->psAudioCodecIF){
		pvAudioList = NMAudioList_Create(psRecordCtx->sMediaAudioCtx.eAudioType, _MAX_AUDIO_LIST_DURATION_); 
		if(pvAudioList == NULL){
			eNMRet = eNM_ERRNO_NULL_RES;
			goto RecordEngine_Create_fail;
		}
	}

	psRecordEngineRes->pvVideoList = pvVideoList;
	psRecordEngineRes->pvAudioList = pvAudioList;
	
	psRecordEngineRes->sMuxRes.psRecordEngRes = psRecordEngineRes;
	psRecordEngineRes->sEncodeRes.psRecordEngRes = psRecordEngineRes;

	//create Mux thread
	eNMRet = RecordMux_ThreadCreate(&psRecordEngineRes->sMuxRes);
	if(eNMRet != eNM_ERRNO_NONE){
		goto RecordEngine_Create_fail;
	}
	
	//Create encode thread
	eNMRet = RecordEncode_ThreadCreate(&psRecordEngineRes->sEncodeRes);
	if(eNMRet != eNM_ERRNO_NONE){
		goto RecordEngine_Create_fail;
	}
	
	RecordMux_Record(&psRecordEngineRes->sMuxRes, true);
	RecordEncode_Run(&psRecordEngineRes->sEncodeRes);
	
	*phRecord = (HRECORD)psRecordEngineRes;
	return eNM_ERRNO_NONE;

RecordEngine_Create_fail:

	RecordEncode_ThreadDestroy(&psRecordEngineRes->sEncodeRes);
	RecordMux_ThreadDestroy(&psRecordEngineRes->sMuxRes);
		
	if(pvVideoList)
		NMVideoList_Destroy(&pvVideoList);
	
	if(pvAudioList)
		NMAudioList_Destroy(&pvAudioList);
	
	if(bEncodeResInit){
		RecordEncode_Final(&psRecordEngineRes->sEncodeRes);
	}

	if(bMuxResInit){
		RecordMux_Final(&psRecordEngineRes->sMuxRes);
	}

	if(psRecordEngineRes)
		free(psRecordEngineRes);

	return eNMRet;
}


E_NM_ERRNO
RecordEngine_Destroy(
	HRECORD hRecord
)
{
	S_RECORD_ENGINE_RES *psRecordEngineRes = (S_RECORD_ENGINE_RES *)hRecord;

	if(psRecordEngineRes == NULL)
		return eNM_ERRNO_NULL_RES;

	RecordEncode_Stop(&psRecordEngineRes->sEncodeRes);

	RecordMux_Pause(&psRecordEngineRes->sMuxRes, true);
			
	//Destroy encode thread
	RecordEncode_ThreadDestroy(&psRecordEngineRes->sEncodeRes);
	
	//Destroy mux thread
	RecordMux_ThreadDestroy(&psRecordEngineRes->sMuxRes);
	
	//Final encode thread
	RecordEncode_Final(&psRecordEngineRes->sEncodeRes);

	// Final mux thread
	RecordMux_Final(&psRecordEngineRes->sMuxRes);

		//Free resource
	if(psRecordEngineRes->pvVideoList)
		NMVideoList_Destroy(&psRecordEngineRes->pvVideoList);
	
	if(psRecordEngineRes->pvAudioList)
		NMAudioList_Destroy(&psRecordEngineRes->pvAudioList);

	free(psRecordEngineRes);
	return eNM_ERRNO_NONE;
}

E_NM_ERRNO
RecordEngine_RegNextMedia(
	HRECORD hRecord,
	S_NM_MEDIAWRITE_IF *psMediaIF,
	void *pvMediaRes,
	void *pvStatusCBPriv
)
{
	S_RECORD_ENGINE_RES *psRecordEngineRes = (S_RECORD_ENGINE_RES *)hRecord;

	if(psRecordEngineRes == NULL)
		return eNM_ERRNO_NULL_RES;

	return RecordMux_RegNextMedia(&psRecordEngineRes->sMuxRes, psMediaIF, pvMediaRes, pvStatusCBPriv, true);
}


