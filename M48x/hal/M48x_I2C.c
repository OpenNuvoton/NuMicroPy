/**************************************************************************//**
 * @file     M48x_I2C.c
 * @version  V0.01
 * @brief    M480 series I2C HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"
#include "M48x_I2C.h"


int32_t I2C_Init(
	I2C_T *i2c,
	I2C_InitTypeDef *psInitDef
)
{

	/* Enable I2C0 module clock */
	if(i2c == I2C0){
		// Reset this module
		SYS_ResetModule(I2C0_RST);
		CLK_EnableModuleClock(I2C0_MODULE);
		NVIC_EnableIRQ(I2C0_IRQn);
	}
	else if(i2c == I2C1){
		// Reset this module
		SYS_ResetModule(I2C1_RST);
		CLK_EnableModuleClock(I2C1_MODULE);
		NVIC_EnableIRQ(I2C1_IRQn);
	}
	else if(i2c == I2C2){
		// Reset this module
		SYS_ResetModule(I2C2_RST);
		CLK_EnableModuleClock(I2C2_MODULE);
		NVIC_EnableIRQ(I2C2_IRQn);
	}
	else
		return -1;

    SystemCoreClockUpdate();

	I2C_Open(i2c, psInitDef->BaudRate);
	
	if(psInitDef->Mode == I2C_MODE_SLAVE){
		/* Set I2C0  Slave Addresses 0*/
		I2C_SetSlaveAddr(i2c, 0, psInitDef->OwnAddress, psInitDef->GeneralCallMode);   /* Slave Address : 0x15 */
		I2C_SetSlaveAddrMask(i2c, 0, 0x01);

		/* I2C enter no address SLV mode */
		I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI | I2C_CTL_AA);
	}


//	I2C_EnableInt(i2c);
	return 0;
}

static int32_t I2C_WatiReady(
	I2C_T *i2c,
	uint32_t u32TimeoutMS
)
{
	uint32_t u32EndTick = mp_hal_ticks_ms() + u32TimeoutMS;
	
	while(!((i2c)->CTL0 & I2C_CTL0_SI_Msk)){
		if(mp_hal_ticks_ms() > u32EndTick)
			return -1;
	}

	return 0;
}

void I2C_Final(
	I2C_T *i2c
)
{

	/* Enable I2C0 module clock */
	if(i2c == I2C0){
		CLK_DisableModuleClock(I2C0_MODULE);
		NVIC_DisableIRQ(I2C0_IRQn);
	}
	else if(i2c == I2C1){
		CLK_DisableModuleClock(I2C1_MODULE);
		NVIC_DisableIRQ(I2C1_IRQn);
	}
	else if(i2c == I2C2){
		CLK_DisableModuleClock(I2C2_MODULE);
		NVIC_DisableIRQ(I2C2_IRQn);
	}
	else
		return;

	I2C_DisableInt(i2c);
	I2C_Close(i2c);
}

int32_t I2C_DeviceReady(
	I2C_T *i2c,
	uint8_t u8SlaveAddr
)
{
	uint8_t u8Xfering = 1u, u8Err = 0u, u8Ctrl = 0u;
//    I2C_DisableInt(i2c);

    I2C_START(i2c);

	uint32_t u32Status;
    while(u8Xfering && (u8Err == 0u))
    {
        if(I2C_WatiReady(i2c, 200) < 0){
			u8Ctrl = I2C_CTL_STO_SI;                         /* Clear SI and send STOP */
			I2C_SET_CONTROL_REG(i2c, u8Ctrl);                          /* Write controlbit to I2C_CTL register */
			return 3u;		//time out
		}

		u32Status = I2C_GET_STATUS(i2c);
        switch(u32Status)
        {
        case 0x08u:
            I2C_SET_DATA(i2c, (uint8_t)((u8SlaveAddr << 1u)));    /* Write SLA+W to Register I2CDAT */
            u8Ctrl = I2C_CTL_SI;                             /* Clear SI */
            break;
        case 0x18u:                                             /* Slave Address ACK */
            u8Ctrl = I2C_CTL_STO_SI;                         /* Clear SI and send STOP */
            u8Err = 0u;
            u8Xfering = 0u;		//done
            break;
        case 0x48u:                                             /* Slave Address NACK */
            u8Ctrl = I2C_CTL_STO_SI;                         /* Clear SI and send STOP */
            u8Err = 1u;			//nack
            break;
        case 0x38u:                                             /* Arbitration Lost */
        default:                                               /* Unknow status */
            u8Ctrl = I2C_CTL_STO_SI;                         /* Clear SI and send STOP */
            u8Err = 2u;			//others
            break;
        }
        I2C_SET_CONTROL_REG(i2c, u8Ctrl);                          /* Write controlbit to I2C_CTL register */
    }

	return u8Err;
}

void Handle_I2C_Irq(I2C_T *i2c, uint32_t u32Status) {
//TODO:


}


