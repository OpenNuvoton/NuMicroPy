/***************************************************************************//**
 * @file     MSC_VCPDesc.h
 * @brief    M5531 series USB class descriptions code for mass storage and VCP
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "NuMicro.h"
#include "MSC_VCPDesc.h"
#include "MSC_VCPTrans.h"


uint8_t s_au8MSCVCP_DeviceDescriptor[] = {
    LEN_DEVICE,         /* bLength */
    DESC_DEVICE,        /* bDescriptorType */
    0x00, 0x02,         /* bcdUSB */
    0xEF,               /* bDeviceClass:*/
    0x02,               /* bDeviceSubClass */
    0x01,               /* bDeviceProtocol */
    EP0_MSC_VCP_MAX_PKT_SIZE,   /* bMaxPacketSize0 */
    /* idVendor */
    USBD_VID & 0x00FF,
    ((USBD_VID & 0xFF00) >> 8),
    /* idProduct */
    USBD_MSC_VCP_PID & 0x00FF,
    ((USBD_MSC_VCP_PID & 0xFF00) >> 8),
    0x00, 0x01,     /* bcdDevice */
    0x01,           /* iManufacture */
    0x02,           /* iProduct */
    0x03,           /* iSerialNumber, must set if enable multi-LUN*/
    0x01            /* bNumConfigurations */
};

/*!<USB Configure Descriptor */
uint8_t s_au8MSCVCP_ConfigDescriptor[] = {
    LEN_CONFIG,     /* bLength              */
    DESC_CONFIG,    /* bDescriptorType      */
    0x62, 0x00,     /* wTotalLength         */
    0x03,           /* bNumInterfaces       */
    0x01,           /* bConfigurationValue  */
    0x00,           /* iConfiguration       */
    0xC0,           /* bmAttributes         */
    0x32,           /* MaxPower             */

    // IAD
    0x08,           // bLength: Interface Descriptor size
    0x0B,           // bDescriptorType: IAD
    0x00,           // bFirstInterface
    0x02,           // bInterfaceCount
    0x02,           // bFunctionClass: CDC
    0x02,           // bFunctionSubClass
    0x01,           // bFunctionProtocol
    0x00,           // iFunction

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

    /* MSC class device */
    /* INTERFACE descriptor */
    LEN_INTERFACE,  // bLength
    DESC_INTERFACE, // bDescriptorType
    0x02,           // bInterfaceNumber
    0x00,           // bAlternateSetting
    0x02,           // bNumEndpoints
    0x08,           // bInterfaceClass
    0x06,           /* bInterfaceSubClass */
    0x50,           // bInterfaceProtocol
    0x00,           // iInterface

    /* ENDPOINT descriptor */
    LEN_ENDPOINT,                    // bLength
    DESC_ENDPOINT,                   // bDescriptorType
    (EP_INPUT | MSC_BULK_IN_EP_NUM),   // bEndpointAddress
    EP_BULK,                         // bmAttributes
    EP5_MSC_MAX_PKT_SIZE, 0x00,          // wMaxPacketSize
    0x00,                            // bInterval

    /* ENDPOINT descriptor */
    LEN_ENDPOINT,                    // bLength
    DESC_ENDPOINT,                   // bDescriptorType
    (EP_OUTPUT | MSC_BULK_OUT_EP_NUM), // bEndpointAddress
    EP_BULK,                         // bmAttributes
    EP6_MSC_MAX_PKT_SIZE, 0x00,          // wMaxPacketSize
    0x00,                            // bInterval
};

/*!<USB Language String Descriptor */
uint8_t s_au8MSCVCP_StringLang[4] = {
    4,              /* bLength */
    DESC_STRING,    /* bDescriptorType */
    0x09, 0x04
};

/*!<USB Vendor String Descriptor */
uint8_t s_au8MSCVCP_VendorStringDesc[] = {
    16,
    DESC_STRING,
    'N', 0, 'u', 0, 'v', 0, 'o', 0, 't', 0, 'o', 0, 'n', 0
};

/*!<USB Product String Descriptor */
uint8_t s_au8MSCVCP_ProductStringDesc[] = {
    22,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    'U', 0, 'S', 0, 'B', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

/*!<USB Serial String Descriptor. Must enable if support mulit-LUN */
uint8_t s_au8MSCVCP_SerialStringDesc[] = {
    6,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    '0', '0', '0', '1'
};


/*!<USB BOS Descriptor */
uint8_t s_au8MSCVCP_BOSDescriptor[] = {
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

uint8_t *s_apu8MSCVCP_UsbString[4] = {
    s_au8MSCVCP_StringLang,
    s_au8MSCVCP_VendorStringDesc,
    s_au8MSCVCP_ProductStringDesc,
    s_au8MSCVCP_SerialStringDesc,
};


uint8_t *gu8UsbHIDReport[3] = {
    NULL,
    NULL,
    NULL,
};

uint32_t gu32UsbHIDReportLen[3] = {
    0,
    0,
    0,
};

uint32_t gu32ConfigHIDDescIdx[3] = {
    0,
    0,
    0,
};

void MSCVCPDesc_SetupDescInfo(
    S_USBD_INFO_T *psDescInfo
)
{
    psDescInfo->gu8DevDesc = s_au8MSCVCP_DeviceDescriptor;
    psDescInfo->gu8ConfigDesc = s_au8MSCVCP_ConfigDescriptor;
    psDescInfo->gu8StringDesc = s_apu8MSCVCP_UsbString;
    psDescInfo->gu8HidReportDesc = gu8UsbHIDReport;
    psDescInfo->gu8BosDesc = s_au8MSCVCP_BOSDescriptor;
    psDescInfo->gu32HidReportSize = gu32UsbHIDReportLen;
    psDescInfo->gu32ConfigHidDescIdx = gu32ConfigHIDDescIdx;
}


void MSCVCPDesc_SetVID(
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

void MSCVCPDesc_SetPID(
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

