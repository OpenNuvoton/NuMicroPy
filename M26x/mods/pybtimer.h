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
#ifndef MICROPY_INCLUDED_M26X_TIMER_H
#define MICROPY_INCLUDED_M26X_TIMER_H

extern const mp_obj_type_t pyb_timer_type;

MP_DECLARE_CONST_FUN_OBJ_KW(pyb_timer_init_obj);
MP_DECLARE_CONST_FUN_OBJ_1(pyb_timer_deinit_obj);

typedef enum{
	eTMR_INT_INT,
	eTMR_INT_EINT,
	eTMR_INT_PWMINT
}E_TMR_INT_TYPE;

__STATIC_INLINE uint32_t TIMER_GetPWMIntFlag(TIMER_T *timer)
{
    return (timer->PWMINTSTS0);
}

__STATIC_INLINE void TIMER_ClearPWMIntFlag(TIMER_T *timer)
{
    timer->PWMINTSTS0 = TIMER_PWMINTSTS0_ZIF_Msk | TIMER_PWMINTSTS0_PIF_Msk | TIMER_PWMINTSTS0_CMPUIF_Msk | TIMER_PWMINTSTS0_CMPDIF_Msk ;
}

extern void Handle_TMR_Irq(int32_t i32TmrID ,E_TMR_INT_TYPE eINTType, uint32_t u32Status);

extern mp_obj_t pytimer_get_timerobj(int timer_id);
extern TIMER_T *pyb_timer_get_handle(mp_obj_t timer);

#endif
