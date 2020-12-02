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

#include "py/runtime.h"
#include "py/mphal.h"

#if MICROPY_PY_THREAD
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif

#include "wblib.h"

#include "N9H26_CPSR.h"

// Core delay function that does an efficient sleep and may switch thread context.
// If IRQs are enabled then we must have the GIL.
void mp_hal_delay_ms(mp_uint_t Delay) {

    if ((CPSR_GetMode() == ARM_CPSR_FIQ_MODE) || (CPSR_GetMode() == ARM_CPSR_IRQ_MODE)){
		//In IRQ/FIQ mode, using clock(busy) delay
        const uint32_t count_1ms = sysGetCPUClock() / 4000;
        for (int i = 0; i < Delay; i++) {
            for (uint32_t count = 0; ++count <= count_1ms;) {
            }
        }
	}
	else{
#if MICROPY_PY_THREAD
		vTaskDelay (Delay / portTICK_PERIOD_MS);
#else
        uint32_t start = sysGetTicks(TIMER1);
        // Wraparound of tick is taken care of by 2's complement arithmetic.
        while (sysGetTicks(TIMER1) - start < Delay) {
            // This macro will execute the necessary idle behaviour.  It may
            // raise an exception, switch threads or enter sleep mode (waiting for
            // (at least) the SysTick interrupt).
            MICROPY_EVENT_POLL_HOOK
        }
#endif

	}
}

// delay for given number of microseconds
void mp_hal_delay_us(mp_uint_t usec) {

    if ((CPSR_GetMode() == ARM_CPSR_FIQ_MODE) || (CPSR_GetMode() == ARM_CPSR_IRQ_MODE)){
		//In IRQ/FIQ mode, using clock(busy) delay
        // sys freq is always a multiple of 2MHz, so division here won't lose precision
        const uint32_t ucount = sysGetCPUClock() / 2000000 * usec / 2;
        for (uint32_t count = 0; ++count <= ucount;) {
        }
	}
	else{
       uint32_t start = mp_hal_ticks_us(); 
       while (mp_hal_ticks_us() - start < usec) {
	   }
 	}
}

uint32_t s_u32SysTickCount = 0;

void mp_hal_tick_inc(uint32_t tick_count)
{
	s_u32SysTickCount += tick_count;
}

mp_uint_t mp_hal_ticks_ms(void) {
#if MICROPY_PY_THREAD
	return s_u32SysTickCount;
#else
    return sysGetTicks(TIMER1);
#endif
}

mp_uint_t mp_hal_ticks_us(void) {
	mp_uint_t irq_state = CPSR_GetIFState();
	CPSR_DisableIF(irq_state);
	uint32_t counter = inp32(REG_TDR1);
	uint32_t milliseconds = mp_hal_ticks_ms();
	uint32_t status  = inp32(REG_TISR);
	CPSR_EnableIF(irq_state);

    if ((status & 0x2) && counter > 50) { //0x2: timer1 interrupt flag
        // This means that the HW reloaded between the time we read counter and the
        // time we read status, which implies that there is an interrupt pending
        // to increment the tick counter.
        milliseconds++;
    }

    uint32_t load = inp32(REG_TICR1);
    //counter = load - counter; //for decrementing counter

    // ((load + 1) / 1000) is the number of counts per microsecond.
    //
    // counter / ((load + 1) / 1000) scales from the systick clock to microseconds
    // and is the same thing as (counter * 1000) / (load + 1)
    return milliseconds * 1000 + (counter * 1000) / (load + 1);
}





