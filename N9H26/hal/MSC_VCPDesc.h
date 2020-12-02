/***************************************************************************//**
 * @file     MSC_VCPDesc.h
 * @brief    N9H26 series USB class descriptions code for mass storage and VCP
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __MSC_VCPDESC_H__
#define __MSC_VCPDESC_H__

#include "wblib.h"
#include "N9H26_USBD.h"

/* USB Descriptor Type */
#define DESC_DEVICE         0x01ul
#define DESC_CONFIG         0x02ul
#define DESC_STRING         0x03ul
#define DESC_INTERFACE      0x04ul
#define DESC_ENDPOINT       0x05ul
#define DESC_QUALIFIER      0x06ul
#define DESC_OTHERSPEED     0x07ul
#define DESC_IFPOWER        0x08ul
#define DESC_OTG            0x09ul
#define DESC_BOS            0x0Ful
#define DESC_CAPABILITY     0x10ul

/* USB Descriptor Length */
#define LEN_DEVICE          18ul
#define LEN_QUALIFIER       10ul
#define LEN_CONFIG          9ul
#define LEN_INTERFACE       9ul
#define LEN_ENDPOINT        7ul
#define LEN_OTG             5ul
#define LEN_BOS             5ul
#define LEN_HID             9ul
#define LEN_CCID            0x36ul
#define LEN_BOSCAP          7ul

void MSCVCPDesc_SetupDescInfo(
	USBD_INFO_T *psUSBDInfo
);

void MSCVCPDesc_SetVID(
	USBD_INFO_T *psUSBDInfo,
	uint16_t u16VID
);

void MSCVCPDesc_SetPID(
	USBD_INFO_T *psUSBDInfo,
	uint16_t u16PID
);

#endif
