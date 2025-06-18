/**************************************************************************//**
 * @file     M5531_RTC.h
 * @version  V1.00
 * @brief    M5531 RTC HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M5531_RTC_H__
#define __M5531_RTC_H__

typedef void (*PFN_RTC_INT_HANDLER)(void *obj);

typedef struct {
    RTC_T *rtc;
    PFN_RTC_INT_HANDLER pfnRTCIntHandler;
} rtc_t;

typedef struct {
    S_RTC_TIME_DATA_T *psRTCTimeDate;
} RTC_InitTypeDef;

int32_t RTC_Init(
    rtc_t *psObj,
    RTC_InitTypeDef *psInitDef
);

int32_t RTC_EnableIntFlag(
    rtc_t *psObj,
    uint32_t u32IntFlagMask,
    PFN_RTC_INT_HANDLER pfnHandler
);

int32_t RTC_DisableIntFlag(
    rtc_t *psObj,
    uint32_t u32IntFlagMask
);

void Handle_RTC_Irq(RTC_T *rtc);

#endif
