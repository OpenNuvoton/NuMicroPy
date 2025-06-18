/**************************************************************************//**
 * @file     M55M1_Timer.c
 * @version  V0.01
 * @brief    M55M1 series Timer HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"

#include "M55M1_Timer.h"
#include "nu_modutil.h"

struct nu_timer_var {
    hw_timer_t *  psObj;
};

static struct nu_timer_var timer0_var = {
    .psObj                =   NULL,
};

static struct nu_timer_var timer1_var = {
    .psObj                =   NULL,
};

static struct nu_timer_var timer2_var = {
    .psObj                =   NULL,
};

static struct nu_timer_var timer3_var = {
    .psObj                =   NULL,
};

static struct nu_timer_var timer4_var = {
    .psObj                =   NULL,
};

static struct nu_timer_var timer5_var = {
    .psObj                =   NULL,
};

static const struct nu_modinit_s timer_modinit_tab[] = {
    {(uint32_t)TIMER0, TMR0_MODULE, CLK_TMRSEL_TMR0SEL_HIRC, MODULE_NoMsk, SYS_TMR0RST, TIMER0_IRQn, &timer0_var},
    {(uint32_t)TIMER1, TMR1_MODULE, CLK_TMRSEL_TMR1SEL_HIRC, MODULE_NoMsk, SYS_TMR1RST, TIMER1_IRQn, &timer1_var},
    {(uint32_t)TIMER2, TMR2_MODULE, CLK_TMRSEL_TMR2SEL_HIRC, MODULE_NoMsk, SYS_TMR2RST, TIMER2_IRQn, &timer2_var},
    {(uint32_t)TIMER3, TMR3_MODULE, CLK_TMRSEL_TMR3SEL_HIRC, MODULE_NoMsk, SYS_TMR3RST, TIMER3_IRQn, &timer3_var},
    {(uint32_t)LPTMR0, LPTMR0_MODULE, CLK_LPTMRSEL_LPTMR0SEL_HIRC, MODULE_NoMsk, SYS_LPTMR0RST, LPTMR0_IRQn, &timer4_var},
    {(uint32_t)LPTMR1, LPTMR1_MODULE, CLK_LPTMRSEL_LPTMR1SEL_HIRC, MODULE_NoMsk, SYS_LPTMR1RST, LPTMR1_IRQn, &timer5_var},

    {0, 0, 0, 0, 0, (IRQn_Type) 0, NULL}
};

int32_t Timer_Init(
    hw_timer_t *psObj,
    Timer_InitTypeDef *psInitDef
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    struct nu_timer_var *timer_var = (struct nu_timer_var *)modinit->var;

    // Reset this module
    SYS_ResetModule(modinit->rsetidx);
    // Select IP clock source
    CLK_SetModuleClock(modinit->clkidx, modinit->clksrc, modinit->clkdiv);
    // Enable IP clock
    CLK_EnableModuleClock(modinit->clkidx);

    timer_var->psObj = psObj;

    if(psObj->bLPTimer) {
        LPTMR_SET_PRESCALE_VALUE(psObj->u_timer.lptimer, psInitDef->u32Prescaler);
        LPTMR_SET_CMP_VALUE(psObj->u_timer.lptimer, psInitDef->u32Period);
        LPTMR_SET_OPMODE(psObj->u_timer.lptimer, psInitDef->u32Mode);

        return 0;
    }


    TIMER_SET_PRESCALE_VALUE(psObj->u_timer.timer, psInitDef->u32Prescaler);
    TIMER_SET_CMP_VALUE(psObj->u_timer.timer, psInitDef->u32Period);
    TIMER_SET_OPMODE(psObj->u_timer.timer, psInitDef->u32Mode);

    return 0;
}

void Timer_Final(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return;

    struct nu_timer_var *timer_var = (struct nu_timer_var *)modinit->var;

    timer_var->psObj = NULL;

    if(psObj->bLPTimer) {
        LPTMR_Stop(psObj->u_timer.lptimer);
        LPTMR_Close(psObj->u_timer.lptimer);
        return;
    }

    TIMER_Stop(psObj->u_timer.timer);
    TIMER_Close(psObj->u_timer.timer);
    return;
}

int32_t Timer_Trigger(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    if(psObj->bLPTimer) {
        LPTMR_Start(psObj->u_timer.lptimer);
        return 0;
    }

    TIMER_Start(psObj->u_timer.timer);
    return 0;
}

int32_t Timer_Suspend(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    if(psObj->bLPTimer) {
        LPTMR_Stop(psObj->u_timer.lptimer);
        return 0;
    }

    TIMER_Stop(psObj->u_timer.timer);
    return 0;
}

int32_t Timer_EnableInt(
    hw_timer_t *psObj,
    PFN_INT_STATUS_HANDLER pfnHandler
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    NVIC_EnableIRQ(modinit->irq_n);
    psObj->pfnStatusHandler = pfnHandler;

    if(psObj->bLPTimer) {
        LPTMR_ClearIntFlag(psObj->u_timer.lptimer);
        LPTMR_EnableInt(psObj->u_timer.lptimer);

        return 0;
    }

    TIMER_ClearIntFlag(psObj->u_timer.timer);
    TIMER_EnableInt(psObj->u_timer.timer);

    return 0;
}

int32_t Timer_DisableInt(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    psObj->pfnStatusHandler = NULL;

    if(psObj->bLPTimer) {
        LPTMR_DisableInt(psObj->u_timer.lptimer);

        return 0;
    }

    TIMER_DisableInt(psObj->u_timer.timer);

    return 0;
}

bool Timer_IsActivate(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return false;

    if(psObj->bLPTimer) {
        return LPTMR_IS_ACTIVE(psObj->u_timer.lptimer);
    }

    return TIMER_IS_ACTIVE(psObj->u_timer.timer);
}

int32_t Timer_EnablePWMPeriodInt(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    NVIC_EnableIRQ(modinit->irq_n);

    if(psObj->bLPTimer) {
        LPTPWM_ENABLE_PERIOD_INT(psObj->u_timer.lptimer);

        return 0;
    }

    TPWM_ENABLE_PERIOD_INT(psObj->u_timer.timer);

    return 0;
}

int32_t Timer_DisablePWMPeriodInt(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;


    if(psObj->bLPTimer) {
        LPTPWM_DISABLE_PERIOD_INT(psObj->u_timer.lptimer);

        return 0;
    }

    TPWM_DISABLE_PERIOD_INT(psObj->u_timer.timer);

    return 0;
}

int32_t Timer_EnableCaptureInt(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    NVIC_EnableIRQ(modinit->irq_n);

    if(psObj->bLPTimer) {
        LPTMR_ClearCaptureIntFlag(psObj->u_timer.lptimer);
        LPTMR_EnableCaptureInt(psObj->u_timer.lptimer);
        return 0;
    }

    TIMER_ClearCaptureIntFlag(psObj->u_timer.timer);
    TIMER_EnableCaptureInt(psObj->u_timer.timer);

    return 0;

}

int32_t Timer_DisableCaptureInt(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;


    if(psObj->bLPTimer) {
        LPTMR_DisableCaptureInt(psObj->u_timer.lptimer);

        return 0;
    }

    TIMER_DisableCaptureInt(psObj->u_timer.timer);

    return 0;
}

int32_t Timer_PWMTrigger(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    if(psObj->bLPTimer) {
        LPTPWM_START_COUNTER(psObj->u_timer.lptimer);
        return 0;
    }

    TPWM_START_COUNTER(psObj->u_timer.timer);
    return 0;
}

int32_t Timer_PWMSuspend(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    if(psObj->bLPTimer) {
        LPTPWM_STOP_COUNTER(psObj->u_timer.lptimer);
        return 0;
    }

    TPWM_STOP_COUNTER(psObj->u_timer.timer);
    return 0;
}

int32_t Timer_UpdatePrescaler(
    hw_timer_t *psObj,
    uint32_t u32Prescaler
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    if(psObj->bLPTimer) {
        LPTMR_SET_PRESCALE_VALUE(psObj->u_timer.lptimer, u32Prescaler);
        return 0;
    }

    TIMER_SET_PRESCALE_VALUE(psObj->u_timer.timer, u32Prescaler);
    return 0;
}

int32_t Timer_UpdatePeriod(
    hw_timer_t *psObj,
    uint32_t u32Period
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    if(psObj->bLPTimer) {
        LPTMR_SET_CMP_VALUE(psObj->u_timer.lptimer, u32Period);
        return 0;
    }

    TIMER_SET_CMP_VALUE(psObj->u_timer.timer, u32Period);
    return 0;
}

uint32_t Timer_GetModuleClock(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return 0;

    if(psObj->bLPTimer) {
        return LPTMR_GetModuleClock(psObj->u_timer.lptimer);
    }

    return TIMER_GetModuleClock(psObj->u_timer.timer);
}

int32_t Timer_SetPWMMode(
    hw_timer_t *psObj,
    uint32_t u32Freq,
    uint32_t u32Duty
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    if(psObj->bLPTimer) {
        /* Change Timer to PWM counter mode */
        LPTPWM_ENABLE_PWM_MODE(psObj->u_timer.lptimer);
        /* Set PWM mode as independent mode*/
        //LPTPWM_ENABLE_INDEPENDENT_MODE(psObj->u_timer.lptimer);

        LPTPWM_ConfigOutputFreqAndDuty(psObj->u_timer.lptimer, u32Freq, u32Duty);
        /* Set PWM down count type */
        //LPTPWM_SET_COUNTER_TYPE(psObj->u_timer.lptimer, LPTPWM_UP_COUNT);

        return 0;
    }

    /* Change Timer to PWM counter mode */
    TPWM_ENABLE_PWM_MODE(psObj->u_timer.timer);
    /* Set PWM mode as independent mode*/
    TPWM_ENABLE_INDEPENDENT_MODE(psObj->u_timer.timer);

    TPWM_ConfigOutputFreqAndDuty(psObj->u_timer.timer, u32Freq, u32Duty);
    /* Set PWM down count type */
    TPWM_SET_COUNTER_TYPE(psObj->u_timer.timer, TPWM_UP_COUNT);

    return 0;
}

int32_t Timer_EnablePWMOutputChannel(
    hw_timer_t *psObj,
    uint32_t u32Channel		//TPWM_CH0/TPWM_CH1
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    if(psObj->bLPTimer) {
        LPTPWM_ENABLE_OUTPUT(psObj->u_timer.lptimer, u32Channel);
        return 0;
    }

    TPWM_ENABLE_OUTPUT(psObj->u_timer.timer, u32Channel);
    return 0;
}

int32_t Timer_EnableCapture(
    hw_timer_t *psObj,
    uint32_t u32Mode,
    uint32_t u32Edge
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    if(psObj->bLPTimer) {
        LPTMR_EnableCapture(psObj->u_timer.lptimer, u32Mode, u32Edge);
        return 0;
    }

    TIMER_EnableCapture(psObj->u_timer.timer, u32Mode, u32Edge);
    return 0;
}

uint32_t Timer_GetCounter(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return 0;

    if(psObj->bLPTimer) {
        return LPTMR_GetCounter(psObj->u_timer.lptimer);
    }

    return TIMER_GetCounter(psObj->u_timer.timer);
}

uint32_t Timer_GetPWMPeriod(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return 0;

    if(psObj->bLPTimer) {
        return LPTPWM_GET_PERIOD(psObj->u_timer.lptimer);
    }

    return TPWM_GET_PERIOD(psObj->u_timer.timer);
}

int32_t Timer_UpdatePWMPeriod(
    hw_timer_t *psObj,
    uint32_t u32Period
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return -1;

    if(psObj->bLPTimer) {
        LPTPWM_SET_CMPDAT(psObj->u_timer.lptimer, u32Period);
        return 0;
    }

    TPWM_SET_CMPDAT(psObj->u_timer.timer, u32Period);
    return 0;
}

uint32_t Timer_GetCaptureData(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return 0;

    if(psObj->bLPTimer) {
        return LPTMR_GetCaptureData(psObj->u_timer.lptimer);
    }

    return TIMER_GetCaptureData(psObj->u_timer.timer);
}

uint32_t Timer_GetCompareData(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return 0;

    if(psObj->bLPTimer) {
        return (psObj->u_timer.lptimer)->CMP;
    }

    return (psObj->u_timer.timer)->CMP;
}

uint32_t Timer_GetPWMCompareData(
    hw_timer_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->u_timer.timer, timer_modinit_tab);

    if(modinit == NULL)
        return 0;

    if(psObj->bLPTimer) {
        return (psObj->u_timer.lptimer)->PWMCMPDAT;
    }

    return (psObj->u_timer.timer)->PWMCMPDAT;
}

void Handle_Timer_Irq(TIMER_T *timer)
{
    uint32_t u32Status;

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)timer, timer_modinit_tab);
    struct nu_timer_var *var = NULL;
    hw_timer_t *psTimerObj = NULL;

    if(modinit) {
        var = (struct nu_timer_var *) modinit->var;

        if(var) {
            psTimerObj = var->psObj;
        }
    }

    u32Status = TIMER_GetIntFlag(timer);
    if(u32Status) {
        TIMER_ClearIntFlag(timer);

        if((psTimerObj) && (psTimerObj->pfnStatusHandler)) {
            psTimerObj->pfnStatusHandler(psTimerObj, u32Status);
        }
    }

    u32Status = TIMER_GetCaptureIntFlag(timer);
    if(u32Status) {
        TIMER_ClearCaptureIntFlag(timer);

        if((psTimerObj) && (psTimerObj->pfnStatusHandler)) {
            psTimerObj->pfnStatusHandler(psTimerObj, u32Status);
        }
    }

    u32Status = (timer)->PWMINTSTS0;
    if(u32Status) {
        (timer)->PWMINTSTS0 = u32Status;

        if((psTimerObj) && (psTimerObj->pfnStatusHandler)) {
            psTimerObj->pfnStatusHandler(psTimerObj, u32Status);
        }
    }

}

void Handle_LPTimer_Irq(LPTMR_T *lptimer)
{
    uint32_t u32Status;

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)lptimer, timer_modinit_tab);
    struct nu_timer_var *var = NULL;
    hw_timer_t *psTimerObj = NULL;

    if(modinit) {
        var = (struct nu_timer_var *) modinit->var;

        if(var) {
            psTimerObj = var->psObj;
        }
    }

    u32Status = LPTMR_GetIntFlag(lptimer);
    if(u32Status) {
        LPTMR_ClearIntFlag(lptimer);
        if((psTimerObj) && (psTimerObj->pfnStatusHandler)) {
            psTimerObj->pfnStatusHandler(psTimerObj, u32Status);
        }
    }

    u32Status = LPTMR_GetCaptureIntFlag(lptimer);
    if(u32Status) {
        LPTMR_ClearCaptureIntFlag(lptimer);
        if((psTimerObj) && (psTimerObj->pfnStatusHandler)) {
            psTimerObj->pfnStatusHandler(psTimerObj, u32Status);
        }
    }

    u32Status = (lptimer)->PWMINTSTS0;
    if(u32Status) {
        (lptimer)->PWMINTSTS0 = u32Status;
        if((psTimerObj) && (psTimerObj->pfnStatusHandler)) {
            psTimerObj->pfnStatusHandler(psTimerObj, u32Status);
        }
    }
}


