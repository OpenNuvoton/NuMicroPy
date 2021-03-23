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
#include "PlayDeMux.h"

#include "Util/NMVideoList.h"
#include "Util/NMAudioList.h"

#define MAX_AV_CHUNK_FRAMES	1
#define MAX_PTS 0xFFFFFFFFFFFFFFFF

typedef enum {
	eDEMUX_THREAD_STATE_INIT,
	eDEMUX_THREAD_STATE_IDLE,
	eDEMUX_THREAD_STATE_TO_PLAY,
	eDEMUX_THREAD_STATE_PLAYING,
	eDEMUX_THREAD_STATE_PAUSE,
	eDEMUX_THREAD_STATE_TO_EXIT,
	eDEMUX_THREAD_STATE_EXIT,
	eDEMUX_THREAD_STATE_EOM,
} E_DEMUX_THREAD_STATE;

typedef struct {
	E_DEMUX_THREAD_STATE eDeMuxThreadState;

	pthread_mutex_t tCmdMutex;
	E_DEMUX_IOCTL_CODE	eCmdCode;
	void *pvCmdArgv;
	E_NM_ERRNO eCmdRet;
	
	S_NM_FRAME_DATA asVideoFrameData[MAX_AV_CHUNK_FRAMES];
	S_NM_FRAME_DATA asAudioFrameData[MAX_AV_CHUNK_FRAMES];
	
	E_NM_PLAY_FASTFORWARD_SPEED eFFSpeed;
	uint32_t u32CurAudChunkID;
	uint32_t u32CurVidChunkID;

} S_DEMUX_PRIV;


static E_NM_ERRNO
SeekVideoChunk(
	S_NM_MEDIAREAD_IF *psMediaIF,
	void *pvMediaRes,
	uint32_t u32TotalChunks,
	S_DEMUX_PRIV *psDeMuxPriv,
	uint64_t u64SeekTime
)
{
	uint32_t u32ChunkSize;
	uint64_t u64ChunkTime;
	bool bSeekable;
	E_NM_ERRNO eRet;

	uint32_t u32MinChunkID = 0;
	uint32_t u32MidChunkID = 1;	
	uint32_t u32MaxChunkID = u32TotalChunks;
	
	while(u32MaxChunkID == 0){
		eRet = psMediaIF->pfnGetVideoChunkInfo(u32MidChunkID, &u32ChunkSize, &u64ChunkTime, &bSeekable, pvMediaRes);
		if(eRet == eNM_ERRNO_EOM){
			u32MaxChunkID = u32MidChunkID;
			break;
		}
		
		if(u64ChunkTime > u64SeekTime){
			u32MaxChunkID = u32MidChunkID;
			break;
		}
		u32MinChunkID = u32MidChunkID;
		u32MidChunkID = u32MidChunkID * 2;
	}

	while(1){
		u32MidChunkID = (u32MinChunkID + u32MaxChunkID) / 2;
		eRet = psMediaIF->pfnGetVideoChunkInfo(u32MidChunkID, &u32ChunkSize, &u64ChunkTime, &bSeekable, pvMediaRes);
		if(eRet == eNM_ERRNO_EOM){
			u32MaxChunkID = u32MidChunkID;
			continue;
		}

		if((u32MinChunkID + 1) >= u32MidChunkID)
			break;
		
		if(u64ChunkTime > u64SeekTime){
			if((u64ChunkTime - u64SeekTime) < 500){			// < 500ms: near seek time
				break;
			}				 
			else{
				u32MaxChunkID = u32MidChunkID;
			}
		}
		else{
			u32MinChunkID = u32MidChunkID;
		}
	}
	
	while((bSeekable == false) && (u32MidChunkID > 0)){
		u32MidChunkID --;
		eRet = psMediaIF->pfnGetVideoChunkInfo(u32MidChunkID, &u32ChunkSize, &u64ChunkTime, &bSeekable, pvMediaRes);
		if(eRet == eNM_ERRNO_EOM){
			return eNM_ERRNO_EOM;
		}
	}

	psDeMuxPriv->u32CurVidChunkID = u32MidChunkID;
	
	printf("SeekVideoChunkID finetune %d, time %d \n", u32MidChunkID, (uint32_t)u64ChunkTime);
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
SeekAudioChunk(
	S_NM_MEDIAREAD_IF *psMediaIF,
	void *pvMediaRes,
	uint32_t u32TotalChunks,
	S_DEMUX_PRIV *psDeMuxPriv,
	uint64_t u64SeekTime
)
{
	uint32_t u32ChunkSize;
	uint64_t u64ChunkTime;
	bool bSeekable;
	E_NM_ERRNO eRet;

	uint32_t u32MinChunkID = 0;
	uint32_t u32MidChunkID = 1;	
	uint32_t u32MaxChunkID = u32TotalChunks;
	
	while(u32MaxChunkID == 0){
		eRet = psMediaIF->pfnGetAudioChunkInfo(u32MidChunkID, &u32ChunkSize, &u64ChunkTime, &bSeekable, pvMediaRes);
		if(eRet == eNM_ERRNO_EOM){
			u32MaxChunkID = u32MidChunkID;
			break;
		}
		
		if(u64ChunkTime > u64SeekTime){
			u32MaxChunkID = u32MidChunkID;
			break;
		}
		u32MinChunkID = u32MidChunkID;
		u32MidChunkID = u32MidChunkID * 2;
	}

	while(1){
		u32MidChunkID = (u32MinChunkID + u32MaxChunkID) / 2;
		eRet = psMediaIF->pfnGetAudioChunkInfo(u32MidChunkID, &u32ChunkSize, &u64ChunkTime, &bSeekable, pvMediaRes);
		if(eRet == eNM_ERRNO_EOM){
			u32MaxChunkID = u32MidChunkID;
			continue;
		}
		
		if((u32MinChunkID + 1) >= u32MidChunkID)
			break;
		
		if(u64ChunkTime > u64SeekTime){
			if((u64ChunkTime - u64SeekTime) < 500){			// < 500ms: near seek time
				break;
			}				 
			else{
				u32MaxChunkID = u32MidChunkID;
			}
		}
		else{
			u32MinChunkID = u32MidChunkID;
		}
	}

	while((bSeekable == false) && (u32MidChunkID > 0)){
		u32MidChunkID --;
		eRet = psMediaIF->pfnGetAudioChunkInfo(u32MidChunkID, &u32ChunkSize, &u64ChunkTime, &bSeekable, pvMediaRes);
		if(eRet == eNM_ERRNO_EOM){
			return eNM_ERRNO_EOM;
		}
	}

	psDeMuxPriv->u32CurAudChunkID = u32MidChunkID;

	printf("SeekAudioChunkID finetune %d, time %d \n", u32MidChunkID, (uint32_t)u64ChunkTime);
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
SeekPlay(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	uint32_t u32SeekTime,
	uint32_t u32TotalVideoChunks,
	uint32_t u32TotalAudioChunks
)
{
	S_DEMUX_PRIV *psDeMuxPriv = (S_DEMUX_PRIV *)psDeMuxRes->pvDeMuxPriv;
	if(psDeMuxRes->sMediaVideoCtx.eVideoType !=  eNM_CTX_VIDEO_NONE){
		SeekVideoChunk(psDeMuxRes->psMediaIF, psDeMuxRes->pvMediaRes, u32TotalVideoChunks, psDeMuxPriv, (uint64_t)u32SeekTime);
	}

	if(psDeMuxRes->sMediaAudioCtx.eAudioType !=  eNM_CTX_VIDEO_NONE){
		SeekAudioChunk(psDeMuxRes->psMediaIF, psDeMuxRes->pvMediaRes, u32TotalAudioChunks, psDeMuxPriv, (uint64_t)u32SeekTime);
	}

	return eNM_ERRNO_NONE;
}

static int
RunDeMuxCmd(
	S_PLAY_DEMUX_RES *psDeMuxRes
)
{
	S_DEMUX_PRIV *psDeMuxPriv = (S_DEMUX_PRIV *)psDeMuxRes->pvDeMuxPriv;
	int i32SemValue;

	pthread_mutex_lock(&psDeMuxPriv->tCmdMutex);

	if(psDeMuxPriv->eCmdCode == eDEMUX_IOCTL_NONE){
		sem_getvalue(&psDeMuxRes->tDeMuxCtrlSem, &i32SemValue);
		if(i32SemValue < 0){
			printf("%s, %s: Warning! post semaphore again! \n", __FILE__, __FUNCTION__);
			sem_post(&psDeMuxRes->tDeMuxCtrlSem);
		}
			
		pthread_mutex_unlock(&psDeMuxPriv->tCmdMutex);
		return 0;
	}

	psDeMuxPriv->eCmdRet = eNM_ERRNO_NONE;

	switch(psDeMuxPriv->eCmdCode){
		case eDEMUX_IOCTL_GET_STATE:
		{
			E_DEMUX_THREAD_STATE *peThreadState = (E_DEMUX_THREAD_STATE *)psDeMuxPriv->pvCmdArgv;
			*peThreadState = psDeMuxPriv->eDeMuxThreadState;
		}
		break;
		case eDEMUX_IOCTL_SET_STATE:
		{
			E_DEMUX_THREAD_STATE eThreadState = (E_DEMUX_THREAD_STATE)psDeMuxPriv->pvCmdArgv;
			psDeMuxPriv->eDeMuxThreadState = eThreadState;
		}
		break;
		case eDEMUX_IOCTL_SET_FF_SPEED:
		{
			E_NM_PLAY_FASTFORWARD_SPEED eSpeed = (E_NM_PLAY_FASTFORWARD_SPEED)psDeMuxPriv->pvCmdArgv;
			psDeMuxPriv->eFFSpeed = eSpeed;

			if(psDeMuxPriv->eDeMuxThreadState == eDEMUX_THREAD_STATE_PLAYING){
				psDeMuxPriv->eDeMuxThreadState = eDEMUX_THREAD_STATE_TO_PLAY;
			}
		}
		break;
		case eDEMUX_IOCTL_SET_SEEK:
		{
			S_DEMUX_IOCTL_SEEK *psSeekArgv = (S_DEMUX_IOCTL_SEEK *)psDeMuxPriv->pvCmdArgv;
			
			psDeMuxPriv->eCmdRet = SeekPlay(psDeMuxRes, psSeekArgv->u32SeekTime, psSeekArgv->u32TotalVideoChunks, psSeekArgv->u32TotalAudioChunks);
			free(psSeekArgv);
			
			if((psDeMuxPriv->eCmdRet == eNM_ERRNO_NONE) && (psDeMuxPriv->eDeMuxThreadState == eDEMUX_THREAD_STATE_PLAYING)){
				psDeMuxPriv->eDeMuxThreadState = eDEMUX_THREAD_STATE_TO_PLAY;
			}
		}
		break;
		case eDEMUX_IOCTL_SET_PLAY:
		{
			if(psDeMuxPriv->eDeMuxThreadState != eDEMUX_THREAD_STATE_PLAYING)
				psDeMuxPriv->eDeMuxThreadState = eDEMUX_THREAD_STATE_TO_PLAY;
		}
		break;
		case eDEMUX_IOCTL_SET_PAUSE:
		{
			if(psDeMuxPriv->eDeMuxThreadState != eDEMUX_THREAD_STATE_IDLE)
				psDeMuxPriv->eDeMuxThreadState = eDEMUX_THREAD_STATE_PAUSE;
		}
		break;
	}
	
	psDeMuxPriv->eCmdCode = eDEMUX_IOCTL_NONE;

	pthread_mutex_unlock(&psDeMuxPriv->tCmdMutex);
	
	sem_getvalue(&psDeMuxRes->tDeMuxCtrlSem,	&i32SemValue);

   /* If semaphore value is equal or larger than zero, there is no
     * thread waiting for this semaphore. Otherwise (<0), call FreeRTOS interface
     * to wake up a thread. */	

	if(i32SemValue < 0)
		sem_post(&psDeMuxRes->tDeMuxCtrlSem);
	return 0;
}

static E_NM_ERRNO
ReadVideoChunk(
	uint32_t u32ChunkID,
	S_NM_MEDIAREAD_IF *psMediaIF,
	void *pvMediaRes,
	S_NM_VIDEO_CTX *psVideoCtx,
	S_NM_FRAME_DATA *psAVFrameData
)
{
	uint32_t u32ChunkSize;
	uint64_t u64ChunkTime;
//	bool bSeekable;
	E_NM_ERRNO eRet;
	
//	eRet = psMediaIF->pfnGetVideoChunkInfo(u32ChunkID, &u32ChunkSize, &u64ChunkTime, &bSeekable, pvMediaRes);
	eRet = psMediaIF->pfnGetVideoChunkInfo(u32ChunkID, &u32ChunkSize, &u64ChunkTime, NULL, pvMediaRes);
	if(eRet != eNM_ERRNO_NONE)
		return eRet;

	if(u32ChunkSize > psAVFrameData->u32ChunkLimit){
		//realloc memory
		uint8_t *pu8Temp = malloc(u32ChunkSize + 1024);

		if(pu8Temp == NULL){
			NMLOG_ERROR("Unable alloc memeory \n");
			return eNM_ERRNO_MALLOC;
		}

		if(psAVFrameData->pu8ChunkBuf)
			free(psAVFrameData->pu8ChunkBuf);

		psAVFrameData->pu8ChunkBuf = pu8Temp;
		psAVFrameData->u32ChunkLimit = u32ChunkSize;		
	}
	
	psVideoCtx->pu8DataBuf = psAVFrameData->pu8ChunkBuf;
	psVideoCtx->u32DataLimit = psAVFrameData->u32ChunkLimit;
	
	eRet = psMediaIF->pfnReadVideoChunk(u32ChunkID, psVideoCtx, pvMediaRes);
	if(eRet != eNM_ERRNO_NONE)
		return eRet;

	psAVFrameData->u32ChunkSize = psVideoCtx->u32DataSize;
	psAVFrameData->u64ChunkPTS = psVideoCtx->u64DataTime;
	psAVFrameData->u64ChunkDataTime = psVideoCtx->u64DataTime;
//	psAVFrameData->bKeyFrame = bSeekable;
	psAVFrameData->bKeyFrame = psVideoCtx->bKeyFrame;

//	if(bSeekable)
//		INFO("Video key frame %lld \n", u64ChunkTime);
	return eNM_ERRNO_NONE;
}

static E_NM_ERRNO
ReadAudioChunk(
	uint32_t u32ChunkID,
	S_NM_MEDIAREAD_IF *psMediaIF,
	void *pvMediaRes,
	S_NM_AUDIO_CTX *psAudioCtx,
	S_NM_FRAME_DATA *psAVFrameData
)
{
	uint32_t u32ChunkSize;
	uint64_t u64ChunkTime;
//	bool bSeekable;
	E_NM_ERRNO eRet;

//	eRet = psMediaIF->pfnGetAudioChunkInfo(u32ChunkID, &u32ChunkSize, &u64ChunkTime, &bSeekable, pvMediaRes);
	eRet = psMediaIF->pfnGetAudioChunkInfo(u32ChunkID, &u32ChunkSize, &u64ChunkTime, NULL, pvMediaRes);
	if(eRet != eNM_ERRNO_NONE)
		return eRet;

	if(u32ChunkSize > psAVFrameData->u32ChunkLimit){
		//realloc memory
		uint8_t *pu8Temp = malloc(u32ChunkSize + 1024);

		if(pu8Temp == NULL){
			NMLOG_ERROR("Unable alloc memeory \n");
			return eNM_ERRNO_MALLOC;
		}

		if(psAVFrameData->pu8ChunkBuf)
			free(psAVFrameData->pu8ChunkBuf);

		psAVFrameData->pu8ChunkBuf = pu8Temp;
		psAVFrameData->u32ChunkLimit = u32ChunkSize;		
	}
	
	psAudioCtx->pu8DataBuf = psAVFrameData->pu8ChunkBuf;
	psAudioCtx->u32DataLimit = psAVFrameData->u32ChunkLimit;
	
	eRet = psMediaIF->pfnReadAudioChunk(u32ChunkID, psAudioCtx, pvMediaRes);
	if(eRet != eNM_ERRNO_NONE)
		return eRet;

	psAVFrameData->u32ChunkSize = psAudioCtx->u32DataSize;
	psAVFrameData->u64ChunkPTS = psAudioCtx->u64DataTime;
	psAVFrameData->u64ChunkDataTime = psAudioCtx->u64DataTime;
	psAVFrameData->bKeyFrame = psAudioCtx->bKeyFrame;
	return eNM_ERRNO_NONE;
}

static int
PushVideoFrameData(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	S_NM_FRAME_DATA *psFrameData
)
{
	void *pvVideoList = psDeMuxRes->psPlayEngRes->pvVideoList;

	if(pvVideoList == NULL)
		return 0;
		
	NMVideoList_PutVideoData(psFrameData, pvVideoList); 
	
	return 0;
}

static int
PushAudioFrameData(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	S_NM_FRAME_DATA *psFrameData
)
{
	void *pvAudioList = psDeMuxRes->psPlayEngRes->pvAudioList;

	if(pvAudioList == NULL)
		return 0;
		
	NMAudioList_PutAudioData(psFrameData, pvAudioList); 
	
	return 0;
}


static void * DeMuxWorkerThread( void * pvArgs )
{
	S_PLAY_DEMUX_RES *psDeMuxRes = (S_PLAY_DEMUX_RES *)pvArgs;
	S_DEMUX_PRIV *psDeMuxPriv = (S_DEMUX_PRIV *)psDeMuxRes->pvDeMuxPriv;

	uint64_t u64CurrentTime;
	uint64_t u64StartPlayTime;
	uint64_t u64PlayTimeOffset;

	uint64_t u64CurAudChunkPTS = 0;		//present timestamp
	uint64_t u64CurVidChunkPTS = 0;		//present timestamp

	S_NM_FRAME_DATA *psVideoFrameData;
	S_NM_FRAME_DATA *psAudioFrameData;

	int32_t i32VideoFrameIdx;
	int32_t i32AudioFrameIdx;

	E_NM_ERRNO eNMRet;
	bool bVideoPlaying = false;
	bool bAudioPlaying = false;	
	
	psDeMuxPriv->eDeMuxThreadState = eDEMUX_THREAD_STATE_IDLE;

//for testing
#if _DEMUX_DEBUG_
	uint64_t u64PrevAudioDataTime = 0;
	uint64_t u64PrevVideoDataTime = 0;
	uint64_t u64StartEachReadTime = 0;
	uint64_t u64StartReadAudioTime = 0;
#endif


	while(psDeMuxPriv->eDeMuxThreadState != eDEMUX_THREAD_STATE_TO_EXIT){
		RunDeMuxCmd(psDeMuxRes);
		u64CurrentTime = NMUtil_GetTimeMilliSec();
	
		if((psDeMuxPriv->eDeMuxThreadState == eDEMUX_THREAD_STATE_IDLE) && (psDeMuxPriv->eDeMuxThreadState == eDEMUX_THREAD_STATE_PAUSE)){
			usleep(1000);
			continue;
		}

		if(psDeMuxPriv->eDeMuxThreadState == eDEMUX_THREAD_STATE_TO_PLAY){
			u64StartPlayTime = u64CurrentTime;
			u64CurAudChunkPTS = MAX_PTS;
			u64CurVidChunkPTS = MAX_PTS;
			u64PlayTimeOffset = MAX_PTS;
			bVideoPlaying = false;
			bAudioPlaying = false;	
		
			i32VideoFrameIdx = 0;
			i32AudioFrameIdx = 0;
			
			psVideoFrameData = &psDeMuxPriv->asVideoFrameData[i32VideoFrameIdx];
			psAudioFrameData = &psDeMuxPriv->asAudioFrameData[i32AudioFrameIdx];

			if(psDeMuxRes->sMediaVideoCtx.eVideoType !=  eNM_CTX_VIDEO_NONE){
				//Get and read first chunk information
				eNMRet = ReadVideoChunk(psDeMuxPriv->u32CurVidChunkID, psDeMuxRes->psMediaIF, psDeMuxRes->pvMediaRes, &psDeMuxRes->sMediaVideoCtx, psVideoFrameData);
				if(eNMRet == eNM_ERRNO_NONE){
					u64PlayTimeOffset = psVideoFrameData->u64ChunkDataTime;
					u64CurVidChunkPTS =  psVideoFrameData->u64ChunkPTS;
					bVideoPlaying = true;
				}
			}

			if(psDeMuxRes->sMediaAudioCtx.eAudioType != eNM_CTX_AUDIO_NONE){
				//Get and read first chunk information
				eNMRet = ReadAudioChunk(psDeMuxPriv->u32CurAudChunkID, psDeMuxRes->psMediaIF, psDeMuxRes->pvMediaRes, &psDeMuxRes->sMediaAudioCtx, psAudioFrameData); 
				if(eNMRet == eNM_ERRNO_NONE){
					if(u64PlayTimeOffset > psAudioFrameData->u64ChunkDataTime)
						u64PlayTimeOffset = psAudioFrameData->u64ChunkDataTime;
					u64CurAudChunkPTS =  psAudioFrameData->u64ChunkPTS;
					bAudioPlaying = true;
				}
			}

			//fine tune present timestamp 
			if(u64CurVidChunkPTS != MAX_PTS){
				u64CurVidChunkPTS = u64StartPlayTime + ((psVideoFrameData->u64ChunkDataTime - u64PlayTimeOffset) / psDeMuxPriv->eFFSpeed);
			}

			if(u64CurAudChunkPTS != MAX_PTS){
				u64CurAudChunkPTS = u64StartPlayTime + ((psAudioFrameData->u64ChunkDataTime - u64PlayTimeOffset) / psDeMuxPriv->eFFSpeed);
			}
			
			psDeMuxPriv->eDeMuxThreadState = eDEMUX_THREAD_STATE_PLAYING;
			
		}

		if(psDeMuxPriv->eDeMuxThreadState == eDEMUX_THREAD_STATE_PLAYING){

#if _DEMUX_DEBUG_
			u64StartEachReadTime = 	NMUtil_GetTimeMilliSec();	
#endif
			
			if(bVideoPlaying){
				if((u64CurVidChunkPTS != MAX_PTS) && (u64CurrentTime > u64CurVidChunkPTS)){
					//To export Video chunk
					psVideoFrameData->u32ChunkID = psDeMuxPriv->u32CurVidChunkID;
					psVideoFrameData->u64ChunkPTS = u64CurVidChunkPTS;

					if(psVideoFrameData->u32ChunkSize){
						PushVideoFrameData(psDeMuxRes, psVideoFrameData);
					}
						
					i32VideoFrameIdx = (i32VideoFrameIdx + 1) %  MAX_AV_CHUNK_FRAMES;
					psVideoFrameData = &psDeMuxPriv->asVideoFrameData[i32VideoFrameIdx];
					u64CurVidChunkPTS = MAX_PTS;
					psDeMuxPriv->u32CurVidChunkID ++;

					eNMRet = ReadVideoChunk(psDeMuxPriv->u32CurVidChunkID, psDeMuxRes->psMediaIF, psDeMuxRes->pvMediaRes, &psDeMuxRes->sMediaVideoCtx, psVideoFrameData);

#if _DEMUX_DEBUG_
					NMLOG_DEBUG("Video each chunk diff time(chunk data time) %d, read time(I/O speed) %d \n", (uint32_t)(psVideoFrameData->u64ChunkDataTime - u64PrevVideoDataTime), (uint32_t)(NMUtil_GetTimeMilliSec() - u64StartEachReadTime));
					u64PrevVideoDataTime = psVideoFrameData->u64ChunkDataTime;
#endif
					
					if(eNMRet == eNM_ERRNO_NONE){
						u64CurVidChunkPTS =   u64StartPlayTime + ((psVideoFrameData->u64ChunkDataTime - u64PlayTimeOffset) / psDeMuxPriv->eFFSpeed);
					}
					else if(eNMRet == eNM_ERRNO_EOM){
						//end of video chunk
						NMLOG_INFO("End of video play \n");
						bVideoPlaying = false;
					}
				}
				else if(u64CurVidChunkPTS == MAX_PTS){
					//Previous chunk is incorrect, try to fetch next chunk
					u64CurVidChunkPTS = MAX_PTS;
					psDeMuxPriv->u32CurVidChunkID ++;

					eNMRet = ReadVideoChunk(psDeMuxPriv->u32CurVidChunkID, psDeMuxRes->psMediaIF, psDeMuxRes->pvMediaRes, &psDeMuxRes->sMediaVideoCtx, psVideoFrameData);

					if(eNMRet == eNM_ERRNO_NONE){
						u64CurVidChunkPTS =   u64StartPlayTime + ((psVideoFrameData->u64ChunkDataTime - u64PlayTimeOffset) / psDeMuxPriv->eFFSpeed);
					}
					else if(eNMRet == eNM_ERRNO_EOM){
						//end of video chunk
						NMLOG_INFO("End of video play \n");
						bVideoPlaying = false;
					}
				}
			}
			
			if(bAudioPlaying){
				if((u64CurAudChunkPTS != MAX_PTS) && (u64CurrentTime > u64CurAudChunkPTS)){
					//To export Audio chunk
					psAudioFrameData->u32ChunkID = psDeMuxPriv->u32CurAudChunkID;
					psAudioFrameData->u64ChunkPTS = u64CurAudChunkPTS;

					if(psAudioFrameData->u32ChunkSize)
						PushAudioFrameData(psDeMuxRes, psAudioFrameData);

					//Fetch next chaunk
					i32AudioFrameIdx = (i32AudioFrameIdx + 1) %  MAX_AV_CHUNK_FRAMES;
					psAudioFrameData = &psDeMuxPriv->asAudioFrameData[i32AudioFrameIdx];
					u64CurAudChunkPTS = MAX_PTS;
					psDeMuxPriv->u32CurAudChunkID ++;

#if _DEMUX_DEBUG_
					u64StartReadAudioTime = NMUtil_GetTimeMilliSec();
#endif
					
					eNMRet = ReadAudioChunk(psDeMuxPriv->u32CurAudChunkID, psDeMuxRes->psMediaIF, psDeMuxRes->pvMediaRes, &psDeMuxRes->sMediaAudioCtx, psAudioFrameData);

					if(eNMRet == eNM_ERRNO_NONE){
						u64CurAudChunkPTS =   u64StartPlayTime + ((psAudioFrameData->u64ChunkDataTime - u64PlayTimeOffset) / psDeMuxPriv->eFFSpeed);
					}
					else if(eNMRet == eNM_ERRNO_EOM){
						//end of audio chunk
						NMLOG_INFO("End of audio play \n");
						bAudioPlaying = false;
					}

#if _DEMUX_DEBUG_
					NMLOG_DEBUG("Audio each chunk diff time(chunk data time) %d, max read time(video+audio)(I/O speed) %d, audio read time %d \n", (uint32_t)(psAudioFrameData->u64ChunkDataTime - u64PrevAudioDataTime), 
																																																																		(uint32_t)(NMUtil_GetTimeMilliSec() - u64StartEachReadTime),
																																																																		(uint32_t)(NMUtil_GetTimeMilliSec() - u64StartReadAudioTime));
					u64PrevAudioDataTime = psAudioFrameData->u64ChunkDataTime;
#endif
					
				}
				else if(u64CurAudChunkPTS == MAX_PTS){
					//Previous chunk is incorrect, try to fetch next chunk
					u64CurAudChunkPTS = MAX_PTS;
					psDeMuxPriv->u32CurAudChunkID ++;

					eNMRet = ReadAudioChunk(psDeMuxPriv->u32CurAudChunkID, psDeMuxRes->psMediaIF, psDeMuxRes->pvMediaRes, &psDeMuxRes->sMediaAudioCtx, psAudioFrameData);

					if(eNMRet == eNM_ERRNO_NONE){
						u64CurAudChunkPTS =   u64StartPlayTime + ((psAudioFrameData->u64ChunkDataTime - u64PlayTimeOffset) / psDeMuxPriv->eFFSpeed);
					}
					else if(eNMRet == eNM_ERRNO_EOM){
						//end of audio chunk
						NMLOG_INFO("End of audio play \n");
						bAudioPlaying = false;
					}
				}
			}

			if((bVideoPlaying == false) && (bAudioPlaying == false)){
				psDeMuxPriv->eDeMuxThreadState = eDEMUX_THREAD_STATE_EOM;
			}			

		}
		
		usleep(1000);
	}
		
	psDeMuxPriv->eDeMuxThreadState = eDEMUX_THREAD_STATE_EXIT;
	
	return NULL;
}

static E_NM_ERRNO
InitDeMuxPriv(
	S_PLAY_DEMUX_RES *psDeMuxRes
)
{
	E_NM_ERRNO eNMRet;
	S_DEMUX_PRIV *psDeMuxPriv = NULL;
	int i;
	uint32_t u32MediaImagePixels;
	uint8_t *pu8TempBuf;
	uint32_t u32MediaAudioSamples;

	psDeMuxPriv = calloc(1, sizeof(S_DEMUX_PRIV));

	if(psDeMuxPriv == NULL){
		eNMRet = eNM_ERRNO_MALLOC;
		goto InitDeMuxPriv_fail;
	}

	//Init command mutex
	if(pthread_mutex_init(&psDeMuxPriv->tCmdMutex, NULL) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto InitDeMuxPriv_fail;
	}
		
	u32MediaImagePixels = psDeMuxRes->sMediaVideoCtx.u32Width * psDeMuxRes->sMediaVideoCtx.u32Height;

	u32MediaAudioSamples = psDeMuxRes->sMediaAudioCtx.u32SamplePerBlock * 2;
	
	for(i = 0; i < MAX_AV_CHUNK_FRAMES; i ++){
		pu8TempBuf = malloc(u32MediaImagePixels + 100);
		
		if(pu8TempBuf == NULL){
			eNMRet = eNM_ERRNO_MALLOC;
			goto InitDeMuxPriv_fail;
		}
		
		psDeMuxPriv->asVideoFrameData[i].u32ChunkLimit = u32MediaImagePixels;
		psDeMuxPriv->asVideoFrameData[i].pu8ChunkBuf = pu8TempBuf;
	}

	for(i = 0; i < MAX_AV_CHUNK_FRAMES; i ++){
		pu8TempBuf = malloc(u32MediaAudioSamples + 100);
		
		if(pu8TempBuf == NULL){
			eNMRet = eNM_ERRNO_MALLOC;
			goto InitDeMuxPriv_fail;
		}
		
		psDeMuxPriv->asAudioFrameData[i].u32ChunkLimit = u32MediaAudioSamples;
		psDeMuxPriv->asAudioFrameData[i].pu8ChunkBuf = pu8TempBuf;
	}

	 
	psDeMuxPriv->eCmdCode = eDEMUX_IOCTL_NONE;	
	psDeMuxPriv->pvCmdArgv = NULL;	
	psDeMuxPriv->eFFSpeed = eNM_PLAY_FASTFORWARD_1X;
	psDeMuxPriv->eDeMuxThreadState = eDEMUX_THREAD_STATE_INIT;
	psDeMuxRes->pvDeMuxPriv = (void *)psDeMuxPriv;
	
	return eNM_ERRNO_NONE;
	
InitDeMuxPriv_fail:

	for(i = 0; i < MAX_AV_CHUNK_FRAMES; i ++){
		if(psDeMuxPriv->asVideoFrameData[i].pu8ChunkBuf){
			free(psDeMuxPriv->asVideoFrameData[i].pu8ChunkBuf);
			psDeMuxPriv->asVideoFrameData[i].pu8ChunkBuf = NULL;
		}
	}

	for(i = 0; i < MAX_AV_CHUNK_FRAMES; i ++){
		if(psDeMuxPriv->asAudioFrameData[i].pu8ChunkBuf){
			free(psDeMuxPriv->asAudioFrameData[i].pu8ChunkBuf);
			psDeMuxPriv->asAudioFrameData[i].pu8ChunkBuf = NULL;
		}
	}
	
	if(psDeMuxPriv){
		pthread_mutex_destroy(&psDeMuxPriv->tCmdMutex);
		free(psDeMuxPriv);
	}

	return eNMRet;
}

static void
FinialDeMuxPriv(
	S_PLAY_DEMUX_RES *psDeMuxRes
)
{
	S_DEMUX_PRIV *psDeMuxPriv;
	int i;
	
	if(psDeMuxRes->pvDeMuxPriv == NULL)
		return;
	
	psDeMuxPriv = (S_DEMUX_PRIV *)psDeMuxRes->pvDeMuxPriv;

	for(i = 0; i < MAX_AV_CHUNK_FRAMES; i ++){
		if(psDeMuxPriv->asVideoFrameData[i].pu8ChunkBuf){
			free(psDeMuxPriv->asVideoFrameData[i].pu8ChunkBuf);
			psDeMuxPriv->asVideoFrameData[i].pu8ChunkBuf = NULL;
		}
	}

	for(i = 0; i < MAX_AV_CHUNK_FRAMES; i ++){
		if(psDeMuxPriv->asAudioFrameData[i].pu8ChunkBuf){
			free(psDeMuxPriv->asAudioFrameData[i].pu8ChunkBuf);
			psDeMuxPriv->asAudioFrameData[i].pu8ChunkBuf = NULL;
		}
	}

	pthread_mutex_destroy(&psDeMuxPriv->tCmdMutex);
	free(psDeMuxPriv);
	psDeMuxRes->pvDeMuxPriv = NULL;
}

static E_NM_ERRNO
DeMuxThread_IOCTL(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	E_DEMUX_IOCTL_CODE eCmdCode,
	void *pvCmdArgv,
	int32_t i32CmdArgvSize,
	bool bBlocking
)
{
	E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
	S_DEMUX_PRIV *psDeMuxPriv;

	if(psDeMuxRes == NULL)
		return eNM_ERRNO_NULL_PTR;
	
	psDeMuxPriv = (S_DEMUX_PRIV *)psDeMuxRes->pvDeMuxPriv;
	if(psDeMuxPriv == NULL)
		return eNM_ERRNO_NONE;
	
	pthread_mutex_lock(&psDeMuxRes->tDeMuxCtrlMutex);
	pthread_mutex_lock(&psDeMuxPriv->tCmdMutex);

	//Argument size check
	switch (eCmdCode){
		case eDEMUX_IOCTL_GET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_DEMUX_THREAD_STATE *)){
				eNMRet = eNM_ERRNO_SIZE;
			}			
		}
		break;
		case eDEMUX_IOCTL_SET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_DEMUX_THREAD_STATE)){
				eNMRet = eNM_ERRNO_SIZE;
			}
		}
		break;
		case eDEMUX_IOCTL_SET_FF_SPEED:
		{
			if(i32CmdArgvSize < sizeof(E_NM_PLAY_FASTFORWARD_SPEED)){
				eNMRet = eNM_ERRNO_SIZE;
			}
		}
		break;
		case eDEMUX_IOCTL_SET_SEEK:
		{
			if(i32CmdArgvSize < sizeof(S_DEMUX_IOCTL_SEEK *)){
				eNMRet = eNM_ERRNO_SIZE;
			}
		}
		break;
		case eDEMUX_IOCTL_SET_PLAY:
		case eDEMUX_IOCTL_SET_PAUSE:
		{
		}
		break;
	}

	if(eNMRet == eNM_ERRNO_NONE){
		psDeMuxPriv->eCmdCode = eCmdCode;
		psDeMuxPriv->pvCmdArgv = pvCmdArgv;		

		pthread_mutex_unlock(&psDeMuxPriv->tCmdMutex);
		if(bBlocking){
			psDeMuxRes->i32SemLock = 1;
			sem_wait(&psDeMuxRes->tDeMuxCtrlSem);
			psDeMuxRes->i32SemLock = 0;
			eNMRet = psDeMuxPriv->eCmdRet;
		}
	}
	else{
		pthread_mutex_unlock(&psDeMuxPriv->tCmdMutex);
	}
	
	pthread_mutex_unlock(&psDeMuxRes->tDeMuxCtrlMutex);
	return eNMRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
PlayDeMux_Final(
	S_PLAY_DEMUX_RES *psDeMuxRes
)
{
	FinialDeMuxPriv(psDeMuxRes);
	pthread_mutex_destroy(&psDeMuxRes->tDeMuxCtrlMutex);
	sem_destroy(&psDeMuxRes->tDeMuxCtrlSem);
}


E_NM_ERRNO
PlayDeMux_Init(
	S_PLAY_DEMUX_RES *psDeMuxRes
)
{
	//Init mutex and semaphore
	E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
	
	//Init control mutex and semphore
	if(pthread_mutex_init(&psDeMuxRes->tDeMuxCtrlMutex, NULL) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto PlayDeMux_Init_fail;
	}
	
	if(sem_init(&psDeMuxRes->tDeMuxCtrlSem, 0, 0) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto PlayDeMux_Init_fail;
	}

	//Alloc demux private data
	eNMRet = InitDeMuxPriv(psDeMuxRes);
	if(eNMRet != eNM_ERRNO_NONE){
		goto PlayDeMux_Init_fail;
	}

	return eNM_ERRNO_NONE;

PlayDeMux_Init_fail:

	PlayDeMux_Final(psDeMuxRes);
	
	return eNMRet;
}

void
PlayDeMux_ThreadDestroy(
	S_PLAY_DEMUX_RES *psDeMuxRes
)
{
	E_DEMUX_THREAD_STATE eDeMuxThreadState = eDEMUX_THREAD_STATE_TO_EXIT;
	void *pvJoin;

	if(psDeMuxRes->tDeMuxThread){
			DeMuxThread_IOCTL(psDeMuxRes, eDEMUX_IOCTL_SET_STATE, (void *)eDeMuxThreadState, sizeof(E_DEMUX_THREAD_STATE), false);
			pthread_join(psDeMuxRes->tDeMuxThread, &pvJoin);
			psDeMuxRes->tDeMuxThread = NULL;
	}
}


E_NM_ERRNO
PlayDeMux_ThreadCreate(
	S_PLAY_DEMUX_RES *psDeMuxRes
)
{
	int iRet;
	E_DEMUX_THREAD_STATE eDeMuxThreadState;
	
	iRet = pthread_create(&psDeMuxRes->tDeMuxThread, NULL, DeMuxWorkerThread, psDeMuxRes);
	
	if(iRet != 0)
		return eNM_ERRNO_OS;

	//Wait thread running
	while(1){
		DeMuxThread_IOCTL(psDeMuxRes, eDEMUX_IOCTL_GET_STATE, &eDeMuxThreadState, sizeof(E_DEMUX_THREAD_STATE *), true);
		if(eDeMuxThreadState != eDEMUX_THREAD_STATE_INIT)
			break;
		usleep(1000);
	}

	
	return eNM_ERRNO_NONE;
}

E_NM_ERRNO
PlayDeMux_Play(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	bool bWait
)
{

	if(psDeMuxRes->tDeMuxThread == NULL)
		return eNM_ERRNO_OS;

	return DeMuxThread_IOCTL(psDeMuxRes, eDEMUX_IOCTL_SET_PLAY, NULL, 0, bWait);	
}

E_NM_ERRNO
PlayDeMux_Pause(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	bool bWait
)
{
	if(psDeMuxRes->tDeMuxThread == NULL)
		return eNM_ERRNO_OS;

	return DeMuxThread_IOCTL(psDeMuxRes, eDEMUX_IOCTL_SET_PAUSE, NULL, 0, bWait);
}

E_NM_ERRNO
PlayDeMux_Fastforward(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	E_NM_PLAY_FASTFORWARD_SPEED eSpeed,
	bool bWait
)
{

	if(psDeMuxRes->tDeMuxThread == NULL)
		return eNM_ERRNO_OS;
		
	return DeMuxThread_IOCTL(psDeMuxRes, eDEMUX_IOCTL_SET_FF_SPEED, (void *)eSpeed, sizeof(E_NM_PLAY_FASTFORWARD_SPEED), bWait);
}

E_NM_ERRNO
PlayDeMux_Seek(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	uint32_t u32SeekTime,
	uint32_t u32TotalVideoChunks,
	uint32_t u32TotalAudioChunks,
	bool bWait
)
{
	S_DEMUX_IOCTL_SEEK *psSeekArgv;
	
	psSeekArgv = malloc(sizeof(S_DEMUX_IOCTL_SEEK));
	if(psSeekArgv == NULL)
		return eNM_ERRNO_MALLOC;
	
	psSeekArgv->u32SeekTime = u32SeekTime;
	psSeekArgv->u32TotalVideoChunks = u32TotalVideoChunks;
	psSeekArgv->u32TotalAudioChunks = u32TotalAudioChunks;
	
	if(psDeMuxRes->tDeMuxThread == NULL)
		return eNM_ERRNO_OS;

	return DeMuxThread_IOCTL(psDeMuxRes, eDEMUX_IOCTL_SET_SEEK, (void *)psSeekArgv, sizeof(S_DEMUX_IOCTL_SEEK *), bWait);
}

E_NM_PLAY_STATUS
PlayDeMux_Status(
	S_PLAY_DEMUX_RES *psDeMuxRes
)
{
	E_DEMUX_THREAD_STATE eDeMuxThreadState;

	if(psDeMuxRes->tDeMuxThread == NULL)
		return eNM_PLAY_STATUS_ERROR;

	DeMuxThread_IOCTL(psDeMuxRes, eDEMUX_IOCTL_GET_STATE, (void *)&eDeMuxThreadState, sizeof(E_DEMUX_THREAD_STATE *), true);
	
	if((eDeMuxThreadState == eDEMUX_THREAD_STATE_INIT) ||
			(eDeMuxThreadState == eDEMUX_THREAD_STATE_IDLE) ||
			(eDeMuxThreadState == eDEMUX_THREAD_STATE_PAUSE))
		return eNM_PLAY_STATUS_PAUSE;

	if((eDeMuxThreadState == eDEMUX_THREAD_STATE_TO_EXIT) ||
			(eDeMuxThreadState == eDEMUX_THREAD_STATE_EXIT) ||
			(eDeMuxThreadState == eDEMUX_THREAD_STATE_EOM))
		return eNM_PLAY_STATUS_EOM;

	return eNM_PLAY_STATUS_PLAYING;
}


