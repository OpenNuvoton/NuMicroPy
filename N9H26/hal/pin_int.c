/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "mods/pybpin.h"

#include "mpconfigboard_common.h"

#include "hal/N9H26_CPSR.h"

#define EXTI_NUM_VECTORS        (PYB_EXTI_NUM_VECTORS)

STATIC mp_obj_t pyb_extint_callback_arg[EXTI_NUM_VECTORS];

extern void EXTINT0_IRQHandler(void);
extern void EXTINT1_IRQHandler(void);
extern void EXTINT2_IRQHandler(void);
extern void EXTINT3_IRQHandler(void);

STATIC void gpioint_enable(const pin_obj_t *pin, uint32_t mode){
	uint32_t u32IRQSrcGroupReg;
	uint32_t u32IRQIntEnableReg;
	uint32_t u32IRQNum;
	uint32_t u32SrcGroupMask = 3 << (pin->pin * 2);
	uint32_t u32IRQEnMask = 0x00010001 << (pin->pin);

	if(pin->port == GPIO_PORTA){
		u32IRQSrcGroupReg = REG_IRQSRCGPA;
		u32IRQIntEnableReg = REG_IRQENGPA;
		u32IRQNum = 0;
	}

	if(pin->port == GPIO_PORTB){
		u32IRQSrcGroupReg = REG_IRQSRCGPB;
		u32IRQIntEnableReg = REG_IRQENGPB;
		u32IRQNum = 0;
	}

	if(pin->port == GPIO_PORTC){
		u32IRQSrcGroupReg = REG_IRQSRCGPC;
		u32IRQIntEnableReg = REG_IRQENGPC;
		u32IRQNum = 1;
	}

	if(pin->port == GPIO_PORTD){
		u32IRQSrcGroupReg = REG_IRQSRCGPD;
		u32IRQIntEnableReg = REG_IRQENGPD;
		u32IRQNum = 1;

	}

	if(pin->port == GPIO_PORTE){
		u32IRQSrcGroupReg = REG_IRQSRCGPE;
		u32IRQIntEnableReg = REG_IRQENGPE;
		u32IRQNum = 2;

	}

	if(pin->port == GPIO_PORTG){
		u32IRQSrcGroupReg = REG_IRQSRCGPG;
		u32IRQIntEnableReg = REG_IRQENGPG;
		u32IRQNum = 2;

	}

	if(pin->port == GPIO_PORTH){
		u32IRQSrcGroupReg = REG_IRQSRCGPH;
		u32IRQIntEnableReg = REG_IRQENGPH;
		u32IRQNum = 3;

	}

	//disalbe IRQ
	CPSR_DisableIF(ARM_CPSR_I_BIT);
	
	u32IRQNum = u32IRQNum << (pin->pin * 2);

	outp32(u32IRQSrcGroupReg, inp32(u32IRQSrcGroupReg) & (~u32SrcGroupMask)); 
	outp32(u32IRQSrcGroupReg, inp32(u32IRQSrcGroupReg) | u32IRQNum); 

	outp32(u32IRQIntEnableReg, inp32(u32IRQIntEnableReg) & (~u32IRQEnMask)); 
	outp32(u32IRQIntEnableReg, inp32(u32IRQIntEnableReg) | (mode << (pin->pin))); 

	if(pin->port == GPIO_PORTA){
		sysInstallISR(IRQ_LEVEL_7,IRQ_EXTINT0, (PVOID)EXTINT0_IRQHandler);
		sysEnableInterrupt(IRQ_EXTINT0);
	}

	if(pin->port == GPIO_PORTB){
		sysInstallISR(IRQ_LEVEL_7,IRQ_EXTINT0, (PVOID)EXTINT0_IRQHandler);
		sysEnableInterrupt(IRQ_EXTINT0);
	}

	if(pin->port == GPIO_PORTC){
		sysInstallISR(IRQ_LEVEL_7,IRQ_EXTINT1, (PVOID)EXTINT1_IRQHandler);
		sysEnableInterrupt(IRQ_EXTINT1);
	}

	if(pin->port == GPIO_PORTD){
		sysInstallISR(IRQ_LEVEL_7,IRQ_EXTINT1, (PVOID)EXTINT1_IRQHandler);
		sysEnableInterrupt(IRQ_EXTINT1);
	}

	if(pin->port == GPIO_PORTE){
		sysInstallISR(IRQ_LEVEL_7,IRQ_EXTINT2, (PVOID)EXTINT2_IRQHandler);
		sysEnableInterrupt(IRQ_EXTINT2);
	}

	if(pin->port == GPIO_PORTG){
		sysInstallISR(IRQ_LEVEL_7,IRQ_EXTINT2, (PVOID)EXTINT2_IRQHandler);
		sysEnableInterrupt(IRQ_EXTINT2);
	}

	if(pin->port == GPIO_PORTH){
		sysInstallISR(IRQ_LEVEL_7,IRQ_EXTINT3, (PVOID)EXTINT3_IRQHandler);
		sysEnableInterrupt(IRQ_EXTINT3);
	}

	//Enable IRQ
	CPSR_EnableIF(ARM_CPSR_I_BIT);
}

STATIC void gpioint_disable(const pin_obj_t *pin){
	uint32_t u32IRQIntEnableReg;
	uint32_t u32IRQEnMask = 0x00010001 << (pin->pin);

	if(pin->port == GPIO_PORTA){
		u32IRQIntEnableReg = REG_IRQENGPA;
	}

	if(pin->port == GPIO_PORTB){
		u32IRQIntEnableReg = REG_IRQENGPB;
	}

	if(pin->port == GPIO_PORTC){
		u32IRQIntEnableReg = REG_IRQENGPC;
	}

	if(pin->port == GPIO_PORTD){
		u32IRQIntEnableReg = REG_IRQENGPD;
	}

	if(pin->port == GPIO_PORTE){
		u32IRQIntEnableReg = REG_IRQENGPE;
	}

	if(pin->port == GPIO_PORTG){
		u32IRQIntEnableReg = REG_IRQENGPG;
	}

	if(pin->port == GPIO_PORTH){
		u32IRQIntEnableReg = REG_IRQENGPH;
	}

	//disalbe IRQ
	CPSR_DisableIF(ARM_CPSR_I_BIT);

	//Clear IntEnable falling and rising edge flag
	outp32(u32IRQIntEnableReg, inp32(u32IRQIntEnableReg) & (~u32IRQEnMask)); 
	
	//Enable IRQ
	CPSR_EnableIF(ARM_CPSR_I_BIT);
}

// This function is intended to be used by the Pin.irq() method
void extint_register_pin(const pin_obj_t *pin, uint32_t mode, bool hard_irq, mp_obj_t callback_obj) {
    uint32_t line = pin->pin;

    // Check if the ExtInt line is already in use by another Pin/ExtInt
    mp_obj_t *cb = &MP_STATE_PORT(pyb_extint_callback)[line];
	if (*cb != mp_const_none && MP_OBJ_FROM_PTR(pin) != pyb_extint_callback_arg[line]) {
        if (MP_OBJ_IS_SMALL_INT(pyb_extint_callback_arg[line])) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError,
                "ExtInt vector %d is already in use", line));
        } else {
            const pin_obj_t *other_pin = (const pin_obj_t*)pyb_extint_callback_arg[line];
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError,
                "IRQ resource already taken by Pin('%q')", other_pin->name));
        }
	}

	gpioint_disable(pin);
	
	*cb = callback_obj;

	if (*cb != mp_const_none) {
		pyb_extint_callback_arg[line] = MP_OBJ_FROM_PTR(pin);

		gpioint_enable(pin, mode);
	}

}

void extint_init0(void) {
    for (int i = 0; i < PYB_EXTI_NUM_VECTORS; i++) {
        MP_STATE_PORT(pyb_extint_callback)[i] = mp_const_none;
   }
}

void Handle_GPIO_Irq(uint32_t line) {

	if (line < EXTI_NUM_VECTORS) {
		 mp_obj_t *cb = &MP_STATE_PORT(pyb_extint_callback)[line];

#if  MICROPY_PY_THREAD
		mp_sched_lock();
#else
		// When executing code within a handler we must lock the GC to prevent
		// any memory allocations.  We must also catch any exceptions.
		gc_lock();
#endif
		nlr_buf_t nlr;
		if (nlr_push(&nlr) == 0) {
			mp_call_function_1(*cb, pyb_extint_callback_arg[line]);
			nlr_pop();
		} else {
			// Uncaught exception; disable the callback so it doesn't run again.
//			*cb = mp_const_none;
//			extint_disable(line);
			printf("Uncaught exception in ExtInt interrupt handler line %u\n", (unsigned int)line);
			mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
		}
#if  MICROPY_PY_THREAD
		mp_sched_unlock(); //TODO: enable it for support mp_thread
#else
		gc_unlock();
#endif	 
	}
}

