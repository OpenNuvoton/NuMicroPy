/**************************************************************************//**
 * @file     M48x_I2C.h
 * @version  V1.00
 * @brief    M480 I2C HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M48X_I2C_H__
#define __M48X_I2C_H__

#define I2C_MODE_MASTER		(0)
#define I2C_MODE_SLAVE		(1)


typedef struct{
	uint32_t Mode;		//Master/Slave mode: I2C_MODE_MASTER/I2C_MODE_SLAVE
	uint32_t BaudRate;	//Baud rate
	uint8_t GeneralCallMode;	//Support general call mode: I2C_GCMODE_ENABLE/I2C_GCMODE_DISABLE
	uint8_t OwnAddress;	//Slave own address 
}I2C_InitTypeDef;

typedef struct{
	uint8_t u8SlaveAddr;	//Used by master
	uint8_t *pu8DataAddr;	//Used by master data address access, it can be one/two bytes
	uint8_t u8DataAddrLen;	//Used by master
	uint8_t *pu8Data;		
	uint32_t u32DataLen;
}I2C_TRANS_PARAM;

int32_t I2C_Init(
	I2C_T *i2c,
	I2C_InitTypeDef *psInitDef
);

void I2C_Final(
	I2C_T *i2c
);


#define I2C_DEVICE_OK (0)
#define I2C_DEVICE_BUSY (1)
#define I2C_DEVICE_ERROR (2)

int32_t I2C_DeviceReady(
	I2C_T *i2c,
	uint8_t u8SlaveAddr
);

void Handle_I2C_Irq(I2C_T *i2c, uint32_t u32Status);

int I2C_MaterSendRecv(
	I2C_T *i2c,
	uint8_t u8Recv,
	I2C_TRANS_PARAM *psParam,
	uint32_t u32TimeOut
);

int I2C_SlaveSendRecv(
	I2C_T *i2c,
	uint8_t u8Recv,
	I2C_TRANS_PARAM *psParam,
	uint32_t u32TimeOut
);

int32_t I2C_HookIRQHandler(
	I2C_T *i2c,
	uint8_t u8Recv,
	uint8_t u8Master
);


#endif
