/**************************************************************************//**
 * @file     M48x_SPI.h
 * @version  V1.00
 * @brief    M480 SPI HAL header file for micropython
 *
  * SPDX-License-Identifier: Apache-2.0
* @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M48X_SPI_H__
#define __M48X_SPI_H__

#include "buffer.h"
#include "dma.h"


#define SPI_DIRECTION_2LINES            0x00000000U
#define SPI_DIRECTION_2LINES_RXONLY     SPI_CTL_RXONLY_Msk
#define SPI_DIRECTION_1LINE             SPI_CTL_HALFDPX_Msk

/** @defgroup SPI_MSB_LSB_transmission SPI MSB LSB Transmission
  * @{
  */
#define SPI_FIRSTBIT_MSB                0x00000000U
#define SPI_FIRSTBIT_LSB                SPI_CTL_LSB_Msk

/** @defgroup SPI_Clock_Phase SPI Clock Phase
  * @{
  */
#define SPI_PHASE_1EDGE                 0x00000000U
#define SPI_PHASE_2EDGE                 0x00000001U

#define SPI_EVENT_ERROR       (1 << 1)
#define SPI_EVENT_COMPLETE    (1 << 2)
#define SPI_EVENT_RX_OVERFLOW (1 << 3)
#define SPI_EVENT_ALL         (SPI_EVENT_ERROR | SPI_EVENT_COMPLETE | SPI_EVENT_RX_OVERFLOW)

#define SPI_EVENT_INTERNAL_TRANSFER_COMPLETE (1 << 30) // Internal flag to report that an event occurred


typedef struct{
	uint32_t Mode;		//Master/Slave mode
	uint32_t BaudRate;	//Baud rate
	uint32_t ClockPolarity;	//Closk polarity
	uint32_t Direction; //SPI_DIRECTION_2LINES/SPI_DIRECTION_2LINES_RXONLY/SPI_DIRECTION_1LINE
	uint32_t Bits; //Data width
	uint32_t FirstBit;	//First bit LSB/MSB
	uint32_t ClockPhase; //Data sampling at SPI_PHASE_1EDGE/SPI_PHASE_2EDGE. ClockPolarity + ClockPhase = SPI mode

}SPI_InitTypeDef;

typedef struct{
	SPI_T *spi;
	int dma_usage;
    int         dma_chn_id_tx;
    int         dma_chn_id_rx;
    uint32_t    event;
    uint32_t    hdlr_async;
	struct buffer_s tx_buff; /**< Tx buffer */
    struct buffer_s rx_buff; /**< Rx buffer */
}spi_t;

int32_t SPI_Init(
	spi_t *psObj,
	SPI_InitTypeDef *psInitDef
);

void SPI_Final(spi_t *obj);


int SPI_MasterBlockWriteRead(
	spi_t *obj, 
	const char *tx_buffer, 
	int tx_length,
	char *rx_buffer, 
	int rx_length, 
	char write_fill
);

int SPI_SlaveBlockWriteRead(
	spi_t *obj, 
	const char *tx_buffer, 
	int tx_length,
	char *rx_buffer, 
	int rx_length, 
	char write_fill
);

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
);

void spi_irq_handler_asynch(spi_t *obj);

int is_spi_trans_done(spi_t *obj);
void Handle_SPI_Irq(SPI_T *spi);


#endif
