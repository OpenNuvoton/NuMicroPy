/***************************************************************************//**
 * @file     StorIF_FMC.c
 * @brief    FMC(internal flash) storage access function
 * @version  0.0.1
 *
 * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include <string.h>
#include "NuMicro.h"
#include "StorIF.h"

#define DATA_FLASH_STORAGE_OFFSET       0x00060000  /* To avoid the code to write APROM */
#define DATA_FLASH_SECTOR_SIZE			512  		/* DATA FLASH sector size */
#define DATA_FLASH_STORAGE_LBA_OFFSET   (DATA_FLASH_STORAGE_OFFSET / DATA_FLASH_SECTOR_SIZE)  /* DATA FLASH logic bass address */
#define DATA_FLASH_STORAGE_SIZE   		(128*1024)  /* Configure the DATA FLASH storage size. To pass USB-IF MSC Test, it needs > 64KB */
#define DATA_FLASH_PAGE_SIZE	  		(4096)  /* DATA FLASH page size */

static S_STORIF_INFO s_sFlashInfo = {
	.u32TotalSector = (DATA_FLASH_STORAGE_SIZE / DATA_FLASH_SECTOR_SIZE),
	.u32DiskSize = (DATA_FLASH_STORAGE_SIZE / 1024),
	.u32SectorSize = DATA_FLASH_SECTOR_SIZE,
	.u32SubType = 0,
};

static  uint32_t s_au32SectorBuf[DATA_FLASH_PAGE_SIZE / 4];

E_STORIF_ERRNO
StorIF_Flash_Init(
	int32_t i32Inst,
	void **ppStorRes
)
{
    uint32_t au32Config[2];

    SYS_UnlockReg();
    /* Enable FMC ISP function */
    FMC_Open();

    /* Check if Data Flash Size is 64K. If not, to re-define Data Flash size and to enable Data Flash function */
    if (FMC_ReadConfig(au32Config, 2) < 0){
		SYS_LockReg();
        return eSTORIF_ERRNO_STOR_OPEN;
    }

    if (((au32Config[0] & 0x01) == 1) || (au32Config[1] != DATA_FLASH_STORAGE_OFFSET) )
    {
        FMC_ENABLE_CFG_UPDATE();
        au32Config[0] &= ~0x1;
        au32Config[1] = DATA_FLASH_STORAGE_OFFSET;
        if (FMC_WriteConfig(au32Config, 2) < 0)
            return eSTORIF_ERRNO_IO;

        FMC_ReadConfig(au32Config, 2);
        if (((au32Config[0] & 0x01) == 1) || (au32Config[1] != DATA_FLASH_STORAGE_OFFSET))
        {
            printf("Error: Program Config Failed!\n");
            /* Disable FMC ISP function */
            FMC_Close();
            SYS_LockReg();
            return -1;
        }

        printf("chip reset\n");
        /* Reset Chip to reload new CONFIG value */
        SYS->IPRST0 = SYS_IPRST0_CHIPRST_Msk;
    }
//        printf("flash open \n");
	return eSTORIF_ERRNO_NONE;
}

int32_t
StorIF_Flash_ReadSector(
	uint8_t *pu8Buff,		/* Data buffer to store read data */
	uint32_t u32Sector,		/* Sector address (LBA) */
	uint32_t u32Count,		/* Number of sectors to read (1..128) */
	void *pvStorRes
)
{
	uint32_t u32FlashAddr;
    uint32_t *pu32Buf = (uint32_t *)pu8Buff;
    int32_t i32ReadCnt = u32Count;
	int i;

	
	u32FlashAddr =  (u32Sector + DATA_FLASH_STORAGE_LBA_OFFSET) * DATA_FLASH_SECTOR_SIZE;
 	
	while(i32ReadCnt > 0){
        for(i = 0; i < DATA_FLASH_SECTOR_SIZE / 4; i++)
            pu32Buf[i] = FMC_Read(u32FlashAddr + (i * 4));

		u32FlashAddr   += DATA_FLASH_SECTOR_SIZE;
		pu8Buff += DATA_FLASH_SECTOR_SIZE;
		pu32Buf = (uint32_t *)pu8Buff;
		i32ReadCnt --;
	}

	return u32Count;
}

void ProgramPage(uint32_t u32StartAddr, uint32_t *pu32Buf)
{
    uint32_t i;

    for(i = 0; i < DATA_FLASH_PAGE_SIZE / 4; i++)
    {
        FMC_Write(u32StartAddr + (i * 4), pu32Buf[i]);
    }
}

void ReadPage(uint32_t u32StartAddr, uint32_t *pu32Buf)
{
    uint32_t i;

    for(i = 0; i < DATA_FLASH_PAGE_SIZE / 4; i++)
        pu32Buf[i] = FMC_Read(u32StartAddr + (i * 4));
}



int32_t
StorIF_Flash_WriteSector(
	uint8_t *pu8Buff,		/* Data buffer to store read data */
	uint32_t u32Sector,		/* Sector address (LBA) */
	uint32_t u32Count,		/* Number of sectors to read (1..128) */
	void *pvStorRes
)
{
	uint32_t u32FlashAddr;
	int32_t i32WriteLen;
	int32_t i32EachWriteLen;
	
	uint32_t u32PageAlignAddr;
	uint32_t u32Offset;
    uint32_t *pu32TempAddr;
	int32_t i;


	u32FlashAddr =  (u32Sector + DATA_FLASH_STORAGE_LBA_OFFSET) * DATA_FLASH_SECTOR_SIZE;
	i32WriteLen = u32Count * DATA_FLASH_SECTOR_SIZE;
	i32EachWriteLen = i32WriteLen;

//	printf("%s:%d: address %d, %d , %d\n", __FUNCTION__, __LINE__, u32Sector, u32Count, u32FlashAddr);
    FMC_ENABLE_AP_UPDATE();
	
	if((i32WriteLen == DATA_FLASH_PAGE_SIZE) && (( u32FlashAddr & (DATA_FLASH_PAGE_SIZE - 1)) == 0) ){
        /* Page erase */
        FMC_Erase(u32FlashAddr);

        while(i32WriteLen >= DATA_FLASH_PAGE_SIZE)
        {
            ProgramPage(u32FlashAddr, (uint32_t *) pu8Buff);
            i32WriteLen -= DATA_FLASH_PAGE_SIZE;
            pu8Buff += DATA_FLASH_PAGE_SIZE;
            u32FlashAddr   += DATA_FLASH_PAGE_SIZE;
        }
	}
	else {

		do{
			u32PageAlignAddr = u32FlashAddr & 0xFFF000;
			
			/* Get the page offset*/
			u32Offset = (u32FlashAddr & (DATA_FLASH_PAGE_SIZE - 1));

			if(u32Offset || (i32WriteLen < DATA_FLASH_PAGE_SIZE))
			{
				/* Not 4096-byte alignment. Read the destination page for modification. Note: It needs to avoid adding MASS_STORAGE_OFFSET twice. */
				ReadPage(u32PageAlignAddr, &s_au32SectorBuf[0]);
			}

			pu32TempAddr = (uint32_t *)pu8Buff;
			i32EachWriteLen = DATA_FLASH_PAGE_SIZE - u32Offset;
			if(i32WriteLen < i32EachWriteLen)
				i32EachWriteLen = i32WriteLen;
				
            /* Update the destination buffer */
            for(i = 0; i < i32EachWriteLen / 4; i++)
            {
                s_au32SectorBuf[(u32Offset / 4) + i] = pu32TempAddr[i];
            }			

            /* Page erase */
            FMC_Erase(u32PageAlignAddr);
            /* Write to the destination page */
            ProgramPage(u32PageAlignAddr, (uint32_t *) s_au32SectorBuf);

			i32WriteLen -= i32EachWriteLen;
			u32FlashAddr += i32EachWriteLen;
			pu8Buff += i32EachWriteLen;

		}
		while(i32WriteLen > 0);

	}
	FMC_DISABLE_AP_UPDATE();
	return u32Count;
}


int32_t
StorIF_Flash_Detect(
	void *pvStorRes
)
{
	return TRUE;
}

E_STORIF_ERRNO
StorIF_Flash_GetInfo(
	S_STORIF_INFO *psInfo,
	void *pvStorRes
)
{
	memcpy(psInfo, &s_sFlashInfo, sizeof(S_STORIF_INFO));
	return eSTORIF_ERRNO_NONE;
}

const S_STORIF_IF g_STORIF_sFlash = {
	.pfnStorInit = StorIF_Flash_Init,
	.pfnReadSector = StorIF_Flash_ReadSector,
	.pfnWriteSector = StorIF_Flash_WriteSector,
	.pfnDetect = StorIF_Flash_Detect,
	.pfnGetInfo = StorIF_Flash_GetInfo,
};

