/**************************************************************************//**
 * @file     M55M1_DAC.h
 * @version  V1.00
 * @brief    M55M1 DAC HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M55M1_DAC_H__
#define __M55M1_DAC_H__

typedef enum {
    eDAC_BITWIDTH_8 = 8,
    eDAC_BITWIDTH_12 = 12,
} E_DAC_BITWIDTH;

typedef struct {
    DAC_T *dac;
} dac_t;

typedef struct {
    uint32_t u32TriggerMode;		// DAC_SOFTWARE_TRIGGER/DAC_TIMER0_TRIGGER/DAC_TIMER1_TRIGGER/DAC_TIMER2_TRIGGER/DAC_TIMER3_TRIGGER
    E_DAC_BITWIDTH eBitWidth;
} DAC_InitTypeDef;

int32_t DAC_Init(
    dac_t *psObj,
    DAC_InitTypeDef *psInitDef
);

void DAC_Final(
    dac_t *psObj
);

int32_t DAC_SingleConv(
    dac_t *psObj,
    uint32_t u32Data
);

int32_t DAC_TimerPDMAConv(
    dac_t *psObj,
    TIMER_T *timer,
    uint32_t u32TriggerMode,
    E_DAC_BITWIDTH eBitWidth,
    uint8_t *pu8Buf,
    uint32_t u32BufLen,
    bool bCircular
);

int32_t DAC_StopTimerPDMAConv(
    dac_t *psObj,
    TIMER_T *timer
);


#endif
