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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mphal.h"

#include "pybwdt.h" 

typedef struct _pyb_wdt_obj_t {
    mp_obj_base_t base;
} pyb_wdt_obj_t;

STATIC const pyb_wdt_obj_t pyb_wdt = {{&pyb_wdt_type}};

STATIC uint32_t find_MSB(
	uint32_t u32Value
)
{
	int i;
	for(i = 31 ; i > 0 ; i--){
		if(u32Value & (0x1 << i))
			break;
	}
	return i;
}


STATIC mp_obj_t pyb_wdt_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse arguments
    enum { ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 5000} },	//in milli
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	uint32_t u32Ticks;
	uint32_t MSBBit;
	uint32_t u32TimeOutInterval = WDT_TIMEOUT_2POW4;
	//use LIRC (10kHz)for WDT clock
    CLK_SetModuleClock(WDT_MODULE, CLK_CLKSEL1_WDTSEL_LIRC, 0);	

	u32Ticks = args[ARG_timeout].u_int * 10;
	MSBBit = find_MSB(u32Ticks);

	if(MSBBit > 16)
		u32TimeOutInterval = WDT_TIMEOUT_2POW18;
	else if(MSBBit > 14)
		u32TimeOutInterval = WDT_TIMEOUT_2POW16;
	else if(MSBBit > 12)
		u32TimeOutInterval = WDT_TIMEOUT_2POW14;
	else if(MSBBit > 10)
		u32TimeOutInterval = WDT_TIMEOUT_2POW12;
	else if(MSBBit > 8)
		u32TimeOutInterval = WDT_TIMEOUT_2POW10;
	else if(MSBBit > 6)
		u32TimeOutInterval = WDT_TIMEOUT_2POW8;
	else if(MSBBit > 4)
		u32TimeOutInterval = WDT_TIMEOUT_2POW6;

	WDT_Open(u32TimeOutInterval, WDT_RESET_DELAY_18CLK, TRUE, TRUE);

	return MP_OBJ_FROM_PTR(&pyb_wdt);
}

STATIC mp_obj_t pyb_wdt_feed(mp_obj_t self_in) {
    WDT_RESET_COUNTER();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_wdt_feed_obj, pyb_wdt_feed);

STATIC const mp_rom_map_elem_t pyb_wdt_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_feed), MP_ROM_PTR(&pyb_wdt_feed_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_wdt_locals_dict, pyb_wdt_locals_dict_table);

const mp_obj_type_t pyb_wdt_type = {
	{ &mp_type_type },
	.name = MP_QSTR_WDT,
	.make_new = pyb_wdt_make_new,
	.locals_dict = (mp_obj_dict_t*)&pyb_wdt_locals_dict,
};

