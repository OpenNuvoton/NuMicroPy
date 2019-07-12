/***************************************************************************//**
 * @file     MSC_Desc.h
 * @brief    M480 series USB class descriptions code for mass storage
 * @version  0.0.1
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __MSC_DESC_H__
#define __MSC_DESC_H__

#include "M26x_USBD.h"

void MSCDesc_SetupDescInfo(
	S_USBD_INFO_T *psDescInfo
);

void MSCDesc_SetVID(
	S_USBD_INFO_T *psDescInfo,
	uint16_t u16VID
);

void MSCDesc_SetPID(
	S_USBD_INFO_T *psDescInfo,
	uint16_t u16PID
);

#endif
