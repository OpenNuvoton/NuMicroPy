/**************************************************************************//**
 * @file     M48x_USB.c
 * @version  V0.01
 * @brief    M480 series USB HAL source file
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"

#include "M48x_USBD.h"
#include "NuMicro.h"
#include "HID_VCPDesc.h"
#include "HID_VCPTrans.h"
#include "MSC_Desc.h"
#include "MSC_Trans.h"

static S_USBDEV_STATE s_sUSBDev_state; 
S_USBD_INFO_T g_sUSBDev_DescInfo;


static void EnableHSUSBDevPhyClock(void){
    /* Select USBD */
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk;

    /* Select IP clock source */
    CLK->CLKDIV0 = (CLK->CLKDIV0 & ~CLK_CLKDIV0_USBDIV_Msk) | CLK_CLKDIV0_USB(4);

    /* Enable IP clock */
    CLK_EnableModuleClock(USBD_MODULE);

    /* Set PA.12 ~ PA.14 to input mode */
    PA->MODE &= ~(GPIO_MODE_MODE12_Msk | GPIO_MODE_MODE13_Msk | GPIO_MODE_MODE14_Msk);
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA12MFP_Msk|SYS_GPA_MFPH_PA13MFP_Msk|SYS_GPA_MFPH_PA14MFP_Msk|SYS_GPA_MFPH_PA15MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA12MFP_USB_VBUS|SYS_GPA_MFPH_PA13MFP_USB_D_N|SYS_GPA_MFPH_PA14MFP_USB_D_P|SYS_GPA_MFPH_PA15MFP_USB_OTG_ID);
}

void USBDEV_Deinit(
	S_USBDEV_STATE *psUSBDevState
)
{
	if(psUSBDevState == NULL)
		return;

	USBD_CLR_SE0();
	USBD_DISABLE_PHY();
	psUSBDevState->eUSBMode = eUSBDEV_MODE_NONE;
}

S_USBDEV_STATE *USBDEV_Init(
	uint16_t u16VID,
	uint16_t u16PID,
	E_USBDEV_MODE eUSBMode
)
{
	if(eUSBMode == eUSBDEV_MODE_NONE){
		USBDEV_Deinit(&s_sUSBDev_state);
		return &s_sUSBDev_state;
	}

	EnableHSUSBDevPhyClock();

	if(eUSBMode == eUSBDEV_MODE_MSC){
		MSCDesc_SetupDescInfo(&g_sUSBDev_DescInfo);
		MSCDesc_SetVID(&g_sUSBDev_DescInfo, u16VID);
		MSCDesc_SetPID(&g_sUSBDev_DescInfo, u16PID);

		USBD_Open(&g_sUSBDev_DescInfo, MSCTrans_ClassRequest, NULL);
		USBD_SetConfigCallback(MSCTrans_SetConfig);
		
		MSCTrans_Init(&g_sUSBDev_DescInfo);

	}
	else{
	
		HIDVCPDesc_SetupDescInfo(&g_sUSBDev_DescInfo, eUSBMode);
		HIDVCPDesc_SetVID(&g_sUSBDev_DescInfo, u16VID);
		HIDVCPDesc_SetPID(&g_sUSBDev_DescInfo, u16PID);

		USBD_Open(&g_sUSBDev_DescInfo, HIDVCPTrans_ClassRequest, NULL);

		/* Endpoint configuration */
		HIDVCPTrans_Init();
	}

//    USBD_Start();
//    NVIC_EnableIRQ(USBD_IRQn);

	s_sUSBDev_state.eUSBMode = eUSBMode;
	return &s_sUSBDev_state;
}

int32_t USBDEV_Start(
	S_USBDEV_STATE *psUSBDevState
)
{
	if(psUSBDevState == NULL)
		return -1;
	
	if(psUSBDevState->eUSBMode == eUSBDEV_MODE_NONE)
		return -2;

    USBD_Start();
    NVIC_EnableIRQ(USBD_IRQn);

	return 0;
}


#if MICROPY_HW_ENABLE_USBD

E_USBDEV_MODE USBDEV_GetMode(
	S_USBDEV_STATE *psUSBDevState
)
{
	if(psUSBDevState == NULL)
		return eUSBDEV_MODE_NONE;
	return psUSBDevState->eUSBMode;
}

int32_t USBDEV_HIDCanSend(
	S_USBDEV_STATE *psUSBDevState
)
{
	return HIDVCPTrans_HIDSetInReportCanSend();
}

int32_t USBDEV_VCPCanSend(
	S_USBDEV_STATE *psUSBDevState
)
{
	return HIDVCPTrans_VCPBulkInCanSend();
}

int32_t USBDEV_HIDSendData(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen,
	uint32_t u32Timeout,
	S_USBDEV_STATE *psUSBDevState
)
{
	int i32SendedLen = 0;
	if(pu8DataBuf == NULL)
		return i32SendedLen;
			
	i32SendedLen = HIDVCPTrans_StartHIDSetInReport(pu8DataBuf, u32DataBufLen);
	
	uint32_t u32SendTime = mp_hal_ticks_ms();
	
	while((mp_hal_ticks_ms() - u32SendTime) < u32Timeout){
		if(i32SendedLen != HIDVCPTrans_HIDSetInReportSendedLen()){
			u32SendTime = mp_hal_ticks_ms();
			i32SendedLen = HIDVCPTrans_HIDSetInReportSendedLen();
		}
		
		if(i32SendedLen ==  u32DataBufLen)
			break;
	}

	HIDVCPTrans_StopHIDSetInReport();

	return i32SendedLen;
}

int32_t USBDEV_VCPSendData(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen,
	uint32_t u32Timeout,
	S_USBDEV_STATE *psUSBDevState
)
{
	int i32SendedLen = 0;
	if(pu8DataBuf == NULL)
		return i32SendedLen;
			
	i32SendedLen = HIDVCPTrans_StartVCPBulkIn(pu8DataBuf, u32DataBufLen);
	
	uint32_t u32SendTime = mp_hal_ticks_ms();
	
	while((mp_hal_ticks_ms() - u32SendTime) < u32Timeout){
		if(i32SendedLen != HIDVCPTrans_VCPBulkInSendedLen()){
			u32SendTime = mp_hal_ticks_ms();
			i32SendedLen = HIDVCPTrans_VCPBulkInSendedLen();
		}
		
		if(i32SendedLen ==  u32DataBufLen)
			break;
	}

	HIDVCPTrans_StopVCPBulkIn();

	return i32SendedLen;
}


int32_t USBDEV_HIDCanRecv(
	S_USBDEV_STATE *psUSBDevState
)
{
	return HIDVCPTrans_HIDGetOutReportCanRecv();
}

int32_t USBDEV_VCPCanRecv(
	S_USBDEV_STATE *psUSBDevState
)
{
	return HIDVCPTrans_VCPBulkOutCanRecv();
}


int32_t USBDEV_HIDRecvData(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen,
	uint32_t u32Timeout,
	S_USBDEV_STATE *psUSBDevState
)
{
	int i32CopyLen = 0;
	int i32RecvLen = 0;

	if(pu8DataBuf == NULL)
		return i32RecvLen;

	uint32_t u32RecvTime = mp_hal_ticks_ms();
	
	while((mp_hal_ticks_ms() - u32RecvTime) < u32Timeout){
		
		i32CopyLen = HIDVCPTrans_HIDGetOutReportRecv(pu8DataBuf + i32RecvLen, u32DataBufLen - i32RecvLen);
		if(i32CopyLen){
			i32RecvLen += i32CopyLen;
			if(i32RecvLen >= u32DataBufLen)
				break;
			u32RecvTime = mp_hal_ticks_ms();
		}
	}

	return i32RecvLen;
}

int32_t USBDEV_VCPRecvData(
	uint8_t *pu8DataBuf,
	uint32_t u32DataBufLen,
	uint32_t u32Timeout,
	S_USBDEV_STATE *psUSBDevState
)
{
	int i32CopyLen = 0;
	int i32RecvLen = 0;

	if(pu8DataBuf == NULL)
		return i32RecvLen;

	uint32_t u32RecvTime = mp_hal_ticks_ms();
	
	while((mp_hal_ticks_ms() - u32RecvTime) < u32Timeout){
		
		i32CopyLen = HIDVCPTrans_VCPBulkOutRecv(pu8DataBuf + i32RecvLen, u32DataBufLen - i32RecvLen);
		if(i32CopyLen){
			i32RecvLen += i32CopyLen;
			if(i32RecvLen >= u32DataBufLen)
				break;
			u32RecvTime = mp_hal_ticks_ms();
		}
	}

	return i32RecvLen;
}


int32_t USBDEV_HIDInReportPacketSize(){
	return EP5_HID_MAX_PKT_SIZE;
}

int32_t USBDEV_HIDOutReportPacketSize(){
	return EP6_HID_MAX_PKT_SIZE;
}


static void EP5_Handler(void)
{
	//HID interrupt IN handler
	HIDVCPTrans_HIDInterruptInHandler();
}

static void EP6_Handler(void)
{
	//HID interrupt OUT handler
    uint8_t *pu8EPAddr;
    /* Interrupt OUT */
    pu8EPAddr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP6));
    HIDVCPTrans_HIDInterruptOutHandler(pu8EPAddr, USBD_GET_PAYLOAD_LEN(EP6));
}


#else

static void EP5_Handler(void)
{

}

static void EP6_Handler(void)
{

}

#endif

static void EP2_Handler(void)
{
	//VCP/MSC Bulk IN handler
	if(s_sUSBDev_state.eUSBMode == eUSBDEV_MODE_MSC){
		MSCTrans_BulkInHandler();
	}
	else if(s_sUSBDev_state.eUSBMode & eUSBDEV_MODE_VCP){
#if MICROPY_HW_ENABLE_USBD
		HIDVCPTrans_VCPBulkInHandler();
#endif
	}

}

static void EP3_Handler(void)
{
	//VCP/MSC Bulk OUT handler

	if(s_sUSBDev_state.eUSBMode == eUSBDEV_MODE_MSC){
		MSCTrans_BulkOutHandler();
	}
	else if(s_sUSBDev_state.eUSBMode & eUSBDEV_MODE_VCP){
#if MICROPY_HW_ENABLE_USBD
		//VCP bulk OUT handler
		uint8_t *pu8EPAddr;
		/* bulk OUT */
		pu8EPAddr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));

		HIDVCPTrans_VCPBulkOutHandler(pu8EPAddr, USBD_GET_PAYLOAD_LEN(EP3));
#endif
	}
}


void Handle_USBDEV_Irq(
	uint32_t u32IntStatus,
	uint32_t u32BusState
)
{
	
//------------------------------------------------------------------
    if (u32IntStatus & USBD_INTSTS_FLDET)
    {
        // Floating detect
        USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET);

        if (USBD_IS_ATTACHED())
        {
            /* USB Plug In */
            USBD_ENABLE_USB();
        }
        else
        {
            /* USB Un-plug */
            USBD_DISABLE_USB();
        }
    }

//------------------------------------------------------------------
    if (u32IntStatus & USBD_INTSTS_BUS)
    {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_BUS);

        if (u32BusState & USBD_STATE_USBRST)
        {
            /* Bus reset */
            USBD_ENABLE_USB();
            USBD_SwReset();
        }
        if (u32BusState & USBD_STATE_SUSPEND)
        {
            /* Enable USB but disable PHY */
            USBD_DISABLE_PHY();
        }
        if (u32BusState & USBD_STATE_RESUME)
        {
            /* Enable USB and enable PHY */
            USBD_ENABLE_USB();
        }
    }

//------------------------------------------------------------------
    if(u32IntStatus & USBD_INTSTS_WAKEUP)
    {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_WAKEUP);
    }

    if (u32IntStatus & USBD_INTSTS_USB)
    {
        // USB event
        if (u32IntStatus & USBD_INTSTS_SETUP)
        {
            // Setup packet
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

            /* Clear the data IN/OUT ready flag of control end-points */
            USBD_STOP_TRANSACTION(EP0);
            USBD_STOP_TRANSACTION(EP1);

            USBD_ProcessSetupPacket();
        }

        // EP events
        if (u32IntStatus & USBD_INTSTS_EP0)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP0);
            // control IN
            USBD_CtrlIn();
        }
        
        if (u32IntStatus & USBD_INTSTS_EP1)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP1);

            // control OUT
            USBD_CtrlOut();

            // In ACK of SET_LINE_CODE, Ignore this implement
//            if(g_usbd_SetupPacket[1] == SET_LINE_CODE)
//            {
//                if(g_usbd_SetupPacket[4] == 0)  /* VCOM-1 */
//                    VCOM_LineCoding(0); /* Apply UART settings */
//            }
        }

        if (u32IntStatus & USBD_INTSTS_EP2)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
            // Bulk IN
            EP2_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP3)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
            // Bulk OUT
            EP3_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP4)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP4);
        }

        if (u32IntStatus & USBD_INTSTS_EP5)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP5);
            // Interrupt IN
            EP5_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP6)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP6);
            // Interrupt OUT
            EP6_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP7)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP7);
        }

        if (u32IntStatus & USBD_INTSTS_EP8)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP8);
        }

	}
}







