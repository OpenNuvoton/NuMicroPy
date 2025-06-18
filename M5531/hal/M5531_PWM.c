/**************************************************************************//**
 * @file     M5531_PWM.c
 * @version  V0.01
 * @brief    M5531 series PWM HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "py/mphal.h"

#include "NuMicro.h"
#include "M5531_PWM.h"
#include "nu_modutil.h"

#define MAX_PWM_INST 2

typedef struct nu_pwm_var {
    pwm_t *     obj;
} s_nu_pwm_var;

static struct nu_pwm_var bpwm0_var = {
    .obj                =   NULL,
};

static struct nu_pwm_var bpwm1_var = {
    .obj                =   NULL,
};

static struct nu_pwm_var epwm0_var = {
    .obj                =   NULL,
};

static struct nu_pwm_var epwm1_var = {
    .obj                =   NULL,
};


static const struct nu_modinit_s bpwm_modinit_tab[] = {
    {(uint32_t)BPWM0, BPWM0_MODULE, CLK_BPWMSEL_BPWM0SEL_PCLK0, 0, SYS_BPWM0RST, BPWM0_IRQn, &bpwm0_var},
    {(uint32_t)BPWM1, BPWM1_MODULE, CLK_BPWMSEL_BPWM1SEL_PCLK2, 0, SYS_BPWM1RST, BPWM1_IRQn, &bpwm1_var},

    {0, 0, 0, 0, 0, (IRQn_Type) 0, NULL}
};

static const struct nu_modinit_s epwm_modinit_tab[] = {
    {(uint32_t)EPWM0, EPWM0_MODULE, CLK_EPWMSEL_EPWM0SEL_PCLK0, 0, SYS_EPWM0RST, EPWM0P0_IRQn, &epwm0_var},
    {(uint32_t)EPWM1, EPWM1_MODULE, CLK_EPWMSEL_EPWM1SEL_PCLK2, 0, SYS_EPWM1RST, EPWM1P0_IRQn, &epwm1_var},

    {0, 0, 0, 0, 0, (IRQn_Type) 0, NULL}
};


int32_t PWM_Init(
    pwm_t *psPWMObj
)
{
    const struct nu_modinit_s *modinit = NULL;

    if(psPWMObj->bEPWM) {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.epwm, epwm_modinit_tab);
    } else {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.bpwm, bpwm_modinit_tab);
    }

    if(modinit == NULL)
        return -1;

    // Reset this module
    SYS_ResetModule(modinit->rsetidx);
    // Select IP clock source
    CLK_SetModuleClock(modinit->clkidx, modinit->clksrc, modinit->clkdiv);

    CLK_EnableModuleClock(modinit->clkidx);

    psPWMObj->pfnCaptureHandler = NULL;
    return 0;
}

void PWM_Final(
    pwm_t *psPWMObj
)
{
    const struct nu_modinit_s *modinit = NULL;

    if(psPWMObj->bEPWM) {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.epwm, epwm_modinit_tab);
    } else {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.bpwm, bpwm_modinit_tab);
    }

    if(modinit == NULL)
        return;

    if(psPWMObj->bEPWM) {
        EPWM_ForceStop(psPWMObj->u_pwm.epwm, 0x3F);
    } else {
        BPWM_ForceStop(psPWMObj->u_pwm.bpwm, 0x3F);
    }


    /* Disable module clock */
    CLK_DisableModuleClock(modinit->clkidx);
}

int32_t PWM_TriggerOutputChannel(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    bool bComplementary,
    uint32_t u32ComplementaryChannel,
    uint32_t u32Freq,
    uint32_t u32Duty
)
{
    const struct nu_modinit_s *modinit = NULL;

    if(psPWMObj->bEPWM) {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.epwm, epwm_modinit_tab);
    } else {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.bpwm, bpwm_modinit_tab);
    }

    if(modinit == NULL)
        return -1;

    if(psPWMObj->bEPWM) {

        EPWM_ConfigOutputChannel(psPWMObj->u_pwm.epwm, u32Channel, u32Freq, u32Duty);

        /* Enable output of EPWM0/1 channel 0~5 */
        EPWM_EnableOutput(psPWMObj->u_pwm.epwm, 1 << u32Channel);

        //Complentary support. Only for EPWM
        if(bComplementary) {
            psPWMObj->u_pwm.epwm->CTL1 |= ((0x1 << (u32ComplementaryChannel / 2))<<EPWM_CTL1_OUTMODE0_Pos);
        }

        /* Start EPWM0/1 counter */
        EPWM_Start(psPWMObj->u_pwm.epwm, 1 << u32Channel);
        return 0;
    }


    BPWM_ConfigOutputChannel(psPWMObj->u_pwm.bpwm, u32Channel, u32Freq, u32Duty);

    /* Enable output of EPWM0/1 channel 0~5 */
    BPWM_EnableOutput(psPWMObj->u_pwm.bpwm, 1 << u32Channel);

    /* Start EPWM0/1 counter */
    BPWM_Start(psPWMObj->u_pwm.bpwm, 1 << u32Channel);
    return 0;

}

int32_t PWM_StopOutputChannel(
    pwm_t *psPWMObj,
    uint32_t u32Channel
)
{
    const struct nu_modinit_s *modinit = NULL;

    if(psPWMObj->bEPWM) {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.epwm, epwm_modinit_tab);
    } else {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.bpwm, bpwm_modinit_tab);
    }

    if(modinit == NULL)
        return -1;

    if(psPWMObj->bEPWM) {
        /* Disable Timer for EPWM0/1 channel 0~5 */
        EPWM_Stop(psPWMObj->u_pwm.epwm, 1 << u32Channel);

        /* Disable EPWM Output path for EPWM0/1 channel 0~5 */
        EPWM_DisableOutput(psPWMObj->u_pwm.epwm, 1 << u32Channel);

        return 0;
    }

    /* Disable Timer for BPWM0/1 channel 0~5 */
    BPWM_Stop(psPWMObj->u_pwm.bpwm, 1 << u32Channel);

    /* Disable BPWM Output path for BPWM0/1 channel 0~5 */
    BPWM_DisableOutput(psPWMObj->u_pwm.bpwm, 1 << u32Channel);

    return 0;

}

#define EPWM_SET_CHANNEL_COUNTER_TYPE(epwm, u32ChannelNum, u32CounterType)	(epwm)->CTL1 = ((epwm)->CTL1 & ~((11UL << EPWM_CTL1_CNTTYPE0_Pos) << (u32ChannelNum << 1U))) | (u32CounterType << (u32ChannelNum << 1U));
#define BPWM_SET_CHANNEL_COUNTER_TYPE(pwm, u32ChannelNum, u32CounterType)	(pwm)->CTL1 = ((pwm)->CTL1 & ~((11UL << BPWM_CTL1_CNTTYPE0_Pos) << (u32ChannelNum << 1U))) | (u32CounterType << (u32ChannelNum << 1U));

int32_t PWM_TriggerCaptureChannel(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    uint32_t u32Freq,
    E_CAPTURE_EDGE_LATCH_TYPE eCaptureEdge
)
{
    const struct nu_modinit_s *modinit = NULL;

    if(psPWMObj->bEPWM) {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.epwm, epwm_modinit_tab);
    } else {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.bpwm, bpwm_modinit_tab);
    }

    if(modinit == NULL)
        return -1;


    if(psPWMObj->bEPWM) {


        /* set EPWM0/1 channel 0~5 capture configuration */
        EPWM_ConfigCaptureChannel(psPWMObj->u_pwm.epwm, u32Channel, 1000000000 / u32Freq, 0);
        EPWM_SET_CHANNEL_COUNTER_TYPE(psPWMObj->u_pwm.epwm, u32Channel, EPWM_UP_COUNTER);

        /* Enable Timer for EPWM0/1 channel 0~5 */
        EPWM_Start(psPWMObj->u_pwm.epwm, 1 << u32Channel);

        /* Enable Capture Function for EPWM0/1 channel 0~5 */
        EPWM_EnableCapture(psPWMObj->u_pwm.epwm, 1 << u32Channel);

#if 0
        /* Enable edge capture reload */
        if(eCaptureEdge & eCAPTURE_RISING_LATCH)
            psPWMObj->u_pwm.epwm->CAPCTL |= (1 << (EPWM_CAPCTL_RCRLDEN0_Pos + u32Channel));
        if(eCaptureEdge & eCAPTURE_FALLING_LATCH)
            psPWMObj->u_pwm.epwm->CAPCTL |= (1 << (EPWM_CAPCTL_FCRLDEN0_Pos + u32Channel));
#endif

        return 0;
    }


    /* set BPWM0/1 channel 0~5 capture configuration */
    BPWM_ConfigCaptureChannel(psPWMObj->u_pwm.bpwm, u32Channel, 1000000000 / u32Freq, 0);
    BPWM_SET_CHANNEL_COUNTER_TYPE(psPWMObj->u_pwm.bpwm, u32Channel, BPWM_UP_COUNTER);

    /* Enable Timer for BPWM0/1 channel 0~5 */
    BPWM_Start(psPWMObj->u_pwm.bpwm, 1 << u32Channel);

    /* Enable Capture Function for BPWM0/1 channel 0~5 */
    BPWM_EnableCapture(psPWMObj->u_pwm.bpwm, 1 << u32Channel);

#if 0
    /* Enable edge capture reload */
    if(eCaptureEdge & CAPTURE_RISING_LATCH)
        psPWMObj->u_pwm.bpwm->CAPCTL |= (1 << (BPWM_CAPCTL_RCRLDEN0_Pos + u32Channel));
    if(eCaptureEdge & CAPTURE_FALLING_LATCH)
        psPWMObj->u_pwm.bpwm->CAPCTL |= (1 << (BPWM_CAPCTL_FCRLDEN0_Pos + u32Channel));
#endif
    return 0;
}

int32_t PWM_StopCaptureChannel(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    E_CAPTURE_EDGE_LATCH_TYPE eCaptureEdge
)
{
    const struct nu_modinit_s *modinit = NULL;

    if(psPWMObj->bEPWM) {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.epwm, epwm_modinit_tab);
    } else {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.bpwm, bpwm_modinit_tab);
    }

    if(modinit == NULL)
        return -1;


    if(psPWMObj->bEPWM) {

        /* Disable Timer for EPWM0/1 channel 0~5 */
        EPWM_Stop(psPWMObj->u_pwm.epwm, 1 << u32Channel);

        /* Disable EPWM Capture path for EPWM0/1 channel 0~5 */
        EPWM_DisableCapture(psPWMObj->u_pwm.epwm, 1 << u32Channel);

        /* Disable edge capture reload */
        if(eCaptureEdge & eCAPTURE_RISING_LATCH) {
            psPWMObj->u_pwm.epwm->CAPCTL &= ~(1 << (EPWM_CAPCTL_RCRLDEN0_Pos + u32Channel));
            EPWM_ClearCaptureIntFlag(psPWMObj->u_pwm.epwm, u32Channel, EPWM_CAPTURE_INT_RISING_LATCH);
        }

        if(eCaptureEdge & eCAPTURE_FALLING_LATCH) {
            psPWMObj->u_pwm.epwm->CAPCTL &= ~(1 << (EPWM_CAPCTL_FCRLDEN0_Pos + u32Channel));
            EPWM_ClearCaptureIntFlag(psPWMObj->u_pwm.epwm, u32Channel, EPWM_CAPTURE_INT_FALLING_LATCH);
        }

        return 0;
    }

    /* Disable Timer for BPWM0/1 channel 0~5 */
    BPWM_Stop(psPWMObj->u_pwm.bpwm, 1 << u32Channel);

    /* Disable BPWM Capture path for BPWM0/1 channel 0~5 */
    BPWM_DisableCapture(psPWMObj->u_pwm.bpwm, 1 << u32Channel);

    /* Disable edge capture reload */
    if(eCaptureEdge & eCAPTURE_RISING_LATCH) {
        psPWMObj->u_pwm.bpwm->CAPCTL &= ~(1 << (BPWM_CAPCTL_RCRLDEN0_Pos + u32Channel));
        BPWM_ClearCaptureIntFlag(psPWMObj->u_pwm.bpwm, u32Channel, BPWM_CAPTURE_INT_RISING_LATCH);
    }

    if(eCaptureEdge & eCAPTURE_FALLING_LATCH) {
        psPWMObj->u_pwm.bpwm->CAPCTL &= ~(1 << (BPWM_CAPCTL_FCRLDEN0_Pos + u32Channel));
        BPWM_ClearCaptureIntFlag(psPWMObj->u_pwm.bpwm, u32Channel, BPWM_CAPTURE_INT_FALLING_LATCH);
    }

    return 0;
}



int32_t PWM_CaptureEnableInt(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    E_CAPTURE_EDGE_LATCH_TYPE eCaptureEdge,
    PFN_CAPTURE_INT_HANDLER pfnHandler
)
{
    const struct nu_modinit_s *modinit = NULL;

    if(psPWMObj->bEPWM) {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.epwm, epwm_modinit_tab);
    } else {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.bpwm, bpwm_modinit_tab);
    }

    if(modinit == NULL)
        return -1;

    uint32_t u32IntFlag = 0;

    struct nu_pwm_var *var = (struct nu_pwm_var *) modinit->var;

    var->obj = psPWMObj;
    psPWMObj->pfnCaptureHandler = pfnHandler;

    if(psPWMObj->bEPWM) {
        if(eCaptureEdge & eCAPTURE_RISING_LATCH)
            u32IntFlag |= EPWM_CAPTURE_INT_RISING_LATCH;
        if(eCaptureEdge & eCAPTURE_FALLING_LATCH)
            u32IntFlag |= EPWM_CAPTURE_INT_FALLING_LATCH;

        EPWM_EnableCaptureInt(psPWMObj->u_pwm.epwm, u32Channel, u32IntFlag);
        return 0;
    }

    if(eCaptureEdge & eCAPTURE_RISING_LATCH)
        u32IntFlag |= BPWM_CAPTURE_INT_RISING_LATCH;
    if(eCaptureEdge & eCAPTURE_FALLING_LATCH)
        u32IntFlag |= BPWM_CAPTURE_INT_FALLING_LATCH;

    BPWM_EnableCaptureInt(psPWMObj->u_pwm.bpwm, u32Channel, u32IntFlag);
    return 0;

}

int32_t PWM_CaptureDisableInt(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    E_CAPTURE_EDGE_LATCH_TYPE eCaptureEdge
)
{
    const struct nu_modinit_s *modinit = NULL;

    if(psPWMObj->bEPWM) {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.epwm, epwm_modinit_tab);
    } else {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.bpwm, bpwm_modinit_tab);
    }

    if(modinit == NULL)
        return -1;

    uint32_t u32IntFlag = 0;
    struct nu_pwm_var *var = (struct nu_pwm_var *) modinit->var;

    psPWMObj->pfnCaptureHandler = NULL;
    var->obj = NULL;

    if(psPWMObj->bEPWM) {
        if(eCaptureEdge & eCAPTURE_RISING_LATCH)
            u32IntFlag |= EPWM_CAPTURE_INT_RISING_LATCH;
        if(eCaptureEdge & eCAPTURE_FALLING_LATCH)
            u32IntFlag |= EPWM_CAPTURE_INT_FALLING_LATCH;

        EPWM_DisableCaptureInt(psPWMObj->u_pwm.epwm, u32Channel, u32IntFlag);

        return 0;
    }

    if(eCaptureEdge & eCAPTURE_RISING_LATCH)
        u32IntFlag |= BPWM_CAPTURE_INT_RISING_LATCH;
    if(eCaptureEdge & eCAPTURE_FALLING_LATCH)
        u32IntFlag |= BPWM_CAPTURE_INT_FALLING_LATCH;

    BPWM_DisableCaptureInt(psPWMObj->u_pwm.bpwm, u32Channel, u32IntFlag);
    return 0;

}

int32_t PWM_CaptureClearInt(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    E_CAPTURE_EDGE_LATCH_TYPE eCaptureEdge
)
{
    const struct nu_modinit_s *modinit = NULL;

    if(psPWMObj->bEPWM) {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.epwm, epwm_modinit_tab);
    } else {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.bpwm, bpwm_modinit_tab);
    }

    if(modinit == NULL)
        return -1;

    uint32_t u32IntFlag = 0;

    if(psPWMObj->bEPWM) {
        if(eCaptureEdge & eCAPTURE_RISING_LATCH)
            u32IntFlag |= EPWM_CAPTURE_INT_RISING_LATCH;
        if(eCaptureEdge & eCAPTURE_FALLING_LATCH)
            u32IntFlag |= EPWM_CAPTURE_INT_FALLING_LATCH;

        EPWM_ClearCaptureIntFlag(psPWMObj->u_pwm.epwm, u32Channel, u32IntFlag);
        return 0;
    }

    if(eCaptureEdge & eCAPTURE_RISING_LATCH)
        u32IntFlag |= BPWM_CAPTURE_INT_RISING_LATCH;
    if(eCaptureEdge & eCAPTURE_FALLING_LATCH)
        u32IntFlag |= BPWM_CAPTURE_INT_FALLING_LATCH;

    BPWM_ClearCaptureIntFlag(psPWMObj->u_pwm.bpwm, u32Channel, u32IntFlag);
    return 0;
}

static uint32_t BPWMCalNewDutyCMR(BPWM_T *pwm, uint32_t u32ChannelNum, uint32_t u32DutyCycle, uint32_t u32CycleResolution)
{
    return (u32DutyCycle * (BPWM_GET_CNR(pwm, u32ChannelNum) + 1) / u32CycleResolution);
}

static uint32_t EPWMCalNewDutyCMR(EPWM_T *epwm, uint32_t u32ChannelNum, uint32_t u32DutyCycle, uint32_t u32CycleResolution)
{
    return (u32DutyCycle * (EPWM_GET_CNR(epwm, u32ChannelNum) + 1) / u32CycleResolution);
}


int32_t PWM_ChangeChannelDuty(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    uint32_t u32Duty
)
{
    const struct nu_modinit_s *modinit = NULL;

    if(psPWMObj->bEPWM) {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.epwm, epwm_modinit_tab);
    } else {
        modinit = get_modinit((uint32_t)psPWMObj->u_pwm.bpwm, bpwm_modinit_tab);
    }

    if(modinit == NULL)
        return -1;

    uint32_t u32NewCMR;

    if(psPWMObj->bEPWM) {
        u32NewCMR = EPWMCalNewDutyCMR(psPWMObj->u_pwm.epwm, u32Channel, u32Duty, 100);

        /* Set new comparator value to register */
        EPWM_SET_CMR(psPWMObj->u_pwm.epwm, u32Channel, u32NewCMR);

        return 0;
    }

    u32NewCMR = BPWMCalNewDutyCMR(psPWMObj->u_pwm.bpwm, u32Channel, u32Duty, 100);

    /* Set new comparator value to register */
    BPWM_SET_CMR(psPWMObj->u_pwm.bpwm, u32Channel, u32NewCMR);

    return 0;
}

/**
 * Handle the BPWM interrupt
 * @param[in] obj The BPWM peripheral that generated the interrupt
 * @return
 */

void Handle_BPWM_Irq(BPWM_T *bpwm)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)bpwm, bpwm_modinit_tab);

    if(modinit == NULL)
        return;

    struct nu_pwm_var *var = (struct nu_pwm_var *) modinit->var;

    pwm_t *psPWMObj = var->obj;


    if((psPWMObj) && (psPWMObj->pfnCaptureHandler)) {
        psPWMObj->pfnCaptureHandler(psPWMObj, 0);
    }
}

/**
 * Handle the EPWM interrupt
 * @param[in] obj The EPWM peripheral that generated the interrupt
 * @return
 */

void Handle_EPWM_Irq(EPWM_T *epwm, uint32_t u32ChannGroup)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)epwm, epwm_modinit_tab);

    if(modinit == NULL)
        return;

    struct nu_pwm_var *var = (struct nu_pwm_var *) modinit->var;

    pwm_t *psPWMObj = var->obj;


    if((psPWMObj) && (psPWMObj->pfnCaptureHandler)) {
        psPWMObj->pfnCaptureHandler(psPWMObj, u32ChannGroup);
    }
}


