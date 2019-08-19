/***************************************************************************//**
 * @file     HID_VCPTrans.h
 * @brief    M261 series USB class transfer code for VCP and HID
 * @version  0.0.1
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#ifndef __HID_VCPTRANS_H__
#define __HID_VCPTRANS_H__


/*!<Define VCP Class Specific Request */
#define VCP_SET_LINE_CODE           0x20
#define VCP_GET_LINE_CODE           0x21
#define VCP_SET_CONTROL_LINE_STATE  0x22

/*!<Define HID Class Specific Request */
#define HID_GET_REPORT              0x01
#define HID_GET_IDLE                0x02
#define HID_GET_PROTOCOL            0x03
#define HID_SET_REPORT              0x09
#define HID_SET_IDLE                0x0A
#define HID_SET_PROTOCOL            0x0B

/*!<USB HID Interface Class protocol */
#define HID_NONE                0x00
#define HID_KEYBOARD            0x01
#define HID_MOUSE               0x02

/*!<USB HID Class Report Type */
#define HID_RPT_TYPE_INPUT      0x01
#define HID_RPT_TYPE_OUTPUT     0x02
#define HID_RPT_TYPE_FEATURE    0x03


/*-------------------------------------------------------------*/
/* Define EP maximum packet size */
#define EP0_HID_VCP_MAX_PKT_SIZE    8
#define EP1_HID_VCP_MAX_PKT_SIZE    EP0_HID_VCP_MAX_PKT_SIZE
#define EP2_VCP_MAX_PKT_SIZE    64
#define EP3_VCP_MAX_PKT_SIZE    64
#define EP4_VCP_MAX_PKT_SIZE    8
#define EP5_HID_MAX_PKT_SIZE    64
#define EP6_HID_MAX_PKT_SIZE    64

#define SETUP_HID_VCP_BUF_BASE  0
#define SETUP_HID_VCP_BUF_LEN   8
#define EP0_HID_VCP_BUF_BASE    (SETUP_HID_VCP_BUF_BASE + SETUP_HID_VCP_BUF_LEN)
#define EP0_HID_VCP_BUF_LEN     EP0_HID_VCP_MAX_PKT_SIZE
#define EP1_HID_VCP_BUF_BASE    (SETUP_HID_VCP_BUF_BASE + SETUP_HID_VCP_BUF_LEN)
#define EP1_HID_VCP_BUF_LEN     EP1_HID_VCP_MAX_PKT_SIZE
#define EP2_VCP_BUF_BASE    (EP1_HID_VCP_BUF_BASE + EP1_HID_VCP_BUF_LEN)
#define EP2_VCP_BUF_LEN     EP2_VCP_MAX_PKT_SIZE
#define EP3_VCP_BUF_BASE    (EP2_VCP_BUF_BASE + EP2_VCP_BUF_LEN)
#define EP3_VCP_BUF_LEN     EP3_VCP_MAX_PKT_SIZE
#define EP4_VCP_BUF_BASE    (EP3_VCP_BUF_BASE + EP3_VCP_BUF_LEN)
#define EP4_VCP_BUF_LEN     EP4_VCP_MAX_PKT_SIZE
#define EP5_HID_BUF_BASE    (EP4_VCP_BUF_BASE + EP4_VCP_BUF_LEN)
#define EP5_HID_BUF_LEN     EP5_HID_MAX_PKT_SIZE
#define EP6_HID_BUF_BASE    (EP5_HID_BUF_BASE + EP5_HID_BUF_LEN)
#define EP6_HID_BUF_LEN     EP6_HID_MAX_PKT_SIZE


/* Define the EP number */
#define VCP_BULK_IN_EP_NUM        0x01
#define VCP_BULK_OUT_EP_NUM       0x02
#define VCP_INT_IN_EP_NUM         0x03
//#define INT_OUT_EP_NUM	      0x04
#define HID_INT_IN_EP_NUM       0x05
#define HID_INT_OUT_EP_NUM      0x06

/* Define Descriptor information */
#define HID_DEFAULT_INT_IN_INTERVAL     1
#define USBD_SELF_POWERED               0
#define USBD_REMOTE_WAKEUP              0
#define USBD_MAX_POWER                  50  /* The unit is in 2mA. ex: 50 * 2mA = 100mA */

/************************************************/
/* for VCP class */
/* Line coding structure
  0-3 dwDTERate    Data terminal rate (baudrate), in bits per second
  4   bCharFormat  Stop bits: 0 - 1 Stop bit, 1 - 1.5 Stop bits, 2 - 2 Stop bits
  5   bParityType  Parity:    0 - None, 1 - Odd, 2 - Even, 3 - Mark, 4 - Space
  6   bDataBits    Data bits: 5, 6, 7, 8, 16  */

typedef struct
{
    uint32_t  u32DTERate;     /* Baud rate    */
    uint8_t   u8CharFormat;   /* stop bit     */
    uint8_t   u8ParityType;   /* parity       */
    uint8_t   u8DataBits;     /* data bits    */
} STR_VCOM_LINE_CODING;

void HIDVCPTrans_ClassRequest(void);
void HIDVCPTrans_Init(void);

//HID send/recv function
int32_t HIDVCPTrans_StartHIDSetInReport(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
);

int32_t HIDVCPTrans_HIDInterruptInHandler();
void HIDVCPTrans_StopHIDSetInReport();
int32_t HIDVCPTrans_HIDSetInReportSendedLen();
int32_t HIDVCPTrans_HIDSetInReportCanSend();

int32_t HIDVCPTrans_HIDInterruptOutHandler(
	uint8_t *pu8EPBuf, 
	uint32_t u32Size
);

int32_t HIDVCPTrans_HIDGetOutReportCanRecv();

int32_t HIDVCPTrans_HIDGetOutReportRecv(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
);


//VCP send/recv function

int32_t HIDVCPTrans_VCPBulkInHandler();

int32_t HIDVCPTrans_StartVCPBulkIn(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
);

int32_t HIDVCPTrans_VCPBulkInSendedLen();

int32_t HIDVCPTrans_VCPBulkInCanSend();

void HIDVCPTrans_StopVCPBulkIn();

int32_t HIDVCPTrans_VCPBulkOutHandler(
	uint8_t *pu8EPBuf, 
	uint32_t u32Size
);

int32_t HIDVCPTrans_VCPBulkOutCanRecv();

int32_t HIDVCPTrans_VCPBulkOutRecv(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
);


#endif
