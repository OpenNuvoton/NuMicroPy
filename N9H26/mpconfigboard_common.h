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

// Whether to expose external flash storage as pyb.Flash
#ifndef MICROPY_HW_HAS_SPIFLASH
#define MICROPY_HW_HAS_SPIFLASH (1)
#endif

// Whether to enable the SD card interface, exposed as pyb.SDCard
#ifndef MICROPY_HW_HAS_SDCARD
#define MICROPY_HW_HAS_SDCARD (1)
#endif

// Whether to enable the Audio In, exposed as AudioIn
#ifndef MICROPY_HW_ENABLE_AUDIOIN
#define MICROPY_HW_ENABLE_AUDIOIN (1)
#endif

// Whether to enable the SPU(sound processing unit), exposed as SPU
#ifndef MICROPY_HW_ENABLE_SPU
#define MICROPY_HW_ENABLE_SPU (1)
#endif

// The volume label used when creating the flash filesystem
#ifndef MICROPY_HW_FLASH_FS_LABEL
#define MICROPY_HW_FLASH_FS_LABEL "pybflash"
#endif

#if defined(MICROPY_NVTMEDIA)
// Whether to enable media record, exposed as Record
#ifndef MICROPY_ENABLE_RECORD
#define MICROPY_ENABLE_RECORD (1)
#endif

// Whether to enable media play, exposed as Record
#ifndef MICROPY_ENABLE_PLAY
#define MICROPY_ENABLE_PLAY (1)
#endif
#endif


/*****************************************************************************/
// General configuration

// Enable hardware I2C if there are any peripherals defined
#if defined(MICROPY_HW_I2C0_SCL)
#define MICROPY_HW_ENABLE_HW_I2C (1)
#else
#define MICROPY_HW_ENABLE_HW_I2C (0)
#endif

// Enable hardware UART if there are any peripherals defined
#if defined(MICROPY_HW_UART0_RXD)
#define MICROPY_HW_ENABLE_HW_UART (1)
#else
#define MICROPY_HW_ENABLE_HW_UART (0)
#endif

// Enable sensro if there are any peripherals defined
#if defined(MICROPY_HW_SENSOR_SCL)
#define MICROPY_PY_SENSOR (1)
#else
#define MICROPY_PY_SENSOR (0)
#endif

#if 0 //CHChen: TODO

// Enable hardware SPI if there are any peripherals defined
#if defined(MICROPY_HW_SPI0_SCK) || defined(MICROPY_HW_SPI1_SCK) \
    || defined(MICROPY_HW_SPI2_SCK) || defined(MICROPY_HW_SPI3_SCK)
#define MICROPY_HW_ENABLE_HW_SPI (1)
#else
#define MICROPY_HW_ENABLE_HW_SPI (0)
#endif

// Enable hardware EADC if there are any peripherals defined
#if defined(MICROPY_HW_EADC0_CH0) \
	||defined(MICROPY_HW_EADC0_CH1)\
	||defined(MICROPY_HW_EADC0_CH2)\
	||defined(MICROPY_HW_EADC0_CH3)\
	||defined(MICROPY_HW_EADC0_CH6)\
	||defined(MICROPY_HW_EADC0_CH7)\
	||defined(MICROPY_HW_EADC0_CH8)\
	||defined(MICROPY_HW_EADC0_CH9)
#define MICROPY_HW_ENABLE_HW_EADC (1)
#endif

// Enable DAC if there are any peripherals defined

#if defined(MICROPY_HW_DAC0_OUT) \
	||defined(MICROPY_HW_DAC1_OUT)

#define MICROPY_HW_ENABLE_DAC (1)
#endif

// Enable CAN if there are any peripherals defined

#if defined(MICROPY_HW_CAN0_RXD) \
	||defined(MICROPY_HW_CAN1_RXD)

#define MICROPY_HW_ENABLE_CAN (1)
#endif

#if defined (MICROPY_HW_USRSW_SW0_PIN) \
	|| defined(MICROPY_HW_USRSW_SW1_PIN) \
	|| defined(MICROPY_HW_USRSW_SW2_PIN) \
	|| defined(MICROPY_HW_USRSW_SW3_PIN)
	
#define MICROPY_HW_HAS_SWITCH (1)
#endif

#if defined (MICROPY_HW_LED0) \
	|| defined(MICROPY_HW_LED1) \
	|| defined(MICROPY_HW_LED2)
	
#define MICROPY_HW_HAS_LED (1)
#endif

//#if !defined(MICROPY_LVGL)
#if defined (MICROPY_NAU88L25_I2C_SCL)
#define MICROPY_PY_MACHINE_AUDIO (1)
#else
#define MICROPY_PY_MACHINE_AUDIO (0)
#endif
//#else
//#define MICROPY_PY_MACHINE_AUDIO (0)
//#endif

#endif

// Pin definition header file
#define MICROPY_PIN_DEFS_PORT_H "hal/pin_defs_N9H26.h"

#define PYB_EXTI_NUM_VECTORS (17)
