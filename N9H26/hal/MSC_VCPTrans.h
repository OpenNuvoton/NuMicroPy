/***************************************************************************//**
 * @file    MSC_VCPTrans.h
 * @brief    N9H26 series USB class transfer code for VCP and MSC
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#ifndef __MSC_VCPTRANS_H__
#define __MSC_VCPTRANS_H__

#include "wblib.h"
#include "N9H26_USBD.h"

#include "N9H26_USBDev.h"

/* Define EP maximum packet size */
#define EPCTRL_MSC_VCP_MAX_PKT_SIZE    64

#define EP_VCP_HS_MAX_PKT_SIZE		512
#define EPC_VCP_HS_MAX_PKT_SIZE		512
#define EPD_VCP_HS_MAX_PKT_SIZE		512
#define EPA_MSC_HS_MAX_PKT_SIZE		512
#define EPB_MSC_HS_MAX_PKT_SIZE		512

#define EP_VCP_FS_MAX_PKT_SIZE     64
#define EPC_VCP_FS_MAX_PKT_SIZE    64
#define EPD_VCP_FS_MAX_PKT_SIZE    64
#define EPA_MSC_FS_MAX_PKT_SIZE    64
#define EPB_MSC_FS_MAX_PKT_SIZE    64

/* Define the EP number */
#define MSC_BULK_IN_EP_NUM			0x01		//for EPA
#define MSC_BULK_OUT_EP_NUM			0x02		//for EPB
#define VCP_BULK_IN_EP_NUM			0x03		//for EPC
#define VCP_BULK_OUT_EP_NUM			0x04		//for EPD
#define VCP_INT_IN_EP_NUM			0x05

/************************************************/
/* for CDC class */
/*!<Define CDC Class Specific Request */
#define SET_LINE_CODE           0x20
#define GET_LINE_CODE           0x21
#define SET_CONTROL_LINE_STATE  0x22

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
}__attribute__((packed)) STR_VCOM_LINE_CODING;



/************************************************/
/* for MSC class */
/*!<Define Mass Storage Class Specific Request */
#define BULK_ONLY_MASS_STORAGE_RESET    0xFF
#define GET_MAX_LUN                     0xFE

/*!<Define Mass Storage Signature */
#define CBW_SIGNATURE       0x43425355
#define CSW_SIGNATURE       0x53425355

/*!<Define Mass Storage UFI Command */
#define UFI_TEST_UNIT_READY                     0x00
#define UFI_REQUEST_SENSE                       0x03
#define UFI_INQUIRY                             0x12
#define UFI_MODE_SELECT_6                       0x15
#define UFI_MODE_SENSE_6                        0x1A
#define UFI_START_STOP                          0x1B
#define UFI_PREVENT_ALLOW_MEDIUM_REMOVAL        0x1E
#define UFI_READ_FORMAT_CAPACITY                0x23
#define UFI_READ_CAPACITY                       0x25
#define UFI_READ_10                             0x28
#define UFI_READ_12                             0xA8
#define UFI_READ_16                             0x9E
#define UFI_WRITE_10                            0x2A
#define UFI_WRITE_12                            0xAA
#define UFI_VERIFY_10                           0x2F
#define UFI_MODE_SELECT_10                      0x55
#define UFI_MODE_SENSE_10                       0x5A


/*------------------------------------------*/
//VCP send/recv function
int32_t VCPTrans_BulkInSend(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
);

int32_t VCPTrans_BulkInCanSend();

int32_t VCPTrans_BulkOutCanRecv();

int32_t VCPTrans_BulkOutRecv(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
);

void VCPTrans_RegisterSingal(
	PFN_USBDEV_VCPRecvSignal pfnSignal
);

/*------------------------------------------*/
//MSC function
//void MSCTrans_Process(void);

/*------------------------------------------*/
//MSC and VCP function
void MSCVCPTrans_Init(
	USBD_INFO_T *psUSBDInfo
);

#endif
