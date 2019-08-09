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
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"

#include "pybpin.h"
#include "pybled.h"


#if defined(MICROPY_HW_HAS_LED)

#define M26X_MAX_LED_INST 3

// the default is that LEDs are not inverted, and pin driven high turns them on
#ifndef MICROPY_HW_LED_INVERTED
#define MICROPY_HW_LED_INVERTED (1)
#endif

/******************************************************************************/
/* MicroPython bindings                                                       */

typedef struct _pyb_led_obj_t {
    mp_obj_base_t base;
    mp_uint_t led_id;
    const pin_obj_t *led_pin;
} pyb_led_obj_t;

STATIC const pyb_led_obj_t pyb_led_obj[M26X_MAX_LED_INST] = {
#if defined(MICROPY_HW_LED0)
    {{&pyb_led_type}, 0, MICROPY_HW_LED0},
#else
    {{&pyb_led_type}, 0, NULL},
#endif
#if defined(MICROPY_HW_LED1)
    {{&pyb_led_type}, 1, MICROPY_HW_LED1},
#else
    {{&pyb_led_type}, 1, NULL},
#endif
#if defined(MICROPY_HW_LED2)
    {{&pyb_led_type}, 2, MICROPY_HW_LED2},
#else
    {{&pyb_led_type}, 2, NULL},
#endif
};


static void led_state(pyb_led_t led, int state) {
    if (led < 0 || led >= M26X_MAX_LED_INST) {
        return;
    }

    const pin_obj_t *led_pin = pyb_led_obj[led].led_pin;
	if(led_pin == NULL)
		return;

    //printf("led_state(%d,%d)\n", led, state);
    if (state == 0) {
        // turn LED off
        MICROPY_HW_LED_OFF(led_pin);
    } else {
        // turn LED on
        MICROPY_HW_LED_ON(led_pin);
    }
}

static void led_toggle(pyb_led_t led) {
    if (led < 0 || led >= M26X_MAX_LED_INST) {
        return;
    }

    const pin_obj_t *led_pin = pyb_led_obj[led].led_pin;

	if(mp_hal_pin_read(led_pin)){
        // turn LED on
		if(MICROPY_HW_LED_INVERTED)
			MICROPY_HW_LED_ON(led_pin);
		else
			MICROPY_HW_LED_OFF(led_pin);
	}
	else{
        // turn LED on
		if(MICROPY_HW_LED_INVERTED)
			MICROPY_HW_LED_OFF(led_pin);
		else
			MICROPY_HW_LED_ON(led_pin);
	}
}

static int led_get_intensity(pyb_led_t led) {
    if (led < 0 || led >= M26X_MAX_LED_INST) {
        return 0;
    }


    const pin_obj_t *led_pin = pyb_led_obj[led].led_pin;

    if (mp_hal_pin_read(led_pin)) {
        // pin is high
        return MICROPY_HW_LED_INVERTED ? 0 : 255;
    } else {
        // pin is low
        return MICROPY_HW_LED_INVERTED ? 255 : 0;
    }
}

static void led_set_intensity(pyb_led_t led, mp_int_t intensity) {
    // intensity not supported for this LED; just turn it on/off
    led_state(led, intensity > 0);
}


/// \method on()
/// Turn the LED on.
mp_obj_t led_obj_on(mp_obj_t self_in) {
    pyb_led_obj_t *self = self_in;
    led_state(self->led_id, 1);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_obj_on_obj, led_obj_on);

/// \method off()
/// Turn the LED off.
mp_obj_t led_obj_off(mp_obj_t self_in) {
    pyb_led_obj_t *self = self_in;
    led_state(self->led_id, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_obj_off_obj, led_obj_off);

/// \method toggle()
/// Toggle the LED between on and off.
mp_obj_t led_obj_toggle(mp_obj_t self_in) {
    pyb_led_obj_t *self = self_in;
    led_toggle(self->led_id);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_obj_toggle_obj, led_obj_toggle);

/// \method intensity([value])
/// Get or set the LED intensity.  Intensity ranges between 0 (off) and 255 (full on).
/// If no argument is given, return the LED intensity.
/// If an argument is given, set the LED intensity and return `None`.
mp_obj_t led_obj_intensity(size_t n_args, const mp_obj_t *args) {
    pyb_led_obj_t *self = args[0];
    if (n_args == 1) {
        return mp_obj_new_int(led_get_intensity(self->led_id));
    } else {
        led_set_intensity(self->led_id, mp_obj_get_int(args[1]));
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_obj_intensity_obj, 1, 2, led_obj_intensity);

STATIC mp_obj_t led_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // work out switch
    mp_uint_t led_idx;
    if (MP_OBJ_IS_STR(args[0])) {
        const char *led_pin = mp_obj_str_get_str(args[0]);
        if (0) {
        #ifdef MICROPY_HW_LED0_NAME
        } else if (strcmp(led_pin, MICROPY_HW_LED0_NAME) == 0) {
            led_idx = 0;
        #endif
        #ifdef MICROPY_HW_LED1_NAME
        } else if (strcmp(led_pin, MICROPY_HW_LED1_NAME) == 0) {
            led_idx = 1;
        #endif			
        #ifdef MICROPY_HW_LED2_NAME
        } else if (strcmp(led_pin, MICROPY_HW_LED2_NAME) == 0) {
            led_idx = 2;
        #endif
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "LED(%s) doesn't exist", led_pin));
        }
    } else {
        led_idx = mp_obj_get_int(args[0]);
    }

    if (led_idx < 0 || led_idx >= M26X_MAX_LED_INST) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "LED(%d) doesn't exist", led_idx));
    }

	if(pyb_led_obj[led_idx].led_pin == NULL){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "LED(%d) is not suuport", led_idx));
	}

    mp_hal_pin_config(pyb_led_obj[led_idx].led_pin, MP_HAL_PIN_MODE_OUTPUT, 0);

    // return static switch object
    return (mp_obj_t)&pyb_led_obj[led_idx];
}

void led_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_led_obj_t *self = self_in;
    mp_printf(print, "LED(%u)", self->led_id);
}

STATIC const mp_rom_map_elem_t led_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&led_obj_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&led_obj_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&led_obj_toggle_obj) },
    { MP_ROM_QSTR(MP_QSTR_intensity), MP_ROM_PTR(&led_obj_intensity_obj) },
};

STATIC MP_DEFINE_CONST_DICT(led_locals_dict, led_locals_dict_table);

const mp_obj_type_t pyb_led_type = {
    { &mp_type_type },
    .name = MP_QSTR_LED,
    .print = led_obj_print,
    .make_new = led_obj_make_new,
    .locals_dict = (mp_obj_dict_t*)&led_locals_dict,
};

#endif
