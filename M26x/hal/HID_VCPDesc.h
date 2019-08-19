/***************************************************************************//**
 * @file     HID_VCPDesc.h
 * @brief    M261 series USB class descriptions code for VCP and HID
 * @version  0.0.1
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __HID_VCPDESC_H__
#define __HID_VCPDESC_H__

#include "M26x_USBD.h"

void HIDVCPDesc_SetupDescInfo(
	S_USBD_INFO_T *psDescInfo,
	E_USBDEV_MODE eUSBMode
);

void HIDVCPDesc_SetVID(
	S_USBD_INFO_T *psDescInfo,
	uint16_t u16VID
);

void HIDVCPDesc_SetPID(
	S_USBD_INFO_T *psDescInfo,
	uint16_t u16PID
);

#endif
