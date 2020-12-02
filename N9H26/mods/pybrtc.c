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

#include "wblib.h"
#include "N9H26_RTC.h"

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

    outp32(REG_APBCLK,inp32(REG_APBCLK) | RTC_CKE);

	if(pyb_rtc_obj.initialized == false){
		RTC_TIME_DATA_T sInitTime;

		RTC_Init();

		/* Time Setting */
		sInitTime.u32Year = 2010;
		sInitTime.u32cMonth = 11;
		sInitTime.u32cDay = 25;
		sInitTime.u32cHour = 13;
		sInitTime.u32cMinute = 20;
		sInitTime.u32cSecond = 0;
		sInitTime.u32cDayOfWeek = RTC_FRIDAY;
		sInitTime.u8cClockDisplay = RTC_CLOCK_24;

		/* Initialization the RTC timer */
		if(RTC_Open(&sInitTime) !=E_RTC_SUCCESS)
			mp_raise_ValueError("unable open RTC");
		pyb_rtc_obj.initialized = true;
	}

    // return constant object
    return (mp_obj_t)&pyb_rtc_obj;
}

mp_obj_t pyb_rtc_datetime(size_t n_args, const mp_obj_t *args) {
	//pyb_rtc_obj_t *self = MP_OBJ_TO_PTR(args[0]);

	RTC_TIME_DATA_T sRTCTimeData;
    if (n_args == 1) {
	    RTC_Read(RTC_CURRENT_TIME, &sRTCTimeData);
		mp_obj_t tuple[8];

		tuple[0] = mp_obj_new_int(sRTCTimeData.u32Year);
        tuple[1] = mp_obj_new_int(sRTCTimeData.u32cMonth);
        tuple[2] =    mp_obj_new_int(sRTCTimeData.u32cDay);
		if(sRTCTimeData.u32cDayOfWeek == RTC_SUNDAY)
			tuple[3] = mp_obj_new_int(7);
		else
			tuple[3] = mp_obj_new_int(sRTCTimeData.u32cDayOfWeek);
        tuple[4] = mp_obj_new_int(sRTCTimeData.u32cHour);
        tuple[5] = mp_obj_new_int(sRTCTimeData.u32cMinute);
        tuple[6] = mp_obj_new_int(sRTCTimeData.u32cSecond);
        tuple[7] = mp_obj_new_int(0);
	     
	    return mp_obj_new_tuple(8, tuple);
	}
	else{
        // set date and time
        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[1], 8, &items);
		uint32_t u32DayOfWeek;

		sRTCTimeData.u32Year = mp_obj_get_int(items[0]);
		sRTCTimeData.u32cMonth = mp_obj_get_int(items[1]);
		sRTCTimeData.u32cDay = mp_obj_get_int(items[2]);

		u32DayOfWeek = mp_obj_get_int(items[3]);
		if(u32DayOfWeek == 7)
			sRTCTimeData.u32cDayOfWeek = RTC_SUNDAY;
		else
			sRTCTimeData.u32cDayOfWeek = u32DayOfWeek;
		
		sRTCTimeData.u32cHour = mp_obj_get_int(items[4]);
		sRTCTimeData.u32cMinute = mp_obj_get_int(items[5]);
		sRTCTimeData.u32cSecond = mp_obj_get_int(items[6]);
		sRTCTimeData.u8cClockDisplay = RTC_CLOCK_24;

		RTC_WriteEnable(TRUE);
		RTC_Write(RTC_CURRENT_TIME, &sRTCTimeData);
		RTC_WriteEnable(FALSE);
		
        return mp_const_none;
	}
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_rtc_datetime_obj, 1, 2, pyb_rtc_datetime);

mp_obj_t pyb_rtc_calibration(mp_obj_t self_in) {
	
	RTC_DoFrequencyCompensation();
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pyb_rtc_calibration_obj, pyb_rtc_calibration);

STATIC const mp_rom_map_elem_t pyb_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&pyb_rtc_datetime_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibration), MP_ROM_PTR(&pyb_rtc_calibration_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_rtc_locals_dict, pyb_rtc_locals_dict_table);


const mp_obj_type_t pyb_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .make_new = pyb_rtc_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_rtc_locals_dict,
};
