/**************************************************************************//**
 * @file     M5531_Timer.h
 * @version  V1.00
 * @brief    M5531 Timer HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M5531_TIMER_H__
#define __M5531_TIMER_H__

typedef void (*PFN_INT_STATUS_HANDLER)(void *obj, uint32_t u32Status);

typedef struct {
    union {
        TIMER_T *timer;
        LPTMR_T *lptimer;
    } u_timer;
    bool bLPTimer;
    PFN_INT_STATUS_HANDLER pfnStatusHandler;
} hw_timer_t;

typedef struct {
    uint32_t u32Mode;		//TTMR_ONESHOT_MODE, TTMR_PERIODIC_MODE, TTMR_CONTINUOUS_MODE
    uint32_t u32Prescaler;	//Prescale
    uint32_t u32Period;		//Period
} Timer_InitTypeDef;

int32_t Timer_Init(
    hw_timer_t *psObj,
    Timer_InitTypeDef *psInitDef
);

void Timer_Final(
    hw_timer_t *psObj
);

int32_t Timer_Trigger(
    hw_timer_t *psObj
);

int32_t Timer_Suspend(
    hw_timer_t *psObj
);

int32_t Timer_EnableInt(
    hw_timer_t *psObj,
    PFN_INT_STATUS_HANDLER pfnHandler
);

int32_t Timer_DisableInt(
    hw_timer_t *psObj
);

bool Timer_IsActivate(
    hw_timer_t *psObj
);

int32_t Timer_EnablePWMPeriodInt(
    hw_timer_t *psObj
);

int32_t Timer_DisablePWMPeriodInt(
    hw_timer_t *psObj
);

int32_t Timer_EnableCaptureInt(
    hw_timer_t *psObj
);

//u32Mode:	TIMER_CAPTURE_FREE_COUNTING_MODE
//			TIMER_CAPTURE_COUNTER_RESET_MODE
//u32Edge:	TIMER_CAPTURE_EVENT_FALLING
//			TIMER_CAPTURE_EVENT_RISING
//			TIMER_CAPTURE_EVENT_FALLING_RISING
//			TIMER_CAPTURE_EVENT_RISING_FALLING
//			TIMER_CAPTURE_EVENT_GET_LOW_PERIOD
//			TIMER_CAPTURE_EVENT_GET_HIGH_PERIOD

int32_t Timer_EnableCapture(
    hw_timer_t *psObj,
    uint32_t u32Mode,
    uint32_t u32Edge
);

int32_t Timer_DisableCaptureInt(
    hw_timer_t *psObj
);

int32_t Timer_SetPWMMode(
    hw_timer_t *psObj,
    uint32_t u32Freq,
    uint32_t u32Duty
);

int32_t Timer_PWMTrigger(
    hw_timer_t *psObj
);

int32_t Timer_PWMSuspend(
    hw_timer_t *psObj
);

int32_t Timer_UpdatePrescaler(
    hw_timer_t *psObj,
    uint32_t u32Prescaler
);

int32_t Timer_UpdatePeriod(
    hw_timer_t *psObj,
    uint32_t u32Period
);

uint32_t Timer_GetModuleClock(
    hw_timer_t *psObj
);

int32_t Timer_SetPWMMode(
    hw_timer_t *psObj,
    uint32_t u32Freq,
    uint32_t u32Duty
);

int32_t Timer_EnablePWMOutputChannel(
    hw_timer_t *psObj,
    uint32_t u32Channel		//TPWM_CH0/TPWM_CH1
);

uint32_t Timer_GetCounter(
    hw_timer_t *psObj
);

uint32_t Timer_GetPWMPeriod(
    hw_timer_t *psObj
);

int32_t Timer_UpdatePWMPeriod(
    hw_timer_t *psObj,
    uint32_t u32Period
);

uint32_t Timer_GetCaptureData(
    hw_timer_t *psObj
);

uint32_t Timer_GetCompareData(
    hw_timer_t *psObj
);

uint32_t Timer_GetPWMCompareData(
    hw_timer_t *psObj
);

void Handle_Timer_Irq(TIMER_T *timer);
void Handle_LPTimer_Irq(LPTMR_T *lptimer);


#endif
