/***************************************************************************//**
 * @file     MSC_Trans.h
 * @brief    M26x series USB class transfer code for MSC
 * @version  0.0.1
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#ifndef __MSC_TRANS_H__
#define __MSC_TRANS_H__

/* Define EP maximum packet size */
#define EP0_MSC_MAX_PKT_SIZE    64
#define EP1_MSC_MAX_PKT_SIZE    EP0_MSC_MAX_PKT_SIZE
#define EP2_MSC_MAX_PKT_SIZE    64
#define EP3_MSC_MAX_PKT_SIZE    64

#define SETUP_MSC_BUF_BASE      0
#define SETUP_MSC_BUF_LEN       8
#define EP0_MSC_BUF_BASE        (SETUP_MSC_BUF_BASE + SETUP_MSC_BUF_LEN)
#define EP0_MSC_BUF_LEN         EP0_MSC_MAX_PKT_SIZE
#define EP1_MSC_BUF_BASE        (SETUP_MSC_BUF_BASE + SETUP_MSC_BUF_LEN)
#define EP1_MSC_BUF_LEN         EP1_MSC_MAX_PKT_SIZE
#define EP2_MSC_BUF_BASE        (EP1_MSC_BUF_BASE + EP1_MSC_BUF_LEN)
#define EP2_MSC_BUF_LEN         EP2_MSC_MAX_PKT_SIZE
#define EP3_MSC_BUF_BASE        (EP2_MSC_BUF_BASE + EP2_MSC_BUF_LEN)
#define EP3_MSC_BUF_LEN         EP3_MSC_MAX_PKT_SIZE

/* Define the interrupt In EP number */
#define MSC_BULK_IN_EP_NUM      0x02
#define MSC_BULK_OUT_EP_NUM     0x03

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


/******************************************************************************/
/*                USBD Mass Storage Structure                                 */
/******************************************************************************/
/** @addtogroup M26x_USBD_Mass_Exported_Struct M26x USBD Mass Exported Struct
  M26x USBD Mass Specific Struct
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

/*-----------------------------------------------------------------*/
void MSCTrans_Init(
	S_USBD_INFO_T *psUSBDevInfo
);

void MSCTrans_SetConfig(void);

void MSCTrans_BulkInHandler(void);
void MSCTrans_BulkOutHandler(void);

void MSCTrans_ClassRequest(void);
void MSCTrans_ProcessCmd(void);

#endif
