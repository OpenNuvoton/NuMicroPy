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

#include "py/objtuple.h"
#include "py/objarray.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/binary.h"
#include "py/stream.h"
#include "py/mphal.h"

#include "bufhelper.h"
#include "classPWM.h"
#include "hal/M5531_PWM.h"

#define M5531_MAX_PWM_INST 2
#define M5531_MAX_PWM_CHANNEL_INST 6

typedef enum {
    CHANNEL_MODE_NONE,
    CHANNEL_MODE_OUTPUT,
    CHANNEL_MODE_CAPTURE,
} pyb_channel_mode;

typedef struct _pyb_pwm_channel_obj_t {
    mp_obj_base_t base;
    struct _pyb_pwm_obj_t *pyb_pwm_obj;
    pyb_channel_mode mode;
    mp_obj_t callback;
    uint32_t duty;
    int32_t chann_id;
    uint32_t capture_edge;
    pin_obj_t *pin;
    IRQn_Type irqn;
    uint32_t freq;		//only used for epwm channel
} pyb_pwm_channel_obj_t;

typedef struct _pyb_pwm_obj_t {
    mp_obj_base_t base;
    uint8_t pwm_id;
    pwm_t *pwm_obj;
    bool enabled;
    uint32_t freq;
    pyb_pwm_channel_obj_t channel[M5531_MAX_PWM_CHANNEL_INST];
} pyb_pwm_obj_t;

static const struct {
    qstr        name;
    uint32_t    oc_mode;
} channel_mode_info[] = {
    { MP_QSTR_NONE,               CHANNEL_MODE_NONE },
    { MP_QSTR_OUTPUT,	          CHANNEL_MODE_OUTPUT },
    { MP_QSTR_CAPTURE,            CHANNEL_MODE_CAPTURE},
};

static pwm_t s_sBPWM0Obj = {.u_pwm.bpwm = BPWM0, .bEPWM = false};
static pwm_t s_sBPWM1Obj = {.u_pwm.bpwm = BPWM1, .bEPWM = false};

static pwm_t s_sEPWM0Obj = {.u_pwm.epwm = EPWM0, .bEPWM = true};
static pwm_t s_sEPWM1Obj = {.u_pwm.epwm = EPWM1, .bEPWM = true};


static pyb_pwm_obj_t pyb_bpwm_obj[M5531_MAX_PWM_INST] = {
    {{&machine_pwm_type}, 0, &s_sBPWM0Obj, false},
    {{&machine_pwm_type}, 1, &s_sBPWM1Obj, false},
};

static pyb_pwm_obj_t pyb_epwm_obj[M5531_MAX_PWM_INST] = {
    {{&machine_pwm_type}, 0, &s_sEPWM0Obj, false},
    {{&machine_pwm_type}, 1, &s_sEPWM1Obj, false},
};


const mp_obj_type_t pyb_pwm_channel_type;
static void PWM_CaptureInt_Handler(void *obj, uint32_t u32ChannelGroup);

static mp_obj_t pyb_pwm_channel_cb(mp_obj_t self_in, mp_obj_t callback)
{
    pyb_pwm_channel_obj_t *self = self_in;

    if (callback == mp_const_none) {
        PWM_CaptureDisableInt(self->pyb_pwm_obj->pwm_obj, self->chann_id, eCAPTURE_RISING_FALLING_LATCH);
        self->callback = mp_const_none;
    } else if (mp_obj_is_callable(callback)) {
        PWM_CaptureDisableInt(self->pyb_pwm_obj->pwm_obj, self->chann_id, eCAPTURE_RISING_FALLING_LATCH);
        self->callback = callback;
        PWM_CaptureClearInt(self->pyb_pwm_obj->pwm_obj, self->chann_id, eCAPTURE_RISING_FALLING_LATCH);
        NVIC_EnableIRQ(self->irqn);
        PWM_CaptureEnableInt(self->pyb_pwm_obj->pwm_obj, self->chann_id, eCAPTURE_RISING_FALLING_LATCH, PWM_CaptureInt_Handler);
    } else {
        mp_raise_ValueError("callback must be None or a callabl5rtte object");
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(pyb_pwm_channel_cb_obj, pyb_pwm_channel_cb);


static mp_obj_t pyb_pwm_init_helper(pyb_pwm_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq,         MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);


    PWM_Init(self->pwm_obj);

    int i;

    //init the channel
    for(i = 0; i < M5531_MAX_PWM_CHANNEL_INST; i ++) {
        self->channel[i].base.type = &pyb_pwm_channel_type;
        self->channel[i].pyb_pwm_obj = self;
        self->channel[i].mode = CHANNEL_MODE_NONE;
        self->channel[i].callback = mp_const_none;
        self->channel[i].duty = 0;
        self->channel[i].chann_id = i;
        self->channel[i].capture_edge = eCAPTURE_RISING_LATCH;
        self->channel[i].pin = NULL;
        self->channel[i].freq = 0;

        if(self->pwm_obj->u_pwm.bpwm == BPWM0) {
            self->channel[i].irqn = BPWM0_IRQn;
        } else if(self->pwm_obj->u_pwm.bpwm == BPWM1) {
            self->channel[i].irqn = BPWM1_IRQn;
        } else if(self->pwm_obj->u_pwm.epwm == EPWM0) {
            if(i == 0 || i == 1)
                self->channel[i].irqn = EPWM0P0_IRQn;
            else if(i == 2 || i == 3)
                self->channel[i].irqn = EPWM0P1_IRQn;
            else if(i == 4 || i == 5)
                self->channel[i].irqn = EPWM0P2_IRQn;
        } else if(self->pwm_obj->u_pwm.epwm == EPWM1) {
            if(i == 0 || i == 1)
                self->channel[i].irqn = EPWM1P0_IRQn;
            else if(i == 2 || i == 3)
                self->channel[i].irqn = EPWM1P1_IRQn;
            else if(i == 4 || i == 5)
                self->channel[i].irqn = EPWM1P2_IRQn;
        }
    }

    self->freq = (uint32_t) mp_obj_get_int(args[0].u_obj);
    self->enabled = true;

    return mp_const_none;
}

static mp_obj_t pyb_pwm_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    return pyb_pwm_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pyb_pwm_init_obj, 1, pyb_pwm_init);

static mp_obj_t pyb_pwm_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the pwm id
    mp_int_t pwm_id = mp_obj_get_int(args[0]);

    mp_int_t is_epwm = 0;
    int32_t has_type = 0;
    //is_epwm = 0; BPWM (with PWM type argument)
    //is_epwm = 1; EPWM (with PWM type argument)


    // get the pwm type BPWM or EPWM
    if(MP_OBJ_IS_INT(args[1])) {
        is_epwm = mp_obj_get_int(args[1]);
        has_type = 1;
    }

    // check if the timer exists
    if (pwm_id < 0 || pwm_id >= M5531_MAX_PWM_INST) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "PWM(%d) doesn't exist", pwm_id));
    }

    pyb_pwm_obj_t *pyb_obj;

    if(is_epwm == 1) {
        pyb_obj = &pyb_epwm_obj[pwm_id];
    } else {
        pyb_obj = &pyb_bpwm_obj[pwm_id];
    }

    if(pyb_obj->enabled) {
        printf("PWM(%d) alreay enabled \n", pwm_id);
        return (mp_obj_t)pyb_obj;
    }

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);

        if(has_type)
            pyb_pwm_init_helper(pyb_obj, n_args - 2, args + 2, &kw_args);
        else
            pyb_pwm_init_helper(pyb_obj, n_args - 1, args + 1, &kw_args);
    }

    return (mp_obj_t)pyb_obj;
}

// pwm.deinit()
static mp_obj_t pyb_pwm_deinit(mp_obj_t self_in)
{
    pyb_pwm_obj_t *self = self_in;
    int i;


    PWM_Final(self->pwm_obj);


    //Disable the channel interrupts
    for(i = 0; i < M5531_MAX_PWM_CHANNEL_INST; i ++) {
        pyb_pwm_channel_cb(&self->channel[i], mp_const_none);

        if(self->channel[i].pin) {
            mp_hal_pin_config(self->channel[i].pin, GPIO_MODE_INPUT, 0);
            self->channel[i].pin = NULL;
        }
    }

    self->enabled = false;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_pwm_deinit_obj, pyb_pwm_deinit);


static mp_obj_t pyb_pwm_channel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,                MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_callback,            MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_pin,                 MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_pulse_width_percent, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 50} },
        { MP_QSTR_capture_edge,        MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = eCAPTURE_RISING_LATCH} },
        { MP_QSTR_freq, 		       MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_complementary,	   MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };

    pyb_pwm_obj_t *self = pos_args[0];
    bool bIsBPWM = false;

    //parse argument
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if(self->pwm_obj->bEPWM == false) {
        bIsBPWM = true;
    }

    //find channel from pin object
    pin_obj_t *pin;
    const pin_af_obj_t *af = NULL;

    mp_obj_t pin_obj = args[2].u_obj;
    if (pin_obj != mp_const_none) {
        if (!MP_OBJ_IS_TYPE(pin_obj, &pin_type)) {
            mp_raise_ValueError("pin argument needs to be be a Pin type");
        }

        pin = pin_obj;

        if(bIsBPWM)
            af = pin_find_af(pin, AF_FN_BPWM, self->pwm_id);
        else
            af = pin_find_af(pin, AF_FN_EPWM, self->pwm_id);

        if (af == NULL) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Pin(%q) doesn't have an af for PWM(%d)", pin->name, self->pwm_id));
        }
    }

    if (af == NULL) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Must have an pin for PWM(%d)", self->pwm_id));
    }

    int chann_num = af->type;
    bool complementary = args[6].u_bool;
    pyb_pwm_channel_obj_t *complement_chann = NULL;

    if((chann_num & 0x1) && (args[6].u_bool == true)) {
        complementary = true;
        complement_chann = &self->channel[chann_num - 1];
    }

    if((complementary == true) && (args[0].u_int == CHANNEL_MODE_OUTPUT)) {
        if(bIsBPWM == false) {
            if(complement_chann->mode != CHANNEL_MODE_OUTPUT) {
                nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Please configure EPWM%d, channel %d to output mode", self->pwm_id, complement_chann->chann_id));
            }
        }
    }

    pyb_pwm_channel_obj_t *chann = &self->channel[chann_num];

    //check mode
    chann->mode = args[0].u_int;
    chann->callback = args[1].u_obj;
    chann->duty = args[3].u_int;
    chann->pin = pin;

    uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;

    if(bIsBPWM)
        mp_hal_pin_config_alt(pin, mode, AF_FN_BPWM, self->pwm_id);
    else
        mp_hal_pin_config_alt(pin, mode, AF_FN_EPWM, self->pwm_id);


    if(args[5].u_int == 0) {
        chann->freq = self->freq;
    } else {
        chann->freq = args[5].u_int;
    }


    if(chann->mode == CHANNEL_MODE_OUTPUT) {

        if(complementary) {
            chann->freq = complement_chann->freq;
        }

        PWM_TriggerOutputChannel(
            self->pwm_obj,
            chann_num,
            complementary,			//Complentary support. Only for EPWM
            complement_chann->chann_id,
            chann->freq,
            chann->duty
        );

    } else if(chann->mode == CHANNEL_MODE_CAPTURE) {

        chann->capture_edge = args[4].u_int;
        pyb_pwm_channel_cb(chann, args[1].u_obj);

        PWM_TriggerCaptureChannel(
            self->pwm_obj,
            chann_num,
            chann->freq,
            chann->capture_edge
        );
    }

    return chann;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pyb_pwm_channel_obj, 1, pyb_pwm_channel);

static void pyb_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_pwm_obj_t *self = self_in;

    if(self->pwm_obj->bEPWM == false) {
        if(!self->enabled) {
            mp_printf(print, "BPWM(%u)", self->pwm_id);
        } else {
            mp_printf(print, "BPWM(%u, freq=%u)", self->pwm_id, self->freq);
        }
    } else {
        if(!self->enabled) {
            mp_printf(print, "EPWM(%u)", self->pwm_id);
        } else {
            mp_printf(print, "EPWM(%u, freq=%u)", self->pwm_id, self->freq);
        }
    }
}

static const mp_rom_map_elem_t pyb_pwm_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_pwm_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel), MP_ROM_PTR(&pyb_pwm_channel_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_pwm_deinit_obj) },

    { MP_ROM_QSTR(MP_QSTR_OUTPUT), MP_ROM_INT(CHANNEL_MODE_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_CAPTURE), MP_ROM_INT(CHANNEL_MODE_CAPTURE) },

    { MP_ROM_QSTR(MP_QSTR_RISING), MP_ROM_INT(eCAPTURE_RISING_LATCH) },
    { MP_ROM_QSTR(MP_QSTR_FALLING), MP_ROM_INT(eCAPTURE_FALLING_LATCH) },
    { MP_ROM_QSTR(MP_QSTR_RISING_FALLING), MP_ROM_INT(eCAPTURE_RISING_FALLING_LATCH) },

    { MP_ROM_QSTR(MP_QSTR_BPWM), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_EPWM), MP_ROM_INT(1) },

};
static MP_DEFINE_CONST_DICT(pyb_pwm_locals_dict, pyb_pwm_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_pwm_type,
    MP_QSTR_PWM,
    MP_TYPE_FLAG_NONE,
    make_new, pyb_pwm_make_new,
    print, pyb_pwm_print,
    locals_dict, &pyb_pwm_locals_dict
);


static void pyb_pwm_channel_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_pwm_channel_obj_t *self = self_in;

    if(self->pyb_pwm_obj->pwm_obj->bEPWM == false) {
        mp_printf(print, "BPWM Channel(bpwm=%u, channel=%u, mode=%s)",
                  self->pyb_pwm_obj->pwm_id,
                  self->chann_id,
                  qstr_str(channel_mode_info[self->mode].name));
    } else {
        mp_printf(print, "EPWM Channel(epwm=%u, channel=%u, mode=%s)",
                  self->pyb_pwm_obj->pwm_id,
                  self->chann_id,
                  qstr_str(channel_mode_info[self->mode].name));

    }
}

/// \method pulse_width_percent([value])
/// Get or set the pulse width percentage associated with a channel.  The value
/// is a number between 0 and 100 and sets the percentage of the timer period
/// for which the pulse is active.  The value can be an integer or
/// floating-point number for more accuracy.  For example, a value of 25 gives
/// a duty cycle of 25%.
static mp_obj_t pyb_pwm_channel_pulse_width_percent(size_t n_args, const mp_obj_t *args)
{
    pyb_pwm_channel_obj_t *self = args[0];

    if (n_args == 1) {
        // get
        return mp_obj_new_int(self->duty);
    } else {
        // set
        if(self->mode == CHANNEL_MODE_OUTPUT) {
            self->duty = mp_obj_get_int(args[1]);

            PWM_ChangeChannelDuty(
                self->pyb_pwm_obj->pwm_obj,
                self->chann_id,
                self->duty
            );

        }

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_channel_pulse_width_percent_obj, 1, 2, pyb_pwm_channel_pulse_width_percent);


static mp_obj_t pyb_pwm_channel_disable(mp_obj_t self_in)
{
    pyb_pwm_channel_obj_t *self = self_in;

    if(self->mode == CHANNEL_MODE_OUTPUT) {
        PWM_StopOutputChannel(
            self->pyb_pwm_obj->pwm_obj,
            self->chann_id
        );
    } else if(self->mode == CHANNEL_MODE_CAPTURE) {

        PWM_StopCaptureChannel(
            self->pyb_pwm_obj->pwm_obj,
            self->chann_id,
            eCAPTURE_RISING_FALLING_LATCH
        );

        pyb_pwm_channel_cb(self, mp_const_none);
    }

    if(self->pin) {
        mp_hal_pin_config(self->pin, GPIO_MODE_INPUT, 0);
        self->pin = NULL;
    }

    self->mode = CHANNEL_MODE_NONE;
    return mp_const_none;

}

static MP_DEFINE_CONST_FUN_OBJ_1(pyb_pwm_channel_disable_obj, pyb_pwm_channel_disable);

static mp_obj_t pyb_pwm_channel_capture(mp_obj_t self_in, mp_obj_t edge_latch)
{
    pyb_pwm_channel_obj_t *self = self_in;
    int edge = mp_obj_get_int(edge_latch);

    if(self->pyb_pwm_obj->pwm_obj->bEPWM == false) {
        if(edge == eCAPTURE_RISING_LATCH)
            return MP_OBJ_NEW_SMALL_INT(BPWM_GET_CAPTURE_RISING_DATA(self->pyb_pwm_obj->pwm_obj->u_pwm.bpwm, self->chann_id));
        else if(edge == eCAPTURE_FALLING_LATCH)
            return MP_OBJ_NEW_SMALL_INT(BPWM_GET_CAPTURE_FALLING_DATA(self->pyb_pwm_obj->pwm_obj->u_pwm.bpwm, self->chann_id));
    } else {
        if(edge == eCAPTURE_RISING_LATCH)
            return MP_OBJ_NEW_SMALL_INT(EPWM_GET_CAPTURE_RISING_DATA(self->pyb_pwm_obj->pwm_obj->u_pwm.epwm, self->chann_id));
        else if(edge == eCAPTURE_FALLING_LATCH)
            return MP_OBJ_NEW_SMALL_INT(EPWM_GET_CAPTURE_FALLING_DATA(self->pyb_pwm_obj->pwm_obj->u_pwm.epwm, self->chann_id));
    }

    mp_raise_ValueError("Parameter Error");
}
static MP_DEFINE_CONST_FUN_OBJ_2(pyb_pwm_channel_capture_obj, pyb_pwm_channel_capture);

static const mp_rom_map_elem_t pyb_pwm_channel_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&pyb_pwm_channel_cb_obj) },
    { MP_ROM_QSTR(MP_QSTR_pulse_width_percent), MP_ROM_PTR(&pyb_pwm_channel_pulse_width_percent_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable), MP_ROM_PTR(&pyb_pwm_channel_disable_obj) },
    { MP_ROM_QSTR(MP_QSTR_capture), MP_ROM_PTR(&pyb_pwm_channel_capture_obj) },

//    { MP_ROM_QSTR(MP_QSTR_compare), MP_ROM_PTR(&pyb_timer_channel_compare_obj) },
};
static MP_DEFINE_CONST_DICT(pyb_pwm_channel_locals_dict, pyb_pwm_channel_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pyb_pwm_channel_type,
    MP_QSTR_PWMChannel,
    MP_TYPE_FLAG_NONE,
    print, pyb_pwm_channel_print,
    locals_dict, &pyb_pwm_channel_locals_dict
);


static pyb_pwm_obj_t* find_pyb_pwm_obj(pwm_t *psPWMObj)
{
    int i;
    int pyb_table_size;

    if(psPWMObj->bEPWM) {
        pyb_table_size = sizeof(pyb_epwm_obj) / sizeof(pyb_pwm_obj_t);

        for(i = 0; i < pyb_table_size; i++) {
            if(pyb_epwm_obj[i].pwm_obj == psPWMObj)
                return (pyb_pwm_obj_t*)&pyb_epwm_obj[i];
        }

        return NULL;
    }

    pyb_table_size = sizeof(pyb_bpwm_obj) / sizeof(pyb_pwm_obj_t);

    for(i = 0; i < pyb_table_size; i++) {
        if(pyb_bpwm_obj[i].pwm_obj == psPWMObj)
            return (pyb_pwm_obj_t*)&pyb_bpwm_obj[i];
    }

    return NULL;
}

static uint32_t BPWM_GetCaptureIntFlag_Fix(
    uint32_t u32IntFlagVal,
    uint32_t u32ChannelNum
)
{
    return ((((u32IntFlagVal & (BPWM_CAPTURE_INT_FALLING_LATCH << u32ChannelNum)) ? 1UL : 0UL) << 1) | \
            ((u32IntFlagVal & (BPWM_CAPTURE_INT_RISING_LATCH << u32ChannelNum)) ? 1UL : 0UL));
}

static uint32_t EPWM_GetCaptureIntFlag_Fix(
    uint32_t u32IntFlagVal,
    uint32_t u32ChannelNum
)
{
    return ((((u32IntFlagVal & (EPWM_CAPTURE_INT_FALLING_LATCH << u32ChannelNum)) ? 1UL : 0UL) << 1) | \
            ((u32IntFlagVal & (EPWM_CAPTURE_INT_RISING_LATCH << u32ChannelNum)) ? 1UL : 0UL));
}

static void pwm_handle_irq_channel(pyb_pwm_channel_obj_t *self, mp_obj_t callback, uint32_t irq_reason)
{

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
            mp_call_function_2(callback, MP_OBJ_FROM_PTR(self), MP_OBJ_NEW_SMALL_INT(irq_reason));
            nlr_pop();
        } else {
            // Uncaught exception; disable the callback so it doesn't run again.
            self->callback = mp_const_none;

            PWM_CaptureDisableInt(self->pyb_pwm_obj->pwm_obj, self->chann_id, eCAPTURE_RISING_FALLING_LATCH);

            printf("uncaught exception in Timer interrupt handler\n");
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
#if  MICROPY_PY_THREAD
        mp_sched_unlock();
#else
        gc_unlock();
#endif
    }
}

static void PWM_CaptureInt_Handler(void *obj, uint32_t u32ChannelGroup)
{
    pwm_t *psPWMObj = (pwm_t *)obj;
    pyb_pwm_obj_t *self = find_pyb_pwm_obj(psPWMObj);

    if(self == NULL)
        return;

    pyb_pwm_channel_obj_t *chann_obj;
    int i;
    uint32_t u32CaptureIntFlag = 0;


    uint32_t u32CaptureIntFlagVal;

    if(psPWMObj->bEPWM) {
        u32CaptureIntFlagVal =  psPWMObj->u_pwm.epwm->CAPIF;
        psPWMObj->u_pwm.epwm->CAPIF |= u32CaptureIntFlagVal & (0x303 << (u32ChannelGroup * 2));

        for(i = (u32ChannelGroup * 2) ; i < (u32ChannelGroup * 2 + 2); i ++) {
            chann_obj = &self->channel[i];
            u32CaptureIntFlag = EPWM_GetCaptureIntFlag_Fix(u32CaptureIntFlagVal, i);
            u32CaptureIntFlag &= chann_obj->capture_edge;

            if(u32CaptureIntFlag > 0) {
                pwm_handle_irq_channel(chann_obj, chann_obj->callback, u32CaptureIntFlag);
            }
        }
    } else {
        u32CaptureIntFlagVal =  psPWMObj->u_pwm.bpwm->CAPIF;
        psPWMObj->u_pwm.bpwm->CAPIF |= u32CaptureIntFlagVal;

        for(i = 0; i < M5531_MAX_PWM_CHANNEL_INST; i ++) {
            chann_obj = &self->channel[i];

            u32CaptureIntFlag = BPWM_GetCaptureIntFlag_Fix(u32CaptureIntFlagVal, i);

            u32CaptureIntFlag &= chann_obj->capture_edge;

            if(u32CaptureIntFlag > 0) {
                pwm_handle_irq_channel(chann_obj, chann_obj->callback, u32CaptureIntFlag);
            }
        }
    }
}

