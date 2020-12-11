/***************************************************************************//**
 * @file     StorIF_SDCard.c
 * @brief    SD storage access function
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "py/obj.h"
#include "mods/pybirq.h"

#include "wblib.h"
#include "N9H26_SIC.h"

#include "StorIF.h"

#if MICROPY_PY_THREAD
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

static xSemaphoreHandle s_tStorIfMutex;
#define STORIF_MUTEX_LOCK()			xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY)
#define STORIF_MUTEX_UNLOCK()		xSemaphoreGive(s_tStorIfMutex)

#else
#define STORIF_MUTEX_LOCK
#define STORIF_MUTEX_UNLOCK
#endif

static FMI_SD_INFO_T *s_psSDInfo = NULL;

#define Sector_Size 512   //512 bytes
static uint32_t Tmp_Buffer[Sector_Size];

static E_STORIF_ERRNO
StorIF_SDCard_Init(
	int32_t i32Inst,
	void **ppStorRes
)
{
	int i32SectorNum = 0;
	
#if MICROPY_PY_THREAD
	s_tStorIfMutex = xSemaphoreCreateMutex();

	if(s_tStorIfMutex == NULL){
		printf("Unable create SD card mutex\n");
		return eSTORIF_ERRNO_NULL_PTR;
	}
	
	xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

	//--- Initial system clock for SD
	sicIoctl(SIC_SET_CLOCK, sysGetPLLOutputHz(eSYS_UPLL, sysGetExternalClock())/1000, 0, 0);    // clock from PLL

	//--- Enable AHB clock for SIC/SD/NAND, interrupt ISR, DMA, and FMI engineer
	sicOpen();

	STORIF_MUTEX_LOCK();
	
	if(i32Inst == 0){
		i32SectorNum = sicSdOpen0();
		s_psSDInfo = pSD0;
	}
	else if (i32Inst == 1){
		i32SectorNum = sicSdOpen1();
		s_psSDInfo = pSD1;
	}
	else if (i32Inst == 2){
		i32SectorNum = sicSdOpen2();
		s_psSDInfo = pSD2;
	}

	STORIF_MUTEX_UNLOCK();

	if(i32SectorNum <= 0)
		return eSTORIF_ERRNO_STOR_OPEN;

	return eSTORIF_ERRNO_NONE;
}

static int32_t
StorIF_SDCard_Detect(
	void *pvStorRes
)
{
	return(s_psSDInfo->bIsCardInsert);
}

/* DISK_DATA_T is defined at BSP source internal, copy it in here*/
#define STOR_STRING_LEN 32

typedef struct disk_data_t
{
    struct disk_data_t  *next;      /* next device */

    /* information about the device -- always good */
    unsigned int  totalSectorN;
    unsigned int  diskSize;         /* disk size in Kbytes */
    int           sectorSize;
    char          vendor[STOR_STRING_LEN];
    char          product[STOR_STRING_LEN];
    char          serial[STOR_STRING_LEN];
} DISK_DATA_T;

VOID fmiGet_SD_info(FMI_SD_INFO_T *pSD, DISK_DATA_T *_info);

static DISK_DATA_T s_sDiskInfo;

static E_STORIF_ERRNO
StorIF_SDCard_GetInfo(
	S_STORIF_INFO *psInfo,
	void *pvStorRes
)
{

	if(!StorIF_SDCard_Detect(pvStorRes)){
		return eSTORIF_ERRNO_NOT_READY;
	}
		
	// get information about SD card
	fmiGet_SD_info(s_psSDInfo, &s_sDiskInfo);
	
	psInfo->u32TotalSector = s_sDiskInfo.totalSectorN; 
	psInfo->u32DiskSize = s_sDiskInfo.diskSize;
	psInfo->u32SectorSize = s_sDiskInfo.sectorSize;
	psInfo->u32SubType = s_psSDInfo->uCardType;

	return eSTORIF_ERRNO_NONE;
}

static int32_t SDCard_Read (
    uint8_t *buff,     /* Data buffer to store read data */
    uint32_t sector,   /* Sector address (LBA) */
    uint32_t count      /* Number of sectors to read (1..128) */
)
{
    int32_t ret = 0;
    uint32_t shift_buf_flag = 0;
    uint32_t tmp_StartBufAddr;

//    printf("disk_read - sec:%d, cnt:%d, buff:0x%x\n",sector, count, (uint32_t)buff);

    if ((uint32_t)buff%4)
    {
        shift_buf_flag = 1;
    }

    if(shift_buf_flag == 1)
    {
        if(count == 1)
        {
			if(s_psSDInfo == pSD0)
				ret = sicSdRead0(sector, count, (INT32)(&Tmp_Buffer));
			else if(s_psSDInfo == pSD1)
				ret = sicSdRead1(sector, count, (INT32)(&Tmp_Buffer));
			else if(s_psSDInfo == pSD2)
				ret = sicSdRead2(sector, count, (INT32)(&Tmp_Buffer));
				
            memcpy(buff, (&Tmp_Buffer), count*s_sDiskInfo.sectorSize);
        }
        else
        {
            tmp_StartBufAddr = (((uint32_t)buff/4 + 1) * 4);

			if(s_psSDInfo == pSD0)
				ret = sicSdRead0(sector, (count -1), (INT32)(tmp_StartBufAddr));
			else if(s_psSDInfo == pSD1)
				ret = sicSdRead1(sector, (count -1), (INT32)(tmp_StartBufAddr));
			else if(s_psSDInfo == pSD2)
				ret = sicSdRead2(sector, (count -1), (INT32)(tmp_StartBufAddr));


            memcpy(buff, (void*)tmp_StartBufAddr, (s_sDiskInfo.sectorSize*(count-1)) );

			if(s_psSDInfo == pSD0)
				ret = sicSdRead0(sector+count-1, 1, (INT32)(&Tmp_Buffer));
			else if(s_psSDInfo == pSD1)
				ret = sicSdRead1(sector+count-1, 1, (INT32)(&Tmp_Buffer));
			else if(s_psSDInfo == pSD2)
				ret = sicSdRead2(sector+count-1, 1, (INT32)(&Tmp_Buffer));

            memcpy( (buff+(s_sDiskInfo.sectorSize*(count-1))), (void*)Tmp_Buffer, s_sDiskInfo.sectorSize);
        }
    }
    else
    {
		if(s_psSDInfo == pSD0)
			ret = sicSdRead0(sector, count, (INT32)(buff));
		else if(s_psSDInfo == pSD1)
			ret = sicSdRead1(sector, count, (INT32)(buff));
		else if(s_psSDInfo == pSD2)
			ret = sicSdRead2(sector, count, (INT32)(buff));
    }

    return ret;
}

static int32_t
StorIF_SDCard_ReadSector(
	uint8_t *pu8Buff,		/* Data buffer to store read data */
	uint32_t u32Sector,		/* Sector address (LBA) */
	uint32_t u32Count,		/* Number of sectors to read (1..128) */
	void *pvStorRes
)
{
	S_STORIF_INFO sStorInfo;
	int32_t i32Ret;
	
	if(StorIF_SDCard_GetInfo(pvStorRes, &sStorInfo) != eSTORIF_ERRNO_NONE)
		return 0;

	STORIF_MUTEX_LOCK();

	i32Ret = SDCard_Read(pu8Buff, u32Sector, u32Count);

	STORIF_MUTEX_UNLOCK();

	return i32Ret;
}

static int32_t SDCard_Write (
    const uint8_t *buff,   /* Data to be written */
    uint32_t sector,       /* Sector address (LBA) */
    uint32_t count          /* Number of sectors to write (1..128) */
)
{
    int32_t  ret = 0;
    uint32_t shift_buf_flag = 0;
    uint32_t tmp_StartBufAddr;
    uint32_t volatile i;

//    printf("disk_write - sec:%d, cnt:%d, buff:0x%x\n", sector, count, (uint32_t)buff);
    if ((uint32_t)buff%4)
    {
        shift_buf_flag = 1;
    }

    if(shift_buf_flag == 1)
    {
        if(count == 1)
        {
            memcpy((&Tmp_Buffer), buff, count*s_sDiskInfo.sectorSize);

			if(s_psSDInfo == pSD0)
				ret = sicSdWrite0(sector, count, (INT32)(&Tmp_Buffer));
			else if(s_psSDInfo == pSD1)
				ret = sicSdWrite1(sector, count, (INT32)(&Tmp_Buffer));
			else if(s_psSDInfo == pSD2)
				ret = sicSdWrite2(sector, count, (INT32)(&Tmp_Buffer));
        }
        else
        {
            tmp_StartBufAddr = (((uint32_t)buff/4 + 1) * 4);
            memcpy((void*)Tmp_Buffer, (buff+(s_sDiskInfo.sectorSize*(count-1))), s_sDiskInfo.sectorSize);

            for(i = (s_sDiskInfo.sectorSize*(count-1)); i > 0; i--)
            {
                memcpy((void *)(tmp_StartBufAddr + i - 1), (buff + i -1), 1);
            }

			if(s_psSDInfo == pSD0){
				ret = sicSdWrite0(sector, count-1, (INT32)(tmp_StartBufAddr));
				ret = sicSdWrite0((sector+count-1), 1, (INT32)(&Tmp_Buffer));
			}
			else if(s_psSDInfo == pSD1){
				ret = sicSdWrite1(sector, count-1, (INT32)(tmp_StartBufAddr));
				ret = sicSdWrite1((sector+count-1), 1, (INT32)(&Tmp_Buffer));
			}
			else if(s_psSDInfo == pSD2){
				ret = sicSdWrite2(sector, count-1, (INT32)(tmp_StartBufAddr));
				ret = sicSdWrite2((sector+count-1), 1, (INT32)(&Tmp_Buffer));
			}
        }
    }
    else
    {
		if(s_psSDInfo == pSD0)
			ret = sicSdWrite0(sector, count, (INT32)(buff));
		else if(s_psSDInfo == pSD1)
			ret = sicSdWrite1(sector, count, (INT32)(buff));
		else if(s_psSDInfo == pSD2)
			ret = sicSdWrite2(sector, count, (INT32)(buff));
    }

    return ret;
}


static int32_t
StorIF_SDCard_WriteSector(
	uint8_t *pu8Buff,		/* Data buffer to store read data */
	uint32_t u32Sector,		/* Sector address (LBA) */
	uint32_t u32Count,		/* Number of sectors to read (1..128) */
	void *pvStorRes
)
{
	S_STORIF_INFO sStorInfo;
	int32_t i32Ret;

	if(StorIF_SDCard_GetInfo(pvStorRes, &sStorInfo) != eSTORIF_ERRNO_NONE)
		return 0;

	STORIF_MUTEX_LOCK();

	i32Ret = SDCard_Write(pu8Buff, u32Sector, u32Count);

	STORIF_MUTEX_UNLOCK();

	return i32Ret;
}

const S_STORIF_IF g_STORIF_sSDCard = {
	.pfnStorInit = StorIF_SDCard_Init,
	.pfnReadSector = StorIF_SDCard_ReadSector,
	.pfnWriteSector = StorIF_SDCard_WriteSector,
	.pfnDetect = StorIF_SDCard_Detect,
	.pfnGetInfo = StorIF_SDCard_GetInfo,
};

