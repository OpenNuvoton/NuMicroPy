/**************************************************************************//**
 * @file     M55M1_I2C.h
 * @version  V1.00
 * @brief    M55M1 I2C HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M55M1_I2C_H__
#define __M55M1_I2C_H__

#define I2C_MODE_MASTER		(0)
#define I2C_MODE_SLAVE		(1)

typedef struct {
    union {
        I2C_T *i2c;
        LPI2C_T *lpi2c;
    } u_i2c;
    bool bLPI2C;
} i2c_t;

typedef struct {
    uint32_t Mode;		//Master/Slave mode: I2C_MODE_MASTER/I2C_MODE_SLAVE
    uint32_t BaudRate;	//Baud rate
    uint8_t GeneralCallMode;	//Support general call mode: I2C_GCMODE_ENABLE/I2C_GCMODE_DISABLE
    uint8_t OwnAddress;	//Slave own address
} I2C_InitTypeDef;

typedef struct {
    uint8_t u8SlaveAddr;	//Used by master
    uint8_t *pu8DataAddr;	//Used by master data address access, it can be one/two bytes
    uint8_t u8DataAddrLen;	//Used by master
    uint8_t *pu8Data;
    uint32_t u32DataLen;
} I2C_TRANS_PARAM;

int32_t I2C_Init(
    i2c_t *psI2CObj,
    I2C_InitTypeDef *psInitDef
);

void I2C_Final(
    i2c_t *psI2CObj
);


#define I2C_DEVICE_OK (0)
#define I2C_DEVICE_BUSY (1)
#define I2C_DEVICE_ERROR (2)

int32_t I2C_DeviceReady(
    i2c_t *psI2CObj,
    uint8_t u8SlaveAddr
);

void Handle_I2C_Irq(I2C_T *i2c, uint32_t u32Status);
void Handle_LPI2C_Irq(LPI2C_T *lpi2c, uint32_t u32Status);

int I2C_MaterSendRecv(
    i2c_t *psI2CObj,
    uint8_t u8Recv,
    I2C_TRANS_PARAM *psParam,
    uint32_t u32TimeOut
);

int I2C_SlaveSendRecv(
    i2c_t *psI2CObj,
    uint8_t u8Recv,
    I2C_TRANS_PARAM *psParam,
    uint32_t u32TimeOut
);

int32_t I2C_HookIRQHandler(
    i2c_t *psI2CObj,
    uint8_t u8Recv,
    uint8_t u8Master
);

#endif
