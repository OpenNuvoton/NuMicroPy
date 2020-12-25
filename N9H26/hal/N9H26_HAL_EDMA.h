/**************************************************************************//**
 * @file     N9H26_EDMA.h
 * @version  V1.00
 * @brief    N9H26 EDMA HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __N9H26_HAL_EDMA_H__
#define __N9H26_HAL_EDMA_H__

#include <inttypes.h>

#include "wblib.h"
#include "DrvEDMA.h"

#define NU_PDMA_EVENT_ABORT				(1 << 0)
#define NU_PDMA_EVENT_TRANSFER_DONE		(1 << 1)
#define NU_PDMA_EVENT_WRA_EMPTY			(1 << 2)
#define NU_PDMA_EVENT_WRA_THREE_FOURTHS	(1 << 3)
#define NU_PDMA_EVENT_WRA_HALF			(1 << 4)
#define NU_PDMA_EVENT_WRA_QUARTER		(1 << 5)
//#define NU_PDMA_EVENT_TIMEOUT			(1 << 6)

#define NU_PDMA_EVENT_ALL				(NU_PDMA_EVENT_ABORT | NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_WRA_EMPTY | NU_PDMA_EVENT_WRA_THREE_FOURTHS | NU_PDMA_EVENT_WRA_HALF | NU_PDMA_EVENT_WRA_QUARTER)
//#define NU_PDMA_EVENT_ALL				(NU_PDMA_EVENT_ABORT | NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_TIMEOUT | NU_PDMA_EVENT_WRA_EMPTY | NU_PDMA_EVENT_WRA_THREE_FOURTHS | NU_PDMA_EVENT_WRA_HALF | NU_PDMA_EVENT_WRA_QUARTER)
#define NU_PDMA_EVENT_MASK              NU_PDMA_EVENT_ALL
#define NU_PDMA_UNUSED                  (-1)

typedef enum
{
	EDMA_MEM,	
	EDMA_SPIMS0_TX,
	EDMA_SPIMS1_TX,  
	EDMA_UART0_TX,	  
	EDMA_UART1_TX,
	EDMA_RF_CODEC_TX,
	EDMA_RS_CODEC_TX,
	
	EDMA_SPIMS0_RX = 0x00008000,
	EDMA_SPIMS1_RX,  
	EDMA_UART0_RX,	  
	EDMA_UART1_RX,
	EDMA_ADC_RX,
	EDMA_RF_CODEC_RX,
	EDMA_RS_CODEC_RX,
}E_EDMA_PERIPH_TYPE;

typedef enum
{
    eMemCtl_SrcFix_DstFix,
    eMemCtl_SrcFix_DstInc,
    eMemCtl_SrcInc_DstFix,
    eMemCtl_SrcInc_DstInc,

    eMemCtl_SrcFix_DstWrap,
    eMemCtl_SrcInc_DstWrap,
    eMemCtl_Undefined = (-1)
} nu_edma_memctrl_t;

typedef S_DRVEDMA_DESCRIPT_FORMAT nu_edma_desc_t;

typedef void (*nu_edma_cb_handler_t)(void *, uint32_t);


int nu_edma_channel_allocate(E_EDMA_PERIPH_TYPE ePeripType);
int nu_edma_channel_free(int i32ChannID);
int nu_edma_callback_register(int i32ChannID, nu_edma_cb_handler_t pfnHandler, void *pvUserData, uint32_t u32EventFilter);
int nu_edma_transfer(
	int i32ChannID,
	uint32_t u32DataWidth,
	uint32_t u32AddrSrc,
	uint32_t u32AddrDst,
	int32_t i32TransferCnt,
	E_DRVEDMA_WRAPAROUND_SELECT eWrapSelect
);
int nu_edma_transferred_byte_get(int32_t i32ChannID, int32_t i32TriggerByteLen);
void nu_edma_channel_terminate(int i32ChannID);
nu_edma_memctrl_t nu_edma_channel_memctrl_get(int i32ChannID);
int nu_edma_channel_memctrl_set(int i32ChannID, nu_edma_memctrl_t eMemCtrl);
nu_edma_cb_handler_t nu_edma_callback_hijack(
	int i32ChannID,
	nu_edma_cb_handler_t *ppfnHandler_Hijack,
	void **ppvUserData_Hijack,
	uint32_t *pu32Events_Hijack
);

// For memory actor
void nu_edma_memfun_actor_init(void);
void *nu_edma_memcpy(void *dest, void *src, unsigned int count);
int nu_edma_mempush(void *dest, void *src, unsigned int transfer_byte);

#endif
