/***************************************************************************//**
 * @file     StorIF_SDCard.c
 * @brief    SD storage access function
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "py/obj.h"

#include "NuMicro.h"
#include "StorIF.h"

#if MICROPY_PY_THREAD
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

static xSemaphoreHandle s_tStorIfMutex;

#endif

typedef struct sdcard_info_t {
    SDH_T 			*pSDH;			/*!< hardware instance */
    SDH_INFO_T		*psSDHInfo;		/*!< hardware information */
} S_SDCARD_INFO;                    /*!< Structure holds SD card info */

#define SECTOR_SIZE 512   //512 bytes


S_SDCARD_INFO	s_sSD0_info;
S_SDCARD_INFO	s_sSD1_info;

void SDH_IntHandler(SDH_T *psSDH, SDH_INFO_T *psSDInfo)
{
    unsigned int volatile isr;
    unsigned int volatile ier;

    // FMI data abort interrupt
    if (psSDH->GINTSTS & SDH_GINTSTS_DTAIF_Msk) {
        /* ResetAllEngine() */
        psSDH->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }

    //----- SD interrupt status
    isr = psSDH->INTSTS;
    ier = psSDH->INTEN;

    if (isr & SDH_INTSTS_BLKDIF_Msk) {
        // block down
        psSDInfo->DataReadyFlag = TRUE;
        psSDH->INTSTS = SDH_INTSTS_BLKDIF_Msk;
        //printf("SD block down\r\n");
    }

    if ((ier & SDH_INTEN_CDIEN_Msk) &&
        (isr & SDH_INTSTS_CDIF_Msk)) {  // card detect
        //----- SD interrupt status
        // it is work to delay 50 times for SD_CLK = 200KHz
        {
            int volatile i;         // delay 30 fail, 50 OK

            for (i = 0; i < 0x500; i++); // delay to make sure got updated value from REG_SDISR.

            isr = psSDH->INTSTS;
        }

#if (DEF_CARD_DETECT_SOURCE == CardDetect_From_DAT3)

        if (!(isr & SDH_INTSTS_CDSTS_Msk))
#else
        if (isr & SDH_INTSTS_CDSTS_Msk)
#endif
        {
            printf("\n***** card remove !\n");
            psSDInfo->IsCardInsert = FALSE;   // SDISR_CD_Card = 1 means card remove for GPIO mode
            //memset(&psSDInfo, 0, sizeof(SDH_INFO_T));
        } else {
            printf("***** card insert !\n");
            SDH_Open(psSDH, CardDetect_From_GPIO);
            SDH_Probe(psSDH);
        }

        psSDH->INTSTS = SDH_INTSTS_CDIF_Msk;
    }

    // CRC error interrupt
    if (isr & SDH_INTSTS_CRCIF_Msk) {
        if (!(isr & SDH_INTSTS_CRC16_Msk)) {
            //printf("***** ISR sdioIntHandler(): CRC_16 error !\n");
            // handle CRC error
        } else if (!(isr & SDH_INTSTS_CRC7_Msk)) {
            if (!psSDInfo->R3Flag) {
                //printf("***** ISR sdioIntHandler(): CRC_7 error !\n");
                // handle CRC error
            }
        }

        psSDH->INTSTS = SDH_INTSTS_CRCIF_Msk;      // clear interrupt flag
    }

    if (isr & SDH_INTSTS_DITOIF_Msk) {
        printf("***** ISR: data in timeout !\n");
        psSDH->INTSTS |= SDH_INTSTS_DITOIF_Msk;
    }

    // Response in timeout interrupt
    if (isr & SDH_INTSTS_RTOIF_Msk) {
        printf("***** ISR: response in timeout !\n");
        psSDH->INTSTS |= SDH_INTSTS_RTOIF_Msk;
    }

    __DSB();
    __ISB();
}


static int32_t SDCard_Init(SDH_T *psSDH)
{
#if defined (MICROPY_HW_SD_nCD)
    MICROPY_HW_SD_nCD();
#endif
#if defined (MICROPY_HW_SD_CLK)
    MICROPY_HW_SD_CLK();
#endif
#if defined (MICROPY_HW_SD_CMD)
    MICROPY_HW_SD_CMD();
#endif
#if defined (MICROPY_HW_SD_DAT0)
    MICROPY_HW_SD_DAT0();
#endif
#if defined (MICROPY_HW_SD_DAT1)
    MICROPY_HW_SD_DAT1();
#endif
#if defined (MICROPY_HW_SD_DAT2)
    MICROPY_HW_SD_DAT2();
#endif
#if defined (MICROPY_HW_SD_DAT3)
    MICROPY_HW_SD_DAT3();
#endif

    if(psSDH == SDH0) {
        //for SD0

        /* Select IP clock source */
        CLK_SetModuleClock(SDH0_MODULE, CLK_SDHSEL_SDH0SEL_HCLK0, CLK_SDHDIV_SDH0DIV(4));

        /* Enable IP clock */
        CLK_EnableModuleClock(SDH0_MODULE);
    } else {
        /* Select IP clock source */
        CLK_SetModuleClock(SDH1_MODULE, CLK_SDHSEL_SDH1SEL_HCLK0, CLK_SDHDIV_SDH1DIV(4));

        /* Enable IP clock */
        CLK_EnableModuleClock(SDH1_MODULE);
    }

    SDH_Open(psSDH, CardDetect_From_GPIO);
    SDH_Probe(psSDH);

    return 0;
}

static int32_t SDCard_Read (
    SDH_T *psSDH,		/*SD hardware instance */
    uint8_t *buff,     /* Data buffer to store read data */
    uint32_t sector,   /* Sector address (LBA) */
    uint32_t count      /* Number of sectors to read (1..128) */
)
{
    int ret = 0;

    if ((count > 0) && psSDH && (buff != NULL)) {
        uint32_t drv_ret = SDH_Read(psSDH, (uint8_t *)buff, sector, count);

        ret = (drv_ret == Successful) ? count : -1;
    }

    return ret;
}

static int32_t SDCard_Write (
    SDH_T *psSDH,		/*SD hardware instance */
    uint8_t *buff,     /* Data buffer to store read data */
    uint32_t sector,   /* Sector address (LBA) */
    uint32_t count      /* Number of sectors to read (1..128) */
)
{
    int ret = 0;

    if ((count > 0) && psSDH && (buff != NULL)) {
        uint32_t drv_ret = SDH_Write(psSDH, (uint8_t *)buff, sector, count);

        ret = (drv_ret == Successful) ? count : -1;
    }

    return ret;
}

static E_STORIF_ERRNO
StorIF_SDCard_Init(
    int32_t i32Inst,
    void **ppStorRes
)
{
    S_SDCARD_INFO	*psSD_info;

    if(i32Inst == 0) {
        psSD_info = &s_sSD0_info;
        psSD_info->pSDH = SDH0;
        psSD_info->psSDHInfo = &SD0;
    } else if(i32Inst == 1) {
        psSD_info = &s_sSD1_info;
        psSD_info->pSDH = SDH1;
        psSD_info->psSDHInfo = &SD1;
    } else {
        return eSTORIF_ERRNO_IO;
    }

#if MICROPY_PY_THREAD
    s_tStorIfMutex = xSemaphoreCreateMutex();

    if(s_tStorIfMutex == NULL) {
        printf("Unable create SD card mutex\n");
        return eSTORIF_ERRNO_NULL_PTR;
    }

    xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

    SDCard_Init(psSD_info->pSDH);

#if MICROPY_PY_THREAD
    xSemaphoreGive(s_tStorIfMutex);
#endif

    *ppStorRes = psSD_info;

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
    S_SDCARD_INFO *psSDInfo = (S_SDCARD_INFO *)pvStorRes;

#if MICROPY_PY_THREAD
    xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

    i32Ret = SDCard_Read(psSDInfo->pSDH, pu8Buff, u32Sector, u32Count);

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
    S_SDCARD_INFO *psSDInfo = (S_SDCARD_INFO *)pvStorRes;

#if MICROPY_PY_THREAD
    xSemaphoreTake(s_tStorIfMutex, portMAX_DELAY);
#endif

    i32Ret = SDCard_Write(psSDInfo->pSDH, pu8Buff, u32Sector, u32Count);

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
    S_SDCARD_INFO *psSDInfo = (S_SDCARD_INFO *)pvStorRes;

    return SDH_CardDetection(psSDInfo->pSDH);
}

static E_STORIF_ERRNO
StorIF_SDCard_GetInfo(
    S_STORIF_INFO *psInfo,
    void *pvStorRes
)
{
    S_SDCARD_INFO *psSDInfo = (S_SDCARD_INFO *)pvStorRes;

    SDH_INFO_T	*psSDHInfo = psSDInfo->psSDHInfo;

//	printf("StorIF_SDCard_GetInfo, psSDHInfo->totalSectorN %d \n", psSDHInfo->totalSectorN);
//	printf("StorIF_SDCard_GetInfo, psSDHInfo->diskSize %d \n", psSDHInfo->diskSize);
//	printf("StorIF_SDCard_GetInfo, psSDHInfo->sectorSize %d \n", psSDHInfo->sectorSize);

    psInfo->u32TotalSector = psSDHInfo->totalSectorN;
    psInfo->u32DiskSize = psSDHInfo->diskSize;
    psInfo->u32SectorSize = psSDHInfo->sectorSize;
    psInfo->u32SubType = psSDHInfo->CardType;

    return eSTORIF_ERRNO_NONE;
}


S_STORIF_IF g_STORIF_sSDCard = {
    .pfnStorInit = StorIF_SDCard_Init,
    .pfnReadSector = StorIF_SDCard_ReadSector,
    .pfnWriteSector = StorIF_SDCard_WriteSector,
    .pfnDetect = StorIF_SDCard_Detect,
    .pfnGetInfo = StorIF_SDCard_GetInfo,
    .pvStorPriv = NULL,
};

