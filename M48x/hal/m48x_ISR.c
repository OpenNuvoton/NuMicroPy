
/**************************************************************************//**
 * @file     m48x_ISR.c
 * @version  V0.10
 * @brief   M480 Main interrupt server routines
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>

#include "NuMicro.h"

#include "mods/pybpin.h"
#include "mods/pybirq.h"
#include "mods/pybtimer.h"
#include "mods/pybcan.h"
#include "mods/pybpwm.h"
#include "hal/pin_int.h"
#include "hal/M48x_I2C.h"
#include "hal/M48x_SPI.h"
#include "hal/M48x_USBD.h"
#include "hal/M48x_UART.h"
#include "hal/M48x_I2S.h"
#include "hal/dma.h"

#if MICROPY_LVGL
#include "lvgl/lvgl.h"
#endif

/**
 * @brief       GPIO PA IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PA default IRQ, declared in startup_M480.s.
 */
void GPA_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPA_IRQn);

	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PA, 1 << i)){
			GPIO_CLR_INT_FLAG(PA, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	
	IRQ_EXIT(GPA_IRQn);
}


/**
 * @brief       GPIO PB IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PB default IRQ, declared in startup_M480.s.
 */
void GPB_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPB_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PB, 1 << i)){
			GPIO_CLR_INT_FLAG(PB, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPB_IRQn);
}

/**
 * @brief       GPIO PC IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PC default IRQ, declared in startup_M480.s.
 */
void GPC_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPC_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PC, 1 << i)){
			GPIO_CLR_INT_FLAG(PC, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPC_IRQn);
}

/**
 * @brief       GPIO PD IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PD default IRQ, declared in startup_M480.s.
 */
void GPD_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPD_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PD, 1 << i)){
			GPIO_CLR_INT_FLAG(PD, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPD_IRQn);
}

/**
 * @brief       GPIO PE IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PE default IRQ, declared in startup_M480.s.
 */
void GPE_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPE_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PE, 1 << i)){
			GPIO_CLR_INT_FLAG(PE, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPE_IRQn);
}

/**
 * @brief       GPIO PF IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PF default IRQ, declared in startup_M480.s.
 */
void GPF_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPF_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PF, 1 << i)){
			GPIO_CLR_INT_FLAG(PF, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPF_IRQn);
}

/**
 * @brief       GPIO PG IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PG default IRQ, declared in startup_M480.s.
 */
void GPG_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPG_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PG, 1 << i)){
			GPIO_CLR_INT_FLAG(PG, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPG_IRQn);
}

/**
 * @brief       GPIO PH IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PH default IRQ, declared in startup_M480.s.
 */
void GPH_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPH_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PH, 1 << i)){
			GPIO_CLR_INT_FLAG(PH, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPH_IRQn);
}


#if !MICROPY_PY_THREAD

__IO uint32_t uwTick;

void SysTick_Handler(void) {
    // Instead of calling HAL_IncTick we do the increment here of the counter.
    // This is purely for efficiency, since SysTick is called 1000 times per
    // second at the highest interrupt priority.
    // Note: we don't need uwTick to be declared volatile here because this is
    // the only place where it can be modified, and the code is more efficient
    // without the volatile specifier.
    uwTick += 1;

#if MICROPY_LVGL
	lv_tick_inc(1);
#endif

    // Read the systick control regster. This has the side effect of clearing
    // the COUNTFLAG bit, which makes the logic in mp_hal_ticks_us
    // work properly.
    SysTick->CTRL;
}

#endif

#if MICROPY_HW_ENABLE_HW_I2C

void I2C0_IRQHandler(void)
{
    uint32_t u32Status;

	IRQ_ENTER(I2C0_IRQn);
    u32Status = I2C_GET_STATUS(I2C0);

    if (I2C_GET_TIMEOUT_FLAG(I2C0))
    {
        /* Clear I2C0 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C0);
    }
    else
    {
//		printf("I2C status %d \n", u32Status);
		Handle_I2C_Irq(I2C0, u32Status);
    }
	IRQ_EXIT(I2C0_IRQn);
}

void I2C1_IRQHandler(void)
{
    uint32_t u32Status;

	IRQ_ENTER(I2C1_IRQn);

    u32Status = I2C_GET_STATUS(I2C1);

    if (I2C_GET_TIMEOUT_FLAG(I2C1))
    {
        /* Clear I2C0 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C1);
    }
    else
    {
		Handle_I2C_Irq(I2C1, u32Status);
    }
	IRQ_EXIT(I2C1_IRQn);
}

void I2C2_IRQHandler(void)
{
    uint32_t u32Status;

	IRQ_ENTER(I2C2_IRQn);

    u32Status = I2C_GET_STATUS(I2C2);

    if (I2C_GET_TIMEOUT_FLAG(I2C2))
    {
        /* Clear I2C0 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C2);
    }
    else
    {
		Handle_I2C_Irq(I2C2, u32Status);
    }
	IRQ_EXIT(I2C2_IRQn);
}

#endif

void PDMA_IRQHandler(void)
{
	IRQ_ENTER(PDMA_IRQn);
	Handle_PDMA_Irq();
	IRQ_EXIT(PDMA_IRQn);
}

void SPI0_IRQHandler(void)
{
	IRQ_ENTER(SPI0_IRQn);
	Handle_SPI_Irq(SPI0);
	IRQ_EXIT(SPI0_IRQn);
}

void SPI1_IRQHandler(void)
{
	IRQ_ENTER(SPI1_IRQn);
	Handle_SPI_Irq(SPI1);
	IRQ_EXIT(SPI1_IRQn);
}

void SPI2_IRQHandler(void)
{
	IRQ_ENTER(SPI2_IRQn);
	Handle_SPI_Irq(SPI2);
	IRQ_EXIT(SPI2_IRQn);
}

void SPI3_IRQHandler(void)
{
	IRQ_ENTER(SPI3_IRQn);
	Handle_SPI_Irq(SPI3);
	IRQ_EXIT(SPI3_IRQn);
}


void TMR0_IRQHandler(void)
{
	uint32_t u32Status;
	IRQ_ENTER(TMR0_IRQn);

	u32Status = TIMER_GetIntFlag(TIMER0);
	if(u32Status){
		TIMER_ClearIntFlag(TIMER0);
		Handle_TMR_Irq(0, eTMR_INT_INT, u32Status);
	}

	u32Status = TIMER_GetCaptureIntFlag(TIMER0);
	if(u32Status){
		TIMER_ClearCaptureIntFlag(TIMER0);
		Handle_TMR_Irq(0, eTMR_INT_EINT, u32Status);
	}

	u32Status = TIMER_GetPWMIntFlag(TIMER0);
	if(u32Status){
		TIMER_ClearPWMIntFlag(TIMER0);
		Handle_TMR_Irq(0, eTMR_INT_PWMINT, u32Status);
	}

	IRQ_EXIT(TMR0_IRQn);
}

void TMR1_IRQHandler(void)
{
	uint32_t u32Status;

	IRQ_ENTER(TMR1_IRQn);

	u32Status = TIMER_GetIntFlag(TIMER1);
	if(u32Status){
		TIMER_ClearIntFlag(TIMER1);
		Handle_TMR_Irq(1, eTMR_INT_INT, u32Status);
	}

	u32Status = TIMER_GetCaptureIntFlag(TIMER1);
	if(u32Status){
		TIMER_ClearCaptureIntFlag(TIMER1);
		Handle_TMR_Irq(1, eTMR_INT_EINT, u32Status);
	}

	u32Status = TIMER_GetPWMIntFlag(TIMER1);
	if(u32Status){
		TIMER_ClearPWMIntFlag(TIMER1);
		Handle_TMR_Irq(1, eTMR_INT_PWMINT, u32Status);
	}

	IRQ_EXIT(TMR1_IRQn);
}

void TMR2_IRQHandler(void)
{
	uint32_t u32Status;

	IRQ_ENTER(TMR2_IRQn);

	u32Status = TIMER_GetIntFlag(TIMER2);
	if(u32Status){
		TIMER_ClearIntFlag(TIMER2);
		Handle_TMR_Irq(2, eTMR_INT_INT, u32Status);
	}

	u32Status = TIMER_GetCaptureIntFlag(TIMER2);
	if(u32Status){
		TIMER_ClearCaptureIntFlag(TIMER2);
		Handle_TMR_Irq(2, eTMR_INT_EINT, u32Status);
	}

	u32Status = TIMER_GetPWMIntFlag(TIMER2);
	if(u32Status){
		TIMER_ClearPWMIntFlag(TIMER2);
		Handle_TMR_Irq(2, eTMR_INT_PWMINT, u32Status);
	}

	IRQ_EXIT(TMR2_IRQn);
}

void TMR3_IRQHandler(void)
{
	uint32_t u32Status;

	IRQ_ENTER(TMR3_IRQn);

	u32Status = TIMER_GetIntFlag(TIMER3);
	if(u32Status){
		TIMER_ClearIntFlag(TIMER3);
		Handle_TMR_Irq(3, eTMR_INT_INT, u32Status);
	}

	u32Status = TIMER_GetCaptureIntFlag(TIMER3);
	if(u32Status){
		TIMER_ClearCaptureIntFlag(TIMER3);
		Handle_TMR_Irq(3, eTMR_INT_EINT, u32Status);
	}

	u32Status = TIMER_GetPWMIntFlag(TIMER3);
	if(u32Status){
		TIMER_ClearPWMIntFlag(TIMER3);
		Handle_TMR_Irq(3, eTMR_INT_PWMINT, u32Status);
	}

	IRQ_EXIT(TMR3_IRQn);
}


/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle UART Channel interrupt event                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void UART0_IRQHandler(void)
{
	IRQ_ENTER(UART0_IRQn);
	Handle_UART_Irq(UART0);
	IRQ_EXIT(UART0_IRQn);
}

void UART1_IRQHandler(void)
{
	IRQ_ENTER(UART1_IRQn);
	Handle_UART_Irq(UART1);
	IRQ_EXIT(UART1_IRQn);

}

void UART2_IRQHandler(void)
{
	IRQ_ENTER(UART2_IRQn);
	Handle_UART_Irq(UART2);
	IRQ_EXIT(UART2_IRQn);
}

void UART3_IRQHandler(void)
{
	IRQ_ENTER(UART3_IRQn);
	Handle_UART_Irq(UART3);
	IRQ_EXIT(UART3_IRQn);
}

void UART4_IRQHandler(void)
{
	IRQ_ENTER(UART4_IRQn);
	Handle_UART_Irq(UART4);
	IRQ_EXIT(UART4_IRQn);

}

void UART5_IRQHandler(void)
{
	IRQ_ENTER(UART5_IRQn);
	Handle_UART_Irq(UART5);
	IRQ_EXIT(UART5_IRQn);
}

/**
  * @brief  CAN0_IRQ Handler.
  * @param  None.
  * @return None.
  */
void CAN0_IRQHandler(void)
{
    uint32_t u32IIDRStatus;

	IRQ_ENTER(CAN0_IRQn);
    u32IIDRStatus = CAN0->IIDR;


	if(u32IIDRStatus){
		Handle_CAN_Irq(0, u32IIDRStatus);

		if(u32IIDRStatus <= 0x20)
			CAN_CLR_INT_PENDING_BIT(CAN0, ((CAN0->IIDR) -1));
	}


	IRQ_EXIT(CAN0_IRQn);

}

/**
  * @brief  CAN1_IRQ Handler.
  * @param  None.
  * @return None.
  */
void CAN1_IRQHandler(void)
{
    uint32_t u32IIDRStatus;

	IRQ_ENTER(CAN1_IRQn);
    u32IIDRStatus = CAN1->IIDR;

	if(u32IIDRStatus){
		Handle_CAN_Irq(1, u32IIDRStatus);

		if(u32IIDRStatus <= 0x20)
			CAN_CLR_INT_PENDING_BIT(CAN1, ((CAN1->IIDR) -1));

	}

	IRQ_EXIT(CAN1_IRQn);
}

void USBD_IRQHandler(void)
{

    uint32_t u32IntStatus = USBD_GET_INT_FLAG();
    uint32_t u32BusState = USBD_GET_BUS_STATE();

	IRQ_ENTER(USBD_IRQn);
	Handle_USBDEV_Irq(u32IntStatus, u32BusState);
	IRQ_EXIT(USBD_IRQn);
}

volatile uint32_t g_ECC_done, g_ECCERR_done;
volatile int  g_Crypto_Int_done = 0;

void CRYPTO_IRQHandler()
{
	IRQ_ENTER(CRPT_IRQn);

    if (TDES_GET_INT_FLAG(CRPT))
    {
        g_Crypto_Int_done = 1;
        TDES_CLR_INT_FLAG(CRPT);
    }

    if (AES_GET_INT_FLAG(CRPT))
    {
        g_Crypto_Int_done = 1;
        AES_CLR_INT_FLAG(CRPT);
    }

    if (SHA_GET_INT_FLAG(CRPT))
    {
        g_Crypto_Int_done = 1;
        SHA_CLR_INT_FLAG(CRPT);
    }

	ECC_Complete(CRPT);
	
	IRQ_EXIT(CRPT_IRQn);
}

void BPWM0_IRQHandler(void)
{

	IRQ_ENTER(BPWM0_IRQn);
	Handle_PWM_Irq(0);
	IRQ_EXIT(BPWM0_IRQn);
}

void BPWM1_IRQHandler(void)
{

	IRQ_ENTER(BPWM1_IRQn);
	Handle_PWM_Irq(1);
	IRQ_EXIT(BPWM1_IRQn);
}

void EPWM0P0_IRQHandler(void)
{
	IRQ_ENTER(EPWM0P0_IRQn);
	Handle_EPWM_Irq(0, 0);
	IRQ_EXIT(EPWM0P0_IRQn);
}

void EPWM0P1_IRQHandler(void)
{
	IRQ_ENTER(EPWM0P1_IRQn);
	Handle_EPWM_Irq(0, 1);
	IRQ_EXIT(EPWM0P1_IRQn);
}

void EPWM0P2_IRQHandler(void)
{
	IRQ_ENTER(EPWM0P2_IRQn);
	Handle_EPWM_Irq(0, 2);
	IRQ_EXIT(EPWM0P2_IRQn);
}

void EPWM1P0_IRQHandler(void)
{
	IRQ_ENTER(EPWM1P0_IRQn);
	Handle_EPWM_Irq(1, 0);
	IRQ_EXIT(EPWM1P0_IRQn);
}

void EPWM1P1_IRQHandler(void)
{
	IRQ_ENTER(EPWM1P1_IRQn);
	Handle_EPWM_Irq(1, 1);
	IRQ_EXIT(EPWM1P1_IRQn);
}

void EPWM1P2_IRQHandler(void)
{
	IRQ_ENTER(EPWM1P2_IRQn);
	Handle_EPWM_Irq(1, 2);
	IRQ_EXIT(EPWM1P2_IRQn);
}

void I2S0_IRQHandler(void)
{
	IRQ_ENTER(I2S0_IRQn);
	Handle_I2S_Irq(I2S0);
	IRQ_EXIT(I2S0_IRQn);
}


