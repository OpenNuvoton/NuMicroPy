/**************************************************************************//**
 * @file     N9H26_CPSR.h
 * @version  V1.00
 * @brief    N9H26 CPSR header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __N9H26_CPSR_H__
#define __N9H26_CPSR_H__

#include <inttypes.h>


#define ARM_CPSR_MODE_MASK		0x1F

#define ARM_CPSR_USER_MODE		0x10
#define ARM_CPSR_FIQ_MODE		0x11
#define ARM_CPSR_IRQ_MODE		0x12
#define ARM_CPSR_SUPER_MODE		0x13
#define ARM_CPSR_ABORT_MODE		0x17
#define ARM_CPSR_UNDEF_MODE		0x1B
#define ARM_CPSR_SYSTEM_MODE	0x1F

#define ARM_CPSR_IF_MASK		0xC0
#define ARM_CPSR_I_BIT			0x80
#define ARM_CPSR_F_BIT			0x40

uint32_t CPSR_GetMode();

uint32_t CPSR_GetIFState();

void CPSR_EnableIF(
	uint32_t u32IFMask
);

void CPSR_DisableIF(
	uint32_t u32IFMask
);


#endif
