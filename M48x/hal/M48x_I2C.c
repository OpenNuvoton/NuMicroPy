/**************************************************************************//**
 * @file     M48x_I2C.c
 * @version  V0.01
 * @brief    M480 series I2C HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "py/mphal.h"

#include "NuMicro.h"
#include "M48x_I2C.h"

#define MAX_I2C_INST 3


typedef struct{
	uint8_t u8TransRx;		//Read/Write tarns
	uint8_t u8DataAddrTrans;
	uint32_t u32DataTrans;
	uint8_t u8InTrans;
	uint8_t u8EndFlag;
	uint8_t u8State;
}I2C_TRANS_PRIV;

typedef void (*PFN_TRANS_IRQ)(I2C_T *i2c, uint32_t u32Status, I2C_TRANS_PARAM *psParam, I2C_TRANS_PRIV *psPriv);

typedef struct{
	I2C_TRANS_PARAM *psTransParam;
	PFN_TRANS_IRQ pfnTransIRQ;
	I2C_TRANS_PRIV sTransPriv;
	uint8_t bHookIRQ;
}I2C_TRANS_HANDLER;

I2C_TRANS_HANDLER s_asI2CTransHandler[MAX_I2C_INST];

int32_t I2C_Init(
	I2C_T *i2c,
	I2C_InitTypeDef *psInitDef
)
{
	int i32Inst;

	/* Enable I2C0 module clock */
	if(i2c == I2C0){
		// Reset this module
		SYS_ResetModule(I2C0_RST);
		CLK_EnableModuleClock(I2C0_MODULE);
		NVIC_EnableIRQ(I2C0_IRQn);
		i32Inst = 0;
	}
	else if(i2c == I2C1){
		// Reset this module
		SYS_ResetModule(I2C1_RST);
		CLK_EnableModuleClock(I2C1_MODULE);
		NVIC_EnableIRQ(I2C1_IRQn);
		i32Inst = 1;
	}
	else if(i2c == I2C2){
		// Reset this module
		SYS_ResetModule(I2C2_RST);
		CLK_EnableModuleClock(I2C2_MODULE);
		NVIC_EnableIRQ(I2C2_IRQn);
		i32Inst = 2;
	}
	else
		return -1;

    SystemCoreClockUpdate();

	I2C_Open(i2c, psInitDef->BaudRate);
	
	if(psInitDef->Mode == I2C_MODE_SLAVE){
		/* Set slave Addresses */
		I2C_SetSlaveAddr(i2c, 0, psInitDef->OwnAddress, psInitDef->GeneralCallMode);
	}

	memset(&s_asI2CTransHandler[i32Inst], 0x0 , sizeof(I2C_TRANS_HANDLER));

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
//            I2C_SET_DATA(i2c, (uint8_t)((u8SlaveAddr << 1u) | 0x01u));    /* Write SLA+R to Register I2CDAT */
            u8Ctrl = I2C_CTL_SI;                             /* Clear SI */
            break;
        case 0x18u:                                             /* Slave Address ACK */
		case 0x40u: //Master receive addresss ACK
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

static void I2C_MasterRxTx_IRQ(I2C_T *i2c, uint32_t u32Status, I2C_TRANS_PARAM *psParam, I2C_TRANS_PRIV *psPriv)
{
	uint8_t u8Ctrl = 0u;

	if(psPriv == NULL)
	{
		u8Ctrl = I2C_CTL_STO_SI;
        I2C_SET_CONTROL_REG(i2c, u8Ctrl);                          /* Write controlbit to I2C_CTL register */		
		return;
	}

//	printf("I2C master status %x\n", u32Status);
	switch(u32Status)
	{
	case 0x08u:	//Start
		if(psParam->u8DataAddrLen)
			I2C_SET_DATA(i2c, (uint8_t)((psParam->u8SlaveAddr << 1u) | 0x00u));		/* Write SLA+W to Register I2CDAT */
		else
			I2C_SET_DATA(i2c, (uint8_t)((psParam->u8SlaveAddr << 1u) | psPriv->u8TransRx));		/* Write SLA+W or SLA+R to Register I2CDAT */
		u8Ctrl = I2C_CTL_SI;                               			/* Clear SI */
	break;
	case 0x10u:	//Master Repeat start
		I2C_SET_DATA(i2c, (uint8_t)((psParam->u8SlaveAddr << 1u) | psPriv->u8TransRx));    /* Write SLA+R to Register I2CDAT */
		u8Ctrl = I2C_CTL_SI;                               /* Clear SI */
	break;
	case 0x18u:	//Master transmit address ACK (for TX access or data address stage)
		u8Ctrl = I2C_CTL_SI;                               /* Clear SI */

		if(psParam == NULL)
		{
			u8Ctrl = I2C_CTL_STO_SI;
		}
		else if(psParam->u8DataAddrLen)
		{
			I2C_SET_DATA(i2c, psParam->pu8DataAddr[psPriv->u8DataAddrTrans]);                          /* Write data to I2CDAT */

			psParam->u8DataAddrLen --;
			psPriv->u8DataAddrTrans ++;
		}
		else if(psParam->u32DataLen)
		{
			if(psPriv->u8TransRx == 0)		//TX access
			{
				I2C_SET_DATA(i2c, psParam->pu8Data[psPriv->u32DataTrans]);                          /* Write data to I2CDAT */
				psParam->u32DataLen --;
				psPriv->u32DataTrans ++;
			}
		}
		else
		{
			psPriv->u8EndFlag = 1;
			u8Ctrl = I2C_CTL_STO_SI;
		}	
	break;
	case 0x20u:	// Master transmit address NACK
	case 0x30u: // Master transmit data NACK
		psPriv->u8EndFlag = 1;
		u8Ctrl = I2C_CTL_STO_SI;                                  /* Clear SI and send STOP */
	break;
	case 0x28u:	// Master transmit data ACK
		u8Ctrl = I2C_CTL_SI;                               /* Clear SI */
		if(psParam->u8DataAddrLen)
		{
			I2C_SET_DATA(i2c, psParam->pu8DataAddr[psPriv->u8DataAddrTrans]);                          /* Write data to I2CDAT */

			psParam->u8DataAddrLen --;
			psPriv->u8DataAddrTrans ++;
		}
		else if(psParam->u32DataLen)
		{
			if(psPriv->u8TransRx == 0)		//TX access
			{
				I2C_SET_DATA(i2c, psParam->pu8Data[psPriv->u32DataTrans]);                          /* Write data to I2CDAT */

				psParam->u32DataLen --;
				psPriv->u32DataTrans ++;
			}
			else 		//RX access
			{
				u8Ctrl = I2C_CTL_STA_SI;
			}
		}
		else
		{
			psPriv->u8EndFlag = 1;
			u8Ctrl = I2C_CTL_STO_SI;
		}

	break;
	case 0x40u: //Master receive addresss ACK
		if(psParam == NULL)
			u8Ctrl = I2C_CTL_SI;
		else if(psParam->u32DataLen > 1)
			u8Ctrl = I2C_CTL_SI_AA; 	/* Clear SI and set ACK */
		else
			u8Ctrl = I2C_CTL_SI;        /* Clear SI */
	break;
	case 0x50u:	//Master receive data ACK
		if(psParam->u32DataLen)
		{
			psParam->pu8Data[psPriv->u32DataTrans] = (unsigned char) I2C_GET_DATA(i2c);
			psPriv->u32DataTrans ++;
			psParam->u32DataLen --;
		}

		if(psParam->u32DataLen)
		{
			u8Ctrl = I2C_CTL_SI_AA;                             /* Clear SI and set ACK */
		}
		else
		{
			u8Ctrl = I2C_CTL_SI;                                /* Clear SI */
		}
	break;
	case 0x58u:	//Master receive NACK
		if(psParam->u32DataLen)
		{
			psParam->pu8Data[psPriv->u32DataTrans] = (unsigned char) I2C_GET_DATA(i2c);
			psPriv->u32DataTrans ++;
			psParam->u32DataLen --;
		}

		u8Ctrl = I2C_CTL_STO_SI;                                /* Clear SI and send STOP */
		psPriv->u8EndFlag = 1;
	break;
	case 0x38u:                                             /* Arbitration Lost */
	default:                                                /* Unknow status */
		u8Ctrl = I2C_CTL_STO_SI;                                /* Clear SI and send STOP */
		psPriv->u8EndFlag = 1;
		printf("I2C transfer arbitration lost or unknow status \n");
	break;	
	}
	I2C_SET_CONTROL_REG(i2c, u8Ctrl);                          /* Write controlbit to I2C_CTL register */
	psPriv->u8State = u32Status;

}

static void I2C_SlaveRxTx_IRQ(I2C_T *i2c, uint32_t u32Status, I2C_TRANS_PARAM *psParam, I2C_TRANS_PRIV *psPriv)
{
	uint8_t u8Ctrl = 0u;

	if(psPriv == NULL)
	{
		u8Ctrl = I2C_CTL_STO_SI;
        I2C_SET_CONTROL_REG(i2c, u8Ctrl);                          /* Write controlbit to I2C_CTL register */		
		return;
	}

//	printf("I2C slave status %x\n", u32Status);
	switch(u32Status)
	{
	case 0xA0u:	//Slave transmit repeat start or stop
		u8Ctrl = I2C_CTL_SI_AA;                                /* Clear SI and set ACK*/

		if((psParam) && (psPriv->u32DataTrans))
			psPriv->u8EndFlag = 1;
		printf("I2C slave transmit repeat start or stop\n");
	break;
	case 0xA8u:	//Slave transmit address ACK
		u8Ctrl = I2C_CTL_SI_AA;                               /* Clear SI and set  ACK*/

		if(psParam == NULL)
		{
			u8Ctrl = I2C_CTL_SI_AA;
		}
		else if(psParam->u32DataLen)
		{
			if(psPriv->u8TransRx == 0)		//TX access
			{
				I2C_SET_DATA(i2c, psParam->pu8Data[psPriv->u32DataTrans]);                          /* Write data to I2CDAT */

				psParam->u32DataLen --;
				psPriv->u32DataTrans ++;
			}
			else 
			{
				psPriv->u8EndFlag = 1;
				u8Ctrl = I2C_CTL_SI;
			}
		}
		else
		{
			psPriv->u8EndFlag = 1;
			u8Ctrl = I2C_CTL_STO_SI;
		}	
	break;
	case 0xB8u:	//Slave transmit data ACK
		u8Ctrl = I2C_CTL_SI_AA;                               /* Clear SI */

		if(psParam == NULL)
		{
			u8Ctrl = I2C_CTL_SI;
		}
		else if(psParam->u32DataLen)
		{
			if(psPriv->u8TransRx == 0)		//TX access
			{
				I2C_SET_DATA(i2c, psParam->pu8Data[psPriv->u32DataTrans]);                          /* Write data to I2CDAT */

				psParam->u32DataLen --;
				psPriv->u32DataTrans ++;
				
				if(psParam->u32DataLen == 0)
				{
					u8Ctrl = I2C_CTL_SI;
				}
			}
			else 		//RX access
			{
				u8Ctrl = I2C_CTL_STA_SI;
			}
		}
		else
		{
			psPriv->u8EndFlag = 1;
			u8Ctrl = I2C_CTL_SI;
		}

	break;
	case 0xC0u:	//Slave transmit data NACK
		if(psPriv->u32DataTrans)
			psPriv->u8EndFlag = 1;

		u8Ctrl = I2C_CTL_SI_AA;
	break;
	case 0xC8u:	//Slave transmit last data ACK
		psPriv->u8EndFlag = 1;
		u8Ctrl = I2C_CTL_SI_AA;
	break;
	case 0x60u:	//Slave receive address ACK
		u8Ctrl = I2C_CTL_SI_AA;
	break;
	case 0x80u:	//Slave receive data ACK
		if(psParam == NULL)
		{
			u8Ctrl = I2C_CTL_SI;
		}
		else
		{
			if(psParam->u32DataLen)
			{
				psParam->pu8Data[psPriv->u32DataTrans] = (unsigned char) I2C_GET_DATA(i2c);
				psPriv->u32DataTrans ++;
				psParam->u32DataLen --;
			}

			if(psParam->u32DataLen)
			{
				u8Ctrl = I2C_CTL_SI_AA;                             /* Clear SI and set ACK */
			}
			else
			{
				u8Ctrl = I2C_CTL_SI;                                /* Clear SI */
			}
		}
	break;
	case 0x88u:	//Slave receive data NACK
		if(psParam == NULL)
		{
			u8Ctrl = I2C_CTL_SI;
		}
		else
		{
			if(psParam->u32DataLen)
			{
				psParam->pu8Data[psPriv->u32DataTrans] = (unsigned char) I2C_GET_DATA(i2c);
				psPriv->u32DataTrans ++;
				psParam->u32DataLen --;
			}

			u8Ctrl = I2C_CTL_SI_AA;                                /* Clear SI and set AA*/
			psPriv->u8EndFlag = 1;
		}
	break;
	case 0xB0u:	//Address transmit aribitration lost
	{
		u8Ctrl = I2C_CTL_SI;                                /* Clear SI */
		printf("I2C slave transfer arbitration lost\n");
	}
	break;
	case 0x68u:	//Address receive aribitration lost
		u8Ctrl = I2C_CTL_SI;                                /* Clear SI */
		printf("I2C slave transfer arbitration lost\n");
	break;
	default:                                                /* Unknow status */
		u8Ctrl = I2C_CTL_SI;                                /* Clear SI and send STOP */
		psPriv->u8EndFlag = 1;
		printf("I2C slave transfer unknow status \n");
	break;	
	}
	I2C_SET_CONTROL_REG(i2c, u8Ctrl);                          /* Write controlbit to I2C_CTL register */
	psPriv->u8State = u32Status;
}

void Handle_I2C_Irq(I2C_T *i2c, uint32_t u32Status) {
	I2C_TRANS_HANDLER *psTransHandler;
	
	if(i2c == I2C0)
	{
		u32Status = I2C_GET_STATUS(I2C0);
		psTransHandler = &s_asI2CTransHandler[0];
	}
	else if(i2c == I2C1)
	{
		u32Status = I2C_GET_STATUS(I2C1);
		psTransHandler = &s_asI2CTransHandler[1];
	}
	else if(i2c == I2C2)
	{
		u32Status = I2C_GET_STATUS(I2C2);
		psTransHandler = &s_asI2CTransHandler[2];
	}
	else
	{
		return;
	}

	if(psTransHandler->pfnTransIRQ)
	{
		psTransHandler->pfnTransIRQ(i2c, u32Status, psTransHandler->psTransParam, &psTransHandler->sTransPriv);
	}
}

int32_t I2C_HookIRQHandler(
	I2C_T *i2c,
	uint8_t u8Recv,
	uint8_t u8Master
)
{
	I2C_TRANS_HANDLER *psTransHandler = NULL;
	I2C_TRANS_PRIV *psTransPriv;
	
	if(i2c == I2C0)
	{
		psTransHandler = &s_asI2CTransHandler[0];
	}
	else if(i2c == I2C1)
	{
		psTransHandler = &s_asI2CTransHandler[1];
	}
	else if(i2c == I2C2)
	{
		psTransHandler = &s_asI2CTransHandler[2];
	}
		
	if(psTransHandler == NULL)
		return -1;

	psTransPriv = &psTransHandler->sTransPriv;

	if(u8Master){
		psTransHandler->pfnTransIRQ = I2C_MasterRxTx_IRQ;
	}
	else{
		psTransHandler->pfnTransIRQ = I2C_SlaveRxTx_IRQ;
		/* I2C enter no address SLV mode */
		I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI | I2C_CTL_AA);
	}

	I2C_EnableInt(i2c);

	psTransHandler->bHookIRQ = 1;
	psTransPriv->u8TransRx = u8Recv;

	return 0;
}

int I2C_MaterSendRecv(
	I2C_T *i2c,
	uint8_t u8Recv,
	I2C_TRANS_PARAM *psParam,
	uint32_t u32TimeOut
)
{
	I2C_TRANS_HANDLER *psTransHandler = NULL;
	I2C_TRANS_PRIV *psTransPriv;
	
	if(i2c == I2C0)
	{
		psTransHandler = &s_asI2CTransHandler[0];
	}
	else if(i2c == I2C1)
	{
		psTransHandler = &s_asI2CTransHandler[1];
	}
	else if(i2c == I2C2)
	{
		psTransHandler = &s_asI2CTransHandler[2];
	}
		
	if((psParam == NULL) || (psTransHandler == NULL))
		return -1;

	psTransPriv = &psTransHandler->sTransPriv;
	if(psTransPriv->u8InTrans)
		return -2;
	
	memset(psTransPriv, 0x0, sizeof(I2C_TRANS_PRIV));
	psTransPriv = &psTransHandler->sTransPriv;
	psTransPriv->u8InTrans = 1;
	psTransPriv->u8TransRx = u8Recv;
	psTransHandler->psTransParam = psParam;

	psTransHandler->pfnTransIRQ = I2C_MasterRxTx_IRQ;
 
	if(psTransHandler->bHookIRQ == 0)
		I2C_EnableInt(i2c);

	I2C_START(i2c);

	uint32_t u32EndTick = mp_hal_ticks_ms() + u32TimeOut;

	while(1){

		if(mp_hal_ticks_ms() > u32EndTick){
			break;
		}

		if(psTransPriv->u8EndFlag)
		{
//			printf("I2C master end state %x \n", psTransPriv->u8State);
			break;
		}
	}

	if(psTransHandler->bHookIRQ == 0)
		I2C_DisableInt(i2c);

	psTransHandler->psTransParam = NULL;
	psTransPriv->u8InTrans = 0;
	
	return psTransPriv->u32DataTrans;
}


int I2C_SlaveSendRecv(
	I2C_T *i2c,
	uint8_t u8Recv,
	I2C_TRANS_PARAM *psParam,
	uint32_t u32TimeOut
)
{
	I2C_TRANS_HANDLER *psTransHandler = NULL;
	I2C_TRANS_PRIV *psTransPriv;
	
	if(i2c == I2C0)
	{
		psTransHandler = &s_asI2CTransHandler[0];
	}
	else if(i2c == I2C1)
	{
		psTransHandler = &s_asI2CTransHandler[1];
	}
	else if(i2c == I2C2)
	{
		psTransHandler = &s_asI2CTransHandler[2];
	}
		
	if((psParam == NULL) || (psTransHandler == NULL))
		return -1;

	psTransPriv = &psTransHandler->sTransPriv;
	if(psTransPriv->u8InTrans)
		return -2;

	memset(psTransPriv, 0x0, sizeof(I2C_TRANS_PRIV));
	psTransPriv->u8InTrans = 1;
	psTransPriv->u8TransRx = u8Recv;
	psTransHandler->psTransParam = psParam;

	/* I2C enter no address SLV mode */
	I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI | I2C_CTL_AA);

	psTransHandler->pfnTransIRQ = I2C_SlaveRxTx_IRQ;

	if(psTransHandler->bHookIRQ == 0)
		I2C_EnableInt(i2c);

	uint32_t u32EndTick = mp_hal_ticks_ms() + u32TimeOut;

	while(1){

		if(mp_hal_ticks_ms() > u32EndTick){
			if((psTransPriv->u8State == 0) || (psTransPriv->u8State == 0xA0))
				break;
		}

		if(psTransPriv->u8EndFlag)
		{
//			printf("I2C slave end state %x \n", psTransPriv->u8State);
			break;
		}
	}

	if(psTransHandler->bHookIRQ == 0)
		I2C_DisableInt(i2c);

	psTransHandler->psTransParam = NULL;
	psTransPriv->u8InTrans = 0;
	
	return psTransPriv->u32DataTrans;
}

