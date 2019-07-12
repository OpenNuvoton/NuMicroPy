/**************************************************************************//**
 * @file     SDCard.h
 * @version  V1.00
 * @brief    M480 SD card HAL header file for micropython
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __SD_CARD_H__
#define __SD_CARD_H__

typedef struct sdcard_info_t
{
    unsigned int    CardType;       /*!< SDHC, SD, or MMC */
    unsigned int    RCA;            /*!< Relative card address */
    unsigned char   IsCardInsert;   /*!< Card insert state */
    unsigned int    totalSectorN;   /*!< Total sector number */
    unsigned int    diskSize;       /*!< Disk size in K bytes */
    int             sectorSize;     /*!< Sector size in bytes */
} S_SDCARD_INFO;                       /*!< Structure holds SD card info */


int32_t SDCard_Init(void);

int32_t SDCard_CardDetection(void);

int32_t SDCard_Read (
    uint8_t *buff,     /* Data buffer to store read data */
    uint32_t sector,   /* Sector address (LBA) */
    uint32_t count      /* Number of sectors to read (1..128) */
);

int32_t SDCard_Write (
    const uint8_t *buff,   /* Data to be written */
    uint32_t sector,       /* Sector address (LBA) */
    uint32_t count          /* Number of sectors to write (1..128) */
);

int32_t SDCard_GetCardInfo(
    S_SDCARD_INFO *psCardInfo
);

#endif


