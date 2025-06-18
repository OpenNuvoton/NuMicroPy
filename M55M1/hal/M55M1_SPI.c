/**************************************************************************//**
 * @file     M55M1_SPI.c
 * @version  V0.01
 * @brief    M55M1 series SPI HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "py/mphal.h"

#include "NuMicro.h"
#include "nu_modutil.h"
#include "nu_bitutil.h"
#include "nu_miscutil.h"

#include "M55M1_SPI.h"

#define DEVICE_SPI_ASYNCH (1)
#define DEVICE_SPISLAVE (1)

extern void SPI0_IRQHandler(void);
extern void SPI1_IRQHandler(void);
extern void SPI2_IRQHandler(void);
extern void SPI3_IRQHandler(void);
extern void LPSPI0_IRQHandler(void);

struct nu_spi_var {
    spi_t *     obj;
    void        (*vec)(void);
#if DEVICE_SPI_ASYNCH
    uint8_t     pdma_perp_tx;
    uint8_t     pdma_perp_rx;
#endif
};

static struct nu_spi_var spi0_var = {
    .obj                =   NULL,
    .vec                =   SPI0_IRQHandler,
#if DEVICE_SPI_ASYNCH
    .pdma_perp_tx       =   PDMA_SPI0_TX,
    .pdma_perp_rx       =   PDMA_SPI0_RX
#endif
};
static struct nu_spi_var spi1_var = {
    .obj                =   NULL,
    .vec                =   SPI1_IRQHandler,
#if DEVICE_SPI_ASYNCH
    .pdma_perp_tx       =   PDMA_SPI1_TX,
    .pdma_perp_rx       =   PDMA_SPI1_RX
#endif
};
static struct nu_spi_var spi2_var = {
    .obj                =   NULL,
    .vec                =   SPI2_IRQHandler,
#if DEVICE_SPI_ASYNCH
    .pdma_perp_tx       =   PDMA_SPI2_TX,
    .pdma_perp_rx       =   PDMA_SPI2_RX
#endif
};
static struct nu_spi_var spi3_var = {
    .obj                =   NULL,
    .vec                =   SPI3_IRQHandler,
#if DEVICE_SPI_ASYNCH
    .pdma_perp_tx       =   PDMA_SPI3_TX,
    .pdma_perp_rx       =   PDMA_SPI3_RX
#endif
};
static struct nu_spi_var spi4_var = {
    .obj                =   NULL,
    .vec                =   LPSPI0_IRQHandler,
#if DEVICE_SPI_ASYNCH
    .pdma_perp_tx       =   LPPDMA_LPSPI0_TX,
    .pdma_perp_rx       =   LPPDMA_LPSPI0_RX
#endif
};

static const struct nu_modinit_s spi_modinit_tab[] = {
    {(uint32_t)SPI0, SPI0_MODULE, CLK_SPISEL_SPI0SEL_PCLK0, MODULE_NoMsk, SYS_SPI0RST, SPI0_IRQn, &spi0_var},
    {(uint32_t)SPI1, SPI1_MODULE, CLK_SPISEL_SPI1SEL_PCLK2, MODULE_NoMsk, SYS_SPI1RST, SPI1_IRQn, &spi1_var},
    {(uint32_t)SPI2, SPI2_MODULE, CLK_SPISEL_SPI2SEL_PCLK0, MODULE_NoMsk, SYS_SPI2RST, SPI2_IRQn, &spi2_var},
    {(uint32_t)SPI3, SPI3_MODULE, CLK_SPISEL_SPI3SEL_PCLK2, MODULE_NoMsk, SYS_SPI3RST, SPI3_IRQn, &spi3_var},
    {(uint32_t)LPSPI0, LPSPI0_MODULE, CLK_LPSPISEL_LPSPI0SEL_PCLK4, MODULE_NoMsk, SYS_LPSPI0RST, LPSPI0_IRQn, &spi4_var},

    {0, 0, 0, 0, 0, (IRQn_Type) 0, NULL}
};


/* Synchronous version of SPI_ENABLE()/SPI_DISABLE() macros
 *
 * The SPI peripheral clock is asynchronous with the system clock. In order to make sure the SPI
 * control logic is enabled/disabled, this bit indicates the real status of SPI controller.
 *
 * NOTE: All configurations shall be ready before calling SPI_ENABLE_SYNC().
 * NOTE: Before changing the configurations of SPIx_CTL, SPIx_CLKDIV, SPIx_SSCTL and SPIx_FIFOCTL registers,
 *       user shall clear the SPIEN (SPIx_CTL[0]) and confirm the SPIENSTS (SPIx_STATUS[15]) is 0
 *       (by SPI_DISABLE_SYNC here).
 */
__STATIC_INLINE void SPI_ENABLE_SYNC(SPI_T *spi_base)
{
    if (! (spi_base->CTL & SPI_CTL_SPIEN_Msk)) {
        SPI_ENABLE(spi_base);
    }
    while (! (spi_base->STATUS & SPI_STATUS_SPIENSTS_Msk));
}
__STATIC_INLINE void SPI_DISABLE_SYNC(SPI_T *spi_base)
{
    if (spi_base->CTL & SPI_CTL_SPIEN_Msk) {
        // NOTE: SPI H/W may get out of state without the busy check.
        while (SPI_IS_BUSY(spi_base)) {
            MICROPY_THREAD_YIELD();
        }

        SPI_DISABLE(spi_base);
    }
    while (spi_base->STATUS & SPI_STATUS_SPIENSTS_Msk);
}

__STATIC_INLINE void LPSPI_ENABLE_SYNC(LPSPI_T *lpspi_base)
{
    if (! (lpspi_base->CTL & LPSPI_CTL_SPIEN_Msk)) {
        SPI_ENABLE(lpspi_base);
    }
    while (! (lpspi_base->STATUS & LPSPI_STATUS_SPIENSTS_Msk));
}
__STATIC_INLINE void LPSPI_DISABLE_SYNC(LPSPI_T *lpspi_base)
{
    if (lpspi_base->CTL & LPSPI_CTL_SPIEN_Msk) {
        // NOTE: SPI H/W may get out of state without the busy check.
        while (LPSPI_IS_BUSY(lpspi_base)) {
            MICROPY_THREAD_YIELD();
        }

        LPSPI_DISABLE(lpspi_base);
    }
    while (lpspi_base->STATUS & LPSPI_STATUS_SPIENSTS_Msk);
}

static void spi_format(SPI_T *spi_base, int bits, int mode, int slave, int msb_first, int direction)
{

    SPI_DISABLE_SYNC(spi_base);

//	printf("bits: %d \n", bits);
//	printf("mode: %d \n", mode);
//	printf("slave: %d \n", slave);
//	printf("msb_first: %d \n", msb_first);
//	printf("direction: %d \n", direction);

    SPI_Open(spi_base,
             slave,
             (mode == 0) ? SPI_MODE_0 : (mode == 1) ? SPI_MODE_1 : (mode == 2) ? SPI_MODE_2 : SPI_MODE_3,
             bits,
             0);


    if(msb_first == SPI_FIRSTBIT_MSB)
        SPI_SET_MSB_FIRST(spi_base);
    else
        SPI_SET_LSB_FIRST(spi_base);


    if (slave == SPI_MASTER) {
        // Master
        // Configure SS as low active.
        SPI_EnableAutoSS(spi_base, SPI_SS, SPI_SS_ACTIVE_LOW);
        if(direction == SPI_DIRECTION_2LINES_RXONLY) {
            spi_base->CTL |= SPI_CTL_RXONLY_Msk;
        }
    } else {
        // Slave
        // Configure SS as low active.
        spi_base->SSCTL &= ~SPI_SSCTL_SSACTPOL_Msk;
    }

    /* NOTE: M451's/M480's/M2351's SPI_Open() will enable SPI transfer (SPI_CTL_SPIEN_Msk).
     *       We cannot use SPI_CTL_SPIEN_Msk for judgement of spi_active().
     *       Judge with vector instead. */

    if(direction == SPI_DIRECTION_1LINE) {
        spi_base->CTL |= SPI_CTL_HALFDPX_Msk;
    }
}

static void lpspi_format(LPSPI_T *lpspi_base, int bits, int mode, int slave, int msb_first, int direction)
{

    LPSPI_DISABLE_SYNC(lpspi_base);

//	printf("bits: %d \n", bits);
//	printf("mode: %d \n", mode);
//	printf("slave: %d \n", slave);
//	printf("msb_first: %d \n", msb_first);
//	printf("direction: %d \n", direction);

    LPSPI_Open(lpspi_base,
               slave,
               (mode == 0) ? LPSPI_MODE_0 : (mode == 1) ? LPSPI_MODE_1 : (mode == 2) ? LPSPI_MODE_2 : LPSPI_MODE_3,
               bits,
               0);


    if(msb_first == SPI_FIRSTBIT_MSB)
        LPSPI_SET_MSB_FIRST(lpspi_base);
    else
        LPSPI_SET_LSB_FIRST(lpspi_base);


    if (slave == SPI_MASTER) {
        // Master
        // Configure SS as low active.
        LPSPI_EnableAutoSS(lpspi_base, LPSPI_SS, SPI_SS_ACTIVE_LOW);
        if(direction == SPI_DIRECTION_2LINES_RXONLY) {
            lpspi_base->CTL |= LPSPI_CTL_RXONLY_Msk;
        }
    } else {
        // Slave
        // Configure SS as low active.
        lpspi_base->SSCTL &= ~LPSPI_SSCTL_SSACTPOL_Msk;
    }

    /* NOTE: M451's/M480's/M2351's SPI_Open() will enable SPI transfer (SPI_CTL_SPIEN_Msk).
     *       We cannot use SPI_CTL_SPIEN_Msk for judgement of spi_active().
     *       Judge with vector instead. */

    if(direction == SPI_DIRECTION_1LINE) {
        lpspi_base->CTL |= LPSPI_CTL_HALFDPX_Msk;
    }
}


static void spi_frequency(SPI_T *spi_base, int hz)
{

    SPI_DISABLE_SYNC(spi_base);

    SPI_SetBusClock(spi_base, hz);
}

static void lpspi_frequency(LPSPI_T *lpspi_base, int hz)
{

    LPSPI_DISABLE_SYNC(lpspi_base);

    LPSPI_SetBusClock(lpspi_base, hz);
}

static int spi_readable(SPI_T *spi_base)
{
    return ! SPI_GET_RX_FIFO_EMPTY_FLAG(spi_base);
}

static int lpspi_readable(LPSPI_T *lpspi_base)
{
    return ! LPSPI_GET_RX_FIFO_EMPTY_FLAG(lpspi_base);
}

static int spi_writeable(SPI_T *spi_base)
{
    // Receive FIFO must not be full to avoid receive FIFO overflow on next transmit/receive
    return (! SPI_GET_TX_FIFO_FULL_FLAG(spi_base));
}

static int lpspi_writeable(LPSPI_T *lpspi_base)
{
    // Receive FIFO must not be full to avoid receive FIFO overflow on next transmit/receive
    return (! LPSPI_GET_TX_FIFO_FULL_FLAG(lpspi_base));
}

static uint8_t spi_get_data_width(spi_t *obj)
{
    SPI_T *spi_base = (SPI_T *) obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *) obj->u_spi.lpspi;
    uint32_t data_width;

    if(obj->bLPSPI)
        data_width = ((lpspi_base->CTL & LPSPI_CTL_DWIDTH_Msk) >> LPSPI_CTL_DWIDTH_Pos);
    else
        data_width = ((spi_base->CTL & SPI_CTL_DWIDTH_Msk) >> SPI_CTL_DWIDTH_Pos);

    if (data_width == 0) {
        data_width = 32;
    }

    return data_width;
}

static void spi_check_dma_usage(spi_t *obj, E_PDMAUsage *dma_usage, int *dma_ch_tx, int *dma_ch_rx)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->u_spi.spi, spi_modinit_tab);
    if(modinit == NULL)
        return;

    struct nu_spi_var *psSpiVar = (struct nu_spi_var *)modinit->var;

    if (*dma_usage != ePDMA_USAGE_NEVER) {
//printf("spi_check_dma_usage 0 \n");
        if (*dma_ch_tx == NU_PDMA_OUT_OF_CHANNELS) {
            if(obj->bLPSPI)
                *dma_ch_tx = nu_lppdma_channel_dynamic_allocate(psSpiVar->pdma_perp_tx);
            else
                *dma_ch_tx = nu_pdma_channel_dynamic_allocate(psSpiVar->pdma_perp_tx);
        }
        if (*dma_ch_rx == NU_PDMA_OUT_OF_CHANNELS) {
            if(obj->bLPSPI)
                *dma_ch_rx = nu_lppdma_channel_dynamic_allocate(psSpiVar->pdma_perp_rx);
            else
                *dma_ch_rx = nu_pdma_channel_dynamic_allocate(psSpiVar->pdma_perp_rx);
        }

        if (*dma_ch_tx == NU_PDMA_OUT_OF_CHANNELS || *dma_ch_rx == NU_PDMA_OUT_OF_CHANNELS) {
            *dma_usage = ePDMA_USAGE_NEVER;
        }
    }

    if (*dma_usage == ePDMA_USAGE_NEVER) {
//printf("spi_check_dma_usage 1 \n");
        if(obj->bLPSPI) {
            if(*dma_ch_tx != NU_PDMA_OUT_OF_CHANNELS)
                nu_lppdma_channel_free(*dma_ch_tx);

            if(*dma_ch_rx != NU_PDMA_OUT_OF_CHANNELS)
                nu_lppdma_channel_free(*dma_ch_rx);
        } else {
            if(*dma_ch_tx != NU_PDMA_OUT_OF_CHANNELS)
                nu_pdma_channel_free(*dma_ch_tx);

            if(*dma_ch_rx != NU_PDMA_OUT_OF_CHANNELS)
                nu_pdma_channel_free(*dma_ch_rx);
        }

        *dma_ch_tx = NU_PDMA_OUT_OF_CHANNELS;
        *dma_ch_rx = NU_PDMA_OUT_OF_CHANNELS;
    }
}

static void spi_enable_event(spi_t *obj, uint32_t event, uint8_t enable)
{
    obj->event &= ~SPI_EVENT_ALL;

    if (event & SPI_EVENT_RX_OVERFLOW) {
        if(obj->bLPSPI)
            LPSPI_EnableInt(obj->u_spi.lpspi, LPSPI_FIFOCTL_RXOVIEN_Msk);
        else
            SPI_EnableInt(obj->u_spi.spi, SPI_FIFOCTL_RXOVIEN_Msk);
    }
}

static void spi_buffer_set(spi_t *obj, const void *tx, size_t tx_length, void *rx, size_t rx_length)
{
    obj->tx_buff.buffer = (void *) tx;
    obj->tx_buff.length = tx_length;
    obj->tx_buff.pos = 0;
    obj->tx_buff.width = spi_get_data_width(obj);
    obj->rx_buff.buffer = rx;
    obj->rx_buff.length = rx_length;
    obj->rx_buff.pos = 0;
    obj->rx_buff.width = spi_get_data_width(obj);
}

static int spi_is_tx_complete(spi_t *obj)
{
    return (obj->tx_buff.pos == obj->tx_buff.length);
}

static int spi_is_rx_complete(spi_t *obj)
{
    return (obj->rx_buff.pos == obj->rx_buff.length);
}

/** Return FIFO depth of the SPI peripheral
 *
 * @details
 *      SPI0/1/2/3/4      8 if data width <=16; 4 otherwise
 */
static uint32_t spi_fifo_depth(spi_t *obj)
{
    return (spi_get_data_width(obj) <= 16) ? 8 : 4;
}


/**
 * Send words from the SPI TX buffer until the send limit is reached or the TX FIFO is full
 * tx_limit is provided to ensure that the number of SPI frames (words) in flight can be managed.
 * @param[in] obj       The SPI object on which to operate
 * @param[in] tx_limit  The maximum number of words to send
 * @return The number of SPI words that have been transfered
 */
static uint32_t spi_master_write_asynch(spi_t *obj, uint32_t tx_limit)
{
    uint32_t n_words = 0;
    uint32_t tx_rmn = obj->tx_buff.length - obj->tx_buff.pos;
    uint32_t rx_rmn = obj->rx_buff.length - obj->rx_buff.pos;
    uint32_t max_tx = NU_MAX(tx_rmn, rx_rmn);
    max_tx = NU_MIN(max_tx, tx_limit);
    uint8_t data_width = spi_get_data_width(obj);
    uint8_t bytes_per_word = (data_width + 7) / 8;
//    uint8_t *tx = (uint8_t *)(obj->tx_buff.buffer) + bytes_per_word * obj->tx_buff.pos;
    uint8_t *tx = (uint8_t *)(obj->tx_buff.buffer) + obj->tx_buff.pos;
    SPI_T *spi_base = (SPI_T *)obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *)obj->u_spi.lpspi;

    while (n_words < max_tx) {
//    if ((n_words < max_tx)) {
        if((obj->bLPSPI) && (!lpspi_writeable(lpspi_base)))
            break;
        else if(!spi_writeable(spi_base))
            break;

        if (spi_is_tx_complete(obj)) {
            // Transmit dummy as transmit buffer is empty
            if(obj->bLPSPI)
                LPSPI_WRITE_TX(lpspi_base, 0);
            else
                SPI_WRITE_TX(spi_base, 0);

        } else {
            switch (bytes_per_word) {
            case 4:
                if(obj->bLPSPI)
                    LPSPI_WRITE_TX(lpspi_base, nu_get32_le(tx));
                else
                    SPI_WRITE_TX(spi_base, nu_get32_le(tx));
                //printf("DDDDD 0 %x\n", tx);
                tx += 4;
                break;
            case 2:
                if(obj->bLPSPI)
                    LPSPI_WRITE_TX(lpspi_base, nu_get16_le(tx));
                else
                    SPI_WRITE_TX(spi_base, nu_get16_le(tx));
                tx += 2;
                break;
            case 1:
                if(obj->bLPSPI)
                    LPSPI_WRITE_TX(lpspi_base, *((uint8_t *) tx));
                else
                    SPI_WRITE_TX(spi_base, *((uint8_t *) tx));
                tx += 1;
                break;
            }

            obj->tx_buff.pos += bytes_per_word;
        }
        n_words ++;
    }

    //Return the number of words that have been sent
    return n_words;
}


/**
 * Read SPI words out of the RX FIFO
 * Continues reading words out of the RX FIFO until the following condition is met:
 * o There are no more words in the FIFO
 * OR BOTH OF:
 * o At least as many words as the TX buffer have been received
 * o At least as many words as the RX buffer have been received
 * This way, RX overflows are not generated when the TX buffer size exceeds the RX buffer size
 * @param[in] obj The SPI object on which to operate
 * @return Returns the number of words extracted from the RX FIFO
 */
static uint32_t spi_master_read_asynch(spi_t *obj)
{
    uint32_t n_words = 0;
    uint32_t tx_rmn = obj->tx_buff.length - obj->tx_buff.pos;
    uint32_t rx_rmn = obj->rx_buff.length - obj->rx_buff.pos;
    uint32_t max_rx = NU_MAX(tx_rmn, rx_rmn);
    uint8_t data_width = spi_get_data_width(obj);
    uint8_t bytes_per_word = (data_width + 7) / 8;
//    uint8_t *rx = (uint8_t *)(obj->rx_buff.buffer) + bytes_per_word * obj->rx_buff.pos;
    uint8_t *rx = (uint8_t *)(obj->rx_buff.buffer) + obj->rx_buff.pos;
    SPI_T *spi_base = (SPI_T *)obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *)obj->u_spi.lpspi;

//	printf("DDDDDDD spi_master_read_asynch 1 %d \n", max_rx);

    while (n_words < max_rx) {
//    if ((n_words < max_rx)) {
        if((obj->bLPSPI) && (!lpspi_readable(lpspi_base)))
            break;
        else if(!spi_readable(spi_base))
            break;

        if (spi_is_rx_complete(obj)) {
            // Disregard as receive buffer is full
            if(obj->bLPSPI)
                LPSPI_READ_RX(lpspi_base);
            else
                SPI_READ_RX(spi_base);
        } else {
            switch (bytes_per_word) {
            case 4: {
                uint32_t val;

                if(obj->bLPSPI)
                    val = LPSPI_READ_RX(lpspi_base);
                else
                    val = SPI_READ_RX(spi_base);

                nu_set32_le(rx, val);
                rx += 4;
                break;
            }
            case 2: {
                uint16_t val;

                if(obj->bLPSPI)
                    val = LPSPI_READ_RX(lpspi_base);
                else
                    val = SPI_READ_RX(spi_base);

                nu_set16_le(rx, val);
                rx += 2;
                break;
            }
            case 1:
                if(obj->bLPSPI)
                    *rx ++ = LPSPI_READ_RX(lpspi_base);
                else
                    *rx ++ = SPI_READ_RX(spi_base);
                break;
            }

            obj->rx_buff.pos += bytes_per_word;
        }
        n_words ++;
    }

    // Return the number of words received
    return n_words;
}


static void spi_enable_vector_interrupt(spi_t *obj, uint32_t handler, uint8_t enable)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->u_spi.spi, spi_modinit_tab);

    if(modinit == NULL)
        return;

    struct nu_spi_var *var = (struct nu_spi_var *) modinit->var;

    if (enable) {
        var->obj = obj;
        obj->hdlr_async = handler;
        /* Fixed vector table is fixed in ROM and cannot be modified. */
        NVIC_EnableIRQ(modinit->irq_n);
    } else {
        NVIC_DisableIRQ(modinit->irq_n);
        /* Fixed vector table is fixed in ROM and cannot be modified. */
        var->obj = NULL;
        obj->hdlr_async = 0;
    }
}

static void spi_master_enable_interrupt(spi_t *obj, uint8_t enable)
{
    SPI_T *spi_base = (SPI_T *) obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *) obj->u_spi.lpspi;

    if(obj->bLPSPI) {
        if (enable) {
            uint32_t fifo_depth = spi_fifo_depth(obj);
            LPSPI_SetFIFO(lpspi_base, fifo_depth / 2, fifo_depth / 2);
            // Enable tx/rx FIFO threshold interrupt
            LPSPI_EnableInt(lpspi_base, LPSPI_FIFO_RXTH_INT_MASK | LPSPI_FIFO_TXTH_INT_MASK);
        } else {
            LPSPI_DisableInt(lpspi_base, LPSPI_FIFO_RXTH_INT_MASK | LPSPI_FIFO_TXTH_INT_MASK);
        }

        return;
    }

    if (enable) {
        uint32_t fifo_depth = spi_fifo_depth(obj);
        SPI_SetFIFO(spi_base, (fifo_depth / 2), (fifo_depth / 2));
        // Enable tx/rx FIFO threshold interrupt
        SPI_EnableInt(spi_base, SPI_FIFO_RXTH_INT_MASK | SPI_FIFO_TXTH_INT_MASK);
    } else {
        SPI_DisableInt(spi_base, SPI_FIFO_RXTH_INT_MASK | SPI_FIFO_TXTH_INT_MASK);
    }
}

static void spi_dma_handler_tx(void* id, uint32_t event_dma)
{
    spi_t *obj = (spi_t *) id;

    // TODO: Pass this error to caller
    if (event_dma & NU_PDMA_EVENT_ABORT) {
        printf("spi_dma_handler_tx 0 \n");
    }
    // Expect SPI IRQ will catch this transfer done event
    if (event_dma & NU_PDMA_EVENT_TRANSFER_DONE) {
        obj->tx_buff.pos = obj->tx_buff.length;
//		printf("spi_dma_handler_tx 1 \n");
    }
    // TODO: Pass this error to caller
    if (event_dma & NU_PDMA_EVENT_TIMEOUT) {
        printf("spi_dma_handler_tx 2 \n");
    }

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->u_spi.spi, spi_modinit_tab);
    if(modinit == NULL)
        return;

    if (obj && obj->hdlr_async) {
        void (*hdlr_async)(spi_t *) = (void(*)(spi_t *))(obj->hdlr_async);
        hdlr_async(obj);
    }
}

static void spi_dma_handler_rx(void* id, uint32_t event_dma)
{
    spi_t *obj = (spi_t *) id;

    // TODO: Pass this error to caller
    if (event_dma & NU_PDMA_EVENT_ABORT) {
        printf("spi_dma_handler_rx 0 \n");
    }
    // Expect SPI IRQ will catch this transfer done event
    if (event_dma & NU_PDMA_EVENT_TRANSFER_DONE) {
        obj->rx_buff.pos = obj->rx_buff.length;
    }
    // TODO: Pass this error to caller
    if (event_dma & NU_PDMA_EVENT_TIMEOUT) {
        printf("spi_dma_handler_rx 2 \n");
    }

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->u_spi.spi, spi_modinit_tab);
    if(modinit == NULL)
        return;

    if (obj && obj->hdlr_async) {
        void (*hdlr_async)(spi_t *) = (void(*)(spi_t *))(obj->hdlr_async);
        hdlr_async(obj);
    }

}

/**
 * Abort an SPI transfer
 * This is a helper function for event handling. When any of the events listed occurs, the HAL will abort any ongoing
 * transfers
 * @param[in] obj The SPI peripheral to stop
 */
static void spi_abort_asynch(spi_t *obj)
{
    SPI_T *spi_base = (SPI_T *) obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *) obj->u_spi.lpspi;

    printf("DDDDDDD spi_abort_asynch \n");
    if (obj->dma_usage != ePDMA_USAGE_NEVER) {
        // Receive FIFO Overrun in case of tx length > rx length on DMA way
        if(obj->bLPSPI) {
            if (lpspi_base->STATUS & LPSPI_STATUS_RXOVIF_Msk) {
                lpspi_base->STATUS = LPSPI_STATUS_RXOVIF_Msk;
            }

            if (obj->dma_chn_id_tx != NU_PDMA_OUT_OF_CHANNELS) {
                nu_lppdma_channel_terminate(obj->dma_chn_id_tx);
            }
            LPSPI_DISABLE_TX_PDMA(lpspi_base);

            if (obj->dma_chn_id_rx != NU_PDMA_OUT_OF_CHANNELS) {
                nu_lppdma_channel_terminate(obj->dma_chn_id_rx);
            }

            LPSPI_DISABLE_RX_PDMA(lpspi_base);
        } else {
            if (spi_base->STATUS & SPI_STATUS_RXOVIF_Msk) {
                spi_base->STATUS = SPI_STATUS_RXOVIF_Msk;
            }

            if (obj->dma_chn_id_tx != NU_PDMA_OUT_OF_CHANNELS) {
                nu_pdma_channel_terminate(obj->dma_chn_id_tx);
            }
            SPI_DISABLE_TX_PDMA(spi_base);

            if (obj->dma_chn_id_rx != NU_PDMA_OUT_OF_CHANNELS) {
                nu_pdma_channel_terminate(obj->dma_chn_id_rx);
            }

            SPI_DISABLE_RX_PDMA(spi_base);
        }
    }

    // Necessary for both interrupt way and DMA way
    spi_enable_vector_interrupt(obj, 0, 0);
    spi_master_enable_interrupt(obj, 0);

    /* Necessary for accessing FIFOCTL below */
    if(obj->bLPSPI) {
        LPSPI_DISABLE_SYNC(lpspi_base);

        LPSPI_ClearRxFIFO(lpspi_base);
        LPSPI_ClearTxFIFO(lpspi_base);
    } else {
        SPI_DISABLE_SYNC(spi_base);

        SPI_ClearRxFIFO(spi_base);
        SPI_ClearTxFIFO(spi_base);
    }
}

static uint32_t spi_event_check(spi_t *obj)
{
    SPI_T *spi_base = (SPI_T *) obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *) obj->u_spi.lpspi;
    uint32_t event = 0;
//	uint32_t u32Status = 0;

//	u32Status = spi_base->STATUS;

    if (obj->dma_usage == ePDMA_USAGE_NEVER) {
        uint32_t n_rec = spi_master_read_asynch(obj);
        spi_master_write_asynch(obj, n_rec);
    }

    if(spi_is_tx_complete(obj)) {
        if(obj->bLPSPI) {
            LPSPI_ClearIntFlag(lpspi_base, LPSPI_TXUF_INT_MASK);
            LPSPI_DisableInt(lpspi_base, LPSPI_FIFO_TXTH_INT_MASK);
        } else {
            SPI_ClearIntFlag(spi_base, SPI_TXUF_INT_MASK);
            SPI_DisableInt(spi_base, SPI_FIFO_TXTH_INT_MASK);
        }
    }

    if(spi_is_rx_complete(obj)) {
        if(obj->bLPSPI) {
            LPSPI_ClearIntFlag(lpspi_base, LPSPI_FIFO_RXOV_INT_MASK);
            LPSPI_DisableInt(lpspi_base, LPSPI_FIFO_RXTH_INT_MASK);
        } else {
            SPI_ClearIntFlag(spi_base, SPI_FIFO_RXOV_INT_MASK);
            SPI_DisableInt(spi_base, SPI_FIFO_RXTH_INT_MASK);
        }
    }

    if (spi_is_tx_complete(obj) && spi_is_rx_complete(obj)) {
//		printf("DDDDDDD spi_event_check  complete %x \n", u32Status);
        spi_master_enable_interrupt(obj, 0);
        event |= SPI_EVENT_COMPLETE;
    }

    if(obj->bLPSPI) {
        // Receive FIFO Overrun
        if (lpspi_base->STATUS & LPSPI_STATUS_RXOVIF_Msk) {
            lpspi_base->STATUS = LPSPI_STATUS_RXOVIF_Msk;
            printf("spi_event_check error 0 \n");
            // In case of tx length > rx length on DMA way
            if (obj->dma_usage == ePDMA_USAGE_NEVER) {
                event |= SPI_EVENT_RX_OVERFLOW;
            }
        }

        // Receive Time-Out
        if (lpspi_base->STATUS & LPSPI_STATUS_RXTOIF_Msk) {
            lpspi_base->STATUS = LPSPI_STATUS_RXTOIF_Msk;
            // Not using this IF. Just clear it.
            printf("spi_event_check error 1 \n");
        }
        // Transmit FIFO Under-Run
        if (lpspi_base->STATUS & LPSPI_STATUS_TXUFIF_Msk) {
            lpspi_base->STATUS = LPSPI_STATUS_TXUFIF_Msk;
            event |= SPI_EVENT_ERROR;
            printf("spi_event_check error 2 \n");
        }

        return event;
    }

    // Receive FIFO Overrun
    if (spi_base->STATUS & SPI_STATUS_RXOVIF_Msk) {
        spi_base->STATUS = SPI_STATUS_RXOVIF_Msk;
        printf("spi_event_check error 0 \n");
        // In case of tx length > rx length on DMA way
        if (obj->dma_usage == ePDMA_USAGE_NEVER) {
            event |= SPI_EVENT_RX_OVERFLOW;
        }
    }

    // Receive Time-Out
    if (spi_base->STATUS & SPI_STATUS_RXTOIF_Msk) {
        spi_base->STATUS = SPI_STATUS_RXTOIF_Msk;
        // Not using this IF. Just clear it.
        printf("spi_event_check error 1 \n");
    }

    // Transmit FIFO Under-Run
    if (spi_base->STATUS & SPI_STATUS_TXUFIF_Msk) {
        spi_base->STATUS = SPI_STATUS_TXUFIF_Msk;
        event |= SPI_EVENT_ERROR;
        printf("spi_event_check error 2 \n");
    }

    return event;
}


int is_spi_trans_done(spi_t *obj)
{
    if (spi_is_tx_complete(obj) && spi_is_rx_complete(obj)) {
        return 1;
    }
    return 0;
}

int32_t SPI_Init(
    spi_t *obj,
    SPI_InitTypeDef *psInitDef
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->u_spi.spi, spi_modinit_tab);

    if(modinit == NULL)
        return -1;

    // Reset this module
    SYS_ResetModule(modinit->rsetidx);

    // Select IP clock source
    CLK_SetModuleClock(modinit->clkidx, modinit->clksrc, modinit->clkdiv);
    // Enable IP clock
    CLK_EnableModuleClock(modinit->clkidx);


#if DEVICE_SPI_ASYNCH
    obj->dma_usage = ePDMA_USAGE_NEVER;
    obj->event = 0;
    obj->hdlr_async = 0;
    obj->dma_chn_id_tx = NU_PDMA_OUT_OF_CHANNELS;
    obj->dma_chn_id_rx = NU_PDMA_OUT_OF_CHANNELS;

#endif

    int mode =0;
    if(psInitDef->ClockPolarity == 0) {
        if(psInitDef->ClockPhase == SPI_PHASE_1EDGE)
            mode = 0;
        else
            mode = 1;
    } else {
        if(psInitDef->ClockPhase == SPI_PHASE_1EDGE)
            mode = 3;
        else
            mode = 2;
    }

    if(obj->bLPSPI) {
        lpspi_format(obj->u_spi.lpspi, psInitDef->Bits, mode, psInitDef->Mode, psInitDef->FirstBit, psInitDef->Direction);

        if(psInitDef->Mode == SPI_MASTER)
            lpspi_frequency(obj->u_spi.lpspi, psInitDef->BaudRate);
    } else {
        spi_format(obj->u_spi.spi, psInitDef->Bits, mode, psInitDef->Mode, psInitDef->FirstBit, psInitDef->Direction);

        if(psInitDef->Mode == SPI_MASTER)
            spi_frequency(obj->u_spi.spi, psInitDef->BaudRate);
    }

    return 0;
}

void SPI_Final(spi_t *obj)
{
#if DEVICE_SPI_ASYNCH
    if (obj->dma_chn_id_tx != NU_PDMA_OUT_OF_CHANNELS) {
        nu_pdma_channel_free(obj->dma_chn_id_tx);
        obj->dma_chn_id_tx = NU_PDMA_OUT_OF_CHANNELS;
    }
    if (obj->dma_chn_id_rx != NU_PDMA_OUT_OF_CHANNELS) {
        nu_pdma_channel_free(obj->dma_chn_id_rx);
        obj->dma_chn_id_rx = NU_PDMA_OUT_OF_CHANNELS;
    }
#endif

    if(obj->bLPSPI)
        LPSPI_Close(obj->u_spi.lpspi);
    else
        SPI_Close(obj->u_spi.spi);

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->u_spi.spi, spi_modinit_tab);
    if(modinit == NULL)
        return;
    if(modinit->modname != (int) obj->u_spi.spi)
        return;


    if(obj->bLPSPI)
        LPSPI_DisableInt((obj->u_spi.lpspi), (LPSPI_FIFO_RXOV_INT_MASK | LPSPI_FIFO_RXTH_INT_MASK | LPSPI_FIFO_TXTH_INT_MASK));
    else
        SPI_DisableInt((obj->u_spi.spi), (SPI_FIFO_RXOV_INT_MASK | SPI_FIFO_RXTH_INT_MASK | SPI_FIFO_TXTH_INT_MASK));

    NVIC_DisableIRQ(modinit->irq_n);

    // Disable IP clock
    CLK_DisableModuleClock(modinit->clkidx);
}


int SPI_SetDataWidth(
    spi_t *obj,
    int data_width_bits
)
{

    if((data_width_bits != 8) && (data_width_bits != 16) && (data_width_bits != 32))
        return -1;

    if(obj->bLPSPI)
        LPSPI_SET_DATA_WIDTH(obj->u_spi.lpspi, data_width_bits);
    else
        SPI_SET_DATA_WIDTH(obj->u_spi.spi, data_width_bits);

    return 0;
}


int SPI_MasterReadOnly(spi_t *obj)
{
    SPI_T *spi_base = (SPI_T *) obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *) obj->u_spi.lpspi;
    int value;

    if(obj->bLPSPI) {
        // NOTE: Data in receive FIFO can be read out via ICE.
        LPSPI_ENABLE_SYNC(lpspi_base);

        // Wait for rx buffer full
        while (! lpspi_readable(lpspi_base)) {
            MICROPY_THREAD_YIELD();
        }

        value = LPSPI_READ_RX(lpspi_base);

        return value;
    }


    // NOTE: Data in receive FIFO can be read out via ICE.
    SPI_ENABLE_SYNC(spi_base);

    // Wait for rx buffer full
    while (! spi_readable(spi_base)) {
        MICROPY_THREAD_YIELD();
    }

    value = SPI_READ_RX(spi_base);

    /* We don't call SPI_DISABLE_SYNC here for performance. */

    return value;
}

int SPI_MasterWrite(spi_t *obj, int value)
{
    SPI_T *spi_base = (SPI_T *) obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *) obj->u_spi.lpspi;
    int value2;

    if(obj->bLPSPI) {
        // NOTE: Data in receive FIFO can be read out via ICE.
        LPSPI_ENABLE_SYNC(lpspi_base);

        // Wait for tx buffer empty
        while(! lpspi_writeable(lpspi_base)) {
            MICROPY_THREAD_YIELD();
        }
        LPSPI_WRITE_TX(lpspi_base, value);

        // Wait for rx buffer full
        while (! lpspi_readable(lpspi_base)) {
            MICROPY_THREAD_YIELD();
        }

        value2 = LPSPI_READ_RX(lpspi_base);

        return value2;
    }

    // NOTE: Data in receive FIFO can be read out via ICE.
    SPI_ENABLE_SYNC(spi_base);

    // Wait for tx buffer empty
    while(! spi_writeable(spi_base)) {
        MICROPY_THREAD_YIELD();
    }
    SPI_WRITE_TX(spi_base, value);

    // Wait for rx buffer full
    while (! spi_readable(spi_base)) {
        MICROPY_THREAD_YIELD();
    }
    value2 = SPI_READ_RX(spi_base);

    /* We don't call SPI_DISABLE_SYNC here for performance. */

    return value2;
}

int SPI_MasterBlockWriteRead(spi_t *obj, const char *tx_buffer, int tx_length,
                             char *rx_buffer, int rx_length, char write_fill)
{
    int total = (tx_length > rx_length) ? tx_length : rx_length;

    //for testing
//	int j;
//	char *buf = (char *)tx_buffer;
//	for(j = 0; j < tx_length; j++){
//		buf[j] = j;
//	}

    uint8_t data_width = spi_get_data_width(obj);
    uint8_t bytes_per_word = (data_width + 7) / 8;
    int32_t in = 0;
    uint8_t *tx = (uint8_t *)tx_buffer;
    uint8_t *rx = (uint8_t *)rx_buffer;
    int i = 0;
    int32_t fill = 0;

    if(bytes_per_word == 4)
        fill = (write_fill << 24) | (write_fill << 16) | (write_fill << 8) | write_fill;
    else if(bytes_per_word == 2)
        fill = (write_fill << 8) | write_fill;
    else
        fill = write_fill;

    while(i < total) {
        if (i > tx_length) {
            in = SPI_MasterWrite(obj, fill);
        } else {
            switch (bytes_per_word) {
            case 4:
                in = SPI_MasterWrite(obj, nu_get32_le(tx));
                tx += 4;
                break;
            case 2:
                in = SPI_MasterWrite(obj, nu_get16_le(tx));
                tx += 2;
                break;
            case 1:
                in = SPI_MasterWrite(obj, *((uint8_t *) tx));
                tx += 1;
                break;
            }
        }


        if((rx) && (i <= rx_length)) {
            switch (bytes_per_word) {
            case 4:
                nu_set32_le(rx, in);
                rx += 4;
                break;
            case 2:
                nu_set16_le(rx, in);
                rx += 2;
                break;
            case 1:
                *rx ++ = (uint8_t) in;
                break;
            }
        }

        i += bytes_per_word;
    }


    //for testing
//	buf = (char *)rx_buffer;
//	for(j = 0; j < rx_length; j++){
//		printf("[%x]", buf[j]);
//	}
//	printf("\n");

    return total;
}

int SPI_MasterWriteThenRead(spi_t *obj, const char *tx_buffer, int tx_length,
                            char *rx_buffer, int rx_length, char write_fill)
{

    uint8_t data_width = spi_get_data_width(obj);
    uint8_t bytes_per_word = (data_width + 7) / 8;
    int32_t in = 0;
    uint8_t *tx = (uint8_t *)tx_buffer;
    uint8_t *rx = (uint8_t *)rx_buffer;
    int i = 0;
    int32_t fill = 0;

    if(bytes_per_word == 4)
        fill = (write_fill << 24) | (write_fill << 16) | (write_fill << 8) | write_fill;
    else if(bytes_per_word == 2)
        fill = (write_fill << 8) | write_fill;
    else
        fill = write_fill;

    while(i < tx_length) {
        switch (bytes_per_word) {
        case 4:
            in = SPI_MasterWrite(obj, nu_get32_le(tx));
            tx += 4;
            break;
        case 2:
            in = SPI_MasterWrite(obj, nu_get16_le(tx));
            tx += 2;
            break;
        case 1:
            in = SPI_MasterWrite(obj, *((uint8_t *) tx));
            tx += 1;
            break;
        }

        i += bytes_per_word;
    }

    i = 0;

    while(i < rx_length) {

        in = SPI_MasterWrite(obj, fill);
        printf("in is %lx \n", in);

        switch (bytes_per_word) {
        case 4:
            nu_set32_le(rx, in);
            rx += 4;
            break;
        case 2:
            nu_set16_le(rx, in);
            rx += 2;
            break;
        case 1:
            *rx ++ = (uint8_t) in;
            break;
        }

        i += bytes_per_word;
    }

    return rx_length;
}

#if DEVICE_SPISLAVE

int SPI_SlaveReceive(spi_t *obj)
{
    SPI_T *spi_base = (SPI_T *)obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *)obj->u_spi.lpspi;

    if(obj->bLPSPI) {
        LPSPI_ENABLE_SYNC(lpspi_base);
        return lpspi_readable(lpspi_base);
    }

    SPI_ENABLE_SYNC(spi_base);
    return spi_readable(spi_base);
};

int SPI_SlaveReadOnly(spi_t *obj)
{
    SPI_T *spi_base = (SPI_T *)obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *)obj->u_spi.lpspi;
    int value;

    if(obj->bLPSPI) {
        LPSPI_ENABLE_SYNC(lpspi_base);

        // Wait for rx buffer full
        while (! lpspi_readable(lpspi_base)) {
            MICROPY_THREAD_YIELD();
        }
        value = LPSPI_READ_RX(lpspi_base);
    }

    SPI_ENABLE_SYNC(spi_base);

    // Wait for rx buffer full
    while (! spi_readable(spi_base)) {
        MICROPY_THREAD_YIELD();
    }

    value = SPI_READ_RX(spi_base);
    return value;
}


int SPI_SlaveWrite(spi_t *obj, int value)
{
    SPI_T *spi_base = (SPI_T *)obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *)obj->u_spi.lpspi;
    int value2;

    if(obj->bLPSPI) {
        LPSPI_ENABLE_SYNC(lpspi_base);

        // Wait for tx buffer empty
        while(! lpspi_writeable(lpspi_base)) {
            MICROPY_THREAD_YIELD();
        }

        LPSPI_WRITE_TX(lpspi_base, value);

        // Wait for rx buffer full
        while (! lpspi_readable(lpspi_base)) {
            MICROPY_THREAD_YIELD();
        }

        value2 = LPSPI_READ_RX(lpspi_base);
        return value2;
    }

    SPI_ENABLE_SYNC(spi_base);

    // Wait for tx buffer empty
    while(! spi_writeable(spi_base)) {
        MICROPY_THREAD_YIELD();
    }

    SPI_WRITE_TX(spi_base, value);

    // Wait for rx buffer full
    while (! spi_readable(spi_base)) {
        MICROPY_THREAD_YIELD();
    }
    value2 = SPI_READ_RX(spi_base);

    /* We don't call SPI_DISABLE_SYNC here for performance. */

    return value2;
}


int SPI_SlaveBlockWriteRead(spi_t *obj, const char *tx_buffer, int tx_length,
                            char *rx_buffer, int rx_length, char write_fill)
{
    int total = (tx_length > rx_length) ? tx_length : rx_length;

    uint8_t data_width = spi_get_data_width(obj);
    uint8_t bytes_per_word = (data_width + 7) / 8;
    int32_t in = 0;
    uint8_t *tx = (uint8_t *)tx_buffer;
    uint8_t *rx = (uint8_t *)rx_buffer;
    int i = 0;
    int32_t fill = 0;

    if(bytes_per_word == 4)
        fill = (write_fill << 24) | (write_fill << 16) | (write_fill << 8) | write_fill;
    else if(bytes_per_word == 2)
        fill = (write_fill << 8) | write_fill;
    else
        fill = write_fill;

    while(i < total) {
        if (i > tx_length) {
            in = SPI_MasterWrite(obj, fill);
        } else {
            switch (bytes_per_word) {
            case 4:
                in = SPI_MasterWrite(obj, nu_get32_le(tx));
                tx += 4;
                break;
            case 2:
                in = SPI_MasterWrite(obj, nu_get16_le(tx));
                tx += 2;
                break;
            case 1:
                in = SPI_MasterWrite(obj, *((uint8_t *) tx));
                tx += 1;
                break;
            }
        }


        if(i <= rx_length) {
            switch (bytes_per_word) {
            case 4:
                nu_set32_le(rx, in);
                rx += 4;
                break;
            case 2:
                nu_set16_le(rx, in);
                rx += 2;
                break;
            case 1:
                *rx ++ = (uint8_t) in;
                break;
            }
        }

        i += bytes_per_word;
    }

    return total;
}


#endif

#if DEVICE_SPI_ASYNCH
void spi_master_transfer(
    spi_t *obj,
    const void *tx,
    size_t tx_length,
    void *rx,
    size_t rx_length,
    uint8_t bit_width,
    uint32_t handler,
    uint32_t event,
    E_PDMAUsage hint
)
{
    SPI_T *spi_base = (SPI_T *) obj->u_spi.spi;
    LPSPI_T *lpspi_base = (LPSPI_T *) obj->u_spi.lpspi;

    if(obj->bLPSPI)
        LPSPI_SET_DATA_WIDTH(lpspi_base, bit_width);
    else
        SPI_SET_DATA_WIDTH(spi_base, bit_width);

    obj->dma_usage = hint;

    spi_check_dma_usage(obj, (E_PDMAUsage *)&obj->dma_usage, &obj->dma_chn_id_tx, &obj->dma_chn_id_rx);
    uint32_t data_width = spi_get_data_width(obj);

    // Conditions to go DMA way:
    // (1) No DMA support for non-8 multiple data width.
    // (2) tx length >= rx length. Otherwise, as tx DMA is done, no bus activity for remaining rx.
    if ((data_width % 8) ||
        (tx_length < rx_length)) {

        if(obj->bLPSPI) {
            nu_lppdma_channel_free(obj->dma_chn_id_tx);
            nu_lppdma_channel_free(obj->dma_chn_id_rx);
        } else {
            nu_pdma_channel_free(obj->dma_chn_id_tx);
            nu_pdma_channel_free(obj->dma_chn_id_rx);
        }
        obj->dma_usage = ePDMA_USAGE_NEVER;
        obj->dma_chn_id_tx = NU_PDMA_OUT_OF_CHANNELS;
        obj->dma_chn_id_rx = NU_PDMA_OUT_OF_CHANNELS;
    }

    // SPI IRQ is necessary for both interrupt way and DMA way
    spi_enable_event(obj, event, 1);
    spi_buffer_set(obj, tx, tx_length, rx, rx_length);


    if(obj->bLPSPI)
        LPSPI_ENABLE_SYNC(lpspi_base);
    else
        SPI_ENABLE_SYNC(spi_base);

    if (obj->dma_usage == ePDMA_USAGE_NEVER) {
        // Interrupt way
        if(obj->bLPSPI) {
            LPSPI_ClearRxFIFO(lpspi_base);
            LPSPI_ClearTxFIFO(lpspi_base);
        } else {
            SPI_ClearRxFIFO(spi_base);
            SPI_ClearTxFIFO(spi_base);
        }

        spi_master_write_asynch(obj, spi_fifo_depth(obj));
        spi_master_enable_interrupt(obj, 1);
        spi_enable_vector_interrupt(obj, handler, 1);
    } else {
        // DMA way
        int ret = 0;
        const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->u_spi.spi, spi_modinit_tab);
        if(modinit == NULL)
            return;

        if(obj->bLPSPI)
            lpspi_base->PDMACTL &= ~(LPSPI_PDMACTL_TXPDMAEN_Msk | LPSPI_PDMACTL_RXPDMAEN_Msk);
        else
            spi_base->PDMACTL &= ~(SPI_PDMACTL_TXPDMAEN_Msk | SPI_PDMACTL_RXPDMAEN_Msk);

        if(tx_length) {
            if(data_width == 16) {
                tx_length = tx_length / 2;
            } else if(data_width == 32) {
                tx_length = tx_length / 4;
            }

            struct nu_pdma_chn_cb pdma_tx_chn_cb;

            pdma_tx_chn_cb.m_eCBType = eCBType_Event;
            pdma_tx_chn_cb.m_pfnCBHandler = spi_dma_handler_tx;
            pdma_tx_chn_cb.m_pvUserData = obj;

            if(obj->bLPSPI) {
                nu_lppdma_filtering_set(obj->dma_chn_id_tx, NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_TIMEOUT);
                nu_lppdma_callback_register(obj->dma_chn_id_tx, &pdma_tx_chn_cb);
                ret = nu_lppdma_transfer( obj->dma_chn_id_tx,
                                          data_width,
                                          (uint32_t)tx,
                                          (uint32_t)&lpspi_base->TX,
                                          tx_length,
                                          0
                                        );

            } else {
                nu_pdma_filtering_set(obj->dma_chn_id_tx, NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_TIMEOUT);
                nu_pdma_callback_register(obj->dma_chn_id_tx, &pdma_tx_chn_cb);
                ret = nu_pdma_transfer( obj->dma_chn_id_tx,
                                        data_width,
                                        (uint32_t)tx,
                                        (uint32_t)&spi_base->TX,
                                        tx_length,
                                        0
                                      );
            }

        }

        if(ret != 0 ) {
            printf("Error: Unable setup SPI TX PDMA transfer %d \n", ret);
            return;
        }

        if(rx_length) {
            if(data_width == 16) {
                rx_length = rx_length / 2;
            } else if(data_width == 32) {
                rx_length = rx_length / 4;
            }

            struct nu_pdma_chn_cb pdma_rx_chn_cb;

            pdma_rx_chn_cb.m_eCBType = eCBType_Event;
            pdma_rx_chn_cb.m_pfnCBHandler = spi_dma_handler_rx;
            pdma_rx_chn_cb.m_pvUserData = obj;

            if(obj->bLPSPI) {
                nu_lppdma_filtering_set(obj->dma_chn_id_rx, NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_TIMEOUT);
                nu_lppdma_callback_register(obj->dma_chn_id_rx, &pdma_rx_chn_cb);
                ret = nu_lppdma_transfer( obj->dma_chn_id_rx,
                                          data_width,
                                          (uint32_t)&lpspi_base->RX,
                                          (uint32_t)rx,
                                          rx_length,
                                          0
                                        );

            } else {
                nu_pdma_filtering_set(obj->dma_chn_id_rx, NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_TIMEOUT);
                nu_pdma_callback_register(obj->dma_chn_id_rx, &pdma_rx_chn_cb);
                ret = nu_pdma_transfer( obj->dma_chn_id_rx,
                                        data_width,
                                        (uint32_t)&spi_base->RX,
                                        (uint32_t)rx,
                                        rx_length,
                                        0
                                      );
            }

        }

        if(ret != 0 ) {
            printf("Error: Unable setup SPI RX PDMA transfer %d \n", ret);
            return;
        }

        /* Start tx/rx DMA transfer
         *
         * If we have both PDMA and SPI interrupts enabled and PDMA priority is lower than SPI priority,
         * we would trap in SPI interrupt handler endlessly with the sequence:
         *
         * 1. PDMA TX transfer done interrupt occurs and is well handled.
         * 2. SPI RX FIFO threshold interrupt occurs. Trap here because PDMA RX transfer done interrupt doesn't get handled.
         * 3. PDMA RX transfer done interrupt occurs but it cannot be handled due to above.
         *
         * To fix it, we don't enable SPI TX/RX threshold interrupts but keep SPI vector handler set to be called
         * in PDMA TX/RX transfer done interrupt handlers (spi_dma_handler_tx/spi_dma_handler_rx).
         */
        spi_enable_vector_interrupt(obj, handler, 1);
        /* No TX/RX FIFO threshold interrupt */
        spi_master_enable_interrupt(obj, 0);

        /* Order to enable PDMA TX/RX functions
         *
         * H/W spec: In SPI Master mode with full duplex transfer, if both TX and RX PDMA functions are
         *           enabled, RX PDMA function cannot be enabled prior to TX PDMA function. User can enable
         *           TX PDMA function firstly or enable both functions simultaneously.
         * Per real test, it is safer to start RX PDMA first and then TX PDMA. Otherwise, receive FIFO is
         * subject to overflow by TX DMA.
         *
         * With the above conflicts, we enable PDMA TX/RX functions simultaneously.
         */

        if(tx_length && rx_length) {
            if(obj->bLPSPI)
                LPSPI_TRIGGER_TX_RX_PDMA(lpspi_base);
            else
                SPI_TRIGGER_TX_RX_PDMA(spi_base);
        } else {
            if(obj->bLPSPI) {
                if(tx_length)
                    LPSPI_TRIGGER_TX_PDMA(lpspi_base);

                if(rx_length)
                    LPSPI_TRIGGER_RX_PDMA(lpspi_base);
            } else {

                if(tx_length)
                    SPI_TRIGGER_TX_PDMA(spi_base);

                if(rx_length)
                    SPI_TRIGGER_RX_PDMA(spi_base);
            }
        }
    }

}

#endif

/**
 * Handle the SPI interrupt
 * Read frames until the RX FIFO is empty.  Write at most as many frames as were read.  This way,
 * it is unlikely that the RX FIFO will overflow.
 * @param[in] obj The SPI peripheral that generated the interrupt
 * @return
 */
void spi_irq_handler_asynch(spi_t *obj)
{
    // Check for SPI events
    uint32_t event = spi_event_check(obj);

    if (event & ~SPI_EVENT_COMPLETE) {
        spi_abort_asynch(obj);
    }

    obj->event = event | ((event & SPI_EVENT_COMPLETE) ? SPI_EVENT_INTERNAL_TRANSFER_COMPLETE : 0);
}

/**
 * Handle the SPI interrupt
 * Read frames until the RX FIFO is empty.  Write at most as many frames as were read.  This way,
 * it is unlikely that the RX FIFO will overflow.
 * @param[in] obj The SPI peripheral that generated the interrupt
 * @return
 */

void Handle_SPI_Irq(SPI_T *spi)
{
    spi_t *obj = NULL;
    if(spi == SPI0)
        obj = spi0_var.obj;
    else if(spi == SPI1)
        obj = spi1_var.obj;
    else if(spi == SPI2)
        obj = spi2_var.obj;
    else if(spi == SPI3)
        obj = spi3_var.obj;

    if (obj && obj->hdlr_async) {
        void (*hdlr_async)(spi_t *) = (void(*)(spi_t *))(obj->hdlr_async);
        hdlr_async(obj);
    }
}

/**
 * Handle the LPSPI interrupt
 * Read frames until the RX FIFO is empty.  Write at most as many frames as were read.  This way,
 * it is unlikely that the RX FIFO will overflow.
 * @param[in] obj The SPI peripheral that generated the interrupt
 * @return
 */

void Handle_LPSPI_Irq(LPSPI_T *lpspi)
{
    spi_t *obj = NULL;

    if(lpspi == LPSPI0)
        obj = spi4_var.obj;

    if (obj && obj->hdlr_async) {
        void (*hdlr_async)(spi_t *) = (void(*)(spi_t *))(obj->hdlr_async);
        hdlr_async(obj);
    }
}

