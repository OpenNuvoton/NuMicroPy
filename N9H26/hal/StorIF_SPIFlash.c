/***************************************************************************//**
 * @file     StorIF_SPIFlash.c
 * @brief    SPI flash storage access function
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <string.h>

#include "wblib.h"
#include "N9H26_SPI.h"

#include "StorIF.h"

#if MICROPY_PY_THREAD
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

static xSemaphoreHandle s_tStorIfMutex;

#endif


typedef struct{
	int32_t i32ID;
	uint32_t u32BlockSize;	//unit: Byte
	uint32_t u32SectorSize;	//unit: Byte
	uint32_t u32TotalBlockN;
	uint32_t u32PageSize;	//unit: Byte
}S_SPIFLASH_INFO;

#define SPIFLASH_CS_PIN 	0
#define READ_ID_CMD		0x9F					// Change it if the SPI read ID commmand is not 0x9F
#define SPIFLASH_DATA_START_ADDR (1024*1024)	//1MB, Must align to flash's sector size 

static uint32_t s_u32SPIPort = 0;
static S_SPIFLASH_INFO s_sSPIFlashInfo;

static E_STORIF_ERRNO
MapSPIFlash(
	int32_t i32FlashID,
	S_SPIFLASH_INFO *psInfo
)
{
	switch(i32FlashID){
		case 0xEF4015:			//W25Q16CV (2MB)
			psInfo->i32ID = i32FlashID;
			psInfo->u32BlockSize = 	64 * 1024;
			psInfo->u32SectorSize = 4 * 1024;
			psInfo->u32TotalBlockN = 32;
			psInfo->u32PageSize = 256;
		break;
		case 0xEF4016:			//W25Q32FV (4MB)
			psInfo->i32ID = i32FlashID;
			psInfo->u32BlockSize = 	64 * 1024;
			psInfo->u32SectorSize = 4 * 1024;
			psInfo->u32TotalBlockN = 64;
			psInfo->u32PageSize = 256;
		break;		
		case 0xEF4019:			//W25Q256FV (32MB)
			psInfo->i32ID = i32FlashID;
			psInfo->u32BlockSize = 	64 * 1024;
			psInfo->u32SectorSize = 4 * 1024;
			psInfo->u32TotalBlockN = 512;
			psInfo->u32PageSize = 256;
		break;		
		case 0xEF4018:			//W25Q128 (16MB)
			psInfo->i32ID = i32FlashID;
			psInfo->u32BlockSize = 	64 * 1024;
			psInfo->u32SectorSize = 4 * 1024;
			psInfo->u32TotalBlockN = 256;
			psInfo->u32PageSize = 256;
		break;		
		default:{
			printf("Unknown SPI flash. JEDEC ID:%x \n", (unsigned int)i32FlashID);
			//psInfo->i32ID = i32FlashID;
			//psInfo->u32BlockSize = 	64 * 1024;
			//psInfo->u32SectorSize = 4 * 1024;
			//psInfo->u32TotalBlockN = 32;			
			return eSTORIF_ERRNO_DEVICE;
		}
		
	}
	
	printf("SPI Flash JEDEC ID:%x \n", (unsigned int)i32FlashID);

	return eSTORIF_ERRNO_NONE;
}

static uint32_t 
SPIFlash_Read_JEDEC_ID(void)
{
	uint32_t volatile id;
	
	if(s_u32SPIPort == 0){
		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

		// command 8 bit
		outpw(REG_SPI0_TX0, READ_ID_CMD);
		spiTxLen(s_u32SPIPort, 0, 8);
		spiActive(s_u32SPIPort);

		// data 24 bit
		outpw(REG_SPI0_TX0, 0xffff);
		spiTxLen(s_u32SPIPort, 0, 24);
		spiActive(s_u32SPIPort);
		id = inp32(REG_SPI0_RX0) & 0xFFFFFF;

		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0
	}
	else{
		outpw(REG_SPI1_SSR, inpw(REG_SPI1_SSR) | 0x01);	// CS0

		// command 8 bit
		outpw(REG_SPI0_TX1, READ_ID_CMD);
		spiTxLen(s_u32SPIPort, 0, 8);
		spiActive(s_u32SPIPort);

		// data 24 bit
		outpw(REG_SPI0_TX1, 0xffff);
		spiTxLen(s_u32SPIPort, 0, 24);
		spiActive(s_u32SPIPort);
		id = inp32(REG_SPI0_RX1) & 0xFFFFFF;

		outpw(REG_SPI0_SSR, inpw(REG_SPI1_SSR) & 0xfe);	// CS0

	}
	
	return id;
}

static E_STORIF_ERRNO
StorIF_SPIFlash_Init(
	int32_t i32Inst,
	void **ppStorRes
)
{
	int i32Ret;
	uint32_t u32SPIFlashID;

	if(i32Inst > 1)
		return eSTORIF_ERRNO_IO;

	s_u32SPIPort = i32Inst;
	
#if MICROPY_PY_THREAD
	s_tStorIfMutex = xSemaphoreCreateMutex();

	if(s_tStorIfMutex == NULL){
		printf("Unable create SPI flash mutex\n");
		return eSTORIF_ERRNO_NULL_PTR;
	}
	
	xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

	spiRtosInit(s_u32SPIPort);

	i32Ret = spiFlashInit(s_u32SPIPort, SPIFLASH_CS_PIN);

	if(i32Ret != 0){
		return eSTORIF_ERRNO_STOR_OPEN;
	}

	/* Read SPI Flash ID */
	u32SPIFlashID = SPIFlash_Read_JEDEC_ID();

#if MICROPY_PY_THREAD
	xSemaphoreGive(s_tStorIfMutex);
#endif

	/* Get SPI Flash information */
	return MapSPIFlash(u32SPIFlashID, &s_sSPIFlashInfo);
}

static int32_t
StorIF_SPIFlash_ReadSector(
	uint8_t *pu8Buff,		/* Data buffer to store read data */
	uint32_t u32Sector,		/* Sector address (LBA) */
	uint32_t u32Count,		/* Number of sectors to read (1..128) */
	void *pvStorRes
)
{
	int i;
	uint32_t u32FlashAddr;

#if MICROPY_PY_THREAD
	xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

	u32FlashAddr = SPIFLASH_DATA_START_ADDR + (u32Sector * s_sSPIFlashInfo.u32SectorSize);
//	printf("USB StorIF_SPIFlash_ReadSector u32Sector %d, %d \n", u32Sector, u32Count);

	for(i = 0; i < u32Count; i ++){
		spiFlashRead(s_u32SPIPort, SPIFLASH_CS_PIN, u32FlashAddr, s_sSPIFlashInfo.u32SectorSize, pu8Buff); 
		
		u32FlashAddr += s_sSPIFlashInfo.u32SectorSize;
		pu8Buff += s_sSPIFlashInfo.u32SectorSize;
	}

#if MICROPY_PY_THREAD
	xSemaphoreGive(s_tStorIfMutex);
#endif

	return u32Count;
}

static int32_t
StorIF_SPIFlash_WriteSector(
	uint8_t *pu8Buff,		/* Data buffer to store read data */
	uint32_t u32Sector,		/* Sector address (LBA) */
	uint32_t u32Count,		/* Number of sectors to read (1..128) */
	void *pvStorRes
)
{
	int i, j;
	uint32_t u32FlashAddr;
	uint32_t u32PageProgCnt;
//	uint8_t *pu8SrcBuff = pu8Buff;

#if MICROPY_PY_THREAD
	xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

	u32FlashAddr = SPIFLASH_DATA_START_ADDR + (u32Sector * s_sSPIFlashInfo.u32SectorSize);

	u32PageProgCnt = s_sSPIFlashInfo.u32SectorSize / s_sSPIFlashInfo.u32PageSize;

	for(i = 0; i < u32Count; i ++){
		spiFlashEraseSector(s_u32SPIPort, SPIFLASH_CS_PIN, u32FlashAddr, 1); 
		
		for(j = 0; j <  u32PageProgCnt; j ++){
			spiFlashWrite(s_u32SPIPort, SPIFLASH_CS_PIN, u32FlashAddr, s_sSPIFlashInfo.u32PageSize, pu8Buff);

			u32FlashAddr += s_sSPIFlashInfo.u32PageSize;
			pu8Buff += s_sSPIFlashInfo.u32PageSize;
		}
	}

#if MICROPY_PY_THREAD
	xSemaphoreGive(s_tStorIfMutex);
#endif

//Verify code
//	static uint8_t s_pu8TempSPIbuf[4096];

//	StorIF_SPIFlash_ReadSector(s_pu8TempSPIbuf, u32Sector, 1, NULL);
//	for(i = 0; i <10; i ++){
//		printf("[%x][%x]", pu8SrcBuff[i], s_pu8TempSPIbuf[i]);
//	}

//	printf("StorIF_SPIFlash_WriteSector \n");
	return u32Count;
}

static int32_t
StorIF_SPIFlash_Detect(
	void *pvStorRes
)
{
	return TRUE;
}

static E_STORIF_ERRNO
StorIF_SPIFlash_GetInfo(
	S_STORIF_INFO *psInfo,
	void *pvStorRes
)
{
	psInfo->u32TotalSector = s_sSPIFlashInfo.u32TotalBlockN * (s_sSPIFlashInfo.u32BlockSize / s_sSPIFlashInfo.u32SectorSize); 
	psInfo->u32DiskSize = s_sSPIFlashInfo.u32TotalBlockN * (s_sSPIFlashInfo.u32BlockSize / 1024);
	psInfo->u32SectorSize = s_sSPIFlashInfo.u32SectorSize;
	psInfo->u32SubType = 0;

	return eSTORIF_ERRNO_NONE;
}


const S_STORIF_IF g_STORIF_sSPIFlash = {
	.pfnStorInit = StorIF_SPIFlash_Init,
	.pfnReadSector = StorIF_SPIFlash_ReadSector,
	.pfnWriteSector = StorIF_SPIFlash_WriteSector,
	.pfnDetect = StorIF_SPIFlash_Detect,
	.pfnGetInfo = StorIF_SPIFlash_GetInfo,
};
