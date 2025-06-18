/*
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

#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"

#include "pybirq.h"
#include "classUART.h"
#include "classI2C.h"
#include "classSPI.h"
#include "classCAN.h"
#include "classPWM.h"
#include "classADC.h"
#include "classDAC.h"
#include "classTimer.h"
#include "classRTC.h"
#include "classWDT.h"


#if MICROPY_PY_PYB

/// \function elapsed_millis(start)
/// Returns the number of milliseconds which have elapsed since `start`.
///
/// This function takes care of counter wrap, and always returns a positive
/// number. This means it can be used to measure periods upto about 12.4 days.
///
/// Example:
///     start = pyb.millis()
///     while pyb.elapsed_millis(start) < 1000:
///         # Perform some operation
static mp_obj_t pyb_elapsed_millis(mp_obj_t start)
{
    uint32_t startMillis = mp_obj_get_int(start);
    uint32_t currMillis = mp_hal_ticks_ms();
    return MP_OBJ_NEW_SMALL_INT((currMillis - startMillis) & 0x3fffffff);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_elapsed_millis_obj, pyb_elapsed_millis);

/// \function elapsed_micros(start)
/// Returns the number of microseconds which have elapsed since `start`.
///
/// This function takes care of counter wrap, and always returns a positive
/// number. This means it can be used to measure periods upto about 17.8 minutes.
///
/// Example:
///     start = pyb.micros()
///     while pyb.elapsed_micros(start) < 1000:
///         # Perform some operation
static mp_obj_t pyb_elapsed_micros(mp_obj_t start)
{
    uint32_t startMicros = mp_obj_get_int(start);
    uint32_t currMicros = mp_hal_ticks_us();
    return MP_OBJ_NEW_SMALL_INT((currMicros - startMicros) & 0x3fffffff);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_elapsed_micros_obj, pyb_elapsed_micros);


static const mp_rom_map_elem_t pyb_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_pyb) },

    //IRQ
    { MP_ROM_QSTR(MP_QSTR_disable_irq), MP_ROM_PTR(&pyb_disable_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_irq), MP_ROM_PTR(&pyb_enable_irq_obj) },
#if IRQ_ENABLE_STATS
    { MP_ROM_QSTR(MP_QSTR_irq_stats), MP_ROM_PTR(&pyb_irq_stats_obj) },
#endif

    //Time related
    { MP_ROM_QSTR(MP_QSTR_elapsed_millis), MP_ROM_PTR(&pyb_elapsed_millis_obj) },
    { MP_ROM_QSTR(MP_QSTR_elapsed_micros), MP_ROM_PTR(&pyb_elapsed_micros_obj) },

    //Power related
    { MP_ROM_QSTR(MP_QSTR_wfi), MP_ROM_PTR(&pyb_wfi_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&pyb_power_stop_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_standby), MP_ROM_PTR(&pyb_power_standby_obj) },

    //Classes
    { MP_ROM_QSTR(MP_QSTR_Timer), MP_ROM_PTR(&machine_timer_type) },
    { MP_ROM_QSTR(MP_QSTR_PWM), MP_ROM_PTR(&machine_pwm_type) },
    { MP_ROM_QSTR(MP_QSTR_WDT), MP_ROM_PTR(&machine_wdt_type) },
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&pin_type) },
    { MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&machine_uart_type) },
    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&machine_i2c_type) },
    { MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&machine_spi_type) },
    { MP_ROM_QSTR(MP_QSTR_RTC), MP_ROM_PTR(&machine_rtc_type) },
#if MICROPY_HW_ENABLE_CAN
    { MP_ROM_QSTR(MP_QSTR_CAN), MP_ROM_PTR(&machine_can_type) },
#endif

#if MICROPY_HW_ENABLE_HW_EADC
    { MP_ROM_QSTR(MP_QSTR_ADCALL), MP_ROM_PTR(&machine_adc_all_type) },
    { MP_ROM_QSTR(MP_QSTR_ADC), MP_ROM_PTR(&machine_adc_type) },
#endif

#if MICROPY_HW_ENABLE_HW_DAC
    { MP_ROM_QSTR(MP_QSTR_DAC), MP_ROM_PTR(&machine_dac_type) },
#endif
};

static MP_DEFINE_CONST_DICT(pyb_module_globals, pyb_module_globals_table);

const mp_obj_module_t pyb_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pyb_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pyb, pyb_module);

#endif


