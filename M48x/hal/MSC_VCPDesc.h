/***************************************************************************//**
 * @file     MSC_VCPDesc.h
 * @brief    M480 series USB class descriptions code for mass storage and VCP
 * @version  0.0.1
 *
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __MSC_VCPDESC_H__
#define __MSC_VCPDESC_H__

#include "M48x_USBD.h"

void MSCVCPDesc_SetupDescInfo(
	S_USBD_INFO_T *psDescInfo
);

void MSCVCPDesc_SetVID(
	S_USBD_INFO_T *psDescInfo,
	uint16_t u16VID
);

void MSCVCPDesc_SetPID(
	S_USBD_INFO_T *psDescInfo,
	uint16_t u16PID
);

#endif
