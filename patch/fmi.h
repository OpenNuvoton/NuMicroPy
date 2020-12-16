/**************************************************************************//**
 * @file     fmi.h
 * @version  V3.00
 * @brief    N9H26 series SIC driver header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#ifndef _FMI_H
#define _FMI_H

#include <stdio.h>
#include "wbio.h"

/*-----------------------------------------------------------------------------
 * To define some flag for emulation or FPGA
 *---------------------------------------------------------------------------*/
// Define OPT_FPGA_DEBUG to run special code only for FPGA board
//#define OPT_FPGA_DEBUG

// define DATE FMI_DATE_CODE and show it when running to make maintaining easy.
#ifdef OPT_FPGA_DEBUG
    #define FMI_DATE_CODE   "20181029 for FPGA"
#else
    #define FMI_DATE_CODE   "20181029"
#endif

// Define _SIC_USE_INT_ to run code that use interrupt function
#define _SIC_USE_INT_

// Define DEBUG to show more information for debugging
//#define DEBUG

#define TIMER0  0

//#define _USE_DAT3_DETECT_

//-- function return value
#define    Successful  0
#define    Fail        1

//--- define type of SD card or MMC
#define FMI_TYPE_UNKNOWN                0
#define FMI_TYPE_SD_HIGH                1
#define FMI_TYPE_SD_LOW                 2
#define FMI_TYPE_MMC                    3   // MMC access mode: Byte mode for capacity <= 2GB
#define FMI_TYPE_MMC_SECTOR_MODE        4   // MMC access mode: Sector mode for capacity > 2GB

#ifdef ECOS
#define sysGetTicks(TIMER0)  cyg_current_time()
#endif

// extern global variables
extern UINT32 _fmi_uFMIReferenceClock;
extern BOOL volatile _fmi_bIsSDDataReady;
extern BOOL volatile _fmi_bIsSMPRegionDetect;

#define STOR_STRING_LEN 32

/* we allocate one of these for every device that we remember */
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


// function declaration

// SD functions
INT  fmiSDCommand(FMI_SD_INFO_T *pSD, UINT8 ucCmd, UINT32 uArg);
INT  fmiSDCmdAndRsp(FMI_SD_INFO_T *pSD, UINT8 ucCmd, UINT32 uArg, INT nCount);
INT  fmiSDCmdAndRsp2(FMI_SD_INFO_T *pSD, UINT8 ucCmd, UINT32 uArg, UINT *puR2ptr);
INT  fmiSDCmdAndRspDataIn(FMI_SD_INFO_T *pSD, UINT8 ucCmd, UINT32 uArg);
INT  fmiSD_Init(FMI_SD_INFO_T *pSD);
INT  fmiSelectCard(FMI_SD_INFO_T *pSD);
VOID fmiGet_SD_info(FMI_SD_INFO_T *pSD, DISK_DATA_T *_info);
INT  fmiSD_Read_in(FMI_SD_INFO_T *pSD, UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr);
INT  fmiSD_Write_in(FMI_SD_INFO_T *pSD, UINT32 uSector, UINT32 uBufcnt, UINT32 uSAddr);
VOID fmiCheckRB(void);

/*-----------------------------------------------------------------------------
 * 2011/6/29, declaration more functions
 *---------------------------------------------------------------------------*/
VOID fmiSD_Set_clock(UINT32 sd_clock_khz);
VOID fmiSD_Show_info(int sdport);
INT  fmiSD_Read_in_blksize(FMI_SD_INFO_T *pSD, UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr, UINT32 blksize);
VOID fmiSD_Change_Driver_Strength(int card_no, int card_type);
INT  fmiSD_CardStatus(void);

// function prototype for FMI
VOID fmiInitDevice(void);
VOID fmiSetFMIReferenceClock(UINT32 uClock);
VOID fmiSetCallBack(UINT32 uCard, PVOID pvRemove, PVOID pvInsert);  // callback function
INT  fmiInitSDDevice(INT cardSel);  // for file system

// function prototype for FMI/SD
INT  fmiSD_CardSel(INT cardSel);
INT  fmiSD_Read(UINT32 uSector, UINT32 uBufcnt, UINT32 uDAddr);
INT  fmiSD_Write(UINT32 uSector, UINT32 uBufcnt, UINT32 uSAddr);

// function prototype for FMI/SM
VOID fmiSMClose(INT chipSel);

INT sicSdRead_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdRead0_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdRead1_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdRead2_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);

INT sicSdWrite_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdWrite0_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdWrite1_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);
INT sicSdWrite2_SG(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr, INT32 outOfOrder);

INT sicSMRegionProtect(INT chipSel, INT PBA, INT page);

#endif  // end of _FMI_H
