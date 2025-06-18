/**************************************************************************//**
 * @file     StorIF_SDCard.h
 * @version  V1.00
 * @brief    M55M1 SD card storage interface header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __STORIF_SDCARD_H__
#define __STORIF_SDCARD_H__

extern void SDH_IntHandler(SDH_T *psSDH, SDH_INFO_T *psSDInfo);

#endif
