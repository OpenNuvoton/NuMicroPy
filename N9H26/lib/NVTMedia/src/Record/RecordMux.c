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

#include "Util/NMVideoList.h"
#include "Util/NMAudioList.h"

#define DEF_REPORT_REC_STATUS_DURATION 1000			//1sec

typedef enum {
	eMUX_THREAD_STATE_INIT,
	eMUX_THREAD_STATE_IDLE,
	eMUX_THREAD_STATE_TO_RECORD,
	eMUX_THREAD_STATE_RECORDING,
	eMUX_THREAD_STATE_PAUSE,
	eMUX_THREAD_STATE_TO_EXIT,
	eMUX_THREAD_STATE_EXIT,
} E_MUX_THREAD_STATE;

typedef struct{
	S_NM_MEDIAWRITE_IF *psMediaWriteIF;
	void *pvMediaRes;
	void *pvStatusCBPriv;
}S_MUX_MEDIA_ATTR;

typedef struct {
	E_MUX_THREAD_STATE eMuxThreadState;

	pthread_mutex_t tCmdMutex;
	E_MUX_IOCTL_CODE	eCmdCode;
	void *pvCmdArgv;
	E_NM_ERRNO eCmdRet;

	S_MUX_MEDIA_ATTR sCurrentMedia;
	S_MUX_MEDIA_ATTR sNextMedia;
	
	uint64_t u64CurStartRecTime;
	uint64_t u64CurVideoStartTime;
	uint64_t u64CurAudioStartTime;
	uint32_t u32CurVideoChunks;
	uint32_t u32CurAudioChunks;

	uint64_t u64TotalChunkSize;
} S_MUX_PRIV;

static int
RunMuxCmd(
	S_RECORD_MUX_RES *psMuxRes
)
{
	int i32SemValue;

	S_MUX_PRIV *psMuxPriv = (S_MUX_PRIV *)psMuxRes->pvMuxPriv;

	pthread_mutex_lock(&psMuxPriv->tCmdMutex);

	if(psMuxPriv->eCmdCode == eMUX_IOCTL_NONE){
		sem_getvalue(&psMuxRes->tMuxCtrlSem, &i32SemValue);
		if(i32SemValue < 0){
			printf("%s, %s: Warning! post semaphore again! \n", __FILE__, __FUNCTION__);
			sem_post(&psMuxRes->tMuxCtrlSem);
		}
		
		pthread_mutex_unlock(&psMuxPriv->tCmdMutex);
		return 0;
	}

	psMuxPriv->eCmdRet = eNM_ERRNO_NONE;

	switch(psMuxPriv->eCmdCode){
		case eMUX_IOCTL_GET_STATE:
		{
			E_MUX_THREAD_STATE *peThreadState = (E_MUX_THREAD_STATE *)psMuxPriv->pvCmdArgv;
			*peThreadState = psMuxPriv->eMuxThreadState;
		}
		break;
		case eMUX_IOCTL_SET_STATE:
		{
			E_MUX_THREAD_STATE eThreadState = (E_MUX_THREAD_STATE)psMuxPriv->pvCmdArgv;
			psMuxPriv->eMuxThreadState = eThreadState;
		}
		break;
		case eMUX_IOCTL_SET_RECORD:
		{
			if(psMuxPriv->eMuxThreadState != eMUX_THREAD_STATE_RECORDING)
				psMuxPriv->eMuxThreadState = eMUX_THREAD_STATE_TO_RECORD;
		}
		break;
		case eMUX_IOCTL_SET_PAUSE:
		{
			if(psMuxPriv->eMuxThreadState != eMUX_THREAD_STATE_IDLE)
				psMuxPriv->eMuxThreadState = eMUX_THREAD_STATE_PAUSE;
		}
		break;		
		case eMUX_IOCTL_SET_NEXT_MEDIA:
		{
			S_MUX_MEDIA_ATTR *psMediaAttr = (S_MUX_MEDIA_ATTR *)psMuxPriv->pvCmdArgv;

			if(psMuxPriv->sNextMedia.psMediaWriteIF == NULL){
				psMuxPriv->sNextMedia.psMediaWriteIF = psMediaAttr->psMediaWriteIF;
				psMuxPriv->sNextMedia.pvMediaRes = psMediaAttr->pvMediaRes;
				psMuxPriv->sNextMedia.pvStatusCBPriv = psMediaAttr->pvStatusCBPriv;				
			}
			else{
				NMLOG_ERROR("Unable register next media \n");
				psMuxPriv->eCmdRet = eNM_ERRNO_BAD_PARAM; 
			}			
			free(psMediaAttr);
		}
		break;
	}
	
	psMuxPriv->eCmdCode = eMUX_IOCTL_NONE;

	pthread_mutex_unlock(&psMuxPriv->tCmdMutex);
	
	sem_getvalue(&psMuxRes->tMuxCtrlSem,	&i32SemValue);

   /* If semaphore value is equal or larger than zero, there is no
     * thread waiting for this semaphore. Otherwise (<0), call FreeRTOS interface
     * to wake up a thread. */	

	if(i32SemValue < 0)
		sem_post(&psMuxRes->tMuxCtrlSem);
	return 0;
}



static void FillRecordInfo(
	S_NM_RECORD_INFO *psRecordInfo,
	S_RECORD_MUX_RES *psMuxRes,
	S_MUX_PRIV *psMuxPriv
)
{
	psRecordInfo->eMediaFormat = eNM_MEDIA_FORMAT_FILE;
	psRecordInfo->eVideoType = psMuxRes->sMediaVideoCtx.eVideoType;
	psRecordInfo->eAudioType = psMuxRes->sMediaAudioCtx.eAudioType;
	if(psMuxPriv->u64CurStartRecTime == 0xFFFFFFFFFFFFFFFF)
		psRecordInfo->u32Duration = 0;
	else
		psRecordInfo->u32Duration = (uint32_t)(NMUtil_GetTimeMilliSec() - psMuxPriv->u64CurStartRecTime);
	psRecordInfo->u32VideoChunks = psMuxPriv->u32CurVideoChunks;
	psRecordInfo->u32AudioChunks = psMuxPriv->u32CurAudioChunks;
	psRecordInfo->u64TotalChunkSize = psMuxPriv->u64TotalChunkSize;
}

static void SupplementZeroVideoFrame(
	S_MUX_PRIV	*psMuxPriv,
	S_NMPACKET *psNewPacket,
	S_NM_VIDEO_CTX *psVideoCtx
)
{
	uint32_t u32ExpectFrameRate = psVideoCtx->u32FrameRate;
	uint32_t u32ExpectFrames = 0;
	uint32_t u32SupplementFrames = 0;
	S_MUX_MEDIA_ATTR *psCurMediaAttr = &psMuxPriv->sCurrentMedia;

	if(psMuxPriv->u64CurVideoStartTime  == 0){
		psMuxPriv->u64CurVideoStartTime = psNewPacket->u64DataTime;
//		NMLOG_INFO("Video start chunk timestamp is %d\n", (uint32_t)psMuxPriv->u64CurVideoStartTime);
		return;
	}
	
	if(psNewPacket->u64DataTime < psMuxPriv->u64CurVideoStartTime){
		NMLOG_ERROR("Video chunk timestamp is incorrect \n");
		return; 
	}
	
//	printf("Video packet time is %d \n", (uint32_t)psNewPacket->u64DataTime);

	u32ExpectFrames = (psNewPacket->u64DataTime - psMuxPriv->u64CurVideoStartTime) / (1000 / u32ExpectFrameRate);

	if(u32ExpectFrames > (psMuxPriv->u32CurVideoChunks + 2))
		u32SupplementFrames = u32ExpectFrames - (psMuxPriv->u32CurVideoChunks + 2);

	if(u32SupplementFrames){
		NMLOG_INFO("Supplement %d video frames\n", u32SupplementFrames);
	}
	
	psVideoCtx->pu8DataBuf = psNewPacket->pu8DataPtr;
	psVideoCtx->u32DataSize = 0;
	psVideoCtx->u32DataLimit = psNewPacket->u32UsedDataLen;
	psVideoCtx->u64DataTime = psNewPacket->u64DataTime;
	psVideoCtx->bKeyFrame = false;

	while(u32SupplementFrames){
		psCurMediaAttr->psMediaWriteIF->pfnWriteVideoChunk(psMuxPriv->u32CurVideoChunks, psVideoCtx, psCurMediaAttr->pvMediaRes);
		psMuxPriv->u32CurVideoChunks ++;
		u32SupplementFrames --;
	}	
}

static void SupplementZeroAudioFrame(
	S_MUX_PRIV	*psMuxPriv,
	S_NMPACKET *psNewPacket,
	S_NM_AUDIO_CTX *psAudioCtx
)
{
	uint32_t u32FrameDuration = 0;
	uint32_t u32ExpectFrames = 0;
	uint32_t u32SupplementFrames = 0;
	S_MUX_MEDIA_ATTR *psCurMediaAttr = &psMuxPriv->sCurrentMedia;

	u32FrameDuration = (psAudioCtx->u32SamplePerBlock * 1000) / psAudioCtx->u32SampleRate;

	if(psMuxPriv->u64CurAudioStartTime  == 0){
		psMuxPriv->u64CurAudioStartTime = psNewPacket->u64DataTime;
//		NMLOG_INFO("Audio start chunk timestamp is %d\n", (uint32_t)psMuxPriv->u64CurAudioStartTime);
		return;
	}

	if(psNewPacket->u64DataTime < psMuxPriv->u64CurAudioStartTime){
		NMLOG_ERROR("Audio chunk timestamp is incorrect \n");
		return; 
	}
	
//	printf("Audio packet time is %d \n", (uint32_t)psNewPacket->u64DataTime);
	u32ExpectFrames = (psNewPacket->u64DataTime - psMuxPriv->u64CurAudioStartTime) / u32FrameDuration;
	
	if(u32ExpectFrames > (psMuxPriv->u32CurAudioChunks + 2))
		u32SupplementFrames = u32ExpectFrames - (psMuxPriv->u32CurAudioChunks + 2);

	if(u32SupplementFrames){
		NMLOG_INFO("Supplement %d audio frames\n", u32SupplementFrames);
	}
	
	psAudioCtx->pu8DataBuf = psNewPacket->pu8DataPtr;
	psAudioCtx->u32DataSize = 0;
	psAudioCtx->u32DataLimit = psNewPacket->u32UsedDataLen;
	psAudioCtx->u64DataTime = psNewPacket->u64DataTime;
	
	while(u32SupplementFrames){
		psCurMediaAttr->psMediaWriteIF->pfnWriteAudioChunk(psMuxPriv->u32CurAudioChunks, psAudioCtx, psCurMediaAttr->pvMediaRes);
		psMuxPriv->u32CurAudioChunks ++;
		u32SupplementFrames --;
	}

}


static void * MuxWorkerThread( void * pvArgs )
{
	S_RECORD_MUX_RES *psMuxRes = (S_RECORD_MUX_RES *)pvArgs;
	S_MUX_PRIV *psMuxPriv = (S_MUX_PRIV *)psMuxRes->pvMuxPriv;
//	uint64_t u64CurrentTime;
	void *pvVideoList = psMuxRes->psRecordEngRes->pvVideoList;
	void *pvAudioList = psMuxRes->psRecordEngRes->pvAudioList;
	S_NMPACKET *psVideoPacket = NULL;
	S_NMPACKET *psAudioPacket = NULL;
	S_NMPACKET *psFirstPacket = NULL;
	bool bOutputVideoFirst = true;
	S_NM_RECORD_INFO sRecordInfo;
	S_MUX_MEDIA_ATTR *psCurMediaAttr = &psMuxPriv->sCurrentMedia;
	S_MUX_MEDIA_ATTR *psNextMediaAttr = &psMuxPriv->sNextMedia;
	S_NM_VIDEO_CTX *psVideoCtx = &psMuxRes->sMediaVideoCtx;
	S_NM_AUDIO_CTX *psAudioCtx = &psMuxRes->sMediaAudioCtx;
	uint64_t u64ReportStatusTime = 0;
	
	psMuxPriv->eMuxThreadState = eMUX_THREAD_STATE_IDLE;

	psMuxPriv->u64CurStartRecTime = 0xFFFFFFFFFFFFFFFF;
	psMuxPriv->u64TotalChunkSize = 0;
	
	while(psMuxPriv->eMuxThreadState != eMUX_THREAD_STATE_TO_EXIT){
		RunMuxCmd(psMuxRes);
//		u64CurrentTime = NMUtil_GetTimeMilliSec();

		if((psMuxPriv->eMuxThreadState == eMUX_THREAD_STATE_IDLE) || (psMuxPriv->eMuxThreadState == eMUX_THREAD_STATE_PAUSE)){
			usleep(1000);
			continue;
		}

		if(psMuxPriv->eMuxThreadState == eMUX_THREAD_STATE_TO_RECORD){
			FillRecordInfo(&sRecordInfo, psMuxRes, psMuxPriv);
			
			if(psMuxRes->pfnStatusCB)
				psMuxRes->pfnStatusCB(eNM_RECORD_STATUS_RECORDING, &sRecordInfo, psCurMediaAttr->pvStatusCBPriv);

			psMuxPriv->eMuxThreadState = eMUX_THREAD_STATE_RECORDING;
			u64ReportStatusTime = NMUtil_GetTimeMilliSec() + DEF_REPORT_REC_STATUS_DURATION;
		}

		psVideoPacket = NULL;
		psAudioPacket = NULL;
		psFirstPacket = NULL;
		bOutputVideoFirst = false;
		
		//Acquire frame from video packet list
		if(pvVideoList){
			psVideoPacket = NMVideoList_AcquirePacket(pvVideoList);
		}

		//Acquire frame from audio packet list
		if(pvAudioList){
			psAudioPacket = NMAudioList_AcquirePacket(pvAudioList);
		}
	
		if((psVideoPacket == NULL) && (psAudioPacket == NULL)){
			usleep(5000);
			continue;
		}		

		if((psVideoPacket == NULL) && (psAudioPacket)){
			bOutputVideoFirst = false;
		}
		else if((psVideoPacket) && (psAudioPacket == NULL)){
			bOutputVideoFirst = true;
		}
		else{
			//both have video and audio packet, compare timestamp
			if(psVideoPacket->u64DataTime >= psAudioPacket->u64DataTime)
				bOutputVideoFirst = true;
			else
				bOutputVideoFirst = false;
		}

		if(bOutputVideoFirst){
			psFirstPacket = psVideoPacket;
		}
		else{
			psFirstPacket = psAudioPacket;
		}
		
		if(psFirstPacket->u64DataTime < psMuxPriv->u64CurStartRecTime)
			psMuxPriv->u64CurStartRecTime = psFirstPacket->u64DataTime;

		
		//Checking switch media file or not
		if(psMuxRes->u32RecordTimeLimit != eNM_UNLIMIT_TIME){

			if(psFirstPacket->u64DataTime > (psMuxPriv->u64CurStartRecTime + psMuxRes->u32RecordTimeLimit)){
				//Switch next media
				if(((bOutputVideoFirst == true) && (psFirstPacket->ePktType & eNMPACKET_IFRAME)) ||
					 (pvVideoList == NULL)){

					//Send status notify
					if(psMuxRes->pfnStatusCB){
						FillRecordInfo(&sRecordInfo, psMuxRes, psMuxPriv);

						if(psMuxRes->pfnStatusCB)
							psMuxRes->pfnStatusCB(eNM_RECORD_STATUS_CHANGE_MEDIA, &sRecordInfo, psCurMediaAttr->pvStatusCBPriv);
					}
					//change next media
					psCurMediaAttr->psMediaWriteIF = psNextMediaAttr->psMediaWriteIF;
					psCurMediaAttr->pvMediaRes = psNextMediaAttr->pvMediaRes;
					psCurMediaAttr->pvStatusCBPriv = psNextMediaAttr->pvStatusCBPriv; 
					psNextMediaAttr->psMediaWriteIF = NULL;
					psNextMediaAttr->pvMediaRes = NULL;
					psNextMediaAttr->pvStatusCBPriv = NULL;

					psMuxPriv->u64CurStartRecTime = psFirstPacket->u64DataTime;
					psMuxPriv->u64CurVideoStartTime = 0;
					psMuxPriv->u64CurAudioStartTime = 0;
					psMuxPriv->u32CurVideoChunks = 0;
					psMuxPriv->u32CurAudioChunks = 0;

					if(psCurMediaAttr->psMediaWriteIF == NULL){
						if(psMuxRes->pfnStatusCB)
							psMuxRes->pfnStatusCB(eNM_RECORD_STATUS_STOP, &sRecordInfo, psCurMediaAttr->pvStatusCBPriv);
					}
				}				
			}
		}

		//If media_if is null, just release package
		if(psCurMediaAttr->psMediaWriteIF == NULL){
			if(psVideoPacket){
				NMVideoList_ReleasePacket(psVideoPacket, pvVideoList);
				psVideoPacket = NULL;
			}

			if(psAudioPacket){
				NMAudioList_ReleasePacket(psAudioPacket, pvAudioList);
				psAudioPacket = NULL;
			}
			
			psMuxPriv->u64CurStartRecTime = 0xFFFFFFFFFFFFFFFF;
			continue;
		}

		if(psVideoPacket){
			SupplementZeroVideoFrame(psMuxPriv, psVideoPacket, psVideoCtx);

			psVideoCtx->pu8DataBuf = psVideoPacket->pu8DataPtr;
			psVideoCtx->u32DataSize = psVideoPacket->u32UsedDataLen;
			psVideoCtx->u32DataLimit = psVideoPacket->u32UsedDataLen;
			psVideoCtx->u64DataTime = psVideoPacket->u64DataTime;

			if(psVideoPacket->ePktType & eNMPACKET_IFRAME)
				psVideoCtx->bKeyFrame = true;
			else
				psVideoCtx->bKeyFrame = false;
		}
		
		if(psAudioPacket){
			SupplementZeroAudioFrame(psMuxPriv, psAudioPacket, psAudioCtx);

			psAudioCtx->pu8DataBuf = psAudioPacket->pu8DataPtr;
			psAudioCtx->u32DataSize = psAudioPacket->u32UsedDataLen;
			psAudioCtx->u32DataLimit = psAudioPacket->u32UsedDataLen;
			psAudioCtx->u64DataTime = psAudioPacket->u64DataTime;
		}
		
		//Write packet to current media
		if(bOutputVideoFirst){

			psCurMediaAttr->psMediaWriteIF->pfnWriteVideoChunk(psMuxPriv->u32CurVideoChunks, psVideoCtx, psCurMediaAttr->pvMediaRes);
			psMuxPriv->u32CurVideoChunks ++;
			psMuxPriv->u64TotalChunkSize += psVideoCtx->u32DataSize;
			
			if(psAudioPacket){
				psCurMediaAttr->psMediaWriteIF->pfnWriteAudioChunk(psMuxPriv->u32CurAudioChunks, psAudioCtx, psCurMediaAttr->pvMediaRes);
				psMuxPriv->u32CurAudioChunks ++;
				psMuxPriv->u64TotalChunkSize += psAudioCtx->u32DataSize;
			}
		}
		else{
			psCurMediaAttr->psMediaWriteIF->pfnWriteAudioChunk(psMuxPriv->u32CurAudioChunks, psAudioCtx, psCurMediaAttr->pvMediaRes);
			psMuxPriv->u32CurAudioChunks ++;
			psMuxPriv->u64TotalChunkSize += psAudioCtx->u32DataSize;

			if(psVideoPacket){
				psCurMediaAttr->psMediaWriteIF->pfnWriteVideoChunk(psMuxPriv->u32CurVideoChunks, psVideoCtx, psCurMediaAttr->pvMediaRes);
				psMuxPriv->u32CurVideoChunks ++;
				psMuxPriv->u64TotalChunkSize += psVideoCtx->u32DataSize;
			}
		}
		
		if(psVideoPacket){
			NMVideoList_ReleasePacket(psVideoPacket, pvVideoList);
			psVideoPacket = NULL;
		}

		if(psAudioPacket){
			NMAudioList_ReleasePacket(psAudioPacket, pvAudioList);
			psAudioPacket = NULL;
		}
		
		if(NMUtil_GetTimeMilliSec() > u64ReportStatusTime){
			//Send status notify
			if(psMuxRes->pfnStatusCB){
				FillRecordInfo(&sRecordInfo, psMuxRes, psMuxPriv);
				psMuxRes->pfnStatusCB(eNM_RECORD_STATUS_RECORDING, &sRecordInfo, psCurMediaAttr->pvStatusCBPriv);
			}
			u64ReportStatusTime = NMUtil_GetTimeMilliSec() + DEF_REPORT_REC_STATUS_DURATION;
		}

		usleep(1000);
	}

	// Free video/audio packet
	if(psVideoPacket){
		NMVideoList_ReleasePacket(psVideoPacket, pvVideoList);
	}

	if(psAudioPacket){
		NMAudioList_ReleasePacket(psAudioPacket, pvAudioList);
	}
	
	psMuxPriv->eMuxThreadState = eMUX_THREAD_STATE_EXIT;
	
	return NULL;
}

static E_NM_ERRNO
InitMuxPriv(
	S_RECORD_MUX_RES *psMuxRes
)
{
	E_NM_ERRNO eNMRet;
	S_MUX_PRIV *psMuxPriv = NULL;

	psMuxPriv = calloc(1, sizeof(S_MUX_PRIV));

	if(psMuxPriv == NULL){
		eNMRet = eNM_ERRNO_MALLOC;
		goto InitMuxPriv_fail;
	}

	//Init command mutex
	if(pthread_mutex_init(&psMuxPriv->tCmdMutex, NULL) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto InitMuxPriv_fail;
	}
	
	psMuxPriv->eCmdCode = eMUX_IOCTL_NONE;	
	psMuxPriv->pvCmdArgv = NULL;	
	psMuxPriv->eMuxThreadState = eMUX_THREAD_STATE_INIT;

	psMuxPriv->sCurrentMedia.psMediaWriteIF = psMuxRes->psMediaIF;
	psMuxPriv->sCurrentMedia.pvMediaRes = psMuxRes->pvMediaRes;
	psMuxPriv->sCurrentMedia.pvStatusCBPriv = psMuxRes->pvStatusCBPriv;

	psMuxRes->pvMuxPriv = (void *)psMuxPriv;
	
	return eNM_ERRNO_NONE;
InitMuxPriv_fail:
	
	if(psMuxPriv){
		pthread_mutex_destroy(&psMuxPriv->tCmdMutex);		
		free(psMuxPriv);
	}
	
	return eNMRet;	
}

static void
FinialMuxPriv(
	S_RECORD_MUX_RES *psMuxRes
)
{
	S_MUX_PRIV *psMuxPriv;
	
	if(psMuxRes->pvMuxPriv == NULL)
		return;

	psMuxPriv = (S_MUX_PRIV *)psMuxRes->pvMuxPriv;

	pthread_mutex_destroy(&psMuxPriv->tCmdMutex);		
	free(psMuxPriv);
	psMuxRes->pvMuxPriv = NULL;
}


static E_NM_ERRNO
MuxThread_IOCTL(
	S_RECORD_MUX_RES *psMuxRes,
	E_MUX_IOCTL_CODE eCmdCode,
	void *pvCmdArgv,
	int32_t i32CmdArgvSize,
	bool bBlocking
)
{
	E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
	S_MUX_PRIV *psMuxPriv;

	if(psMuxRes == NULL)
		return eNM_ERRNO_NULL_PTR;
	
	psMuxPriv = (S_MUX_PRIV *)psMuxRes->pvMuxPriv;
	if(psMuxPriv == NULL)
		return eNM_ERRNO_NONE;
	
	pthread_mutex_lock(&psMuxRes->tMuxCtrlMutex);
	pthread_mutex_lock(&psMuxPriv->tCmdMutex);

	//Argument size check
	switch (eCmdCode){
		case eMUX_IOCTL_GET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_MUX_THREAD_STATE *)){
				eNMRet = eNM_ERRNO_SIZE;
			}			
		}
		break;
		case eMUX_IOCTL_SET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_MUX_THREAD_STATE)){
				eNMRet = eNM_ERRNO_SIZE;
			}
		}
		break;
		case eMUX_IOCTL_SET_NEXT_MEDIA:
		{
			if(i32CmdArgvSize < sizeof(S_MUX_MEDIA_ATTR *)){
				eNMRet = eNM_ERRNO_SIZE;
			}
		}
		break;
		case eMUX_IOCTL_SET_RECORD:
		case eMUX_IOCTL_SET_PAUSE:
		{
		}
		break;
	}

	if(eNMRet == eNM_ERRNO_NONE){
		psMuxPriv->eCmdCode = eCmdCode;
		psMuxPriv->pvCmdArgv = pvCmdArgv;		

		pthread_mutex_unlock(&psMuxPriv->tCmdMutex);
		if(bBlocking){
			sem_wait(&psMuxRes->tMuxCtrlSem);
			eNMRet = psMuxPriv->eCmdRet;
		}
	}
	else{
		pthread_mutex_unlock(&psMuxPriv->tCmdMutex);
	}
	
	pthread_mutex_unlock(&psMuxRes->tMuxCtrlMutex);
	return eNMRet;
}

/////////////////////////////////////////////////////////////////////////////

void
RecordMux_Final(
	S_RECORD_MUX_RES *psMuxRes
)
{
	FinialMuxPriv(psMuxRes);
	pthread_mutex_destroy(&psMuxRes->tMuxCtrlMutex);
	sem_destroy(&psMuxRes->tMuxCtrlSem);
}

E_NM_ERRNO
RecordMux_Init(
	S_RECORD_MUX_RES *psMuxRes
)
{
	E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
	
	//Init control mutex and semphore
	if(pthread_mutex_init(&psMuxRes->tMuxCtrlMutex, NULL) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto RecordMux_Init_fail;
	}
	
	if(sem_init(&psMuxRes->tMuxCtrlSem, 0, 0) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto RecordMux_Init_fail;
	}

	//Alloc demux private data
	eNMRet = InitMuxPriv(psMuxRes);
	if(eNMRet != eNM_ERRNO_NONE){
		goto RecordMux_Init_fail;
	}

	return eNM_ERRNO_NONE;

RecordMux_Init_fail:
	
	RecordMux_Final(psMuxRes);
	return eNMRet;
}

void
RecordMux_ThreadDestroy(
	S_RECORD_MUX_RES *psMuxRes
)
{
	E_MUX_THREAD_STATE eMuxThreadState = eMUX_THREAD_STATE_TO_EXIT;
	void *pvJoin;

	if(psMuxRes->tMuxThread){
			MuxThread_IOCTL(psMuxRes, eMUX_IOCTL_SET_STATE, (void *)eMuxThreadState, sizeof(E_MUX_THREAD_STATE), false);
			printf("DDDDDDDD RecordMux_ThreadDestroy 0 \n");
			pthread_join(psMuxRes->tMuxThread, &pvJoin);
			printf("DDDDDDDD RecordMux_ThreadDestroy 1 \n");
			psMuxRes->tMuxThread = NULL;
	}
}

E_NM_ERRNO
RecordMux_ThreadCreate(
	S_RECORD_MUX_RES *psMuxRes
)
{
	int iRet;
	E_MUX_THREAD_STATE eMuxThreadState;
	
	iRet = pthread_create(&psMuxRes->tMuxThread, NULL, MuxWorkerThread, psMuxRes);
	
	if(iRet != 0)
		return eNM_ERRNO_OS;

	//Wait thread running
	while(1){
		MuxThread_IOCTL(psMuxRes, eMUX_IOCTL_GET_STATE, &eMuxThreadState, sizeof(E_MUX_THREAD_STATE *), true);
		if(eMuxThreadState != eMUX_THREAD_STATE_INIT)
			break;
		usleep(1000);
	}

	return eNM_ERRNO_NONE;
}

E_NM_ERRNO
RecordMux_Record(
	S_RECORD_MUX_RES *psMuxRes,
	bool bWait
)
{
	if(psMuxRes->tMuxThread == NULL)
		return eNM_ERRNO_OS;

	return MuxThread_IOCTL(psMuxRes, eMUX_IOCTL_SET_RECORD, NULL, 0, bWait);	
}

E_NM_ERRNO
RecordMux_Pause(
	S_RECORD_MUX_RES *psMuxRes,
	bool bWait
)
{
	if(psMuxRes->tMuxThread == NULL)
		return eNM_ERRNO_OS;

	return MuxThread_IOCTL(psMuxRes, eMUX_IOCTL_SET_PAUSE, NULL, 0, bWait);	
}

E_NM_ERRNO
RecordMux_RegNextMedia(
	S_RECORD_MUX_RES *psMuxRes,
	S_NM_MEDIAWRITE_IF *psMediaIF,
	void *pvMediaRes,
	void *pvStatusCBPriv,
	bool bWait
)
{
	S_MUX_MEDIA_ATTR *psMediaAttr;

	if(psMuxRes->tMuxThread == NULL)
		return eNM_ERRNO_OS;

	psMediaAttr = malloc(sizeof(S_MUX_MEDIA_ATTR));

	if(psMediaAttr == NULL)
		return eNM_ERRNO_MALLOC;
	
	psMediaAttr->psMediaWriteIF = psMediaIF;
	psMediaAttr->pvMediaRes = pvMediaRes;
	psMediaAttr->pvStatusCBPriv = pvStatusCBPriv;
		
	return MuxThread_IOCTL(psMuxRes, eMUX_IOCTL_SET_NEXT_MEDIA, psMediaAttr, sizeof(S_MUX_MEDIA_ATTR *), bWait);	
}


