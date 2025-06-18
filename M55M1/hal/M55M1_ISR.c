/**************************************************************************//**
 * @file     M46x_ISR.c
 * @version  V0.10
 * @brief   M460 Main interrupt server routines
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>

#include "NuMicro.h"

#include "mods/classPin.h"
#include "hal/M55M1_IRQ.h"
#include "hal/M55M1_USBD.h"
#include "hal/M55M1_UART.h"
#include "hal/M55M1_I2C.h"
#include "hal/M55M1_SPI.h"
#include "hal/M55M1_CANFD.h"
#include "hal/M55M1_PWM.h"
#include "hal/M55M1_Timer.h"
#include "hal/M55M1_RTC.h"
#include "hal/drv_pdma.h"
#include "hal/pin_int.h"
#include "hal/StorIF_SDCard.h"

#if IRQ_ENABLE_STATS
uint32_t irq_stats[TOTAL_IRQn_CNT + 1] = {0};
#endif

#if !MICROPY_PY_THREAD

__IO uint32_t uwTick;

void SysTick_Handler(void)
{
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

void USBD_IRQHandler(void)
{

    uint32_t u32IntStatus = USBD_GET_INT_FLAG();
    uint32_t u32BusState = USBD_GET_BUS_STATE();

    IRQ_ENTER(USBD_IRQn);
    Handle_USBDEV_Irq(u32IntStatus, u32BusState);
    IRQ_EXIT(USBD_IRQn);
}

void PDMA0_IRQHandler(void)
{
    IRQ_ENTER(PDMA0_IRQn);
    Handle_PDMA_Irq(PDMA0);
    IRQ_EXIT(PDMA0_IRQn);
}

void PDMA1_IRQHandler(void)
{
    IRQ_ENTER(PDMA1_IRQn);
    Handle_PDMA_Irq(PDMA1);
    IRQ_EXIT(PDMA1_IRQn);
}

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

void UART6_IRQHandler(void)
{
    IRQ_ENTER(UART6_IRQn);
    Handle_UART_Irq(UART6);
    IRQ_EXIT(UART6_IRQn);
}

void UART7_IRQHandler(void)
{
    IRQ_ENTER(UART7_IRQn);
    Handle_UART_Irq(UART7);
    IRQ_EXIT(UART7_IRQn);
}

void UART8_IRQHandler(void)
{
    IRQ_ENTER(UART8_IRQn);
    Handle_UART_Irq(UART8);
    IRQ_EXIT(UART8_IRQn);

}

void UART9_IRQHandler(void)
{
    IRQ_ENTER(UART9_IRQn);
    Handle_UART_Irq(UART9);
    IRQ_EXIT(UART9_IRQn);
}

void LPUART0_IRQHandler(void)
{
    IRQ_ENTER(LPUART0_IRQn);
    Handle_LPUART_Irq(LPUART0);
    IRQ_EXIT(LPUART0_IRQn);
}

void GPA_IRQHandler(void)
{
    int i;

    IRQ_ENTER(GPA_IRQn);

    for(i = 0; i < 16; i ++) {
        if(GPIO_GET_INT_FLAG(PA, 1 << i)) {
            GPIO_CLR_INT_FLAG(PA, 1 << i);
            //Call gpio irq handler
            Handle_GPIO_Irq(i);
        }
    }

    IRQ_EXIT(GPA_IRQn);
}

void GPB_IRQHandler(void)
{
    int i;

    IRQ_ENTER(GPB_IRQn);
    for(i = 0; i < 16; i ++) {
        if(GPIO_GET_INT_FLAG(PB, 1 << i)) {
            GPIO_CLR_INT_FLAG(PB, 1 << i);
            //Call gpio irq handler
            Handle_GPIO_Irq(i);
        }
    }
    IRQ_EXIT(GPB_IRQn);
}

void GPC_IRQHandler(void)
{
    int i;

    IRQ_ENTER(GPC_IRQn);
    for(i = 0; i < 16; i ++) {
        if(GPIO_GET_INT_FLAG(PC, 1 << i)) {
            GPIO_CLR_INT_FLAG(PC, 1 << i);
            //Call gpio irq handler
            Handle_GPIO_Irq(i);
        }
    }
    IRQ_EXIT(GPC_IRQn);
}

void GPD_IRQHandler(void)
{
    int i;

    IRQ_ENTER(GPD_IRQn);
    for(i = 0; i < 16; i ++) {
        if(GPIO_GET_INT_FLAG(PD, 1 << i)) {
            GPIO_CLR_INT_FLAG(PD, 1 << i);
            //Call gpio irq handler
            Handle_GPIO_Irq(i);
        }
    }
    IRQ_EXIT(GPD_IRQn);
}

void GPE_IRQHandler(void)
{
    int i;

    IRQ_ENTER(GPE_IRQn);
    for(i = 0; i < 16; i ++) {
        if(GPIO_GET_INT_FLAG(PE, 1 << i)) {
            GPIO_CLR_INT_FLAG(PE, 1 << i);
            //Call gpio irq handler
            Handle_GPIO_Irq(i);
        }
    }
    IRQ_EXIT(GPE_IRQn);
}

void GPF_IRQHandler(void)
{
    int i;

    IRQ_ENTER(GPF_IRQn);
    for(i = 0; i < 16; i ++) {
        if(GPIO_GET_INT_FLAG(PF, 1 << i)) {
            GPIO_CLR_INT_FLAG(PF, 1 << i);
            //Call gpio irq handler
            Handle_GPIO_Irq(i);
        }
    }
    IRQ_EXIT(GPF_IRQn);
}

void GPG_IRQHandler(void)
{
    int i;

    IRQ_ENTER(GPG_IRQn);
    for(i = 0; i < 16; i ++) {
        if(GPIO_GET_INT_FLAG(PG, 1 << i)) {
            GPIO_CLR_INT_FLAG(PG, 1 << i);
            //Call gpio irq handler
            Handle_GPIO_Irq(i);
        }
    }
    IRQ_EXIT(GPG_IRQn);
}

void GPH_IRQHandler(void)
{
    int i;

    IRQ_ENTER(GPH_IRQn);
    for(i = 0; i < 16; i ++) {
        if(GPIO_GET_INT_FLAG(PH, 1 << i)) {
            GPIO_CLR_INT_FLAG(PH, 1 << i);
            //Call gpio irq handler
            Handle_GPIO_Irq(i);
        }
    }
    IRQ_EXIT(GPH_IRQn);
}

void GPI_IRQHandler(void)
{
    int i;

    IRQ_ENTER(GPI_IRQn);
    for(i = 0; i < 16; i ++) {
        if(GPIO_GET_INT_FLAG(PI, 1 << i)) {
            GPIO_CLR_INT_FLAG(PI, 1 << i);
            //Call gpio irq handler
            Handle_GPIO_Irq(i);
        }
    }
    IRQ_EXIT(GPI_IRQn);
}

void GPJ_IRQHandler(void)
{
    int i;

    IRQ_ENTER(GPJ_IRQn);
    for(i = 0; i < 16; i ++) {
        if(GPIO_GET_INT_FLAG(PJ, 1 << i)) {
            GPIO_CLR_INT_FLAG(PJ, 1 << i);
            //Call gpio irq handler
            Handle_GPIO_Irq(i);
        }
    }
    IRQ_EXIT(GPJ_IRQn);
}

void I2C0_IRQHandler(void)
{
    uint32_t u32Status;

    IRQ_ENTER(I2C0_IRQn);
    u32Status = I2C_GET_STATUS(I2C0);

    if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
        /* Clear I2C0 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C0);
    } else {
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

    if (I2C_GET_TIMEOUT_FLAG(I2C1)) {
        /* Clear I2C1 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C1);
    } else {
        Handle_I2C_Irq(I2C1, u32Status);
    }
    IRQ_EXIT(I2C1_IRQn);
}

void I2C2_IRQHandler(void)
{
    uint32_t u32Status;

    IRQ_ENTER(I2C2_IRQn);

    u32Status = I2C_GET_STATUS(I2C2);

    if (I2C_GET_TIMEOUT_FLAG(I2C2)) {
        /* Clear I2C2 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C2);
    } else {
        Handle_I2C_Irq(I2C2, u32Status);
    }
    IRQ_EXIT(I2C2_IRQn);
}

void I2C3_IRQHandler(void)
{
    uint32_t u32Status;

    IRQ_ENTER(I2C3_IRQn);

    u32Status = I2C_GET_STATUS(I2C3);

    if (I2C_GET_TIMEOUT_FLAG(I2C3)) {
        /* Clear I2C3 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C3);
    }

    Handle_I2C_Irq(I2C3, u32Status);
    IRQ_EXIT(I2C3_IRQn);
}

void LPI2C0_IRQHandler(void)
{
    uint32_t u32Status;

    IRQ_ENTER(LPI2C0_IRQn);

    u32Status = LPI2C_GET_STATUS(LPI2C0);

    if (LPI2C_GET_TIMEOUT_FLAG(LPI2C0)) {
        /* Clear I2C0 Timeout Flag */
        LPI2C_ClearTimeoutFlag(LPI2C0);
    } else {
        Handle_LPI2C_Irq(LPI2C0, u32Status);
    }
    IRQ_EXIT(LPI2C0_IRQn);
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

void LPSPI0_IRQHandler(void)
{
    IRQ_ENTER(LPSPI0_IRQn);
    Handle_LPSPI_Irq(LPSPI0);
    IRQ_EXIT(LPSPI0_IRQn);
}

void CANFD00_IRQHandler(void)
{
    uint32_t u32IRStatus;

    IRQ_ENTER(CANFD00_IRQn);

    u32IRStatus = CANFD0->IR;

    if(u32IRStatus) {
        Handle_CANFD_Irq(CANFD0, u32IRStatus);
        /* Clear the Interrupt flag */
        CANFD_ClearStatusFlag(CANFD0, u32IRStatus);
    }

    IRQ_EXIT(CANFD00_IRQn);
}
void CANFD01_IRQHandler(void)
{
    uint32_t u32IRStatus;

    IRQ_ENTER(CANFD01_IRQn);

    u32IRStatus = CANFD0->IR;

    if(u32IRStatus) {
        Handle_CANFD_Irq(CANFD0, u32IRStatus);
        /* Clear the Interrupt flag */
        CANFD_ClearStatusFlag(CANFD0, u32IRStatus);
    }

    IRQ_EXIT(CANFD01_IRQn);
}
void CANFD10_IRQHandler(void)
{
    uint32_t u32IRStatus;

    IRQ_ENTER(CANFD10_IRQn);

    u32IRStatus = CANFD1->IR;

    if(u32IRStatus) {
        Handle_CANFD_Irq(CANFD1, u32IRStatus);
        /* Clear the Interrupt flag */
        CANFD_ClearStatusFlag(CANFD1, u32IRStatus);
    }

    IRQ_EXIT(CANFD10_IRQn);
}
void CANFD11_IRQHandler(void)
{
    uint32_t u32IRStatus;

    IRQ_ENTER(CANFD11_IRQn);

    u32IRStatus = CANFD1->IR;

    if(u32IRStatus) {
        Handle_CANFD_Irq(CANFD1, u32IRStatus);
        /* Clear the Interrupt flag */
        CANFD_ClearStatusFlag(CANFD1, u32IRStatus);
    }

    IRQ_EXIT(CANFD11_IRQn);
}

void BPWM0_IRQHandler(void)
{

    IRQ_ENTER(BPWM0_IRQn);
    Handle_BPWM_Irq(BPWM0);
    IRQ_EXIT(BPWM0_IRQn);
}

void BPWM1_IRQHandler(void)
{

    IRQ_ENTER(BPWM1_IRQn);
    Handle_BPWM_Irq(BPWM0);
    IRQ_EXIT(BPWM1_IRQn);
}

void EPWM0P0_IRQHandler(void)
{
    IRQ_ENTER(EPWM0P0_IRQn);
    Handle_EPWM_Irq(EPWM0, 0);
    IRQ_EXIT(EPWM0P0_IRQn);
}

void EPWM0P1_IRQHandler(void)
{
    IRQ_ENTER(EPWM0P1_IRQn);
    Handle_EPWM_Irq(EPWM0, 1);
    IRQ_EXIT(EPWM0P1_IRQn);
}

void EPWM0P2_IRQHandler(void)
{
    IRQ_ENTER(EPWM0P2_IRQn);
    Handle_EPWM_Irq(EPWM0, 2);
    IRQ_EXIT(EPWM0P2_IRQn);
}

void EPWM1P0_IRQHandler(void)
{
    IRQ_ENTER(EPWM1P0_IRQn);
    Handle_EPWM_Irq(EPWM1, 0);
    IRQ_EXIT(EPWM1P0_IRQn);
}

void EPWM1P1_IRQHandler(void)
{
    IRQ_ENTER(EPWM1P1_IRQn);
    Handle_EPWM_Irq(EPWM1, 1);
    IRQ_EXIT(EPWM1P1_IRQn);
}

void EPWM1P2_IRQHandler(void)
{
    IRQ_ENTER(EPWM1P2_IRQn);
    Handle_EPWM_Irq(EPWM1, 2);
    IRQ_EXIT(EPWM1P2_IRQn);
}

void TIMER0_IRQHandler(void)
{
    IRQ_ENTER(TIMER0_IRQn);
    Handle_Timer_Irq(TIMER0);
    IRQ_EXIT(TIMER0_IRQn);
}

void TIMER1_IRQHandler(void)
{
    IRQ_ENTER(TIMER1_IRQn);
    Handle_Timer_Irq(TIMER1);
    IRQ_EXIT(TIMER1_IRQn);

}

void TIMER2_IRQHandler(void)
{
    IRQ_ENTER(TIMER2_IRQn);
    Handle_Timer_Irq(TIMER2);
    IRQ_EXIT(TIMER2_IRQn);

}

void TIMER3_IRQHandler(void)
{
    IRQ_ENTER(TIMER3_IRQn);
    Handle_Timer_Irq(TIMER3);
    IRQ_EXIT(TIMER3_IRQn);

}

void LPTMR0_IRQHandler(void)
{
    IRQ_ENTER(LPTMR0_IRQn);
    Handle_LPTimer_Irq(LPTMR0);
    IRQ_EXIT(LPTMR0_IRQn);

}

void LPTMR1_IRQHandler(void)
{
    IRQ_ENTER(LPTMR1_IRQn);
    Handle_LPTimer_Irq(LPTMR1);
    IRQ_EXIT(LPTMR1_IRQn);
}

void RTC_IRQHandler(void)
{
    IRQ_ENTER(RTC_IRQn);
    Handle_RTC_Irq(RTC);
    IRQ_EXIT(RTC_IRQn);
}


//SD0/SD1 macro conflict with pin_defs_m55m1.h
#if defined(SD0)
#undef SD0
#endif

#if defined(SD1)
#undef SD1
#endif

void SDH0_IRQHandler(void)
{
    IRQ_ENTER(SDH0_IRQn);
    SDH_IntHandler(SDH0, &SD0);
    IRQ_EXIT(SDH0_IRQn);
}

void SDH1_IRQHandler(void)
{
    IRQ_ENTER(SDH1_IRQn);
    SDH_IntHandler(SDH1, &SD1);
    IRQ_EXIT(SDH1_IRQn);
}
