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

//#include "SDCard.h"
#include "StorIF.h"

#if MICROPY_PY_THREAD
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

static xSemaphoreHandle s_tStorIfMutex;


#endif

typedef struct sdcard_info_t
{
    unsigned int    CardType;       /*!< SDHC, SD, or MMC */
    unsigned int    RCA;            /*!< Relative card address */
    unsigned char   IsCardInsert;   /*!< Card insert state */
    unsigned int    totalSectorN;   /*!< Total sector number */
    unsigned int    diskSize;       /*!< Disk size in K bytes */
    int             sectorSize;     /*!< Sector size in bytes */
} S_SDCARD_INFO;                       /*!< Structure holds SD card info */



#define Sector_Size 512   //512 bytes
static uint32_t Tmp_Buffer[Sector_Size];

static SDH_T *s_pSDH = SDH0;
static SDH_INFO_T *s_psSDInfo = &SD0;

void SDH0_IRQHandler(void)
{
    unsigned int volatile isr;
//    unsigned int volatile ier;

	IRQ_ENTER(SDH0_IRQn);
    // FMI data abort interrupt
    if (s_pSDH->GINTSTS & SDH_GINTSTS_DTAIF_Msk)
    {
        /* ResetAllEngine() */
        s_pSDH->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }

    //----- SD interrupt status
    isr = s_pSDH->INTSTS;
    if (isr & SDH_INTSTS_BLKDIF_Msk)
    {
        // block down
        g_u8SDDataReadyFlag = TRUE;
        s_pSDH->INTSTS = SDH_INTSTS_BLKDIF_Msk;
    }

    if (isr & SDH_INTSTS_CDIF_Msk)   // card detect
    {
        //----- SD interrupt status
        // it is work to delay 50 times for SD_CLK = 200KHz
        {
            int volatile i;         // delay 30 fail, 50 OK
            for (i=0; i<0x500; i++);  // delay to make sure got updated value from REG_SDISR.
            isr = s_pSDH->INTSTS;
        }

        if (isr & SDH_INTSTS_CDSTS_Msk)
        {
            printf("\n***** card remove !\n");
            s_psSDInfo->IsCardInsert = FALSE;   // SDISR_CD_Card = 1 means card remove for GPIO mode
            memset(s_psSDInfo, 0, sizeof(SDH_INFO_T));
        }
        else
        {
            printf("***** card insert !\n");
            SDH_Open(s_pSDH, CardDetect_From_GPIO);
            SDH_Probe(s_pSDH);
        }

        s_pSDH->INTSTS = SDH_INTSTS_CDIF_Msk;
    }

    // CRC error interrupt
    if (isr & SDH_INTSTS_CRCIF_Msk)
    {
        if (!(isr & SDH_INTSTS_CRC16_Msk))
        {
            printf("***** ISR sdioIntHandler(): CRC_16 error !\n");
            // handle CRC error
        }
        else if (!(isr & SDH_INTSTS_CRC7_Msk))
        {
            if (!g_u8R3Flag)
            {
                printf("***** ISR sdioIntHandler(): CRC_7 error !\n");
                // handle CRC error
            }
        }
        s_pSDH->INTSTS = SDH_INTSTS_CRCIF_Msk;      // clear interrupt flag
    }

    if (isr & SDH_INTSTS_DITOIF_Msk)
    {
        printf("***** ISR: data in timeout !\n");
        s_pSDH->INTSTS |= SDH_INTSTS_DITOIF_Msk;
    }

    // Response in timeout interrupt
    if (isr & SDH_INTSTS_RTOIF_Msk)
    {
        printf("***** ISR: response in timeout !\n");
        s_pSDH->INTSTS |= SDH_INTSTS_RTOIF_Msk;
    }

	IRQ_EXIT(SDH0_IRQn);
}

//TODO: SD1 interrupt handler



static int32_t SDCard_Init(void){

	if(s_pSDH == SDH0){
		//for SD0

		/* select multi-function pin */
		SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE7MFP_Msk     | SYS_GPE_MFPL_PE6MFP_Msk     | SYS_GPE_MFPL_PE3MFP_Msk      | SYS_GPE_MFPL_PE2MFP_Msk);
		SYS->GPE_MFPL |=  (SYS_GPE_MFPL_PE7MFP_SD0_CMD | SYS_GPE_MFPL_PE6MFP_SD0_CLK | SYS_GPE_MFPL_PE3MFP_SD0_DAT1 | SYS_GPE_MFPL_PE2MFP_SD0_DAT0);

		SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB5MFP_Msk      | SYS_GPB_MFPL_PB4MFP_Msk);
		SYS->GPB_MFPL |=  (SYS_GPB_MFPL_PB5MFP_SD0_DAT3 | SYS_GPB_MFPL_PB4MFP_SD0_DAT2);

		SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD13MFP_Msk);
		SYS->GPD_MFPH |=  (SYS_GPD_MFPH_PD13MFP_SD0_nCD);

		/* Select IP clock source */
		CLK_SetModuleClock(SDH0_MODULE, CLK_CLKSEL0_SDH0SEL_PLL, CLK_CLKDIV0_SDH0(10));

		/* Enable IP clock */
		CLK_EnableModuleClock(SDH0_MODULE);
	}
	else{
		//TODO for SD1

	}


    SystemCoreClockUpdate();

    SDH_Open(s_pSDH, CardDetect_From_GPIO);
    SDH_Probe(s_pSDH);


    return 0;
}

static int32_t SDCard_Read (
    uint8_t *buff,     /* Data buffer to store read data */
    uint32_t sector,   /* Sector address (LBA) */
    uint32_t count      /* Number of sectors to read (1..128) */
)
{
    int32_t ret;
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
            ret = SDH_Read(s_pSDH, (uint8_t*)(&Tmp_Buffer), sector, count);
            memcpy(buff, (&Tmp_Buffer), count*s_psSDInfo->sectorSize);
        }
        else
        {
            tmp_StartBufAddr = (((uint32_t)buff/4 + 1) * 4);
            ret = SDH_Read(s_pSDH, ((uint8_t*)tmp_StartBufAddr), sector, (count -1));
            memcpy(buff, (void*)tmp_StartBufAddr, (s_psSDInfo->sectorSize*(count-1)) );
            ret = SDH_Read(s_pSDH, (uint8_t*)(&Tmp_Buffer), (sector+count-1), 1);
            memcpy( (buff+(s_psSDInfo->sectorSize*(count-1))), (void*)Tmp_Buffer, s_psSDInfo->sectorSize);
        }
    }
    else
    {
        ret = SDH_Read(s_pSDH, buff, sector, count);
    }

    return ret;
}


static int32_t SDCard_Write (
    const uint8_t *buff,   /* Data to be written */
    uint32_t sector,       /* Sector address (LBA) */
    uint32_t count          /* Number of sectors to write (1..128) */
)
{
    int32_t  ret;
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
            memcpy((&Tmp_Buffer), buff, count*s_psSDInfo->sectorSize);
            ret = SDH_Write(s_pSDH, (uint8_t*)(&Tmp_Buffer), sector, count);
        }
        else
        {
            tmp_StartBufAddr = (((uint32_t)buff/4 + 1) * 4);
            memcpy((void*)Tmp_Buffer, (buff+(s_psSDInfo->sectorSize*(count-1))), s_psSDInfo->sectorSize);

            for(i = (s_psSDInfo->sectorSize*(count-1)); i > 0; i--)
            {
                memcpy((void *)(tmp_StartBufAddr + i - 1), (buff + i -1), 1);
            }

            ret = SDH_Write(s_pSDH, ((uint8_t*)tmp_StartBufAddr), sector, (count -1));
            ret = SDH_Write(s_pSDH, (uint8_t*)(&Tmp_Buffer), (sector+count-1), 1);
        }
    }
    else
    {
        ret = SDH_Write(s_pSDH, (uint8_t *)buff, sector, count);
    }

    return ret;
}

static E_STORIF_ERRNO
StorIF_SDCard_Init(
	int32_t i32Inst,
	void **ppStorRes
)
{
	if(i32Inst == 0){
		s_pSDH = SDH0;
		s_psSDInfo = &SD0;
	}
	else if(i32Inst == 1){
		s_pSDH = SDH1;
		s_psSDInfo = &SD1;
	}
	else{
		return eSTORIF_ERRNO_IO;
	}

#if MICROPY_PY_THREAD
	s_tStorIfMutex = xSemaphoreCreateMutex();

	if(s_tStorIfMutex == NULL){
		printf("Unable create SD card mutex\n");
		return eSTORIF_ERRNO_NULL_PTR;
	}
	
	xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

	SDCard_Init();
	
#if MICROPY_PY_THREAD
	xSemaphoreGive(s_tStorIfMutex);
#endif

	return eSTORIF_ERRNO_NONE;
}


static int32_t
StorIF_SDCard_ReadSector(
	uint8_t *pu8Buff,		/* Data buffer to store read data */
	uint32_t u32Sector,		/* Sector address (LBA) */
	uint32_t u32Count,		/* Number of sectors to read (1..128) */
	void *pvStorRes
)
{
	int32_t i32Ret;

#if MICROPY_PY_THREAD	
	xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

	i32Ret = SDCard_Read(pu8Buff, u32Sector, u32Count);

#if MICROPY_PY_THREAD	
	xSemaphoreGive(s_tStorIfMutex);
#endif

	return i32Ret;
}

static int32_t
StorIF_SDCard_WriteSector(
	uint8_t *pu8Buff,		/* Data buffer to store read data */
	uint32_t u32Sector,		/* Sector address (LBA) */
	uint32_t u32Count,		/* Number of sectors to read (1..128) */
	void *pvStorRes
)
{
	int32_t i32Ret;

#if MICROPY_PY_THREAD	
	xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

	i32Ret = SDCard_Write(pu8Buff, u32Sector, u32Count);

#if MICROPY_PY_THREAD	
	xSemaphoreGive(s_tStorIfMutex);
#endif

	return i32Ret;
}


static int32_t
StorIF_SDCard_Detect(
	void *pvStorRes
)
{
	return SDH_CardDetection(s_pSDH);
}

static E_STORIF_ERRNO
StorIF_SDCard_GetInfo(
	S_STORIF_INFO *psInfo,
	void *pvStorRes
)
{
//	printf("StorIF_SDCard_GetInfo, s_psSDInfo->totalSectorN %d \n", s_psSDInfo->totalSectorN);
//	printf("StorIF_SDCard_GetInfo, s_psSDInfo->diskSize %d \n", s_psSDInfo->diskSize);
//	printf("StorIF_SDCard_GetInfo, s_psSDInfo->sectorSize %d \n", s_psSDInfo->sectorSize);


	psInfo->u32TotalSector = s_psSDInfo->totalSectorN; 
	psInfo->u32DiskSize = s_psSDInfo->diskSize;
	psInfo->u32SectorSize = s_psSDInfo->sectorSize;
	psInfo->u32SubType = s_psSDInfo->CardType;

	return eSTORIF_ERRNO_NONE;
}


const S_STORIF_IF g_STORIF_sSDCard = {
	.pfnStorInit = StorIF_SDCard_Init,
	.pfnReadSector = StorIF_SDCard_ReadSector,
	.pfnWriteSector = StorIF_SDCard_WriteSector,
	.pfnDetect = StorIF_SDCard_Detect,
	.pfnGetInfo = StorIF_SDCard_GetInfo,
};




