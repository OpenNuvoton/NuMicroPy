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

#include "RecordEncode.h"

#include "Util/NMVideoList.h"
#include "Util/NMAudioList.h"

#define MAX_ENCODED_FRAMES	1
#define _ENCODE_DEBUG_ 0

typedef enum {
	eENCODE_THREAD_STATE_INIT,
	eENCODE_THREAD_STATE_IDLE,
	eENCODE_THREAD_STATE_RUN,
	eENCODE_THREAD_STATE_TO_EXIT,
	eENCODE_THREAD_STATE_EXIT,
} E_ENCODE_THREAD_STATE;

typedef struct {
	uint8_t *pu8FrameData;
	uint32_t u32FrameDataLimit;
	uint64_t u64PTS;	//present time
}S_ENCODE_FRAME;

typedef struct {
	E_ENCODE_THREAD_STATE eEncThreadState;

	E_ENCODE_IOCTL_CODE	eCmdCode;
	void *pvCmdArgv;
	E_NM_ERRNO eCmdRet;
	pthread_mutex_t tCmdMutex;

	S_ENCODE_FRAME asEncodeFrame[MAX_ENCODED_FRAMES];
	
} S_VIDEO_ENC_PRIV;

typedef struct {
	E_ENCODE_THREAD_STATE eEncThreadState;

	E_ENCODE_IOCTL_CODE	eCmdCode;
	void *pvCmdArgv;
	E_NM_ERRNO eCmdRet;
	pthread_mutex_t tCmdMutex;

	S_ENCODE_FRAME asEncodeFrame[MAX_ENCODED_FRAMES];
	
	uint8_t *pu8PCMBuf;
	uint32_t u32PCMBufLimit;

} S_AUDIO_ENC_PRIV;

static int
RunVideoEncCmd(
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes
)
{
	S_VIDEO_ENC_PRIV *psVideoEncPriv = (S_VIDEO_ENC_PRIV *)psVideoEncodeRes->pvVideoEncPriv;
	int i32SemValue;

	pthread_mutex_lock(&psVideoEncPriv->tCmdMutex);

	if(psVideoEncPriv->eCmdCode == eENCODE_IOCTL_NONE){
		sem_getvalue(&psVideoEncodeRes->tVideoEncodeCtrlSem, &i32SemValue);
		if(i32SemValue < 0){
			printf("%s, %s: Error! some thing wrong! \n", __FILE__, __FUNCTION__);
			sem_post(&psVideoEncodeRes->tVideoEncodeCtrlSem);
		}
			
		pthread_mutex_unlock(&psVideoEncPriv->tCmdMutex);
		return 0;
	}

	psVideoEncPriv->eCmdRet = eNM_ERRNO_NONE;
	
	switch(psVideoEncPriv->eCmdCode){
		case eENCODE_IOCTL_GET_STATE:
		{
			E_ENCODE_THREAD_STATE *peThreadState = (E_ENCODE_THREAD_STATE *)psVideoEncPriv->pvCmdArgv;
			*peThreadState = psVideoEncPriv->eEncThreadState;
		}
		break;
		case eENCODE_IOCTL_SET_STATE:
		{
			E_ENCODE_THREAD_STATE eThreadState = (E_ENCODE_THREAD_STATE)psVideoEncPriv->pvCmdArgv;
			psVideoEncPriv->eEncThreadState = eThreadState;
		}
		break;
	}
	
	psVideoEncPriv->eCmdCode = eENCODE_IOCTL_NONE;
	pthread_mutex_unlock(&psVideoEncPriv->tCmdMutex);

	sem_getvalue(&psVideoEncodeRes->tVideoEncodeCtrlSem,	&i32SemValue);

   /* If semaphore value is equal or larger than zero, there is no
     * thread waiting for this semaphore. Otherwise (<0), call FreeRTOS interface
     * to wake up a thread. */	

	if(i32SemValue < 0)
		sem_post(&psVideoEncodeRes->tVideoEncodeCtrlSem);

	return 0;
}

static int
RunAudioEncCmd(
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes
)
{
	S_AUDIO_ENC_PRIV *psAudioEncPriv = (S_AUDIO_ENC_PRIV *)psAudioEncodeRes->pvAudioEncPriv;

	int i32SemValue;

	pthread_mutex_lock(&psAudioEncPriv->tCmdMutex);

	if(psAudioEncPriv->eCmdCode == eENCODE_IOCTL_NONE){
		sem_getvalue(&psAudioEncodeRes->tAudioEncodeCtrlSem, &i32SemValue);

		if(i32SemValue < 0){
			printf("%s, %s: Error! some thing wrong! \n", __FILE__, __FUNCTION__);
			sem_post(&psAudioEncodeRes->tAudioEncodeCtrlSem);
		}
			
		pthread_mutex_unlock(&psAudioEncPriv->tCmdMutex);
		return 0;
	}

	psAudioEncPriv->eCmdRet = eNM_ERRNO_NONE;
	
	switch(psAudioEncPriv->eCmdCode){
		case eENCODE_IOCTL_GET_STATE:
		{
			E_ENCODE_THREAD_STATE *peThreadState = (E_ENCODE_THREAD_STATE *)psAudioEncPriv->pvCmdArgv;
			*peThreadState = psAudioEncPriv->eEncThreadState;
		}
		break;
		case eENCODE_IOCTL_SET_STATE:
		{
			E_ENCODE_THREAD_STATE eThreadState = (E_ENCODE_THREAD_STATE)psAudioEncPriv->pvCmdArgv;
			psAudioEncPriv->eEncThreadState = eThreadState;
		}
		break;
	}
	
	psAudioEncPriv->eCmdCode = eENCODE_IOCTL_NONE;
	pthread_mutex_unlock(&psAudioEncPriv->tCmdMutex);
	
	sem_getvalue(&psAudioEncodeRes->tAudioEncodeCtrlSem,	&i32SemValue);

   /* If semaphore value is equal or larger than zero, there is no
     * thread waiting for this semaphore. Otherwise (<0), call FreeRTOS interface
     * to wake up a thread. */	

	if(i32SemValue < 0)
		sem_post(&psAudioEncodeRes->tAudioEncodeCtrlSem);

	return 0;
}

static void * VideoEnocdeWorkerThread( void * pvArgs )
{
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes = (S_RECORD_VIDEO_ENCODE_RES *)pvArgs;
	S_VIDEO_ENC_PRIV *psVideoEncPriv = (S_VIDEO_ENC_PRIV *)psVideoEncodeRes->pvVideoEncPriv;
	void *pvVideoList;
	uint64_t u64CurrentTime;
	uint64_t u64FillTime = 0;
	uint64_t u64VideoInPeriod;
	S_NM_VIDEO_CTX *psVideoMediaCtx = NULL;
	S_NM_VIDEO_CTX *psVideoFillCtx = NULL;
	uint32_t u32EncodedFrameIndex = 0;
	E_NM_ERRNO eNMRet;
	uint32_t u32RemainDataSize;
	S_NM_FRAME_DATA sVideoFrameData;
	bool bPutVideoList;
	
	psVideoEncPriv->eEncThreadState = eENCODE_THREAD_STATE_IDLE;
	pvVideoList = psVideoEncodeRes->psRecordEncodeRes->psRecordEngRes->pvVideoList;
	psVideoMediaCtx = &psVideoEncodeRes->sMediaVideoCtx;
	psVideoFillCtx = &psVideoEncodeRes->sFillVideoCtx;

//	u64VideoInPeriod = ((1000 / psVideoMediaCtx->u32FrameRate) * 90) / 100;
	u64VideoInPeriod = ((1000 / psVideoMediaCtx->u32FrameRate) * 50) / 100;
	if(u64VideoInPeriod == 0){
		u64VideoInPeriod = 10;
	}

#if _ENCODE_DEBUG_
	uint64_t u64StartEncTime;
#endif

	
	while(psVideoEncPriv->eEncThreadState != eENCODE_THREAD_STATE_TO_EXIT){

		RunVideoEncCmd(psVideoEncodeRes);
		u64CurrentTime = NMUtil_GetTimeMilliSec();
		
		if(psVideoEncPriv->eEncThreadState == eENCODE_THREAD_STATE_IDLE){
			usleep(1000);
			continue;
		}

		if(u64FillTime == 0){
			u64FillTime = u64CurrentTime + u64VideoInPeriod;
		}
		
		//fetch video in 
		if(u64CurrentTime >= u64FillTime){
			if(psVideoEncodeRes->pfnVideoFill(psVideoFillCtx) == false){
				usleep(1000);
				continue;
			}
		}
		else{
				usleep(1000);
				continue;
		}

		//encode
		psVideoMediaCtx->pu8DataBuf = psVideoEncPriv->asEncodeFrame[u32EncodedFrameIndex].pu8FrameData;
		psVideoMediaCtx->u32DataSize = 0;
		psVideoMediaCtx->u32DataLimit = psVideoEncPriv->asEncodeFrame[u32EncodedFrameIndex].u32FrameDataLimit;
		psVideoMediaCtx->u64DataTime = psVideoFillCtx->u64DataTime;
		
		u32RemainDataSize = 0;
		bPutVideoList = false;
		
		if(psVideoEncodeRes->pvVideoCodecRes){

#if _ENCODE_DEBUG_
			u64StartEncTime = NMUtil_GetTimeMilliSec();
#endif		

			eNMRet = psVideoEncodeRes->psVideoCodecIF->pfnEncodeVideo(psVideoFillCtx, psVideoMediaCtx, &u32RemainDataSize, psVideoEncodeRes->pvVideoCodecRes);
			bPutVideoList = true;

#if _ENCODE_DEBUG_
			NMLOG_DEBUG("Video each chunk encode time %d \n", (uint32_t)(NMUtil_GetTimeMilliSec() - u64StartEncTime));
#endif		
			
			//check remain data size
			if((eNMRet == eNM_ERRNO_NONE) && (u32RemainDataSize)){
				//realloc new encode buffer
				uint8_t *pu8TempBuf = NULL;
				uint32_t u32TempBufSize = psVideoMediaCtx->u32DataSize + u32RemainDataSize;
				
				pu8TempBuf = realloc(psVideoEncPriv->asEncodeFrame[u32EncodedFrameIndex].pu8FrameData, u32TempBufSize + 100);
				
				if(pu8TempBuf){
					uint32_t u32TempDataSize = psVideoMediaCtx->u32DataSize;
					psVideoEncPriv->asEncodeFrame[u32EncodedFrameIndex].pu8FrameData = pu8TempBuf;
					psVideoEncPriv->asEncodeFrame[u32EncodedFrameIndex].u32FrameDataLimit = u32TempBufSize;
					
					psVideoMediaCtx->pu8DataBuf = pu8TempBuf + psVideoMediaCtx->u32DataSize;
					psVideoMediaCtx->u32DataLimit = u32TempBufSize - psVideoMediaCtx->u32DataSize;
					psVideoMediaCtx->u32DataSize = 0;
//					psVideoMediaCtx->u64DataTime = psVideoFillCtx->u64DataTime;
					
					//Read Remain data
					psVideoEncodeRes->psVideoCodecIF->pfnEncodeVideo(NULL, psVideoMediaCtx, &u32RemainDataSize, psVideoEncodeRes->pvVideoCodecRes);

					psVideoMediaCtx->pu8DataBuf = pu8TempBuf;
					psVideoMediaCtx->u32DataSize += u32TempDataSize;
					psVideoMediaCtx->u32DataLimit = u32TempBufSize;
					bPutVideoList = true;
				}
				else{
					NMLOG_ERROR("Unable realloc buffer \n");
					bPutVideoList = false;
				}
			}
		}
		
		//Put it to video list
		if(bPutVideoList == true){
			sVideoFrameData.pu8ChunkBuf = psVideoMediaCtx->pu8DataBuf;
			sVideoFrameData.u32ChunkLimit = psVideoMediaCtx->u32DataLimit;		
			sVideoFrameData.u32ChunkSize = psVideoMediaCtx->u32DataSize;
			sVideoFrameData.u64ChunkPTS = psVideoMediaCtx->u64DataTime;
			sVideoFrameData.u64ChunkDataTime = psVideoMediaCtx->u64DataTime;

			if(pvVideoList){
					NMVideoList_PutVideoData(&sVideoFrameData, pvVideoList); 
			}
		}
		
		u32EncodedFrameIndex = (u32EncodedFrameIndex + 1) % MAX_ENCODED_FRAMES;
				
		//Update next fetch time
		u64FillTime += u64VideoInPeriod;
		if(u64FillTime < u64CurrentTime)
			u64FillTime = u64CurrentTime;
		
		usleep(1000);	
	}

	psVideoEncPriv->eEncThreadState = eENCODE_THREAD_STATE_EXIT;
	return NULL;
}

static void * AudioEnocdeWorkerThread( void * pvArgs )
{
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes = (S_RECORD_AUDIO_ENCODE_RES *)pvArgs;
	S_AUDIO_ENC_PRIV *psAudioEncPriv = (S_AUDIO_ENC_PRIV *)psAudioEncodeRes->pvAudioEncPriv;
	void *pvAudioList;
	S_NM_AUDIO_CTX *psAudioMediaCtx = NULL;
	S_NM_AUDIO_CTX *psAudioFillCtx = NULL;
	uint32_t u32EncodedFrameIndex = 0;
	uint32_t u32RemainDataSize;
	bool bPutAudioList;
	E_NM_ERRNO eNMRet;
	S_NM_FRAME_DATA sAudioFrameData;
	
	psAudioEncPriv->eEncThreadState = eENCODE_THREAD_STATE_IDLE;
	pvAudioList = psAudioEncodeRes->psRecordEncodeRes->psRecordEngRes->pvAudioList;
	psAudioMediaCtx = &psAudioEncodeRes->sMediaAudioCtx;
	psAudioFillCtx = &psAudioEncodeRes->sFillAudioCtx;

	while(psAudioEncPriv->eEncThreadState != eENCODE_THREAD_STATE_TO_EXIT){

		RunAudioEncCmd(psAudioEncodeRes);

		if(psAudioEncPriv->eEncThreadState == eENCODE_THREAD_STATE_IDLE){
			usleep(1000);
			continue;
		}

		psAudioFillCtx->pu8DataBuf = psAudioEncPriv->pu8PCMBuf;
		psAudioFillCtx->u32DataLimit = psAudioEncPriv->u32PCMBufLimit;
		
		///Each fill must be one encode block (psAudioFillCtx->u32SamplePerBlock)
		if(psAudioEncodeRes->pfnAudioFill(psAudioFillCtx) == false){
			usleep(1000);
			continue;
		}
		
		//encode
		psAudioMediaCtx->pu8DataBuf = psAudioEncPriv->asEncodeFrame[u32EncodedFrameIndex].pu8FrameData;
		psAudioMediaCtx->u32DataSize = 0;
		psAudioMediaCtx->u32DataLimit = psAudioEncPriv->asEncodeFrame[u32EncodedFrameIndex].u32FrameDataLimit;
		psAudioMediaCtx->u64DataTime = psAudioFillCtx->u64DataTime;
		
		u32RemainDataSize = 0;
		bPutAudioList = false;

		if(psAudioEncodeRes->pvAudioCodecRes){
			eNMRet = psAudioEncodeRes->psAudioCodecIF->pfnEncodeAudio(psAudioFillCtx, psAudioMediaCtx, &u32RemainDataSize, psAudioEncodeRes->pvAudioCodecRes);
			bPutAudioList = true;

			//check remain data size
			if((eNMRet == eNM_ERRNO_NONE) && (u32RemainDataSize)){
				//realloc new encode buffer
				uint8_t *pu8TempBuf = NULL;
				uint32_t u32TempBufSize = psAudioMediaCtx->u32DataSize + u32RemainDataSize;
			
				if(pu8TempBuf){
					uint32_t u32TempDataSize = psAudioMediaCtx->u32DataSize;
					
					psAudioEncPriv->asEncodeFrame[u32EncodedFrameIndex].pu8FrameData = pu8TempBuf;
					psAudioEncPriv->asEncodeFrame[u32EncodedFrameIndex].u32FrameDataLimit = u32TempBufSize;
					
					psAudioMediaCtx->pu8DataBuf = pu8TempBuf + psAudioMediaCtx->u32DataSize;
					psAudioMediaCtx->u32DataLimit = u32TempBufSize - psAudioMediaCtx->u32DataSize;
					psAudioMediaCtx->u32DataSize = 0;
//					psAudioMediaCtx->u64DataTime = psAudioFillCtx->u64DataTime;
					
					//Read Remain data
					psAudioEncodeRes->psAudioCodecIF->pfnEncodeAudio(NULL, psAudioMediaCtx, &u32RemainDataSize, psAudioEncodeRes->pvAudioCodecRes);

					psAudioMediaCtx->pu8DataBuf = pu8TempBuf;
					psAudioMediaCtx->u32DataSize += u32TempDataSize;
					psAudioMediaCtx->u32DataLimit = u32TempBufSize;
					bPutAudioList = true;
				}
				else{
					NMLOG_ERROR("Unable realloc buffer \n");
					bPutAudioList = false;
				}
			}
		}
			
		//Put it to video list
		if(bPutAudioList == true){
			sAudioFrameData.pu8ChunkBuf = psAudioMediaCtx->pu8DataBuf;
			sAudioFrameData.u32ChunkLimit = psAudioMediaCtx->u32DataLimit;		
			sAudioFrameData.u32ChunkSize = psAudioMediaCtx->u32DataSize;
			sAudioFrameData.u64ChunkPTS = psAudioMediaCtx->u64DataTime;
			sAudioFrameData.u64ChunkDataTime = psAudioMediaCtx->u64DataTime;

			if(pvAudioList){
					NMAudioList_PutAudioData(&sAudioFrameData, pvAudioList); 
			}
		}

		u32EncodedFrameIndex = (u32EncodedFrameIndex + 1) % MAX_ENCODED_FRAMES;
		
		usleep(5000);	
	}

	psAudioEncPriv->eEncThreadState = eENCODE_THREAD_STATE_EXIT;
	return NULL;
}


static E_NM_ERRNO
InitVideoEncPriv(
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes
)
{
	E_NM_ERRNO eNMRet;
	S_VIDEO_ENC_PRIV *psVideoEncPriv = NULL;
	int i;
	uint32_t u32BSBufDataSize;
	uint8_t *pu8TempBuf;

	if(psVideoEncodeRes->psVideoCodecIF == NULL){
		psVideoEncodeRes->pvVideoEncPriv = NULL;
		return eNM_ERRNO_NONE;
	}
	
	//Alloc decoder private data
	psVideoEncPriv = calloc(1, sizeof(S_VIDEO_ENC_PRIV));

	if(psVideoEncPriv == NULL){
		eNMRet = eNM_ERRNO_MALLOC;
		goto InitVideoEncPriv_fail;
	}

	//Init command mutex
	if(pthread_mutex_init(&psVideoEncPriv->tCmdMutex, NULL) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto InitVideoEncPriv_fail;
	}
		
	u32BSBufDataSize = psVideoEncodeRes->sFillVideoCtx.u32Width * psVideoEncodeRes->sFillVideoCtx.u32Height;

	for(i = 0; i < MAX_ENCODED_FRAMES; i ++){
		pu8TempBuf = malloc(u32BSBufDataSize + 100);
		
		if(pu8TempBuf == NULL){
			eNMRet = eNM_ERRNO_MALLOC;
			goto InitVideoEncPriv_fail;
		}
		
		psVideoEncPriv->asEncodeFrame[i].u32FrameDataLimit = u32BSBufDataSize;
		psVideoEncPriv->asEncodeFrame[i].pu8FrameData = pu8TempBuf;
	}

	psVideoEncPriv->eCmdCode = eENCODE_IOCTL_NONE;
	psVideoEncPriv->pvCmdArgv = NULL;	
	psVideoEncPriv->eEncThreadState = eENCODE_THREAD_STATE_INIT;
	psVideoEncodeRes->pvVideoEncPriv = (void *)psVideoEncPriv;
	
	return eNM_ERRNO_NONE;
		
InitVideoEncPriv_fail:
	
	for(i = 0; i < MAX_ENCODED_FRAMES; i ++){
		if(psVideoEncPriv->asEncodeFrame[i].pu8FrameData){
			free(psVideoEncPriv->asEncodeFrame[i].pu8FrameData);
			psVideoEncPriv->asEncodeFrame[i].pu8FrameData = NULL;
		}
	}
		
	if(psVideoEncPriv){
		pthread_mutex_destroy(&psVideoEncPriv->tCmdMutex);
		free(psVideoEncPriv);
	}
	
	return eNMRet;
}

static void
FinialVideoEncPriv(
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes
)
{
	S_VIDEO_ENC_PRIV *psVideoEncPriv;
	int i;
	
	if(psVideoEncodeRes->pvVideoEncPriv == NULL)
		return;
	
	psVideoEncPriv = (S_VIDEO_ENC_PRIV *)psVideoEncodeRes->pvVideoEncPriv;

	for(i = 0; i < MAX_ENCODED_FRAMES; i ++){
		if(psVideoEncPriv->asEncodeFrame[i].pu8FrameData){
			free(psVideoEncPriv->asEncodeFrame[i].pu8FrameData);
			psVideoEncPriv->asEncodeFrame[i].pu8FrameData = NULL;
		}
	}
	
	pthread_mutex_destroy(&psVideoEncPriv->tCmdMutex);
	free(psVideoEncPriv);
	psVideoEncodeRes->pvVideoEncPriv = NULL;
}

static E_NM_ERRNO
InitAudioEncPriv(
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes
)
{
	E_NM_ERRNO eNMRet;
	S_AUDIO_ENC_PRIV *psAudioEncPriv = NULL;
	uint32_t u32BSBufDataSize;
	uint32_t u32PCMBufDataSize;
	uint8_t *pu8TempBuf;
	int i;
	
	if(psAudioEncodeRes->psAudioCodecIF == NULL){
		psAudioEncodeRes->pvAudioEncPriv = NULL;
		return eNM_ERRNO_NONE;
	}

	psAudioEncPriv = calloc(1, sizeof(S_AUDIO_ENC_PRIV));

	if(psAudioEncPriv == NULL){
		eNMRet = eNM_ERRNO_MALLOC;
		goto InitAudioEncPriv_fail;
	}

	//Init command mutex
	if(pthread_mutex_init(&psAudioEncPriv->tCmdMutex, NULL) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto InitAudioEncPriv_fail;
	}
	
	u32BSBufDataSize = psAudioEncodeRes->sFillAudioCtx.u32SampleRate * psAudioEncodeRes->sFillAudioCtx.u32Channel * 2;

	//Allocate encoded bit stream buffer
	for(i = 0; i < MAX_ENCODED_FRAMES; i ++){
		pu8TempBuf = malloc(u32BSBufDataSize + 100);
		
		if(pu8TempBuf == NULL){
			eNMRet = eNM_ERRNO_MALLOC;
			goto InitAudioEncPriv_fail;
		}
		
		psAudioEncPriv->asEncodeFrame[i].u32FrameDataLimit = u32BSBufDataSize;
		psAudioEncPriv->asEncodeFrame[i].pu8FrameData = pu8TempBuf;
	}
	
	//Allocate audio PCM buffer
	u32PCMBufDataSize = psAudioEncodeRes->sFillAudioCtx.u32SamplePerBlock * psAudioEncodeRes->sFillAudioCtx.u32Channel * 2;
	pu8TempBuf = malloc(u32PCMBufDataSize + 100);
	
	if(pu8TempBuf == NULL){
			eNMRet = eNM_ERRNO_MALLOC;
			goto InitAudioEncPriv_fail;
	}
	
	psAudioEncPriv->eCmdCode = eENCODE_IOCTL_NONE;
	psAudioEncPriv->pvCmdArgv = NULL;
	psAudioEncPriv->eEncThreadState = eENCODE_THREAD_STATE_INIT;
	psAudioEncodeRes->pvAudioEncPriv = (void *)psAudioEncPriv;

	psAudioEncPriv->pu8PCMBuf = pu8TempBuf;
	psAudioEncPriv->u32PCMBufLimit = u32PCMBufDataSize;
		
	return eNM_ERRNO_NONE;

InitAudioEncPriv_fail:

	if(psAudioEncPriv->pu8PCMBuf)
		free(psAudioEncPriv->pu8PCMBuf);
	
	for(i = 0; i < MAX_ENCODED_FRAMES; i ++){
		if(psAudioEncPriv->asEncodeFrame[i].pu8FrameData){
			free(psAudioEncPriv->asEncodeFrame[i].pu8FrameData);
		}
	}
	
	if(psAudioEncPriv){
		pthread_mutex_destroy(&psAudioEncPriv->tCmdMutex);
		free(psAudioEncPriv);
	}

	return eNMRet;
}
static void
FinialAudioEncPriv(
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes
)
{
	S_AUDIO_ENC_PRIV *psAudioEncPriv;
	int i;
	
	if(psAudioEncodeRes->pvAudioEncPriv == NULL)
		return;
	
	psAudioEncPriv = (S_AUDIO_ENC_PRIV *)psAudioEncodeRes->pvAudioEncPriv;

	if(psAudioEncPriv->pu8PCMBuf)
		free(psAudioEncPriv->pu8PCMBuf);
	
	for(i = 0; i < MAX_ENCODED_FRAMES; i ++){
		if(psAudioEncPriv->asEncodeFrame[i].pu8FrameData){
			free(psAudioEncPriv->asEncodeFrame[i].pu8FrameData);
		}
	}
	
	pthread_mutex_destroy(&psAudioEncPriv->tCmdMutex);
	free(psAudioEncPriv);
	psAudioEncodeRes->pvAudioEncPriv = NULL;
}

static E_NM_ERRNO
VideoEncThread_IOCTL(
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes,
	E_ENCODE_IOCTL_CODE eCmdCode,
	void *pvCmdArgv,
	int32_t i32CmdArgvSize,
	bool bBlocking
)
{
	S_VIDEO_ENC_PRIV *psVideoEncPriv;
	E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;

	if(psVideoEncodeRes == NULL)
		return eNM_ERRNO_NULL_PTR;
	
	psVideoEncPriv = (S_VIDEO_ENC_PRIV *)psVideoEncodeRes->pvVideoEncPriv;
	if(psVideoEncPriv == NULL)
		return eNM_ERRNO_NONE;
	
	pthread_mutex_lock(&psVideoEncodeRes->tVideoEncodeCtrlMutex);
	pthread_mutex_lock(&psVideoEncPriv->tCmdMutex);

	//Argument size check
	switch (eCmdCode){
		case eENCODE_IOCTL_GET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_ENCODE_THREAD_STATE *)){
				eNMRet = eNM_ERRNO_SIZE;
			}			
		}
		break;
		case eENCODE_IOCTL_SET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_ENCODE_THREAD_STATE)){
				eNMRet = eNM_ERRNO_SIZE;
			}
		}
		break;
	}

	if(eNMRet == eNM_ERRNO_NONE){
		psVideoEncPriv->pvCmdArgv = pvCmdArgv;		
		psVideoEncPriv->eCmdCode = eCmdCode;

		pthread_mutex_unlock(&psVideoEncPriv->tCmdMutex);
		if(bBlocking){
			sem_wait(&psVideoEncodeRes->tVideoEncodeCtrlSem);
			eNMRet = psVideoEncPriv->eCmdRet;
		}
	}
	else{
		pthread_mutex_unlock(&psVideoEncPriv->tCmdMutex);
	}
	
	pthread_mutex_unlock(&psVideoEncodeRes->tVideoEncodeCtrlMutex);
	return eNMRet;
}


static E_NM_ERRNO
AudioEncThread_IOCTL(
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes,
	E_ENCODE_IOCTL_CODE eCmdCode,
	void *pvCmdArgv,
	int32_t i32CmdArgvSize,
	bool bBlocking
)
{
	S_AUDIO_ENC_PRIV *psAudioEncPriv;
	E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;

	if(psAudioEncodeRes == NULL)
		return eNM_ERRNO_NULL_PTR;
	
	psAudioEncPriv = (S_AUDIO_ENC_PRIV *)psAudioEncodeRes->pvAudioEncPriv;
	if(psAudioEncPriv == NULL)
		return eNM_ERRNO_NONE;
	
	pthread_mutex_lock(&psAudioEncodeRes->tAudioEncodeCtrlMutex);
	pthread_mutex_lock(&psAudioEncPriv->tCmdMutex);

	//Argument size check
	switch (eCmdCode){
		case eENCODE_IOCTL_GET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_ENCODE_THREAD_STATE *)){
				eNMRet = eNM_ERRNO_SIZE;
			}			
		}
		break;
		case eENCODE_IOCTL_SET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_ENCODE_THREAD_STATE)){
				eNMRet = eNM_ERRNO_SIZE;
			}
		}
		break;
	}

	if(eNMRet == eNM_ERRNO_NONE){
		psAudioEncPriv->pvCmdArgv = pvCmdArgv;		
		psAudioEncPriv->eCmdCode = eCmdCode;

		pthread_mutex_unlock(&psAudioEncPriv->tCmdMutex);
		if(bBlocking){
			sem_wait(&psAudioEncodeRes->tAudioEncodeCtrlSem);
			eNMRet = psAudioEncPriv->eCmdRet;
		}
	}
	else{
		pthread_mutex_unlock(&psAudioEncPriv->tCmdMutex);
	}
	
	pthread_mutex_unlock(&psAudioEncodeRes->tAudioEncodeCtrlMutex);
	return eNMRet;
}


//////////////////////////////////////////////////////////////////////

void
RecordEncode_Final(
	S_RECORD_ENCODE_RES *psEncodeRes
)
{
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes = &psEncodeRes->sVideoEncodeRes;
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes = &psEncodeRes->sAudioEncodeRes;

	FinialVideoEncPriv(psVideoEncodeRes);
	FinialAudioEncPriv(psAudioEncodeRes);

	//Close codec
	if(psVideoEncodeRes->pvVideoCodecRes){
		psVideoEncodeRes->psVideoCodecIF->pfnCloseCodec(&psVideoEncodeRes->pvVideoCodecRes);
		psVideoEncodeRes->pvVideoCodecRes = NULL;
	}
	
	if(psAudioEncodeRes->pvAudioCodecRes){ 
		psAudioEncodeRes->psAudioCodecIF->pfnCloseCodec(&psAudioEncodeRes->pvAudioCodecRes);
		psAudioEncodeRes->pvAudioCodecRes = NULL;
	}

	//Destroy mutex and semphore
	pthread_mutex_destroy(&psVideoEncodeRes->tVideoEncodeCtrlMutex);

	sem_destroy(&psVideoEncodeRes->tVideoEncodeCtrlSem);

	pthread_mutex_destroy(&psAudioEncodeRes->tAudioEncodeCtrlMutex);

	sem_destroy(&psAudioEncodeRes->tAudioEncodeCtrlSem);
}

E_NM_ERRNO
RecordEncode_Init(
	S_RECORD_ENCODE_RES *psEncodeRes
)
{
	E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
	
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes = &psEncodeRes->sVideoEncodeRes;
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes = &psEncodeRes->sAudioEncodeRes;

	//Init control mutex and semphore
	if(pthread_mutex_init(&psVideoEncodeRes->tVideoEncodeCtrlMutex, NULL) != 0){
			eNMRet = eNM_ERRNO_OS;
			goto RecordEncode_Init_fail;
	}
	
	if(sem_init(&psVideoEncodeRes->tVideoEncodeCtrlSem, 0, 0) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto RecordEncode_Init_fail;
	}

	if(pthread_mutex_init(&psAudioEncodeRes->tAudioEncodeCtrlMutex, NULL) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto RecordEncode_Init_fail;
	}
	
	if(sem_init(&psAudioEncodeRes->tAudioEncodeCtrlSem, 0, 0) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto RecordEncode_Init_fail;
	}
	
	//Open codec
	if(psVideoEncodeRes->psVideoCodecIF){
		eNMRet = psVideoEncodeRes->psVideoCodecIF->pfnOpenCodec(&psVideoEncodeRes->sMediaVideoCtx, &psVideoEncodeRes->pvVideoCodecRes);

		if(eNMRet != eNM_ERRNO_NONE){
			goto RecordEncode_Init_fail;
		}
	}

	if(psAudioEncodeRes->psAudioCodecIF){
		eNMRet = psAudioEncodeRes->psAudioCodecIF->pfnOpenCodec(&psAudioEncodeRes->sMediaAudioCtx, &psAudioEncodeRes->pvAudioCodecRes);

		if(eNMRet != eNM_ERRNO_NONE){
			goto RecordEncode_Init_fail;
		}
	}

	//Alloc encoder private data
	eNMRet = InitVideoEncPriv(psVideoEncodeRes);
	if(eNMRet != eNM_ERRNO_NONE){
		goto RecordEncode_Init_fail;
	}
	
	eNMRet = InitAudioEncPriv(psAudioEncodeRes);
	if(eNMRet != eNM_ERRNO_NONE){
		goto RecordEncode_Init_fail;
	}
	
	psVideoEncodeRes->psRecordEncodeRes = psEncodeRes;
	psAudioEncodeRes->psRecordEncodeRes = psEncodeRes;

	return eNM_ERRNO_NONE;
		
RecordEncode_Init_fail:
	
	RecordEncode_Final(psEncodeRes);
	return eNMRet;
}


void
RecordEncode_ThreadDestroy(
	S_RECORD_ENCODE_RES *psEncodeRes
)
{
	E_ENCODE_THREAD_STATE eEncThreadState = eENCODE_THREAD_STATE_TO_EXIT;
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes = &psEncodeRes->sVideoEncodeRes;
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes = &psEncodeRes->sAudioEncodeRes;
	void *pvJoin;
	
	if(psVideoEncodeRes->tVideoEncodeThread){
			VideoEncThread_IOCTL(psVideoEncodeRes, eENCODE_IOCTL_SET_STATE, (void *)eEncThreadState, sizeof(E_ENCODE_THREAD_STATE), false);
			pthread_join(psVideoEncodeRes->tVideoEncodeThread, &pvJoin);
			psVideoEncodeRes->tVideoEncodeThread = NULL;
	}
		
	if(psAudioEncodeRes->tAudioEncodeThread){
			AudioEncThread_IOCTL(psAudioEncodeRes, eENCODE_IOCTL_SET_STATE, (void *)eEncThreadState, sizeof(E_ENCODE_THREAD_STATE), false);
			pthread_join(psAudioEncodeRes->tAudioEncodeThread, &pvJoin);
			psAudioEncodeRes->tAudioEncodeThread = NULL;
	}

}

E_NM_ERRNO
RecordEncode_ThreadCreate(
	S_RECORD_ENCODE_RES *psEncodeRes
)
{
	int iRet;
	E_ENCODE_THREAD_STATE eEncThreadState;
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes = &psEncodeRes->sVideoEncodeRes;
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes = &psEncodeRes->sAudioEncodeRes;

	pthread_attr_t sAudioThreadAttr;
	
	pthread_attr_init(&sAudioThreadAttr);
	pthread_attr_setstacksize(&sAudioThreadAttr, 1024 * 10);

	if(psVideoEncodeRes->psVideoCodecIF){
		iRet = pthread_create(&psVideoEncodeRes->tVideoEncodeThread, NULL, VideoEnocdeWorkerThread, psVideoEncodeRes);
		if(iRet != 0){
			goto RecordEncode_ThreadCreate_fail;
		}

		//Wait thread running
		while(1){
			VideoEncThread_IOCTL(psVideoEncodeRes, eENCODE_IOCTL_GET_STATE, &eEncThreadState, sizeof(E_ENCODE_THREAD_STATE *), true);
			if(eEncThreadState != eENCODE_THREAD_STATE_INIT)
				break;
			usleep(1000);
		}
	}
	
	if(psAudioEncodeRes->psAudioCodecIF){
		iRet = pthread_create(&psAudioEncodeRes->tAudioEncodeThread, &sAudioThreadAttr, AudioEnocdeWorkerThread, psAudioEncodeRes);
		if(iRet != 0){
			goto RecordEncode_ThreadCreate_fail;
		}
		//Wait thread running
		while(1){
			AudioEncThread_IOCTL(psAudioEncodeRes, eENCODE_IOCTL_GET_STATE, &eEncThreadState, sizeof(E_ENCODE_THREAD_STATE *), true);
			if(eEncThreadState != eENCODE_THREAD_STATE_INIT)
				break;
			usleep(1000);
		}
	}

	return eNM_ERRNO_NONE;

RecordEncode_ThreadCreate_fail:

	RecordEncode_ThreadDestroy(psEncodeRes);
	return eNM_ERRNO_OS;
}

E_NM_ERRNO
RecordEncode_Run(
	S_RECORD_ENCODE_RES *psEncodeRes
)
{
	E_ENCODE_THREAD_STATE eEncThreadState;
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes = &psEncodeRes->sVideoEncodeRes;
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes = &psEncodeRes->sAudioEncodeRes;

	eEncThreadState = eENCODE_THREAD_STATE_RUN;
	if(psVideoEncodeRes->tVideoEncodeThread){
			VideoEncThread_IOCTL(psVideoEncodeRes, eENCODE_IOCTL_SET_STATE, (void *)eEncThreadState, sizeof(E_ENCODE_THREAD_STATE), true);
	}
	
	if(psAudioEncodeRes->tAudioEncodeThread){
			AudioEncThread_IOCTL(psAudioEncodeRes, eENCODE_IOCTL_SET_STATE, (void *)eEncThreadState, sizeof(E_ENCODE_THREAD_STATE), true);
	}

	return eNM_ERRNO_NONE;
}

E_NM_ERRNO
RecordEncode_Stop(
	S_RECORD_ENCODE_RES *psEncodeRes
)
{
	E_ENCODE_THREAD_STATE eEncThreadState;
	S_RECORD_VIDEO_ENCODE_RES *psVideoEncodeRes = &psEncodeRes->sVideoEncodeRes;
	S_RECORD_AUDIO_ENCODE_RES *psAudioEncodeRes = &psEncodeRes->sAudioEncodeRes;

	eEncThreadState = eENCODE_THREAD_STATE_IDLE;
	if(psVideoEncodeRes->tVideoEncodeThread){
			VideoEncThread_IOCTL(psVideoEncodeRes, eENCODE_IOCTL_SET_STATE, (void *)eEncThreadState, sizeof(E_ENCODE_THREAD_STATE), true);
	}
	
	if(psAudioEncodeRes->tAudioEncodeThread){
			AudioEncThread_IOCTL(psAudioEncodeRes, eENCODE_IOCTL_SET_STATE, (void *)eEncThreadState, sizeof(E_ENCODE_THREAD_STATE), true);
	}

	return eNM_ERRNO_NONE;
}

