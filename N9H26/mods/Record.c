/***************************************************************************//**
 * @file     Record.c
 * @brief    Media record source file
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#define _MODE_T_DECLARED
#define _CLOCKID_T_DECLARED
#define _TIMER_T_DECLARED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Scheduler includes. */
/* FreeRTOS+POSIX. */
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"

#include "Record.h"
#include "NVTMedia.h"
#include "NVTMedia_SAL_FS.h"
#include "Record_FileMgr.h"

typedef struct
{
    char *szFileName;
    void *pvOpenRes;
} S_MEDIA_ATTR;

typedef enum
{
	eREC_WORKER_STOP,
	eREC_WORKER_TO_STOP,
	eREC_WORKER_RUN,
}E_REC_WORKER_STATE;

typedef struct
{
    HRECORD					m_hRecord;
    uint64_t				m_u64CurTimeInMs;
    uint64_t				m_u64CurFreeSpaceInByte;
    uint64_t				m_u64CurWroteSizeInByte;

//    S_NM_RECORD_IF			m_sRecIf;
    S_NM_RECORD_CONTEXT		m_sRecCtx;
    S_MEDIA_ATTR			m_sCurMediaAttr;
    E_NM_RECORD_STATUS		m_eCurRecordingStatus;

    S_MEDIA_ATTR			m_sNextMediaAttr;
	bool					m_bCreateNextMedia;
	uint32_t 				m_u32EachRecFileDuration;


//////////////////////////////////////////////////////
	xSemaphoreHandle		m_tMediaMutex;
	E_REC_WORKER_STATE		m_eWorkerState;	
	bool					m_bPreviewEnalbe;

// device in, out driver
	record_vin_drv_t		m_sVinDrv;
	record_ain_drv_t		m_sAinDrv;
	record_vout_drv_t		m_sVoutDrv;
} S_RECORDER;

static S_RECORDER g_sRecorder =
{
    .m_hRecord = (HRECORD)eNM_INVALID_HANDLE,
    .m_eWorkerState = eREC_WORKER_STOP,
    0
};

//////////////////////////////////////////////////////////////////////////////////////////////
//	Local functions
//////////////////////////////////////////////////////////////////////////////////////////////


//Close current media and switch next media to current media
static int32_t
CloseAndChangeMedia(void)
{
	
	xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	

	//Close current media
	if(g_sRecorder.m_sCurMediaAttr.pvOpenRes)
		NMRecord_Close((HRECORD)eNM_INVALID_HANDLE, &g_sRecorder.m_sCurMediaAttr.pvOpenRes);
		
	if(g_sRecorder.m_sCurMediaAttr.szFileName){
		//Update current media information to file manager
		FileMgr_UpdateFileInfo(DEF_RECORD_FILE_MGR_TYPE, g_sRecorder.m_sCurMediaAttr.szFileName);
		free(g_sRecorder.m_sCurMediaAttr.szFileName);
	}
		
	//Change current media to next media
	g_sRecorder.m_sCurMediaAttr.pvOpenRes = g_sRecorder.m_sNextMediaAttr.pvOpenRes;
	g_sRecorder.m_sCurMediaAttr.szFileName = g_sRecorder.m_sNextMediaAttr.szFileName;
	
	//Set next media to empty
	g_sRecorder.m_sNextMediaAttr.pvOpenRes = NULL;
	g_sRecorder.m_sNextMediaAttr.szFileName = NULL;
	
	xSemaphoreGive(g_sRecorder.m_tMediaMutex);	

	return 0;
}

//Create and register next media
static int32_t 
CreateAndRegNextMedia(
	HRECORD	hRecord,
	S_NM_RECORD_CONTEXT *psRecCtx	
)
{
	S_MEDIA_ATTR *psMediaAttr = NULL;
	int32_t i32Ret;
	E_NM_ERRNO eNMRet;
	S_NM_RECORD_IF sRecIf;
		
	if(hRecord == (HRECORD)eNM_INVALID_HANDLE){
		return 0;
	}
	
	if((g_sRecorder.m_sCurMediaAttr.pvOpenRes != NULL) && (g_sRecorder.m_sNextMediaAttr.pvOpenRes != NULL)){
		return 0;
	}
	
	if(g_sRecorder.m_sCurMediaAttr.pvOpenRes == NULL){
		psMediaAttr = &g_sRecorder.m_sCurMediaAttr;
	}
	else{
		psMediaAttr = &g_sRecorder.m_sNextMediaAttr;
	}
	
	//Get new file name from file manager
	i32Ret = FileMgr_CreateNewFileName(	DEF_RECORD_FILE_MGR_TYPE, 
										DEF_REC_FOLDER_PATH,
										0,
										&psMediaAttr->szFileName,
										0,
										DEF_REC_RESERVE_STORE_SPACE);

	if(i32Ret != 0){
		NMLOG_ERROR("Unable create new file name %d \n", i32Ret)
		return i32Ret;		
	}
	
	//Open new media
	eNMRet = NMRecord_Open(
		psMediaAttr->szFileName,
		DEF_NM_MEDIA_FILE_TYPE,
		g_sRecorder.m_u32EachRecFileDuration,
		psRecCtx,
		&sRecIf,
		&psMediaAttr->pvOpenRes	
	);

	if(eNMRet != eNM_ERRNO_NONE){
		NMLOG_ERROR("Unable NMRecord_Open(): %d\n", eNMRet);
		return eNMRet;		
	}

	//Append new file name to file manager
	FileMgr_AppendFileInfo(DEF_RECORD_FILE_MGR_TYPE, psMediaAttr->szFileName);	

	//Register new media to next media
	eNMRet = NMRecord_RegNextMedia(hRecord, sRecIf.psMediaIF, sRecIf.pvMediaRes, NULL);
	if(eNMRet != eNM_ERRNO_NONE){
		NMLOG_ERROR("Unable NMRecord_RegNextMedia(): %d\n", eNMRet);
		return eNMRet;		
	}

	return 0;
}


static void ResetRecorderMember(void)
{
	g_sRecorder.m_hRecord = (HRECORD)eNM_INVALID_HANDLE;
	g_sRecorder.m_u64CurTimeInMs = 0;
	g_sRecorder.m_u64CurFreeSpaceInByte = 0;
	g_sRecorder.m_u64CurWroteSizeInByte = 0;
	memset(&g_sRecorder.m_sRecCtx, 0 , sizeof(S_NM_RECORD_CONTEXT));
	memset(&g_sRecorder.m_sCurMediaAttr, 0 , sizeof(S_MEDIA_ATTR));
	g_sRecorder.m_eCurRecordingStatus = eNM_RECORD_STATUS_STOP;

	memset(&g_sRecorder.m_sNextMediaAttr, 0, sizeof(S_MEDIA_ATTR));	
	g_sRecorder.m_bCreateNextMedia = false;
	g_sRecorder.m_u32EachRecFileDuration = eNM_UNLIMIT_TIME;
		
}

static bool
AudioIn_FillCB(
    S_NM_AUDIO_CTX  *psAudioCtx
)
{
	record_ain_drv_t *psAinDrv = &g_sRecorder.m_sAinDrv;

	if(psAinDrv->pfn_ain_fill_cb){
		if(psAinDrv->pfn_ain_fill_cb(psAudioCtx->u32SamplePerBlock, psAudioCtx->pu8DataBuf, &psAudioCtx->u64DataTime) == true){
			psAudioCtx->u32DataSize = psAudioCtx->u32SamplePerBlock * psAudioCtx->u32Channel * 2;
			return true;
		}
	}

    return false;
}

#if 0
    static uint64_t s_u64PrevVinTime = 0;
#endif

static bool
VideoIn_FillCB(
    S_NM_VIDEO_CTX  *psVideoCtx
)
{
	record_vin_drv_t *psVinDrv = &g_sRecorder.m_sVinDrv;


#if 0
    uint64_t u64CurVinTime = NMUtil_GetTimeMilliSec();
    uint64_t u64DiffTime = u64CurVinTime - s_u64PrevVinTime;

    if (u64DiffTime > 150)
        printf("VideoIn_FillCB time over %d \n", (uint32_t) u64DiffTime);

    s_u64PrevVinTime = u64CurVinTime;
#endif
	
	if(psVinDrv->pfn_vin_planar_img_fill_cb){
		image_t sPlanarImg;
		uint64_t u64FrameTime;

		if(psVinDrv->pfn_vin_planar_img_fill_cb(&sPlanarImg, &u64FrameTime) == true){
			psVideoCtx->pu8DataBuf = sPlanarImg.data;
			psVideoCtx->u64DataTime = u64FrameTime;
			return true;
		}
	}

    return false;
}

static void
Record_StatusCB(
    E_NM_RECORD_STATUS eStatus,
    S_NM_RECORD_INFO *psInfo,
    void *pvPriv
)
{

    g_sRecorder.m_eCurRecordingStatus = eStatus;


    switch (eStatus)
    {
    case eNM_RECORD_STATUS_PAUSE:
        printf("Recorder status: PAUSE \n");
        break;
    case eNM_RECORD_STATUS_RECORDING:
        printf("Recorder status: RECORDING \n");
        if (psInfo)
        {
            g_sRecorder.m_u64CurTimeInMs = psInfo->u32Duration;
            g_sRecorder.m_u64CurWroteSizeInByte = psInfo->u64TotalChunkSize;
        }
        break;
    case eNM_RECORD_STATUS_CHANGE_MEDIA:
        printf("Recorder status: CHANGE MEDIA \n");
		//Current media is finish. Start record next media. 
		CloseAndChangeMedia();
		xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	
		//Set create next media flag
		g_sRecorder.m_bCreateNextMedia = TRUE;
		xSemaphoreGive(g_sRecorder.m_tMediaMutex);	
		break;
    case eNM_RECORD_STATUS_STOP:
        g_sRecorder.m_u64CurTimeInMs = psInfo->u32Duration;
        printf("Recorder status: STOP \n");
        break;
	default :
		break;
    } //switch

}

int Recorder_is_over_limitedsize(void)
{
    return (g_sRecorder.m_u64CurFreeSpaceInByte - g_sRecorder.m_u64CurWroteSizeInByte) <= DEF_REC_RESERVE_STORE_SPACE;
}

static void *worker_recorder(void *pvParameters)
{
	uint64_t u64FrameTime;
	image_t sPacketImg;
	record_vin_drv_t *psVinDrv = &g_sRecorder.m_sVinDrv;
	record_vout_drv_t *psVoutDrv = &g_sRecorder.m_sVoutDrv;
	uint64_t u64CreateNewMediaTime = 0;

	ResetRecorderMember();
	

	g_sRecorder.m_tMediaMutex = xSemaphoreCreateMutex();
	if(g_sRecorder.m_tMediaMutex == NULL){
		printf("Unable create media mutex \n");
        goto exit_worker_recorder;
	}

	g_sRecorder.m_eWorkerState = eREC_WORKER_RUN;
	
	while(g_sRecorder.m_eWorkerState == eREC_WORKER_RUN){

		if(g_sRecorder.m_bPreviewEnalbe){
			if(psVinDrv->pfn_vin_packet_img_fill_cb){
				//Show preview
				if (psVinDrv->pfn_vin_packet_img_fill_cb(&sPacketImg, &u64FrameTime) == true){
					if(psVoutDrv->pfn_vout_flush_cb){
						psVoutDrv->pfn_vout_flush_cb(psVoutDrv->vout_obj, sPacketImg.data);
					}
				}
			}
		}

		//Setup create next media time
		if(g_sRecorder.m_bCreateNextMedia == TRUE){
			xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	
			u64CreateNewMediaTime = NMUtil_GetTimeMilliSec() + (g_sRecorder.m_u32EachRecFileDuration / 2);
			g_sRecorder.m_bCreateNextMedia = FALSE;
			xSemaphoreGive(g_sRecorder.m_tMediaMutex);
		}
		
		//Creat next media
		if(NMUtil_GetTimeMilliSec() > u64CreateNewMediaTime){
			xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	
			CreateAndRegNextMedia(g_sRecorder.m_hRecord, &g_sRecorder.m_sRecCtx);		
			xSemaphoreGive(g_sRecorder.m_tMediaMutex);
			u64CreateNewMediaTime = ULLONG_MAX;
		}

        if (g_sRecorder.m_eCurRecordingStatus == eNM_RECORD_STATUS_RECORDING){
			//Checking storage free space is enough or not
			if((g_sRecorder.m_u32EachRecFileDuration == eNM_UNLIMIT_TIME) && (Recorder_is_over_limitedsize())){
				printf("No enough space to record. Stop recording.\n");
				Record_Stop();
			}
        }

        vTaskDelay(5);
	}

	if(g_sRecorder.m_tMediaMutex){
		vSemaphoreDelete(g_sRecorder.m_tMediaMutex);
		g_sRecorder.m_tMediaMutex = NULL;
	}

exit_worker_recorder:

	g_sRecorder.m_eWorkerState = eREC_WORKER_STOP;
    printf("Die... %s\n", __func__);

    return NULL;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//	Global functions
/////////////////////////////////////////////////////////////////////////////////////////////

void Record_RegisterVinDrv(record_vin_drv_t *psVinDrv)
{
	if(psVinDrv)
		memcpy(&g_sRecorder.m_sVinDrv, psVinDrv, sizeof(record_vin_drv_t));
}

void Record_RegisterAinDrv(record_ain_drv_t *psAinDrv)
{
	printf("DDDDDD Record_RegisterAinDrv psAinDrv->pfn_ain_fill_cb %x \n", psAinDrv->pfn_ain_fill_cb);
	printf("DDDDDD Record_RegisterAinDrv psAinDrv->u32SampleRate %d \n", psAinDrv->u32SampleRate);
	printf("DDDDDD Record_RegisterAinDrv psAinDrv->u32Channel %d \n", psAinDrv->u32Channel);

	if(psAinDrv)
		memcpy(&g_sRecorder.m_sAinDrv, psAinDrv, sizeof(record_ain_drv_t));

}

void Record_RegisterVoutDrv(record_vout_drv_t *psVoutDrv)
{
	printf("DDDDDD Record_RegisterVinDrv psVoutDrv->pfn_vout_flush_cb %x \n", psVoutDrv->pfn_vout_flush_cb);
	printf("DDDDDD Record_RegisterVinDrv psVoutDrv->vout_obj %x \n", psVoutDrv->vout_obj);

	if(psVoutDrv)
		memcpy(&g_sRecorder.m_sVoutDrv, psVoutDrv, sizeof(record_vout_drv_t));

}


int Record_Init(
	bool bEnablePreview
)
{
	g_sRecorder.m_bPreviewEnalbe = bEnablePreview;

	if(g_sRecorder.m_eWorkerState != eREC_WORKER_STOP){
		printf("Recorder already initiate \n");
		return -1;
	}

	//Creaet record task
    pthread_t pxMain_worker;
    struct sched_param;
    pthread_attr_t attr;
	int ret;

	//Create recorder worker thread
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32 * 1024);

    ret = pthread_create(&pxMain_worker, &attr, worker_recorder, NULL);
	printf("DDDD Record_Init --- thread create ret %x \n", ret);
	
		
	return 0;
}

void Record_DeInit(void)
{
	//TODO: if under recording file, stop record first.

	if(g_sRecorder.m_eWorkerState != eREC_WORKER_STOP){
		g_sRecorder.m_eWorkerState = eREC_WORKER_TO_STOP;
		
		while(1){
			if(g_sRecorder.m_eWorkerState == eREC_WORKER_STOP)
				break;
		}
	}
}

int Record_Start(
	const char *szRecFolderPath,
	uint32_t u32FileDuration
)
{
    E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;
	record_vin_drv_t *psVinDrv = &g_sRecorder.m_sVinDrv;
	record_ain_drv_t *psAinDrv = &g_sRecorder.m_sAinDrv;

	xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	

    if (g_sRecorder.m_hRecord == (HRECORD)eNM_INVALID_HANDLE)
    {
        int32_t i32Ret;
		S_NM_RECORD_IF	sRecIf;

		ResetRecorderMember();
			
		g_sRecorder.m_u32EachRecFileDuration = u32FileDuration;		
        g_sRecorder.m_hRecord = (HRECORD)eNM_INVALID_HANDLE;		
        g_sRecorder.m_u64CurFreeSpaceInByte = disk_free_space(szRecFolderPath);
		memset(&sRecIf, 0 , sizeof(S_NM_RECORD_IF));

		if(g_sRecorder.m_u32EachRecFileDuration == eNM_UNLIMIT_TIME){
			//Checking storage free space is enough or not
			if (g_sRecorder.m_u64CurFreeSpaceInByte < DEF_REC_RESERVE_STORE_SPACE)
			{
				eNMRet = eNM_ERRNO_SIZE;
				goto exit_recorder_start;
			}
		}
        printf("Free space in disk %d\n", (uint32_t)g_sRecorder.m_u64CurFreeSpaceInByte);

		//Get current media file name
        i32Ret = FileMgr_CreateNewFileName(DEF_RECORD_FILE_MGR_TYPE,
                                           (char *)szRecFolderPath,
                                           0,
                                           &g_sRecorder.m_sCurMediaAttr.szFileName,
                                           0,
                                           DEF_REC_RESERVE_STORE_SPACE);

        if (i32Ret != 0)
        {
            NMLOG_ERROR("Unable create new file name %d \n", i32Ret)
            eNMRet = eNM_ERRNO_OS;
            goto exit_recorder_start;
        }

        printf("Recorded file name is %s.\n", g_sRecorder.m_sCurMediaAttr.szFileName);

		if(psVinDrv->ePixelFormat != ePLANAR_PIXFORMAT_YUV420MB){
            NMLOG_ERROR("Unable find Vin YUV420 MB pipe \n")
            eNMRet = eNM_ERRNO_CODEC_TYPE;
            goto exit_recorder_start;
		}

		//Setup video fill context
        g_sRecorder.m_sRecCtx.sFillVideoCtx.eVideoType = eNM_CTX_VIDEO_YUV420P_MB;
        g_sRecorder.m_sRecCtx.sFillVideoCtx.u32Width = psVinDrv->u32Width;
        g_sRecorder.m_sRecCtx.sFillVideoCtx.u32Height = psVinDrv->u32Height;
        g_sRecorder.m_sRecCtx.sFillVideoCtx.u32FrameRate = 30;   //TODO: psVinDrv->u32FrameRate;

		//Setup video media context
        g_sRecorder.m_sRecCtx.sMediaVideoCtx.eVideoType = eNM_CTX_VIDEO_H264;
        g_sRecorder.m_sRecCtx.sMediaVideoCtx.u32Width = psVinDrv->u32Width;
        g_sRecorder.m_sRecCtx.sMediaVideoCtx.u32Height = psVinDrv->u32Height;
        g_sRecorder.m_sRecCtx.sMediaVideoCtx.u32FrameRate = 30; //TODO:psPipeInfo->u32FrameRate;

		//Setup audio fill context
		g_sRecorder.m_sRecCtx.sFillAudioCtx.eAudioType = eNM_CTX_AUDIO_PCM_L16;
        g_sRecorder.m_sRecCtx.sFillAudioCtx.u32SampleRate = psAinDrv->u32SampleRate;
        g_sRecorder.m_sRecCtx.sFillAudioCtx.u32Channel = psAinDrv->u32Channel;

		//Setup audio media context
		g_sRecorder.m_sRecCtx.sMediaAudioCtx.eAudioType = eNM_CTX_AUDIO_AAC;
        g_sRecorder.m_sRecCtx.sMediaAudioCtx.u32SampleRate = psAinDrv->u32SampleRate;
        g_sRecorder.m_sRecCtx.sMediaAudioCtx.u32Channel = psAinDrv->u32Channel;
        g_sRecorder.m_sRecCtx.sMediaAudioCtx.u32BitRate = 64000;

		//Setup video and audio fill callback
        sRecIf.pfnVideoFill = VideoIn_FillCB;
        sRecIf.pfnAudioFill = AudioIn_FillCB;

		//Open current media
		eNMRet = NMRecord_Open(g_sRecorder.m_sCurMediaAttr.szFileName,
                               DEF_NM_MEDIA_FILE_TYPE,
                               g_sRecorder.m_u32EachRecFileDuration,
                               &g_sRecorder.m_sRecCtx,
                               &sRecIf,
                               &g_sRecorder.m_sCurMediaAttr.pvOpenRes
                              );

        if (eNMRet != eNM_ERRNO_NONE)
        {
            NMLOG_ERROR("Unable NMRecord_Open(): %d\n", eNMRet);
            goto exit_recorder_start;
        }

		//Append current media file name to file manager
        FileMgr_AppendFileInfo(DEF_RECORD_FILE_MGR_TYPE, g_sRecorder.m_sCurMediaAttr.szFileName);
		//Clean audio-in PCM buffer
//        AudioIn_CleanPCMBuff();
#if 0
		//Start record
        eNMRet = NMRecord_Record(
                     &g_sRecorder.m_hRecord,
                     g_sRecorder.m_u32EachRecFileDuration,
                     &sRecIf,
                     &g_sRecorder.m_sRecCtx,
                     Record_StatusCB,
                     NULL);

        if (eNMRet != eNM_ERRNO_NONE)
        {
            NMLOG_ERROR("Unable start NMRecord_Record(): %d\n", eNMRet);
            goto exit_recorder_start;
        }

		if(g_sRecorder.m_u32EachRecFileDuration != eNM_UNLIMIT_TIME){
			g_sRecorder.m_bCreateNextMedia = TRUE;
		}
#endif
	}

	xSemaphoreGive(g_sRecorder.m_tMediaMutex);
    return eNMRet;

exit_recorder_start:

	xSemaphoreGive(g_sRecorder.m_tMediaMutex);
    return -(eNMRet);
}

int Record_Stop(void)
{
    E_NM_ERRNO eNMRet = eNM_ERRNO_NONE;

	xSemaphoreTake(g_sRecorder.m_tMediaMutex, portMAX_DELAY);	

	//Close next media
	if(g_sRecorder.m_sNextMediaAttr.pvOpenRes){
		NMRecord_Close((HRECORD)eNM_INVALID_HANDLE, &g_sRecorder.m_sNextMediaAttr.pvOpenRes);
		if(g_sRecorder.m_sNextMediaAttr.szFileName){
			//delete next media file
			unlink(g_sRecorder.m_sNextMediaAttr.szFileName);
			free(g_sRecorder.m_sNextMediaAttr.szFileName);
		}
	}

	//Close current media
	if(g_sRecorder.m_sCurMediaAttr.pvOpenRes){
		NMRecord_Close(g_sRecorder.m_hRecord, &g_sRecorder.m_sCurMediaAttr.pvOpenRes);
		if(g_sRecorder.m_sCurMediaAttr.szFileName)
			free(g_sRecorder.m_sCurMediaAttr.szFileName);
	}

	ResetRecorderMember();

	xSemaphoreGive(g_sRecorder.m_tMediaMutex);	
    return eNMRet;
}


