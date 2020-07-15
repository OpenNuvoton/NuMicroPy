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

#if defined(MICROPY_HW_LEDRGB)
#include "hal/M48x_SPI.h"
#endif

#if defined(MICROPY_HW_HAS_LED)

// the default is that LEDs are not inverted, and pin driven high turns them on
#ifndef MICROPY_HW_LED_INVERTED
#define MICROPY_HW_LED_INVERTED (1)
#endif

typedef struct _pyb_led_obj_t {
    mp_obj_base_t base;
    mp_uint_t led_id;
    const pin_obj_t *led_pin;

#if defined(MICROPY_HW_LEDRGB)
	spi_t sSPIObj;
	int32_t i32SPIUnit;
#endif
} pyb_led_obj_t;


/*********************************************************************************/
/* Serial RGB LED driver (WS2811S, WS2812B)										 */
#if defined(MICROPY_HW_LEDRGB)

#define MAX_SLED_NUM (8)
#define SPI_NZR_CLK	 (6400000)		// (1)/(1.25us/8)

typedef enum{
	eNZR_CODE_0 = 0xC0, 
	eNZR_CODE_1 = 0xFC,
}E_NZR_CODE;

typedef enum{
	eSLED_TYPE_RGB, 
	eSLED_TYPE_GRB,
}E_SLED_TYPE;

typedef struct{
	uint8_t u8LEDNum;
	uint8_t *pu8LEDData;
	E_SLED_TYPE eSLEDType;
}S_SERIAL_LED;

static uint8_t s_au8SLEDData[MAX_SLED_NUM * 24];

static S_SERIAL_LED s_sSLEDCtrl = {
	.u8LEDNum = MAX_SLED_NUM,
	.pu8LEDData = s_au8SLEDData,
	.eSLEDType = eSLED_TYPE_GRB,
};

static void
SLED_DataWriteIDLE(
	pyb_led_obj_t *self
)
{
	mp_hal_pin_config(self->led_pin, MP_HAL_PIN_MODE_OUTPUT, 0);
	mp_hal_pin_low(self->led_pin);
}


static void spi_led_pdma_handler(spi_t *obj){
	obj->event = SPI_EVENT_COMPLETE;

    SPI_ClearRxFIFO(obj->spi);
    SPI_ClearTxFIFO(obj->spi);

	SPI_DISABLE_RX_PDMA(obj->spi);
	SPI_DISABLE_TX_PDMA(obj->spi);
}

static int32_t SLED_OpenSPIBus(
	pyb_led_obj_t *self
)
{
	SPI_InitTypeDef sSPIInit;

	sSPIInit.Mode =	SPI_MASTER;
	sSPIInit.BaudRate =	SPI_NZR_CLK;
	sSPIInit.ClockPolarity =	0;
	sSPIInit.Direction = SPI_DIRECTION_2LINES;
	sSPIInit.Bits = 8;
	sSPIInit.FirstBit = SPI_FIRSTBIT_MSB;
	sSPIInit.ClockPhase = SPI_PHASE_1EDGE;

	SPI_Init(&self->sSPIObj, &sSPIInit);

	return 0;
}


static void
SLED_DataWriteTrigger(
	pyb_led_obj_t *self
)
{
	uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;

	mp_hal_pin_config_alt(self->led_pin, mode, AF_FN_SPI, self->i32SPIUnit);

	SLED_OpenSPIBus(self);

	self->sSPIObj.event = 0;
	spi_master_transfer(&self->sSPIObj, (char *)s_sSLEDCtrl.pu8LEDData, MAX_SLED_NUM * 24, NULL, 0, 8, (uint32_t)spi_led_pdma_handler, SPI_EVENT_ALL, DMA_USAGE_ALWAYS);

	// check event flag for complete
	while(self->sSPIObj.event == 0){
		mp_hal_delay_ms(1);
	}

	SPI_Final(&self->sSPIObj);
	SLED_DataWriteIDLE(self);
	mp_hal_delay_ms(1);
}



static int32_t SLED_Initial(
	pyb_led_obj_t *self
)
{
	const pin_af_obj_t *af_spi_mosi = pin_find_af_by_fn_type(self->led_pin, AF_FN_SPI, AF_PIN_TYPE_SPI_MOSI);

	if(af_spi_mosi == NULL){
		return -1;
	}

	self->i32SPIUnit = af_spi_mosi->unit;
	self->sSPIObj.spi = (SPI_T *)(af_spi_mosi->reg);

	int i;
	for(i = 0 ; i < MAX_SLED_NUM * 24; i ++)
		s_sSLEDCtrl.pu8LEDData[i] = eNZR_CODE_0;

	SLED_DataWriteTrigger(self);
	return 0;
}
 
static void SLED_SetRGBData(
	uint32_t u32LED,
	uint8_t u8DataR,
	uint8_t u8DataG,
	uint8_t u8DataB
)
{
	uint32_t u32Temp;
	int8_t k;

	if(u32LED >= MAX_SLED_NUM)
		return;

	/* R */
	u32Temp = (u32LED * 24) + (0 * 8) + 7;
	
	for(k = 7; k >= 0; k--)
	{
			if(u8DataR & (1 << k))
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_1;
			}
			else
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_0;
			}
	}	

	/* G */
	u32Temp = (u32LED * 24) + (1 * 8) + 7;
	
	for(k = 7; k >= 0; k--)
	{
			if(u8DataG & (1 << k))
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_1;
			}
			else
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_0;
			}
	}	

	/* B */
	u32Temp = (u32LED * 24) + (2 * 8) + 7;
	
	for(k = 7; k >= 0; k--)
	{
			if(u8DataB & (1 << k))
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_1;
			}
			else
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_0;
			}
	}
}
	
static void SLED_SetGRBData(
	uint32_t u32LED,
	uint8_t u8DataR,
	uint8_t u8DataG,
	uint8_t u8DataB
)
{
	uint32_t u32Temp;
	int8_t k;

	if(u32LED >= MAX_SLED_NUM)
		return;

	/* G */
	u32Temp = (u32LED * 24) + (0 * 8) + 7;
	
	for(k = 7; k >= 0; k--)
	{
			if(u8DataG & (1 << k))
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_1;
			}
			else
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_0;
			}
	}	

	/* R */
	u32Temp = (u32LED * 24) + (1 * 8) + 7;
	
	for(k = 7; k >= 0; k--)
	{
			if(u8DataR & (1 << k))
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_1;
			}
			else
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_0;
			}
	}	

	/* B */
	u32Temp = (u32LED * 24) + (2 * 8) + 7;
	
	for(k = 7; k >= 0; k--)
	{
			if(u8DataB & (1 << k))
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_1;
			}
			else
			{
					*(s_sSLEDCtrl.pu8LEDData + (u32Temp-k)) = eNZR_CODE_0;
			}
	}
}



#endif


/******************************************************************************/
/* MicroPython bindings                                                       */
typedef enum {
	eLED_0,
	eLED_1,
	eLED_2,
	eLED_RGB,
	eLED_END,	
}E_LED_ID;


STATIC pyb_led_obj_t pyb_led_obj[eLED_END] = {
#if defined(MICROPY_HW_LED0)
    {{&pyb_led_type}, eLED_0, MICROPY_HW_LED0},
#else
    {{&pyb_led_type}, eLED_0, NULL},
#endif
#if defined(MICROPY_HW_LED1)
    {{&pyb_led_type}, eLED_1, MICROPY_HW_LED1},
#else
    {{&pyb_led_type}, eLED_1, NULL},
#endif
#if defined(MICROPY_HW_LED2)
    {{&pyb_led_type}, eLED_2, MICROPY_HW_LED2},
#else
    {{&pyb_led_type}, eLED_2, NULL},
#endif
#if defined(MICROPY_HW_LEDRGB)
    {{&pyb_led_type}, eLED_RGB, MICROPY_HW_LEDRGB},
#else
    {{&pyb_led_type}, eLED_RGB, NULL},
#endif
};


static void led_state(mp_uint_t led, int state) {
    if (led < 0 || led >= eLED_END) {
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

static void led_toggle(mp_uint_t led) {
    if (led < 0 || led >= eLED_END) {
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

static int led_get_intensity(mp_uint_t led) {
    if (led < 0 || led >= eLED_END) {
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

static void led_set_intensity(mp_uint_t led, mp_int_t intensity) {
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

#if defined(MICROPY_HW_LEDRGB)

mp_obj_t led_obj_rgb_write(size_t n_args, const mp_obj_t *args) {

    pyb_led_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	int32_t i32DataR;
	int32_t i32DataG;
	int32_t i32DataB;

	if(self->led_id != eLED_RGB)
		return mp_const_none;

	if(MP_OBJ_IS_TYPE(args[1], &mp_type_list)){
		size_t led_num;
		size_t temp;
		mp_obj_t *elem;
		mp_obj_t *rgb_elem;

		int i;

		mp_obj_get_array(args[1], &led_num, &elem);

		for(i = 0; i < led_num; i ++){
			mp_obj_get_array(elem[i], &temp, &rgb_elem);
			i32DataR = mp_obj_get_int(rgb_elem[0]);
			i32DataG = mp_obj_get_int(rgb_elem[1]);
			i32DataB = mp_obj_get_int(rgb_elem[2]);

			if(i32DataR < 0)
				i32DataR = 0;

			i32DataR &= 0xFF; 	//0~255

			if(i32DataG < 0)
				i32DataG = 0;

			i32DataG &= 0xFF; 	//0~255

			if(i32DataB < 0)
				i32DataB = 0;

			i32DataB &= 0xFF; 	//0~255

			if(s_sSLEDCtrl.eSLEDType == eSLED_TYPE_GRB){
				SLED_SetGRBData(i, i32DataR, i32DataG, i32DataB);
			}
			else{
				SLED_SetRGBData(i, i32DataR, i32DataG, i32DataB);
			}			
		}
		
		SLED_DataWriteTrigger(self);

		return mp_const_none;
	}

	uint32_t u32LedNum = mp_obj_get_int(args[1]);
	i32DataR = mp_obj_get_int(args[2]);
	i32DataG = mp_obj_get_int(args[3]);
	i32DataB = mp_obj_get_int(args[4]);
	
	if(i32DataR < 0)
		i32DataR = 0;

	i32DataR &= 0xFF; 	//0~255

	if(i32DataG < 0)
		i32DataG = 0;

	i32DataG &= 0xFF; 	//0~255

	if(i32DataB < 0)
		i32DataB = 0;

	i32DataB &= 0xFF; 	//0~255
	
	if(s_sSLEDCtrl.eSLEDType == eSLED_TYPE_GRB){
		SLED_SetGRBData(u32LedNum, i32DataR, i32DataG, i32DataB);
	}
	else{
		SLED_SetRGBData(u32LedNum, i32DataR, i32DataG, i32DataB);
	}

	SLED_DataWriteTrigger(self);
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_obj_rgb_write_obj, 2, 5, led_obj_rgb_write);
#endif

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
            led_idx = eLED_0;
        #endif
        #ifdef MICROPY_HW_LED1_NAME
        } else if (strcmp(led_pin, MICROPY_HW_LED1_NAME) == 0) {
            led_idx = eLED_1;
        #endif			
        #ifdef MICROPY_HW_LED2_NAME
        } else if (strcmp(led_pin, MICROPY_HW_LED2_NAME) == 0) {
            led_idx = eLED_2;
        #endif
        #ifdef MICROPY_HW_LEDRGB_NAME
        } else if (strcmp(led_pin, MICROPY_HW_LEDRGB_NAME) == 0) {
            led_idx = eLED_RGB;
        #endif
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "LED(%s) doesn't exist", led_pin));
        }
    } else {
        led_idx = mp_obj_get_int(args[0]);
    }

    if (led_idx < 0 || led_idx >= eLED_END) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "LED(%d) doesn't exist", led_idx));
    }

	if(pyb_led_obj[led_idx].led_pin == NULL){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "LED(%d) is not suuport", led_idx));
	}

	if(led_idx != eLED_RGB){
		mp_hal_pin_config(pyb_led_obj[led_idx].led_pin, MP_HAL_PIN_MODE_OUTPUT, 0);
	}
	else{
		//init serial RGB LED
#if defined(MICROPY_HW_LEDRGB)
		SLED_Initial((pyb_led_obj_t *)&pyb_led_obj[led_idx]);
#endif
	}

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
#if defined(MICROPY_HW_LEDRGB)
    { MP_ROM_QSTR(MP_QSTR_rgb_write), MP_ROM_PTR(&led_obj_rgb_write_obj) },
#endif

	// constants for LEDs
    { MP_ROM_QSTR(MP_QSTR_LED0), MP_ROM_INT(eLED_0) },
    { MP_ROM_QSTR(MP_QSTR_LED1), MP_ROM_INT(eLED_1) },
    { MP_ROM_QSTR(MP_QSTR_LED2), MP_ROM_INT(eLED_2) },
    { MP_ROM_QSTR(MP_QSTR_RGB), MP_ROM_INT(eLED_RGB) },	//For Serial RGB LED
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
