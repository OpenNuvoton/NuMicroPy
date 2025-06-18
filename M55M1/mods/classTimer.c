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
#include "classTimer.h"
#include "hal/M55M1_Timer.h"

typedef enum {
    CHANNEL_MODE_PWM_NORMAL,	//PWM mode
    CHANNEL_MODE_OC_TOGGLE,		//output compare mode
    CHANNEL_MODE_IC,			//input capture mode
} pyb_channel_mode;

typedef struct _pyb_timer_channel_obj_t {
    mp_obj_base_t base;
    struct _pyb_timer_obj_t *timer;
    uint8_t mode;
    mp_obj_t callback;
    uint32_t duty;
    struct _pyb_timer_channel_obj_t *next;
} pyb_timer_channel_obj_t;


typedef struct _pyb_timer_obj_t {
    mp_obj_base_t base;
    uint8_t tim_id;
    uint32_t freq;
    hw_timer_t *timer_obj;
    mp_obj_t callback;
    pyb_timer_channel_obj_t *channel;
} pyb_timer_obj_t;

static const struct {
    qstr        name;
    uint32_t    oc_mode;
} channel_mode_info[] = {
    { MP_QSTR_PWM,                CHANNEL_MODE_PWM_NORMAL },
    { MP_QSTR_OC_TOGGLE,          CHANNEL_MODE_OC_TOGGLE },
    { MP_QSTR_IC,                 CHANNEL_MODE_IC},
};

static hw_timer_t s_sTimer0Obj = {.u_timer.timer = TIMER0, .bLPTimer = false};
static hw_timer_t s_sTimer1Obj = {.u_timer.timer = TIMER1, .bLPTimer = false};
static hw_timer_t s_sTimer2Obj = {.u_timer.timer = TIMER2, .bLPTimer = false};
static hw_timer_t s_sTimer3Obj = {.u_timer.timer = TIMER3, .bLPTimer = false};

static hw_timer_t s_sTimer4Obj = {.u_timer.lptimer = LPTMR0, .bLPTimer = true};
static hw_timer_t s_sTimer5Obj = {.u_timer.lptimer = LPTMR1, .bLPTimer = true};

#define M55M1_MAX_TIMER_INST 6

static pyb_timer_obj_t pyb_timer_obj[M55M1_MAX_TIMER_INST] = {
    {{&machine_timer_type}, 0, 0, &s_sTimer0Obj, mp_const_none, NULL},
    {{&machine_timer_type}, 1, 0, &s_sTimer1Obj, mp_const_none, NULL},
    {{&machine_timer_type}, 2, 0, &s_sTimer2Obj, mp_const_none, NULL},
    {{&machine_timer_type}, 3, 0, &s_sTimer3Obj, mp_const_none, NULL},
    {{&machine_timer_type}, 4, 0, &s_sTimer4Obj, mp_const_none, NULL},
    {{&machine_timer_type}, 5, 0, &s_sTimer5Obj, mp_const_none, NULL},
};

static uint32_t compute_prescaler_period_from_freq(hw_timer_t *timer_obj, uint32_t u32Freq, uint32_t *pu32Prescale, uint32_t *pu32Period)
{
    uint32_t u32Clk;

    if(timer_obj->bLPTimer) {
        u32Clk = LPTMR_GetModuleClock(timer_obj->u_timer.lptimer);
    } else {
        u32Clk = TIMER_GetModuleClock(timer_obj->u_timer.timer);
    }

    uint32_t u32Cmpr = 0UL, u32Prescale = 0UL;

    /* Fastest possible timer working freq is (u32Clk / 2). While cmpr = 2, prescaler = 0. */
    if(u32Freq > (u32Clk / 2UL)) {
        u32Cmpr = 2UL;
    } else {
        u32Cmpr = u32Clk / u32Freq;
        u32Prescale = (u32Cmpr >> 24);  /* for 24 bits CMPDAT */
        if (u32Prescale > 0UL)
            u32Cmpr = u32Cmpr / (u32Prescale + 1UL);
    }

    *pu32Prescale = u32Prescale;
    *pu32Period = u32Cmpr;

    return(u32Clk / (u32Cmpr * (u32Prescale + 1UL)));
}

static uint32_t compute_freq_from_prescaler_period(hw_timer_t *timer_obj, uint32_t u32Prescale, uint32_t u32Period)
{
    uint32_t u32Clk;

    if(timer_obj->bLPTimer) {
        u32Clk = LPTMR_GetModuleClock(timer_obj->u_timer.lptimer);
    } else {
        u32Clk = TIMER_GetModuleClock(timer_obj->u_timer.timer);
    }

    uint32_t u32Cmpr = 0;

    if((u32Prescale == 0) && (u32Period == 2))
        return (u32Clk / 2);

    if(u32Prescale > 0UL)
        u32Cmpr = u32Period * (u32Prescale + 1UL);
    return (u32Clk / u32Cmpr);
}

static bool IS_TIM_COUNTER_MODE(uint32_t mode)
{
    if((mode != TIMER_ONESHOT_MODE) &&
       (mode != TIMER_PERIODIC_MODE) &&
       (mode != TIMER_TOGGLE_MODE) &&
       (mode != TIMER_CONTINUOUS_MODE))
        return false;
    return true;
}

/******************************************************************************/
/* MicroPython bindings                                                       */

static void Timer_IntStatus_Handler(void *obj, uint32_t u32Status);


/// \method callback(fun)
/// Set the function to be called when the timer channel triggers.
/// `fun` is passed 1 argument, the timer object.
/// If `fun` is `None` then the callback will be disabled.
static mp_obj_t pyb_timer_channel_callback(mp_obj_t self_in, mp_obj_t callback)
{
    pyb_timer_channel_obj_t *self = self_in;
    if (callback == mp_const_none) {
        // stop interrupt (but not timer)
        if(self->timer->callback != mp_const_none) {
            if(self->mode == CHANNEL_MODE_PWM_NORMAL) {
                Timer_DisablePWMPeriodInt(self->timer->timer_obj);
            } else if(self->mode == CHANNEL_MODE_IC) {
                Timer_DisableCaptureInt(self->timer->timer_obj);
                Timer_DisableInt(self->timer->timer_obj);
            } else {
                Timer_DisableInt(self->timer->timer_obj);
            }
        }
        self->callback = mp_const_none;
    } else if (mp_obj_is_callable(callback)) {

        self->callback = callback;
        if(self->mode == CHANNEL_MODE_PWM_NORMAL) {
            /* Enable period event interrupt */
            Timer_EnablePWMPeriodInt(self->timer->timer_obj);
            /* Start Timer PWM counter */
            Timer_PWMTrigger(self->timer->timer_obj);
        } else if (self->mode == CHANNEL_MODE_IC) {
            Timer_EnableCaptureInt(self->timer->timer_obj);
            Timer_EnableInt(self->timer->timer_obj, Timer_IntStatus_Handler);
            Timer_Trigger(self->timer->timer_obj);
        } else {
            Timer_EnableInt(self->timer->timer_obj, Timer_IntStatus_Handler);
            Timer_Trigger(self->timer->timer_obj);
        }
    } else {
        mp_raise_ValueError("callback must be None or a callable object");
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(pyb_timer_channel_callback_obj, pyb_timer_channel_callback);

static mp_obj_t pyb_timer_callback(mp_obj_t self_in, mp_obj_t callback)
{
    pyb_timer_obj_t *self = self_in;
    if (callback == mp_const_none) {
        // stop interrupt (but not timer)
        Timer_DisableInt(self->timer_obj);
        self->callback = mp_const_none;
    } else if (mp_obj_is_callable(callback)) {
        Timer_DisableInt(self->timer_obj);
        self->callback = callback;
        // start timer, so that it interrupts on overflow, but clear any
        // pending interrupts which may have been set by initializing it.
        Timer_EnableInt(self->timer_obj, Timer_IntStatus_Handler);
        Timer_Trigger(self->timer_obj);
    } else {
        mp_raise_ValueError("callback must be None or a callable object");
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(pyb_timer_callback_obj, pyb_timer_callback);

/// \method init(*, freq, prescaler, period)
/// Initialise the timer.  Initialisation must be either by frequency (in Hz)
/// or by prescaler and period:
///
///     tim.init(freq=100)                  # set the timer to trigger at 100Hz
///     tim.init(prescaler=83, period=999)  # set the prescaler and period directly
///
/// Keyword arguments:
///
///   - `freq` - specifies the periodic frequency of the timer. You migh also
///              view this as the frequency with which the timer goes through
///              one complete cycle.
///
///   - `prescaler` [0-0xffff] - specifies the value to be loaded into the
///                 timer's Prescaler Register (PSC). The timer clock source is divided by
///     (`prescaler + 1`) to arrive at the timer clock. Timers 2-7 and 12-14
///     have a clock source of 84 MHz (pyb.freq()[2] * 2), and Timers 1, and 8-11
///     have a clock source of 168 MHz (pyb.freq()[3] * 2).
///
///   - `period` [0-0xffff] for timers 1, 3, 4, and 6-15. [0-0x3fffffff] for timers 2 & 5.
///              Specifies the value to be loaded into the timer's AutoReload
///     Register (ARR). This determines the period of the timer (i.e. when the
///     counter cycles). The timer counter will roll-over after `period + 1`
///     timer clock cycles.
///
///   - `mode` can be one of:
///     - `Timer.UP` - configures the timer to count from 0 to ARR (default)
///     - `Timer.DOWN` - configures the timer to count from ARR down to 0.
///     - `Timer.CENTER` - confgures the timer to count from 0 to ARR and
///       then back down to 0.
///
///   - `div` can be one of 1, 2, or 4. Divides the timer clock to determine
///       the sampling clock used by the digital filters.
///
///   - `callback` - as per Timer.callback()
///
///   - `deadtime` - specifies the amount of "dead" or inactive time between
///       transitions on complimentary channels (both channels will be inactive)
///       for this time). `deadtime` may be an integer between 0 and 1008, with
///       the following restrictions: 0-128 in steps of 1. 128-256 in steps of
///       2, 256-512 in steps of 8, and 512-1008 in steps of 16. `deadime`
///       measures ticks of `source_freq` divided by `div` clock ticks.
///       `deadtime` is only available on timers 1 and 8.
///
///  You must either specify freq or both of period and prescaler.


static mp_obj_t pyb_timer_init_helper(pyb_timer_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq,         MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_prescaler,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
        { MP_QSTR_period,       MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
        { MP_QSTR_mode,         MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = TIMER_PERIODIC_MODE} },
        { MP_QSTR_callback,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint32_t u32Prescale = 0;
    uint32_t u32Period = 0;
    uint32_t u32CountingMode = TIMER_PERIODIC_MODE;

    if (args[0].u_obj != mp_const_none) {
        self->freq = (uint32_t) mp_obj_get_int(args[0].u_obj);
        // set prescaler and period from desired frequency
        compute_prescaler_period_from_freq(self->timer_obj, self->freq, &u32Prescale, &u32Period);
    } else if (args[1].u_int != 0xffffffff && args[2].u_int != 0xffffffff) {
        // set prescaler and period directly
        u32Prescale = args[1].u_int;
        u32Period = args[2].u_int;
        self->freq = compute_freq_from_prescaler_period(self->timer_obj, u32Prescale, u32Period);
    } else {
        mp_raise_TypeError("must specify either freq, or prescaler and period");
    }

    u32CountingMode = args[3].u_int;
    if (!IS_TIM_COUNTER_MODE(u32CountingMode)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "invalid mode (%d)", u32CountingMode));
    }

    Timer_InitTypeDef timerInit;

    timerInit.u32Mode = u32CountingMode;
    timerInit.u32Prescaler = u32Prescale;
    timerInit.u32Period = u32Period;

    Timer_Init(self->timer_obj, &timerInit);

    // Start the timer running
    if (args[4].u_obj == mp_const_none) {
        Timer_Trigger(self->timer_obj);
    } else {
        pyb_timer_callback(self, args[4].u_obj);
    }

    return mp_const_none;
}

static mp_obj_t pyb_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the timer id
    mp_int_t tim_id = mp_obj_get_int(args[0]);

    // check if the timer exists
    if (tim_id < 0 || tim_id >= M55M1_MAX_TIMER_INST) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Timer(%d) doesn't exist", tim_id));
    }

    pyb_timer_obj_t *tim = &pyb_timer_obj[tim_id];

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        pyb_timer_init_helper(tim, n_args - 1, args + 1, &kw_args);
    }

    return (mp_obj_t)tim;
}

static void pyb_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_timer_obj_t *self = self_in;

    if(!Timer_IsActivate(self->timer_obj)) {
        mp_printf(print, "Timer(%u)", self->tim_id);
    } else {
        mp_printf(print, "Timer(%u, freq=%u)", self->tim_id, self->freq);
    }

}

static mp_obj_t pyb_timer_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    return pyb_timer_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(pyb_timer_init_obj, 1, pyb_timer_init);

// timer.deinit()
static mp_obj_t pyb_timer_deinit(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    // Disable the base interrupt
    pyb_timer_callback(self_in, mp_const_none);

    pyb_timer_channel_obj_t *chan = self->channel;
    self->channel = NULL;

    // Disable the channel interrupts
    while (chan != NULL) {
        pyb_timer_channel_callback(chan, mp_const_none);

        if(chan->mode == CHANNEL_MODE_PWM_NORMAL) {
            /* Start Timer PWM counter */
            Timer_PWMSuspend(chan->timer->timer_obj);
        }

        pyb_timer_channel_obj_t *prev_chan = chan;
        chan = chan->next;
        prev_chan->next = NULL;
    }

    Timer_Final(self->timer_obj);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_deinit_obj, pyb_timer_deinit);


/// \method channel(mode, ...)
///
/// Each timer can be configured to perform pwm, output compare, or
/// input capture.
/// Keyword arguments:
///
///   - `mode` can be one of:
///     - `Timer.PWM` - configure the timer in PWM mode (active high).
///     - `Timer.OC_TOGGLE` - the pin will be toggled when an compare match occurs.
///     - `Timer.IC` - configure the timer in Input Capture mode.
///
///   - `callback` - as per TimerChannel.callback()
///
///   - `pin` None (the default) or a Pin object. If specified (and not None)
///           this will cause the alternate function of the the indicated pin
///      to be configured for this timer channel. An error will be raised if
///      the pin doesn't support any alternate functions for this timer channel.
///
/// Keyword arguments for Timer.PWM modes:
///
///   - `pulse_width_percent` - determines the initial pulse width percentage to use.
///
///
/// Optional keyword arguments for Timer.IC modes:
///
///   - `polarity` can be one of:
///     - `Timer.RISING` - captures on rising edge.
///     - `Timer.FALLING` - captures on falling edge.
///     - `Timer.BOTH` - captures on both edges.
///
///   Note that capture only works on the primary channel, and not on the
///   complimentary channels.
///
///
/// PWM Example:
///
///     timer = pyb.Timer(freq=1000)
///     ch2 = timer.channel(pyb.Timer.PWM, pin=pyb.Pin.board.X2, pulse_width=210000)
///     ch3 = timer.channel(pyb.Timer.PWM, pin=pyb.Pin.board.X3, pulse_width=420000)

static mp_obj_t pyb_timer_channel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,                MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_callback,            MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_pin,                 MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_pulse_width_percent, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 50} },
        { MP_QSTR_polarity,            MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
    };

    pyb_timer_obj_t *self = pos_args[0];
    pyb_timer_channel_obj_t *chan = self->channel;

    // If no argument return the previously allocated
    // channel (or None if no previous channel).
    if (n_args == 1 && kw_args->used == 0) {
        if (chan) {
            return chan;
        }
        return mp_const_none;
    }

    // If there was already a channel, then remove it from the list. Note that
    // the order we do things here is important so as to appear atomic to
    // the IRQ handler.
    if (chan) {
        // Turn off any IRQ associated with the channel.
        pyb_timer_channel_callback(chan, mp_const_none);
        self->channel = NULL;
    }

    // Allocate and initialize a new channel
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    chan = m_new_obj(pyb_timer_channel_obj_t);
    memset(chan, 0, sizeof(*chan));
    chan->base.type = &machine_timer_channel_type;
    chan->timer = self;
    chan->mode = args[0].u_int;
    chan->callback = args[1].u_obj;
    chan->duty = 0;

    const pin_obj_t *pin;
    const pin_af_obj_t *af = NULL;

    mp_obj_t pin_obj = args[2].u_obj;
    if (pin_obj != mp_const_none) {
        if (!MP_OBJ_IS_TYPE(pin_obj, &pin_type)) {
            mp_raise_ValueError("pin argument needs to be be a Pin type");
        }

        pin = pin_obj;
        if(self->timer_obj->bLPTimer)
            af = pin_find_af(pin, AF_FN_LPTM, self->tim_id);
        else
            af = pin_find_af(pin, AF_FN_TM, self->tim_id);
        if (af == NULL) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Pin(%q) doesn't have an af for Timer(%d)", pin->name, self->tim_id));
        }

#if 0
        // pin.init(mode=AF_PP, af=idx)
        const mp_obj_t args2[6] = {
            (mp_obj_t)&pin_init_obj,
            pin_obj,
            MP_OBJ_NEW_QSTR(MP_QSTR_mode),  MP_OBJ_NEW_SMALL_INT(GPIO_MODE_AF_PP),
            MP_OBJ_NEW_QSTR(MP_QSTR_af),    MP_OBJ_NEW_SMALL_INT(af->idx)
        };
        mp_call_method_n_kw(0, 2, args2);
#else
        uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;
        printf("switch timer pin fun \n");

        if(self->timer_obj->bLPTimer)
            mp_hal_pin_config_alt(pin, mode, AF_FN_LPTM, self->tim_id);
        else
            mp_hal_pin_config_alt(pin, mode, AF_FN_TM, self->tim_id);
#endif
    }

    if (af == NULL) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Must have an pin for Timer(%d)", self->tim_id));
    }

    self->channel = chan;

    if(Timer_IsActivate(self->timer_obj)) {
        Timer_Suspend(self->timer_obj);
        while(Timer_IsActivate(self->timer_obj));
    }

    switch (chan->mode) {
    case CHANNEL_MODE_PWM_NORMAL: {
        chan->duty = args[3].u_int;

        Timer_SetPWMMode(self->timer_obj, self->freq, chan->duty);

        if(af->type == AF_PIN_TYPE_TM_TM) {
            Timer_EnablePWMOutputChannel(self->timer_obj, TPWM_CH0);
        } else {
            Timer_EnablePWMOutputChannel(self->timer_obj, TPWM_CH1);
        }

        if (chan->callback == mp_const_none) {
            Timer_PWMTrigger(self->timer_obj);
        } else {
            pyb_timer_channel_callback(chan, chan->callback);
        }
    }
    break;
    case CHANNEL_MODE_OC_TOGGLE: {
        chan->duty = 50;

        uint32_t u32Prescale = 0;
        uint32_t u32Period = 0;
        uint32_t u32CountingMode = TIMER_TOGGLE_MODE;

        /* To generate toggle output, timer frequency must set to 2 * self->freq
           Because toggle output state change on every timer timeout event */
        compute_prescaler_period_from_freq(self->timer_obj, self->freq * 2, &u32Prescale, &u32Period);

        Timer_InitTypeDef timerInit;

        timerInit.u32Mode = u32CountingMode;
        timerInit.u32Prescaler = u32Prescale;
        timerInit.u32Period = u32Period;

        Timer_Init(self->timer_obj, &timerInit);

        if (chan->callback == mp_const_none) {
            Timer_Trigger(self->timer_obj);
        } else {
            pyb_timer_channel_callback(chan, chan->callback);
        }
    }
    break;
    case CHANNEL_MODE_IC: {
        /* Enable Timer2 event counter input and external capture function */
        uint32_t u32Prescale =  Timer_GetModuleClock(self->timer_obj) / self->freq;

        if((u32Prescale == 0) || (u32Prescale > 256)) {
            mp_raise_ValueError("timer capture frequence over range");
        }

        u32Prescale = u32Prescale - 1;

        Timer_UpdatePrescaler(self->timer_obj, u32Prescale);
        Timer_UpdatePeriod(self->timer_obj, 0xFFFFFF);
        Timer_EnableCapture(self->timer_obj, TIMER_CAPTURE_FREE_COUNTING_MODE, args[4].u_int);

        if (chan->callback == mp_const_none) {
            Timer_Trigger(self->timer_obj);
        } else {
            pyb_timer_channel_callback(chan, chan->callback);
        }
    }
    break;
    default:
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "invalid mode (%d)", chan->mode));
    }
    return chan;

}
static MP_DEFINE_CONST_FUN_OBJ_KW(pyb_timer_channel_obj, 1, pyb_timer_channel);

/// \method counter([value])
/// Get or set the timer counter.
static mp_obj_t pyb_timer_counter(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_obj_t *self = args[0];
    if (n_args == 1) {
        // get
        return mp_obj_new_int(Timer_GetCounter(self->timer_obj));
    } else {
        //unable set counter value
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_counter_obj, 1, 2, pyb_timer_counter);

/// \method source_freq()
/// Get the frequency of the source of the timer.
static mp_obj_t pyb_timer_source_freq(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;
    uint32_t source_freq = Timer_GetModuleClock(self->timer_obj);
    return mp_obj_new_int(source_freq);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_source_freq_obj, pyb_timer_source_freq);

/// \method freq([value])
/// Get or set the frequency for the timer (changes prescaler and period if set).
static mp_obj_t pyb_timer_freq(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_obj_t *self = args[0];
    if (n_args == 1) {
        // get
        return mp_obj_new_int(self->freq);
    } else {
        // set
        uint32_t period;
        uint32_t prescaler;

        self->freq = (uint32_t)args[1];
        compute_prescaler_period_from_freq(self->timer_obj, self->freq, &prescaler, &period);

        Timer_UpdatePrescaler(self->timer_obj, prescaler);
        Timer_UpdatePeriod(self->timer_obj, period);

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_freq_obj, 1, 2, pyb_timer_freq);


/// \method period([value])
/// Get or set the period of the timer.
static mp_obj_t pyb_timer_period(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_obj_t *self = args[0];
    uint32_t period;
    uint32_t prescaler;
    compute_prescaler_period_from_freq(self->timer_obj, self->freq, &prescaler, &period);


    if (n_args == 1) {
        // get
        return mp_obj_new_int(period);
    } else {
        // set
        period = mp_obj_get_int(args[1]);
        self->freq = compute_freq_from_prescaler_period(self->timer_obj, prescaler, period);
        Timer_UpdatePeriod(self->timer_obj, period);
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_period_obj, 1, 2, pyb_timer_period);

/// \method prescaler([value])
/// Get or set the prescaler for the timer.
static mp_obj_t pyb_timer_prescaler(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_obj_t *self = args[0];
    uint32_t period;
    uint32_t prescaler;
    compute_prescaler_period_from_freq(self->timer_obj, self->freq, &prescaler, &period);

    if (n_args == 1) {
        // get
        return mp_obj_new_int(prescaler);
    } else {
        // set
        prescaler = mp_obj_get_int(args[1]);
        self->freq = compute_freq_from_prescaler_period(self->timer_obj, prescaler, period);

        Timer_UpdatePrescaler(self->timer_obj, prescaler);
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_prescaler_obj, 1, 2, pyb_timer_prescaler);



static const mp_rom_map_elem_t pyb_timer_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel), MP_ROM_PTR(&pyb_timer_channel_obj) },
    { MP_ROM_QSTR(MP_QSTR_counter), MP_ROM_PTR(&pyb_timer_counter_obj) },
    { MP_ROM_QSTR(MP_QSTR_source_freq), MP_ROM_PTR(&pyb_timer_source_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&pyb_timer_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&pyb_timer_callback_obj) },
    { MP_ROM_QSTR(MP_QSTR_period), MP_ROM_PTR(&pyb_timer_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_prescaler), MP_ROM_PTR(&pyb_timer_prescaler_obj) },

    { MP_ROM_QSTR(MP_QSTR_ONESHOT), MP_ROM_INT(TIMER_ONESHOT_MODE) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(TIMER_PERIODIC_MODE) },
    { MP_ROM_QSTR(MP_QSTR_CONTINUOUS), MP_ROM_INT(TIMER_CONTINUOUS_MODE) },
    { MP_ROM_QSTR(MP_QSTR_PWM), MP_ROM_INT(CHANNEL_MODE_PWM_NORMAL) },
    { MP_ROM_QSTR(MP_QSTR_OC_TOGGLE), MP_ROM_INT(CHANNEL_MODE_OC_TOGGLE) },
    { MP_ROM_QSTR(MP_QSTR_IC), MP_ROM_INT(CHANNEL_MODE_IC) },
    { MP_ROM_QSTR(MP_QSTR_RISING), MP_ROM_INT(TIMER_CAPTURE_EVENT_RISING) },
    { MP_ROM_QSTR(MP_QSTR_FALLING), MP_ROM_INT(TIMER_CAPTURE_EVENT_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_BOTH), MP_ROM_INT(TIMER_CAPTURE_EVENT_RISING_FALLING) },


};
static MP_DEFINE_CONST_DICT(pyb_timer_locals_dict, pyb_timer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_timer_type,
    MP_QSTR_Timer,
    MP_TYPE_FLAG_NONE,
    make_new, pyb_timer_make_new,
    print, pyb_timer_print,
    locals_dict, &pyb_timer_locals_dict
);



/// \moduleref pyb
/// \class TimerChannel - setup a channel for a timer.
///
/// Timer channels are used to generate/capture a signal using a timer.
///
/// TimerChannel objects are created using the Timer.channel() method.
static void pyb_timer_channel_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_timer_channel_obj_t *self = self_in;

    mp_printf(print, "TimerChannel(timer=%u, mode=%s)",
              self->timer->tim_id,
              qstr_str(channel_mode_info[self->mode].name));
}

/// \method pulse_width_percent([value])
/// Get or set the pulse width percentage associated with a channel.  The value
/// is a number between 0 and 100 and sets the percentage of the timer period
/// for which the pulse is active.  The value can be an integer or
/// floating-point number for more accuracy.  For example, a value of 25 gives
/// a duty cycle of 25%.
static mp_obj_t pyb_timer_channel_pulse_width_percent(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_channel_obj_t *self = args[0];
    if (n_args == 1) {
        // get
        return mp_obj_new_int(self->duty);
    } else {
        // set
        if(self->mode == CHANNEL_MODE_PWM_NORMAL) {
            uint32_t u32Period = Timer_GetPWMPeriod(self->timer->timer_obj) + 1;
            self->duty = mp_obj_get_int(args[1]);

            /* Set PWM duty */
            Timer_UpdatePWMPeriod(self->timer->timer_obj, (((u32Period) * self->duty) / 100));
        }

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_channel_pulse_width_percent_obj, 1, 2, pyb_timer_channel_pulse_width_percent);

static mp_obj_t pyb_timer_channel_capture(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_channel_obj_t *self = args[0];

    if(self->mode == CHANNEL_MODE_IC)
        return mp_obj_new_int(Timer_GetCaptureData(self->timer->timer_obj));

    return mp_obj_new_int(0);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_channel_capture_obj, 1, 1, pyb_timer_channel_capture);

static mp_obj_t pyb_timer_channel_compare(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_channel_obj_t *self = args[0];
    if (n_args == 1) {
        // get
        if(self->mode == CHANNEL_MODE_PWM_NORMAL)
            return mp_obj_new_int(Timer_GetPWMCompareData(self->timer->timer_obj));
        else
            return mp_obj_new_int(Timer_GetCompareData(self->timer->timer_obj));
    } else {
        // set
        uint32_t u32CMP = mp_obj_get_int(args[1]);

        if(self->mode == CHANNEL_MODE_PWM_NORMAL)
            Timer_UpdatePWMPeriod(self->timer->timer_obj, u32CMP);
        else
            Timer_UpdatePeriod(self->timer->timer_obj, u32CMP);
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_channel_compare_obj, 1, 2, pyb_timer_channel_compare);


static const mp_rom_map_elem_t pyb_timer_channel_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&pyb_timer_channel_callback_obj) },
    { MP_ROM_QSTR(MP_QSTR_pulse_width_percent), MP_ROM_PTR(&pyb_timer_channel_pulse_width_percent_obj) },
    { MP_ROM_QSTR(MP_QSTR_capture), MP_ROM_PTR(&pyb_timer_channel_capture_obj) },
    { MP_ROM_QSTR(MP_QSTR_compare), MP_ROM_PTR(&pyb_timer_channel_compare_obj) },
};
static MP_DEFINE_CONST_DICT(pyb_timer_channel_locals_dict, pyb_timer_channel_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    machine_timer_channel_type,
    MP_QSTR_TimerChannel,
    MP_TYPE_FLAG_NONE,
    print, pyb_timer_channel_print,
    locals_dict, &pyb_timer_channel_locals_dict
);

static pyb_timer_obj_t* find_pyb_timer_obj(hw_timer_t *psTimerObj)
{
    int i;
    int pyb_table_size = sizeof(pyb_timer_obj) / sizeof(pyb_timer_obj_t);

    for(i = 0; i < pyb_table_size; i++) {
        if(pyb_timer_obj[i].timer_obj == psTimerObj)
            return (pyb_timer_obj_t*)&pyb_timer_obj[i];
    }

    return NULL;
}

static void timer_handle_irq(pyb_timer_obj_t *self, uint32_t status, mp_obj_t callback)
{

    if (status) {
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
                mp_call_function_1(callback, self);
                nlr_pop();
            } else {
                // Uncaught exception; disable the callback so it doesn't run again.
                self->callback = mp_const_none;
                Timer_DisableInt(self->timer_obj);
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
}

static void timer_handle_irq_channel(pyb_timer_channel_obj_t *self, uint32_t status, mp_obj_t callback)
{

    if (status) {
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
                mp_call_function_1(callback, self);
                nlr_pop();
            } else {
                // Uncaught exception; disable the callback so it doesn't run again.
                self->callback = mp_const_none;
                Timer_DisableInt(self->timer->timer_obj);
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
}


static void Timer_IntStatus_Handler(void *obj, uint32_t u32Status)
{
    hw_timer_t *psTimerObj = (hw_timer_t *)obj;

    //search pyb_can_obj
    pyb_timer_obj_t *self = find_pyb_timer_obj(psTimerObj);

    if (self == NULL) {
        return;
    }

    timer_handle_irq(self, u32Status, self->callback);

    // Check to see if a timer channel interrupt was pending
    pyb_timer_channel_obj_t *chan = self->channel;
    while (chan != NULL) {
        timer_handle_irq_channel(chan, u32Status, chan->callback);
        chan = chan->next;
    }
}

mp_obj_t pytimer_get_timerobj(
    int timer_id
)
{
    if (timer_id < 0 || timer_id >= M55M1_MAX_TIMER_INST)
        return NULL;
    return (mp_obj_t) &pyb_timer_obj[timer_id];
}

TIMER_T *pyb_timer_get_handle(mp_obj_t timer)
{
    if (mp_obj_get_type(timer) != &machine_timer_type) {
        mp_raise_ValueError("need a Timer object");
    }
    pyb_timer_obj_t *self = timer;
    return self->timer_obj->u_timer.timer;
}
