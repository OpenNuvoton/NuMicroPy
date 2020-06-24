/**************************************************************************//**
 * @file     M48x_I2S.h
 * @version  V1.00
 * @brief    M480 I2S HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M48X_I2S_H__
#define __M48X_I2S_H__

#include "buffer.h"
#include "dma.h"

#define I2S_EVENT_ERROR       (1 << 1)
#define I2S_EVENT_COMPLETE    (1 << 2)
#define I2S_EVENT_RX_OVERFLOW (1 << 3)
#define I2S_EVENT_DMA_TX_DONE (1 << 4)
#define I2S_EVENT_DMA_RX_DONE (1 << 5)

#define I2S_EVENT_ALL         (I2S_EVENT_ERROR | I2S_EVENT_COMPLETE | I2S_EVENT_RX_OVERFLOW | I2S_EVENT_DMA_TX_DONE | I2S_EVENT_DMA_RX_DONE)

#define I2S_EVENT_INTERNAL_TRANSFER_COMPLETE (1 << 30) // Internal flag to report that an event occurred

typedef void (*pfnTransHandler)(void *psObj);

typedef struct{
	I2S_T *i2s;
	int dma_usage;
    int         dma_chn_id_tx;
    int         dma_chn_id_rx;
    uint32_t    event;
	pfnTransHandler hdl; 
	struct buffer_s tx_buff; /**< Tx buffer */
    struct buffer_s rx_buff; /**< Rx buffer */
	void *pvPriv;
}i2s_t;


/**
  * @brief  This function configures some parameters of I2S interface for general purpose use.
  *         The sample rate may not be used from the parameter, it depends on system's clock settings,
  *         but real sample rate used by system will be returned for reference.
  * @param[in] u32MasterSlave I2S operation mode. Valid values are:
  *                                     - \ref I2S_MODE_MASTER
  *                                     - \ref I2S_MODE_SLAVE
  * @param[in] u32SampleRate Sample rate
  * @param[in] u32WordWidth Data length. Valid values are:
  *                                     - \ref I2S_DATABIT_8
  *                                     - \ref I2S_DATABIT_16
  *                                     - \ref I2S_DATABIT_24
  *                                     - \ref I2S_DATABIT_32
  * @param[in] u32MonoData: Set audio data to mono or not. Valid values are:
  *                                     - \ref I2S_ENABLE_MONO
  *                                     - \ref I2S_DISABLE_MONO
  * @param[in] u32DataFormat: Data format. This is also used to select I2S or PCM(TDM) function. Valid values are:
  *                                     - \ref I2S_FORMAT_I2S
  *                                     - \ref I2S_FORMAT_I2S_MSB
  *                                     - \ref I2S_FORMAT_I2S_LSB
  *                                     - \ref I2S_FORMAT_PCM
  *                                     - \ref I2S_FORMAT_PCM_MSB
  *                                     - \ref I2S_FORMAT_PCM_LSB
  */

typedef struct{
	uint32_t Mode;			//Master/Slave mode: I2C_MODE_MASTER/I2C_MODE_SLAVE
	uint32_t SampleRate;	//Sample rate
	uint32_t WordWidth;		//Data word width 8/16/24/32
	uint32_t MonoData;		//Audio data mono or not
	uint32_t DataFormat;	//Data format	
}I2S_InitTypeDef;

typedef struct{
	const void *tx; 
	size_t tx_length; 
	const void *tx1;		//Used by PDMA transfer
	size_t tx1_length;		//Used by PDMA transfer
	void *rx; 
	size_t rx_length; 
	void *rx1;				//Used by PDMA transfer
	size_t rx1_length; 		//Used by PDMA transfer
}I2S_TransParam;


int32_t I2S_Init(
	i2s_t *psObj,
	I2S_InitTypeDef *psInitDef
);

void I2S_Final(i2s_t *psObj);

void I2S_StartTransfer(
	i2s_t *psObj,
	I2S_TransParam *psTransParam,
	pfnTransHandler handler, 
	uint32_t event, 
	DMAUsage hint
);

void I2S_StopTransfer(
	i2s_t *psObj
);

void Handle_I2S_Irq(I2S_T *i2s);


#endif
