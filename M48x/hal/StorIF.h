/***************************************************************************//**
 * @file     StorIF.h
 * @brief    Storage access interface
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __STORIF_H__
#define __STORIF_H__

#include <inttypes.h>

#define STORIF_SECTOR_SIZE 512

typedef enum{
	eSTORIF_ERRNO_NONE = 0,
	eSTORIF_ERRNO_NULL_PTR = -1,
	eSTORIF_ERRNO_MALLOC = -2,
	eSTORIF_ERRNO_IO = -3,	
	eSTORIF_ERRNO_SIZE = -4,
	eSTORIF_ERRNO_NOT_READY = -5,
	eSTORIF_ERRNO_STOR_OPEN = -6,
	eSTORIF_ERRNO_DEVICE = -7,
}E_STORIF_ERRNO;


typedef	E_STORIF_ERRNO
(*PFN_STORIF_INIT)(
	int32_t i32Inst,
	void **ppStorRes
);

typedef	int32_t
(*PFN_STORIF_READ_SECTOR)(
	uint8_t *pu8Buff,		/* Data buffer to store read data */
	uint32_t u32Sector,		/* Sector address (LBA) */
	uint32_t u32Count,		/* Number of sectors to read (1..128) */
	void *pvStorRes
);

typedef	int32_t
(*PFN_STORIF_WRITE_SECTOR)(
	uint8_t *pu8Buff,		/* Data buffer to store read data */
	uint32_t u32Sector,		/* Sector address (LBA) */
	uint32_t u32Count,		/* Number of sectors to read (1..128) */
	void *pvStorRes
);

typedef	int32_t
(*PFN_STORIF_DETECT)(
	void *pvStorRes
);

typedef struct{
	uint32_t u32TotalSector;
	uint32_t u32DiskSize; //K Bytes
	uint32_t u32SectorSize; // Bytes
	uint32_t u32SubType;
}S_STORIF_INFO;


typedef	E_STORIF_ERRNO
(*PFN_STORIF_GET_INFO)(
	S_STORIF_INFO *psInfo,
	void *pvStorRes
);


typedef struct{
	PFN_STORIF_INIT 			pfnStorInit;
	PFN_STORIF_READ_SECTOR		pfnReadSector;
	PFN_STORIF_WRITE_SECTOR		pfnWriteSector;
	PFN_STORIF_DETECT			pfnDetect;
	PFN_STORIF_GET_INFO			pfnGetInfo;
}S_STORIF_IF;

extern const S_STORIF_IF g_STORIF_sFlash;
extern const S_STORIF_IF g_STORIF_sSPIFlash;
extern const S_STORIF_IF g_STORIF_sSDCard;

#endif
