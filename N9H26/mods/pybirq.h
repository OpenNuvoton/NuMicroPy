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
#ifndef MICROPY_INCLUDED_N9H26_IRQ_H
#define MICROPY_INCLUDED_N9H26_IRQ_H

// these states correspond to values from query_irq, enable_irq and disable_irq
#define IRQ_STATE_DISABLED (0x00000001)
#define IRQ_STATE_ENABLED  (0x00000000)

// Enable this to get a count for the number of times each irq handler is called,
// accessible via pyb.irq_stats().
#define IRQ_ENABLE_STATS (0)

#if IRQ_ENABLE_STATS
extern uint32_t irq_stats[IRQ_TMR3 + 1];
#define IRQ_ENTER(irq) ++irq_stats[irq]
#define IRQ_EXIT(irq)
#else
#define IRQ_ENTER(irq)
#define IRQ_EXIT(irq)
#endif

MP_DECLARE_CONST_FUN_OBJ_0(pyb_disable_irq_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_enable_irq_obj);
MP_DECLARE_CONST_FUN_OBJ_0(pyb_irq_stats_obj);

#endif
