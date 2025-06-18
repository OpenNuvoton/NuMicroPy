/**************************************************************************//**
 * @file     M55M1_UART.h
 * @version  V1.00
 * @brief    M55M1 UART HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M55M1_UART_H__
#define __M55M1_UART_H__

#include "buffer.h"
#include "drv_pdma.h"

typedef enum {
    eUART_HWCONTROL_NONE = 0,
    eUART_HWCONTROL_CTS = 1,
    eUART_HWCONTROL_RTS = 2,
} E_UART_HWCONTROL;

typedef struct {
    uint32_t u32BaudRate;
    uint32_t u32DataWidth;
    uint32_t u32Parity;
    uint32_t u32StopBits;
    E_UART_HWCONTROL eFlowControl;
} UART_InitTypeDef;

typedef void(*PFN_HDLR_DMA_TX)(void *, int);
typedef void(*PFN_HDLR_DMA_RX)(void *, int);


typedef struct {
    union {
        UART_T *uart;
        LPUART_T *lpuart;
    } u_uart;
    bool bLPUART;
    E_PDMAUsage dma_usage;
    int         dma_chn_id_tx;
    int         dma_chn_id_rx;
    uint32_t    event;
    uint32_t    hdlr_async;
    PFN_HDLR_DMA_TX    hdlr_dma_tx;
    PFN_HDLR_DMA_RX    hdlr_dma_rx;
    struct buffer_s tx_buff; /**< Tx buffer */
    struct buffer_s rx_buff; /**< Rx buffer */
} uart_t;


int32_t UART_Init(
    uart_t *psObj,
    UART_InitTypeDef *psInitDef,
    uint32_t IRQHandler
);

void UART_Final(uart_t *psObj);

void Handle_UART_Irq(UART_T *uart);
void Handle_LPUART_Irq(LPUART_T *lpuart);

int32_t UART_DMA_TXRX_Enable(
    uart_t *psObj,
    PFN_HDLR_DMA_TX pfnHandlerDMATx,
    PFN_HDLR_DMA_RX pfnHandlerDMARx
);

int32_t UART_DMA_TXRX_Disable(
    uart_t *psObj
);

int32_t UART_DMA_RX_Start(
    uart_t *psObj,
    uint8_t *pu8DestBuf,
    int32_t i32TriggerLen
);

int32_t UART_DMA_RX_Stop(
    uart_t *psObj
);

int32_t UART_DMA_TX_Start(
    uart_t *psObj,
    uint8_t *pu8SrcBuf,
    int32_t i32TriggerLen
);

int32_t UART_DMA_TX_Stop(
    uart_t *psObj
);

#endif
