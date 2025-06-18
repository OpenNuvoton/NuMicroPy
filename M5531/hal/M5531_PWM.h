/**************************************************************************//**
 * @file     M5531_PWM.h
 * @version  V1.00
 * @brief    M5531 pWM HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M5531_PWM_H__
#define __M5531_PWM_H__

typedef enum {
    eCAPTURE_RISING_LATCH = 1,
    eCAPTURE_FALLING_LATCH,
    eCAPTURE_RISING_FALLING_LATCH,
} E_CAPTURE_EDGE_LATCH_TYPE;

typedef void (*PFN_CAPTURE_INT_HANDLER)(void *obj, uint32_t u32ChannelGroup);

typedef struct {
    union {
        BPWM_T *bpwm;
        EPWM_T *epwm;
    } u_pwm;
    bool bEPWM;
    PFN_CAPTURE_INT_HANDLER pfnCaptureHandler;
} pwm_t;

int32_t PWM_Init(
    pwm_t *psPWMObj
);

void PWM_Final(
    pwm_t *psPWMObj
);

int32_t PWM_TriggerOutputChannel(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    bool bComplementary,			//Complentary support. Only for EPWM
    uint32_t u32ComplementaryChannel,
    uint32_t u32Freq,
    uint32_t u32Duty
);

int32_t PWM_StopOutputChannel(
    pwm_t *psPWMObj,
    uint32_t u32Channel
);

int32_t PWM_TriggerCaptureChannel(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    uint32_t u32Freq,
    E_CAPTURE_EDGE_LATCH_TYPE eCaptureEdge
);

int32_t PWM_StopCaptureChannel(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    E_CAPTURE_EDGE_LATCH_TYPE eCaptureEdge
);

int32_t PWM_CaptureEnableInt(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    E_CAPTURE_EDGE_LATCH_TYPE eCaptureEdge,
    PFN_CAPTURE_INT_HANDLER pfnHandler
);

int32_t PWM_CaptureDisableInt(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    E_CAPTURE_EDGE_LATCH_TYPE eCaptureEdge
);

int32_t PWM_CaptureClearInt(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    E_CAPTURE_EDGE_LATCH_TYPE eCaptureEdge
);

int32_t PWM_ChangeChannelDuty(
    pwm_t *psPWMObj,
    uint32_t u32Channel,
    uint32_t u32Duty
);

void Handle_BPWM_Irq(BPWM_T *bpwm);
void Handle_EPWM_Irq(EPWM_T *epwm, uint32_t u32ChannGroup);


#endif

