/**************************************************************************//**
 * @file     M48x_USB.c
 * @version  V0.01
 * @brief    M480 series USB HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"

#include "M55M1_USBD.h"
#include "NuMicro.h"
#include "MSC_VCPDesc.h"
#include "MSC_VCPTrans.h"

static S_USBDEV_STATE s_sUSBDev_state;
S_USBD_INFO_T g_sUSBDev_DescInfo;

uint32_t volatile g_u32MSCOutToggle = 0, g_u32MSCOutSkip = 0;
uint32_t volatile g_u32VCPOutToggle = 0;

uint8_t volatile g_u8MSCRemove = 0;
uint32_t volatile g_u32VCPConnect = 0;
uint32_t volatile g_u32MSCConnect = 0;
uint32_t volatile g_u32DataBusConnect = 0;


static void EnableHSUSBDevPhyClock(void)
{

    /* Select USB clock source as HIRC48M and USB clock divider as 1 */
    CLK_SetModuleClock(USBD0_MODULE, CLK_USBSEL_USBSEL_HIRC48M, CLK_USBDIV_USBDIV(1));

    /* Enable OTG0 module clock */
    CLK_EnableModuleClock(OTG0_MODULE);

    /* Select USBD */
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk ;

    /* Enable USBD module clock */
    CLK_EnableModuleClock(USBD0_MODULE);

    /* USBD multi-function pins for VBUS, D+, D-, and ID pins */
    SET_USB_VBUS_PA12();
    SET_USB_D_MINUS_PA13();
    SET_USB_D_PLUS_PA14();
    SET_USB_OTG_ID_PA15();

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
    if(eUSBMode == eUSBDEV_MODE_NONE) {
        USBDEV_Deinit(&s_sUSBDev_state);
        return &s_sUSBDev_state;
    }

    EnableHSUSBDevPhyClock();

    if((eUSBMode & eUSBDEV_MODE_MSC_VCP) == eUSBDEV_MODE_MSC_VCP) {
        MSCVCPDesc_SetupDescInfo(&g_sUSBDev_DescInfo);
        MSCVCPDesc_SetVID(&g_sUSBDev_DescInfo, u16VID);
        MSCVCPDesc_SetPID(&g_sUSBDev_DescInfo, u16PID);

        USBD_Open(&g_sUSBDev_DescInfo, MSCVCPTrans_ClassRequest, NULL);
        USBD_SetConfigCallback(MSCTrans_SetConfig);

        if(eUSBMode == eUSBDEV_MODE_MSC_WP_VCP)
            MSCVCPTrans_Init(&g_sUSBDev_DescInfo, true);
        else
            MSCVCPTrans_Init(&g_sUSBDev_DescInfo, false);
    } else {
        //Not support now
    }

    s_sUSBDev_state.eUSBMode = eUSBMode;
    return &s_sUSBDev_state;
}

S_USBDEV_STATE *USBDEV_UpdateState(void)
{
    s_sUSBDev_state.bConnected = 0;
    if((s_sUSBDev_state.eUSBMode & eUSBDEV_MODE_MSC_VCP) == eUSBDEV_MODE_MSC_VCP) {
        if((g_u32MSCConnect) && (g_u32VCPConnect))
            s_sUSBDev_state.bConnected = 1;
    }

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

E_USBDEV_MODE USBDEV_GetMode(
    S_USBDEV_STATE *psUSBDevState
)
{
    if(psUSBDevState == NULL)
        return eUSBDEV_MODE_NONE;
    return psUSBDevState->eUSBMode;
}

int32_t USBDEV_DataBusConnect(void)
{
    return g_u32DataBusConnect;
}

void USBDEV_MSCEnDisable(
    int32_t i32EnDisable
)
{
    if(i32EnDisable)
        g_u8MSCRemove = 0;
    else
        g_u8MSCRemove = 1;
}

int32_t USBDEV_VCPCanSend(
    S_USBDEV_STATE *psUSBDevState
)
{
    if(!USBD_IS_ATTACHED())
        return 0;

    return VCPTrans_BulkInCanSend();
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

    i32SendedLen = VCPTrans_BulkInSend(pu8DataBuf, u32DataBufLen);

    uint32_t u32SendTime = mp_hal_ticks_ms();

    while((mp_hal_ticks_ms() - u32SendTime) < u32Timeout) {
        if((i32SendedLen >=  u32DataBufLen))
            break;

        if(VCPTrans_BulkInCanSend()) {
            i32SendedLen += VCPTrans_BulkInSend(pu8DataBuf + i32SendedLen, u32DataBufLen);
        }
    }

    return i32SendedLen;
}

int32_t USBDEV_VCPCanRecv(
    S_USBDEV_STATE *psUSBDevState
)
{
    return VCPTrans_BulkOutCanRecv();
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

    while((mp_hal_ticks_ms() - u32RecvTime) < u32Timeout) {

        i32CopyLen = VCPTrans_BulkOutRecv(pu8DataBuf + i32RecvLen, u32DataBufLen - i32RecvLen);
        if(i32CopyLen) {
            i32RecvLen += i32CopyLen;
            if(i32RecvLen >= u32DataBufLen)
                break;
            u32RecvTime = mp_hal_ticks_ms();
        }
    }

    return i32RecvLen;
}

static void EP5_Handler(void)
{
    MSCTrans_BulkInHandler();
}

static void EP6_Handler(void)
{
    MSCTrans_BulkOutHandler();

}

static void EP2_Handler(void)
{
    //VCP/MSC Bulk IN handler
    if(s_sUSBDev_state.eUSBMode & eUSBDEV_MODE_VCP) {
        VCPTrans_BulkInHandler();
    } else if(s_sUSBDev_state.eUSBMode & eUSBDEV_MODE_HID) {
    }

}

static void EP3_Handler(void)
{
    //VCP bulk OUT handler

    if(s_sUSBDev_state.eUSBMode & eUSBDEV_MODE_VCP) {
        /* Bulk OUT */
        if (g_u32VCPOutToggle == (USBD->EPSTS0 & 0xf000)) {
            USBD_SET_PAYLOAD_LEN(EP3, EP3_VCP_MAX_PKT_SIZE);
        } else {

            //VCP bulk OUT handler
            uint8_t *pu8EPAddr;
            /* bulk OUT */
            pu8EPAddr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));

            VCPTrans_BulkOutHandler(pu8EPAddr, USBD_GET_PAYLOAD_LEN(EP3));
            g_u32VCPOutToggle = USBD->EPSTS0 & 0xf000;
        }
    } else if(s_sUSBDev_state.eUSBMode & eUSBDEV_MODE_HID) {
    }
}


void Handle_USBDEV_Irq(
    uint32_t u32IntStatus,
    uint32_t u32BusState
)
{

//------------------------------------------------------------------
    if (u32IntStatus & USBD_INTSTS_FLDET) {
        // Floating detect
        USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET);

        if (USBD_IS_ATTACHED()) {
            /* USB Plug In */
            USBD_ENABLE_USB();
        } else {
            /* USB Un-plug */
            USBD_DISABLE_USB();
        }
    }

//------------------------------------------------------------------
    if (u32IntStatus & USBD_INTSTS_BUS) {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_BUS);

        if (u32BusState & USBD_STATE_USBRST) {
            /* Bus reset */
            USBD_ENABLE_USB();
            USBD_SwReset();
            g_u8MSCRemove = 0;
            g_u32VCPOutToggle = g_u32MSCOutToggle = g_u32MSCOutSkip = 0;
            g_u32VCPConnect = 0;
            g_u32MSCConnect = 0;
            g_u32DataBusConnect = 1;
            printf("USBD_STATE_USBRST \n");
        }
        if (u32BusState & USBD_STATE_SUSPEND) {
            /* Enable USB but disable PHY */
            printf("USBD_STATE_SUSPEND \n");
            g_u32DataBusConnect = 0;
            USBD_DISABLE_PHY();
        }
        if (u32BusState & USBD_STATE_RESUME) {
            /* Enable USB and enable PHY */
            printf("USBD_STATE_RESUME \n");
            USBD_ENABLE_USB();
        }
    }

//------------------------------------------------------------------
    if(u32IntStatus & USBD_INTSTS_WAKEUP) {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_WAKEUP);
    }

    if (u32IntStatus & USBD_INTSTS_USB) {
        // USB event
        if (u32IntStatus & USBD_INTSTS_SETUP) {
            // Setup packet
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

            /* Clear the data IN/OUT ready flag of control end-points */
            USBD_STOP_TRANSACTION(EP0);
            USBD_STOP_TRANSACTION(EP1);

            USBD_ProcessSetupPacket();
        }

        // EP events
        if (u32IntStatus & USBD_INTSTS_EP0) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP0);
            // control IN
            USBD_CtrlIn();
        }

        if (u32IntStatus & USBD_INTSTS_EP1) {
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

        if (u32IntStatus & USBD_INTSTS_EP2) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
            // Bulk IN
            EP2_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP3) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
            // Bulk OUT
            EP3_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP4) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP4);
        }

        if (u32IntStatus & USBD_INTSTS_EP5) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP5);
            // Interrupt IN
            EP5_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP6) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP6);
            // Interrupt OUT
            EP6_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP7) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP7);
        }

        if (u32IntStatus & USBD_INTSTS_EP8) {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP8);
        }

    }
}

void USBDEV_VCPRegisterSingal(
    PFN_USBDEV_VCPRecvSignal pfnSignal
)
{
    VCPTrans_RegisterSingal(pfnSignal);
}







