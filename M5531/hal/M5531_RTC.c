/**************************************************************************//**
 * @file     M5531_RTC.c
 * @version  V0.01
 * @brief    M5531 series RTC HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"

#include "M5531_RTC.h"
#include "nu_modutil.h"

struct nu_rtc_var {
    rtc_t *  psObj;
};

static struct nu_rtc_var rtc0_var = {
    .psObj                =   NULL,
};

static const struct nu_modinit_s rtc_modinit_tab[] = {
    {(uint32_t)RTC, RTC0_MODULE, MODULE_NoMsk, MODULE_NoMsk, SYS_RTC0RST, RTC_IRQn, &rtc0_var},

    {0, 0, 0, 0, 0, (IRQn_Type) 0, NULL}
};

int32_t RTC_Init(
    rtc_t *psObj,
    RTC_InitTypeDef *psInitDef
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->rtc, rtc_modinit_tab);

    if(modinit == NULL)
        return -1;

    struct nu_rtc_var *var = (struct nu_rtc_var *) modinit->var;

    // Reset this module
    SYS_ResetModule(modinit->rsetidx);
    /* Set LXT as RTC clock source */
    RTC_SetClockSource(RTC_CLOCK_SOURCE_LXT);
    // Enable IP clock
    CLK_EnableModuleClock(modinit->clkidx);
    var->psObj = psObj;
    return RTC_Open(psInitDef->psRTCTimeDate);
}

int32_t RTC_EnableIntFlag(
    rtc_t *psObj,
    uint32_t u32IntFlagMask,
    PFN_RTC_INT_HANDLER pfnHandler
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->rtc, rtc_modinit_tab);

    if(modinit == NULL)
        return -1;

    psObj->pfnRTCIntHandler = pfnHandler;

    /* Enable RTC interrupt */
    RTC_EnableInt(u32IntFlagMask);

    /* Enable RTC NVIC */
    NVIC_EnableIRQ(modinit->irq_n);

    return 0;
}

int32_t RTC_DisableIntFlag(
    rtc_t *psObj,
    uint32_t u32IntFlagMask
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->rtc, rtc_modinit_tab);

    if(modinit == NULL)
        return -1;

    /* Disable RTC NVIC */
    NVIC_DisableIRQ(modinit->irq_n);
    /* Disable RTC interrupt */
    RTC_DisableInt(u32IntFlagMask);
    psObj->pfnRTCIntHandler = NULL;

    return 0;
}

void Handle_RTC_Irq(RTC_T *rtc)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)rtc, rtc_modinit_tab);
    struct nu_rtc_var *var = NULL;
    rtc_t *psRTCObj = NULL;

    if(modinit) {
        var = (struct nu_rtc_var *) modinit->var;
        psRTCObj = var->psObj;
    }

    if((psRTCObj) && (psRTCObj->pfnRTCIntHandler)) {
        psRTCObj->pfnRTCIntHandler(psRTCObj);
    }
}
