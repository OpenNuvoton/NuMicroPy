/***************************************************************************//**
 * @file     HID_VCPTrans.c
 * @brief    M480 series USB class transfer code for VCP and HID
 * @version  0.0.1
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include <string.h>
#include "NuMicro.h"
#include "HID_VCPTrans.h"


//EP5 for HID interrupt in and address 0x5
//EP6 for HID interrupt out and address 0x6
//EP2 for VCP bulk in and address  0x1
//EP3 for VCP bulk out and address 0x2
//EP4 for VCP interrupt in and address 0x3


/*--------------------------------------------------------------------------*/
//VCP releated parameter

STR_VCOM_LINE_CODING s_sLineCoding = {115200, 0, 0, 8};   /* Baud rate : 115200    */
/* Stop bit     */
/* parity       */
/* data bits    */
uint16_t s_u16CtrlSignal = 0;     /* BIT0: DTR(Data Terminal Ready) , BIT1: RTS(Request To Send) */



void HIDVCPTrans_ClassRequest(void)
{
    uint8_t buf[8];

    USBD_GetSetupPacket(buf);

    if (buf[0] & 0x80)   /* request data transfer direction */
    {
        // Device to host
        switch (buf[1])
        {
        case VCP_GET_LINE_CODE:
        {
            if (buf[4] == 0)   /* VCOM-1 */
            {
                USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)), (uint8_t *)&s_sLineCoding, 7);
            }
            /* Data stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 7);
            /* Status stage */
            USBD_PrepareCtrlOut(0,0);
            break;
        }
        case HID_GET_REPORT:
        case HID_GET_IDLE:
        case HID_GET_PROTOCOL:
        default:
        {
            /* Setup error, stall the device */
            USBD_SetStall(0);
            break;
        }
        }
    }
    else
    {
        // Host to device
        switch (buf[1])
        {
        case VCP_SET_CONTROL_LINE_STATE:
        {
            if (buf[4] == 0)   /* VCOM-1 */
            {
                s_u16CtrlSignal = buf[3];
                s_u16CtrlSignal = (s_u16CtrlSignal << 8) | buf[2];
                //printf("RTS=%d  DTR=%d\n", (gCtrlSignal0 >> 1) & 1, gCtrlSignal0 & 1);
            }

            /* Status stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 0);
            break;
        }
        case VCP_SET_LINE_CODE:
        {
            //g_usbd_UsbConfig = 0100;
            if (buf[4] == 0) /* VCOM-1 */
                USBD_PrepareCtrlOut((uint8_t *)&s_sLineCoding, 7);

            /* Status stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 0);

            break;
        }
        case HID_SET_REPORT:
        {
            if (buf[3] == 3)
            {
                /* Request Type = Feature */
                USBD_SET_DATA1(EP1);
                USBD_SET_PAYLOAD_LEN(EP1, 0);
            }
            break;
        }
        case HID_SET_IDLE:
        {
            /* Status stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 0);
            break;
        }
        case HID_SET_PROTOCOL:
        default:
        {
            // Stall
            /* Setup error, stall the device */
            USBD_SetStall(0);
            break;
        }
        }
    }
}


/*--------------------------------------------------------------------------*/
/**
  * @brief  USBD Endpoint Config.
  * @param  None.
  * @retval None.
  */
void HIDVCPTrans_Init(void)
{
    /* Init setup packet buffer */
    /* Buffer range for setup packet -> [0 ~ 0x7] */
    USBD->STBUFSEG = SETUP_HID_VCP_BUF_BASE;

    /*****************************************************/
    /* EP0 ==> control IN endpoint, address 0 */
    USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
    /* Buffer range for EP0 */
    USBD_SET_EP_BUF_ADDR(EP0, EP0_HID_VCP_BUF_BASE);

    /* EP1 ==> control OUT endpoint, address 0 */
    USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
    /* Buffer range for EP1 */
    USBD_SET_EP_BUF_ADDR(EP1, EP1_HID_VCP_BUF_BASE);

    /*****************************************************/
    /* EP2 ==> Bulk IN endpoint, address 1 */
    USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_IN | VCP_BULK_IN_EP_NUM);
    /* Buffer offset for EP2 */
    USBD_SET_EP_BUF_ADDR(EP2, EP2_VCP_BUF_BASE);

    /* EP3 ==> Bulk Out endpoint, address 2 */
    USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_OUT | VCP_BULK_OUT_EP_NUM);
    /* Buffer offset for EP3 */
    USBD_SET_EP_BUF_ADDR(EP3, EP3_VCP_BUF_BASE);
    /* trigger receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP3, EP3_VCP_MAX_PKT_SIZE);

    /* EP4 ==> Interrupt IN endpoint, address 3 */
    USBD_CONFIG_EP(EP4, USBD_CFG_EPMODE_IN | VCP_INT_IN_EP_NUM);
    /* Buffer offset for EP4 ->  */
    USBD_SET_EP_BUF_ADDR(EP4, EP4_VCP_BUF_BASE);

    /*****************************************************/
    /* EP5 ==> Interrupt IN endpoint, address 4 */
    USBD_CONFIG_EP(EP5, USBD_CFG_EPMODE_IN | HID_INT_IN_EP_NUM);
    /* Buffer range for EP5 */
    USBD_SET_EP_BUF_ADDR(EP5, EP5_HID_BUF_BASE);

    /* EP6 ==> Interrupt OUT endpoint, address 5 */
    USBD_CONFIG_EP(EP6, USBD_CFG_EPMODE_OUT | HID_INT_OUT_EP_NUM);
    /* Buffer range for EP6 */
    USBD_SET_EP_BUF_ADDR(EP6, EP6_HID_BUF_BASE);
    /* trigger to receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP6, EP6_HID_MAX_PKT_SIZE);

}

static volatile uint8_t *s_pu8HIDSendBuf = NULL;
static volatile uint32_t s_u32HIDSendBufLen = 0;
static volatile uint32_t s_u32HIDSendBufPos = 0;

int32_t HIDVCPTrans_HIDInterruptInHandler()
{
	int32_t i32Len;
	uint8_t *pu8EPBufAddr;
	
	if(s_pu8HIDSendBuf == NULL)
		return 0;
		
	i32Len = s_u32HIDSendBufLen - s_u32HIDSendBufPos;
	
	if(i32Len > EP5_HID_MAX_PKT_SIZE)
		i32Len = EP5_HID_MAX_PKT_SIZE;
	
	if(i32Len == 0){
		return 0;
	}

	pu8EPBufAddr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP5));
	USBD_MemCopy(pu8EPBufAddr, (void *)s_pu8HIDSendBuf + s_u32HIDSendBufPos, i32Len);
	USBD_SET_PAYLOAD_LEN(EP5, i32Len);

	s_u32HIDSendBufPos += i32Len;
	
	return i32Len;
}


int32_t HIDVCPTrans_StartHIDSetInReport(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
)
{
	s_pu8HIDSendBuf = pu8DataBuf;
	s_u32HIDSendBufLen = u32DataBufLen;
	s_u32HIDSendBufPos = 0;

	return HIDVCPTrans_HIDInterruptInHandler();
}

int32_t HIDVCPTrans_HIDSetInReportSendedLen()
{
	return s_u32HIDSendBufPos;
}

int32_t HIDVCPTrans_HIDSetInReportCanSend()
{
	if(s_pu8HIDSendBuf != NULL)
		return 0;
	return 1;
}

void HIDVCPTrans_StopHIDSetInReport()
{
	s_pu8HIDSendBuf = NULL;
	s_u32HIDSendBufLen = 0;
	s_u32HIDSendBufPos = 0;

}

#define MAX_HID_RECV_BUF_LEN (EP6_HID_MAX_PKT_SIZE * 3)
static uint8_t s_au8HIDRecvBuf[MAX_HID_RECV_BUF_LEN];
static volatile uint32_t s_u32HIDRecvBufIn = 0;
static volatile uint32_t s_u32HIDRecvBufOut = 0;

int32_t HIDVCPTrans_HIDInterruptOutHandler(
	uint8_t *pu8EPBuf, 
	uint32_t u32Size
)
{
	int32_t i32BufFreeLen;
	uint32_t u32LimitLen;
	uint32_t u32NewInIdx = s_u32HIDRecvBufIn;
	uint32_t u32OutIdx = s_u32HIDRecvBufOut;

	if(u32NewInIdx >= u32OutIdx){
		i32BufFreeLen = MAX_HID_RECV_BUF_LEN - u32NewInIdx +  u32OutIdx;
		u32LimitLen = MAX_HID_RECV_BUF_LEN - u32NewInIdx;
	}
	else{
		i32BufFreeLen = u32OutIdx - u32NewInIdx;
		u32LimitLen = i32BufFreeLen;
	}
	
	if(i32BufFreeLen < u32Size){
		printf("USBD receive buffer size overflow, lost packet \n");
		return 0;
	}

	if(u32LimitLen < u32Size){
		USBD_MemCopy(s_au8HIDRecvBuf + u32NewInIdx, pu8EPBuf, u32LimitLen);		
		USBD_MemCopy(s_au8HIDRecvBuf, pu8EPBuf + u32LimitLen, u32Size - u32LimitLen);
	}
	else{
		USBD_MemCopy(s_au8HIDRecvBuf + u32NewInIdx, pu8EPBuf, u32Size);
	}
	
	u32NewInIdx = (u32NewInIdx + u32Size) % MAX_HID_RECV_BUF_LEN;
	s_u32HIDRecvBufIn = u32NewInIdx;
	return u32Size;
}

int32_t HIDVCPTrans_HIDGetOutReportCanRecv()
{
	uint32_t u32InIdx;
	uint32_t u32OutIdx;
	int i32RecvLen = 0;

	u32InIdx = s_u32HIDRecvBufIn;
	u32OutIdx = s_u32HIDRecvBufOut;

	if(u32InIdx >= u32OutIdx){
		i32RecvLen = u32InIdx - u32OutIdx;
	}
	else{
		i32RecvLen = MAX_HID_RECV_BUF_LEN - u32OutIdx +  u32InIdx;
	}
	
	return i32RecvLen;
}

int32_t HIDVCPTrans_HIDGetOutReportRecv(
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
		
	u32InIdx = s_u32HIDRecvBufIn;
	u32NewOutIdx = s_u32HIDRecvBufOut;

	if(u32InIdx >= u32NewOutIdx){
		i32CopyLen = u32InIdx - u32NewOutIdx;
		u32LimitLen = i32CopyLen;
	}
	else{
		i32CopyLen = MAX_HID_RECV_BUF_LEN - u32NewOutIdx +  u32InIdx;
		u32LimitLen = MAX_HID_RECV_BUF_LEN - u32NewOutIdx;
	}

	if(i32CopyLen > u32DataBufLen){
		i32CopyLen = u32DataBufLen;
		u32LimitLen = i32CopyLen;
	}

	if(i32CopyLen){

		if((i32CopyLen + u32NewOutIdx) > MAX_HID_RECV_BUF_LEN){
			memcpy(pu8DataBuf, s_au8HIDRecvBuf + u32NewOutIdx, u32LimitLen);
			memcpy(pu8DataBuf + u32LimitLen, s_au8HIDRecvBuf, i32CopyLen - u32LimitLen);
		}
		else{
			memcpy(pu8DataBuf, s_au8HIDRecvBuf + u32NewOutIdx, i32CopyLen);
		}

		u32NewOutIdx = (u32NewOutIdx + i32CopyLen) % MAX_HID_RECV_BUF_LEN;
		s_u32HIDRecvBufOut = u32NewOutIdx;
		USBD_SET_PAYLOAD_LEN(EP6, EP6_HID_MAX_PKT_SIZE);
	}

	return i32CopyLen;
}


static volatile uint8_t *s_pu8VCPSendBuf = NULL;
static volatile uint32_t s_u32VCPSendBufLen = 0;
static volatile uint32_t s_u32VCPSendBufPos = 0;

int32_t HIDVCPTrans_VCPBulkInHandler()
{
	int32_t i32Len;
	uint8_t *pu8EPBufAddr;
	
	if(s_pu8VCPSendBuf == NULL)
		return 0;
		
	i32Len = s_u32VCPSendBufLen - s_u32VCPSendBufPos;
	
	if(i32Len > EP2_VCP_MAX_PKT_SIZE)
		i32Len = EP2_VCP_MAX_PKT_SIZE;
	
	if(i32Len == 0){
		return 0;
	}

	pu8EPBufAddr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2));
	USBD_MemCopy(pu8EPBufAddr, (void *)s_pu8VCPSendBuf + s_u32VCPSendBufPos, i32Len);
	USBD_SET_PAYLOAD_LEN(EP2, i32Len);

	s_u32VCPSendBufPos += i32Len;
	
	return i32Len;
}

int32_t HIDVCPTrans_StartVCPBulkIn(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen
)
{
	s_pu8VCPSendBuf = pu8DataBuf;
	s_u32VCPSendBufLen = u32DataBufLen;
	s_u32VCPSendBufPos = 0;

	return HIDVCPTrans_VCPBulkInHandler();
}

int32_t HIDVCPTrans_VCPBulkInSendedLen()
{
	return s_u32VCPSendBufPos;
}

int32_t HIDVCPTrans_VCPBulkInCanSend()
{
	if(s_pu8VCPSendBuf != NULL)
		return 0;
	return 1;
}

void HIDVCPTrans_StopVCPBulkIn()
{
	s_pu8VCPSendBuf = NULL;
	s_u32VCPSendBufLen = 0;
	s_u32VCPSendBufPos = 0;

}

#define MAX_VCP_RECV_BUF_LEN (EP3_VCP_MAX_PKT_SIZE * 3)
static uint8_t s_au8VCPRecvBuf[MAX_VCP_RECV_BUF_LEN];
static volatile uint32_t s_u32VCPRecvBufIn = 0;
static volatile uint32_t s_u32VCPRecvBufOut = 0;

int32_t HIDVCPTrans_VCPBulkOutHandler(
	uint8_t *pu8EPBuf, 
	uint32_t u32Size
)
{
	int32_t i32BufFreeLen;
	uint32_t u32LimitLen;
	uint32_t u32NewInIdx = s_u32VCPRecvBufIn;
	uint32_t u32OutIdx = s_u32VCPRecvBufOut;

	if(u32NewInIdx >= u32OutIdx){
		i32BufFreeLen = MAX_VCP_RECV_BUF_LEN - u32NewInIdx +  u32OutIdx;
		u32LimitLen = MAX_VCP_RECV_BUF_LEN - u32NewInIdx;
	}
	else{
		i32BufFreeLen = u32OutIdx - u32NewInIdx;
		u32LimitLen = i32BufFreeLen;
	}
	
	if(i32BufFreeLen < u32Size){
		printf("USBD receive buffer size overflow, lost packet \n");
		return 0;
	}

	if(u32LimitLen < u32Size){
		USBD_MemCopy(s_au8VCPRecvBuf + u32NewInIdx, pu8EPBuf, u32LimitLen);		
		USBD_MemCopy(s_au8VCPRecvBuf, pu8EPBuf + u32LimitLen, u32Size - u32LimitLen);
	}
	else{
		USBD_MemCopy(s_au8VCPRecvBuf + u32NewInIdx, pu8EPBuf, u32Size);
	}
	
	u32NewInIdx = (u32NewInIdx + u32Size) % MAX_VCP_RECV_BUF_LEN;
	s_u32VCPRecvBufIn = u32NewInIdx;
	return u32Size;
}

int32_t HIDVCPTrans_VCPBulkOutCanRecv()
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

int32_t HIDVCPTrans_VCPBulkOutRecv(
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
		USBD_SET_PAYLOAD_LEN(EP3, EP3_VCP_MAX_PKT_SIZE);
	}

	return i32CopyLen;
}
