
/***************************************************************************//**
 * @file     MediumQueue.c
 * @brief    Medium queue
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MediumQueue.h"

#include "FreeRTOS.h"
#include "semphr.h"

// medium resource data
typedef struct medium_queue_data_t {
	uint8_t		 	*pu8Buf;					// data buffer
	uint32_t		u32BufSize;					// data buffer byte size
	uint32_t		u32DataLen;					// data length in buffer
	uint64_t		u64DataUpdateTime;			// data update timestamp
	uint64_t		u64DataSeqNumber;			// data sequence number
	void			*pvDataPriv;				// data priv attribute					
} S_MEDIUM_QUEUE_DATA;

// medium queue
typedef struct medium_queue_t {
    SemaphoreHandle_t     hMutex;
	S_MEDIUM_QUEUE_DATA	asQueueData[DEF_MEDIUM_QUEUE_CNT];
	volatile int32_t i32DataReadIdx;
	volatile int32_t i32DataWriteIdx;
	PFN_GET_CALLER_STATE pfnGetCaller;
	void *pvGetCallerPriv;
	bool bSkipNonIFrame;
	uint32_t u32DiscardFrames;
} S_MEDIUM_QUEUE;

// New medium queue
PV_MEDIUM_QUEUE
MediumQueue_New(
	PFN_GET_CALLER_STATE pfnGetCaller,
	void *pvGetCallerPriv
)
{
	S_MEDIUM_QUEUE *psQueue;

	psQueue = calloc(1, sizeof(S_MEDIUM_QUEUE));

	if(!psQueue)
		return NULL;

	psQueue->hMutex = xSemaphoreCreateMutex();
	if(psQueue->hMutex == NULL){
		free(psQueue);
		return NULL;
	}
	
	psQueue->pfnGetCaller = pfnGetCaller;
	psQueue->pvGetCallerPriv = pvGetCallerPriv;
	
	return psQueue;
}

// Free medium queue
void
MediumQueue_Free(
	PV_MEDIUM_QUEUE *ppsMediumQueue
)
{
	int32_t i;
	S_MEDIUM_QUEUE_DATA *psQueueData;
	S_MEDIUM_QUEUE *psMediumQueue;

	if((ppsMediumQueue == NULL) || (*ppsMediumQueue == NULL))
		return;

	psMediumQueue = (S_MEDIUM_QUEUE *)*ppsMediumQueue;
	
	for(i = 0; i < DEF_MEDIUM_QUEUE_CNT; i ++){
		psQueueData = &psMediumQueue->asQueueData[i];
		if(psQueueData->pu8Buf){
			free((void *)psQueueData->pu8Buf);
		}
	}

	vSemaphoreDelete(psMediumQueue->hMutex);
	free((void *)psMediumQueue);
	*ppsMediumQueue = NULL;
}

// Export medium data
E_MEDIUM_QUEUE_ERRNO
MediumQueue_ExportQueueData(
	PV_MEDIUM_QUEUE pvMediumQueue,
	uint8_t 	*pu8DataBuf,
	uint32_t 	u32DataLen,
	uint64_t	u64TimeStamp,
	bool		bVideoData,
	E_UTIL_H264_NALTYPE_MASK eNALType,
	void		*pvPriv
)
{
	S_MEDIUM_QUEUE_DATA *psQueueData;
	int32_t i32WriteIdx;
	bool bBufFull = false;
	S_MEDIUM_QUEUE *psMediumQueue = (S_MEDIUM_QUEUE *)pvMediumQueue;

	xSemaphoreTake(psMediumQueue->hMutex, portMAX_DELAY);

	i32WriteIdx = (psMediumQueue->i32DataWriteIdx + 1) & (DEF_MEDIUM_QUEUE_CNT - 1);
	
	if(i32WriteIdx == psMediumQueue->i32DataReadIdx){
		// buffer full
		bBufFull = true;
		
		if(bVideoData == false){
			//Don't discard/overwrite audio data. Return buffer full
			xSemaphoreGive(psMediumQueue->hMutex);
			return eMEDUIM_QUEUE_FULL;
		}
	}

	if((bVideoData == true) && (bBufFull == true) && (!(eNALType & eUTIL_H264_NAL_I))){
		//Discard non-I frame if buffer is full
		psMediumQueue->bSkipNonIFrame = true;
		psMediumQueue->u32DiscardFrames ++;
		xSemaphoreGive(psMediumQueue->hMutex);
		return eMEDUIM_QUEUE_NONE;
	}
	else if((bVideoData == true) && (psMediumQueue->bSkipNonIFrame == true) && (!(eNALType & eUTIL_H264_NAL_I))){
		//Discard non-I frame until we have I frame
		psMediumQueue->u32DiscardFrames ++;
		xSemaphoreGive(psMediumQueue->hMutex);
		return eMEDUIM_QUEUE_NONE;
	}
	else if((bVideoData == true) && (bBufFull == true) && (eNALType & eUTIL_H264_NAL_I)){
		//Overwrite the last frame if buffer is full and new frame is I frame
		i32WriteIdx = (psMediumQueue->i32DataWriteIdx - 1) & (DEF_MEDIUM_QUEUE_CNT - 1);
	}
	else{
		i32WriteIdx = psMediumQueue->i32DataWriteIdx;
	}

	if(psMediumQueue->bSkipNonIFrame == true){
		printf("RTP reader medium resource discard %ld frames \n", psMediumQueue->u32DiscardFrames);
		psMediumQueue->u32DiscardFrames = 0;
	}
	
	psMediumQueue->bSkipNonIFrame = false;
	
	psQueueData = &psMediumQueue->asQueueData[i32WriteIdx];
	
	if(psQueueData->u32BufSize < u32DataLen){
		// Realloc data buff
		uint8_t *pu8Temp = realloc(psQueueData->pu8Buf, u32DataLen);
		
		if(!pu8Temp){
			printf("Realloc buffer failed \n");
			xSemaphoreGive(psMediumQueue->hMutex);
			return eMEDUIM_QUEUE_NO_MEM;
		}

		psQueueData->pu8Buf = pu8Temp;
		psQueueData->u32BufSize = u32DataLen;
	}
	else if((psQueueData->u32BufSize > 2048) && (u32DataLen < (psQueueData->u32BufSize / 2))){
		free((void *)psQueueData->pu8Buf);
		psQueueData->pu8Buf = malloc(u32DataLen);
		if(psQueueData->pu8Buf == NULL){
			printf("Allocate buffer failed \n");
			psQueueData->u32BufSize = 0;
			xSemaphoreGive(psMediumQueue->hMutex);
			return eMEDUIM_QUEUE_NO_MEM;
		}
		psQueueData->u32BufSize = u32DataLen;
	}

	memcpy(psQueueData->pu8Buf, pu8DataBuf, u32DataLen);
	psQueueData->u32DataLen = u32DataLen;
	psQueueData->u64DataUpdateTime = u64TimeStamp;
	psQueueData->u64DataSeqNumber ++;
	psQueueData->pvDataPriv = pvPriv;
	
	i32WriteIdx = (i32WriteIdx + 1) & (DEF_MEDIUM_QUEUE_CNT - 1);

	psMediumQueue->i32DataWriteIdx = i32WriteIdx;
	xSemaphoreGive(psMediumQueue->hMutex);

	return eMEDUIM_QUEUE_NONE;
}

// Touch medium data. Without update data read index.
E_MEDIUM_QUEUE_ERRNO
MediumQueue_TouchQueueData(
	PV_MEDIUM_QUEUE pvMediumQueue,
	uint8_t 	*pu8DataBuf,
	uint32_t 	u32DataBufLimit,
	uint32_t 	*pu32DataLen,
	uint64_t	*pu64TimeStamp,
	void		**ppvPriv
)
{
	S_MEDIUM_QUEUE_DATA *psQueueData;
	S_MEDIUM_QUEUE *psMediumQueue = (S_MEDIUM_QUEUE *)pvMediumQueue;
	int32_t i32ReadIdx;

	*pu32DataLen = 0;
	*pu64TimeStamp = 0;
	*ppvPriv = NULL;

	xSemaphoreTake(psMediumQueue->hMutex, portMAX_DELAY);

	i32ReadIdx = psMediumQueue->i32DataReadIdx;
	psQueueData = &psMediumQueue->asQueueData[i32ReadIdx];

	if(i32ReadIdx == psMediumQueue->i32DataWriteIdx){
		xSemaphoreGive(psMediumQueue->hMutex);
		return eMEDUIM_QUEUE_EMPTY;
	}

	if(u32DataBufLimit < psQueueData->u32DataLen){
		printf("Data buffer size is too samll \n");
		xSemaphoreGive(psMediumQueue->hMutex);
		return eMEDUIM_QUEUE_SIZE;
	}

	memcpy(pu8DataBuf, psQueueData->pu8Buf, psQueueData->u32DataLen);
	*pu32DataLen = psQueueData->u32DataLen;
	*pu64TimeStamp = psQueueData->u64DataUpdateTime;
	*ppvPriv = psQueueData->pvDataPriv;

	xSemaphoreGive(psMediumQueue->hMutex);

	return eMEDUIM_QUEUE_NONE;
}

// Flush medium data. Update data read index only.
E_MEDIUM_QUEUE_ERRNO
MediumQueue_FlushQueueData(
	PV_MEDIUM_QUEUE pvMediumQueue
)
{
	S_MEDIUM_QUEUE *psMediumQueue = (S_MEDIUM_QUEUE *)pvMediumQueue;

	int32_t i32ReadIdx;

	xSemaphoreTake(psMediumQueue->hMutex, portMAX_DELAY);

	i32ReadIdx = psMediumQueue->i32DataReadIdx;

	if(i32ReadIdx == psMediumQueue->i32DataWriteIdx){
		xSemaphoreGive(psMediumQueue->hMutex);
		return eMEDUIM_QUEUE_EMPTY;
	}

	psMediumQueue->i32DataReadIdx = (psMediumQueue->i32DataReadIdx + 1) & (DEF_MEDIUM_QUEUE_CNT - 1);

	xSemaphoreGive(psMediumQueue->hMutex);
	return eMEDUIM_QUEUE_NONE;
}

// Import medium data
E_MEDIUM_QUEUE_ERRNO
MediumQueue_ImportQueueData(
	PV_MEDIUM_QUEUE pvMediumQueue,
	uint8_t 	*pu8DataBuf,
	uint32_t 	u32DataBufLimit,
	uint32_t 	*pu32DataLen,
	uint64_t	*pu64TimeStamp,
	void		**ppvPriv
)
{
	S_MEDIUM_QUEUE *psMediumQueue = (S_MEDIUM_QUEUE *)pvMediumQueue;
	S_MEDIUM_QUEUE_DATA *psQueueData;
	int32_t i32ReadIdx;

	*pu32DataLen = 0;
	*pu64TimeStamp = 0;
	*ppvPriv = NULL;

	xSemaphoreTake(psMediumQueue->hMutex, portMAX_DELAY);

	i32ReadIdx = psMediumQueue->i32DataReadIdx;
	psQueueData = &psMediumQueue->asQueueData[i32ReadIdx];

	if(i32ReadIdx == psMediumQueue->i32DataWriteIdx){
		xSemaphoreGive(psMediumQueue->hMutex);
		return eMEDUIM_QUEUE_EMPTY;
	}

	if(u32DataBufLimit < psQueueData->u32DataLen){
		printf("Data buffer size is too samll\n");
		xSemaphoreGive(psMediumQueue	->hMutex);
		return eMEDUIM_QUEUE_SIZE;
	}

	memcpy(pu8DataBuf, psQueueData->pu8Buf, psQueueData->u32DataLen);
	*pu32DataLen = psQueueData->u32DataLen;
	*pu64TimeStamp = psQueueData->u64DataUpdateTime;
	*ppvPriv = psQueueData->pvDataPriv;

	psMediumQueue->i32DataReadIdx = (psMediumQueue->i32DataReadIdx + 1) & (DEF_MEDIUM_QUEUE_CNT - 1);

	xSemaphoreGive(psMediumQueue->hMutex);
	return eMEDUIM_QUEUE_NONE; 

}

// Get medium info
E_MEDIUM_QUEUE_ERRNO
MediumQueue_GetQueueInfo(
	PV_MEDIUM_QUEUE pvMediumQueue,
	uint8_t 	**pu8DataBuf,
	uint32_t 	*pu32DataLen,
	uint64_t	*pu64TimeStamp,
	void		**ppvPriv
)
{
	S_MEDIUM_QUEUE *psMediumQueue = (S_MEDIUM_QUEUE *)pvMediumQueue;
	S_MEDIUM_QUEUE_DATA *psQueueData;
	int32_t i32ReadIdx;

	*pu32DataLen = 0;
	*pu64TimeStamp = 0;
	*ppvPriv = NULL;
	*pu8DataBuf = NULL;

	xSemaphoreTake(psMediumQueue->hMutex, portMAX_DELAY);

	i32ReadIdx = psMediumQueue->i32DataReadIdx;
	psQueueData = &psMediumQueue->asQueueData[i32ReadIdx];

	if(i32ReadIdx == psMediumQueue->i32DataWriteIdx){
		xSemaphoreGive(psMediumQueue->hMutex);
		return eMEDUIM_QUEUE_EMPTY;
	}

	*pu8DataBuf = psQueueData->pu8Buf;
	*pu32DataLen = psQueueData->u32DataLen;
	*pu64TimeStamp = psQueueData->u64DataUpdateTime;
	*ppvPriv = psQueueData->pvDataPriv;

	xSemaphoreGive(psMediumQueue->hMutex);
	return eMEDUIM_QUEUE_NONE; 
}


