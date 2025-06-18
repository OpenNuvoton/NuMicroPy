/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2015 Damien P. George
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
#include "extmod/modmachine.h"

#include "classUART.h"
#include "classI2C.h"
#include "classSPI.h"
#include "classCAN.h"
#include "classPWM.h"
#include "classADC.h"
#include "classTimer.h"
#include "classDAC.h"
#include "classRTC.h"
#include "classWDT.h"


// This file is never compiled standalone, it's included directly from
// extmod/modmachine.c via MICROPY_PY_MACHINE_INCLUDEFILE.

#if MICROPY_HW_ENABLE_CAN
#define MACHINE_CAN_CLASS      { MP_ROM_QSTR(MP_QSTR_CAN), MP_ROM_PTR(&machine_can_type) },
#else
#define MACHINE_CAN_CLASS
#endif

#if MICROPY_HW_ENABLE_HW_EADC
#define MACHINE_ADC_CLASS      { MP_ROM_QSTR(MP_QSTR_ADCALL), MP_ROM_PTR(&machine_adc_all_type) }, \
                               { MP_ROM_QSTR(MP_QSTR_ADC), MP_ROM_PTR(&machine_adc_type) },
#else
#define MACHINE_ADC_CLASS
#endif

#if MICROPY_HW_ENABLE_HW_DAC
#define MACHINE_DAC_CLASS      { MP_ROM_QSTR(MP_QSTR_DAC), MP_ROM_PTR(&machine_dac_type) },
#else
#define MACHINE_DAC_CLASS
#endif



#define MICROPY_PY_MACHINE_EXTRA_GLOBALS \
    \
    { MP_ROM_QSTR(MP_QSTR_disable_irq),         MP_ROM_PTR(&machine_disable_irq_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_enable_irq),          MP_ROM_PTR(&machine_enable_irq_obj) }, \
    \
    { MP_ROM_QSTR(MP_QSTR_Pin),                 MP_ROM_PTR(&pin_type) }, \
    { MP_ROM_QSTR(MP_QSTR_UART),                MP_ROM_PTR(&machine_uart_type) }, \
    { MP_ROM_QSTR(MP_QSTR_I2C),                 MP_ROM_PTR(&machine_i2c_type) }, \
    { MP_ROM_QSTR(MP_QSTR_SPI), 				MP_ROM_PTR(&machine_spi_type) }, \
    { MP_ROM_QSTR(MP_QSTR_PWM), 				MP_ROM_PTR(&machine_pwm_type) }, \
	{ MP_ROM_QSTR(MP_QSTR_Timer),				MP_ROM_PTR(&machine_timer_type) }, \
	{ MP_ROM_QSTR(MP_QSTR_RTC),					MP_ROM_PTR(&machine_rtc_type) }, \
	{ MP_ROM_QSTR(MP_QSTR_WDT),					MP_ROM_PTR(&machine_wdt_type) }, \
	MACHINE_CAN_CLASS	\
	MACHINE_ADC_CLASS	\
	MACHINE_DAC_CLASS 	\
    \


static void mp_machine_idle(void)
{
    __WFI();
}

