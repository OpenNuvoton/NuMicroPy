/**************************************************************************//**
 * @file     M48x_I2S.c
 * @version  V0.01
 * @brief    M480 series I2S HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"
#include "nu_modutil.h"
#include "nu_miscutil.h"
#include "nu_bitutil.h"


#include "dma.h"
#include "M48x_I2S.h"

extern void I2S0_IRQHandler(void);

typedef struct dma_desc_t
{
    uint32_t ctl;
    uint32_t src;
    uint32_t dest;
    uint32_t offset;
    uint32_t res1;
    uint32_t res2;
    uint32_t res3;
    uint32_t res4;
} DMA_DESC_T;

struct nu_i2s_var {
    i2s_t *     obj;
    void        (*vec)(void);
    uint8_t     pdma_perp_tx;
    uint8_t     pdma_perp_rx;
	uint8_t 	Buff[16] __attribute__((aligned(32)));       /* Working buffer */
    DMA_DESC_T DMA_DESC_TX[2] __attribute__((aligned(32)));
    DMA_DESC_T DMA_DESC_RX[2] __attribute__((aligned(32)));
};


static struct nu_i2s_var i2s0_var = {
	.obj                =   NULL,
	.vec                =   I2S0_IRQHandler,
    .pdma_perp_tx       =   PDMA_I2S0_TX,
    .pdma_perp_rx       =   PDMA_I2S0_RX
};


static const struct nu_modinit_s i2s_modinit_tab[] = {
    {(uint32_t)I2S0, I2S0_MODULE, CLK_CLKSEL3_I2S0SEL_HXT, MODULE_NoMsk, I2S0_RST, I2S0_IRQn, &i2s0_var},

    {0, 0, 0, 0, 0, (IRQn_Type) 0, NULL}
};


static void i2s_check_dma_usage(DMAUsage *dma_usage, int *dma_ch_tx, int *dma_ch_rx)
{
    if (*dma_usage != DMA_USAGE_NEVER) {
        if (*dma_ch_tx == DMA_ERROR_OUT_OF_CHANNELS) {
            *dma_ch_tx = dma_channel_allocate(DMA_CAP_NONE);
        }
        if (*dma_ch_rx == DMA_ERROR_OUT_OF_CHANNELS) {
            *dma_ch_rx = dma_channel_allocate(DMA_CAP_NONE);
        }

        if (*dma_ch_tx == DMA_ERROR_OUT_OF_CHANNELS || *dma_ch_rx == DMA_ERROR_OUT_OF_CHANNELS) {
            *dma_usage = DMA_USAGE_NEVER;
        }
    }

    if (*dma_usage == DMA_USAGE_NEVER) {
        dma_channel_free(*dma_ch_tx);
        *dma_ch_tx = DMA_ERROR_OUT_OF_CHANNELS;
        dma_channel_free(*dma_ch_rx);
        *dma_ch_rx = DMA_ERROR_OUT_OF_CHANNELS;
    }
}

static uint8_t i2s_get_data_width(i2s_t *obj)
{    
    I2S_T *i2s_base = (I2S_T *) obj->i2s;

    uint32_t data_width = ((i2s_base->CTL0 & I2S_CTL0_DATWIDTH_Msk) >> I2S_CTL0_DATWIDTH_Pos);
	if (data_width == 0) {
		data_width = 8;
	}
	else if(data_width == 1){
		data_width = 16;
	}
	else if(data_width == 2){
		data_width = 24;
	}
	else if(data_width == 3){
		data_width = 32;
	}

    return data_width;
}

/** Return FIFO depth of the I2S peripheral
 *
 * @details
 *  M487
 *      I2S0            16
 */
static uint32_t i2s_fifo_depth(i2s_t *obj)
{
	return 16;
}


static void i2s_enable_event(i2s_t *obj, uint32_t event, uint8_t enable)
{
    obj->event &= ~I2S_EVENT_ALL;
}

static void i2s_buffer_set(i2s_t *obj, const void *tx, size_t tx_length, void *rx, size_t rx_length)
{
    obj->tx_buff.buffer = (void *) tx;
    obj->tx_buff.length = tx_length;
    obj->tx_buff.pos = 0;
    obj->tx_buff.width = i2s_get_data_width(obj);
    obj->rx_buff.buffer = rx;
    obj->rx_buff.length = rx_length;
    obj->rx_buff.pos = 0;
    obj->rx_buff.width = i2s_get_data_width(obj);
}

static int i2s_is_tx_complete(i2s_t *obj)
{
    return (obj->tx_buff.pos == obj->tx_buff.length);
}

static int i2s_is_rx_complete(i2s_t *obj)
{
    return (obj->rx_buff.pos == obj->rx_buff.length);
}

int is_i2s_trans_done(i2s_t *obj){
    I2S_T *i2s_base = (I2S_T *) obj->i2s;

    if (i2s_is_tx_complete(obj) && (I2S_GET_TX_FIFO_LEVEL(i2s_base) == 0) && i2s_is_rx_complete(obj)) {
		return 1;
	}
	return 0;
}

static void i2s_enable_interrupt(i2s_t *obj, uint8_t enable)
{
    I2S_T *i2s_base = (I2S_T *) obj->i2s;

    if (enable) {
		uint32_t u32IntFlag = 0;

		if(obj->tx_buff.length){
			u32IntFlag |= I2S_IEN_TXTHIEN_Msk;
		}
		
		if(obj->rx_buff.length){
			u32IntFlag |= I2S_IEN_RXTHIEN_Msk;
		}

        // Enable tx/rx FIFO threshold interrupt
        I2S_EnableInt(i2s_base, u32IntFlag);

		if(u32IntFlag & I2S_IEN_TXTHIEN_Msk)
			I2S_ENABLE_TX(i2s_base);
			
		if(u32IntFlag & I2S_IEN_RXTHIEN_Msk)
			I2S_ENABLE_RX(i2s_base);
        
    } else {
        I2S_DisableInt(i2s_base, I2S_IEN_TXTHIEN_Msk | I2S_IEN_RXTHIEN_Msk);
		I2S_DISABLE_TX(i2s_base);
		I2S_DISABLE_TX(i2s_base);
    }
}

static void i2s_enable_vector_interrupt(i2s_t *obj, pfnTransHandler handler, uint8_t enable)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->i2s, i2s_modinit_tab);
    if(modinit == NULL)
		return;
    if(modinit->modname != (int) obj->i2s)
		return;
    
    struct nu_i2s_var *var = (struct nu_i2s_var *) modinit->var;
    
    if (enable) {
        var->obj = obj;
        obj->hdl = handler;
        /* Fixed vector table is fixed in ROM and cannot be modified. */
        NVIC_EnableIRQ(modinit->irq_n);
    }
    else {
        NVIC_DisableIRQ(modinit->irq_n);
        /* Fixed vector table is fixed in ROM and cannot be modified. */
        var->obj = NULL;
        obj->hdl = 0;
    }
}


/**
 * Send words from the I2S TX buffer until the send limit is reached or the TX FIFO is full
 * tx_limit is provided to ensure that the number of I2S frames (words) in flight can be managed.
 * @param[in] obj       The I2S object on which to operate
 * @param[in] tx_limit  The maximum number of words to send
 * @return The number of I2S words that have been transfered
 */
static uint32_t i2s_write_asynch(i2s_t *obj, uint32_t tx_limit)
{
    uint32_t n_words = 0;
    uint32_t tx_rmn = obj->tx_buff.length - obj->tx_buff.pos;
    uint32_t rx_rmn = obj->rx_buff.length - obj->rx_buff.pos;
    uint32_t max_tx = NU_MAX(tx_rmn, rx_rmn);
    max_tx = NU_MIN(max_tx, tx_limit);

    uint8_t data_width = i2s_get_data_width(obj);
    uint8_t bytes_per_word = (data_width + 7) / 8;

    uint8_t *tx = (uint8_t *)(obj->tx_buff.buffer) + obj->tx_buff.pos;
    I2S_T *i2s_base = (I2S_T *)obj->i2s;

    while (n_words < max_tx) {
        if (i2s_is_tx_complete(obj)) {
            // Transmit dummy as transmit buffer is empty
            I2S_WRITE_TX_FIFO(i2s_base, 0);
        } else {
            switch (bytes_per_word) {
            case 4:
                I2S_WRITE_TX_FIFO(i2s_base, nu_get32_le(tx));
                tx += 4;
                break;
            case 2:
                I2S_WRITE_TX_FIFO(i2s_base, nu_get16_le(tx));
                tx += 2;
                break;
            case 1:
                I2S_WRITE_TX_FIFO(i2s_base, *((uint8_t *) tx));
                tx += 1;
                break;
            }

            obj->tx_buff.pos += bytes_per_word;	
		}
        n_words ++;
	}

	return n_words;
}

/**
 * Read I2S words out of the RX FIFO
 * Continues reading words out of the RX FIFO until the following condition is met:
 * o There are no more words in the FIFO
 * OR BOTH OF:
 * o At least as many words as the TX buffer have been received
 * o At least as many words as the RX buffer have been received
 * This way, RX overflows are not generated when the TX buffer size exceeds the RX buffer size
 * @param[in] obj The I2S object on which to operate
 * @return Returns the number of words extracted from the RX FIFO
 */
static uint32_t i2s_read_asynch(i2s_t *obj, uint32_t rx_limit)
{
    uint32_t n_words = 0;
    uint32_t tx_rmn = obj->tx_buff.length - obj->tx_buff.pos;
    uint32_t rx_rmn = obj->rx_buff.length - obj->rx_buff.pos;
    uint32_t max_rx = NU_MAX(tx_rmn, rx_rmn);

    max_rx = NU_MIN(max_rx, rx_limit);

    uint8_t data_width = i2s_get_data_width(obj);
    uint8_t bytes_per_word = (data_width + 7) / 8;

    uint8_t *rx = (uint8_t *)(obj->rx_buff.buffer) + obj->rx_buff.pos;
    I2S_T *i2s_base = (I2S_T *)obj->i2s;


    while (n_words < max_rx) {
        if (i2s_is_rx_complete(obj)) {
            // Disregard as receive buffer is full
            I2S_READ_RX_FIFO(i2s_base);
        } else {

            switch (bytes_per_word) {
            case 4: {
                uint32_t val = I2S_READ_RX_FIFO(i2s_base);
                nu_set32_le(rx, val);
                rx += 4;
                break;
            }
            case 2: {
                uint16_t val = I2S_READ_RX_FIFO(i2s_base);
                nu_set16_le(rx, val);
                rx += 2;
                break;
            }
            case 1:
                *rx ++ = I2S_READ_RX_FIFO(i2s_base);
                break;
            }

            obj->rx_buff.pos += bytes_per_word;
		}

        n_words ++;
	}
	return n_words;
}


static uint32_t i2s_event_check(i2s_t *obj)
{
    I2S_T *i2s_base = (I2S_T *) obj->i2s;
    uint32_t event = 0;
    //uint32_t u32Reg;

	//u32Reg = I2S_GET_INT_FLAG(i2s_base, I2S_STATUS0_TXTHIF_Msk | I2S_STATUS0_RXTHIF_Msk);

    if (obj->dma_usage == DMA_USAGE_NEVER) {
        i2s_read_asynch(obj, I2S_GET_RX_FIFO_LEVEL(i2s_base));
        i2s_write_asynch(obj, i2s_fifo_depth(obj) - I2S_GET_TX_FIFO_LEVEL(i2s_base));
    }

	if(is_i2s_trans_done(obj)){
        event |= I2S_EVENT_COMPLETE;
    }

    // Receive FIFO Overrun
    if (i2s_base->STATUS0 & I2S_STATUS0_RXOVIF_Msk) {
        i2s_base->STATUS0 = I2S_STATUS0_RXOVIF_Msk;
        // In case of tx length > rx length on DMA way
        if (obj->dma_usage == DMA_USAGE_NEVER) {
            event |= I2S_EVENT_RX_OVERFLOW;
        }
    }

    // Transmit FIFO Under-Run
    if (i2s_base->STATUS0 & I2S_STATUS0_TXUDIF_Msk) {
        i2s_base->STATUS0 = I2S_STATUS0_TXUDIF_Msk;
        event |= I2S_EVENT_ERROR;
    }

    return event;
}

static void i2s_dma_handler_tx(uint32_t id, uint32_t event_dma)
{
    i2s_t *obj = (i2s_t *) id;

//	printf("i2s_dma_handler_tx \n");

    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_ABORT) {
		obj->event = I2S_EVENT_ERROR;
    }
    // Expect SPI IRQ will catch this transfer done event
    if (event_dma & DMA_EVENT_TRANSFER_DONE) {
		obj->event = I2S_EVENT_DMA_TX_DONE;
    }
    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_TIMEOUT) {
		obj->event = I2S_EVENT_ERROR;
    }

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->i2s, i2s_modinit_tab);
    if(modinit == NULL)
		return;
    if(modinit->modname != (int) obj->i2s)
		return;

    if (obj && obj->hdl) {
       obj->hdl(obj);
    }
}

static void i2s_dma_handler_rx(uint32_t id, uint32_t event_dma)
{
    i2s_t *obj = (i2s_t *) id;

    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_ABORT) {
		obj->event = I2S_EVENT_ERROR;
    }
    // Expect SPI IRQ will catch this transfer done event
    if (event_dma & DMA_EVENT_TRANSFER_DONE) {
		obj->event = I2S_EVENT_DMA_RX_DONE;
    }
    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_TIMEOUT) {
		obj->event = I2S_EVENT_ERROR;
    }

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->i2s, i2s_modinit_tab);
    if(modinit == NULL)
		return;
    if(modinit->modname != (int) obj->i2s)
		return;

    if (obj && obj->hdl) {
       obj->hdl(obj);
    }
}


int32_t I2S_Init(
	i2s_t *psObj,
	I2S_InitTypeDef *psInitDef
)
{

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->i2s, i2s_modinit_tab);
    I2S_T *i2s_base = (I2S_T *) psObj->i2s;

	if(modinit == NULL)
		return -1;

   // Reset this module
    SYS_ResetModule(modinit->rsetidx);

    // Select IP clock source
    CLK_SetModuleClock(modinit->clkidx, modinit->clksrc, modinit->clkdiv);
    // Enable IP clock
    CLK_EnableModuleClock(modinit->clkidx);

    psObj->dma_usage = DMA_USAGE_NEVER;
    psObj->event = 0;
    psObj->hdl = 0;
    psObj->dma_chn_id_tx = DMA_ERROR_OUT_OF_CHANNELS;
    psObj->dma_chn_id_rx = DMA_ERROR_OUT_OF_CHANNELS;
    
    /* Open I2S0 interface and set to slave mode, stereo channel, I2S format */
    I2S_Open(psObj->i2s, psInitDef->Mode, psInitDef->SampleRate, psInitDef->WordWidth, psInitDef->MonoData, psInitDef->DataFormat);

    /* Set MCLK and enable MCLK */
    I2S_EnableMCLK(psObj->i2s, 12000000);

    i2s_base->CTL0 |= I2S_CTL0_ORDER_Msk;

	return 0;
}


void I2S_Final(i2s_t *psObj)
{
	I2S_DisableInt(psObj->i2s, I2S_IEN_RXTHIEN_Msk | I2S_IEN_TXTHIEN_Msk);
	I2S_DisableMCLK(psObj->i2s);
	I2S_Close(psObj->i2s);
}


void I2S_StartTransfer(
	i2s_t *psObj,
	I2S_TransParam *psTransParam,
	pfnTransHandler handler, 
	uint32_t event, 
	DMAUsage hint
)
{
    psObj->dma_usage = hint;
    i2s_check_dma_usage((DMAUsage *)&psObj->dma_usage, &psObj->dma_chn_id_tx, &psObj->dma_chn_id_rx);

    i2s_enable_event(psObj, event, 1);
    i2s_buffer_set(psObj, psTransParam->tx, psTransParam->tx_length, psTransParam->rx, psTransParam->rx_length);

    if (psObj->dma_usage == DMA_USAGE_NEVER) {
        // Interrupt way
        i2s_write_asynch(psObj, i2s_fifo_depth(psObj));
        i2s_enable_vector_interrupt(psObj, handler, 1);
        i2s_enable_interrupt(psObj, 1);
    } else {
         // DMA way
		const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->i2s, i2s_modinit_tab);
		I2S_T *i2s_base = (I2S_T *) psObj->i2s;

		if(modinit == NULL)
			return;

		uint8_t data_width = i2s_get_data_width(psObj);
		struct nu_i2s_var *var = (struct nu_i2s_var *) modinit->var;
		
		//uint32_t u32DataWidth;
		//if(data_width ==  8)
		//	u32DataWidth = PDMA_WIDTH_8;
		//else if(data_width ==  16)
		//	u32DataWidth = PDMA_WIDTH_16;
		//else
		//	u32DataWidth = PDMA_WIDTH_32;
		
        var->obj = psObj;
        psObj->hdl = handler;

		//Fill TX desc
		if((psTransParam->tx_length) && (psTransParam->tx1_length)){
//			var->DMA_DESC_TX[0].ctl = ((psTransParam->tx_length - 1)<<PDMA_DSCT_CTL_TXCNT_Pos)|u32DataWidth|PDMA_SAR_INC|PDMA_DAR_FIX|PDMA_REQ_SINGLE|PDMA_OP_SCATTER;
			var->DMA_DESC_TX[0].ctl = ((psTransParam->tx_length - 1)<<PDMA_DSCT_CTL_TXCNT_Pos)|PDMA_WIDTH_32|PDMA_SAR_INC|PDMA_DAR_FIX|PDMA_REQ_SINGLE|PDMA_OP_SCATTER;
			var->DMA_DESC_TX[0].src = (uint32_t)psTransParam->tx;
			var->DMA_DESC_TX[0].dest = (uint32_t)&i2s_base->TXFIFO;
			var->DMA_DESC_TX[0].offset = (uint32_t)&var->DMA_DESC_TX[1] - (PDMA->SCATBA);

//			var->DMA_DESC_TX[1].ctl = ((psTransParam->tx1_length - 1)<<PDMA_DSCT_CTL_TXCNT_Pos)|u32DataWidth|PDMA_SAR_INC|PDMA_DAR_FIX|PDMA_REQ_SINGLE|PDMA_OP_SCATTER;
			var->DMA_DESC_TX[1].ctl = ((psTransParam->tx1_length - 1)<<PDMA_DSCT_CTL_TXCNT_Pos)|PDMA_WIDTH_32|PDMA_SAR_INC|PDMA_DAR_FIX|PDMA_REQ_SINGLE|PDMA_OP_SCATTER;
			var->DMA_DESC_TX[1].src = (uint32_t)psTransParam->tx1;
			var->DMA_DESC_TX[1].dest = (uint32_t)&i2s_base->TXFIFO;
			var->DMA_DESC_TX[1].offset = (uint32_t)&var->DMA_DESC_TX[0] - (PDMA->SCATBA);
			printf("var->DMA_DESC_TX[0].offset %lx \n", var->DMA_DESC_TX[0].offset);
			printf("var->DMA_DESC_TX[1].offset %lx \n", var->DMA_DESC_TX[1].offset);

			dma_fill_description (psObj->dma_chn_id_tx,
									var->pdma_perp_tx,
									data_width,
									(uint32_t)&var->DMA_DESC_TX[0],
									0,
									0,
									0, 1);

			// Register DMA event handler
			dma_set_handler(psObj->dma_chn_id_tx, (uint32_t) i2s_dma_handler_tx, (uint32_t) psObj, DMA_EVENT_ALL);

			/* Enable I2S Tx function */
			I2S_ENABLE_TXDMA(i2s_base);
			I2S_ENABLE_TX(i2s_base);
		}

		//Fill RX desc
		if((psTransParam->rx_length) && (psTransParam->rx1_length)){
//			var->DMA_DESC_RX[0].ctl = ((psTransParam->rx_length - 1)<<PDMA_DSCT_CTL_TXCNT_Pos)|u32DataWidth|PDMA_SAR_FIX|PDMA_DAR_INC|PDMA_REQ_SINGLE|PDMA_OP_SCATTER;
			var->DMA_DESC_RX[0].ctl = ((psTransParam->rx_length - 1)<<PDMA_DSCT_CTL_TXCNT_Pos)|PDMA_WIDTH_32|PDMA_SAR_FIX|PDMA_DAR_INC|PDMA_REQ_SINGLE|PDMA_OP_SCATTER;
			var->DMA_DESC_RX[0].src = (uint32_t)&i2s_base->RXFIFO;
			var->DMA_DESC_RX[0].dest = (uint32_t)psTransParam->rx;
			var->DMA_DESC_RX[0].offset = (uint32_t)&var->DMA_DESC_RX[1] - (PDMA->SCATBA);

//			var->DMA_DESC_RX[1].ctl = ((psTransParam->rx1_length - 1)<<PDMA_DSCT_CTL_TXCNT_Pos)|u32DataWidth|PDMA_SAR_FIX|PDMA_DAR_INC|PDMA_REQ_SINGLE|PDMA_OP_SCATTER;
			var->DMA_DESC_RX[1].ctl = ((psTransParam->rx1_length - 1)<<PDMA_DSCT_CTL_TXCNT_Pos)|PDMA_WIDTH_32|PDMA_SAR_FIX|PDMA_DAR_INC|PDMA_REQ_SINGLE|PDMA_OP_SCATTER;
			var->DMA_DESC_RX[1].src = (uint32_t)&i2s_base->RXFIFO;
			var->DMA_DESC_RX[1].dest = (uint32_t)psTransParam->rx1;
			var->DMA_DESC_RX[1].offset = (uint32_t)&var->DMA_DESC_RX[0] - (PDMA->SCATBA);

			dma_fill_description (psObj->dma_chn_id_rx,
									var->pdma_perp_rx,
									data_width,
									(uint32_t)&var->DMA_DESC_RX[0],
									0,
									0,
									0, 1);

			// Register DMA event handler
			dma_set_handler(psObj->dma_chn_id_rx, (uint32_t) i2s_dma_handler_rx, (uint32_t) psObj, DMA_EVENT_ALL);

			/* Enable I2S Rx function */
			I2S_ENABLE_RXDMA(i2s_base);
			I2S_ENABLE_RX(i2s_base);
		}
	}

}

void I2S_StopTransfer(
	i2s_t *psObj
)
{
    I2S_T *i2s_base = (I2S_T *) psObj->i2s;
    //PDMA_T *pdma_base = dma_modbase();


    if (psObj->dma_usage != DMA_USAGE_NEVER) {
		I2S_DISABLE_TXDMA(i2s_base);
		I2S_DISABLE_RXDMA(i2s_base);

		if (psObj->dma_chn_id_tx != DMA_ERROR_OUT_OF_CHANNELS) {
			dma_channel_terminate(psObj->dma_chn_id_tx);
			dma_channel_free(psObj->dma_chn_id_tx);
			dma_unset_handler(psObj->dma_chn_id_tx);
			psObj->dma_chn_id_tx = DMA_ERROR_OUT_OF_CHANNELS;
		}

		if (psObj->dma_chn_id_rx != DMA_ERROR_OUT_OF_CHANNELS) {
			dma_channel_terminate(psObj->dma_chn_id_rx);
			dma_channel_free(psObj->dma_chn_id_rx);
			dma_unset_handler(psObj->dma_chn_id_rx);
			psObj->dma_chn_id_rx = DMA_ERROR_OUT_OF_CHANNELS;
		}
	}

	// Necessary for both interrupt way and DMA way
	i2s_enable_vector_interrupt(psObj, 0, 0);
	i2s_enable_interrupt(psObj, 0);

	I2S_CLR_TX_FIFO(i2s_base);
	I2S_CLR_RX_FIFO(i2s_base);
}

/**
 * Handle the I2S interrupt for TX/RX transfer
 * Read frames until the RX FIFO is empty.  Write at most as many frames as were read.  This way,
 * it is unlikely that the RX FIFO will overflow.
 * @param[in] obj The I2S peripheral that generated the interrupt
 * @return
 */
void i2s_irq_handler_asynch(i2s_t *obj)
{
    // Check for I2S events
    uint32_t event = i2s_event_check(obj);
    if (event) {
        I2S_StopTransfer(obj);
    }

	obj->event = event | ((event & I2S_EVENT_COMPLETE) ? I2S_EVENT_INTERNAL_TRANSFER_COMPLETE : 0);
//    return (obj->event & event) | ((event & SPI_EVENT_COMPLETE) ? SPI_EVENT_INTERNAL_TRANSFER_COMPLETE : 0);
}

/**
 * Handle the I2S interrupt
 * Read frames until the RX FIFO is empty.  Write at most as many frames as were read.  This way,
 * it is unlikely that the RX FIFO will overflow.
 * @param[in] obj The SPI peripheral that generated the interrupt
 * @return
 */
 
void Handle_I2S_Irq(I2S_T *i2s)
{
	i2s_t *obj = NULL;
	if(i2s == I2S0)
		obj = i2s0_var.obj;

    if (obj && obj->hdl) {
        obj->hdl(obj);
    }
}



