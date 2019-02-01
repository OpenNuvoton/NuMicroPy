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

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mphal.h"

#include "pybtimer.h"
#include "pybpin.h"
#include "hal/dma.h"
#include "pybdac.h"

/// \moduleref pyb
/// \class DAC - digital to analog conversion
///
/// The DAC is used to output analog values (a specific voltage) on pin X5 or pin X6.
/// The voltage will be between 0 and 3.3V.
///
/// *This module will undergo changes to the API.*
///
/// Example usage:
///
///     from pyb import DAC
///
///     dac = DAC(1)            # create DAC 1 on pin X5
///     dac.write(128)          # write a value to the DAC (makes X5 1.65V)
///
/// To output a continuous sine-wave:
///
///     import math
///     from pyb import DAC
///
///     # create a buffer containing a sine-wave
///     buf = bytearray(100)
///     for i in range(len(buf)):
///         buf[i] = 128 + int(127 * math.sin(2 * math.pi * i / len(buf)))
///
///     # output the sine-wave at 400Hz
///     dac = DAC(1)
///     dac.write_timed(buf, 400 * len(buf), mode=DAC.CIRCULAR)

#if defined(MICROPY_HW_ENABLE_DAC) && MICROPY_HW_ENABLE_DAC

enum{
	DMA_NORMAL,	
	DMA_CIRCULAR
};

typedef enum {
    DAC_STATE_RESET,
    DAC_STATE_WRITE_SINGLE,
    DAC_STATE_BUILTIN_WAVEFORM,
    DAC_STATE_DMA_WAVEFORM, // should be last enum since we use space beyond it
} pyb_dac_state_t;

typedef struct _pyb_dac_obj_t {
    mp_obj_base_t base;
    uint32_t dac_channel; // DAC_CHANNEL_1 or DAC_CHANNEL_2
	DAC_T *dac;
    uint8_t bits; // 8 or 12
	uint32_t pdma_perp_tx;
    uint8_t state;
//    uint8_t outbuf_single;
//    uint8_t outbuf_waveform;
	uint32_t dma_chan_id;
	uint32_t dma_trans_len;
	mp_obj_t timer_obj;
} pyb_dac_obj_t;

#define M48X_MAX_DAC_INST 2


STATIC pyb_dac_obj_t pyb_dac_obj[M48X_MAX_DAC_INST] = {
#if defined(MICROPY_HW_DAC0_OUT)
    {{&pyb_dac_type}, 0, DAC0, 8, PDMA_DAC0_TX, DAC_STATE_RESET},
#else
    {{&pyb_dac_type}, 0, NULL, 8, PDMA_DAC0_TX, DAC_STATE_RESET},
#endif
#if defined(MICROPY_HW_DAC1_OUT)
    {{&pyb_dac_type}, 1, DAC1, 8, PDMA_DAC1_TX, DAC_STATE_RESET},
#else
    {{&pyb_dac_type}, 1, NULL, 8, PDMA_DAC1_TX, DAC_STATE_RESET},
#endif
};

STATIC void enable_dac_clock(const pyb_dac_obj_t *self, bool bEnable){

	if(bEnable == false){
		if(self->dac_channel == 0){
			CLK_DisableModuleClock(DAC_MODULE);
		}
		else if(self->dac_channel == 1){
			CLK_DisableModuleClock(DAC_MODULE);
		}
		return;
	}

	if(self->dac_channel == 0){
		CLK_EnableModuleClock(DAC_MODULE);
	}
	else if(self->dac_channel == 1){
		CLK_EnableModuleClock(DAC_MODULE);
	}
}

STATIC void switch_pinfun(const pyb_dac_obj_t *self, bool bDAC){

	const pin_obj_t *dac_out_pin;

    if (0) {
    #if defined(MICROPY_HW_DAC0_OUT)
    } else if (self->dac_channel == 0) {
		dac_out_pin = MICROPY_HW_DAC0_OUT;
	#endif
    #if defined(MICROPY_HW_DAC1_OUT)
    } else if (self->dac_channel == 1) {
		dac_out_pin = MICROPY_HW_DAC1_OUT;
	#endif
    } else {
        // DAC does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

	if(bDAC == true){
		uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;
		printf("switch pin fun \n");

		mp_hal_pin_config_alt(dac_out_pin, mode, AF_FN_DAC, self->dac_channel);

		if (0) {
		#if defined(MICROPY_HW_DAC0_OUT)
		} else if(dac_out_pin == pin_B12){
			PB->MODE &= ~GPIO_MODE_MODE12_Msk;
			GPIO_DISABLE_DIGITAL_PATH(PB, (1ul << 12));		
		#endif
		#if defined(MICROPY_HW_DAC1_OUT)
		}
		else if(dac_out_pin == pin_B13){
			PB->MODE &= ~GPIO_MODE_MODE13_Msk;
			GPIO_DISABLE_DIGITAL_PATH(PB, (1ul << 13));
		#endif
		}

	}
	else{
		mp_hal_pin_config(dac_out_pin, GPIO_MODE_INPUT, 0);

		if (0) {
		#if defined(MICROPY_HW_DAC0_OUT)
		} else if(dac_out_pin == pin_B12){
			GPIO_ENABLE_DIGITAL_PATH(PB, (1ul << 12));
		#endif
		#if defined(MICROPY_HW_DAC1_OUT)
		}
		else if(dac_out_pin == pin_B13){
			GPIO_ENABLE_DIGITAL_PATH(PB, (1ul << 13));
		#endif
		}

	}
}

STATIC TIMER_T * TIM3_Config(pyb_dac_obj_t *self, uint freq) {
    // Init TIM3 at the required frequency (in Hz)
	mp_obj_t timer_obj = pytimer_get_timerobj(3);
	if(timer_obj == NULL)
		return NULL;
	
	
   // timer.init(freq=xxxx, mode=TIMER_PERIODIC_MODE)
	const mp_obj_t args[6] = {
		(mp_obj_t)&pyb_timer_init_obj,
		timer_obj,
		MP_OBJ_NEW_QSTR(MP_QSTR_freq),  MP_OBJ_NEW_SMALL_INT(freq),
		MP_OBJ_NEW_QSTR(MP_QSTR_mode),  MP_OBJ_NEW_SMALL_INT(TIMER_PERIODIC_MODE)
	};
	mp_call_method_n_kw(0, 2, args);
	self->timer_obj = timer_obj;

	return TIMER3;
}

STATIC TIMER_T * TIMx_Config(mp_obj_t timer_obj, uint32_t *puTrigger) {
	TIMER_T *timer =  pyb_timer_get_handle(timer_obj);
	
	if(timer == NULL)
		return NULL;
		
	if(timer == TIMER0){
		*puTrigger = DAC_TIMER0_TRIGGER;
	}
	else if (timer == TIMER1){
		*puTrigger = DAC_TIMER1_TRIGGER;
	}
	else if (timer == TIMER2){
		*puTrigger = DAC_TIMER2_TRIGGER;
	}
	else if (timer == TIMER3){
		*puTrigger = DAC_TIMER3_TRIGGER;
	}
	return timer;
}


static void dac_dma_handler_tx(uint32_t id, uint32_t event_dma)
{
    pyb_dac_obj_t *self = (pyb_dac_obj_t *) id;

    PDMA_T *pdma_base = dma_modbase();
	
	/* Re-Set transfer count and basic operation mode */
	if(self->bits == 8)
		PDMA_SetTransferCnt(pdma_base, self->dma_chan_id, PDMA_WIDTH_8 , self->dma_trans_len);
	else
		PDMA_SetTransferCnt(pdma_base, self->dma_chan_id, PDMA_WIDTH_16 , self->dma_trans_len / 2);

	PDMA_SetTransferMode(PDMA, self->dma_chan_id, self->pdma_perp_tx, FALSE, 0);
}

/******************************************************************************/
// MicroPython bindings

#define DAC_CTL_BWSEL_8BIT                (0x1ul << DAC_CTL_BWSEL_Pos)                      /*!< DAC_T::CTL: BWSEL 8 bits                 */

STATIC mp_obj_t pyb_dac_init_helper(pyb_dac_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 8} },
//        { MP_QSTR_buffering, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
    };

   // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);


	if(self->state != DAC_STATE_RESET)
		DAC_Close(self->dac, 0);

    // set bit resolution
    if (args[0].u_int == 8 || args[0].u_int == 12) {
        self->bits = args[0].u_int;
    } else {
        mp_raise_ValueError("unsupported bits");
    }

	switch_pinfun(self, true);
	
	enable_dac_clock(self, true);

    /* Set the software trigger DAC and enable D/A converter */
    DAC_Open(self->dac, 0, DAC_SOFTWARE_TRIGGER);

    /* The DAC conversion settling time is 1us */
    DAC_SetDelayTime(self->dac, 1);

    /* Clear the DAC conversion complete finish flag for safe */
    DAC_CLR_INT_FLAG(self->dac, 0);

	if(self->bits == 8){
		(self->dac)->CTL |= DAC_CTL_BWSEL_8BIT;
	}
	else{
		(self->dac)->CTL &= ~DAC_CTL_BWSEL_Msk;
	}

    // reset state of DAC
    self->state = DAC_STATE_WRITE_SINGLE;

    return mp_const_none;
}


STATIC void pyb_dac_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_dac_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "DAC(%u, bits=%u)",
        self->dac_channel,
        self->bits);
}

STATIC mp_obj_t pyb_dac_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

   // get the timer id
    mp_int_t dac_id = mp_obj_get_int(args[0]);

    // check if the timer exists
    if (dac_id < 0 || dac_id >= M48X_MAX_DAC_INST) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "DAC(%d) doesn't exist", dac_id));
    }

    pyb_dac_obj_t *self = &pyb_dac_obj[dac_id];

	if(self->dac == NULL){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "DAC(%d) is not suuport ", dac_id));
	}
	
    // configure the peripheral
	if (n_args > 1 || n_kw > 0) {
		mp_map_t kw_args;
		mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
		pyb_dac_init_helper(self, n_args - 1, args + 1, &kw_args);
	}
    // return object
    return self;

}

STATIC mp_obj_t pyb_dac_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_dac_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_dac_init_obj, 1, pyb_dac_init);

/// \method deinit()
/// Turn off the DAC, enable other use of pin.
STATIC mp_obj_t pyb_dac_deinit(mp_obj_t self_in) {
    pyb_dac_obj_t *self = self_in;

	//Close PDMA, if enable dma wavefrom
	if(self->state == DAC_STATE_DMA_WAVEFORM){
		PDMA_T *pdma_base = dma_modbase();
		PDMA_DisableInt(pdma_base, self->dma_chan_id , PDMA_INT_TRANS_DONE);
		pdma_base->CHCTL &= ~(1 << self->dma_chan_id);

		dma_channel_free(self->dma_chan_id);
		self->dma_chan_id = DMA_ERROR_OUT_OF_CHANNELS;
		
		//deinit timer
		if(self->timer_obj != mp_const_none){
			// timer.deinit()
			mp_call_function_1((mp_obj_t)&pyb_timer_deinit_obj, self->timer_obj);			
			self->timer_obj = mp_const_none;
		}
		DAC_DISABLE_PDMA(self->dac);
	}
	
	
	DAC_Close(self->dac, 0);

	switch_pinfun(self, false);
	
	enable_dac_clock(self, false);

    // reset state of DAC
    self->state = DAC_STATE_RESET;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_dac_deinit_obj, pyb_dac_deinit);

/// \method write(value)
/// Direct access to the DAC output (8 bit only at the moment).
STATIC mp_obj_t pyb_dac_write(mp_obj_t self_in, mp_obj_t val) {
    pyb_dac_obj_t *self = self_in;

    if (self->state != DAC_STATE_WRITE_SINGLE) {
		DAC_Close(self->dac, 0);
		/* Set the software trigger DAC and enable D/A converter */
		DAC_Open(self->dac, 0, DAC_SOFTWARE_TRIGGER);

		/* The DAC conversion settling time is 1us */
		DAC_SetDelayTime(self->dac, 1);

		/* Clear the DAC conversion complete finish flag for safe */
		DAC_CLR_INT_FLAG(self->dac, 0);

        self->state = DAC_STATE_WRITE_SINGLE;
    }

    // DAC output is always 12-bit at the hardware level, and we provide support
    // for multiple bit "resolutions" simply by shifting the input value.
	DAC_WRITE_DATA(self->dac, 0, mp_obj_get_int(val));

    /* Start A/D conversion */
    DAC_START_CONV(self->dac);	 

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_dac_write_obj, pyb_dac_write);


mp_obj_t pyb_dac_write_timed(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_data, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_freq, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_mode, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DMA_NORMAL} },
    };

    // parse args
    pyb_dac_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the data to write
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);


	uint32_t dac_trigger = DAC_TIMER3_TRIGGER;
	TIMER_T *timer;
    if (mp_obj_is_integer(args[1].u_obj)) {
        // set TIM6 to trigger the DAC at the given frequency
        timer = TIM3_Config(self, mp_obj_get_int(args[1].u_obj));
        dac_trigger = DAC_TIMER3_TRIGGER;
    } else {
        // set the supplied timer to trigger the DAC (timer should be initialised)
        timer = TIMx_Config(args[1].u_obj, &dac_trigger);
    }

	if(timer == NULL){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "DAC(%d) need a timer", self->dac_channel));
	}


	//setup pdma
    PDMA_T *pdma_base = dma_modbase();

	self->dma_chan_id = dma_channel_allocate(DMA_CAP_NONE);

	if(self->dma_chan_id == DMA_ERROR_OUT_OF_CHANNELS){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "DAC(%d) need a dma channel", self->dac_channel));
	}

	pdma_base->CHCTL |= 1 << self->dma_chan_id;  // Enable this DMA channel

	PDMA_SetTransferMode(pdma_base, self->dma_chan_id,
						 self->pdma_perp_tx,    // Peripheral connected to this PDMA
						 0,  // Scatter-gather disabled
						 0); // Scatter-gather descriptor address

	self->dma_trans_len = bufinfo.len;
	if(self->bits == 8)
		PDMA_SetTransferCnt(pdma_base, self->dma_chan_id, PDMA_WIDTH_8 , bufinfo.len);
	else
		PDMA_SetTransferCnt(pdma_base, self->dma_chan_id, PDMA_WIDTH_16 , bufinfo.len / 2);

    /* transfer width is one word(32 bit) */
    PDMA_SetTransferAddr(pdma_base, self->dma_chan_id, (uint32_t)bufinfo.buf, PDMA_SAR_INC, (uint32_t)&self->dac->DAT, PDMA_DAR_FIX);

    /* Set transfer type and burst size */
    PDMA_SetBurstType(pdma_base, self->dma_chan_id, PDMA_REQ_SINGLE, PDMA_BURST_128);

	if(args[2].u_int == DMA_CIRCULAR){
		PDMA_EnableInt(pdma_base, self->dma_chan_id,
                       PDMA_INT_TRANS_DONE);   // Interrupt type
       
         // Register DMA event handler
        dma_set_handler(self->dma_chan_id, (uint32_t) dac_dma_handler_tx, (uint32_t) self, DMA_EVENT_ALL);
	}


    if (self->state != DAC_STATE_DMA_WAVEFORM) {
		DAC_Close(self->dac, 0);
    }

	/* Set the software trigger DAC and enable D/A converter */
	DAC_Open(self->dac, 0, dac_trigger);

	/* The DAC conversion settling time is 1us */
	DAC_SetDelayTime(self->dac, 1);

	/* Clear the DAC conversion complete finish flag for safe */
	DAC_CLR_INT_FLAG(self->dac, 0);

    /* Enable the PDMA Mode */
    DAC_ENABLE_PDMA(self->dac);

	self->state = DAC_STATE_DMA_WAVEFORM;

	TIMER_Stop(timer);
	TIMER_ResetCounter(timer);
    TIMER_SetTriggerTarget(timer, TIMER_TRG_TO_DAC);
    TIMER_Start(timer);	


    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_dac_write_timed_obj, 1, pyb_dac_write_timed);



STATIC const mp_rom_map_elem_t pyb_dac_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_dac_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_dac_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&pyb_dac_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_timed), MP_ROM_PTR(&pyb_dac_write_timed_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_NORMAL), MP_ROM_INT(DMA_NORMAL) },
    { MP_ROM_QSTR(MP_QSTR_CIRCULAR), MP_ROM_INT(DMA_CIRCULAR) },

#if 0
    { MP_ROM_QSTR(MP_QSTR_noise), MP_ROM_PTR(&pyb_dac_noise_obj) },
    { MP_ROM_QSTR(MP_QSTR_triangle), MP_ROM_PTR(&pyb_dac_triangle_obj) },
#endif
};

STATIC MP_DEFINE_CONST_DICT(pyb_dac_locals_dict, pyb_dac_locals_dict_table);

const mp_obj_type_t pyb_dac_type = {
    { &mp_type_type },
    .name = MP_QSTR_DAC,
    .print = pyb_dac_print,
    .make_new = pyb_dac_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_dac_locals_dict,
};

#endif // MICROPY_HW_ENABLE_DAC
