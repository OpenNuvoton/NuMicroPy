/**************************************************************************//**
 * @file     M48x_SPI.c
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
#include "M48x_SPI.h"

#define DEVICE_SPI_ASYNCH (1)
#define DEVICE_SPISLAVE (1)

extern void SPI0_IRQHandler(void);
extern void SPI1_IRQHandler(void);
extern void SPI2_IRQHandler(void);
extern void SPI3_IRQHandler(void);

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

static int spi_writeable(spi_t * obj);
static int spi_readable(spi_t * obj);

#if DEVICE_SPI_ASYNCH

static void spi_check_dma_usage(DMAUsage *dma_usage, int *dma_ch_tx, int *dma_ch_rx);
static uint8_t spi_get_data_width(spi_t *obj);
static void spi_enable_event(spi_t *obj, uint32_t event, uint8_t enable);
static void spi_buffer_set(spi_t *obj, const void *tx, size_t tx_length, void *rx, size_t rx_length);
static uint32_t spi_master_write_asynch(spi_t *obj, uint32_t tx_limit);
static uint32_t spi_master_read_asynch(spi_t *obj);
static void spi_master_enable_interrupt(spi_t *obj, uint8_t enable);
static void spi_enable_vector_interrupt(spi_t *obj, uint32_t handler, uint8_t enable);
static uint32_t spi_fifo_depth(spi_t *obj);
static void spi_dma_handler_tx(uint32_t id, uint32_t event_dma);
static void spi_dma_handler_rx(uint32_t id, uint32_t event_dma);
static uint32_t spi_event_check(spi_t *obj);

#endif


static const struct nu_modinit_s spi_modinit_tab[] = {
    {(uint32_t)SPI0, SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk, SPI0_RST, SPI0_IRQn, &spi0_var},
    {(uint32_t)SPI1, SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_PCLK0, MODULE_NoMsk, SPI1_RST, SPI1_IRQn, &spi1_var},
    {(uint32_t)SPI2, SPI2_MODULE, CLK_CLKSEL2_SPI2SEL_PCLK1, MODULE_NoMsk, SPI2_RST, SPI2_IRQn, &spi2_var},
    {(uint32_t)SPI3, SPI3_MODULE, CLK_CLKSEL2_SPI3SEL_PCLK0, MODULE_NoMsk, SPI3_RST, SPI3_IRQn, &spi3_var},

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
        while (SPI_IS_BUSY(spi_base));
    
        SPI_DISABLE(spi_base);
    }
    while (spi_base->STATUS & SPI_STATUS_SPIENSTS_Msk);
}

static void spi_format(spi_t *obj, int bits, int mode, int slave, int msb_first, int direction)
{

    SPI_T *spi_base = (SPI_T *) obj->spi;

    SPI_DISABLE_SYNC(spi_base);
    
	printf("bits: %d \n", bits);
	printf("mode: %d \n", mode);
	printf("slave: %d \n", slave);
	printf("msb_first: %d \n", msb_first);
	printf("direction: %d \n", direction);

    SPI_Open(spi_base,
             slave ? SPI_SLAVE : SPI_MASTER,
             (mode == 0) ? SPI_MODE_0 : (mode == 1) ? SPI_MODE_1 : (mode == 2) ? SPI_MODE_2 : SPI_MODE_3,
             bits,
             SPI_GetBusClock(spi_base));


	if(msb_first == SPI_FIRSTBIT_MSB)
		SPI_SET_MSB_FIRST(spi_base);
	else
		SPI_SET_LSB_FIRST(spi_base);
	

    if (! slave) {
        // Master
		// Configure SS as low active.
		SPI_EnableAutoSS(spi_base, SPI_SS, SPI_SS_ACTIVE_LOW);
		if(direction == SPI_DIRECTION_2LINES_RXONLY){
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

	if(direction == SPI_DIRECTION_1LINE){
		spi_base->CTL |= SPI_CTL_HALFDPX_Msk;
	}
}

static void spi_frequency(spi_t *obj, int hz)
{
    SPI_T *spi_base = (SPI_T *) obj->spi;

    SPI_DISABLE_SYNC(spi_base);

    SPI_SetBusClock((SPI_T *) obj->spi, hz);
}


/*
 * SPI mode0 = ClockPolarity(0) + ClockPhase(SPI_PHASE_1EDGE)
 * SPI mode1 = ClockPolarity(0) + ClockPhase(SPI_PHASE_2EDGE)
 * SPI mode2 = ClockPolarity(1) + ClockPhase(SPI_PHASE_2EDGE)
 * SPI mode3 = ClockPolarity(1) + ClockPhase(SPI_PHASE_1EDGE)
 */


int32_t SPI_Init(
	spi_t *psObj,
 	SPI_InitTypeDef *psInitDef
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->spi, spi_modinit_tab);

	if(modinit == NULL)
		return -1;
		
   // Reset this module
    SYS_ResetModule(modinit->rsetidx);

    // Select IP clock source
    CLK_SetModuleClock(modinit->clkidx, modinit->clksrc, modinit->clkdiv);
    // Enable IP clock
    CLK_EnableModuleClock(modinit->clkidx);


#if DEVICE_SPI_ASYNCH
    psObj->dma_usage = DMA_USAGE_NEVER;
    psObj->event = 0;
    psObj->hdlr_async = 0;
    psObj->dma_chn_id_tx = DMA_ERROR_OUT_OF_CHANNELS;
    psObj->dma_chn_id_rx = DMA_ERROR_OUT_OF_CHANNELS;
    
#endif

	int mode =0;
	if(psInitDef->ClockPolarity == 0){
		if(psInitDef->ClockPhase == SPI_PHASE_1EDGE)
			mode = 0;
		else
			mode = 1;
	}
	else{
		if(psInitDef->ClockPhase == SPI_PHASE_1EDGE)
			mode = 3;
		else
			mode = 2;
	}


	spi_format(psObj, psInitDef->Bits, mode, psInitDef->Mode, psInitDef->FirstBit, psInitDef->Direction);
	spi_frequency(psObj, psInitDef->BaudRate);
	
	return 0;
}

void SPI_Final(spi_t *obj)
{
#if DEVICE_SPI_ASYNCH
    if (obj->dma_chn_id_tx != DMA_ERROR_OUT_OF_CHANNELS) {
        dma_channel_free(obj->dma_chn_id_tx);
        obj->dma_chn_id_tx = DMA_ERROR_OUT_OF_CHANNELS;
    }
    if (obj->dma_chn_id_rx != DMA_ERROR_OUT_OF_CHANNELS) {
        dma_channel_free(obj->dma_chn_id_rx);
        obj->dma_chn_id_rx = DMA_ERROR_OUT_OF_CHANNELS;
    }
#endif

    SPI_Close((SPI_T *)obj->spi);

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->spi, spi_modinit_tab);
    if(modinit == NULL)
		return;
    if(modinit->modname != (int) obj->spi)
		return;

    SPI_DisableInt(((SPI_T *) obj->spi), (SPI_FIFO_RXOV_INT_MASK | SPI_FIFO_RXTH_INT_MASK | SPI_FIFO_TXTH_INT_MASK));
    NVIC_DisableIRQ(modinit->irq_n);

    // Disable IP clock
    CLK_DisableModuleClock(modinit->clkidx);
}

int SPI_SetDataWidth(
	spi_t *obj,
	int data_width_bits
)
{
    SPI_T *spi_base = (SPI_T *) obj->spi;

	if((data_width_bits != 8) && (data_width_bits != 16) && (data_width_bits != 32))
		return -1;

	SPI_SET_DATA_WIDTH(spi_base, data_width_bits);

    return 0;
}

int SPI_MasterReadOnly(spi_t *obj)
{
    SPI_T *spi_base = (SPI_T *) obj->spi;

    // NOTE: Data in receive FIFO can be read out via ICE.
    SPI_ENABLE_SYNC(spi_base);

    // Wait for rx buffer full
    while (! spi_readable(obj));
    int value2 = SPI_READ_RX(spi_base);

    /* We don't call SPI_DISABLE_SYNC here for performance. */

    return value2;
}


int SPI_MasterWrite(spi_t *obj, int value)
{
    SPI_T *spi_base = (SPI_T *) obj->spi;

    // NOTE: Data in receive FIFO can be read out via ICE.
    SPI_ENABLE_SYNC(spi_base);

    // Wait for tx buffer empty
    while(! spi_writeable(obj));
    SPI_WRITE_TX(spi_base, value);
	 
    // Wait for rx buffer full
    while (! spi_readable(obj));
    int value2 = SPI_READ_RX(spi_base);

    /* We don't call SPI_DISABLE_SYNC here for performance. */

    return value2;
}

int SPI_MasterBlockWriteRead(spi_t *obj, const char *tx_buffer, int tx_length,
                           char *rx_buffer, int rx_length, char write_fill) {
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

	while(i < total){
        if (i > tx_length){
			in = SPI_MasterWrite(obj, fill);			
		} 
		else{
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
		

		if((rx) && (i <= rx_length)){
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
                           char *rx_buffer, int rx_length, char write_fill) {

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

	while(i < tx_length){
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
	
	while(i < rx_length){

		in = SPI_MasterWrite(obj, fill);			
		printf("in is %x \n", in);

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
    SPI_T *spi_base = (SPI_T *)obj->spi;

    SPI_ENABLE_SYNC(spi_base);

    return spi_readable(obj);
};

int SPI_SlaveReadOnly(spi_t *obj)
{
    SPI_T *spi_base = (SPI_T *)obj->spi;

    SPI_ENABLE_SYNC(spi_base);

    // Wait for rx buffer full
    while (! spi_readable(obj));
    int value = SPI_READ_RX(spi_base);
    return value;
}

int SPI_SlaveWrite(spi_t *obj, int value)
{
    SPI_T *spi_base = (SPI_T *)obj->spi;

    SPI_ENABLE_SYNC(spi_base);

    // Wait for tx buffer empty
    while(! spi_writeable(obj));
    SPI_WRITE_TX(spi_base, value);

    // Wait for rx buffer full
    while (! spi_readable(obj));
    int value2 = SPI_READ_RX(spi_base);

    /* We don't call SPI_DISABLE_SYNC here for performance. */

    return value2;
}

int SPI_SlaveBlockWriteRead(spi_t *obj, const char *tx_buffer, int tx_length,
                           char *rx_buffer, int rx_length, char write_fill) {
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

	while(i < total){
        if (i > tx_length){
			in = SPI_MasterWrite(obj, fill);			
		} 
		else{
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
		

		if(i <= rx_length){
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

static int spi_writeable(spi_t * obj)
{
    // Receive FIFO must not be full to avoid receive FIFO overflow on next transmit/receive
    return (! SPI_GET_TX_FIFO_FULL_FLAG(((SPI_T *) obj->spi)));
}

static int spi_readable(spi_t * obj)
{
    return ! SPI_GET_RX_FIFO_EMPTY_FLAG(((SPI_T *) obj->spi));
}


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
	DMAUsage hint
)
{
    SPI_T *spi_base = (SPI_T *) obj->spi;
    SPI_SET_DATA_WIDTH(spi_base, bit_width);

    obj->dma_usage = hint;
    spi_check_dma_usage((DMAUsage *)&obj->dma_usage, &obj->dma_chn_id_tx, &obj->dma_chn_id_rx);
    uint32_t data_width = spi_get_data_width(obj);

// for testing
//	int i;
//	char *buf = (char *)tx;
//	for(i = 0; i < tx_length; i++){
//		buf[i] = i;
//	}

     // Conditions to go DMA way:
    // (1) No DMA support for non-8 multiple data width.
    // (2) tx length >= rx length. Otherwise, as tx DMA is done, no bus activity for remaining rx.
    if ((data_width % 8) ||
        (tx_length < rx_length)) {
        obj->dma_usage = DMA_USAGE_NEVER;
        dma_channel_free(obj->dma_chn_id_tx);
        obj->dma_chn_id_tx = DMA_ERROR_OUT_OF_CHANNELS;
        dma_channel_free(obj->dma_chn_id_rx);
        obj->dma_chn_id_rx = DMA_ERROR_OUT_OF_CHANNELS;
    }

     // SPI IRQ is necessary for both interrupt way and DMA way
    spi_enable_event(obj, event, 1);
    spi_buffer_set(obj, tx, tx_length, rx, rx_length);

    SPI_ENABLE_SYNC(spi_base);

    if (obj->dma_usage == DMA_USAGE_NEVER) {
        // Interrupt way
        spi_master_write_asynch(obj, spi_fifo_depth(obj) / 2);
        spi_enable_vector_interrupt(obj, handler, 1);
        spi_master_enable_interrupt(obj, 1);
    } else {
         // DMA way
        const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->spi, spi_modinit_tab);
        if(modinit == NULL)
			return;
        if(modinit->modname != (int) obj->spi)
			return;

       
        PDMA_T *pdma_base = dma_modbase();
		
		spi_base->PDMACTL &= ~(SPI_PDMACTL_TXPDMAEN_Msk | SPI_PDMACTL_RXPDMAEN_Msk);

         // Configure tx DMA
		if(tx_length){
			if(data_width == 16){
				tx_length = tx_length / 2;
			}
			else if(data_width == 32){
				tx_length = tx_length / 4;
			}

			pdma_base->CHCTL |= 1 << obj->dma_chn_id_tx;  // Enable this DMA channel
			PDMA_SetTransferMode(pdma_base, obj->dma_chn_id_tx,
								 ((struct nu_spi_var *) modinit->var)->pdma_perp_tx,    // Peripheral connected to this PDMA
								 0,  // Scatter-gather disabled
								 0); // Scatter-gather descriptor address
			PDMA_SetTransferCnt(pdma_base, obj->dma_chn_id_tx,
								(data_width == 8) ? PDMA_WIDTH_8 : (data_width == 16) ? PDMA_WIDTH_16 : PDMA_WIDTH_32,
								tx_length);
			PDMA_SetTransferAddr(pdma_base, obj->dma_chn_id_tx,
								 (uint32_t) tx,  // NOTE:
								 // NUC472: End of source address
								 // M451/M480: Start of source address
								 PDMA_SAR_INC,   // Source address incremental
								 (uint32_t) &spi_base->TX,   // Destination address
								 PDMA_DAR_FIX);  // Destination address fixed
			PDMA_SetBurstType(pdma_base, obj->dma_chn_id_tx,
							  PDMA_REQ_SINGLE,    // Single mode
							  0); // Burst size
			PDMA_EnableInt(pdma_base, obj->dma_chn_id_tx,
						   PDMA_INT_TRANS_DONE);   // Interrupt type
		   
			 // Register DMA event handler
			dma_set_handler(obj->dma_chn_id_tx, (uint32_t) spi_dma_handler_tx, (uint32_t) obj, DMA_EVENT_ALL);

		}

        // Configure rx DMA
		if(rx_length){
			if(data_width == 16){
				rx_length = rx_length / 2;
			}
			else if(data_width == 32){
				rx_length = rx_length / 4;
			}
		
			pdma_base->CHCTL |= 1 << obj->dma_chn_id_rx;  // Enable this DMA channel
			PDMA_SetTransferMode(pdma_base, obj->dma_chn_id_rx,
								 ((struct nu_spi_var *) modinit->var)->pdma_perp_rx,    // Peripheral connected to this PDMA
								 0,  // Scatter-gather disabled
								 0); // Scatter-gather descriptor address
			PDMA_SetTransferCnt(pdma_base, obj->dma_chn_id_rx,
								(data_width == 8) ? PDMA_WIDTH_8 : (data_width == 16) ? PDMA_WIDTH_16 : PDMA_WIDTH_32,
								rx_length);
			PDMA_SetTransferAddr(pdma_base, obj->dma_chn_id_rx,
								 (uint32_t) &spi_base->RX,   // Source address
								 PDMA_SAR_FIX,   // Source address fixed
								 (uint32_t) rx,  // NOTE:
								 // NUC472: End of destination address
								 // M451/M480: Start of destination address
								 PDMA_DAR_INC);  // Destination address incremental
			PDMA_SetBurstType(pdma_base, obj->dma_chn_id_rx,
							  PDMA_REQ_SINGLE,    // Single mode
							  0); // Burst size
			PDMA_EnableInt(pdma_base, obj->dma_chn_id_rx,
						   PDMA_INT_TRANS_DONE);   // Interrupt type
			 // Register DMA event handler
			dma_set_handler(obj->dma_chn_id_rx, (uint32_t) spi_dma_handler_rx, (uint32_t) obj, DMA_EVENT_ALL);
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
		if(tx_length && rx_length){
			spi_base->PDMACTL |= (SPI_PDMACTL_TXPDMAEN_Msk | SPI_PDMACTL_RXPDMAEN_Msk);
		}
		else{
			if(tx_length)
				SPI_TRIGGER_TX_PDMA(spi_base);
				
			if(rx_length)
				SPI_TRIGGER_RX_PDMA(spi_base);
		}

        /* Don't enable SPI TX/RX threshold interrupts as commented above */

		//PDMA_Trigger(pdma_base, obj->spi.dma_chn_id_rx);
        //PDMA_Trigger(pdma_base, obj->spi.dma_chn_id_tx);

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
    SPI_T *spi_base = (SPI_T *) obj->spi;
    PDMA_T *pdma_base = dma_modbase();
	printf("spi_abort_asynch 0 \n");
    if (obj->dma_usage != DMA_USAGE_NEVER) {
        // Receive FIFO Overrun in case of tx length > rx length on DMA way
        if (spi_base->STATUS & SPI_STATUS_RXOVIF_Msk) {
            spi_base->STATUS = SPI_STATUS_RXOVIF_Msk;
        }

	printf("spi_abort_asynch 1 \n");
        if (obj->dma_chn_id_tx != DMA_ERROR_OUT_OF_CHANNELS) {
	printf("spi_abort_asynch 2 \n");
            PDMA_DisableInt(pdma_base, obj->dma_chn_id_tx, PDMA_INT_TRANS_DONE);
            // NOTE: On NUC472, next PDMA transfer will fail with PDMA_STOP() called. Cause is unknown.
            pdma_base->CHCTL &= ~(1 << obj->dma_chn_id_tx);
        }
	printf("spi_abort_asynch 3 \n");
        SPI_DISABLE_TX_PDMA(((SPI_T *) obj->spi));

        if (obj->dma_chn_id_rx != DMA_ERROR_OUT_OF_CHANNELS) {
	printf("spi_abort_asynch 4 \n");
            PDMA_DisableInt(pdma_base, obj->dma_chn_id_rx, PDMA_INT_TRANS_DONE);
            // NOTE: On NUC472, next PDMA transfer will fail with PDMA_STOP() called. Cause is unknown.
            pdma_base->CHCTL &= ~(1 << obj->dma_chn_id_rx);
        }
	printf("spi_abort_asynch 5 \n");
        SPI_DISABLE_RX_PDMA(((SPI_T *) obj->spi));
    }

    // Necessary for both interrupt way and DMA way
	printf("spi_abort_asynch 6 \n");
    spi_enable_vector_interrupt(obj, 0, 0);
	printf("spi_abort_asynch 7 \n");
    spi_master_enable_interrupt(obj, 0);
	printf("spi_abort_asynch 8 \n");

    /* Necessary for accessing FIFOCTL below */
	printf("spi_abort_asynch 9 \n");
    SPI_DISABLE_SYNC(spi_base);

	printf("spi_abort_asynch 10 \n");
    SPI_ClearRxFIFO(spi_base);
	printf("spi_abort_asynch 11 \n");
    SPI_ClearTxFIFO(spi_base);
}

static void spi_check_dma_usage(DMAUsage *dma_usage, int *dma_ch_tx, int *dma_ch_rx)
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
//    return (obj->event & event) | ((event & SPI_EVENT_COMPLETE) ? SPI_EVENT_INTERNAL_TRANSFER_COMPLETE : 0);
}

static uint8_t spi_get_data_width(spi_t *obj)
{    
    SPI_T *spi_base = (SPI_T *) obj->spi;

    uint32_t data_width = ((spi_base->CTL & SPI_CTL_DWIDTH_Msk) >> SPI_CTL_DWIDTH_Pos);
    if (data_width == 0) {
        data_width = 32;
    }

    return data_width;
}

static void spi_enable_event(spi_t *obj, uint32_t event, uint8_t enable)
{
    obj->event &= ~SPI_EVENT_ALL;
//    obj->event |= (event & SPI_EVENT_ALL);
    if (event & SPI_EVENT_RX_OVERFLOW) {
        SPI_EnableInt((SPI_T *) obj->spi, SPI_FIFOCTL_RXOVIEN_Msk);
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

int is_spi_trans_done(spi_t *obj){
    if (spi_is_tx_complete(obj) && spi_is_rx_complete(obj)) {
		return 1;
	}
	return 0;
}


static uint32_t spi_event_check(spi_t *obj)
{
    SPI_T *spi_base = (SPI_T *) obj->spi;
    uint32_t event = 0;

    if (obj->dma_usage == DMA_USAGE_NEVER) {
        uint32_t n_rec = spi_master_read_asynch(obj);
        spi_master_write_asynch(obj, n_rec);
    }

    if (spi_is_tx_complete(obj) && spi_is_rx_complete(obj)) {
		printf("spi_event_check complete \n");
        event |= SPI_EVENT_COMPLETE;
    }

    // Receive FIFO Overrun
    if (spi_base->STATUS & SPI_STATUS_RXOVIF_Msk) {
        spi_base->STATUS = SPI_STATUS_RXOVIF_Msk;
		printf("spi_event_check error 0 \n");
        // In case of tx length > rx length on DMA way
        if (obj->dma_usage == DMA_USAGE_NEVER) {
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
    SPI_T *spi_base = (SPI_T *)obj->spi;

//    while ((n_words < max_tx) && spi_writeable(obj)) {
    if ((n_words < max_tx)) {
		while(!spi_writeable(obj));
        if (spi_is_tx_complete(obj)) {
            // Transmit dummy as transmit buffer is empty
            SPI_WRITE_TX(spi_base, 0);
        } else {
            switch (bytes_per_word) {
            case 4:
                SPI_WRITE_TX(spi_base, nu_get32_le(tx));
                tx += 4;
                break;
            case 2:
                SPI_WRITE_TX(spi_base, nu_get16_le(tx));
                tx += 2;
                break;
            case 1:
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
    SPI_T *spi_base = (SPI_T *)obj->spi;

//    while ((n_words < max_rx) && spi_readable(obj)) {
    if ((n_words < max_rx)) {
		while(!spi_readable(obj));
        if (spi_is_rx_complete(obj)) {
            // Disregard as receive buffer is full
            SPI_READ_RX(spi_base);
        } else {
            switch (bytes_per_word) {
            case 4: {
                uint32_t val = SPI_READ_RX(spi_base);
                nu_set32_le(rx, val);
                rx += 4;
                break;
            }
            case 2: {
                uint16_t val = SPI_READ_RX(spi_base);
                nu_set16_le(rx, val);
                rx += 2;
                break;
            }
            case 1:
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

static void spi_master_enable_interrupt(spi_t *obj, uint8_t enable)
{
    SPI_T *spi_base = (SPI_T *) obj->spi;

    if (enable) {
        uint32_t fifo_depth = spi_fifo_depth(obj);
        SPI_SetFIFO(spi_base, fifo_depth / 2, fifo_depth / 2);
        // Enable tx/rx FIFO threshold interrupt
        SPI_EnableInt(spi_base, SPI_FIFO_RXTH_INT_MASK | SPI_FIFO_TXTH_INT_MASK);
    } else {
        SPI_DisableInt(spi_base, SPI_FIFO_RXTH_INT_MASK | SPI_FIFO_TXTH_INT_MASK);
    }
}

static void spi_enable_vector_interrupt(spi_t *obj, uint32_t handler, uint8_t enable)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->spi, spi_modinit_tab);
    if(modinit == NULL)
		return;
    if(modinit->modname != (int) obj->spi)
		return;
    
    struct nu_spi_var *var = (struct nu_spi_var *) modinit->var;
    
    if (enable) {
        var->obj = obj;
        obj->hdlr_async = handler;
        /* Fixed vector table is fixed in ROM and cannot be modified. */
        NVIC_EnableIRQ(modinit->irq_n);
    }
    else {
        NVIC_DisableIRQ(modinit->irq_n);
        /* Fixed vector table is fixed in ROM and cannot be modified. */
        var->obj = NULL;
        obj->hdlr_async = 0;
    }
}

static void spi_dma_handler_tx(uint32_t id, uint32_t event_dma)
{
    spi_t *obj = (spi_t *) id;
	
    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_ABORT) {
		printf("spi_dma_handler_tx 0 \n");
    }
    // Expect SPI IRQ will catch this transfer done event
    if (event_dma & DMA_EVENT_TRANSFER_DONE) {
        obj->tx_buff.pos = obj->tx_buff.length;
		printf("spi_dma_handler_tx 1 \n");
    }
    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_TIMEOUT) {
		printf("spi_dma_handler_tx 2 \n");
    }

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->spi, spi_modinit_tab);
    if(modinit == NULL)
		return;
    if(modinit->modname != (int) obj->spi)
		return;

#if 0
//    void (*vec)(void) = (void (*)(void)) NVIC_GetVector(modinit->irq_n);
	struct nu_spi_var *spi_var = (struct nu_spi_var *)modinit->var;
    void (*vec)(void) = (void (*)(void))(spi_var->vec);
    vec();
#else
    if (obj && obj->hdlr_async) {
       void (*hdlr_async)(spi_t *) = (void(*)(spi_t *))(obj->hdlr_async);
		printf("spi_dma_handler_tx 3 \n");
       hdlr_async(obj);
    }
#endif
}


static void spi_dma_handler_rx(uint32_t id, uint32_t event_dma)
{
    spi_t *obj = (spi_t *) id;

    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_ABORT) {
		printf("spi_dma_handler_rx 0 \n");
    }
    // Expect SPI IRQ will catch this transfer done event
    if (event_dma & DMA_EVENT_TRANSFER_DONE) {
        obj->rx_buff.pos = obj->rx_buff.length;
		printf("spi_dma_handler_rx 1 \n");
    }
    // TODO: Pass this error to caller
    if (event_dma & DMA_EVENT_TIMEOUT) {
		printf("spi_dma_handler_rx 2 \n");
    }

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)obj->spi, spi_modinit_tab);
    if(modinit == NULL)
		return;
    if(modinit->modname != (int) obj->spi)
		return;

#if 0
//    void (*vec)(void) = (void (*)(void)) NVIC_GetVector(modinit->irq_n);
	struct nu_spi_var *spi_var = (struct nu_spi_var *)modinit->var;
    void (*vec)(void) = (void (*)(void))(spi_var->vec);
    vec();
#else
    if (obj && obj->hdlr_async) {
        void (*hdlr_async)(spi_t *) = (void(*)(spi_t *))(obj->hdlr_async);
		printf("spi_dma_handler_rx 3 \n");
        hdlr_async(obj);
    }

#endif
}


/** Return FIFO depth of the SPI peripheral
 *
 * @details
 *  M487
 *      SPI0            8
 *      SPI1/2/3/4      8 if data width <=16; 4 otherwise
 */
static uint32_t spi_fifo_depth(spi_t *obj)
{
    SPI_T *spi_base = (SPI_T *) obj->spi;

    if (spi_base == SPI0) {
        return 8;
    }

    return (spi_get_data_width(obj) <= 16) ? 8 : 4;
}

#endif


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

 


