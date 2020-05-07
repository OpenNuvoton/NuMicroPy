/**************************************************************************//**
 * @file     M48x_USB.h
 * @version  V1.00
 * @brief    M480 USB HAL header file for micropython
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M48X_USB_H__
#define __M48X_USB_H__

#include <inttypes.h>

/* Define the vendor id and product id */
#define USBD_VID        	0x0416
#define USBD_VCP_HID_PID    0xDC00
#define USBD_MSC_PID        0x501F
#define USBD_MSC_VCP_PID    0xDC01

/* Define Descriptor information */
#define USBD_SELF_POWERED               0
#define USBD_REMOTE_WAKEUP              0
#define USBD_MAX_POWER                  50  /* The unit is in 2mA. ex: 50 * 2mA = 100mA */


typedef enum{
	eUSBDEV_MODE_NONE = 0,
	eUSBDEV_MODE_HID = 0x1,
//	eUSBDEV_MODE_VCP = 0x2,
//	eUSBDEV_MODE_HID_VCP = 0x3,
	eUSBDEV_MODE_MSC_VCP = 0x4,	
} E_USBDEV_MODE;

typedef struct{
	E_USBDEV_MODE eUSBMode;
	int bConnected;
}S_USBDEV_STATE;

E_USBDEV_MODE USBDEV_GetMode(S_USBDEV_STATE *psUSBDevState);

S_USBDEV_STATE *USBDEV_Init(
	uint16_t u16VID,
	uint16_t u16PID,
	E_USBDEV_MODE eUSBMode
);

S_USBDEV_STATE *USBDEV_UpdateState(void);

int32_t USBDEV_Start(
	S_USBDEV_STATE *psUSBDevState
);

void USBDEV_Deinit(
	S_USBDEV_STATE *psUSBDevState
);

void Handle_USBDEV_Irq(
	uint32_t u32IntStatus,
	uint32_t u32BusState
);

int32_t USBDEV_HIDCanRecv(
	S_USBDEV_STATE *psUSBDevState
);

int32_t USBDEV_VCPCanRecv(
	S_USBDEV_STATE *psUSBDevState
);

int32_t USBDEV_HIDRecvData(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen,
	uint32_t u32Timeout,
	S_USBDEV_STATE *psUSBDevState
);

int32_t USBDEV_VCPRecvData(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen,
	uint32_t u32Timeout,
	S_USBDEV_STATE *psUSBDevState
);


int32_t USBDEV_HIDCanSend(
	S_USBDEV_STATE *psUSBDevState
);

int32_t USBDEV_VCPCanSend(
	S_USBDEV_STATE *psUSBDevState
);

int32_t USBDEV_HIDSendData(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen,
	uint32_t u32Timeout,
	S_USBDEV_STATE *psUSBDevState
);

int32_t USBDEV_VCPSendData(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen,
	uint32_t u32Timeout,
	S_USBDEV_STATE *psUSBDevState
);


int32_t USBDEV_HIDInReportPacketSize();
int32_t USBDEV_HIDOutReportPacketSize();


#endif


