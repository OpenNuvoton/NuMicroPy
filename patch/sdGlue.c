/**************************************************************************//**
 * @file     sdGlue.c
 * @version  V3.00
 * @brief    N9H26 series SIC driver source file. Driver for FMI devices SD layer glue code.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#ifdef ECOS
    #include "stdlib.h"
    #include "string.h"
    #include "drv_api.h"
    #include "diag.h"
    #include "wbtypes.h"
    #include "wbio.h"
#else
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "wblib.h"
#endif

#include "N9H26_SIC.h"
#include "fmi.h"
#include "N9H26_NVTFAT.h"

DISK_DATA_T SD_DiskInfo0;
DISK_DATA_T SD_DiskInfo1;
DISK_DATA_T SD_DiskInfo2;

FMI_SD_INFO_T *pSD0 = NULL;
FMI_SD_INFO_T *pSD1 = NULL;
FMI_SD_INFO_T *pSD2 = NULL;

UINT8 pSD0_offset = 0;
UINT8 pSD1_offset = 0;
UINT8 pSD2_offset = 0;

PDISK_T *pDisk_SD0 = NULL;
PDISK_T *pDisk_SD1 = NULL;
PDISK_T *pDisk_SD2 = NULL;

INT32 g_SD0_card_detect = FALSE;    // default is DISABLE SD0 card detect feature.

extern INT  fsPhysicalDiskConnected(PDISK_T *pDisk);


static INT  sd_disk_init(PDISK_T  *lDisk)
{
    return 0;
}

static INT  sd_disk_ioctl(PDISK_T *lDisk, INT control, VOID *param)
{
    return 0;
}

static INT  sd_disk_read(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff)
{
    int status;

    fmiSD_CardSel(0);

    // enable SD
    outpw(REG_FMICR, FMI_SD_EN);
    status = fmiSD_Read_in(pSD0, sector_no, number_of_sector, (unsigned int)buff);

    if (status != Successful)
        return status;

    return FS_OK;
}

static INT  sd_disk_write(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff, BOOL IsWait)
{
    int status;

    fmiSD_CardSel(0);

    // enable SD
    outpw(REG_FMICR, FMI_SD_EN);
    status = fmiSD_Write_in(pSD0, sector_no, number_of_sector, (unsigned int)buff);

    if (status != Successful)
        return status;

    return FS_OK;
}

static INT  sd_disk_init0(PDISK_T  *lDisk)
{
    return 0;
}

static INT  sd_disk_ioctl0(PDISK_T *lDisk, INT control, VOID *param)
{
    return 0;
}

static INT  sd_disk_read0(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff)
{
    return sd_disk_read(pDisk, sector_no, number_of_sector, buff);
}

static INT  sd_disk_write0(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff, BOOL IsWait)
{
    return sd_disk_write(pDisk, sector_no, number_of_sector, buff, IsWait);
}

static INT  sd_disk_init1(PDISK_T  *lDisk)
{
    return 0;
}

static INT  sd_disk_ioctl1(PDISK_T *lDisk, INT control, VOID *param)
{
    return 0;
}

static INT  sd_disk_read1(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff)
{
    int status;

    fmiSD_CardSel(1);

    // enable SD
    outpw(REG_FMICR, FMI_SD_EN);
    status = fmiSD_Read_in(pSD1, sector_no, number_of_sector, (unsigned int)buff);

    if (status != Successful)
        return status;

    return FS_OK;
}

static INT  sd_disk_write1(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff, BOOL IsWait)
{
    int status;

    fmiSD_CardSel(1);

    // enable SD
    outpw(REG_FMICR, FMI_SD_EN);
    status = fmiSD_Write_in(pSD1, sector_no, number_of_sector, (unsigned int)buff);

    if (status != Successful)
        return status;

    return FS_OK;
}

static INT  sd_disk_init2(PDISK_T  *lDisk)
{
    return 0;
}

static INT  sd_disk_ioctl2(PDISK_T *lDisk, INT control, VOID *param)
{
    return 0;
}

static INT  sd_disk_read2(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff)
{
    int status;

    fmiSD_CardSel(2);

    // enable SD
    outpw(REG_FMICR, FMI_SD_EN);
    status = fmiSD_Read_in(pSD2, sector_no, number_of_sector, (unsigned int)buff);

    if (status != Successful)
        return status;

    return FS_OK;
}

static INT  sd_disk_write2(PDISK_T *pDisk, UINT32 sector_no, INT number_of_sector, UINT8 *buff, BOOL IsWait)
{
    int status;

    fmiSD_CardSel(2);

    // enable SD
    outpw(REG_FMICR, FMI_SD_EN);
    status = fmiSD_Write_in(pSD2, sector_no, number_of_sector, (unsigned int)buff);

    if (status != Successful)
        return status;

    return FS_OK;
}

STORAGE_DRIVER_T  _SDDiskDriver =
{
    sd_disk_init,
    sd_disk_read,
    sd_disk_write,
    sd_disk_ioctl
};

STORAGE_DRIVER_T  _SD0DiskDriver =
{
    sd_disk_init0,
    sd_disk_read0,
    sd_disk_write0,
    sd_disk_ioctl0
};

STORAGE_DRIVER_T  _SD1DiskDriver =
{
    sd_disk_init1,
    sd_disk_read1,
    sd_disk_write1,
    sd_disk_ioctl1
};

STORAGE_DRIVER_T  _SD2DiskDriver =
{
    sd_disk_init2,
    sd_disk_read2,
    sd_disk_write2,
    sd_disk_ioctl2
};

static int sd0_ok = 0;
static int sd1_ok = 0;
static int sd2_ok = 0;


/*-----------------------------------------------------------------------------
 * Switch SD port.
 * fmiSD_CardSel() config GPIO/SDCR register/SD clock/Driver strength for selected SD port.
 * Do nothing if the selected SD port is same to original.
 *---------------------------------------------------------------------------*/
INT  fmiSD_CardSel(INT cardSel)
{
    FMI_SD_INFO_T *pSD = NULL;

    if (cardSel==0)
    {
        if (inpw(REG_SDCR) & (SDCR_SDPORT) == SDCR_SDPORT_0)
            return 0;   // don't need to change value and wait for stable

        outpw(REG_GPEFUN0, (inpw(REG_GPEFUN0) & (~0xFFFFFF00)) | 0x22222200);   // SD0_CLK/CMD/DAT0_3 pins selected
        outpw(REG_SDCR, inpw(REG_SDCR) & (~SDCR_SDPORT));               // SD_0 port selected
        pSD = pSD0;
    }
    else if (cardSel==1)
    {
        if (inpw(REG_SDCR) & (SDCR_SDPORT) == SDCR_SDPORT_1)
            return 0;   // don't need to change value and wait for stable

        outpw(REG_GPBFUN0, (inpw(REG_GPBFUN0) & (~0x00FFFFFF)) | 0x00222222);   // SD2_CLK/CMD/DAT0_3 pins selected
        outpw(REG_SDCR, inpw(REG_SDCR) & (~SDCR_SDPORT) | SDCR_SDPORT_1);   // SD_1 port selected
        pSD = pSD1;
    }
    else if (cardSel==2)
    {
        if (inpw(REG_SDCR) & (SDCR_SDPORT) == SDCR_SDPORT_2)
            return 0;   // don't need to change value and wait for stable

        outpw(REG_GPDFUN0, (inpw(REG_GPDFUN0) & (~0xF0F00000)) | 0x10100000);   // SD2_DAT2/SD2_CLK pins selected
        outpw(REG_GPDFUN1, (inpw(REG_GPDFUN1) & (~0x0000000F)) | 0x00000001);   // SD2_CMD pins selected
        outpw(REG_GPEFUN1, (inpw(REG_GPEFUN1) & (~0x000FFF00)) | 0x00011100);   // SD2_DAT0_1_3 pins selected
        outpw(REG_SDCR, inpw(REG_SDCR) & (~SDCR_SDPORT) | SDCR_SDPORT_2);   // SD_2 port selected
        pSD = pSD2;
    }
    else
        return 1;       // wrong SD card select

    //--- 2014/2/26, Reset SD controller and DMAC to keep clean status for next access.
    // Reset DMAC engine and interrupt satus
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_SWRST | DMAC_EN);
    while(inpw(REG_DMACCSR) & DMAC_SWRST);
    outpw(REG_DMACCSR, inpw(REG_DMACCSR) | DMAC_EN);
    outpw(REG_DMACISR, WEOT_IF | TABORT_IF);    // clear all interrupt flag

    // Reset FMI engine and interrupt status
    outpw(REG_FMICR, FMI_SWRST);
    while(inpw(REG_FMICR) & FMI_SWRST);
    outpw(REG_FMIISR, FMI_DAT_IF);              // clear all interrupt flag

    // Reset SD engine and interrupt status
    outpw(REG_FMICR, FMI_SD_EN);
    outpw(REG_SDCR, inpw(REG_SDCR) | SDCR_SWRST);
    while(inpw(REG_SDCR) & SDCR_SWRST);
    outpw(REG_SDISR, 0xFFFFFFFF);               // clear all interrupt flag

    if (pSD != NULL)
    {
        //--- change SD clock for SD card on selected SD port.
        //--- NOTE: the clock setting also delay time to wait for stability of SD port change, especially for SD card.
        switch (pSD->uCardType)
        {
            case FMI_TYPE_MMC:              fmiSD_Set_clock(MMC_FREQ);  break;
            case FMI_TYPE_MMC_SECTOR_MODE:  fmiSD_Set_clock(EMMC_FREQ); break;
            case FMI_TYPE_SD_LOW:           fmiSD_Set_clock(SD_FREQ);   break;
            case FMI_TYPE_SD_HIGH:          fmiSD_Set_clock(SDHC_FREQ); break;
        }
        fmiSD_Change_Driver_Strength(cardSel, pSD->uCardType);
    }

    return 0;
}


/*-----------------------------------------------------------------------------
 * Get SD card status for SD port 0 and assign the status to global value pSD0->bIsCardInsert.
 * Return:  0              is SD card inserted;
 *          FMI_NO_SD_CARD is SD card removed.
 *---------------------------------------------------------------------------*/
INT  fmiSD_CardStatus()
{
    if (g_SD0_card_detect)
    {
        if (inpw(REG_SDISR) & SDISR_CD_Card)    // CD pin status
        {
            pSD0->bIsCardInsert = FALSE;
            return FMI_NO_SD_CARD;
        }
        else
        {
            pSD0->bIsCardInsert = TRUE;
            return 0;
        }
    }
    else
    {
        pSD0->bIsCardInsert = TRUE;     // always report card inserted.
        return 0;
    }
}


INT  fmiInitSDDevice(INT cardSel)
{
    PDISK_T *pDisk;
    DISK_DATA_T* pSDDisk;
    FMI_SD_INFO_T *pSD_temp = NULL;

    // Enable AHB clock for SD. MUST also enable SIC clock for SIC/SD engine.
    outpw(REG_AHBCLK, inpw(REG_AHBCLK) | SD_CKE | SIC_CKE);

    // Reset FMI
    outpw(REG_FMICR, FMI_SWRST);        // Start reset FMI controller.
    while(inpw(REG_FMICR)&FMI_SWRST);

    // enable SD
    outpw(REG_FMICR, FMI_SD_EN);
    outpw(REG_SDCR, inpw(REG_SDCR) | SDCR_SWRST);   // SD software reset
    while(inpw(REG_SDCR) & SDCR_SWRST);

    outpw(REG_SDISR, 0xFFFFFFFF);     // write bit 1 to clear all SDISR

    outpw(REG_SDCR, inpw(REG_SDCR) & ~0xFF);        // disable SD clock ouput

    if (cardSel == 0)
    {
        if(sd0_ok == 1)
            return(0);
        pSDDisk = &SD_DiskInfo0;
    }
    else if (cardSel == 1)
    {
        if(sd1_ok == 1)
            return(0);
        pSDDisk = &SD_DiskInfo1;
    }
    else if (cardSel == 2)
    {
        if(sd2_ok == 1)
            return(0);
        pSDDisk = &SD_DiskInfo2;
    }

    // enable SD-0/1/2 pin?
    if (fmiSD_CardSel(cardSel))
        return FMI_NO_SD_CARD;

    //Enable SD-0 card detectipn pin
    if (cardSel==0)
    {
         if (g_SD0_card_detect)
            outpw(REG_GPAFUN0, (inpw(REG_GPAFUN0) & (~MF_GPA1)) | 0x00000020);  // set GPA1 to 0010b for SD0 card detection
        outpw(REG_SDIER, inpw(REG_SDIER) | SDIER_CDSRC);    // SD card detection source from GPIO but not DAT3
    }

    // Disable FMI/SD host interrupt
    outpw(REG_FMIIER, 0);

//  outpw(REG_SDCR, (inpw(REG_SDCR) & ~SDCR_SDNWR) | (0x01 << 24));     // set SDNWR = 1
    outpw(REG_SDCR, (inpw(REG_SDCR) & ~SDCR_SDNWR) | (0x09 << 24));     // set SDNWR = 9
    outpw(REG_SDCR, (inpw(REG_SDCR) & ~SDCR_BLKCNT) | (0x01 << 16));    // set BLKCNT = 1
    outpw(REG_SDCR, inpw(REG_SDCR) & ~SDCR_DBW);        // SD 1-bit data bus

    pSD_temp = malloc(sizeof(FMI_SD_INFO_T)+4);
    if (pSD_temp == NULL)
        return FMI_NO_MEMORY;

    memset((char *)pSD_temp, 0, sizeof(FMI_SD_INFO_T)+4);

    if (cardSel==0)
    {
        pSD0_offset = (UINT32)pSD_temp %4;
        pSD0 = (FMI_SD_INFO_T *)((UINT32)pSD_temp + pSD0_offset);
    }
    else if (cardSel==1)
    {
        pSD1_offset = (UINT32)pSD_temp %4;
        pSD1 = (FMI_SD_INFO_T *)((UINT32)pSD_temp + pSD1_offset);
    }
    else if (cardSel==2)
    {
        pSD2_offset = (UINT32)pSD_temp %4;
        pSD2 = (FMI_SD_INFO_T *)((UINT32)pSD_temp + pSD2_offset);
    }

#ifdef _SIC_USE_INT_
    if (g_SD0_card_detect)
        outpw(REG_SDIER, inpw(REG_SDIER) | SDIER_CD_IEN);       // enable card detect interrupt
    else
        outpw(REG_SDIER, inpw(REG_SDIER) & (~SDIER_CD_IEN));    // disable card detect interrupt
    outpw(REG_SDIER, inpw(REG_SDIER) | SDIER_CRC_IEN);  // enable CRC error interrupt
#endif

    if (cardSel==0)
    {
        if (fmiSD_CardStatus() == FMI_NO_SD_CARD)
        {
            if (pSD0 != NULL)
            {
                free((FMI_SD_INFO_T *)((UINT32)pSD0 - pSD0_offset));
                pSD0 = 0;
            }
            free((FMI_SD_INFO_T *)((UINT32)pSD0 - pSD0_offset));
            sysprintf("ERROR: fmiInitSDDevice(): SD port %d has no card ! REG_SDISR = 0x%08X\n", cardSel, inpw(REG_SDISR));
            return FMI_NO_SD_CARD;
        }

        if (fmiSD_Init(pSD0) < 0)
            return FMI_SD_INIT_ERROR;

        /* divider */
        switch (pSD0->uCardType)
        {
            case FMI_TYPE_MMC:              fmiSD_Set_clock(MMC_FREQ);  break;
            case FMI_TYPE_MMC_SECTOR_MODE:  fmiSD_Set_clock(EMMC_FREQ); break;
            case FMI_TYPE_SD_LOW:           fmiSD_Set_clock(SD_FREQ);   break;
            case FMI_TYPE_SD_HIGH:          fmiSD_Set_clock(SDHC_FREQ); break;
            default:                        sysprintf("ERROR: Unknown card type in SD port 0 !!\n");                       break;
        }
        fmiSD_Change_Driver_Strength(cardSel, pSD0->uCardType);
    }
    else if (cardSel==1)
    {
        // SD-1 no card detect
        pSD1->bIsCardInsert = TRUE;

        // SD-1 initial
        if (fmiSD_Init(pSD1) < 0)
            return FMI_SD_INIT_ERROR;

        /* divider */
        switch (pSD1->uCardType)
        {
            case FMI_TYPE_MMC:              fmiSD_Set_clock(MMC_FREQ);  break;
            case FMI_TYPE_MMC_SECTOR_MODE:  fmiSD_Set_clock(EMMC_FREQ); break;
            case FMI_TYPE_SD_LOW:           fmiSD_Set_clock(SD_FREQ);   break;
            case FMI_TYPE_SD_HIGH:          fmiSD_Set_clock(SDHC_FREQ); break;
            default:                        sysprintf("ERROR: Unknown card type in SD port 1 !!\n");                       break;
        }
        fmiSD_Change_Driver_Strength(cardSel, pSD1->uCardType);
    }
    else if (cardSel==2)
    {
        // SD-2 no card detect
        pSD2->bIsCardInsert = TRUE;

        // SD-2 initial
        if (fmiSD_Init(pSD2) < 0)
            return FMI_SD_INIT_ERROR;

        /* divider */
        switch (pSD2->uCardType)
        {
            case FMI_TYPE_MMC:              fmiSD_Set_clock(MMC_FREQ);  break;
            case FMI_TYPE_MMC_SECTOR_MODE:  fmiSD_Set_clock(EMMC_FREQ); break;
            case FMI_TYPE_SD_LOW:           fmiSD_Set_clock(SD_FREQ);   break;
            case FMI_TYPE_SD_HIGH:          fmiSD_Set_clock(SDHC_FREQ); break;
            default:                        sysprintf("ERROR: Unknown card type in SD port 2 !!\n");                       break;
        }
        fmiSD_Change_Driver_Strength(cardSel, pSD2->uCardType);
    }

    /* init SD interface */
    if (cardSel==0)
    {
        fmiGet_SD_info(pSD0, pSDDisk);
        if (fmiSelectCard(pSD0))
            return FMI_SD_SELECT_ERROR;
    }
    else if (cardSel==1)
    {
        fmiGet_SD_info(pSD1, pSDDisk);
        if (fmiSelectCard(pSD1))
            return FMI_SD_SELECT_ERROR;
    }
    else if (cardSel==2)
    {
        fmiGet_SD_info(pSD2, pSDDisk);
        if (fmiSelectCard(pSD2))
            return FMI_SD_SELECT_ERROR;
    }

    /*
     * Create physical disk descriptor
     */
    pDisk = malloc(sizeof(PDISK_T));
    if (pDisk == NULL)
        return FMI_NO_MEMORY;
    memset((char *)pDisk, 0, sizeof(PDISK_T));

    /* read Disk information */
    pDisk->szManufacture[0] = '\0';
    strcpy(pDisk->szProduct, (char *)pSDDisk->product);
    strcpy(pDisk->szSerialNo, (char *)pSDDisk->serial);

    pDisk->nDiskType = DISK_TYPE_SD_MMC;

    pDisk->nPartitionN = 0;
    pDisk->ptPartList = NULL;

    pDisk->nSectorSize = 512;
    pDisk->uTotalSectorN = pSDDisk->totalSectorN;
    pDisk->uDiskSize = pSDDisk->diskSize;

    /* create relationship between UMAS device and file system hard disk device */
    if (cardSel==0)
        pDisk->ptDriver = &_SD0DiskDriver;
    else if (cardSel==1)
        pDisk->ptDriver = &_SD1DiskDriver;
    else if (cardSel==2)
        pDisk->ptDriver = &_SD2DiskDriver;

#ifdef DEBUG
    printf("SD disk found: size=%d MB\n", (int)pDisk->uDiskSize / 1024);
#endif

    if (cardSel==0)
    {
        pDisk_SD0 = pDisk;
    }
    else if (cardSel==1)
    {
        pDisk_SD1 = pDisk;
    }
    else if (cardSel==2)
    {
        pDisk_SD2 = pDisk;
    }

//    fsPhysicalDiskConnected(pDisk);

    if (cardSel == 0)
        sd0_ok = 1;
    else if (cardSel == 1)
        sd1_ok = 1;
    else if (cardSel == 2)
        sd2_ok = 1;

    return pDisk->uTotalSectorN;
}

INT  fmiSD_Read(UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr)
{
    int status=0;

    // enable SD
    outpw(REG_FMICR, FMI_SD_EN);
    status = fmiSD_Read_in(pSD0, uSector, uBufcnt, uDAddr);

    return status;
}

INT  fmiSD_Write(UINT32 uSector, UINT32 uBufcnt, UINT32 uSAddr)
{
    int status=0;

    // enable SD
    outpw(REG_FMICR, FMI_SD_EN);
    status = fmiSD_Write_in(pSD0, uSector, uBufcnt, uSAddr);

    return status;
}


VOID sicSdClose_sel(INT cardSel)
{
    if (cardSel==0)
    {
        sd0_ok = 0;
        //--- 2014/12/4, MUST free pDisk_SD0 BEFORE free pSD0
        //      because pSD0 could be used within fsUnmountPhysicalDisk().
        if (pDisk_SD0 != NULL)
        {
            fsUnmountPhysicalDisk(pDisk_SD0);
            free(pDisk_SD0);
            pDisk_SD0 = NULL;
        }
        if (pSD0 != NULL)
        {
            free((FMI_SD_INFO_T *)((UINT32)pSD0 - pSD0_offset));
            pSD0 = 0;
        }
    }
    else if (cardSel==1)
    {
        sd1_ok = 0;
        if (pDisk_SD1 != NULL)
        {
            fsUnmountPhysicalDisk(pDisk_SD1);
            free(pDisk_SD1);
            pDisk_SD1 = NULL;
        }
        if (pSD1 != NULL)
        {
            free((FMI_SD_INFO_T *)((UINT32)pSD1 - pSD1_offset));
            pSD1 = 0;
        }
    }
    else if (cardSel==2)
    {
        sd2_ok = 0;
        if (pDisk_SD2 != NULL)
        {
            fsUnmountPhysicalDisk(pDisk_SD2);
            free(pDisk_SD2);
            pDisk_SD2 = NULL;
        }
        if (pSD2 != NULL)
        {
            free((FMI_SD_INFO_T *)((UINT32)pSD2 - pSD2_offset));
            pSD2 = 0;
        }
    }
}

VOID sicSdClose(void)
{
    sicSdClose_sel(0);
}

VOID sicSdClose0(void)
{
    sicSdClose_sel(0);
}

VOID sicSdClose1(void)
{
    sicSdClose_sel(1);
}

VOID sicSdClose2(void)
{
    sicSdClose_sel(2);
}


/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicSdOpen                                                                       */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sdPortNo               SD port number(port0 or port1).                          */
/*                                                                                   */
/* Returns:                                                                          */
/*   >0                     Total sector.                                            */
/*   FMI_NO_SD_CARD         No SD card insert.                                       */
/*   FMI_SD_INIT_ERROR      Card initial and identify error.                         */
/*   FMI_SD_SELECT_ERROR    Select card from identify mode to stand-by mode error.   */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function initial SD from identification to stand-by mode.                  */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
INT sicSdOpen_sel(INT cardSel)
{
    return fmiInitSDDevice(cardSel);
}

INT sicSdOpen(void)
{
    return fmiInitSDDevice(0);
}

INT sicSdOpen0(void)
{
    return fmiInitSDDevice(0);
}

INT sicSdOpen1(void)
{
    return fmiInitSDDevice(1);
}

INT sicSdOpen2(void)
{
    return fmiInitSDDevice(2);
}
