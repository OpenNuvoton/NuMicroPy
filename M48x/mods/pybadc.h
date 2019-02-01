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
#ifndef MICROPY_INCLUDED_M48X_EADC_H
#define MICROPY_INCLUDED_M48X_EADC_H

#include "hal/M48x_EADC.h"

typedef struct _pyb_eadc_channel_obj {
	const pin_obj_t* eadc_channel_pin_obj;
} eadc_channel_obj_t;

typedef struct _pyb_adc_all_obj_t {
    mp_obj_base_t base;
    //ADC_HandleTypeDef handle;
    EADC_T *eadc_base;
    //eadc_channel_obj_t eadc_channel_obj[16+3]; //total channels
} pyb_adc_all_obj_t;

extern const mp_obj_type_t pyb_adc_type;
extern const mp_obj_type_t pyb_adc_all_type;

#endif // MICROPY_HW_ENABLE_HW_ADC
