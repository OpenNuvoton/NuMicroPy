/**************************************************************************//**
 * @file     N9H26_USBDev.c
 * @version  V0.01
 * @brief    N9H26 series USB HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"

#include "N9H26_USBDev.h"

#include "MSC_VCPDesc.h"
#include "MSC_VCPTrans.h"

extern volatile USBD_INFO_T usbdInfo;	//in USBD driver

static S_USBDEV_STATE s_sUSBDev_state; 

uint8_t volatile g_u8MSCRemove = 0;
uint32_t volatile g_u32VCPConnect = 0;
uint32_t volatile g_u32MSCConnect = 0;
uint32_t volatile g_u32DataBusConnect = 0;

void USBDEV_Deinit(
	S_USBDEV_STATE *psUSBDevState
)
{
	if(psUSBDevState == NULL)
		return;

    udcDeinit();
    udcClose();
    
	psUSBDevState->eUSBMode = eUSBDEV_MODE_NONE;
}


S_USBDEV_STATE *USBDEV_Init(
	uint16_t u16VID,
	uint16_t u16PID,
	E_USBDEV_MODE eUSBMode
)
{
	if(eUSBMode == eUSBDEV_MODE_NONE){
		USBDEV_Deinit(&s_sUSBDev_state);
		return &s_sUSBDev_state;
	}

	if(eUSBMode == eUSBDEV_MODE_MSC_VCP){
		
		/* Enable USB */
		udcOpen();

		MSCVCPDesc_SetupDescInfo((USBD_INFO_T *)&usbdInfo);
		MSCVCPDesc_SetVID((USBD_INFO_T *)&usbdInfo, u16VID);
		MSCVCPDesc_SetPID((USBD_INFO_T *)&usbdInfo, u16PID);
		
		MSCVCPTrans_Init((USBD_INFO_T *)&usbdInfo);
	}
	else{
		printf("Not implement for HID class");
	}

	s_sUSBDev_state.eUSBMode = eUSBMode;
	return &s_sUSBDev_state;
}

S_USBDEV_STATE *USBDEV_UpdateState(void)
{
	s_sUSBDev_state.bConnected = 0;
	if(s_sUSBDev_state.eUSBMode == eUSBDEV_MODE_MSC_VCP){
		if((g_u32MSCConnect) && (g_u32VCPConnect))
			s_sUSBDev_state.bConnected = 1;
	}

	return &s_sUSBDev_state;
}

int32_t USBDEV_Start(
	S_USBDEV_STATE *psUSBDevState
)
{
	if(psUSBDevState == NULL)
		return -1;
	
	if(psUSBDevState->eUSBMode == eUSBDEV_MODE_NONE)
		return -2;

    udcInit();

	return 0;
}

E_USBDEV_MODE USBDEV_GetMode(
	S_USBDEV_STATE *psUSBDevState
)
{
	if(psUSBDevState == NULL)
		return eUSBDEV_MODE_NONE;
	return psUSBDevState->eUSBMode;
}

int32_t USBDEV_DataBusConnect(void)
{
	return g_u32DataBusConnect;
}

void USBDEV_MSCEnDisable(
	int32_t i32EnDisable
)
{
	if(i32EnDisable)
		g_u8MSCRemove = 0;
	else
		g_u8MSCRemove = 1;	
}

int32_t USBDEV_VCPCanSend(
	S_USBDEV_STATE *psUSBDevState
)
{
	if(!udcIsAttached())
		return -1;

	return VCPTrans_BulkInCanSend();
}

int32_t USBDEV_VCPSendData(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen,
	uint32_t u32Timeout,
	S_USBDEV_STATE *psUSBDevState
)
{
	int i32SendedLen = 0;
	if(pu8DataBuf == NULL)
		return i32SendedLen;
			
	i32SendedLen = VCPTrans_BulkInSend(pu8DataBuf, u32DataBufLen);
	
	uint32_t u32SendTime = mp_hal_ticks_ms();

	while((mp_hal_ticks_ms() - u32SendTime) < u32Timeout){
		if((i32SendedLen >=  u32DataBufLen))
			break;

		if(VCPTrans_BulkInCanSend()){
			i32SendedLen += VCPTrans_BulkInSend(pu8DataBuf + i32SendedLen, u32DataBufLen);
		}
	}
	
	return i32SendedLen;
}

int32_t USBDEV_VCPCanRecv(
	S_USBDEV_STATE *psUSBDevState
)
{
	return VCPTrans_BulkOutCanRecv();
}

int32_t USBDEV_VCPRecvData(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen,
	uint32_t u32Timeout,
	S_USBDEV_STATE *psUSBDevState
)
{
	int i32CopyLen = 0;
	int i32RecvLen = 0;

	if(pu8DataBuf == NULL)
		return i32RecvLen;

	uint32_t u32RecvTime = mp_hal_ticks_ms();
	
	while((mp_hal_ticks_ms() - u32RecvTime) < u32Timeout){
		
		i32CopyLen = VCPTrans_BulkOutRecv(pu8DataBuf + i32RecvLen, u32DataBufLen - i32RecvLen);
		if(i32CopyLen){
			i32RecvLen += i32CopyLen;
			if(i32RecvLen >= u32DataBufLen)
				break;
			u32RecvTime = mp_hal_ticks_ms();
		}
	}

	return i32RecvLen;
}

void USBDEV_VCPRegisterSingal(
	PFN_USBDEV_VCPRecvSignal pfnSignal
)
{
	VCPTrans_RegisterSingal(pfnSignal);
}

