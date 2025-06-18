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
#include "mods/classPin.h"

#include "mpconfigboard_common.h"


#define EXTI_NUM_VECTORS        (PYB_EXTI_NUM_VECTORS)

static mp_obj_t pyb_extint_callback_arg[EXTI_NUM_VECTORS];

static void gpioint_enable(const pin_obj_t *pin, uint32_t mode)
{
    mp_uint_t irq_state = disable_irq();

    GPIO_EnableInt(pin->gpio, pin->pin, mode);

    //Configure NVIC
    if(pin->gpio == PA)
        NVIC_EnableIRQ(GPA_IRQn);
    else if (pin->gpio == PB)
        NVIC_EnableIRQ(GPB_IRQn);
    else if (pin->gpio == PC)
        NVIC_EnableIRQ(GPC_IRQn);
    else if (pin->gpio == PD)
        NVIC_EnableIRQ(GPD_IRQn);
    else if (pin->gpio == PE)
        NVIC_EnableIRQ(GPE_IRQn);
    else if (pin->gpio == PF)
        NVIC_EnableIRQ(GPF_IRQn);
    else if (pin->gpio == PG)
        NVIC_EnableIRQ(GPG_IRQn);
    else if (pin->gpio == PH)
        NVIC_EnableIRQ(GPH_IRQn);
    else if (pin->gpio == PI)
        NVIC_EnableIRQ(GPI_IRQn);
    else if (pin->gpio == PJ)
        NVIC_EnableIRQ(GPJ_IRQn);

    enable_irq(irq_state);
}


static void gpioint_disable(const pin_obj_t *pin)
{
    mp_uint_t irq_state = disable_irq();
    GPIO_DisableInt(pin->gpio, pin->pin);
    enable_irq(irq_state);
}


// This function is intended to be used by the Pin.irq() method
void extint_register_pin(const pin_obj_t *pin, uint32_t mode, bool hard_irq, mp_obj_t callback_obj)
{
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

//		gpioint_enable(pin, mode);
    }
    gpioint_enable(pin, mode);

}


void extint_init0(void)
{
#if 0
    for (int i = 0; i < PYB_EXTI_NUM_PORTS; i++) {
        for (int j = 0; j < PYB_EXTI_NUM_VECTORS; j++) {
            MP_STATE_PORT(pyb_extint_callback)[i][j] = mp_const_none;
        }
    }
#else
    for (int j = 0; j < PYB_EXTI_NUM_VECTORS; j++) {
        MP_STATE_PORT(pyb_extint_callback)[j] = mp_const_none;
    }
#endif
}

void Handle_GPIO_Irq(uint32_t line)
{

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

