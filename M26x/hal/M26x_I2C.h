/**************************************************************************//**
 * @file     M26x_I2C.h
 * @version  V1.00
 * @brief    M26x I2C HAL header file for micropython
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M26X_I2C_H__
#define __M26X_I2C_H__

#define I2C_MODE_MASTER		(0)
#define I2C_MODE_SLAVE		(1)


typedef struct{
	uint32_t Mode;		//Master/Slave mode: I2C_MODE_MASTER/I2C_MODE_SLAVE
	uint32_t BaudRate;	//Baud rate
	uint8_t GeneralCallMode;	//Support general call mode: I2C_GCMODE_ENABLE/I2C_GCMODE_DISABLE
	uint8_t OwnAddress;	//Slave own address 
}I2C_InitTypeDef;

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

#endif
