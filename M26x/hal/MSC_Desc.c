/***************************************************************************//**
 * @file     MSC_Desc.c
 * @brief    M480 series USB class descriptions code for MSC
 * @version  0.0.1
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
/*!<Includes */
#include "NuMicro.h"
#include "MSC_Desc.h"
#include "MSC_Trans.h"

static uint8_t s_au8MSC_DeviceDescriptor[] __attribute__((aligned(4))) =
{
    LEN_DEVICE,         /* bLength */
    DESC_DEVICE,        /* bDescriptorType */
    0x10, 0x01,         /* bcdUSB */
    0x00,               /* bDeviceClass */
    0x00,               /* bDeviceSubClass */
    0x00,               /* bDeviceProtocol */
    EP0_MSC_MAX_PKT_SIZE,   /* bMaxPacketSize0 */
    /* idVendor */
    USBD_VID & 0x00FF,
    ((USBD_VID & 0xFF00) >> 8),
    /* idProduct */
    USBD_MSC_PID & 0x00FF,
    ((USBD_MSC_PID & 0xFF00) >> 8),
    0x00, 0x00,        /* bcdDevice */
    0x01,              /* iManufacture */
    0x02,              /* iProduct */
    0x03,              /* iSerialNumber -  is required for BOT device */
    0x01               /* bNumConfigurations */
};

static uint8_t s_au8MSC_ConfigDescriptor[] __attribute__((aligned(4))) =
{
    LEN_CONFIG,                                         // bLength
    DESC_CONFIG,                                        // bDescriptorType
    (LEN_CONFIG+LEN_INTERFACE+LEN_ENDPOINT*2), 0x00,    // wTotalLength
    0x01,                                               // bNumInterfaces
    0x01,                                               // bConfigurationValue
    0x00,                                               // iConfiguration
    0xC0,                                               // bmAttributes
    0x32,                                               // MaxPower

    /* const BYTE cbyInterfaceDescriptor[LEN_INTERFACE] = */
    LEN_INTERFACE,  // bLength
    DESC_INTERFACE, // bDescriptorType
    0x00,           // bInterfaceNumber
    0x00,           // bAlternateSetting
    0x02,           // bNumEndpoints
    0x08,           // bInterfaceClass
    0x05,           // bInterfaceSubClass
    0x50,           // bInterfaceProtocol
    0x00,           // iInterface

    /* const BYTE cbyEndpointDescriptor1[LEN_ENDPOINT] = */
    LEN_ENDPOINT,           // bLength
    DESC_ENDPOINT,          // bDescriptorType
    (EP_INPUT | MSC_BULK_IN_EP_NUM),    /* bEndpointAddress */
    EP_BULK,                // bmAttributes
    EP2_MSC_MAX_PKT_SIZE, 0x00, // wMaxPacketSize
    0x00,                   // bInterval

    /* const BYTE cbyEndpointDescriptor2[LEN_ENDPOINT] = */
    LEN_ENDPOINT,           // bLength
    DESC_ENDPOINT,          // bDescriptorType
    (EP_OUTPUT | MSC_BULK_OUT_EP_NUM),  /* bEndpointAddress */
    EP_BULK,                // bmAttributes
    EP3_MSC_MAX_PKT_SIZE, 0x00,  // wMaxPacketSize
    0x00                    // bInterval
};

static uint8_t s_au8MSC_StringLang[4] __attribute__((aligned(4))) =
{
    4,              /* bLength */
    DESC_STRING,    /* bDescriptorType */
    0x09, 0x04
};

static uint8_t s_au8MSC_VendorStringDesc[] __attribute__((aligned(4))) =
{
    16,
    DESC_STRING,
    'N', 0, 'u', 0, 'v', 0, 'o', 0, 't', 0, 'o', 0, 'n', 0
};

static uint8_t s_au8MSC_ProductStringDesc[] __attribute__((aligned(4))) =
{
    22,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    'U', 0, 'S', 0, 'B', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

static uint8_t s_au8MSC_StringSerial[] __attribute__((aligned(4))) =
{
    26,             // bLength
    DESC_STRING,    // bDescriptorType
    'A', 0, '0', 0, '2', 0, '0', 0, '0', 0, '8', 0, '0', 0, '4', 0, '0', 0, '1', 0, '1', 0, '4', 0
};


static uint8_t *s_apu8MSC_UsbString[4] =
{
    s_au8MSC_StringLang,
    s_au8MSC_VendorStringDesc,
    s_au8MSC_ProductStringDesc,
    s_au8MSC_StringSerial,
};

void MSCDesc_SetupDescInfo(
	S_USBD_INFO_T *psDescInfo
)
{
	psDescInfo->gu8DevDesc = s_au8MSC_DeviceDescriptor;
	psDescInfo->gu8ConfigDesc = s_au8MSC_ConfigDescriptor;
	psDescInfo->gu8StringDesc = s_apu8MSC_UsbString;
	psDescInfo->gu8HidReportDesc = NULL;
	psDescInfo->gu8BosDesc = NULL;
	psDescInfo->gu32HidReportSize = NULL;
	psDescInfo->gu32ConfigHidDescIdx = NULL;
}

void MSCDesc_SetVID(
	S_USBD_INFO_T *psDescInfo,
	uint16_t u16VID
)
{
	uint8_t *pu8DeviceDesc = psDescInfo->gu8DevDesc;

	if(pu8DeviceDesc == NULL)
		return;

	pu8DeviceDesc[8] = u16VID & 0x00FF;
	pu8DeviceDesc[9] = (u16VID & 0xFF00) >> 8;
}

void MSCDesc_SetPID(
	S_USBD_INFO_T *psDescInfo,
	uint16_t u16PID
)
{
	uint8_t *pu8DeviceDesc = psDescInfo->gu8DevDesc;

	if(pu8DeviceDesc == NULL)
		return;

	pu8DeviceDesc[10] = u16PID & 0x00FF;
	pu8DeviceDesc[11] = (u16PID & 0xFF00) >> 8;
}



