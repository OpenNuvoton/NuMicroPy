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
#include "extmod/virtpin.h"
#include "pybpin.h"
#include "hal/pin_int.h"
#include "pybswitch.h"

#if MICROPY_HW_HAS_SWITCH

int switch_get(
	const pin_obj_t *pin_obj
) 
{
    int val = mp_hal_pin_read(pin_obj);
    return val == MICROPY_HW_USRSW_PRESSED;
}

/******************************************************************************/
// MicroPython bindings



/// \moduleref pyb
/// \class Switch - switch object
///
/// A Switch object is used to control a push-button switch.
///
/// Usage:
///
///      sw = pyb.Switch()       # create a switch object
///      sw()                    # get state (True if pressed, False otherwise)
///      sw.callback(f)          # register a callback to be called when the
///                              #   switch is pressed down
///      sw.callback(None)       # remove the callback
///
/// Example:
///
///      pyb.Switch().callback(lambda: pyb.LED(1).toggle())

#define M48X_MAX_SWITCH_INST 4

typedef struct _pyb_switch_obj_t {
	mp_obj_base_t base;
	mp_uint_t switch_id;
	const pin_obj_t *pin_obj;
	mp_obj_t callback;
} pyb_switch_obj_t;


pyb_switch_obj_t pyb_switch_obj[M48X_MAX_SWITCH_INST] = {
#if defined(MICROPY_HW_USRSW_SW0_PIN)
    {{&pyb_switch_type}, 0, MICROPY_HW_USRSW_SW0_PIN, NULL},
#else
    {{&pyb_switch_type}, 0, NULL, NULL},
#endif
#if defined(MICROPY_HW_USRSW_SW1_PIN)
    {{&pyb_switch_type}, 1, MICROPY_HW_USRSW_SW1_PIN, NULL},
#else
    {{&pyb_switch_type}, 1, NULL, NULL},
#endif
#if defined(MICROPY_HW_USRSW_SW2_PIN)
    {{&pyb_switch_type}, 2, MICROPY_HW_USRSW_SW2_PIN, NULL},
#else
    {{&pyb_switch_type}, 2, NULL, NULL},
#endif
#if defined(MICROPY_HW_USRSW_SW3_PIN)
    {{&pyb_switch_type}, 3, MICROPY_HW_USRSW_SW3_PIN, NULL},
#else
    {{&pyb_switch_type}, 3, NULL, NULL},
#endif
};



STATIC mp_obj_t pyb_switch_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // work out switch
    mp_uint_t switch_idx;
    if (MP_OBJ_IS_STR(args[0])) {
        const char *switch_pin = mp_obj_str_get_str(args[0]);
        if (0) {
        #ifdef MICROPY_HW_USRSW_SW0_NAME
        } else if (strcmp(switch_pin, MICROPY_HW_USRSW_SW0_NAME) == 0) {
            switch_idx = 0;
        #endif
        #ifdef MICROPY_HW_USRSW_SW1_NAME
        } else if (strcmp(switch_pin, MICROPY_HW_USRSW_SW1_NAME) == 0) {
            switch_idx = 1;
        #endif			
        #ifdef MICROPY_HW_USRSW_SW2_NAME
        } else if (strcmp(switch_pin, MICROPY_HW_USRSW_SW2_NAME) == 0) {
            switch_idx = 2;
        #endif
        #ifdef MICROPY_HW_USRSW_SW3_NAME
        } else if (strcmp(switch_pin, MICROPY_HW_USRSW_SW3_NAME) == 0) {
            switch_idx = 3;
        #endif
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Switch(%s) doesn't exist", switch_pin));
        }
    } else {
        switch_idx = mp_obj_get_int(args[0]);
    }

    if (switch_idx < 0 || switch_idx >= M48X_MAX_SWITCH_INST) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Switch(%d) doesn't exist", switch_idx));
    }

	if(pyb_switch_obj[switch_idx].pin_obj == NULL){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Switch(%d) is not suuport", switch_idx));
	}

    mp_hal_pin_config(pyb_switch_obj[switch_idx].pin_obj, MP_HAL_PIN_MODE_INPUT, 0);

    // return static switch object
    return (mp_obj_t)&pyb_switch_obj[switch_idx];
}

mp_obj_t pyb_switch_value(mp_obj_t self_in) {
    pyb_switch_obj_t *self = self_in;
    return mp_obj_new_bool(switch_get(self->pin_obj));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_switch_value_obj, pyb_switch_value);

STATIC mp_obj_t switch_callback(mp_obj_t line) {
	int i;

	for(i = 0 ; i < M48X_MAX_SWITCH_INST; i ++){
		if(line == MP_OBJ_FROM_PTR(pyb_switch_obj[i].pin_obj)){
			mp_call_function_0(pyb_switch_obj[i].callback);
			break;
		}
	}

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(switch_callback_obj, switch_callback);


/// \method callback(fun)
/// Register the given function to be called when the switch is pressed down.
/// If `fun` is `None`, then it disables the callback.
mp_obj_t pyb_switch_callback(mp_obj_t self_in, mp_obj_t callback) {
    pyb_switch_obj_t *self = self_in;

    self->callback = callback;
    // Init the EXTI each time this function is called, since the EXTI
    // may have been disabled by an exception in the interrupt, or the
    // user disabling the line explicitly.
    extint_register_pin(self->pin_obj,
                    MICROPY_HW_USRSW_EXTI_MODE,
                    false,
                    callback == mp_const_none ? mp_const_none : (mp_obj_t)&switch_callback_obj);
//                    callback);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_switch_callback_obj, pyb_switch_callback);


STATIC const mp_rom_map_elem_t pyb_switch_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&pyb_switch_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&pyb_switch_callback_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_switch_locals_dict, pyb_switch_locals_dict_table);

void pyb_switch_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_print_str(print, "Switch()");
}

/// \method \call()
/// Return the switch state: `True` if pressed down, `False` otherwise.
mp_obj_t pyb_switch_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // get switch state
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    pyb_switch_obj_t *self = self_in;

    return switch_get(self->pin_obj) ? mp_const_true : mp_const_false;
}

const mp_obj_type_t pyb_switch_type = {
    { &mp_type_type },
    .name = MP_QSTR_Switch,
    .print = pyb_switch_print,
    .make_new = pyb_switch_make_new,
    .call = pyb_switch_call,
    .locals_dict = (mp_obj_dict_t*)&pyb_switch_locals_dict,
};


#endif // MICROPY_HW_HAS_SWITCH


