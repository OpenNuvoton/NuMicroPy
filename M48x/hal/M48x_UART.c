/**************************************************************************//**
 * @file     M48x_UART.c
 * @version  V0.01
 * @brief    M480 series SPI HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"
#include "nu_modutil.h"
#include "nu_miscutil.h"
#include "nu_bitutil.h"


#include "dma.h"
#include "M48x_UART.h"

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


static const struct nu_modinit_s uart_modinit_tab[] = {
    {(uint32_t)UART0, UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, MODULE_NoMsk, UART0_RST, UART0_IRQn, &uart0_var},
    {(uint32_t)UART1, UART1_MODULE, CLK_CLKSEL1_UART1SEL_HXT, MODULE_NoMsk, UART1_RST, UART1_IRQn, &uart1_var},
    {(uint32_t)UART2, UART2_MODULE, CLK_CLKSEL3_UART2SEL_HXT, MODULE_NoMsk, UART2_RST, UART2_IRQn, &uart2_var},
    {(uint32_t)UART3, UART3_MODULE, CLK_CLKSEL3_UART3SEL_HXT, MODULE_NoMsk, UART3_RST, UART3_IRQn, &uart3_var},
    {(uint32_t)UART4, UART4_MODULE, CLK_CLKSEL3_UART4SEL_HXT, MODULE_NoMsk, UART4_RST, UART4_IRQn, &uart4_var},
    {(uint32_t)UART5, UART5_MODULE, CLK_CLKSEL3_UART5SEL_HXT, MODULE_NoMsk, UART5_RST, UART5_IRQn, &uart5_var},

    {0, 0, 0, 0, 0, (IRQn_Type) 0, NULL}
};


int32_t UART_Init(
	uart_t *psObj,
	UART_InitTypeDef *psInitDef,
	uint32_t IRQHandler
)
{
	const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->uart, uart_modinit_tab);
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
    psObj->dma_chn_id_tx = DMA_ERROR_OUT_OF_CHANNELS;
    psObj->dma_chn_id_rx = DMA_ERROR_OUT_OF_CHANNELS;

	//open uart
	UART_Open(psObj->uart, psInitDef->u32BaudRate);

	//line configure
	UART_SetLineConfig(psObj->uart, psInitDef->u32BaudRate, psInitDef->u32DataWidth, psInitDef->u32Parity, psInitDef->u32StopBits);

	//setup cts/rts if flow control enabled
	if(psInitDef->eFlowControl & eUART_HWCONTROL_CTS){
		(psObj->uart)->MODEMSTS  |= UART_MODEMSTS_CTSACTLV_Msk;
		u32IntFlag |= UART_INTEN_ATOCTSEN_Msk;
	}

	if(psInitDef->eFlowControl & eUART_HWCONTROL_RTS){
		(psObj->uart)->MODEM  |= UART_MODEM_RTSACTLV_Msk;
		u32IntFlag |= UART_INTEN_ATORTSEN_Msk;
		(psObj->uart)->FIFO = ((psObj->uart)->FIFO &~ UART_FIFO_RTSTRGLV_Msk) | UART_FIFO_RTSTRGLV_14BYTES;	
	}
	
	psUARTVar->i32RefCnt ++;
	psUARTVar->obj = psObj;

	if(IRQHandler){
		(psObj->uart)->FIFO = ((psObj->uart)->FIFO & (~UART_FIFO_RFITL_Msk)) | UART_FIFO_RFITL_1BYTE;

		UART_SetTimeoutCnt(psObj->uart, 40);

		u32IntFlag |= UART_INTEN_RDAIEN_Msk|UART_INTEN_RXTOIEN_Msk;
		psObj->hdlr_async = IRQHandler;

		UART_ENABLE_INT(psObj->uart, u32IntFlag);
		NVIC_EnableIRQ(modinit->irq_n);
	}
	else{
		psObj->hdlr_async = 0;
	}

	
	return 0;
}


void UART_Final(uart_t *psObj)
{
    if (psObj->dma_chn_id_tx != DMA_ERROR_OUT_OF_CHANNELS) {
        dma_channel_free(psObj->dma_chn_id_tx);
        psObj->dma_chn_id_tx = DMA_ERROR_OUT_OF_CHANNELS;
    }
    if (psObj->dma_chn_id_rx != DMA_ERROR_OUT_OF_CHANNELS) {
        dma_channel_free(psObj->dma_chn_id_rx);
        psObj->dma_chn_id_rx = DMA_ERROR_OUT_OF_CHANNELS;
    }

	UART_DisableFlowCtrl(psObj->uart);
	UART_DisableInt(psObj->uart, (UART_INTEN_RDAIEN_Msk|UART_INTEN_RXTOIEN_Msk));
	UART_Close(psObj->uart);

	psObj->hdlr_async = 0;

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->uart, uart_modinit_tab);
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


    if(u32INTSTS & UART_INTSTS_HWRLSIF_Msk)
    {
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

static void UART_DMA_Handler_TX(uint32_t id, uint32_t event_dma)
{
    uart_t *obj = (uart_t *) id;

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->uart, uart_modinit_tab);
    if(modinit == NULL)
		return;

	struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;
	
	if(psUARTVar->obj != obj)
		return;

    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_ABORT) {
    }
    
    // Expect UART IRQ will catch this transfer done event
    if (event_dma & DMA_EVENT_TRANSFER_DONE) {
		if (obj && obj->hdlr_dma_tx) {
			obj->hdlr_dma_tx(obj, event_dma);
		}
 
    }
    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_TIMEOUT) {

    }
}

static void UART_DMA_Handler_RX(uint32_t id, uint32_t event_dma)
{
	//TODO:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//	printf("UART_DMA_Handler_RX \n");
	
    uart_t *obj = (uart_t *) id;

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->uart, uart_modinit_tab);
    if(modinit == NULL)
		return;

	struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;
	
	if(psUARTVar->obj != obj)
		return;

    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_ABORT) {
    }

//    if ((event_dma & DMA_EVENT_TIMEOUT) && (!UART_GET_RX_EMPTY(obj->uart))){
//		return;
 //   }

    // Expect UART IRQ will catch this transfer done event
    if (event_dma & ( DMA_EVENT_TIMEOUT | DMA_EVENT_TRANSFER_DONE )) {
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
	const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->uart, uart_modinit_tab);
	if(modinit == NULL)
		return -1;


    if (psObj->dma_usage == DMA_USAGE_NEVER) {
        if (psObj->dma_chn_id_rx == DMA_ERROR_OUT_OF_CHANNELS) {
            psObj->dma_chn_id_rx = dma_channel_allocate(DMA_CAP_NONE);
        }

        if (psObj->dma_chn_id_tx == DMA_ERROR_OUT_OF_CHANNELS) {
            psObj->dma_chn_id_tx = dma_channel_allocate(DMA_CAP_NONE);
        }

        if (psObj->dma_chn_id_tx == DMA_ERROR_OUT_OF_CHANNELS || psObj->dma_chn_id_rx == DMA_ERROR_OUT_OF_CHANNELS) {
            psObj->dma_usage = DMA_USAGE_NEVER;
        }
        else{
            psObj->dma_usage = DMA_USAGE_ALWAYS;
		}
    }

	if(psObj->dma_usage != DMA_USAGE_NEVER){

		psObj->hdlr_dma_tx = pfnHandlerDMATx;
		psObj->hdlr_dma_rx = pfnHandlerDMARx;
		
		dma_set_handler ( psObj->dma_chn_id_rx,
							  (uint32_t)UART_DMA_Handler_RX,
							  (uint32_t)psObj,
							  DMA_EVENT_TRANSFER_DONE|DMA_EVENT_TIMEOUT );	

		dma_set_handler ( psObj->dma_chn_id_tx,
							  (uint32_t)UART_DMA_Handler_TX,
							  (uint32_t)psObj,
							  DMA_EVENT_TRANSFER_DONE|DMA_EVENT_TIMEOUT );	

		/* Enable Interrupt */
		UART_ENABLE_INT(psObj->uart, UART_INTEN_RLSIEN_Msk);
		NVIC_EnableIRQ(modinit->irq_n);
	}

	return 0;
}


int32_t UART_DMA_TXRX_Disable(
	uart_t *psObj
)
{
    if (psObj->dma_usage != DMA_USAGE_NEVER) {
		dma_unset_handler(psObj->dma_chn_id_tx);
		dma_unset_handler(psObj->dma_chn_id_rx);

		psObj->hdlr_dma_tx = NULL;
		psObj->hdlr_dma_rx = NULL;

        dma_channel_free(psObj->dma_chn_id_tx);
        psObj->dma_chn_id_tx = DMA_ERROR_OUT_OF_CHANNELS;
        dma_channel_free(psObj->dma_chn_id_rx);
        psObj->dma_chn_id_rx = DMA_ERROR_OUT_OF_CHANNELS;
        psObj->dma_usage = DMA_USAGE_NEVER;
    }
	return 0;
}

int32_t UART_DMA_RX_Start(
	uart_t *psObj,
	uint8_t *pu8DestBuf,
	int32_t i32TriggerLen
)
{

	const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->uart, uart_modinit_tab);
	if(modinit == NULL)
		return -1;

	struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;
	if(psUARTVar->obj != psObj)
		return -2;

	dma_fill_description (	psObj->dma_chn_id_rx,
                                (uint32_t)psUARTVar->pdma_perp_rx,
                                8,
                                (uint32_t)&(psObj->uart)->DAT,
                                (uint32_t)pu8DestBuf,
                                i32TriggerLen, //half buffer
                                5000, 0);
    //(1000000*40)/115200 );

    UART_ENABLE_INT(psObj->uart, UART_INTEN_RXPDMAEN_Msk); // Start DMA transfe
	return 0;
}

int32_t UART_DMA_RX_Stop(
	uart_t *psObj
)
{
	const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->uart, uart_modinit_tab);
	if(modinit == NULL)
		return -1;

	struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;
	if(psUARTVar->obj != psObj)
		return -2;


    UART_DISABLE_INT(psObj->uart, UART_INTEN_RXPDMAEN_Msk); // Stop DMA transfer
	return 0;
}

int32_t UART_DMA_TX_Start(
	uart_t *psObj,
	uint8_t *pu8SrcBuf,
	int32_t i32TriggerLen
)
{
	const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->uart, uart_modinit_tab);
	if(modinit == NULL)
		return -1;

	struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;
	if(psUARTVar->obj != psObj)
		return -2;

	dma_fill_description (	psObj->dma_chn_id_tx,
                                (uint32_t)psUARTVar->pdma_perp_tx,
                                8,
                                (uint32_t)pu8SrcBuf,
                                (uint32_t)&(psObj->uart)->DAT,
                                i32TriggerLen,
                                0, 0);

    UART_ENABLE_INT(psObj->uart, UART_INTEN_TXPDMAEN_Msk); // Start DMA transfe
	return 0;
}

int32_t UART_DMA_TX_Stop(
	uart_t *psObj
)
{
	const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->uart, uart_modinit_tab);
	if(modinit == NULL)
		return -1;

	struct nu_uart_var *psUARTVar = (struct nu_uart_var *)modinit->var;
	if(psUARTVar->obj != psObj)
		return -2;

    UART_DISABLE_INT(psObj->uart, UART_INTEN_TXPDMAEN_Msk); // Stop DMA transfer
	return 0;
}

