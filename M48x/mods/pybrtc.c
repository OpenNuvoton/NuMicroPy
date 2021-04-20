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

#include "py/runtime.h"

#include "rtc.h"
#include "pybrtc.h"

/******************************************************************************/
// MicroPython bindings

typedef struct _pyb_rtc_obj_t {
    mp_obj_base_t base;
    bool initialized;
    int32_t i32Calibration;
    mp_obj_t callback;
} pyb_rtc_obj_t;

STATIC pyb_rtc_obj_t pyb_rtc_obj = {{&pyb_rtc_type}, false, 0, mp_const_none};

STATIC mp_obj_t pyb_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

	CLK->PWRCTL |= CLK_PWRCTL_LXTEN_Msk; // 32K (LXT) Enabled    
	CLK->APBCLK0 |= CLK_APBCLK0_RTCCKEN_Msk; // RTC Clock Enable

    // return constant object
    return (mp_obj_t)&pyb_rtc_obj;
}


mp_obj_t pyb_rtc_datetime(size_t n_args, const mp_obj_t *args) {
	
	pyb_rtc_obj_t *self = MP_OBJ_TO_PTR(args[0]);

	S_RTC_TIME_DATA_T sRTCTimeData;
    if (n_args == 1) {
		RTC_GetDateAndTime(&sRTCTimeData);
		mp_obj_t tuple[8];

		tuple[0] = mp_obj_new_int(sRTCTimeData.u32Year);
        tuple[1] = mp_obj_new_int(sRTCTimeData.u32Month);
        tuple[2] =    mp_obj_new_int(sRTCTimeData.u32Day);
		if(sRTCTimeData.u32DayOfWeek == RTC_SUNDAY)
			tuple[3] = mp_obj_new_int(7);
		else
			tuple[3] = mp_obj_new_int(sRTCTimeData.u32DayOfWeek);
        tuple[4] = mp_obj_new_int(sRTCTimeData.u32Hour);
        tuple[5] = mp_obj_new_int(sRTCTimeData.u32Minute);
        tuple[6] = mp_obj_new_int(sRTCTimeData.u32Second);
        tuple[7] = mp_obj_new_int(0);
	     
	    return mp_obj_new_tuple(8, tuple);
	}
	else{
        // set date and time
        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[1], 8, &items);
		uint32_t u32DayOfWeek;

		sRTCTimeData.u32Year = mp_obj_get_int(items[0]);
		sRTCTimeData.u32Month = mp_obj_get_int(items[1]);
		sRTCTimeData.u32Day = mp_obj_get_int(items[2]);

		u32DayOfWeek = mp_obj_get_int(items[3]);
		if(u32DayOfWeek == 7)
			sRTCTimeData.u32DayOfWeek = RTC_SUNDAY;
		else
			sRTCTimeData.u32DayOfWeek = u32DayOfWeek;
		
		sRTCTimeData.u32Hour = mp_obj_get_int(items[4]);
		sRTCTimeData.u32Minute = mp_obj_get_int(items[5]);
		sRTCTimeData.u32Second = mp_obj_get_int(items[6]);
		sRTCTimeData.u32TimeScale = RTC_CLOCK_24;

		if(self->initialized == false){
			RTC_Open(&sRTCTimeData);
			self->initialized = true;
		}
		else{
			RTC_SetDateAndTime(&sRTCTimeData);
		}
		
        return mp_const_none;
	}
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_rtc_datetime_obj, 1, 2, pyb_rtc_datetime);

mp_obj_t pyb_rtc_calibration(size_t n_args, const mp_obj_t *args) {
    mp_int_t cal;
    double real_clock;
	pyb_rtc_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (n_args == 2) {
        cal = mp_obj_get_int(args[1]);
        if (cal < -511 || cal > 512) {
            mp_raise_ValueError("calibration value out of range");
		}
		
		real_clock = (((cal * 0.954) / 1000000) * 32768) + 32768;
		real_clock = real_clock * 10000;
		
		RTC_32KCalibration((int32_t)real_clock);
		self->i32Calibration = cal;
		return mp_const_none;
	}
	else{
		return mp_obj_new_int(self->i32Calibration);
	}
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_rtc_calibration_obj, 1, 2, pyb_rtc_calibration);


// wakeup(None)
// wakeup(ms, callback=None)
mp_obj_t pyb_rtc_wakeup(size_t n_args, const mp_obj_t *args) {

	pyb_rtc_obj_t *self = MP_OBJ_TO_PTR(args[0]);

	if (args[1] == mp_const_none) {
		// disable wakeup
		RTC_WaitAccessEnable();
		RTC->TICK = (RTC->TICK & ~RTC_TICK_TICK_Msk);

		/* Disable RTC Tick interrupt */
		RTC_DisableInt(RTC_INTEN_TICKIEN_Msk);
		self->callback = mp_const_none;
	}
	else {
		uint32_t u32WakeupTime = mp_obj_get_int(args[1]);

		if(u32WakeupTime > RTC_TICK_1_128_SEC)
			u32WakeupTime = RTC_TICK_1_128_SEC;
		
        if (n_args == 3) {
            self->callback = args[2];
        }

		if(self->initialized == false){
			RTC_Open(NULL);
			self->initialized = true;
		}

		RTC_SetTickPeriod(u32WakeupTime);
		/* Enable RTC Tick interrupt */
		RTC_EnableInt(RTC_INTEN_TICKIEN_Msk);

		if(self->callback != mp_const_none){
			/* Enable RTC NVIC */
			NVIC_EnableIRQ(RTC_IRQn);
		}
	}

	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_rtc_wakeup_obj, 2, 3, pyb_rtc_wakeup);


STATIC const mp_rom_map_elem_t pyb_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&pyb_rtc_datetime_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibration), MP_ROM_PTR(&pyb_rtc_calibration_obj) },
	{ MP_ROM_QSTR(MP_QSTR_wakeup), MP_ROM_PTR(&pyb_rtc_wakeup_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_WAKEUP_1_SEC), MP_ROM_INT(RTC_TICK_1_SEC) },
    { MP_ROM_QSTR(MP_QSTR_WAKEUP_1_2_SEC), MP_ROM_INT(RTC_TICK_1_2_SEC) },
    { MP_ROM_QSTR(MP_QSTR_WAKEUP_1_4_SEC), MP_ROM_INT(RTC_TICK_1_4_SEC) },
    { MP_ROM_QSTR(MP_QSTR_WAKEUP_1_8_SEC), MP_ROM_INT(RTC_TICK_1_8_SEC) },
    { MP_ROM_QSTR(MP_QSTR_WAKEUP_1_16_SEC), MP_ROM_INT(RTC_TICK_1_16_SEC) },
    { MP_ROM_QSTR(MP_QSTR_WAKEUP_1_32_SEC), MP_ROM_INT(RTC_TICK_1_32_SEC) },
    { MP_ROM_QSTR(MP_QSTR_WAKEUP_1_64_SEC), MP_ROM_INT(RTC_TICK_1_64_SEC) },
    { MP_ROM_QSTR(MP_QSTR_WAKEUP_1_128_SEC), MP_ROM_INT(RTC_TICK_1_128_SEC) },

#if 0
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&pyb_rtc_info_obj) },
#endif
};
STATIC MP_DEFINE_CONST_DICT(pyb_rtc_locals_dict, pyb_rtc_locals_dict_table);


const mp_obj_type_t pyb_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .make_new = pyb_rtc_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_rtc_locals_dict,
};


STATIC void rtc_handle_tick_irq(pyb_rtc_obj_t *self, mp_obj_t callback) {

	// execute callback if it's set
	if (callback != mp_const_none) {
#if  MICROPY_PY_THREAD
		mp_sched_lock();
#else
		// When executing code within a handler we must lock the GC to prevent
		// any memory allocations.  We must also catch any exceptions.
		gc_lock();
#endif
		nlr_buf_t nlr;
		if (nlr_push(&nlr) == 0) {
			mp_call_function_0(callback);
			nlr_pop();
		} else {
			// Uncaught exception; disable the callback so it doesn't run again.
			self->callback = mp_const_none;
			RTC_DisableInt(RTC_INTEN_TICKIEN_Msk);
			printf("uncaught exception in RTC tick interrupt handler\n");
			mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
		}
#if  MICROPY_PY_THREAD
		mp_sched_unlock();
#else
		gc_unlock();
#endif
	}
}

/**
 * @brief       IRQ Handler for RTC Interrupt
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The RTC_IRQHandler is default IRQ of RTC, declared in startup_M2351.s.
 */
void RTC_IRQHandler(void)
{
    /* To check if RTC alarm interrupt occurred */
    if(RTC_GET_TICK_INT_FLAG() == 1)
    {
        /* Clear RTC alarm interrupt flag */
        RTC_CLEAR_TICK_INT_FLAG();
		rtc_handle_tick_irq(&pyb_rtc_obj, pyb_rtc_obj.callback);
    }
}
