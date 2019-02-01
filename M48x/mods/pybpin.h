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
#ifndef MICROPY_INCLUDED_M48X_PIN_H
#define MICROPY_INCLUDED_M48X_PIN_H

// This file requires pin_defs_xxx.h (which has port specific enums and
// defines, so we include it here. It should never be included directly
#include "py/obj.h"
//#include "mpconfigboard_common.h"
#include MICROPY_PIN_DEFS_PORT_H

typedef struct {
  mp_obj_base_t base;
  qstr name;
  uint8_t idx;
  uint8_t fn;
  uint8_t unit;
  uint8_t type;
  void *reg; // The peripheral associated with this AF
  uint32_t mfp_val;
} pin_af_obj_t;

typedef struct {
  mp_obj_base_t base;
  qstr name;
  uint32_t port   : 4;
  uint32_t pin    : 5;      // Some ARM processors use 32 bits/PORT
  uint32_t num_af : 4;
  uint32_t adc_channel : 5; // Some ARM processors use 32 bits/PORT
  uint32_t adc_num  : 3;    // 1 bit per ADC
  uint32_t pin_mask;
  volatile uint32_t *mfp_reg;
  volatile uint32_t *mfos_reg;  
  pin_gpio_t *gpio;
  const pin_af_obj_t *af;
} pin_obj_t;

extern const mp_obj_type_t pin_type;
extern const mp_obj_type_t pin_af_type;

// Include all of the individual pin objects
#include "genhdr/pins.h"

extern const mp_obj_type_t pin_board_pins_obj_type;
extern const mp_obj_type_t pin_cpu_pins_obj_type;


extern const mp_obj_dict_t pin_cpu_pins_locals_dict;
extern const mp_obj_dict_t pin_board_pins_locals_dict;


extern uint32_t pin_get_mode(const pin_obj_t *pin);
extern uint32_t pin_get_pull(const pin_obj_t *pin);
extern uint32_t pin_get_af(const pin_obj_t *pin);
extern void pin_set_af(const pin_obj_t *pin, const pin_af_obj_t *af_obj, int mode);

extern const pin_obj_t *pin_find(mp_obj_t user_obj);
extern const pin_obj_t *pin_find_named_pin(const mp_obj_dict_t *named_pins, mp_obj_t name);
extern const pin_af_obj_t *pin_find_af_by_mfp_value(const pin_obj_t *pin, mp_uint_t af_value);
extern const pin_af_obj_t *pin_find_af(const pin_obj_t *pin, uint8_t fn, uint8_t unit);
extern const pin_af_obj_t *pin_find_af_by_index(const pin_obj_t *pin, mp_uint_t af_idx);

extern void pin_init0(void);

MP_DECLARE_CONST_FUN_OBJ_KW(pin_init_obj);

#endif

