/**************************************************************************//**
 * @file     N9H26_CPSR.c
 * @version  V0.01
 * @brief    N9H26 series CPSR operation source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>

#include "N9H26_CPSR.h"

uint32_t CPSR_GetMode()
{
	int32_t i32Temp;
	
#if defined (__GNUC__) && !(__CC_ARM)
	__asm
    (
        "MRS %0, CPSR   \n"
        :"=r" (i32Temp) : :
    );
#else
	__asm
	{
		MRS	i32Temp, CPSR
	}
#endif
	return i32Temp & ARM_CPSR_MODE_MASK;
}

uint32_t CPSR_GetIFState()
{
	int32_t i32Temp;
	
#if defined (__GNUC__) && !(__CC_ARM)
	__asm
    (
        "MRS %0, CPSR   \n"
        :"=r" (i32Temp) : :
    );
#else
	__asm
	{
		MRS	i32Temp, CPSR
	}
#endif
	return i32Temp & ARM_CPSR_IF_MASK;
}


void CPSR_EnableIF(
	uint32_t u32IFMask
)
{
	uint32_t u32IFTemp;
	u32IFTemp = u32IFMask & ARM_CPSR_IF_MASK;

#if defined (__GNUC__) && !(__CC_ARM)

	if(u32IFTemp ==  ARM_CPSR_IF_MASK){
		__asm
		(
			"mrs    r0, CPSR  \n"
			"bic    r0, r0, #0xC0  \n"
			"msr    CPSR_c, r0  \n"
		);
	}
	else if(u32IFTemp ==  ARM_CPSR_I_BIT){
		__asm
		(
			"mrs    r0, CPSR  \n"
			"bic    r0, r0, #0x80  \n"
			"msr    CPSR_c, r0  \n"
		);
	}
	else if(u32IFTemp ==  ARM_CPSR_F_BIT){
		__asm
		(
			"mrs    r0, CPSR  \n"
			"bic    r0, r0, #0x40  \n"
			"msr    CPSR_c, r0  \n"
		);
	}
	
#else

	uint32_t temp;
	__asm
	{
	   MRS    temp, CPSR
	   AND    temp, temp, u32IFMask
	   MSR    CPSR_c, temp
	}

#endif	
}

void CPSR_DisableIF(
	uint32_t u32IFMask
)
{
	uint32_t u32IFTemp;
	u32IFTemp = u32IFMask & ARM_CPSR_IF_MASK;

#if defined (__GNUC__) && !(__CC_ARM)

	if(u32IFTemp ==  ARM_CPSR_IF_MASK){
		__asm
		(
			"mrs    r0, CPSR  \n"
			"orr    r0, r0, #0xC0  \n"
			"msr    CPSR_c, r0  \n"
		);
	}
	else if(u32IFTemp ==  ARM_CPSR_I_BIT){
		__asm
		(
			"mrs    r0, CPSR  \n"
			"orr    r0, r0, #0x80  \n"
			"msr    CPSR_c, r0  \n"
		);
	}
	else if(u32IFTemp ==  ARM_CPSR_F_BIT){
		__asm
		(
			"mrs    r0, CPSR  \n"
			"orr    r0, r0, #0x40  \n"
			"msr    CPSR_c, r0  \n"
		);
	}
	
#else

	uint32_t temp;
	__asm
	{
	   MRS    temp, CPSR
	   ORR    temp, temp, u32IFMask
	   MSR    CPSR_c, temp
	}

#endif	
}



