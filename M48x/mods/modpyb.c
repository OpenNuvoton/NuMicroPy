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
#include "py/gc.h"
#include "py/builtin.h"
#include "py/mphal.h"

#include "extmod/utime_mphal.h"

#include "pybsdcard.h"
#include "pybirq.h"
#include "pybi2c.h"
#include "pybspi.h"
#include "pybadc.h"
#include "pybrtc.h"
#include "pybtimer.h"
#include "pybuart.h"
#include "pybdac.h"
#include "pybcan.h"
#include "pybusb.h"
#include "pybswitch.h"
#include "pybled.h"
#include "pybpwm.h"
#include "pybwdt.h"
#include "pybaccel.h"
#include "rng.h"
#include "power_manager.h"

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
STATIC mp_obj_t pyb_elapsed_millis(mp_obj_t start) {
    uint32_t startMillis = mp_obj_get_int(start);
    uint32_t currMillis = mp_hal_ticks_ms();
    return MP_OBJ_NEW_SMALL_INT((currMillis - startMillis) & 0x3fffffff);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_elapsed_millis_obj, pyb_elapsed_millis);

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
STATIC mp_obj_t pyb_elapsed_micros(mp_obj_t start) {
    uint32_t startMicros = mp_obj_get_int(start);
    uint32_t currMicros = mp_hal_ticks_us();
    return MP_OBJ_NEW_SMALL_INT((currMicros - startMicros) & 0x3fffffff);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_elapsed_micros_obj, pyb_elapsed_micros);


STATIC mp_obj_t pyb_power_stop(void) {
	PowerManager_EnterNormalPowerDown();
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(pyb_power_stop_obj, pyb_power_stop);

STATIC mp_obj_t pyb_power_standby(void) {
	printf("DDDD pyb_power_standby \n");
	PowerManager_EnterStandbyPowerDown();
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(pyb_power_standby_obj, pyb_power_standby);

STATIC const mp_rom_map_elem_t pyb_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_pyb) },

	//IRQ
    { MP_ROM_QSTR(MP_QSTR_disable_irq), MP_ROM_PTR(&pyb_disable_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_irq), MP_ROM_PTR(&pyb_enable_irq_obj) },
    #if IRQ_ENABLE_STATS
    { MP_ROM_QSTR(MP_QSTR_irq_stats), MP_ROM_PTR(&pyb_irq_stats_obj) },
    #endif

	// Power related
    { MP_ROM_QSTR(MP_QSTR_wfi), MP_ROM_PTR(&pyb_wfi_obj) },
	{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&pyb_power_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_standby), MP_ROM_PTR(&pyb_power_standby_obj) },
	
	//Time related
	{ MP_ROM_QSTR(MP_QSTR_millis), MP_ROM_PTR(&mp_utime_ticks_ms_obj) },
	{ MP_ROM_QSTR(MP_QSTR_elapsed_millis), MP_ROM_PTR(&pyb_elapsed_millis_obj) },
	{ MP_ROM_QSTR(MP_QSTR_micros), MP_ROM_PTR(&mp_utime_ticks_us_obj) },
	{ MP_ROM_QSTR(MP_QSTR_elapsed_micros), MP_ROM_PTR(&pyb_elapsed_micros_obj) },
	{ MP_ROM_QSTR(MP_QSTR_delay), MP_ROM_PTR(&mp_utime_sleep_ms_obj) },
	{ MP_ROM_QSTR(MP_QSTR_udelay), MP_ROM_PTR(&mp_utime_sleep_us_obj) },

	//Classes
	{ MP_ROM_QSTR(MP_QSTR_Timer), MP_ROM_PTR(&pyb_timer_type) },
	{ MP_ROM_QSTR(MP_QSTR_PWM), MP_ROM_PTR(&pyb_pwm_type) },
	{ MP_ROM_QSTR(MP_QSTR_WDT), MP_ROM_PTR(&pyb_wdt_type) },
	{ MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&pin_type) },

#if MICROPY_HW_HAS_SDCARD
	{ MP_ROM_QSTR(MP_QSTR_SDCard), MP_ROM_PTR(&pyb_sdcard_type) },
#endif

#if MICROPY_HW_ENABLE_USBD
    { MP_ROM_QSTR(MP_QSTR_usb_mode), MP_ROM_PTR(&pyb_usb_mode_obj) },
	{ MP_ROM_QSTR(MP_QSTR_USB_HID), MP_ROM_PTR(&pyb_usb_hid_type) },
	{ MP_ROM_QSTR(MP_QSTR_USB_VCP), MP_ROM_PTR(&pyb_usb_vcp_type) },
#endif
    { MP_ROM_QSTR(MP_QSTR_msc_enable), MP_ROM_PTR(&pyb_msc_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_msc_disable), MP_ROM_PTR(&pyb_msc_disable_obj) },

#if MICROPY_HW_ENABLE_HW_I2C
    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&pyb_i2c_type) },
#endif
#if MICROPY_HW_ENABLE_HW_SPI
	{ MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&pyb_spi_type) },
#endif

#if MICROPY_HW_ENABLE_HW_EADC
	{ MP_ROM_QSTR(MP_QSTR_ADCALL), MP_ROM_PTR(&pyb_adc_all_type) },
    { MP_ROM_QSTR(MP_QSTR_ADC), MP_ROM_PTR(&pyb_adc_type) },
#endif

#if MICROPY_HW_ENABLE_RTC
    { MP_ROM_QSTR(MP_QSTR_RTC), MP_ROM_PTR(&pyb_rtc_type) },
#endif

#if MICROPY_HW_ENABLE_DAC
    { MP_ROM_QSTR(MP_QSTR_DAC), MP_ROM_PTR(&pyb_dac_type) },
#endif

#if MICROPY_HW_ENABLE_CAN
    { MP_ROM_QSTR(MP_QSTR_CAN), MP_ROM_PTR(&pyb_can_type) },
#endif

#if MICROPY_HW_HAS_SWITCH
    { MP_ROM_QSTR(MP_QSTR_Switch), MP_ROM_PTR(&pyb_switch_type) },
#endif

#if MICROPY_HW_HAS_LED
    { MP_ROM_QSTR(MP_QSTR_LED), MP_ROM_PTR(&pyb_led_type) },
#endif

#if MICROPY_HW_ENABLE_RNG
    { MP_ROM_QSTR(MP_QSTR_rng), MP_ROM_PTR(&pyb_rng_get_obj) },
#endif

#if MICROPY_HW_HAS_BMX055
    { MP_ROM_QSTR(MP_QSTR_Accel), MP_ROM_PTR(&pyb_accel_type) },
    { MP_ROM_QSTR(MP_QSTR_Gyro), MP_ROM_PTR(&pyb_gyro_type) },
    { MP_ROM_QSTR(MP_QSTR_Mag), MP_ROM_PTR(&pyb_mag_type) },
#endif

   { MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&pyb_uart_type) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_module_globals, pyb_module_globals_table);

const mp_obj_module_t pyb_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pyb_module_globals,
};

