/***************************************************************************//**
 * @file     StorIF_SPIFlash.c
 * @brief    SPI flash storage access function
 * @version  0.0.1
 *
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include <string.h>
#include "NuMicro.h"
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

static QSPI_T *s_psQSPI = QSPI0;
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
	uint8_t u8RxData[4], u8IDCnt = 0;

	// /CS: active
	QSPI_SET_SS_LOW(s_psQSPI);

	// send Command: 0x90, Read JEDEC ID
	QSPI_WRITE_TX(s_psQSPI, 0x9F);

	// send 24-bit '0'
	QSPI_WRITE_TX(s_psQSPI, 0x00);
	QSPI_WRITE_TX(s_psQSPI, 0x00);
	QSPI_WRITE_TX(s_psQSPI, 0x00);

	// wait tx finish
	while(QSPI_IS_BUSY(s_psQSPI));

	// /CS: de-active
	QSPI_SET_SS_HIGH(s_psQSPI);

    while((!QSPI_GET_RX_FIFO_EMPTY_FLAG(s_psQSPI)) && (u8IDCnt < 4))
        u8RxData[u8IDCnt ++] = QSPI_READ_RX(s_psQSPI);

    return ((u8RxData[1]<<16) | (u8RxData[2]<<8) | u8RxData[3] );
}

void SPIFlash_SectorRead(uint32_t u32Address, uint8_t *pu8DataBuffer)
{
	uint32_t i;

	// /CS: active
	QSPI_SET_SS_LOW(s_psQSPI);

	// send Command: 0x0b, fast Read data
	QSPI_WRITE_TX(s_psQSPI, 0x0B);
	while(QSPI_IS_BUSY(s_psQSPI));
 
	// send 24-bit start address
	QSPI_WRITE_TX(s_psQSPI, (u32Address>>16) & 0xFF);
	QSPI_WRITE_TX(s_psQSPI, (u32Address>>8)  & 0xFF);
	QSPI_WRITE_TX(s_psQSPI, u32Address       & 0xFF);

    // dummy byte
    QSPI_WRITE_TX(s_psQSPI, 0x00);

	while(QSPI_IS_BUSY(s_psQSPI));
	// clear RX buffer
	QSPI_ClearRxFIFO(s_psQSPI);

	// read data
	for(i = 0; i < s_sSPIFlashInfo.u32SectorSize; i++)
	{
		QSPI_WRITE_TX(s_psQSPI, 0x00);
		while(QSPI_IS_BUSY(s_psQSPI));
		pu8DataBuffer[i] = QSPI_READ_RX(s_psQSPI);
	}

    // wait tx finish
    while(QSPI_IS_BUSY(s_psQSPI));

    // /CS: de-active
    QSPI_SET_SS_HIGH(s_psQSPI);
}

void SPIFlash_PageProgram(uint32_t u32Address, uint8_t *pu8DataBuffer)
{
	uint32_t i = 0;

	// /CS: active
	QSPI_SET_SS_LOW(s_psQSPI);

	// send Command: 0x06, Write enable
	QSPI_WRITE_TX(s_psQSPI, 0x06);

	// wait tx finish
	while(QSPI_IS_BUSY(s_psQSPI));

	// /CS: de-active
	QSPI_SET_SS_HIGH(s_psQSPI);

	// /CS: active
	QSPI_SET_SS_LOW(s_psQSPI);

	// send Command: 0x02, Page program
	QSPI_WRITE_TX(s_psQSPI, 0x02);

	// send 24-bit start address
	QSPI_WRITE_TX(s_psQSPI, (u32Address>>16) & 0xFF);
	QSPI_WRITE_TX(s_psQSPI, (u32Address>>8)  & 0xFF);
	QSPI_WRITE_TX(s_psQSPI, u32Address       & 0xFF);

	// write data
	while(1)
	{
		if(!QSPI_GET_TX_FIFO_FULL_FLAG(s_psQSPI))
		{
			QSPI_WRITE_TX(s_psQSPI, pu8DataBuffer[i]);
			i++;
			if(i >= s_sSPIFlashInfo.u32PageSize) break;
		}
	}

	// wait tx finish
	while(QSPI_IS_BUSY(s_psQSPI));

	// /CS: de-active
	QSPI_SET_SS_HIGH(s_psQSPI);

	QSPI_ClearRxFIFO(s_psQSPI);
}

void SPIFlash_SectorErase(uint32_t u32Address)
{
	// /CS: active
	QSPI_SET_SS_LOW(s_psQSPI);

	// send Command: 0x06, Write enable
	QSPI_WRITE_TX(s_psQSPI, 0x06);

	// wait tx finish
	while(QSPI_IS_BUSY(s_psQSPI));

	// /CS: de-active
	QSPI_SET_SS_HIGH(s_psQSPI);


	// /CS: active
	QSPI_SET_SS_LOW(s_psQSPI);

	// send Command: 0x20, sector erase
	QSPI_WRITE_TX(s_psQSPI, 0x20);

	// send 24-bit start address
	QSPI_WRITE_TX(s_psQSPI, (u32Address>>16) & 0xFF);
	QSPI_WRITE_TX(s_psQSPI, (u32Address>>8)  & 0xFF);
	QSPI_WRITE_TX(s_psQSPI, u32Address       & 0xFF);

    // wait tx finish
    while(QSPI_IS_BUSY(s_psQSPI));

    // /CS: de-active
    QSPI_SET_SS_HIGH(s_psQSPI);

    QSPI_ClearRxFIFO(s_psQSPI);
}

uint8_t SPIFlash_ReadStatusReg(void)
{
    uint8_t u8Val;

    QSPI_ClearRxFIFO(s_psQSPI);

    // /CS: active
    QSPI_SET_SS_LOW(s_psQSPI);

    // send Command: 0x05, Read status register
    QSPI_WRITE_TX(s_psQSPI, 0x05);

    // read status
    QSPI_WRITE_TX(s_psQSPI, 0x00);

    // wait tx finish
    while(QSPI_IS_BUSY(s_psQSPI));

    // /CS: de-active
    QSPI_SET_SS_HIGH(s_psQSPI);

    // skip first rx data
    u8Val = QSPI_READ_RX(s_psQSPI);
    u8Val = QSPI_READ_RX(s_psQSPI);

    return u8Val;
}


void SPIFlash_WaitReady(void)
{
    volatile uint8_t ReturnValue;

    do
    {
        ReturnValue = SPIFlash_ReadStatusReg();
        ReturnValue = ReturnValue & 1;
    }
    while(ReturnValue!=0);   // check the BUSY bit
}

void SpiFlash_ChipErase(void)
{
    // /CS: active
    QSPI_SET_SS_LOW(s_psQSPI);

    // send Command: 0x06, Write enable
    QSPI_WRITE_TX(s_psQSPI, 0x06);

    // wait tx finish
    while(QSPI_IS_BUSY(s_psQSPI));

    // /CS: de-active
    QSPI_SET_SS_HIGH(s_psQSPI);

    //////////////////////////////////////////

    // /CS: active
    QSPI_SET_SS_LOW(s_psQSPI);

    // send Command: 0xC7, Chip Erase
    QSPI_WRITE_TX(s_psQSPI, 0xC7);

    // wait tx finish
    while(QSPI_IS_BUSY(s_psQSPI));

    // /CS: de-active
    QSPI_SET_SS_HIGH(s_psQSPI);

    QSPI_ClearRxFIFO(s_psQSPI);
}

static E_STORIF_ERRNO
StorIF_SPIFlash_Init(
	int32_t i32Inst,
	void **ppStorRes
)
{
	uint32_t u32SPIFlashID;
	
	if(i32Inst == 0){
		s_psQSPI = QSPI0;
	}
	else{
		return eSTORIF_ERRNO_IO;
	}

#if MICROPY_PY_THREAD
	s_tStorIfMutex = xSemaphoreCreateMutex();

	if(s_tStorIfMutex == NULL){
		printf("Unable create SPI flash mutex\n");
		return eSTORIF_ERRNO_NULL_PTR;
	}
	
	xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

    /* Configure QSPI_FLASH_PORT as a master, MSB first, 8-bit transaction, QSPI Mode-0 timing, clock is 20MHz */
    QSPI_Open(s_psQSPI, QSPI_MASTER, QSPI_MODE_0, 8, 20000000);

    /* Disable auto SS function, control SS signal manually. */
    QSPI_DisableAutoSS(s_psQSPI);

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

	u32FlashAddr = u32Sector * s_sSPIFlashInfo.u32SectorSize;
//	printf("USB StorIF_SPIFlash_ReadSector u32Sector %d, %d \n", u32Sector, u32Count);

	for(i = 0; i < u32Count; i ++){
		SPIFlash_SectorRead(u32FlashAddr, pu8Buff); 
		
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

	u32FlashAddr = u32Sector * s_sSPIFlashInfo.u32SectorSize;

	u32PageProgCnt = s_sSPIFlashInfo.u32SectorSize / s_sSPIFlashInfo.u32PageSize;

	for(i = 0; i < u32Count; i ++){
		SPIFlash_SectorErase(u32FlashAddr); 
		SPIFlash_WaitReady();
		
		for(j = 0; j <  u32PageProgCnt; j ++){
			SPIFlash_PageProgram(u32FlashAddr, pu8Buff);
			SPIFlash_WaitReady();
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
