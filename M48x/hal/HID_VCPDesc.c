/***************************************************************************//**
 * @file     HID_VCPDesc.c
 * @brief    M480 series USB class descriptions code for VCP and HID
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
/*!<Includes */
#include "NuMicro.h"
#include "M48x_USBD.h"
#include "HID_VCPDesc.h"
#include "HID_VCPTrans.h"

/*!<USB HID Report Descriptor */
uint8_t s_au8HIDVCP_DeviceReportDescriptor[] =
{
    0x06, 0x00, 0xFF,   // Usage Page = 0xFF00 (Vendor Defined Page 1)
    0x09, 0x01,         // Usage (Vendor Usage 1)
    0xA1, 0x01,         // Collection (Application)
    0x19, 0x01,         // Usage Minimum
    0x29, 0x40,         // Usage Maximum //64 input usages total (0x01 to 0x40)
    0x15, 0x00,         // Logical Minimum (data bytes in the report may have minimum value = 0x00)
    0x26, 0xFF, 0x00,   // Logical Maximum (data bytes in the report may have maximum value = 0x00FF = unsigned 255)
    0x75, 0x08,         // Report Size: 8-bit field size
    0x95, 0x40,         // Report Count: Make sixty-four 8-bit fields (the next time the parser hits
    // an "Input", "Output", or "Feature" item)
    0x81, 0x00,         // Input (Data, Array, Abs): Instantiates input packet fields based on the
    // above report size, count, logical min/max, and usage.
    0x19, 0x01,         // Usage Minimum
    0x29, 0x40,         // Usage Maximum //64 output usages total (0x01 to 0x40)
    0x91, 0x00,         // Output (Data, Array, Abs): Instantiates output packet fields. Uses same
    // report size and count as "Input" fields, since nothing new/different was
    // specified to the parser since the "Input" item.
    0xC0                // End Collection
};


/*----------------------------------------------------------------------------*/
/*!<USB Device Descriptor */
uint8_t s_au8HID_DeviceDescriptor[] =
{
    LEN_DEVICE,     /* bLength */
    DESC_DEVICE,    /* bDescriptorType */
    0x10, 0x01,     /* bcdUSB */
    0x00,           /* bDeviceClass */
    0x00,           /* bDeviceSubClass */
    0x00,           /* bDeviceProtocol */
    EP0_HID_VCP_MAX_PKT_SIZE,   /* bMaxPacketSize0 */
    /* idVendor */
    USBD_VID & 0x00FF,
    ((USBD_VID & 0xFF00) >> 8),
    /* idProduct */
    USBD_VCP_HID_PID & 0x00FF,
    ((USBD_VCP_HID_PID & 0xFF00) >> 8),
    0x00, 0x00,     /* bcdDevice */
    0x01,           /* iManufacture */
    0x02,           /* iProduct */
    0x00,           /* iSerialNumber - no serial */
    0x01            /* bNumConfigurations */
};

uint8_t s_au8HIDVCP_DeviceDescriptor[] =
{
    LEN_DEVICE,         /* bLength */
    DESC_DEVICE,        /* bDescriptorType */
    0x00, 0x02,         /* bcdUSB */
    0xEF,               /* bDeviceClass: IAD*/
    0x02,               /* bDeviceSubClass */
    0x01,               /* bDeviceProtocol */
    EP0_HID_VCP_MAX_PKT_SIZE,   /* bMaxPacketSize0 */
    /* idVendor */
    USBD_VID & 0x00FF,
    ((USBD_VID & 0xFF00) >> 8),
    /* idProduct */
    USBD_VCP_HID_PID & 0x00FF,
    ((USBD_VCP_HID_PID & 0xFF00) >> 8),
    0x00, 0x03,     /* bcdDevice */
    0x01,           /* iManufacture */
    0x02,           /* iProduct */
    0x00,           /* iSerialNumber - no serial */
    0x01            /* bNumConfigurations */
};

uint8_t s_au8VCP_DeviceDescriptor[] =
{
    LEN_DEVICE,     /* bLength */
    DESC_DEVICE,    /* bDescriptorType */
    0x10, 0x01,     /* bcdUSB */
    0x02,           /* bDeviceClass */
    0x00,           /* bDeviceSubClass */
    0x00,           /* bDeviceProtocol */
    EP0_HID_VCP_MAX_PKT_SIZE,   /* bMaxPacketSize0 */
    /* idVendor */
    USBD_VID & 0x00FF,
    ((USBD_VID & 0xFF00) >> 8),
    /* idProduct */
    USBD_VCP_HID_PID & 0x00FF,
    ((USBD_VCP_HID_PID & 0xFF00) >> 8),
    0x00, 0x03,     /* bcdDevice */
    0x01,           /* iManufacture */
    0x02,           /* iProduct */
    0x00,           /* iSerialNumber - no serial */
    0x01            /* bNumConfigurations */
};


/*!<USB Configure Descriptor */
uint8_t s_au8HID_ConfigDescriptor[] =
{
    LEN_CONFIG,     /* bLength */
    DESC_CONFIG,    /* bDescriptorType */
    /* wTotalLength */
    (LEN_CONFIG+LEN_INTERFACE+LEN_HID+LEN_ENDPOINT*2) & 0x00FF,
    (((LEN_CONFIG+LEN_INTERFACE+LEN_HID+LEN_ENDPOINT*2) & 0xFF00) >> 8),
    0x01,           /* bNumInterfaces */
    0x01,           /* bConfigurationValue */
    0x00,           /* iConfiguration */
    0x80 | (USBD_SELF_POWERED << 6) | (USBD_REMOTE_WAKEUP << 5),/* bmAttributes */
    USBD_MAX_POWER,         /* MaxPower */

    /* I/F descr: HID */
    LEN_INTERFACE,  /* bLength */
    DESC_INTERFACE, /* bDescriptorType */
    0x00,           /* bInterfaceNumber */
    0x00,           /* bAlternateSetting */
    0x02,           /* bNumEndpoints */
    0x03,           /* bInterfaceClass */
    0x00,           /* bInterfaceSubClass */
    0x00,           /* bInterfaceProtocol */
    0x00,           /* iInterface */

    /* HID Descriptor */
    LEN_HID,        /* Size of this descriptor in UINT8s. */
    DESC_HID,       /* HID descriptor type. */
    0x10, 0x01,     /* HID Class Spec. release number. */
    0x00,           /* H/W target country. */
    0x01,           /* Number of HID class descriptors to follow. */
    DESC_HID_RPT,   /* Descriptor type. */
    /* Total length of report descriptor. */
    sizeof(s_au8HIDVCP_DeviceReportDescriptor) & 0x00FF,
    ((sizeof(s_au8HIDVCP_DeviceReportDescriptor) & 0xFF00) >> 8),

    /* EP Descriptor: interrupt in. */
    LEN_ENDPOINT,                       /* bLength */
    DESC_ENDPOINT,                      /* bDescriptorType */
    (HID_INT_IN_EP_NUM | EP_INPUT),         /* bEndpointAddress */
    EP_INT,                             /* bmAttributes */
    /* wMaxPacketSize */
    EP5_HID_MAX_PKT_SIZE & 0x00FF,
    ((EP5_HID_MAX_PKT_SIZE & 0xFF00) >> 8),
    HID_DEFAULT_INT_IN_INTERVAL,        /* bInterval */

    /* EP Descriptor: interrupt out. */
    LEN_ENDPOINT,                       /* bLength */
    DESC_ENDPOINT,                      /* bDescriptorType */
    (HID_INT_OUT_EP_NUM | EP_OUTPUT),   /* bEndpointAddress */
    EP_INT,                             /* bmAttributes */
    /* wMaxPacketSize */
    EP6_HID_MAX_PKT_SIZE & 0x00FF,
    ((EP6_HID_MAX_PKT_SIZE & 0xFF00) >> 8),
    HID_DEFAULT_INT_IN_INTERVAL         /* bInterval */
};

#if 0
/*!<USB Configure Descriptor */
uint8_t s_au8HIDVCP_ConfigDescriptor[] =
{
    LEN_CONFIG,         /* bLength              */
    DESC_CONFIG,        /* bDescriptorType      */
    0x6B, 0x00,         /* wTotalLength         */
    0x03,               /* bNumInterfaces       */
    0x01,               /* bConfigurationValue  */
    0x00,               /* iConfiguration       */
    0xC0,               /* bmAttributes         */
    0x32,               /* MaxPower             */

    // IAD
    0x08,               // bLength: Interface Descriptor size
    0x0B,               // bDescriptorType: IAD
    0x00,               // bFirstInterface
    0x02,               // bInterfaceCount
    0x02,               // bFunctionClass: CDC
    0x02,               // bFunctionSubClass
    0x01,               // bFunctionProtocol
    0x02,               // iFunction

    /* VCOM */
    /* INTERFACE descriptor */
    LEN_INTERFACE,  /* bLength              */
    DESC_INTERFACE, /* bDescriptorType      */
    0x00,           /* bInterfaceNumber     */
    0x00,           /* bAlternateSetting    */
    0x01,           /* bNumEndpoints        */
    0x02,           /* bInterfaceClass      */
    0x02,           /* bInterfaceSubClass   */
    0x01,           /* bInterfaceProtocol   */
    0x00,           /* iInterface           */

    /* Communication Class Specified INTERFACE descriptor */
    0x05,           /* Size of the descriptor, in bytes */
    0x24,           /* CS_INTERFACE descriptor type */
    0x00,           /* Header functional descriptor subtype */
    0x10, 0x01,     /* Communication device compliant to the communication spec. ver. 1.10 */

    /* Communication Class Specified INTERFACE descriptor */
    0x05,           /* Size of the descriptor, in bytes */
    0x24,           /* CS_INTERFACE descriptor type */
    0x01,           /* Call management functional descriptor */
    0x00,           /* BIT0: Whether device handle call management itself. */
    /* BIT1: Whether device can send/receive call management information over a Data Class Interface 0 */
    0x01,           /* Interface number of data class interface optionally used for call management */

    /* Communication Class Specified INTERFACE descriptor */
    0x04,           /* Size of the descriptor, in bytes */
    0x24,           /* CS_INTERFACE descriptor type */
    0x02,           /* Abstract control management functional descriptor subtype */
    0x00,           /* bmCapabilities       */

    /* Communication Class Specified INTERFACE descriptor */
    0x05,           /* bLength              */
    0x24,           /* bDescriptorType: CS_INTERFACE descriptor type */
    0x06,           /* bDescriptorSubType   */
    0x00,           /* bMasterInterface     */
    0x01,           /* bSlaveInterface0     */

    /* ENDPOINT descriptor */
    LEN_ENDPOINT,                   /* bLength          */
    DESC_ENDPOINT,                  /* bDescriptorType  */
    (EP_INPUT | VCP_INT_IN_EP_NUM),     /* bEndpointAddress */
    EP_INT,                         /* bmAttributes     */
    EP4_VCP_MAX_PKT_SIZE, 0x00,         /* wMaxPacketSize   */
    0x01,                           /* bInterval        */

    /* INTERFACE descriptor */
    LEN_INTERFACE,                  /* bLength              */
    DESC_INTERFACE,                 /* bDescriptorType      */
    0x01,                           /* bInterfaceNumber     */
    0x00,                           /* bAlternateSetting    */
    0x02,                           /* bNumEndpoints        */
    0x0A,                           /* bInterfaceClass      */
    0x00,                           /* bInterfaceSubClass   */
    0x00,                           /* bInterfaceProtocol   */
    0x00,                           /* iInterface           */

    /* ENDPOINT descriptor */
    LEN_ENDPOINT,                   /* bLength          */
    DESC_ENDPOINT,                  /* bDescriptorType  */
    (EP_INPUT | VCP_BULK_IN_EP_NUM),    /* bEndpointAddress */
    EP_BULK,                        /* bmAttributes     */
    EP2_VCP_MAX_PKT_SIZE, 0x00,         /* wMaxPacketSize   */
    0x00,                           /* bInterval        */

    /* ENDPOINT descriptor */
    LEN_ENDPOINT,                   /* bLength          */
    DESC_ENDPOINT,                  /* bDescriptorType  */
    (EP_OUTPUT | VCP_BULK_OUT_EP_NUM),  /* bEndpointAddress */
    EP_BULK,                        /* bmAttributes     */
    EP3_VCP_MAX_PKT_SIZE, 0x00,         /* wMaxPacketSize   */
    0x00,                           /* bInterval        */

    /* HID class device */
    /* I/F descr: HID */
    LEN_INTERFACE,  /* bLength */
    DESC_INTERFACE, /* bDescriptorType */
    0x02,           /* bInterfaceNumber */
    0x00,           /* bAlternateSetting */
    0x02,           /* bNumEndpoints */
    0x03,           /* bInterfaceClass */
    0x00,           /* bInterfaceSubClass */
    0x00,           /* bInterfaceProtocol */
    0x00,           /* iInterface */

    /* HID Descriptor */
    LEN_HID,        /* Size of this descriptor in UINT8s. */
    DESC_HID,       /* HID descriptor type. */
    0x10, 0x01,     /* HID Class Spec. release number. */
    0x00,           /* H/W target country. */
    0x01,           /* Number of HID class descriptors to follow. */
    DESC_HID_RPT,   /* Descriptor type. */
    /* Total length of report descriptor. */
    sizeof(s_au8HIDVCP_DeviceReportDescriptor) & 0x00FF,
    ((sizeof(s_au8HIDVCP_DeviceReportDescriptor) & 0xFF00) >> 8),

    /* EP Descriptor: interrupt in. */
    LEN_ENDPOINT,                       /* bLength */
    DESC_ENDPOINT,                      /* bDescriptorType */
    (HID_INT_IN_EP_NUM | EP_INPUT),       /* bEndpointAddress */
    EP_INT,                             /* bmAttributes */
    /* wMaxPacketSize */
    EP5_HID_MAX_PKT_SIZE & 0x00FF,
    ((EP5_HID_MAX_PKT_SIZE & 0xFF00) >> 8),
    HID_DEFAULT_INT_IN_INTERVAL,        /* bInterval */

    /* EP Descriptor: interrupt out. */
    LEN_ENDPOINT,                       /* bLength */
    DESC_ENDPOINT,                      /* bDescriptorType */
    (HID_INT_OUT_EP_NUM | EP_OUTPUT),     /* bEndpointAddress */
    EP_INT,                             /* bmAttributes */
    /* wMaxPacketSize */
    EP6_HID_MAX_PKT_SIZE & 0x00FF,
    ((EP6_HID_MAX_PKT_SIZE & 0xFF00) >> 8),
    HID_DEFAULT_INT_IN_INTERVAL,        /* bInterval */
};

/*!<USB Configure Descriptor */
uint8_t s_au8VCP_ConfigDescriptor[] =
{
    LEN_CONFIG,     /* bLength              */
    DESC_CONFIG,    /* bDescriptorType      */
    0x43, 0x00,     /* wTotalLength         */
    0x02,           /* bNumInterfaces       */
    0x01,           /* bConfigurationValue  */
    0x00,           /* iConfiguration       */
    0xC0,           /* bmAttributes         */
    0x32,           /* MaxPower             */

    /* INTERFACE descriptor */
    LEN_INTERFACE,  /* bLength              */
    DESC_INTERFACE, /* bDescriptorType      */
    0x00,           /* bInterfaceNumber     */
    0x00,           /* bAlternateSetting    */
    0x01,           /* bNumEndpoints        */
    0x02,           /* bInterfaceClass      */
    0x02,           /* bInterfaceSubClass   */
    0x01,           /* bInterfaceProtocol   */
    0x00,           /* iInterface           */

    /* Communication Class Specified INTERFACE descriptor */
    0x05,           /* Size of the descriptor, in bytes */
    0x24,           /* CS_INTERFACE descriptor type */
    0x00,           /* Header functional descriptor subtype */
    0x10, 0x01,     /* Communication device compliant to the communication spec. ver. 1.10 */

    /* Communication Class Specified INTERFACE descriptor */
    0x05,           /* Size of the descriptor, in bytes */
    0x24,           /* CS_INTERFACE descriptor type */
    0x01,           /* Call management functional descriptor */
    0x00,           /* BIT0: Whether device handle call management itself. */
    /* BIT1: Whether device can send/receive call management information over a Data Class Interface 0 */
    0x01,           /* Interface number of data class interface optionally used for call management */

    /* Communication Class Specified INTERFACE descriptor */
    0x04,           /* Size of the descriptor, in bytes */
    0x24,           /* CS_INTERFACE descriptor type */
    0x02,           /* Abstract control management funcational descriptor subtype */
    0x00,           /* bmCapabilities       */

    /* Communication Class Specified INTERFACE descriptor */
    0x05,           /* bLength              */
    0x24,           /* bDescriptorType: CS_INTERFACE descriptor type */
    0x06,           /* bDescriptorSubType   */
    0x00,           /* bMasterInterface     */
    0x01,           /* bSlaveInterface0     */

    /* ENDPOINT descriptor */
    LEN_ENDPOINT,                   /* bLength          */
    DESC_ENDPOINT,                  /* bDescriptorType  */
    (EP_INPUT | VCP_INT_IN_EP_NUM),     /* bEndpointAddress */
    EP_INT,                         /* bmAttributes     */
    EP4_VCP_MAX_PKT_SIZE, 0x00,         /* wMaxPacketSize   */
    0x01,                           /* bInterval        */

    /* INTERFACE descriptor */
    LEN_INTERFACE,  /* bLength              */
    DESC_INTERFACE, /* bDescriptorType      */
    0x01,           /* bInterfaceNumber     */
    0x00,           /* bAlternateSetting    */
    0x02,           /* bNumEndpoints        */
    0x0A,           /* bInterfaceClass      */
    0x00,           /* bInterfaceSubClass   */
    0x00,           /* bInterfaceProtocol   */
    0x00,           /* iInterface           */

    /* ENDPOINT descriptor */
    LEN_ENDPOINT,                   /* bLength          */
    DESC_ENDPOINT,                  /* bDescriptorType  */
    (EP_INPUT | VCP_BULK_IN_EP_NUM),    /* bEndpointAddress */
    EP_BULK,                        /* bmAttributes     */
    EP2_VCP_MAX_PKT_SIZE, 0x00,         /* wMaxPacketSize   */
    0x00,                           /* bInterval        */

    /* ENDPOINT descriptor */
    LEN_ENDPOINT,                   /* bLength          */
    DESC_ENDPOINT,                  /* bDescriptorType  */
    (EP_OUTPUT | VCP_BULK_OUT_EP_NUM),  /* bEndpointAddress */
    EP_BULK,                        /* bmAttributes     */
    EP3_VCP_MAX_PKT_SIZE, 0x00,         /* wMaxPacketSize   */
    0x00,                           /* bInterval        */
};

#endif

/*!<USB Language String Descriptor */
uint8_t s_au8HIDVCP_StringLang[4] =
{
    4,              /* bLength */
    DESC_STRING,    /* bDescriptorType */
    0x09, 0x04
};

/*!<USB Vendor String Descriptor */
uint8_t s_au8HIDVCP_VendorStringDesc[] =
{
    16,
    DESC_STRING,
    'N', 0, 'u', 0, 'v', 0, 'o', 0, 't', 0, 'o', 0, 'n', 0
};


/*!<USB Product String Descriptor */
uint8_t s_au8HID_ProductStringDesc[] =
{
    26,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    'H', 0, 'I', 0, 'D', 0, ' ', 0, 'T', 0, 'r', 0, 'a', 0, 'n', 0, 's', 0, 'f', 0, 'e', 0, 'r', 0
};

/*!<USB Product String Descriptor */
uint8_t s_au8HIDVCP_ProductStringDesc[] =
{
    22,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    'U', 0, 'S', 0, 'B', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

/*!<USB Product String Descriptor */
uint8_t s_au8VCP_ProductStringDesc[] =
{
    32,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    'U', 0, 'S', 0, 'B', 0, ' ', 0, 'V', 0, 'i', 0, 'r', 0, 't', 0, 'u', 0, 'a', 0, 'l', 0, ' ', 0, 'C', 0, 'O', 0, 'M', 0
};

/*!<USB BOS Descriptor */
uint8_t s_au8HIDVCP_BOSDescriptor[] =
{
    LEN_BOS,        /* bLength */
    DESC_BOS,       /* bDescriptorType */
    /* wTotalLength */
    0x0C & 0x00FF,
    ((0x0C & 0xFF00) >> 8),
    0x01,           /* bNumDeviceCaps */

    /* Device Capability */
    LEN_BOSCAP,     /* bLength */
    DESC_CAPABILITY,/* bDescriptorType */
    CAP_USB20_EXT,  /* bDevCapabilityType */
    0x02, 0x00, 0x00, 0x00  /* bmAttributes */
};


uint8_t *s_apu8HID_UsbString[4] =
{
    s_au8HIDVCP_StringLang,
    s_au8HIDVCP_VendorStringDesc,
    s_au8HID_ProductStringDesc,
    NULL,
};

uint8_t *s_apu8HIDVCP_UsbString[4] =
{
    s_au8HIDVCP_StringLang,
    s_au8HIDVCP_VendorStringDesc,
    s_au8HIDVCP_ProductStringDesc,
    NULL,
};

uint8_t *s_apu8VCP_UsbString[4] =
{
    s_au8HIDVCP_StringLang,
    s_au8HIDVCP_VendorStringDesc,
    s_au8VCP_ProductStringDesc,
    NULL,
};


uint8_t *s_apu8HID_UsbHidReport[3] =
{
    s_au8HIDVCP_DeviceReportDescriptor,
    NULL,
    NULL,
};

uint32_t s_au32HID_UsbHidReportLen[3] =
{
    sizeof(s_au8HIDVCP_DeviceReportDescriptor),
    0,
    0,
};

uint32_t s_au32HID_ConfigHidDescIdx[3] =
{
    (LEN_CONFIG+LEN_INTERFACE),
    0,
    0,
};

uint8_t *s_apu8HIDVCP_UsbHidReport[3] =
{
    NULL,
    NULL,
    s_au8HIDVCP_DeviceReportDescriptor,
};

uint32_t s_au32HIDVCP_UsbHidReportLen[3] =
{
    0,
    0,
    sizeof(s_au8HIDVCP_DeviceReportDescriptor),
};

#if 0
uint32_t s_au32HIDVCP_ConfigHidDescIdx[3] =
{
    0,
    0,
    (sizeof(s_au8HIDVCP_ConfigDescriptor) - LEN_HID - (2*LEN_ENDPOINT)),
};
#endif

uint8_t *s_apu8VCP_UsbHidReport[3] =
{
    NULL,
    NULL,
    NULL,
};

uint32_t s_au32VCP_UsbHidReportLen[3] =
{
    0,
    0,
    0,
};

uint32_t s_au32VCP_ConfigHidDescIdx[3] =
{
    0,
    0,
    0,
};



void HIDVCPDesc_SetupDescInfo(
	S_USBD_INFO_T *psDescInfo,
	E_USBDEV_MODE eUSBMode
)
{
	if(eUSBMode == eUSBDEV_MODE_HID){
		psDescInfo->gu8DevDesc = s_au8HID_DeviceDescriptor;
		psDescInfo->gu8ConfigDesc = s_au8HID_ConfigDescriptor;
		psDescInfo->gu8StringDesc = s_apu8HID_UsbString;
		psDescInfo->gu8HidReportDesc = s_apu8HID_UsbHidReport;
		psDescInfo->gu8BosDesc = s_au8HIDVCP_BOSDescriptor;
		psDescInfo->gu32HidReportSize = s_au32HID_UsbHidReportLen;
		psDescInfo->gu32ConfigHidDescIdx = s_au32HID_ConfigHidDescIdx;

	}

#if 0
	else if(eUSBMode == eUSBDEV_MODE_HID_VCP){
		psDescInfo->gu8DevDesc = s_au8HIDVCP_DeviceDescriptor;
		psDescInfo->gu8ConfigDesc = s_au8HIDVCP_ConfigDescriptor;
		psDescInfo->gu8StringDesc = s_apu8HIDVCP_UsbString;
		psDescInfo->gu8HidReportDesc = s_apu8HIDVCP_UsbHidReport;
		psDescInfo->gu8BosDesc = s_au8HIDVCP_BOSDescriptor;
		psDescInfo->gu32HidReportSize = s_au32HIDVCP_UsbHidReportLen;
		psDescInfo->gu32ConfigHidDescIdx = s_au32HIDVCP_ConfigHidDescIdx;

	}
	else if(eUSBMode == eUSBDEV_MODE_VCP){
		psDescInfo->gu8DevDesc = s_au8VCP_DeviceDescriptor;
		psDescInfo->gu8ConfigDesc = s_au8VCP_ConfigDescriptor;
		psDescInfo->gu8StringDesc = s_apu8VCP_UsbString;
		psDescInfo->gu8HidReportDesc = s_apu8VCP_UsbHidReport;
		psDescInfo->gu8BosDesc = s_au8HIDVCP_BOSDescriptor;
		psDescInfo->gu32HidReportSize = s_au32VCP_UsbHidReportLen;
		psDescInfo->gu32ConfigHidDescIdx = s_au32VCP_ConfigHidDescIdx;

	}
#endif
}


void HIDVCPDesc_SetVID(
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

void HIDVCPDesc_SetPID(
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

