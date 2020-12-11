/***************************************************************************//**
 * @file     MSC_VCPTrans.c
 * @brief    N9H26 series USB class transfer code for VCP and MSC
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
/*!<Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MSC_VCPTrans.h"

#include "N9H26_USBDev.h"
#include "StorIF.h"

#include "mpconfigboard_common.h"

typedef struct{
    UINT32  Mass_Base_Addr;			//MSC CBW command buffer
    UINT32  Storage_Base_Addr;		//MSC data buffer
    UINT32  SizePerSector;
    UINT32  _usbd_DMA_Dir;
    UINT32  _usbd_Less_MPS;
    UINT32  USBD_DMA_LEN;
    UINT32  bulkonlycmd;
    UINT32  dwUsedBytes;
    UINT32  dwResidueLen;
    UINT32  dwCBW_flag;
    UINT32  TxedFlag;
    UINT8   SenseKey ;
    UINT8   ASC;
    UINT8   ASCQ;
    UINT32  Bulk_First_Flag;
    UINT32  MB_Invalid_Cmd_Flg;
    UINT32  preventflag;

}MSC_INFO_T;

typedef struct {
    UINT32    dCBWSignature;
    UINT8    dCBWTag[4];
    union
    {
        UINT8    c[4];
        UINT32    c32;
    } dCBWDataTferLh;
    UINT8    bmCBWFlags;
    UINT8    bCBWLUN;
    UINT8    bCBWCBLength;
    UINT8    OP_Code;
    UINT8    LUN;
    union 
    {
        UINT8    d[4];
        __packed    UINT32    d32;
    } LBA;
    union
    {
        UINT8    f[4];
        __packed    UINT32    f32;
    } CBWCB_In;
    UINT8    CBWCB_Reserved[6];
} __attribute__((packed)) CBW;

union {
    CBW        CBWD;
    UINT8      CBW_Array[31];
} CBW_In;

typedef struct {
    UINT8    dCSWSignature[4];
    UINT8    dCSWTag[4];
    union
    {
        UINT8    g[4];
        UINT32   g32;
    } dCSWDataResidue;
    UINT8    bCSWStatus;
} __attribute__((packed)) CSW;

union {
    CSW        CSWD;
    UINT8      CSW_Array[13];
} CSW_In;


#define MSC_BUFFER_SECTOR 	256
#define MSC_SECTOR_SIZE 	512

/*--------------------------------------------------------------------------*/
/* Global variables for all */

/* USB Device Property */
extern USB_CMD_T  _usb_cmd_pkt;
static USBD_INFO_T *s_psUSBDevInfo =  NULL;

extern uint32_t volatile g_u32VCPConnect;
extern uint32_t volatile g_u32MSCConnect;
extern uint32_t volatile g_u32DataBusConnect;
extern uint8_t volatile g_u8MSCRemove;


/*----------------------------------------------------*/
/* Global variables for VCP */
static STR_VCOM_LINE_CODING s_sLineCoding = {115200, 0, 0, 8};   /* Baud rate : 115200    */
/* Stop bit     */
/* parity       */
/* data bits    */
//static uint16_t s_u16CtrlSignal = 0;     /* BIT0: DTR(Data Terminal Ready) , BIT1: RTS(Request To Send) */

/*----------------------------------------------------*/
/* Global variables for MSC */

#define DEF_MAX_LUN 4

//static uint8_t volatile s_u8EPAReady = 0;
//static uint8_t volatile s_u8EPBReady = 0;

static int32_t volatile s_i32MSCConnectCheck = 0;

static int32_t volatile s_i32MSCMaxLun;

static S_STORIF_IF *s_apsLUNStorIf[DEF_MAX_LUN];
static MSC_INFO_T s_sMSCDInfo;

static UINT8 MSC_DATA_BUFFER[MSC_BUFFER_SECTOR * MSC_SECTOR_SIZE]  __attribute__((aligned(64)));	//Used for MSC BOT data buffer
static UINT8 MSC_CMD_BUFFER[4096]  __attribute__((aligned(64)));		//Used for MSC BOT CBW command buffer

static uint8_t s_au8InquiryID[36] =
{
    0x00,                   /* Peripheral Device Type */
    0x80,                   /* RMB */
    0x00,                   /* ISO/ECMA, ANSI Version */
    0x00,                   /* Response Data Format */
    0x1F, 0x00, 0x00, 0x00, /* Additional Length */

    /* Vendor Identification */
    'N', 'u', 'v', 'o', 't', 'o', 'n', ' ',

    /* Product Identification */
    'U', 'S', 'B', ' ', 'M', 'a', 's', 's', ' ', 'S', 't', 'o', 'r', 'a', 'g', 'e',

    /* Product Revision */
    '1', '.', '0', '0'
};

// code = 5Ah, Mode Sense 10
static uint8_t s_au8ModePage_01[12] =
{
    0x01, 0x0A, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00
};

static uint8_t s_au8ModePage_05[32] =
{
    0x05, 0x1E, 0x13, 0x88, 0x08, 0x20, 0x02, 0x00,
    0x01, 0xF4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x1E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x68, 0x00, 0x00
};

static uint8_t s_au8ModePage_1B[12] =
{
    0x1B, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static uint8_t s_au8ModePage_1C[8] =
{
    0x1C, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00
};

/*-------------------------------------------------------------------------------*/
/* Functions for VCP*/

#define MAX_VCP_SEND_BUF_LEN	1024

static uint8_t	s_au8VCPTempBuf[512] __attribute__((aligned(64)));
static PFN_USBDEV_VCPRecvSignal s_pfnVCPRecvSignel = NULL;

static uint8_t s_au8VCPSendBuf[MAX_VCP_SEND_BUF_LEN]__attribute__((aligned(64)));;
static volatile uint32_t s_u32VCPSendBufIn = 0;
static volatile uint32_t s_u32VCPSendBufOut = 0;
static volatile int32_t s_b32VCPSendTrans = 0;

static void VCPTrans_BulkInHandler(UINT32 u32IntEn, UINT32 u32IntStatus)
{
	int32_t i32Len;
	uint32_t u32DMABuffAddr;

	UINT32 volatile u32MaxPktSize = inp32(EPC_MPS);                /* Get EPC Maximum Packet */

    /* Send data to HOST */
    if(u32IntStatus & IN_TK_IS)                              /* IN Token Interrupt - Host wants to get data from Device */
    {
		if(s_b32VCPSendTrans == 0)
		{
			outp32(EPC_RSP_SC,  PK_END);                     /* Trigger Short Packet */
			return;
		}

		i32Len = s_u32VCPSendBufIn - s_u32VCPSendBufOut;

		if(i32Len > u32MaxPktSize)
			i32Len = u32MaxPktSize;

		if(i32Len == 0){
			s_b32VCPSendTrans = 0;
			outp32(EPC_RSP_SC,  PK_END);                     /* Trigger Short Packet */
			return;
		}

		if((i32Len + inp32(EPC_DATA_CNT)) > u32MaxPktSize)       /* Endpopint Buffer is not enough - Must wait Host read data to release more space */
        {
            printf("IN Endpoint Buffer is Full!!!\n");
			s_b32VCPSendTrans = 0;
            return;
        }

        while(inp32(DMA_CTRL_STS) & DMA_EN);                 /* Wait DMA Ready */

        outp32(DMA_CTRL_STS, 0x10 | VCP_BULK_IN_EP_NUM);     /* Config DMA - Bulk IN, Read data from DRAM to Endpoint Buufer, EP3 */		
		u32DMABuffAddr = (uint32_t)&s_au8VCPSendBuf[0];
		u32DMABuffAddr |= 0x80000000;
		outp32(AHB_DMA_ADDR, (UINT32)u32DMABuffAddr);
        outp32(DMA_CNT, i32Len);                                /* DMA length */
        outp32(DMA_CTRL_STS, inp32(DMA_CTRL_STS)|DMA_EN);    /* Trigger DMA */

        while(inp32(DMA_CTRL_STS) & DMA_EN);                 /* Wait DMA Complete */

        if(inp32(EPC_DATA_CNT) < inp32(EPC_MPS))             /* If data is less than Maximum packet size */
            outp32(EPC_RSP_SC,  PK_END);                     /* Trigger Short Packet */

		s_u32VCPSendBufOut += i32Len;
		
		if(s_u32VCPSendBufIn == s_u32VCPSendBufOut)
			s_b32VCPSendTrans = 0;
	}

}

int32_t VCPTrans_BulkInSend(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
)
{
	/* Don't wait s_b32VCPSendTrans done in here, VCP send will be called in IRQ handler*/ 
	uint32_t u32MaxLen;
	uint32_t u32DMABuffAddr;
	int i;

	if(s_u32VCPSendBufIn == s_u32VCPSendBufOut)
	{
		s_u32VCPSendBufIn = 0;
		s_u32VCPSendBufOut = 0;
	}
	
	u32MaxLen = u32DataBufLen;
	u32DMABuffAddr = (uint32_t)s_au8VCPSendBuf;
	u32DMABuffAddr |= 0x80000000;	
	
	for(i = 0; i < u32MaxLen; i ++)
	{
		outp8(u32DMABuffAddr + s_u32VCPSendBufIn, pu8DataBuf[i]);
		s_u32VCPSendBufIn ++;
	}

	s_b32VCPSendTrans = 1;

	return u32MaxLen;
}

int32_t VCPTrans_BulkInCanSend()
{
	/* Don't wait s_b32VCPSendTrans done in here, VCP send will be called in IRQ handler*/ 
	if(g_u32VCPConnect == 0){
		return -1;
	}

	if(s_u32VCPSendBufIn < MAX_VCP_SEND_BUF_LEN)
		return 1;
	return 0;
}

#define MAX_VCP_RECV_BUF_LEN (512 * 3)
static uint8_t s_au8VCPRecvBuf[MAX_VCP_RECV_BUF_LEN];
static volatile uint32_t s_u32VCPRecvBufIn = 0;
static volatile uint32_t s_u32VCPRecvBufOut = 0;

void VCPTrans_RegisterSingal(
	PFN_USBDEV_VCPRecvSignal pfnSignal
)
{
	s_pfnVCPRecvSignel = pfnSignal;
}

static void VCPTrans_BulkOutHandler(UINT32 u32IntEn, UINT32 u32IntStatus)
{
	uint32_t u32Len = 0;

	int32_t i32BufFreeLen;
	uint32_t u32LimitLen;
	uint32_t u32NewInIdx = s_u32VCPRecvBufIn;
	uint32_t u32OutIdx = s_u32VCPRecvBufOut;
	uint32_t u32DMABuffAddr = (uint32_t)s_au8VCPTempBuf;

	if(u32NewInIdx >= u32OutIdx){
		i32BufFreeLen = MAX_VCP_RECV_BUF_LEN - u32NewInIdx +  u32OutIdx;
		u32LimitLen = MAX_VCP_RECV_BUF_LEN - u32NewInIdx;
	}
	else{
		i32BufFreeLen = u32OutIdx - u32NewInIdx;
		u32LimitLen = i32BufFreeLen;
	}

    /* Receive data from HOST */
    if(u32IntStatus & (DATA_RxED_IS | SHORT_PKT_IS))
    {
		u32DMABuffAddr |= 0x80000000;
		u32Len = inp32(EPD_DATA_CNT) & 0xffff;                  /* Get data from Endpoint FIFO */   
		outp32(DMA_CTRL_STS, VCP_BULK_OUT_EP_NUM);                          /* Config DMA - Bulk OUT, Write data from Endpoint Buufer to DRAM, EP4 */
		outp32(AHB_DMA_ADDR, (UINT32)u32DMABuffAddr);       /* Using word-aligned for DMA */
        outp32(DMA_CNT, u32Len);                                /* DMA length */
        outp32(DMA_CTRL_STS, inp32(DMA_CTRL_STS)|DMA_EN);    /* Trigger DMA */

        while(inp32(DMA_CTRL_STS) & DMA_EN);                 /* Wait DMA Ready */

	}

	if(i32BufFreeLen < u32Len){
		printf("USBD receive buffer size overflow, lost packet \n");
		return;
	}

	if(u32Len)
	{
		if(s_pfnVCPRecvSignel){
			if(s_pfnVCPRecvSignel(s_au8VCPTempBuf, u32Len) == 0){
				return;
			}
		}

		if(u32LimitLen < u32Len){
			memcpy(s_au8VCPRecvBuf + u32NewInIdx, (void *)u32DMABuffAddr, u32LimitLen);		
			memcpy(s_au8VCPRecvBuf, (void *)(u32DMABuffAddr + u32LimitLen), u32Len - u32LimitLen);
		}
		else{
			memcpy(s_au8VCPRecvBuf + u32NewInIdx, (void *)u32DMABuffAddr, u32Len);
		}
	
		u32NewInIdx = (u32NewInIdx + u32Len) % MAX_VCP_RECV_BUF_LEN;
		s_u32VCPRecvBufIn = u32NewInIdx;

	}

}

int32_t VCPTrans_BulkOutCanRecv()
{
	uint32_t u32InIdx;
	uint32_t u32OutIdx;
	int i32RecvLen = 0;

	u32InIdx = s_u32VCPRecvBufIn;
	u32OutIdx = s_u32VCPRecvBufOut;

	if(u32InIdx >= u32OutIdx){
		i32RecvLen = u32InIdx - u32OutIdx;
	}
	else{
		i32RecvLen = MAX_VCP_RECV_BUF_LEN - u32OutIdx +  u32InIdx;
	}
	
	return i32RecvLen;
}

int32_t VCPTrans_BulkOutRecv(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
)
{
	int i32CopyLen = 0;
	uint32_t u32InIdx;
	uint32_t u32NewOutIdx;
	uint32_t u32LimitLen;

	if(pu8DataBuf == NULL)
		return 0;
		
	u32InIdx = s_u32VCPRecvBufIn;
	u32NewOutIdx = s_u32VCPRecvBufOut;

	if(u32InIdx >= u32NewOutIdx){
		i32CopyLen = u32InIdx - u32NewOutIdx;
		u32LimitLen = i32CopyLen;
	}
	else{
		i32CopyLen = MAX_VCP_RECV_BUF_LEN - u32NewOutIdx +  u32InIdx;
		u32LimitLen = MAX_VCP_RECV_BUF_LEN - u32NewOutIdx;
	}

	if(i32CopyLen > u32DataBufLen){
		i32CopyLen = u32DataBufLen;
		u32LimitLen = i32CopyLen;
	}

	if(i32CopyLen){

		if((i32CopyLen + u32NewOutIdx) > MAX_VCP_RECV_BUF_LEN){
			memcpy(pu8DataBuf, s_au8VCPRecvBuf + u32NewOutIdx, u32LimitLen);
			memcpy(pu8DataBuf + u32LimitLen, s_au8VCPRecvBuf, i32CopyLen - u32LimitLen);
		}
		else{
			memcpy(pu8DataBuf, s_au8VCPRecvBuf + u32NewOutIdx, i32CopyLen);
		}

		u32NewOutIdx = (u32NewOutIdx + i32CopyLen) % MAX_VCP_RECV_BUF_LEN;
		s_u32VCPRecvBufOut = u32NewOutIdx;
	}

	return i32CopyLen;
}


/*-------------------------------------------------------------------------------*/
/* Functions for MSC and VCP*/

//MSCVCPTrans_ClassRequset
static void MSCVCPTrans_ClassReqDataIn(void)
{
	switch(_usb_cmd_pkt.bRequest){
		case GET_MAX_LUN:
		{
			if(_usb_cmd_pkt.wValue != 0 || _usb_cmd_pkt.wIndex != 2  || _usb_cmd_pkt.wLength != 1)
			{
				/* Invalid Get MaxLun Command */
				outp32(CEP_IRQ_ENB, (inp32(CEP_IRQ_ENB) | (CEP_SETUP_TK_IE | CEP_SETUP_PK_IE)));
				outp32(CEP_CTRL_STAT, CEP_SEND_STALL);
				break;
			}
			
			uint8_t u8MaxLUN = 0;

#if MICROPY_HW_HAS_SPIFLASH		
			if(u8MaxLUN < DEF_MAX_LUN){
				s_apsLUNStorIf[u8MaxLUN] = (S_STORIF_IF *)&g_STORIF_sSPIFlash;
				u8MaxLUN ++;
			}				
#endif

#if MICROPY_HW_HAS_SDCARD		
			if(u8MaxLUN < DEF_MAX_LUN){
				s_apsLUNStorIf[u8MaxLUN] = (S_STORIF_IF *)&g_STORIF_sSDCard;
				u8MaxLUN ++;
			}				
#endif
			
			s_i32MSCMaxLun = u8MaxLUN;
			s_i32MSCConnectCheck = s_i32MSCMaxLun * 3;

            /* Valid Get MaxLun Command */
            outp8(CEP_DATA_BUF, (u8MaxLUN - 1));
            outp32(IN_TRNSFR_CNT, 1);
			
		}
		break;
		case GET_LINE_CODE:
		{
			int i;
			uint8_t *pu8Temp = (uint8_t *)&s_sLineCoding;
			
			for(i = 0; i < sizeof(STR_VCOM_LINE_CODING); i++){
				outp8(CEP_DATA_BUF, pu8Temp[i]);
			}
			outp32(IN_TRNSFR_CNT, sizeof(STR_VCOM_LINE_CODING)); 
		}
		break;
		default:
		{
			/* Valid GET Command */
			outp32(CEP_IRQ_ENB, (inp32(CEP_IRQ_ENB) | (CEP_SETUP_TK_IE | CEP_SETUP_PK_IE)));
			outp32(CEP_CTRL_STAT, CEP_SEND_STALL);
		}
		break;
	}

}

//MSCVCPTrans_ClassRequset
static void MSCVCPTrans_ClassReqDataOut(void)
{
	switch(_usb_cmd_pkt.bRequest){
		case SET_CONTROL_LINE_STATE:
		{
//			printf("TODO: RTS=%d  DTR=%d\n", (s_u16CtrlSignal >> 1) & 1, s_u16CtrlSignal & 1);
		}
		break;
		case SET_LINE_CODE:
		{
			int i;
			uint8_t *pu8Temp = (uint8_t *)&s_sLineCoding;
			
			for(i=0; i < sizeof(STR_VCOM_LINE_CODING); i++){
				pu8Temp[i] = inp8(CEP_DATA_BUF);
			}
			g_u32VCPConnect = 1;
		}
		break;
		case BULK_ONLY_MASS_STORAGE_RESET:
		{
			if(_usb_cmd_pkt.wValue != 0 || _usb_cmd_pkt.wIndex != 0 || _usb_cmd_pkt.wLength != 0)
			{
				/* Invalid BOT MSC Reset Command */
				outp32(CEP_IRQ_ENB, (inp32(CEP_IRQ_ENB) | (CEP_SETUP_TK_IE | CEP_SETUP_PK_IE)));
				outp32(CEP_CTRL_STAT, CEP_SEND_STALL);
				printf("Wrong MSC Reset\n");
				break;
			}

            outp32(CEP_CTRL_STAT, CEP_ZEROLEN);
            outp32(USB_IRQ_ENB, (RST_IE|SUS_IE|RUM_IE|VBUS_IE));
            outp32(CEP_IRQ_STAT, ~(CEP_SETUP_TK_IS | CEP_SETUP_PK_IS));
            outp32(EPA_RSP_SC, 0);
            outp32(EPA_RSP_SC, BUF_FLUSH);    /* flush fifo */
            outp32(EPA_RSP_SC, TOGGLE);
            outp32(EPB_RSP_SC, 0);
            outp32(EPB_RSP_SC, BUF_FLUSH);    /* flush fifo */
            outp32(EPB_RSP_SC, TOGGLE);
            outp32(CEP_CTRL_STAT,FLUSH);
            printf("MSC Reset\n");
			
		}
		break;
		default:
		{
			/* Invalid SET Command */    
			outp32(CEP_IRQ_ENB, (inp32(CEP_IRQ_ENB) | (CEP_SETUP_TK_IE | CEP_SETUP_PK_IE)));
			outp32(CEP_CTRL_STAT, CEP_SEND_STALL);
		}
		break;
	}

}

/* USB Transfer (DMA configuration) */
static void MSCTrans_SDRAM_USB_Transfer(uint32_t DRAM_Addr ,uint32_t Tran_Size)
{
    if(Tran_Size != 0)
    {
        outp32(USB_IRQ_ENB, (USB_DMA_REQ | USB_RST_STS | USB_SUS_REQ|VBUS_IE));
        outp32(AHB_DMA_ADDR, DRAM_Addr);
        outp32(DMA_CNT, Tran_Size);
        s_psUSBDevInfo->_usbd_DMA_Flag=0;
        if((inp32(DMA_CTRL_STS) & 0x03) == 0x01)		//Checking EP1 bulk-in transfer or not
            s_sMSCDInfo.TxedFlag = 0;
        outp32(DMA_CTRL_STS, inp32(DMA_CTRL_STS)|DMA_EN);	//DMA enable
        
        while(s_psUSBDevInfo->USBModeFlag)
            if((inp32(DMA_CTRL_STS) & DMA_EN) == 0)
                break;
        

        if((inp32(DMA_CTRL_STS) & 0x03) == 0x01)		//Checking EP1 bulk-in transfer or not
        {
            if(Tran_Size % inp32(EPA_MPS) != 0)
                outp32(EPA_RSP_SC, (inp32(EPA_RSP_SC)&0xf7)|0x00000040);    /* packet end */
            while((s_psUSBDevInfo->USBModeFlag)&&((inp32(EPA_DATA_CNT) & 0xFFFF) != 0));
        }
        
    }
}

/* USB means usb host, sdram->host=> bulk in */
static void MSCTrans_SDRAM2USB_Bulk(UINT32 DRAM_Addr ,UINT32 Tran_Size)
{
    UINT32 volatile count=0;

    s_sMSCDInfo._usbd_DMA_Dir = Ep_In;
    
    outp32(DMA_CTRL_STS, 0x11);    /* bulk in, dma read, ep1 */
    if (Tran_Size < s_psUSBDevInfo->usbdMaxPacketSize)
    {
        
        s_sMSCDInfo._usbd_Less_MPS = 1;
        while(s_psUSBDevInfo->USBModeFlag)
        {
            if (inp32(EPA_IRQ_STAT) & 0x02)
            {
                MSCTrans_SDRAM_USB_Transfer(DRAM_Addr, Tran_Size);
                break;
            }
        }
    }
    else if (Tran_Size <= s_sMSCDInfo.USBD_DMA_LEN)
    {
        count = Tran_Size / s_psUSBDevInfo->usbdMaxPacketSize;
        if (count != 0)
        {
            //outp32(EPA_IRQ_ENB, 0x08);
            s_sMSCDInfo._usbd_Less_MPS = 0;
            while(s_psUSBDevInfo->USBModeFlag)
            {
                if (inp32(EPA_IRQ_STAT) & 0x02)
                {
                    MSCTrans_SDRAM_USB_Transfer(DRAM_Addr, s_psUSBDevInfo->usbdMaxPacketSize*count);
                    break;
                }
            }
        }

        if ((Tran_Size % s_psUSBDevInfo->usbdMaxPacketSize) != 0)
        {
            //outp32(EPA_IRQ_ENB, 0x08);
            s_sMSCDInfo._usbd_Less_MPS = 1;
            while(s_psUSBDevInfo->USBModeFlag)
            {
                if (inp32(EPA_IRQ_STAT) & 0x02)
                {
                    MSCTrans_SDRAM_USB_Transfer((DRAM_Addr+count * s_psUSBDevInfo->usbdMaxPacketSize), (Tran_Size % s_psUSBDevInfo->usbdMaxPacketSize));
                    break;
                }
            }
        }
    }
}  

/* USB means usb host, host->sdram => bulk out */
static void MSCTrans_USB2SDRAM_Bulk(UINT32 DRAM_Addr ,UINT32 Tran_Size)
{
    unsigned int volatile count=0;
    int volatile i;

    s_sMSCDInfo._usbd_DMA_Dir = Ep_Out;
    outp32(DMA_CTRL_STS, 0x02);    /* bulk out, dma write, ep2 */

    if (Tran_Size >= s_sMSCDInfo.USBD_DMA_LEN)
    {
        count = Tran_Size / s_sMSCDInfo.USBD_DMA_LEN;
        for (i = 0; i < count; i ++)
            MSCTrans_SDRAM_USB_Transfer(DRAM_Addr + (i * s_sMSCDInfo.USBD_DMA_LEN), s_sMSCDInfo.USBD_DMA_LEN);

        if ((Tran_Size % s_sMSCDInfo.USBD_DMA_LEN) != 0)
                MSCTrans_SDRAM_USB_Transfer(DRAM_Addr + (i * s_sMSCDInfo.USBD_DMA_LEN),(Tran_Size % s_sMSCDInfo.USBD_DMA_LEN));
    }
    else
        MSCTrans_SDRAM_USB_Transfer(DRAM_Addr,Tran_Size);
}

/* Get CBW from USB buffer */
static void MSCTrans_FlushBuf2CBW(void)
{
    uint32_t i,j;

    for (i = 0 ; i < 17 ; i++)
        CBW_In.CBW_Array[i] = inp8(s_sMSCDInfo.Mass_Base_Addr+i);

    j = s_sMSCDInfo.Mass_Base_Addr+20;
    for (i = 17 ; i < 21 ; i++)
        CBW_In.CBW_Array[i] = inp8(j--);

    j = s_sMSCDInfo.Mass_Base_Addr+24;
    for (i = 21 ; i < 25 ; i++)
        CBW_In.CBW_Array[i] = inp8(j--);

    for (i = 25 ; i < 31 ; i++)
        CBW_In.CBW_Array[i] = inp8(s_sMSCDInfo.Mass_Base_Addr+i);
}

/* Set CSW to USB buffer */
static void MSCTrans_FlushCSW2Buf(void)
{
    UINT8 i;

    for (i = 0 ; i < 13 ; i++)
        CSW_In.CSW_Array[i] = 0x00;

    CSW_In.CSWD.dCSWSignature[0] = 0x55;
    CSW_In.CSWD.dCSWSignature[1] = 0x53;
    CSW_In.CSWD.dCSWSignature[2] = 0x42;
    CSW_In.CSWD.dCSWSignature[3] = 0x53;

    for (i = 0 ; i < 4 ; i++)
        CSW_In.CSWD.dCSWTag[i] = CBW_In.CBWD.dCBWTag[i];

    outp8(s_sMSCDInfo.Mass_Base_Addr + 12, 0x00);	//CSW.status byte

    if(s_sMSCDInfo.MB_Invalid_Cmd_Flg == 1)
    {
        if(s_sMSCDInfo.dwResidueLen)
            s_sMSCDInfo.dwUsedBytes = CBW_In.CBWD.dCBWDataTferLh.c32- s_sMSCDInfo.dwResidueLen;
        else
            s_sMSCDInfo.dwUsedBytes = CBW_In.CBWD.dCBWDataTferLh.c32 - s_sMSCDInfo.dwUsedBytes;

        CSW_In.CSWD.dCSWDataResidue.g[0] = (UINT8)(s_sMSCDInfo.dwUsedBytes);
        CSW_In.CSWD.dCSWDataResidue.g[1] = (UINT8)(s_sMSCDInfo.dwUsedBytes >> 8 );
        CSW_In.CSWD.dCSWDataResidue.g[2] = (UINT8)(s_sMSCDInfo.dwUsedBytes >> 16);
        CSW_In.CSWD.dCSWDataResidue.g[3] = (UINT8)(s_sMSCDInfo.dwUsedBytes >> 24);
        outp8(s_sMSCDInfo.Mass_Base_Addr + 12, 0x01);	//CSW.status byte
    }

    CSW_In.CSWD.bCSWStatus = s_sMSCDInfo.preventflag;

    for (i = 0 ; i < 13 ; i++)
        outp8(s_sMSCDInfo.Mass_Base_Addr+i,CSW_In.CSW_Array[i]); //Copy CSW.dCSWSignature, CSW.dCSWTag and CSW.dCSWDataResidue to buffer 

	MSCTrans_SDRAM2USB_Bulk(s_sMSCDInfo.Mass_Base_Addr, 0x0d); //CSW_In total 13 bytes
	s_sMSCDInfo.Bulk_First_Flag=0;
	s_sMSCDInfo.MB_Invalid_Cmd_Flg =0;
	s_sMSCDInfo.dwUsedBytes = 0;
	if((inp32(EPB_DATA_CNT) & 0xFFFF) == 0)
	s_sMSCDInfo.dwCBW_flag = 0;
}

static void MSC_ReadMedia(uint32_t u32Addr, uint32_t u32Size, uint8_t *pu8Buffer)
{
	uint32_t u32SectorAlignAddr;
	uint32_t u32Offset;
	uint8_t *pu8TempBuf = NULL;
	int32_t i32EachReadLen;
 
	S_STORIF_IF *psStorIF = s_apsLUNStorIf[CBW_In.CBWD.bCBWLUN];	
	S_STORIF_INFO sStorInfo;

//	printf("MSC_ReadMedia enter address %x , size %d \n", u32Addr, u32Size);
	psStorIF->pfnGetInfo(&sStorInfo, NULL);
	u32SectorAlignAddr = u32Addr & (~(sStorInfo.u32SectorSize - 1));

	/* read none sector-aligned data */
	if(u32SectorAlignAddr != u32Addr){
		/* Get the sector offset*/
		u32Offset = (u32Addr & (sStorInfo.u32SectorSize - 1));

		pu8TempBuf = malloc(sStorInfo.u32SectorSize);
		if(pu8TempBuf == NULL)
			return;

		psStorIF->pfnReadSector(pu8TempBuf, u32SectorAlignAddr / sStorInfo.u32SectorSize, 1, NULL);
		i32EachReadLen = sStorInfo.u32SectorSize - u32Offset;

		if(i32EachReadLen > u32Size)
			i32EachReadLen = u32Size;
		
		memcpy(pu8Buffer, pu8TempBuf + u32Offset, i32EachReadLen);

		u32Addr += i32EachReadLen;
		pu8Buffer += i32EachReadLen;
		u32Size -= i32EachReadLen;
	} 

	/*read sector aligned data */
	i32EachReadLen = u32Size & (~(sStorInfo.u32SectorSize - 1));

	if(i32EachReadLen > 0){
		psStorIF->pfnReadSector(pu8Buffer, u32Addr / sStorInfo.u32SectorSize, i32EachReadLen / sStorInfo.u32SectorSize, NULL);
		
		u32Addr += i32EachReadLen;
		pu8Buffer += i32EachReadLen;
		u32Size -= i32EachReadLen;
	}

	/*read remain data */
	if(u32Size > 0){
		if(pu8TempBuf == NULL){
			pu8TempBuf = malloc(sStorInfo.u32SectorSize);
			if(pu8TempBuf == NULL)
				return;
		}

		psStorIF->pfnReadSector(pu8TempBuf, u32Addr / sStorInfo.u32SectorSize, 1, NULL);
		memcpy(pu8Buffer, pu8TempBuf, u32Size);
	}
	
	if(pu8TempBuf)
		free(pu8TempBuf);

//	printf("MSC_ReadMedia leave \n");
}

static void MSC_WriteMedia(uint32_t u32Addr, uint32_t u32Size, uint8_t *pu8Buffer)
{
	uint32_t u32SectorAlignAddr;
	uint32_t u32Offset;
	uint8_t *pu8TempBuf = NULL;
	int32_t i32EachWriteLen;

	S_STORIF_IF *psStorIF = s_apsLUNStorIf[CBW_In.CBWD.bCBWLUN];	
	S_STORIF_INFO sStorInfo;
	
	psStorIF->pfnGetInfo(&sStorInfo, NULL);
	u32SectorAlignAddr = u32Addr & (~(sStorInfo.u32SectorSize - 1));
//	printf("MSC_WriteMedia enter %x , size %d \n", u32Addr, u32Size);

	/* write none sector-aligned data */
	if(u32SectorAlignAddr != u32Addr){
		/* Get the sector offset*/
		u32Offset = (u32Addr & (sStorInfo.u32SectorSize - 1));

		pu8TempBuf = malloc(sStorInfo.u32SectorSize);
		if(pu8TempBuf == NULL)
			return;

		psStorIF->pfnReadSector(pu8TempBuf, u32SectorAlignAddr / sStorInfo.u32SectorSize, 1, NULL);
		i32EachWriteLen = sStorInfo.u32SectorSize - u32Offset;

		if(i32EachWriteLen > u32Size)
			i32EachWriteLen = u32Size;

		memcpy(pu8TempBuf + u32Offset, pu8Buffer, i32EachWriteLen);
		psStorIF->pfnWriteSector(pu8TempBuf, u32SectorAlignAddr / sStorInfo.u32SectorSize, 1, NULL);
		
		u32Addr += i32EachWriteLen;
		pu8Buffer += i32EachWriteLen;
		u32Size -= i32EachWriteLen;
	} 

	/*write sector aligned data */
	i32EachWriteLen = u32Size & (~(sStorInfo.u32SectorSize - 1));

	if(i32EachWriteLen > 0){
		psStorIF->pfnWriteSector(pu8Buffer, u32Addr / sStorInfo.u32SectorSize, i32EachWriteLen / sStorInfo.u32SectorSize, NULL);
		
		u32Addr += i32EachWriteLen;
		pu8Buffer += i32EachWriteLen;
		u32Size -= i32EachWriteLen;
	}
	
	/*write remain data */
	if(u32Size > 0){
		if(pu8TempBuf == NULL){
			pu8TempBuf = malloc(sStorInfo.u32SectorSize);
			if(pu8TempBuf == NULL)
				return;
		}

		psStorIF->pfnReadSector(pu8TempBuf, u32Addr / sStorInfo.u32SectorSize, 1, NULL);
		memcpy(pu8TempBuf, pu8Buffer, u32Size);
		psStorIF->pfnWriteSector(pu8TempBuf, u32Addr / sStorInfo.u32SectorSize, 1, NULL);
	}
	
//	printf("MSC_WriteMedia leave \n");

	if(pu8TempBuf)
		free(pu8TempBuf);
}


/* Write(10) & Write(12) */
static void MSCTrans_Write10_Command(void)
{
	uint32_t u32Len;
	uint32_t volatile u32SectorCount = 0, u32Loop = 0 , i, u32SectorOffset = 0, u32SectorStart;

	u32Len = CBW_In.CBWD.dCBWDataTferLh.c32;
	u32SectorCount = u32Len / s_sMSCDInfo.SizePerSector;
	u32SectorStart = CBW_In.CBWD.LBA.d32;

    u32Loop = u32SectorCount / MSC_BUFFER_SECTOR;

	for(i = 0; i < u32Loop; i ++)
	{
		MSCTrans_USB2SDRAM_Bulk(s_sMSCDInfo.Storage_Base_Addr, MSC_BUFFER_SECTOR * MSC_SECTOR_SIZE);
		MSC_WriteMedia((u32SectorStart + u32SectorOffset) * MSC_SECTOR_SIZE, MSC_BUFFER_SECTOR * MSC_SECTOR_SIZE, (uint8_t *)s_sMSCDInfo.Storage_Base_Addr);
		u32SectorOffset += MSC_BUFFER_SECTOR;
		u32SectorCount -= MSC_BUFFER_SECTOR;
	}

    if(u32SectorCount % MSC_BUFFER_SECTOR)
    {
		MSCTrans_USB2SDRAM_Bulk(s_sMSCDInfo.Storage_Base_Addr, u32SectorCount * MSC_SECTOR_SIZE);
		MSC_WriteMedia((u32SectorStart + u32SectorOffset) * MSC_SECTOR_SIZE, u32SectorCount * MSC_SECTOR_SIZE, (uint8_t *)s_sMSCDInfo.Storage_Base_Addr);
	}
}

/* Read(10) & Read(12) */
static void MSCTrans_Read10_Command(void)
{
	uint32_t u32Len;
	uint32_t volatile u32SectorCount = 0, u32Loop = 0 , i, u32SectorOffset = 0, u32SectorStart;

	u32Len = CBW_In.CBWD.dCBWDataTferLh.c32;
	u32SectorCount = u32Len / s_sMSCDInfo.SizePerSector;
	u32SectorStart = CBW_In.CBWD.LBA.d32;

    u32Loop = u32SectorCount / MSC_BUFFER_SECTOR;

	for(i = 0; i < u32Loop; i ++)
	{
		MSC_ReadMedia((u32SectorStart + u32SectorOffset) * MSC_SECTOR_SIZE, MSC_BUFFER_SECTOR * MSC_SECTOR_SIZE, (uint8_t *)s_sMSCDInfo.Storage_Base_Addr);
		MSCTrans_SDRAM2USB_Bulk(s_sMSCDInfo.Storage_Base_Addr, MSC_BUFFER_SECTOR * MSC_SECTOR_SIZE);
		u32SectorOffset += MSC_BUFFER_SECTOR;
		u32SectorCount -= MSC_BUFFER_SECTOR;
	}

    if(u32SectorCount % MSC_BUFFER_SECTOR)
    {
		MSC_ReadMedia((u32SectorStart + u32SectorOffset) * MSC_SECTOR_SIZE, u32SectorCount * MSC_SECTOR_SIZE, (uint8_t *)s_sMSCDInfo.Storage_Base_Addr);
		MSCTrans_SDRAM2USB_Bulk(s_sMSCDInfo.Storage_Base_Addr, u32SectorCount * MSC_SECTOR_SIZE);
	}
}

/* Inquiry */
static void MSCTrans_Inquiry_Command(void)
{
    int i;
    uint32_t u32WD;
    
    u32WD = CBW_In.CBWD.dCBWDataTferLh.c32;

    if(u32WD > sizeof(s_au8InquiryID))
        u32WD = sizeof(s_au8InquiryID);

    for (i = 0 ; i < u32WD; i++)
        outp8(s_sMSCDInfo.Mass_Base_Addr+i, s_au8InquiryID[i]);

	MSCTrans_SDRAM2USB_Bulk(s_sMSCDInfo.Mass_Base_Addr, u32WD);

}

/* Read Format Capacity */
static void MSCTrans_ReadFormatCapacity_Command(void)
{
	S_STORIF_IF *psStorIF = NULL;	
	S_STORIF_INFO sStorInfo;
	uint32_t u32TotalSector;
    int i;
    uint32_t u32RD;
    uint32_t u32TmpVal;
	
	psStorIF = s_apsLUNStorIf[CBW_In.CBWD.bCBWLUN];	

	psStorIF->pfnGetInfo(&sStorInfo, NULL);
	u32TotalSector = sStorInfo.u32DiskSize * (1024 / MSC_SECTOR_SIZE);

    u32TmpVal = s_sMSCDInfo.Mass_Base_Addr;
    for (i = 0 ; i < 36 ; i++)
        outp8(u32TmpVal + i, 0x00);
    outp8(u32TmpVal + 3, 0x10);

	outp8(u32TmpVal+4, *((UINT8 *)&u32TotalSector+3));
	outp8(u32TmpVal+5, *((UINT8 *)&u32TotalSector+2));
	outp8(u32TmpVal+6, *((UINT8 *)&u32TotalSector+1));
	outp8(u32TmpVal+7, *((UINT8 *)&u32TotalSector+0));
	outp8(u32TmpVal+8, 0x02);
	outp8(u32TmpVal+10, 0x02);
	outp8(u32TmpVal+12, *((UINT8 *)&u32TotalSector+3));
	outp8(u32TmpVal+13, *((UINT8 *)&u32TotalSector+2));
	outp8(u32TmpVal+14, *((UINT8 *)&u32TotalSector+1));
	outp8(u32TmpVal+15, *((UINT8 *)&u32TotalSector+0));
	outp8(u32TmpVal+18, 0x02);

    u32RD = CBW_In.CBWD.dCBWDataTferLh.c32;
    MSCTrans_SDRAM2USB_Bulk(s_sMSCDInfo.Mass_Base_Addr, u32RD);
}

/* Read Format Capacity */
static void MSCTrans_ReadCapacity_Command(void)
{
	S_STORIF_IF *psStorIF = s_apsLUNStorIf[CBW_In.CBWD.bCBWLUN];	
	S_STORIF_INFO sStorInfo;
	uint32_t u32TotalSector;
    int i;
    uint32_t u32RD;
    uint32_t u32TmpVal;

	psStorIF->pfnGetInfo(&sStorInfo, NULL);
	u32TotalSector = sStorInfo.u32DiskSize * (1024 / MSC_SECTOR_SIZE);
	u32TotalSector = u32TotalSector - 1;

    u32TmpVal = s_sMSCDInfo.Mass_Base_Addr;
    for (i = 0 ; i < 36 ; i++)
        outp8(u32TmpVal + i, 0x00);


    outp8(u32TmpVal, *((UINT8 *)&u32TotalSector+3));
    outp8(u32TmpVal+1, *((UINT8 *)&u32TotalSector+2));
    outp8(u32TmpVal+2, *((UINT8 *)&u32TotalSector+1));
    outp8(u32TmpVal+3, *((UINT8 *)&u32TotalSector+0));
    outp8(u32TmpVal+6, 0x02);

    u32RD = CBW_In.CBWD.dCBWDataTferLh.c32;
    MSCTrans_SDRAM2USB_Bulk(s_sMSCDInfo.Mass_Base_Addr, u32RD);
}

/* Sense10 */
static void MSCTrans_Sense10_Command(void)
{
    uint8_t i,j;
    uint8_t NumHead,NumSector;
    uint16_t NumCyl=0;

	S_STORIF_IF *psStorIF = s_apsLUNStorIf[CBW_In.CBWD.bCBWLUN];	
	S_STORIF_INFO sStorInfo;
	uint32_t u32TotalSector;
	
	psStorIF->pfnGetInfo(&sStorInfo, NULL);
	u32TotalSector = sStorInfo.u32DiskSize * (1024 / MSC_SECTOR_SIZE);

	for (i = 0 ; i < 8 ; i++)
		outp8(s_sMSCDInfo.Mass_Base_Addr + i, 0x00);

    switch (CBW_In.CBWD.LBA.d[0])
    {
		case 0x01:
		{
			outp8(s_sMSCDInfo.Mass_Base_Addr,19);
			i = 8;
			for (j = 0; j < 12; j ++)
			{
				outp8(s_sMSCDInfo.Mass_Base_Addr + i, s_au8ModePage_01[j]);
				i++;
			}
		}
		break;
		case 0x05:
		{
			outp8(s_sMSCDInfo.Mass_Base_Addr,39);
            i = 8;
            for (j = 0; j < 32; j ++)
            {
                outp8(s_sMSCDInfo.Mass_Base_Addr + i, s_au8ModePage_05[j]);
                i++;
            }

			NumHead = 2;
			NumSector = 64;
			NumCyl = u32TotalSector / 128;

			outp8(s_sMSCDInfo.Mass_Base_Addr+12, NumHead);
			outp8(s_sMSCDInfo.Mass_Base_Addr+13, NumSector);
			outp8(s_sMSCDInfo.Mass_Base_Addr+16, (UINT8)(NumCyl >> 8));
			outp8(s_sMSCDInfo.Mass_Base_Addr+17, (UINT8)(NumCyl & 0x00ff));
		}
		break;
		case 0x1B:
		{
			outp8(s_sMSCDInfo.Mass_Base_Addr,19);
			i = 8;
			for (j = 0; j < 12; j ++)
			{
				outp8(s_sMSCDInfo.Mass_Base_Addr+i, s_au8ModePage_1B[j]);
				i++;
			}
		}
		break;
		case 0x1C:
		{
			outp8(s_sMSCDInfo.Mass_Base_Addr,15);
			i = 8;
			for (j = 0; j < 8; j++)
			{
				outp8(s_sMSCDInfo.Mass_Base_Addr+i, s_au8ModePage_1C[j]);
				i++;
			}
		}
		break;
		case 0x3F:
		{
			outp8(s_sMSCDInfo.Mass_Base_Addr, 0x47);
			i = 8;
			for (j = 0; j<12; j++)
			{
				outp8(s_sMSCDInfo.Mass_Base_Addr+i, s_au8ModePage_01[j]);
				i++;
			}

			for (j = 0; j<32; j++)
			{
				outp8(s_sMSCDInfo.Mass_Base_Addr+i, s_au8ModePage_05[j]);
				i++;
			}

			for (j = 0; j<12; j++)
			{
				outp8(s_sMSCDInfo.Mass_Base_Addr+i, s_au8ModePage_1B[j]);
				i++;
			}

			for (j = 0; j<8; j++)
			{
				outp8(s_sMSCDInfo.Mass_Base_Addr+i, s_au8ModePage_1C[j]);
				i++;
			}

			NumHead = 2;
			NumSector = 64;
			NumCyl = u32TotalSector / 128;

            outp8(s_sMSCDInfo.Mass_Base_Addr+24, NumHead);
            outp8(s_sMSCDInfo.Mass_Base_Addr+25, NumSector);
            outp8(s_sMSCDInfo.Mass_Base_Addr+28, (UINT8)(NumCyl >> 8));
            outp8(s_sMSCDInfo.Mass_Base_Addr+29, (UINT8)(NumCyl & 0x00ff));
		}
		break;
		default:
		{
             /* INVALID FIELD IN COMMAND PACKET */
            s_sMSCDInfo.SenseKey = 0x05;
            s_sMSCDInfo.ASC = 0x24;
            s_sMSCDInfo.ASCQ = 0;
		}
	}

    MSCTrans_SDRAM2USB_Bulk(s_sMSCDInfo.Mass_Base_Addr, CBW_In.CBWD.dCBWDataTferLh.c32);
}

/* Sense6 */
static void MSCTrans_Sense6_Command(void)
{
	outp8(s_sMSCDInfo.Mass_Base_Addr + 0, 0x03);
	outp8(s_sMSCDInfo.Mass_Base_Addr + 1, 0x00);
	outp8(s_sMSCDInfo.Mass_Base_Addr + 2, 0x00);
	outp8(s_sMSCDInfo.Mass_Base_Addr + 3, 0x00);
	MSCTrans_SDRAM2USB_Bulk(s_sMSCDInfo.Mass_Base_Addr, CBW_In.CBWD.dCBWDataTferLh.c32);
}

/* Test Unit Ready_ */
static void MSCTrans_TestUnitReady_Command(void)
{
	S_STORIF_IF *psStorIF = s_apsLUNStorIf[CBW_In.CBWD.bCBWLUN];	
	if((!psStorIF->pfnDetect(NULL)) || (g_u8MSCRemove))
	{
		s_sMSCDInfo.SenseKey = 0x02;
		s_sMSCDInfo.ASC = 0x3a;
		s_sMSCDInfo.ASCQ = 0;
	}
}

/* Request Sense */
static void MSCTrans_RequestSense_Command(void)
{
    UINT8 i;

    for (i = 0 ; i < 18 ; i++)
        outp8(s_sMSCDInfo.Mass_Base_Addr + i, 0x00);
        
    if (s_sMSCDInfo.preventflag == 1)
    {
        s_sMSCDInfo.preventflag=0;
        outp8(s_sMSCDInfo.Mass_Base_Addr, 0x70);
    }
    else
        outp8(s_sMSCDInfo.Mass_Base_Addr, 0xf0);

    outp8(s_sMSCDInfo.Mass_Base_Addr+2, s_sMSCDInfo.SenseKey);
    outp8(s_sMSCDInfo.Mass_Base_Addr+7,0x0a);
    outp8(s_sMSCDInfo.Mass_Base_Addr+12, s_sMSCDInfo.ASC);
    outp8(s_sMSCDInfo.Mass_Base_Addr+13, s_sMSCDInfo.ASCQ);
    MSCTrans_SDRAM2USB_Bulk(s_sMSCDInfo.Mass_Base_Addr, CBW_In.CBWD.dCBWDataTferLh.c32);
   
    /* NO SENSE */
    s_sMSCDInfo.SenseKey = 0;
    s_sMSCDInfo.ASC= 0;
    s_sMSCDInfo.ASCQ = 0;
}

/* Select10 */
static void MSCTrans_Select10_Command(void)
{
    MSCTrans_SDRAM2USB_Bulk(s_sMSCDInfo.Mass_Base_Addr, CBW_In.CBWD.dCBWDataTferLh.c32);
}

void MSCTrans_Process(void)
{
	uint32_t HCount, DCount,CBWCount;

	if (s_psUSBDevInfo->USBModeFlag)
	{
		if(s_sMSCDInfo.Bulk_First_Flag==0)
		{
			if(!s_sMSCDInfo.dwCBW_flag)
				return;
			if(!s_psUSBDevInfo->USBModeFlag)
				return;

            /* CBW Check */
            CBWCount = inp32(EPB_DATA_CNT) & 0xFFFF;
            MSCTrans_USB2SDRAM_Bulk(s_sMSCDInfo.Mass_Base_Addr, CBWCount);
			MSCTrans_FlushBuf2CBW();

            if((CBWCount != 31)  || CBW_In.CBWD.dCBWSignature != 0x43425355)
            {
                if(s_psUSBDevInfo->USBModeFlag)
                {
                    /* Invalid CBW */
                    outp32(EPA_RSP_SC, EP_HALT);
                    s_psUSBDevInfo->_usbd_haltep = 1;
                    s_sMSCDInfo.preventflag = 1;
                    s_sMSCDInfo.bulkonlycmd = 0;
                }
                else
                    return;
            }
            else
            {
                /* Valid CBW */
                s_sMSCDInfo.Bulk_First_Flag=1;
                s_sMSCDInfo.bulkonlycmd = 1;
                s_sMSCDInfo.dwCBW_flag = 0;
                s_sMSCDInfo.dwResidueLen = 0;
            }

		}
		else
		{
            if(s_sMSCDInfo.preventflag == 1 && s_psUSBDevInfo->_usbd_haltep != 1 && s_psUSBDevInfo->_usbd_haltep != 2)
            {
                MSCTrans_FlushCSW2Buf();
            }
		}
	}

    if ((s_psUSBDevInfo->USBModeFlag) && (s_sMSCDInfo.bulkonlycmd == 1))
    {
        HCount = CBW_In.CBWD.dCBWDataTferLh.c32;

//		printf("CBW_In.CBWD.OP_Code %d \n", CBW_In.CBWD.OP_Code);
		switch (CBW_In.CBWD.OP_Code)
		{
			case UFI_WRITE_10:
			case UFI_WRITE_12:
			{
				if(CBW_In.CBWD.OP_Code == UFI_WRITE_10)
					DCount = (CBW_In.CBWD.CBWCB_In.f32) >> 8;
				else
					DCount = (CBW_In.CBWD.CBWCB_In.f32);

				DCount = DCount * s_sMSCDInfo.SizePerSector;

				if(CBW_In.CBWD.bmCBWFlags == 0x00) /* OUT */
				{
					/* Ho == Do (Case 12) */
					if(HCount == DCount)
					{
						MSCTrans_Write10_Command();
						MSCTrans_FlushCSW2Buf();
					}
					/* Hn < Do (Case 3) || Ho < Do (Case 13) */
					else if(HCount < DCount)
					{
						if(HCount)    /* Ho < Do (Case 13) */
							MSCTrans_Write10_Command();

						s_sMSCDInfo.preventflag = 1;
						MSCTrans_FlushCSW2Buf();
					}
					/* Ho > Do (Case 11) */
					else if(HCount > DCount)
					{
						s_sMSCDInfo.dwResidueLen = DCount;
						MSCTrans_Write10_Command();
						s_sMSCDInfo.MB_Invalid_Cmd_Flg = 1;
						s_sMSCDInfo.preventflag = 1;
						MSCTrans_FlushCSW2Buf();
					}
				}
				/* Hi <> Do (Case 8) */
				else
				{
					s_sMSCDInfo.MB_Invalid_Cmd_Flg = 1;
					outp32(EPA_RSP_SC, EP_HALT);
					s_psUSBDevInfo->_usbd_haltep = 1;
					s_sMSCDInfo.preventflag = 1;
				}
			}
			break;
			case UFI_READ_10:
			case UFI_READ_12:
			{

				s_i32MSCConnectCheck = s_i32MSCMaxLun * 3;

				if(CBW_In.CBWD.OP_Code == UFI_READ_10)
					DCount = (CBW_In.CBWD.CBWCB_In.f32) >> 8;
				else
					DCount = (CBW_In.CBWD.CBWCB_In.f32);

				DCount = DCount * s_sMSCDInfo.SizePerSector;

				if(CBW_In.CBWD.bmCBWFlags == 0x80)    /* IN */
				{
					if(HCount == DCount)    /* Hi == Di (Case 6) */
					{
						MSCTrans_Read10_Command();
						MSCTrans_FlushCSW2Buf();
					}
					/* Hn < Di (Case 2) || Hi < Di (Case 7) */    
					else if(HCount < DCount)
					{
						if(HCount) /* Hi < Di (Case 7) */
						{
							outp32(EPA_RSP_SC, EP_HALT);
							s_psUSBDevInfo->_usbd_haltep = 1;
							s_sMSCDInfo.preventflag = 1;
						}
						else    /* Hn < Di (Case 2) */
						{
							s_sMSCDInfo.preventflag = 1;
							MSCTrans_FlushCSW2Buf();
						}
					}
					/* Hi > Dn (Case 4) || Hi > Di (Case 5) */
					else if(HCount > DCount)
					{
						outp32(EPA_RSP_SC, EP_HALT);
						s_psUSBDevInfo->_usbd_haltep = 1;
						s_sMSCDInfo.preventflag = 1;
					}
				}
				else
				{
					/* Ho <> Di (Case 10) */
					s_sMSCDInfo.MB_Invalid_Cmd_Flg = 1;
					outp32(EPB_RSP_SC, EP_HALT);
					s_psUSBDevInfo->_usbd_haltep = 2;
					s_sMSCDInfo.preventflag = 1;
				}
			}
			break;
			case UFI_INQUIRY:
			{
				MSCTrans_Inquiry_Command();
				MSCTrans_FlushCSW2Buf();
			}
			break;
			case UFI_READ_FORMAT_CAPACITY:
			{
				MSCTrans_ReadFormatCapacity_Command();
				MSCTrans_FlushCSW2Buf();
			}
			break;
			case UFI_READ_CAPACITY:
			{
				MSCTrans_ReadCapacity_Command();
				MSCTrans_FlushCSW2Buf();
			}
			break;
			case UFI_MODE_SENSE_10:
			{
				MSCTrans_Sense10_Command();
				MSCTrans_FlushCSW2Buf();
			}
			break;
			case UFI_MODE_SENSE_6:
			{
				MSCTrans_Sense6_Command();
				MSCTrans_FlushCSW2Buf();
			}
			break;
			case UFI_TEST_UNIT_READY:
			{
				if(s_i32MSCConnectCheck > 0)
					s_i32MSCConnectCheck --;
				else
					g_u32MSCConnect = 1;

				if(HCount != 0)
				{
					if(CBW_In.CBWD.bmCBWFlags == 0x00)    /* Ho > Dn (Case 9) */
					{
						outp32(EPB_RSP_SC, EP_HALT);
						s_psUSBDevInfo->_usbd_haltep = 2;
						s_sMSCDInfo.preventflag = 1;
					}
				}
				else
				{
					MSCTrans_TestUnitReady_Command();
					MSCTrans_FlushCSW2Buf();
				}
			}
			break;
			case UFI_PREVENT_ALLOW_MEDIUM_REMOVAL:
			{
				if (CBW_In.CBW_Array[19] == 1)
				{
					s_sMSCDInfo.preventflag = 1;
					s_sMSCDInfo.SenseKey = 0x05;
					s_sMSCDInfo.ASC = 0x24;
					s_sMSCDInfo.ASCQ = 0x00;
				}
				else
				{
					s_sMSCDInfo.preventflag = 0;
				}
				MSCTrans_FlushCSW2Buf();
			}
			break;
			case UFI_REQUEST_SENSE:
			{
				MSCTrans_RequestSense_Command();
				MSCTrans_FlushCSW2Buf();
			}
			break;
			case UFI_VERIFY_10:
			{
				MSCTrans_FlushCSW2Buf();
			}
			break;
			case UFI_START_STOP:
			{
                if ((CBW_In.CBWD.LBA.d[2] & 0x03) == 0x2)
                {
					g_u8MSCRemove = 1;
				}
				MSCTrans_FlushCSW2Buf();
			}
			break;
			case UFI_MODE_SELECT_10:
			{
				MSCTrans_Select10_Command();
				MSCTrans_FlushCSW2Buf();
			}
			break;
			default:
			{
				s_sMSCDInfo.dwUsedBytes = 0;
				/* INVALID FIELD IN COMMAND PACKET */
				s_sMSCDInfo.SenseKey = 0x05;
				s_sMSCDInfo.ASC= 0x24;
				s_sMSCDInfo.ASCQ = 0x00;
				s_sMSCDInfo.MB_Invalid_Cmd_Flg = 1;
				MSCTrans_FlushCSW2Buf();
			}
		}
        s_sMSCDInfo.bulkonlycmd = 0;
	}
}

static void MSCTrans_BulkInHandler(UINT32 u32IntEn, UINT32 u32IntStatus)
{
   /* Send data to HOST (CBW/Data) */
    if(u32IntStatus & DATA_TxED_IS)
        s_sMSCDInfo.TxedFlag = 1;
}

static void MSCTrans_BulkOutHandler(UINT32 u32IntEn, UINT32 u32IntStatus)
{
    /* Receive data from HOST (CBW/Data) */
    if(u32IntStatus & DATA_RxED_IS)
        s_sMSCDInfo.dwCBW_flag = 1;
}

/* USB Reset Callback function */
static void MSCVCPTrans_Reset(void)
{
    s_sMSCDInfo.Bulk_First_Flag=0;
    g_u8MSCRemove = 0;
	g_u32VCPConnect = 0;
	g_u32MSCConnect = 0;
	g_u32DataBusConnect = 1;
}


static void MSCVCPTrans_HighSpeedInit()
{
    s_psUSBDevInfo->usbdMaxPacketSize = 0x200;
    outp32(EPA_MPS, 0x00000200);                /* mps 512 */
    while(inp32(EPA_MPS) != 0x00000200);        /* mps 512 */

    /* MSC bulk in */
    outp32(EPA_IRQ_ENB, 0x00000008);            /* tx transmitted */
    outp32(EPA_RSP_SC, 0x00000000);     /* auto validation */
    outp32(EPA_MPS, 0x00000200);        /* mps 512 */
    outp32(EPA_CFG, 0x0000001b);        /* Bulk in, ep no 1 */
    outp32(EPA_START_ADDR, 0x00000100);
    outp32(EPA_END_ADDR, 0x000002ff);

    /* MSC bulk out */
    outp32(EPB_IRQ_ENB, 0x00000010);    /* data pkt received */
    outp32(EPB_RSP_SC, 0x00000000);     /* auto validation */
    outp32(EPB_MPS, 0x00000200);        /* mps 512 */
    outp32(EPB_CFG, 0x00000023);        /* Bulk out ep no 2 */
    outp32(EPB_START_ADDR, 0x00000300);
    outp32(EPB_END_ADDR, 0x000004ff);

    /* VCP bulk in */
//    outp32(EPC_IRQ_ENB, 0x00000008);            /* tx transmitted */
    outp32(EPC_IRQ_ENB, IN_TK_IE);
    outp32(EPC_RSP_SC, 0x00000000);             /* auto validation */
    outp32(EPC_MPS, 0x00000200);                /* mps 512 */
    outp32(EPC_CFG, 0x0000003b);                /* bulk in ep no 3 */
    outp32(EPC_START_ADDR, 0x00000500);
    outp32(EPC_END_ADDR, 0x000006ff);

    /* VCP bulk out */
//    outp32(EPD_IRQ_ENB, 0x00000010);            /* data pkt received */
    outp32(EPD_IRQ_ENB, inp32(EPD_IRQ_ENB) | O_SHORT_PKT_IE | DATA_RxED_IE);    
    outp32(EPD_RSP_SC, 0x00000000);             /* auto validation */
    outp32(EPD_MPS, 0x00000200);                /* mps 512 */
    outp32(EPD_CFG, 0x00000043);                /* bulk out ep no 4 */
    outp32(EPD_START_ADDR, 0x00000700);
    outp32(EPD_END_ADDR, 0x000008FF);
}


static void MSCVCPTrans_FullSpeedInit()
{
    s_psUSBDevInfo->usbdMaxPacketSize = 0x40;

    /* MSC bulk in */
    outp32(EPA_IRQ_ENB, 0x00000008);            /* tx transmitted */
    outp32(EPA_RSP_SC, 0x00000000);     /* auto validation */
    outp32(EPA_MPS, 0x00000040);        /* mps 64 */
    outp32(EPA_CFG, 0x0000001b);        /* Bulk in, ep no 1 */
    outp32(EPA_START_ADDR, 0x00000100);
    outp32(EPA_END_ADDR, 0x0000017f);

    /* MSC bulk out */
    outp32(EPB_IRQ_ENB, 0x00000010);    /* data pkt received */
    outp32(EPB_RSP_SC, 0x00000000);     /* auto validation */
    outp32(EPB_MPS, 0x00000040);        /* mps 64 */
    outp32(EPB_CFG, 0x00000023);        /* Bulk out ep no 2 */
    outp32(EPB_START_ADDR, 0x00000300);
    outp32(EPB_END_ADDR, 0x0000037f);

    /* VCP bulk in */
//    outp32(EPC_IRQ_ENB, 0x00000008);            /* tx transmitted */
    outp32(EPC_IRQ_ENB, IN_TK_IE);
    outp32(EPC_RSP_SC, 0x00000000);             /* auto validation */
    outp32(EPC_MPS, 0x00000040);                /* mps 64 */
    outp32(EPC_CFG, 0x0000003b);                /* bulk in ep no 3 */
    outp32(EPC_START_ADDR, 0x00000500);
    outp32(EPC_END_ADDR, 0x0000057f);

    /* VCP bulk out */
//    outp32(EPD_IRQ_ENB, 0x00000010);            /* data pkt received */
    outp32(EPD_IRQ_ENB, inp32(EPD_IRQ_ENB) | O_SHORT_PKT_IE | DATA_RxED_IE);    
    outp32(EPD_RSP_SC, 0x00000000);             /* auto validation */
    outp32(EPD_MPS, 0x00000040);                /* mps 64 */
    outp32(EPD_CFG, 0x00000043);                /* bulk out ep no 4 */
    outp32(EPD_START_ADDR, 0x00000700);
    outp32(EPD_END_ADDR, 0x0000077F);
}

void MSCVCPTrans_Init(
	USBD_INFO_T *psUSBDInfo
)
{
    /* Set Endpoint map */
    psUSBDInfo->i32EPA_Num = MSC_BULK_IN_EP_NUM;    /* Endpoint 1 */
    psUSBDInfo->i32EPB_Num = MSC_BULK_OUT_EP_NUM;    /* Endpoint 2 */
    psUSBDInfo->i32EPC_Num = VCP_BULK_IN_EP_NUM;    /* Endpoint 3 */
    psUSBDInfo->i32EPD_Num = VCP_BULK_OUT_EP_NUM;    /* Endpoint 4 */

    /* Set Callback Function */
    psUSBDInfo->pfnClassDataOUTCallBack = MSCVCPTrans_ClassReqDataOut;
    psUSBDInfo->pfnClassDataINCallBack = MSCVCPTrans_ClassReqDataIn;
    psUSBDInfo->pfnReset = MSCVCPTrans_Reset;

    psUSBDInfo->pfnEPACallBack = MSCTrans_BulkInHandler;
    psUSBDInfo->pfnEPBCallBack = MSCTrans_BulkOutHandler;
    psUSBDInfo->pfnEPCCallBack = VCPTrans_BulkInHandler;
    psUSBDInfo->pfnEPDCallBack = VCPTrans_BulkOutHandler;

    /* Set MSCVCP initialize function */
    psUSBDInfo->pfnFullSpeedInit = MSCVCPTrans_FullSpeedInit;
    psUSBDInfo->pfnHighSpeedInit = MSCVCPTrans_HighSpeedInit;

    /* Set MSC property */
	s_sMSCDInfo.SizePerSector = 512;
	s_sMSCDInfo.USBD_DMA_LEN = 0x20000;
	s_sMSCDInfo.Mass_Base_Addr = (UINT32)MSC_CMD_BUFFER;
	s_sMSCDInfo.Mass_Base_Addr |= 0x80000000;
	s_sMSCDInfo.Storage_Base_Addr = (UINT32)MSC_DATA_BUFFER;
	s_sMSCDInfo.Storage_Base_Addr |= 0x80000000;
	s_sMSCDInfo.SenseKey = 0x00;
	s_sMSCDInfo.ASC= 0x00;
	s_sMSCDInfo.ASCQ=0x00;

	s_psUSBDevInfo = psUSBDInfo;
}
