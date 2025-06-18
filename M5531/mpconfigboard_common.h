/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Damien P. George
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

// Common settings and defaults for board configuration.
// The defaults here should be overridden in mpconfigboard.h.


/*****************************************************************************/
// Feature settings with defaults

// Whether to include the pyb module
#ifndef MICROPY_PY_PYB
#define MICROPY_PY_PYB (1)
#endif

// Whether to enable the RTC, exposed as pyb.RTC
#ifndef MICROPY_HW_ENABLE_RTC
#define MICROPY_HW_ENABLE_RTC (0)
#endif

// Whether to enable the hardware RNG peripheral, exposed as pyb.rng()
#ifndef MICROPY_HW_ENABLE_RNG
#define MICROPY_HW_ENABLE_RNG (0)
#endif

// Whether to enable the ADC peripheral, exposed as pyb.ADC and pyb.ADCAll
#ifndef MICROPY_HW_ENABLE_ADC
#define MICROPY_HW_ENABLE_ADC (0)
#endif

// Whether to enable USB support
#ifndef MICROPY_HW_ENABLE_USBD
#define MICROPY_HW_ENABLE_USBD (0)
#endif

// Whether to enable the PA0-PA3 servo driver, exposed as pyb.Servo
#ifndef MICROPY_HW_ENABLE_SERVO
#define MICROPY_HW_ENABLE_SERVO (0)
#endif

// Whether to expose internal flash storage as pyb.Flash
#ifndef MICROPY_HW_HAS_FLASH
#define MICROPY_HW_HAS_FLASH (1)
#endif

// Enable CAN if there are any peripherals defined
#if defined(MICROPY_HW_CANFD0_RXD) \
	||defined(MICROPY_HW_CANFD1_RXD)
#define MICROPY_HW_ENABLE_CAN (1)
#endif

// Whether to enable the SD card interface, exposed as pyb.SDCard
#if defined(MICROPY_HW_SD_CLK)
#define MICROPY_HW_HAS_SDCARD (1)
#else
#define MICROPY_HW_HAS_SDCARD (0)
#endif


// The volume label used when creating the flash filesystem
#ifndef MICROPY_HW_FLASH_FS_LABEL
#define MICROPY_HW_FLASH_FS_LABEL "pybflash"
#endif

/*****************************************************************************/
// General configuration

// Enable hardware EADC if there are any peripherals defined
#if defined(MICROPY_HW_EADC0_CH0) \
	||defined(MICROPY_HW_EADC0_CH1)\
	||defined(MICROPY_HW_EADC0_CH2)\
	||defined(MICROPY_HW_EADC0_CH3)\
	||defined(MICROPY_HW_EADC0_CH4)\
	||defined(MICROPY_HW_EADC0_CH5)\
	||defined(MICROPY_HW_EADC0_CH6)\
	||defined(MICROPY_HW_EADC0_CH7)\
	||defined(MICROPY_HW_EADC0_CH8)\
	||defined(MICROPY_HW_EADC0_CH9)\
	||defined(MICROPY_HW_EADC0_CH10)\
	||defined(MICROPY_HW_EADC0_CH11)\
	||defined(MICROPY_HW_EADC0_CH12)\
	||defined(MICROPY_HW_EADC0_CH13)\
	||defined(MICROPY_HW_EADC0_CH14)\
	||defined(MICROPY_HW_EADC0_CH15)\
	||defined(MICROPY_HW_EADC0_CH16)\
	||defined(MICROPY_HW_EADC0_CH17)\
	||defined(MICROPY_HW_EADC0_CH18)\
	||defined(MICROPY_HW_EADC0_CH19)\
	||defined(MICROPY_HW_EADC0_CH20)\
	||defined(MICROPY_HW_EADC0_CH21)\
	||defined(MICROPY_HW_EADC0_CH22)\
	||defined(MICROPY_HW_EADC0_CH23)
#define MICROPY_HW_ENABLE_HW_EADC (1)
#else
#define MICROPY_HW_ENABLE_HW_EADC (0)
#endif

#if defined(MICROPY_HW_DAC0_OUT) \
	||defined(MICROPY_HW_DAC1_OUT)
#define MICROPY_HW_ENABLE_HW_DAC (1)
#else
#define MICROPY_HW_ENABLE_HW_DAC (0)
#endif


// Pin definition header file
#define MICROPY_PIN_DEFS_PORT_H "hal/pin_defs_m5531.h"

//pin0 ~ pin15
#define PYB_EXTI_NUM_VECTORS (17)
//port a ~ port j
#define PYB_EXTI_NUM_PORTS (10)

