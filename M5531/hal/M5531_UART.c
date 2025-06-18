/**************************************************************************//**
 * @file     M5531_UART.c
 * @version  V0.01
 * @brief    M5531 series UART HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"

#include "M5531_UART.h"
#include "nu_modutil.h"

struct nu_uart_var {
    uart_t *     obj;
    int32_t		i32RefCnt;
//    void        (*vec)(void);
    uint8_t     pdma_perp_tx;
    uint8_t     pdma_perp_rx;
};

static struct nu_uart_var uart0_var = {
    .obj                =   NULL,
    .i32RefCnt			=	0,
    .pdma_perp_tx       =   PDMA_UART0_TX,
    .pdma_perp_rx       =   PDMA_UART0_RX
};
static struct nu_uart_var uart1_var = {
    .obj                =   NULL,
    .i32RefCnt			= 	0,
    .pdma_perp_tx       =   PDMA_UART1_TX,
    .pdma_perp_rx       =   PDMA_UART1_RX
};
static struct nu_uart_var uart2_var = {
    .obj                =   NULL,
    .i32RefCnt			=	0,
    .pdma_perp_tx       =   PDMA_UART2_TX,
    .pdma_perp_rx       =   PDMA_UART2_RX
};
static struct nu_uart_var uart3_var = {
    .obj                =   NULL,
    .i32RefCnt			=	0,
    .pdma_perp_tx       =   PDMA_UART3_TX,
    .pdma_perp_rx       =   PDMA_UART3_RX
};
static struct nu_uart_var uart4_var = {
    .obj                =   NULL,
    .i32RefCnt			=	0,
    .pdma_perp_tx       =   PDMA_UART4_TX,
    .pdma_perp_rx       =   PDMA_UART4_RX
};
static struct nu_uart_var uart5_var = {
    .obj                =   NULL,
    .i32RefCnt			=	0,
    .pdma_perp_tx       =   PDMA_UART5_TX,
    .pdma_perp_rx       =   PDMA_UART5_RX
};
static struct nu_uart_var uart6_var = {
    .obj                =   NULL,
    .i32RefCnt			=	0,
    .pdma_perp_tx       =   PDMA_UART6_TX,
    .pdma_perp_rx       =   PDMA_UART6_RX
};
static struct nu_uart_var uart7_var = {
    .obj                =   NULL,
    .i32RefCnt			=	0,
    .pdma_perp_tx       =   PDMA_UART7_TX,
    .pdma_perp_rx       =   PDMA_UART7_RX
};
static struct nu_uart_var uart8_var = {
    .obj                =   NULL,
    .i32RefCnt			=	0,
    .pdma_perp_tx       =   PDMA_UART8_TX,
    .pdma_perp_rx       =   PDMA_UART8_RX
};
static struct nu_uart_var uart9_var = {
    .obj                =   NULL,
    .i32RefCnt			=	0,
    .pdma_perp_tx       =   PDMA_UART9_TX,
    .pdma_perp_rx       =   PDMA_UART9_RX
};
static struct nu_uart_var uart10_var = {
    .obj                =   NULL,
    .i32RefCnt			=	0,
    .pdma_perp_tx       =   LPPDMA_LPUART0_TX,
    .pdma_perp_rx       =   LPPDMA_LPUART0_RX
};


static const struct nu_modinit_s uart_modinit_tab[] = {
    {(uint32_t)UART0, UART0_MODULE, CLK_UARTSEL0_UART0SEL_HIRC, MODULE_NoMsk, SYS_UART0RST, UART0_IRQn, &uart0_var},
    {(uint32_t)UART1, UART1_MODULE, CLK_UARTSEL0_UART1SEL_HIRC, MODULE_NoMsk, SYS_UART1RST, UART1_IRQn, &uart1_var},
    {(uint32_t)UART2, UART2_MODULE, CLK_UARTSEL0_UART2SEL_HIRC, MODULE_NoMsk, SYS_UART2RST, UART2_IRQn, &uart2_var},
    {(uint32_t)UART3, UART3_MODULE, CLK_UARTSEL0_UART3SEL_HIRC, MODULE_NoMsk, SYS_UART3RST, UART3_IRQn, &uart3_var},
    {(uint32_t)UART4, UART4_MODULE, CLK_UARTSEL0_UART4SEL_HIRC, MODULE_NoMsk, SYS_UART4RST, UART4_IRQn, &uart4_var},
    {(uint32_t)UART5, UART5_MODULE, CLK_UARTSEL0_UART5SEL_HIRC, MODULE_NoMsk, SYS_UART5RST, UART5_IRQn, &uart5_var},
    {(uint32_t)UART6, UART6_MODULE, CLK_UARTSEL0_UART6SEL_HIRC, MODULE_NoMsk, SYS_UART6RST, UART6_IRQn, &uart6_var},
    {(uint32_t)UART7, UART7_MODULE, CLK_UARTSEL0_UART7SEL_HIRC, MODULE_NoMsk, SYS_UART7RST, UART7_IRQn, &uart7_var},
    {(uint32_t)UART8, UART8_MODULE, CLK_UARTSEL1_UART8SEL_HIRC, MODULE_NoMsk, SYS_UART8RST, UART8_IRQn, &uart8_var},
    {(uint32_t)UART9, UART9_MODULE, CLK_UARTSEL1_UART9SEL_HIRC, MODULE_NoMsk, SYS_UART9RST, UART9_IRQn, &uart9_var},

    {(uint32_t)LPUART0, LPUART0_MODULE, CLK_LPUARTSEL_LPUART0SEL_HIRC, MODULE_NoMsk, SYS_LPUART0RST, LPUART0_IRQn, &uart10_var},

    {0, 0, 0, 0, 0, (IRQn_Type) 0, NULL}
};

int32_t UART_Init(
    uart_t *psObj,
    UART_InitTypeDef *psInitDef,
    uint32_t IRQHandler
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_uart.uart, uart_modinit_tab);
    uint32_t u32IntFlag = 0;

    if(modinit == NULL)
        return -1;

    struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;

    if(psUARTVar->i32RefCnt)
        return -2;

    // Reset this module
    SYS_ResetModule(modinit->rsetidx);
    // Select IP clock source
    CLK_SetModuleClock(modinit->clkidx, modinit->clksrc, modinit->clkdiv);
    // Enable IP clock
    CLK_EnableModuleClock(modinit->clkidx);

//    psObj->dma_usage = DMA_USAGE_ALWAYS;
    psObj->event = 0;
    psObj->hdlr_async = 0;
    psObj->dma_chn_id_tx = NU_PDMA_OUT_OF_CHANNELS;
    psObj->dma_chn_id_rx = NU_PDMA_OUT_OF_CHANNELS;

    if(psObj->bLPUART) {
        //open uart
        LPUART_Open(psObj->u_uart.lpuart, psInitDef->u32BaudRate);

        //line configure
        LPUART_SetLineConfig(psObj->u_uart.lpuart, psInitDef->u32BaudRate, psInitDef->u32DataWidth, psInitDef->u32Parity, psInitDef->u32StopBits);

        //setup cts/rts if flow control enabled
        if(psInitDef->eFlowControl & eUART_HWCONTROL_CTS) {
            (psObj->u_uart.lpuart)->MODEMSTS  |= LPUART_MODEMSTS_CTSACTLV_Msk;
            u32IntFlag |= UART_INTEN_ATOCTSEN_Msk;
        }

        if(psInitDef->eFlowControl & eUART_HWCONTROL_RTS) {
            (psObj->u_uart.lpuart)->MODEM  |= LPUART_MODEM_RTSACTLV_Msk;
            u32IntFlag |= LPUART_INTEN_ATORTSEN_Msk;
            (psObj->u_uart.lpuart)->FIFO = ((psObj->u_uart.lpuart)->FIFO &~ LPUART_FIFO_RTSTRGLV_Msk) | LPUART_FIFO_RTSTRGLV_14BYTES;
        }

        psUARTVar->i32RefCnt ++;
        psUARTVar->obj = psObj;

        if(IRQHandler) {
            (psObj->u_uart.lpuart)->FIFO = ((psObj->u_uart.lpuart)->FIFO & (~LPUART_FIFO_RFITL_Msk)) | LPUART_FIFO_RFITL_1BYTE;

            LPUART_SetTimeoutCnt(psObj->u_uart.lpuart, 40);

            u32IntFlag |= LPUART_INTEN_RDAIEN_Msk|LPUART_INTEN_RXTOIEN_Msk;
            psObj->hdlr_async = IRQHandler;

            LPUART_ENABLE_INT(psObj->u_uart.lpuart, u32IntFlag);
            NVIC_EnableIRQ(modinit->irq_n);
        } else {
            psObj->hdlr_async = 0;
        }

    } else {

        //open uart
        UART_Open(psObj->u_uart.uart, psInitDef->u32BaudRate);

        //line configure
        UART_SetLineConfig(psObj->u_uart.uart, psInitDef->u32BaudRate, psInitDef->u32DataWidth, psInitDef->u32Parity, psInitDef->u32StopBits);

        //setup cts/rts if flow control enabled
        if(psInitDef->eFlowControl & eUART_HWCONTROL_CTS) {
            (psObj->u_uart.uart)->MODEMSTS  |= UART_MODEMSTS_CTSACTLV_Msk;
            u32IntFlag |= UART_INTEN_ATOCTSEN_Msk;
        }

        if(psInitDef->eFlowControl & eUART_HWCONTROL_RTS) {
            (psObj->u_uart.uart)->MODEM  |= UART_MODEM_RTSACTLV_Msk;
            u32IntFlag |= UART_INTEN_ATORTSEN_Msk;
            (psObj->u_uart.uart)->FIFO = ((psObj->u_uart.uart)->FIFO &~ UART_FIFO_RTSTRGLV_Msk) | UART_FIFO_RTSTRGLV_14BYTES;
        }

        psUARTVar->i32RefCnt ++;
        psUARTVar->obj = psObj;

        if(IRQHandler) {
            (psObj->u_uart.uart)->FIFO = ((psObj->u_uart.uart)->FIFO & (~UART_FIFO_RFITL_Msk)) | UART_FIFO_RFITL_1BYTE;

            UART_SetTimeoutCnt(psObj->u_uart.uart, 40);

            u32IntFlag |= UART_INTEN_RDAIEN_Msk|UART_INTEN_RXTOIEN_Msk;
            psObj->hdlr_async = IRQHandler;

            UART_ENABLE_INT(psObj->u_uart.uart, u32IntFlag);
            NVIC_EnableIRQ(modinit->irq_n);
        } else {
            psObj->hdlr_async = 0;
        }
    }

    return 0;
}

void UART_Final(uart_t *psObj)
{
    if (psObj->dma_chn_id_tx != NU_PDMA_OUT_OF_CHANNELS) {
        if(psObj->bLPUART) {
            nu_lppdma_channel_free(psObj->dma_chn_id_tx);
        } else {
            nu_pdma_channel_free(psObj->dma_chn_id_tx);
        }
        psObj->dma_chn_id_tx = NU_PDMA_OUT_OF_CHANNELS;
    }
    if (psObj->dma_chn_id_rx != NU_PDMA_OUT_OF_CHANNELS) {
        if(psObj->bLPUART) {
            nu_lppdma_channel_free(psObj->dma_chn_id_rx);
        } else {
            nu_pdma_channel_free(psObj->dma_chn_id_rx);
        }
        psObj->dma_chn_id_rx = NU_PDMA_OUT_OF_CHANNELS;
    }

    if(psObj->bLPUART) {
        LPUART_DisableFlowCtrl(psObj->u_uart.lpuart);
        LPUART_DisableInt(psObj->u_uart.lpuart, (LPUART_INTEN_RDAIEN_Msk|LPUART_INTEN_RXTOIEN_Msk));
        LPUART_Close(psObj->u_uart.lpuart);
    } else {
        UART_DisableFlowCtrl(psObj->u_uart.uart);
        UART_DisableInt(psObj->u_uart.uart, (UART_INTEN_RDAIEN_Msk|UART_INTEN_RXTOIEN_Msk));
        UART_Close(psObj->u_uart.uart);
    }

    psObj->hdlr_async = 0;

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_uart.uart, uart_modinit_tab);
    if(modinit == NULL)
        return;

    struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;

    NVIC_DisableIRQ(modinit->irq_n);

    // Disable IP clock
    CLK_DisableModuleClock(modinit->clkidx);

    if(psUARTVar->i32RefCnt)
        psUARTVar->i32RefCnt--;

    psUARTVar->obj = NULL;

}


void Handle_UART_Irq(UART_T *uart)
{
    uint32_t u32INTSTS = uart->INTSTS;
    uint32_t u32FIFOSTS = uart->FIFOSTS;


    if(u32INTSTS & UART_INTSTS_HWRLSIF_Msk) {
        uart->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
        return;
    }

    uart_t *obj = NULL;

    if(uart == UART0)
        obj = uart0_var.obj;
    else if(uart == UART1)
        obj = uart1_var.obj;
    else if(uart == UART2)
        obj = uart2_var.obj;
    else if(uart == UART3)
        obj = uart3_var.obj;
    else if(uart == UART4)
        obj = uart4_var.obj;
    else if(uart == UART5)
        obj = uart5_var.obj;
    else if(uart == UART6)
        obj = uart6_var.obj;
    else if(uart == UART7)
        obj = uart7_var.obj;
    else if(uart == UART8)
        obj = uart8_var.obj;
    else if(uart == UART9)
        obj = uart9_var.obj;

    if ( u32INTSTS & (UART_INTSTS_RDAINT_Msk|UART_INTSTS_RXTOINT_Msk) ) {
        if (obj && obj->hdlr_async) {
            void (*hdlr_async)(uart_t *) = (void(*)(uart_t *))(obj->hdlr_async);
            hdlr_async(obj);
        }
    }

    // FIXME: Ignore all other interrupt flags. Clear them. Otherwise, program will get stuck in interrupt.
    uart->INTSTS = u32INTSTS;
    uart->FIFOSTS = u32FIFOSTS;
}

void Handle_LPUART_Irq(LPUART_T *lpuart)
{
    uint32_t u32INTSTS = lpuart->INTSTS;
    uint32_t u32FIFOSTS = lpuart->FIFOSTS;


    if(u32INTSTS & LPUART_INTSTS_HWRLSIF_Msk) {
        lpuart->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk);
        return;
    }

    uart_t *obj = NULL;

    if(lpuart == LPUART0)
        obj = uart10_var.obj;

    if ( u32INTSTS & (LPUART_INTSTS_RDAINT_Msk|LPUART_INTSTS_RXTOINT_Msk) ) {
        if (obj && obj->hdlr_async) {
            void (*hdlr_async)(uart_t *) = (void(*)(uart_t *))(obj->hdlr_async);
            hdlr_async(obj);
        }
    }

    // FIXME: Ignore all other interrupt flags. Clear them. Otherwise, program will get stuck in interrupt.
    lpuart->INTSTS = u32INTSTS;
    lpuart->FIFOSTS = u32FIFOSTS;
}

static void UART_DMA_Handler_TX(void* id, uint32_t event_dma)
{
    uart_t *obj = (uart_t *) id;

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->u_uart.uart, uart_modinit_tab);
    if(modinit == NULL)
        return;

    struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;

    if(psUARTVar->obj != obj)
        return;

    // TODO: Pass this error to caller
    if (event_dma & NU_PDMA_EVENT_ABORT) {
    }

    // Expect UART IRQ will catch this transfer done event
    if (event_dma & NU_PDMA_EVENT_TRANSFER_DONE) {
        if (obj && obj->hdlr_dma_tx) {
            if(obj->bLPUART)
                LPUART_WAIT_TX_EMPTY(obj->u_uart.lpuart);
            else
                UART_WAIT_TX_EMPTY(obj->u_uart.uart);

            obj->hdlr_dma_tx(obj, event_dma);
        }
    }
    // TODO: Pass this error to caller
    if (event_dma & NU_PDMA_EVENT_TIMEOUT) {

    }
}

static void UART_DMA_Handler_RX(void* id, uint32_t event_dma)
{
    //TODO:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//	printf("UART_DMA_Handler_RX \n");

    uart_t *obj = (uart_t *) id;

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->u_uart.uart, uart_modinit_tab);
    if(modinit == NULL)
        return;

    struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;

    if(psUARTVar->obj != obj)
        return;

    // TODO: Pass this error to caller
    if (event_dma & NU_PDMA_EVENT_ABORT) {
    }

//    if ((event_dma & DMA_EVENT_TIMEOUT) && (!UART_GET_RX_EMPTY(obj->uart))){
//		return;
//   }

    // Expect UART IRQ will catch this transfer done event
    if (event_dma & ( NU_PDMA_EVENT_TIMEOUT | NU_PDMA_EVENT_TRANSFER_DONE )) {
        if (obj && obj->hdlr_dma_rx) {
            obj->hdlr_dma_rx(obj, event_dma);
        }

    }
}

int32_t UART_DMA_TXRX_Enable(
    uart_t *psObj,
    PFN_HDLR_DMA_TX pfnHandlerDMATx,
    PFN_HDLR_DMA_RX pfnHandlerDMARx
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_uart.uart, uart_modinit_tab);
    if(modinit == NULL)
        return -1;

    struct nu_uart_var *psUartVar = (struct nu_uart_var *)modinit->var;

    if (psObj->dma_usage == ePDMA_USAGE_NEVER) {
        if (psObj->dma_chn_id_rx == NU_PDMA_OUT_OF_CHANNELS) {
            if(psObj->bLPUART)
                psObj->dma_chn_id_rx = nu_lppdma_channel_dynamic_allocate(psUartVar->pdma_perp_rx);
            else
                psObj->dma_chn_id_rx = nu_pdma_channel_dynamic_allocate(psUartVar->pdma_perp_rx);
        }

        if (psObj->dma_chn_id_tx == NU_PDMA_OUT_OF_CHANNELS) {
            if(psObj->bLPUART)
                psObj->dma_chn_id_tx = nu_lppdma_channel_dynamic_allocate(psUartVar->pdma_perp_tx);
            else
                psObj->dma_chn_id_tx = nu_pdma_channel_dynamic_allocate(psUartVar->pdma_perp_tx);
        }

        if (psObj->dma_chn_id_tx == NU_PDMA_OUT_OF_CHANNELS || psObj->dma_chn_id_rx == NU_PDMA_OUT_OF_CHANNELS) {
            psObj->dma_usage = ePDMA_USAGE_NEVER;
        } else {
            psObj->dma_usage = ePDMA_USAGE_ALWAYS;
        }
    }

    if(psObj->dma_usage != ePDMA_USAGE_NEVER) {

        psObj->hdlr_dma_tx = pfnHandlerDMATx;
        psObj->hdlr_dma_rx = pfnHandlerDMARx;

        struct nu_pdma_chn_cb pdma_rx_chn_cb;
        struct nu_pdma_chn_cb pdma_tx_chn_cb;

        if(psObj->bLPUART) {
            nu_lppdma_filtering_set(psObj->dma_chn_id_rx, NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_TIMEOUT);
            nu_lppdma_filtering_set(psObj->dma_chn_id_tx, NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_TIMEOUT);
        } else {
            nu_pdma_filtering_set(psObj->dma_chn_id_rx, NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_TIMEOUT);
            nu_pdma_filtering_set(psObj->dma_chn_id_tx, NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_TIMEOUT);
        }

        pdma_rx_chn_cb.m_eCBType = eCBType_Event;
        pdma_rx_chn_cb.m_pfnCBHandler = UART_DMA_Handler_RX;
        pdma_rx_chn_cb.m_pvUserData = psObj;

        pdma_tx_chn_cb.m_eCBType = eCBType_Event;
        pdma_tx_chn_cb.m_pfnCBHandler = UART_DMA_Handler_TX;
        pdma_tx_chn_cb.m_pvUserData = psObj;

        if(psObj->bLPUART) {
            nu_lppdma_callback_register(psObj->dma_chn_id_rx, &pdma_rx_chn_cb);
            nu_lppdma_callback_register(psObj->dma_chn_id_tx, &pdma_tx_chn_cb);
            /* Enable Interrupt */
            LPUART_ENABLE_INT(psObj->u_uart.lpuart, LPUART_INTEN_RLSIEN_Msk);
        } else {
            nu_pdma_callback_register(psObj->dma_chn_id_rx, &pdma_rx_chn_cb);
            nu_pdma_callback_register(psObj->dma_chn_id_tx, &pdma_tx_chn_cb);
            /* Enable Interrupt */
            UART_ENABLE_INT(psObj->u_uart.uart, UART_INTEN_RLSIEN_Msk);
        }

        NVIC_EnableIRQ(modinit->irq_n);
    }

    return 0;
}

int32_t UART_DMA_TXRX_Disable(
    uart_t *psObj
)
{
    if (psObj->dma_usage != ePDMA_USAGE_NEVER) {

        psObj->hdlr_dma_tx = NULL;
        psObj->hdlr_dma_rx = NULL;

        if(psObj->bLPUART) {
            nu_lppdma_channel_free(psObj->dma_chn_id_tx);
            nu_lppdma_channel_free(psObj->dma_chn_id_rx);
        } else {
            nu_pdma_channel_free(psObj->dma_chn_id_tx);
            nu_pdma_channel_free(psObj->dma_chn_id_rx);
        }
        psObj->dma_chn_id_tx = NU_PDMA_OUT_OF_CHANNELS;
        psObj->dma_chn_id_rx = NU_PDMA_OUT_OF_CHANNELS;
        psObj->dma_usage = ePDMA_USAGE_NEVER;
    }
    return 0;
}

int32_t UART_DMA_RX_Start(
    uart_t *psObj,
    uint8_t *pu8DestBuf,
    int32_t i32TriggerLen
)
{

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_uart.uart, uart_modinit_tab);
    if(modinit == NULL)
        return -1;

    struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;
    if(psUARTVar->obj != psObj)
        return -2;

    if(psObj->bLPUART) {
        nu_lppdma_transfer( psObj->dma_chn_id_rx,
                            8,
                            (uint32_t)&(psObj->u_uart.lpuart)->DAT,
                            (uint32_t)pu8DestBuf,
                            i32TriggerLen,
                            5000
                          );

        //(1000000*40)/115200 );

        LPUART_ENABLE_INT(psObj->u_uart.lpuart, LPUART_INTEN_RXPDMAEN_Msk); // Start DMA transfe
    } else {
        nu_pdma_transfer( psObj->dma_chn_id_rx,
                          8,
                          (uint32_t)&(psObj->u_uart.uart)->DAT,
                          (uint32_t)pu8DestBuf,
                          i32TriggerLen,
                          5000
                        );

        //(1000000*40)/115200 );

        UART_ENABLE_INT(psObj->u_uart.uart, UART_INTEN_RXPDMAEN_Msk); // Start DMA transfe
    }

    return 0;
}

int32_t UART_DMA_RX_Stop(
    uart_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_uart.uart, uart_modinit_tab);
    if(modinit == NULL)
        return -1;

    struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;
    if(psUARTVar->obj != psObj)
        return -2;

    if(psObj->bLPUART) {
        LPUART_DISABLE_INT(psObj->u_uart.lpuart, LPUART_INTEN_RXPDMAEN_Msk); // Stop DMA transfer
    } else {
        UART_DISABLE_INT(psObj->u_uart.uart, UART_INTEN_RXPDMAEN_Msk); // Stop DMA transfer
    }
    return 0;
}

int32_t UART_DMA_TX_Start(
    uart_t *psObj,
    uint8_t *pu8SrcBuf,
    int32_t i32TriggerLen
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_uart.uart, uart_modinit_tab);
    if(modinit == NULL)
        return -1;

    struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;
    if(psUARTVar->obj != psObj)
        return -2;

    if(psObj->bLPUART) {
        nu_lppdma_transfer( psObj->dma_chn_id_tx,
                            8,
                            (uint32_t)pu8SrcBuf,
                            (uint32_t)&(psObj->u_uart.lpuart)->DAT,
                            i32TriggerLen,
                            0
                          );

        LPUART_ENABLE_INT(psObj->u_uart.lpuart, LPUART_INTEN_TXPDMAEN_Msk); // Start DMA transfe
    } else {
        nu_pdma_transfer( psObj->dma_chn_id_tx,
                          8,
                          (uint32_t)pu8SrcBuf,
                          (uint32_t)&(psObj->u_uart.uart)->DAT,
                          i32TriggerLen,
                          0
                        );

        UART_ENABLE_INT(psObj->u_uart.uart, UART_INTEN_TXPDMAEN_Msk); // Start DMA transfe
    }
    return 0;
}

int32_t UART_DMA_TX_Stop(
    uart_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_uart.uart, uart_modinit_tab);
    if(modinit == NULL)
        return -1;

    struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;
    if(psUARTVar->obj != psObj)
        return -2;

    if(psObj->bLPUART) {
        LPUART_DISABLE_INT(psObj->u_uart.lpuart, LPUART_INTEN_TXPDMAEN_Msk); // Stop DMA transfer
    } else {
        UART_DISABLE_INT(psObj->u_uart.uart, UART_INTEN_TXPDMAEN_Msk); // Stop DMA transfer
    }
    return 0;
}



