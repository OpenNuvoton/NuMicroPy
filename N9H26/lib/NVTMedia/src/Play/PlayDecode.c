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
#include "Util/NMVideoList.h"
#include "Util/NMAudioList.h"

#define MAX_DECODED_FRAMES	3

typedef enum {
	eDECODE_THREAD_STATE_INIT,
	eDECODE_THREAD_STATE_IDLE,
	eDECODE_THREAD_STATE_RUN,
	eDECODE_THREAD_STATE_TO_EXIT,
	eDECODE_THREAD_STATE_EXIT,
} E_DECODE_THREAD_STATE;

typedef struct {
	uint8_t *pu8FrameData;
	uint32_t u32FrameDataLimit;
	uint64_t u64PTS;	//present time
}S_DECODE_FRAME;

typedef struct {
	E_DECODE_THREAD_STATE eDecThreadState;
	E_DECODE_IOCTL_CODE	eCmdCode;
	void *pvCmdArgv;
	pthread_mutex_t tCmdMutex;
	S_DECODE_FRAME asDecodeFrame[MAX_DECODED_FRAMES];
	
} S_VIDEO_DEC_PRIV;

typedef struct {
	E_DECODE_THREAD_STATE eDecThreadState;
	E_DECODE_IOCTL_CODE	eCmdCode;
	void *pvCmdArgv;
	pthread_mutex_t tCmdMutex;
	S_DECODE_FRAME asDecodeFrame[MAX_DECODED_FRAMES];
	
} S_AUDIO_DEC_PRIV;


static int
RunVideoDecCmd(
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes
)
{
	S_VIDEO_DEC_PRIV *psVideoDecPriv = (S_VIDEO_DEC_PRIV *)psVideoDecodeRes->pvVideoDecPriv;
	int i32SemValue;

	pthread_mutex_lock(&psVideoDecPriv->tCmdMutex);

	if(psVideoDecPriv->eCmdCode == eDECODE_IOCTL_NONE){

		sem_getvalue(&psVideoDecodeRes->tVideoDecodeCtrlSem, &i32SemValue);
		if(i32SemValue < 0){
			printf("%s, %s: Warning! post semaphore again! \n", __FILE__, __FUNCTION__);
			sem_post(&psVideoDecodeRes->tVideoDecodeCtrlSem);
		}
			
		pthread_mutex_unlock(&psVideoDecPriv->tCmdMutex);
		return 0;
	}

	switch(psVideoDecPriv->eCmdCode){
		case eDECODE_IOCTL_GET_STATE:
		{
			E_DECODE_THREAD_STATE *peThreadState = (E_DECODE_THREAD_STATE *)psVideoDecPriv->pvCmdArgv;
			*peThreadState = psVideoDecPriv->eDecThreadState;
		}
		break;
		case eDECODE_IOCTL_SET_STATE:
		{
			E_DECODE_THREAD_STATE eThreadState = (E_DECODE_THREAD_STATE)psVideoDecPriv->pvCmdArgv;
			psVideoDecPriv->eDecThreadState = eThreadState;
		}
		break;
	}
	
	psVideoDecPriv->eCmdCode = eDECODE_IOCTL_NONE;
	pthread_mutex_unlock(&psVideoDecPriv->tCmdMutex);
	
	sem_getvalue(&psVideoDecodeRes->tVideoDecodeCtrlSem,	&i32SemValue);

   /* If semaphore value is equal or larger than zero, there is no
     * thread waiting for this semaphore. Otherwise (<0), call FreeRTOS interface
     * to wake up a thread. */	

	if(i32SemValue < 0)
		sem_post(&psVideoDecodeRes->tVideoDecodeCtrlSem);

	return 0;
}

static int
RunAudioDecCmd(
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes
)
{
	int i32SemValue;
	S_AUDIO_DEC_PRIV *psAudioDecPriv = (S_AUDIO_DEC_PRIV *)psAudioDecodeRes->pvAudioDecPriv;

	pthread_mutex_lock(&psAudioDecPriv->tCmdMutex);

	if(psAudioDecPriv->eCmdCode == eDECODE_IOCTL_NONE){

		sem_getvalue(&psAudioDecodeRes->tAudioDecodeCtrlSem, &i32SemValue);
		if(i32SemValue < 0){
			printf("%s, %s: Warning! post semaphore again! \n", __FILE__, __FUNCTION__);
			sem_post(&psAudioDecodeRes->tAudioDecodeCtrlSem);
		}
			
		pthread_mutex_unlock(&psAudioDecPriv->tCmdMutex);
		return 0;
	}

	switch(psAudioDecPriv->eCmdCode){
		case eDECODE_IOCTL_GET_STATE:
		{
			E_DECODE_THREAD_STATE *peThreadState = (E_DECODE_THREAD_STATE *)psAudioDecPriv->pvCmdArgv;
			*peThreadState = psAudioDecPriv->eDecThreadState;
		}
		break;
		case eDECODE_IOCTL_SET_STATE:
		{
			E_DECODE_THREAD_STATE eThreadState = (E_DECODE_THREAD_STATE)psAudioDecPriv->pvCmdArgv;
			psAudioDecPriv->eDecThreadState = eThreadState;
		}
		break;
	}
	
	psAudioDecPriv->eCmdCode = eDECODE_IOCTL_NONE;	
	pthread_mutex_unlock(&psAudioDecPriv->tCmdMutex);
	
	sem_getvalue(&psAudioDecodeRes->tAudioDecodeCtrlSem,	&i32SemValue);

   /* If semaphore value is equal or larger than zero, there is no
     * thread waiting for this semaphore. Otherwise (<0), call FreeRTOS interface
     * to wake up a thread. */	

	if(i32SemValue < 0)
		sem_post(&psAudioDecodeRes->tAudioDecodeCtrlSem);

	return 0;
}


static void * VideoDeocdeWorkerThread( void * pvArgs )
{
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes = (S_PLAY_VIDEO_DECODE_RES *)pvArgs;
	S_VIDEO_DEC_PRIV *psVideoDecPriv = (S_VIDEO_DEC_PRIV *)psVideoDecodeRes->pvVideoDecPriv;
	uint32_t u32DecodedFrameIndex = 0;
	void *pvVideoList;
	S_NMPACKET *psVideoPacket = NULL;
	uint64_t u64CurrentTime;
	S_NM_VIDEO_CTX *psVideoMediaCtx = NULL;
	S_NM_VIDEO_CTX *psVideoFlushCtx = NULL;
	uint32_t u32RemainDataSize;

#if _DECODE_DEBUG_
	uint64_t u64StartDecTime;
	uint64_t u64PrevChunkDataTime;

	NMLOG_DEBUG("VideoDeocdeWorkerThread run \n");
#endif
	
	psVideoDecPriv->eDecThreadState = eDECODE_THREAD_STATE_IDLE;
	pvVideoList = psVideoDecodeRes->psPlayDecodeRes->psPlayEngRes->pvVideoList;
	psVideoMediaCtx = &psVideoDecodeRes->sMediaVideoCtx;
	psVideoFlushCtx = &psVideoDecodeRes->sFlushVideoCtx;
	
	while(psVideoDecPriv->eDecThreadState != eDECODE_THREAD_STATE_TO_EXIT){

		RunVideoDecCmd(psVideoDecodeRes);
		u64CurrentTime = NMUtil_GetTimeMilliSec();
		
		if(psVideoDecPriv->eDecThreadState == eDECODE_THREAD_STATE_IDLE){
			usleep(1000);
			continue;
		}

		//Acquire frame from video list
		if(pvVideoList){
			psVideoPacket = NMVideoList_AcquireClosedPacket(pvVideoList, u64CurrentTime);
		}

		if(psVideoPacket == NULL){
			usleep(1000);
			continue;
		}
		
		psVideoMediaCtx->pu8DataBuf = psVideoPacket->pu8DataPtr;
		psVideoMediaCtx->u32DataSize = psVideoPacket->u32UsedDataLen;
		psVideoMediaCtx->u32DataLimit = psVideoPacket->u32UsedDataLen;
		psVideoMediaCtx->u64DataTime = psVideoPacket->u64DataTime;
		
		psVideoFlushCtx->pu8DataBuf = psVideoDecPriv->asDecodeFrame[u32DecodedFrameIndex].pu8FrameData;
		psVideoFlushCtx->u32DataLimit = psVideoDecPriv->asDecodeFrame[u32DecodedFrameIndex].u32FrameDataLimit;
		psVideoFlushCtx->u32DataSize = 0;

#if _DECODE_DEBUG_
		u64StartDecTime = NMUtil_GetTimeMilliSec();
#endif		

		if(psVideoDecodeRes->pvVideoCodecRes){
			psVideoDecodeRes->psVideoCodecIF->pfnDecodeVideo(psVideoMediaCtx, psVideoFlushCtx, &u32RemainDataSize, psVideoDecodeRes->pvVideoCodecRes);
			//TODO: check remain data size
		}

#if _DECODE_DEBUG_
		NMLOG_DEBUG("Video each chunk diff time(chunk data time) %d, decode time %d \n", (uint32_t)(psVideoMediaCtx->u64DataTime - u64PrevChunkDataTime), (uint32_t)(NMUtil_GetTimeMilliSec() - u64StartDecTime));
		u64PrevChunkDataTime = psVideoMediaCtx->u64DataTime;

#endif		


		if((psVideoFlushCtx->u32DataSize) &&(psVideoDecodeRes->pfnVideoFlush)){
			psVideoDecodeRes->pfnVideoFlush(psVideoFlushCtx);
		}
		
		NMVideoList_ReleasePacket(psVideoPacket, pvVideoList);
		u32DecodedFrameIndex = (u32DecodedFrameIndex + 1) % MAX_DECODED_FRAMES;
		
		usleep(1000);
	}
	
	psVideoDecPriv->eDecThreadState = eDECODE_THREAD_STATE_EXIT;
	return NULL;
}

static void * AudioDeocdeWorkerThread( void * pvArgs )
{
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes = (S_PLAY_AUDIO_DECODE_RES *)pvArgs;
	S_AUDIO_DEC_PRIV *psAudioDecPriv = (S_AUDIO_DEC_PRIV *)psAudioDecodeRes->pvAudioDecPriv;
	uint32_t u32DecodedFrameIndex = 0;
	void *pvAudioList;
	S_NMPACKET *psAudioPacket = NULL;
	uint64_t u64CurrentTime;
	S_NM_AUDIO_CTX *psAudioMediaCtx = NULL;
	S_NM_AUDIO_CTX *psAudioFlushCtx = NULL;
	uint32_t u32RemainDataSize;

	psAudioDecPriv->eDecThreadState = eDECODE_THREAD_STATE_IDLE;
	pvAudioList = psAudioDecodeRes->psPlayDecodeRes->psPlayEngRes->pvAudioList;
	psAudioMediaCtx = &psAudioDecodeRes->sMediaAudioCtx;
	psAudioFlushCtx = &psAudioDecodeRes->sFlushAudioCtx;
	
#if _DECODE_DEBUG_
	uint64_t u64StartDecTime;
	uint64_t u64PrevChunkDataTime;

	NMLOG_DEBUG("AudioDeocdeWorkerThread run \n");
#endif
	
	while(psAudioDecPriv->eDecThreadState != eDECODE_THREAD_STATE_TO_EXIT){
		RunAudioDecCmd(psAudioDecodeRes);
		u64CurrentTime = NMUtil_GetTimeMilliSec();

		if(psAudioDecPriv->eDecThreadState == eDECODE_THREAD_STATE_IDLE){
			usleep(1000);
			continue;
		}
		//Acquire frame from audio list
		if(pvAudioList){
			psAudioPacket = NMAudioList_AcquireClosedPacket(pvAudioList, u64CurrentTime);
		}

		if(psAudioPacket == NULL){
			usleep(1000);
			continue;
		}

		psAudioMediaCtx->pu8DataBuf = psAudioPacket->pu8DataPtr;
		psAudioMediaCtx->u32DataSize = psAudioPacket->u32UsedDataLen;
		psAudioMediaCtx->u32DataLimit = psAudioPacket->u32UsedDataLen;
		psAudioMediaCtx->u64DataTime = psAudioPacket->u64DataTime;
		
		psAudioFlushCtx->pu8DataBuf = psAudioDecPriv->asDecodeFrame[u32DecodedFrameIndex].pu8FrameData;
		psAudioFlushCtx->u32DataLimit = psAudioDecPriv->asDecodeFrame[u32DecodedFrameIndex].u32FrameDataLimit;
		psAudioFlushCtx->u32DataSize = 0;

#if _DECODE_DEBUG_
		u64StartDecTime = NMUtil_GetTimeMilliSec();
#endif		

		if(psAudioDecodeRes->pvAudioCodecRes){
			psAudioDecodeRes->psAudioCodecIF->pfnDecodeAudio(psAudioMediaCtx, psAudioFlushCtx, &u32RemainDataSize, psAudioDecodeRes->pvAudioCodecRes);
			//TODO: check remain data size
		}

#if _DECODE_DEBUG_
//		NMLOG_DEBUG("Audio each chunk diff time(chunk data time) %d, decode time %d \n", (uint32_t)(psAudioMediaCtx->u64DataTime - u64PrevChunkDataTime), (uint32_t)(NMUtil_GetTimeMilliSec() - u64StartDecTime));
		u64PrevChunkDataTime = psAudioMediaCtx->u64DataTime;

#endif		
		
		if((psAudioFlushCtx->u32DataSize) &&(psAudioDecodeRes->pfnAudioFlush)){
			psAudioDecodeRes->pfnAudioFlush(psAudioFlushCtx);
		}
		
		NMAudioList_ReleasePacket(psAudioPacket, pvAudioList);
		u32DecodedFrameIndex = (u32DecodedFrameIndex + 1) % MAX_DECODED_FRAMES;
				
		usleep(1000);
	}
	
	psAudioDecPriv->eDecThreadState = eDECODE_THREAD_STATE_EXIT;

	return NULL;
}

static E_NM_ERRNO
InitVideoDecPriv(
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes
)
{
	E_NM_ERRNO eNMRet;
	S_VIDEO_DEC_PRIV *psVideoDecPriv = NULL;
	int i;
	uint32_t u32FlushImageDataSize;
	uint8_t *pu8TempBuf;
	
	if(psVideoDecodeRes->psVideoCodecIF == NULL){
		psVideoDecodeRes->pvVideoDecPriv = NULL;
		return eNM_ERRNO_NONE;
	}

	//Alloc decoder private data
	psVideoDecPriv = calloc(1, sizeof(S_VIDEO_DEC_PRIV));

	if(psVideoDecPriv == NULL){
		eNMRet = eNM_ERRNO_MALLOC;
		goto InitVideoDecPriv_fail;
	}

	//Init command mutex
	if(pthread_mutex_init(&psVideoDecPriv->tCmdMutex, NULL) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto InitVideoDecPriv_fail;
	}
	
	u32FlushImageDataSize = psVideoDecodeRes->sFlushVideoCtx.u32Width * psVideoDecodeRes->sFlushVideoCtx.u32Height * 2;
	
	for(i = 0; i < MAX_DECODED_FRAMES; i ++){
		pu8TempBuf = malloc(u32FlushImageDataSize + 100);
		
		if(pu8TempBuf == NULL){
			eNMRet = eNM_ERRNO_MALLOC;
			goto InitVideoDecPriv_fail;
		}
		
		psVideoDecPriv->asDecodeFrame[i].u32FrameDataLimit = u32FlushImageDataSize;
		psVideoDecPriv->asDecodeFrame[i].pu8FrameData = pu8TempBuf;
	}

	psVideoDecPriv->eCmdCode = eDECODE_IOCTL_NONE;
	psVideoDecPriv->pvCmdArgv = NULL;	
	psVideoDecPriv->eDecThreadState = eDECODE_THREAD_STATE_INIT;
	psVideoDecodeRes->pvVideoDecPriv = (void *)psVideoDecPriv;
	
	
	return eNM_ERRNO_NONE;

InitVideoDecPriv_fail:

	for(i = 0; i < MAX_DECODED_FRAMES; i ++){
		if(psVideoDecPriv->asDecodeFrame[i].pu8FrameData){
			free(psVideoDecPriv->asDecodeFrame[i].pu8FrameData);
			psVideoDecPriv->asDecodeFrame[i].pu8FrameData = NULL;
		}
	}
		
	if(psVideoDecPriv){
		pthread_mutex_destroy(&psVideoDecPriv->tCmdMutex);
		free(psVideoDecPriv);
	}
	
	return eNMRet;
}

static void
FinialVideoDecPriv(
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes
)
{
	S_VIDEO_DEC_PRIV *psVideoDecPriv;
	int i;
	
	if(psVideoDecodeRes->pvVideoDecPriv == NULL)
		return;
	
	psVideoDecPriv = (S_VIDEO_DEC_PRIV *)psVideoDecodeRes->pvVideoDecPriv;

	for(i = 0; i < MAX_DECODED_FRAMES; i ++){
		if(psVideoDecPriv->asDecodeFrame[i].pu8FrameData){
			free(psVideoDecPriv->asDecodeFrame[i].pu8FrameData);
			psVideoDecPriv->asDecodeFrame[i].pu8FrameData = NULL;
		}
	}
	
	pthread_mutex_destroy(&psVideoDecPriv->tCmdMutex);
	free(psVideoDecPriv);
	psVideoDecodeRes->pvVideoDecPriv = NULL;
}

static E_NM_ERRNO
InitAudioDecPriv(
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes
)
{
	E_NM_ERRNO eNMRet;
	S_AUDIO_DEC_PRIV *psAudioDecPriv = NULL;
	uint32_t u32FlushFrameSize;
	uint8_t *pu8TempBuf;
	int i;
	
	if(psAudioDecodeRes->psAudioCodecIF == NULL){
		psAudioDecodeRes->pvAudioDecPriv = NULL;
		return eNM_ERRNO_NONE;
	}
	
	psAudioDecPriv = calloc(1, sizeof(S_AUDIO_DEC_PRIV));

	if(psAudioDecPriv == NULL){
		eNMRet = eNM_ERRNO_MALLOC;
		goto InitAudioDecPriv_fail;
	}

	//Init command mutex
	if(pthread_mutex_init(&psAudioDecPriv->tCmdMutex, NULL) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto InitAudioDecPriv_fail;
	}

	u32FlushFrameSize = psAudioDecodeRes->sFlushAudioCtx.u32SampleRate * psAudioDecodeRes->sFlushAudioCtx.u32Channel * 2;

	for(i = 0; i < MAX_DECODED_FRAMES; i ++){
		pu8TempBuf = malloc(u32FlushFrameSize + 100);
		
		if(pu8TempBuf == NULL){
			eNMRet = eNM_ERRNO_MALLOC;
			goto InitAudioDecPriv_fail;
		}
		
		psAudioDecPriv->asDecodeFrame[i].u32FrameDataLimit = u32FlushFrameSize;
		psAudioDecPriv->asDecodeFrame[i].pu8FrameData = pu8TempBuf;
	}
	
	psAudioDecPriv->eCmdCode = eDECODE_IOCTL_NONE;
	psAudioDecPriv->pvCmdArgv = NULL;
	psAudioDecPriv->eDecThreadState = eDECODE_THREAD_STATE_INIT;
	psAudioDecodeRes->pvAudioDecPriv = (void *)psAudioDecPriv;

	return eNM_ERRNO_NONE;

InitAudioDecPriv_fail:

	for(i = 0; i < MAX_DECODED_FRAMES; i ++){
		if(psAudioDecPriv->asDecodeFrame[i].pu8FrameData){
			free(psAudioDecPriv->asDecodeFrame[i].pu8FrameData);
			psAudioDecPriv->asDecodeFrame[i].pu8FrameData = NULL;
		}
	}
	
	if(psAudioDecPriv){
		pthread_mutex_destroy(&psAudioDecPriv->tCmdMutex);
		free(psAudioDecPriv);
	}
		
	return eNMRet;
}

static void
FinialAudioDecPriv(
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes
)
{
	S_AUDIO_DEC_PRIV *psAudioDecPriv;
	int i;
	
	if(psAudioDecodeRes->pvAudioDecPriv == NULL)
		return;
	
	psAudioDecPriv = (S_AUDIO_DEC_PRIV *)psAudioDecodeRes->pvAudioDecPriv;

	for(i = 0; i < MAX_DECODED_FRAMES; i ++){
		if(psAudioDecPriv->asDecodeFrame[i].pu8FrameData){
			free(psAudioDecPriv->asDecodeFrame[i].pu8FrameData);
			psAudioDecPriv->asDecodeFrame[i].pu8FrameData = NULL;
		}
	}

	
	pthread_mutex_destroy(&psAudioDecPriv->tCmdMutex);
	free(psAudioDecPriv);
	psAudioDecodeRes->pvAudioDecPriv = NULL;
}

static int
VideoDecThread_IOCTL(
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes,
	E_DECODE_IOCTL_CODE eCmdCode,
	void *pvCmdArgv,
	int32_t i32CmdArgvSize,
	bool bBlocking
)
{
	int32_t i32Ret = 0;
	S_VIDEO_DEC_PRIV *psVideoDecPriv;

	if(psVideoDecodeRes == NULL)
		return -1;
	
	psVideoDecPriv = (S_VIDEO_DEC_PRIV *)psVideoDecodeRes->pvVideoDecPriv;
	if(psVideoDecPriv == NULL)
		return 0;
	
	pthread_mutex_lock(&psVideoDecodeRes->tVideoDecodeCtrlMutex);
	pthread_mutex_lock(&psVideoDecPriv->tCmdMutex);

	//Argument size check
	switch (eCmdCode){
		case eDECODE_IOCTL_GET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_DECODE_THREAD_STATE *)){
				i32Ret = -2;
			}			
		}
		break;
		case eDECODE_IOCTL_SET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_DECODE_THREAD_STATE)){
				i32Ret = -2;
			}
		}
		break;
	}

	if(i32Ret == 0){
		psVideoDecPriv->pvCmdArgv = pvCmdArgv;		
		psVideoDecPriv->eCmdCode = eCmdCode;
		pthread_mutex_unlock(&psVideoDecPriv->tCmdMutex);

		if(bBlocking)
			sem_wait(&psVideoDecodeRes->tVideoDecodeCtrlSem);
	}
	else{
		pthread_mutex_unlock(&psVideoDecPriv->tCmdMutex);
	}
			
	pthread_mutex_unlock(&psVideoDecodeRes->tVideoDecodeCtrlMutex);
	return i32Ret;
}

static int
AudioDecThread_IOCTL(
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes,
	E_DECODE_IOCTL_CODE eCmdCode,
	void *pvCmdArgv,
	int32_t i32CmdArgvSize,
	bool bBlocking
)
{
	int32_t i32Ret = 0;
	S_AUDIO_DEC_PRIV *psAudioDecPriv;

	if(psAudioDecodeRes == NULL)
		return -1;
	
	psAudioDecPriv = (S_AUDIO_DEC_PRIV *)psAudioDecodeRes->pvAudioDecPriv;
	if(psAudioDecPriv == NULL)
		return 0;
	
	pthread_mutex_lock(&psAudioDecodeRes->tAudioDecodeCtrlMutex);
	pthread_mutex_lock(&psAudioDecPriv->tCmdMutex);

	//Argument size check
	switch (eCmdCode){
		case eDECODE_IOCTL_GET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_DECODE_THREAD_STATE *)){
				i32Ret = -2;
			}			
		}
		break;
		case eDECODE_IOCTL_SET_STATE:
		{
			if(i32CmdArgvSize < sizeof(E_DECODE_THREAD_STATE)){
				i32Ret = -2;
			}
		}
		break;
	}

	if(i32Ret == 0){
		psAudioDecPriv->eCmdCode = eCmdCode;
		psAudioDecPriv->pvCmdArgv = pvCmdArgv;		
		pthread_mutex_unlock(&psAudioDecPriv->tCmdMutex);

		if(bBlocking)
			sem_wait(&psAudioDecodeRes->tAudioDecodeCtrlSem);
	}
	else{
		pthread_mutex_unlock(&psAudioDecPriv->tCmdMutex);
	}
	
	pthread_mutex_unlock(&psAudioDecodeRes->tAudioDecodeCtrlMutex);
	return i32Ret;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////

void
PlayDecode_Final(
	S_PLAY_DECODE_RES *psDecodeRes
)
{
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes = &psDecodeRes->sVideoDecodeRes;
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes = &psDecodeRes->sAudioDecodeRes;

	FinialVideoDecPriv(psVideoDecodeRes);
	FinialAudioDecPriv(psAudioDecodeRes);
	
	
	//Close codec
	if(psVideoDecodeRes->pvVideoCodecRes){
		psVideoDecodeRes->psVideoCodecIF->pfnCloseCodec(&psVideoDecodeRes->pvVideoCodecRes);
		psVideoDecodeRes->pvVideoCodecRes = NULL;
	}
	
	if(psAudioDecodeRes->pvAudioCodecRes){
		psAudioDecodeRes->psAudioCodecIF->pfnCloseCodec(&psAudioDecodeRes->pvAudioCodecRes);
		psAudioDecodeRes->pvAudioCodecRes = NULL;
	}
	
	//Destroy mutex and semphore
//	if(psVideoDecodeRes->tVideoDecodeCtrlMutex != NULL){
		pthread_mutex_destroy(&psVideoDecodeRes->tVideoDecodeCtrlMutex);
//	}

//	if(psVideoDecodeRes->tVideoDecodeCtrlSem != NULL){
		sem_destroy(&psVideoDecodeRes->tVideoDecodeCtrlSem);
//	}

//	if(psAudioDecodeRes->tAudioDecodeCtrlMutex != NULL){
		pthread_mutex_destroy(&psAudioDecodeRes->tAudioDecodeCtrlMutex);
//	}

//	if(psAudioDecodeRes->tAudioDecodeCtrlSem != NULL){
		sem_destroy(&psAudioDecodeRes->tAudioDecodeCtrlSem);
//	}
}


E_NM_ERRNO
PlayDecode_Init(
	S_PLAY_DECODE_RES *psDecodeRes
)
{
	E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
	
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes = &psDecodeRes->sVideoDecodeRes;
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes = &psDecodeRes->sAudioDecodeRes;
	
	//Init control mutex and semphore
	if(pthread_mutex_init(&psVideoDecodeRes->tVideoDecodeCtrlMutex, NULL) != 0){
			eNMRet = eNM_ERRNO_OS;
			goto PlayDecode_Init_fail;
	}
	
	if(sem_init(&psVideoDecodeRes->tVideoDecodeCtrlSem, 0, 0) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto PlayDecode_Init_fail;
	}

	if(pthread_mutex_init(&psAudioDecodeRes->tAudioDecodeCtrlMutex, NULL) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto PlayDecode_Init_fail;
	}
	
	if(sem_init(&psAudioDecodeRes->tAudioDecodeCtrlSem, 0, 0) != 0){
		eNMRet = eNM_ERRNO_OS;
		goto PlayDecode_Init_fail;
	}
		
	//Open codec
	if(psVideoDecodeRes->psVideoCodecIF){
		eNMRet = psVideoDecodeRes->psVideoCodecIF->pfnOpenCodec(&psVideoDecodeRes->sFlushVideoCtx, &psVideoDecodeRes->pvVideoCodecRes);

		if(eNMRet != eNM_ERRNO_NONE){
			goto PlayDecode_Init_fail;
		}
	}

	if(psAudioDecodeRes->psAudioCodecIF){
		eNMRet = psAudioDecodeRes->psAudioCodecIF->pfnOpenCodec(&psAudioDecodeRes->sFlushAudioCtx, &psAudioDecodeRes->pvAudioCodecRes);

		if(eNMRet != eNM_ERRNO_NONE){
			goto PlayDecode_Init_fail;
		}
	}
	
	//Alloc decoder private data
	eNMRet = InitVideoDecPriv(psVideoDecodeRes);
	if(eNMRet != eNM_ERRNO_NONE){
		goto PlayDecode_Init_fail;
	}
	
	eNMRet = InitAudioDecPriv(psAudioDecodeRes);
	if(eNMRet != eNM_ERRNO_NONE){
		goto PlayDecode_Init_fail;
	}
	
	psVideoDecodeRes->psPlayDecodeRes = psDecodeRes;
	psAudioDecodeRes->psPlayDecodeRes = psDecodeRes;

	return eNM_ERRNO_NONE;

PlayDecode_Init_fail:
	
	PlayDecode_Final(psDecodeRes);

	return eNMRet;
}


void
PlayDecode_ThreadDestroy(
	S_PLAY_DECODE_RES *psDecodeRes
)
{
	E_DECODE_THREAD_STATE eDecThreadState = eDECODE_THREAD_STATE_TO_EXIT;
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes = &psDecodeRes->sVideoDecodeRes;
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes = &psDecodeRes->sAudioDecodeRes;
	void *pvJoin;
	
	if(psVideoDecodeRes->tVideoDecodeThread){
			VideoDecThread_IOCTL(psVideoDecodeRes, eDECODE_IOCTL_SET_STATE, (void *)eDecThreadState, sizeof(E_DECODE_THREAD_STATE), false);
			pthread_join(psVideoDecodeRes->tVideoDecodeThread, &pvJoin);
			psVideoDecodeRes->tVideoDecodeThread = NULL;
	}
		
	if(psAudioDecodeRes->tAudioDecodeThread){
			AudioDecThread_IOCTL(psAudioDecodeRes, eDECODE_IOCTL_SET_STATE, (void *)eDecThreadState, sizeof(E_DECODE_THREAD_STATE), false);
			pthread_join(psAudioDecodeRes->tAudioDecodeThread, &pvJoin);
			psAudioDecodeRes->tAudioDecodeThread = NULL;
	}
}


E_NM_ERRNO
PlayDecode_ThreadCreate(
	S_PLAY_DECODE_RES *psDecodeRes
)
{
	int iRet;
	E_DECODE_THREAD_STATE eDecThreadState;
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes = &psDecodeRes->sVideoDecodeRes;
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes = &psDecodeRes->sAudioDecodeRes;

	pthread_attr_t sAudioThreadAttr;
	
	pthread_attr_init(&sAudioThreadAttr);
	pthread_attr_setstacksize(&sAudioThreadAttr, 1024 * 10);
	
	if(psVideoDecodeRes->psVideoCodecIF){
		iRet = pthread_create(&psVideoDecodeRes->tVideoDecodeThread, NULL, VideoDeocdeWorkerThread, psVideoDecodeRes);
		if(iRet != 0){
			goto PlayDecode_ThreadCreate_fail;
		}
		
		//Wait thread running
		while(1){
			VideoDecThread_IOCTL(psVideoDecodeRes, eDECODE_IOCTL_GET_STATE, &eDecThreadState, sizeof(E_DECODE_THREAD_STATE *), true);
			if(eDecThreadState != eDECODE_THREAD_STATE_INIT)
				break;
			usleep(1000);
		}
	}

	
	if(psAudioDecodeRes->psAudioCodecIF){
		iRet = pthread_create(&psAudioDecodeRes->tAudioDecodeThread, &sAudioThreadAttr, AudioDeocdeWorkerThread, psAudioDecodeRes);
		if(iRet != 0){
			goto PlayDecode_ThreadCreate_fail;
		}
		//Wait thread running
		while(1){
			AudioDecThread_IOCTL(psAudioDecodeRes, eDECODE_IOCTL_GET_STATE, &eDecThreadState, sizeof(E_DECODE_THREAD_STATE *), true);
			if(eDecThreadState != eDECODE_THREAD_STATE_INIT)
				break;
			usleep(1000);
		}
	}

	return eNM_ERRNO_NONE;
	
PlayDecode_ThreadCreate_fail:

	PlayDecode_ThreadDestroy(psDecodeRes);
	
	return eNM_ERRNO_OS;
}

E_NM_ERRNO
PlayDecode_Run(
	S_PLAY_DECODE_RES *psDecodeRes
)
{
	E_DECODE_THREAD_STATE eDecThreadState;
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes = &psDecodeRes->sVideoDecodeRes;
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes = &psDecodeRes->sAudioDecodeRes;

	eDecThreadState = eDECODE_THREAD_STATE_RUN;
	if(psVideoDecodeRes->tVideoDecodeThread){
			VideoDecThread_IOCTL(psVideoDecodeRes, eDECODE_IOCTL_SET_STATE, (void *)eDecThreadState, sizeof(E_DECODE_THREAD_STATE), true);
	}
	
	if(psAudioDecodeRes->tAudioDecodeThread){
			AudioDecThread_IOCTL(psAudioDecodeRes, eDECODE_IOCTL_SET_STATE, (void *)eDecThreadState, sizeof(E_DECODE_THREAD_STATE), true);
	}

	return eNM_ERRNO_NONE;
}

E_NM_ERRNO
PlayDecode_Stop(
	S_PLAY_DECODE_RES *psDecodeRes
)
{
	E_DECODE_THREAD_STATE eDecThreadState;
	S_PLAY_VIDEO_DECODE_RES *psVideoDecodeRes = &psDecodeRes->sVideoDecodeRes;
	S_PLAY_AUDIO_DECODE_RES *psAudioDecodeRes = &psDecodeRes->sAudioDecodeRes;

	eDecThreadState = eDECODE_THREAD_STATE_IDLE;
	if(psVideoDecodeRes->tVideoDecodeThread){
			VideoDecThread_IOCTL(psVideoDecodeRes, eDECODE_IOCTL_SET_STATE, (void *)eDecThreadState, sizeof(E_DECODE_THREAD_STATE), true);
	}
	
	if(psAudioDecodeRes->tAudioDecodeThread){
			AudioDecThread_IOCTL(psAudioDecodeRes, eDECODE_IOCTL_SET_STATE, (void *)eDecThreadState, sizeof(E_DECODE_THREAD_STATE), true);
	}

	return eNM_ERRNO_NONE;
}

