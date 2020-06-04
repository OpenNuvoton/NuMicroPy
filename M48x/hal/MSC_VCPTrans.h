/***************************************************************************//**
 * @file    MSC_VCPTrans.h
 * @brief    M480 series USB class transfer code for VCP and MSC
 * @version  0.0.1
 *
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#ifndef __MSC_VCPTRANS_H__
#define __MSC_VCPTRANS_H__

/* Define EP maximum packet size */
#define EP0_MSC_VCP_MAX_PKT_SIZE    64
#define EP1_MSC_VCP_MAX_PKT_SIZE    EP0_MSC_VCP_MAX_PKT_SIZE
#define EP2_VCP_MAX_PKT_SIZE    64
#define EP3_VCP_MAX_PKT_SIZE    64
#define EP4_VCP_MAX_PKT_SIZE    8
#define EP5_MSC_MAX_PKT_SIZE    64
#define EP6_MSC_MAX_PKT_SIZE    64

#define SETUP_MSC_VCP_BUF_BASE      0
#define SETUP_MSC_VCP_BUF_LEN       8
#define EP0_MSC_VCP_BUF_BASE        (SETUP_MSC_VCP_BUF_BASE + SETUP_MSC_VCP_BUF_LEN)
#define EP0_MSC_VCP_BUF_LEN         EP0_MSC_VCP_MAX_PKT_SIZE
#define EP1_MSC_VCP_BUF_BASE        (SETUP_MSC_VCP_BUF_BASE + SETUP_MSC_VCP_BUF_LEN)
#define EP1_MSC_VCP_BUF_LEN         EP1_MSC_VCP_MAX_PKT_SIZE
#define EP2_VCP_BUF_BASE        	(EP1_MSC_VCP_BUF_BASE + EP1_MSC_VCP_BUF_LEN)
#define EP2_VCP_BUF_LEN         	EP2_VCP_MAX_PKT_SIZE
#define EP3_VCP_BUF_BASE        	(EP2_VCP_BUF_BASE + EP2_VCP_BUF_LEN)
#define EP3_VCP_BUF_LEN         	EP3_VCP_MAX_PKT_SIZE
#define EP4_VCP_BUF_BASE       		(EP3_VCP_BUF_BASE + EP3_VCP_BUF_LEN)
#define EP4_VCP_BUF_LEN        		EP4_VCP_MAX_PKT_SIZE
#define EP5_MSC_BUF_BASE        	(EP4_VCP_BUF_BASE + EP4_VCP_BUF_LEN)
#define EP5_MSC_BUF_LEN         	EP5_MSC_MAX_PKT_SIZE
#define EP6_MSC_BUF_BASE        	(EP5_MSC_BUF_BASE + EP5_MSC_BUF_LEN)
#define EP6_MSC_BUF_LEN         	EP6_MSC_MAX_PKT_SIZE


/* Define the EP number */
#define VCP_BULK_IN_EP_NUM        0x02
#define VCP_BULK_OUT_EP_NUM       0x03
#define VCP_INT_IN_EP_NUM         0x04
#define MSC_BULK_IN_EP_NUM      0x05
#define MSC_BULK_OUT_EP_NUM     0x06

/*!<Define CDC Class Specific Request */
#define SET_LINE_CODE           0x20
#define GET_LINE_CODE           0x21
#define SET_CONTROL_LINE_STATE  0x22

/************************************************/
/* for CDC class */
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


/*!<Define Mass storage Class Specific Request */
#define LEN_CONFIG_AND_SUBORDINATE      (LEN_CONFIG+LEN_INTERFACE+LEN_ENDPOINT*2)

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

/*-----------------------------------------*/
#define BULK_CBW  0x00
#define BULK_IN   0x01
#define BULK_OUT  0x02
#define BULK_CSW  0x04
#define BULK_NORMAL 0xFF

static __INLINE uint32_t get_be32(uint8_t *buf)
{
    return ((uint32_t) buf[0] << 24) | ((uint32_t) buf[1] << 16) |
           ((uint32_t) buf[2] << 8) | ((uint32_t) buf[3]);
}


/*------------------------------------------*/
//VCP send/recv function

int32_t VCPTrans_BulkInHandler();

#if 0

int32_t VCPTrans_StartBulkIn(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
);

int32_t VCPTrans_BulkInSendedLen();

int32_t VCPTrans_BulkInCanSend();

int32_t VCPTrans_BulkInTransDone();

void VCPTrans_StopBulkIn();

#else

int32_t VCPTrans_BulkInSend(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
);

int32_t VCPTrans_BulkInCanSend();

#endif


int32_t VCPTrans_BulkOutHandler(
	uint8_t *pu8EPBuf, 
	uint32_t u32Size
);

int32_t VCPTrans_BulkOutCanRecv();

int32_t VCPTrans_BulkOutRecv(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
);


/******************************************************************************/
/*                USBD Mass Storage Structure                                 */
/******************************************************************************/
/** @addtogroup M480_USBD_Mass_Exported_Struct M480 USBD Mass Exported Struct
  M480 USBD Mass Specific Struct
  @{
*/

/*!<USB Mass Storage Class - Command Block Wrapper Structure */
struct CBW
{
    uint32_t  dCBWSignature;
    uint32_t  dCBWTag;
    uint32_t  dCBWDataTransferLength;
    uint8_t   bmCBWFlags;
    uint8_t   bCBWLUN;
    uint8_t   bCBWCBLength;
    uint8_t   u8OPCode;
    uint8_t   u8LUN;
    uint8_t   au8Data[14];
};

/*!<USB Mass Storage Class - Command Status Wrapper Structure */
struct CSW
{
    uint32_t  dCSWSignature;
    uint32_t  dCSWTag;
    uint32_t  dCSWDataResidue;
    uint8_t   bCSWStatus;
};

/*-------------------------------------------------------------*/
#define MASS_BUFFER_SIZE    256                /* Mass Storage command buffer size */
#define STORAGE_BUFFER_SIZE 512               /* Data transfer buffer size in 512 bytes alignment */
#define UDC_SECTOR_SIZE   512               /* logic sector size */

/*------------------------------------------*/
void MSCVCPTrans_ClassRequest(void);
void MSCVCPTrans_Init(
	S_USBD_INFO_T *psUSBDevInfo
);
void MSCTrans_SetConfig(void);
void MSCTrans_BulkInHandler(void);
void MSCTrans_BulkOutHandler(void);
void MSCTrans_ProcessCmd(void);


#endif
