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
} pyb_rtc_obj_t;

STATIC pyb_rtc_obj_t pyb_rtc_obj = {{&pyb_rtc_type}, false, 0};

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

STATIC const mp_rom_map_elem_t pyb_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&pyb_rtc_datetime_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibration), MP_ROM_PTR(&pyb_rtc_calibration_obj) },
#if 0
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&pyb_rtc_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_wakeup), MP_ROM_PTR(&pyb_rtc_wakeup_obj) },
#endif
};
STATIC MP_DEFINE_CONST_DICT(pyb_rtc_locals_dict, pyb_rtc_locals_dict_table);


const mp_obj_type_t pyb_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .make_new = pyb_rtc_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_rtc_locals_dict,
};
