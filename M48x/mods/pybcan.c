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
#include "pybpin.h"
#include "pybcan.h"

#if MICROPY_HW_ENABLE_CAN

enum {
    CAN_STATE_ERROR_ACTIVE = 0,
    CAN_STATE_ERROR_WARNING = 1,
    CAN_STATE_ERROR_PASSIVE = 2,
    CAN_STATE_BUS_OFF = 4,
    CAN_STATE_STOPPED = 8,
};

typedef struct _pyb_can_obj_t {
    mp_obj_base_t base;
    mp_uint_t can_id;
    CAN_T *can;
    IRQn_Type irqn;
    bool is_enabled;
    bool extframe;
    mp_obj_t callback;
    int32_t mode;
    uint16_t num_error_warning;
    uint16_t num_error_passive;
    uint16_t num_bus_off;
//    byte rx_state0;
//    byte rx_state1;
} pyb_can_obj_t;

#define M48X_MAX_CAN_INST 2

STATIC pyb_can_obj_t pyb_can_obj[M48X_MAX_CAN_INST] = {
#if defined(MICROPY_HW_CAN0_RXD)
    {{&pyb_can_type}, 0, CAN0, CAN0_IRQn, FALSE, FALSE, mp_const_none, 0, 0, 0, 0},
#else
    {{&pyb_can_type}, 0, NULL, CAN0_IRQn, FALSE, FALSE, mp_const_none, 0, 0, 0, 0},
#endif
#if defined(MICROPY_HW_CAN1_RXD)
    {{&pyb_can_type}, 1, CAN1, CAN1_IRQn, FALSE, FALSE, mp_const_none, 0, 0, 0, 0},
#else
    {{&pyb_can_type}, 1, NULL, CAN1_IRQn, FALSE, FALSE, mp_const_none, 0, 0, 0, 0},
#endif
};


STATIC void enable_can_clock(const pyb_can_obj_t *self, bool bEnable){

	if(bEnable == false){
		if(self->can_id == 0){
			CLK_DisableModuleClock(CAN0_MODULE);
		}
		else if(self->can_id == 1){
			CLK_DisableModuleClock(CAN1_MODULE);
		}
		return;
	}

	if(self->can_id == 0){
		CLK_EnableModuleClock(CAN0_MODULE);
	}
	else if(self->can_id == 1){
		CLK_EnableModuleClock(CAN1_MODULE);
	}
}

STATIC void switch_pinfun(const pyb_can_obj_t *self, bool bCAN){

	const pin_obj_t *can_txd_pin;
	const pin_obj_t *can_rxd_pin;

    if (0) {
    #if defined(MICROPY_HW_CAN0_RXD)
    } else if (self->can_id == 0) {
		can_txd_pin = MICROPY_HW_CAN0_TXD;
		can_rxd_pin = MICROPY_HW_CAN0_RXD;
	#endif
    #if defined(MICROPY_HW_CAN1_RXD)
    } else if (self->can_id == 1) {
		can_txd_pin = MICROPY_HW_CAN1_TXD;
		can_rxd_pin = MICROPY_HW_CAN1_RXD;
	#endif
    } else {
        // CAN does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

	if(bCAN == true){
		uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;
		printf("switch pin fun \n");

		mp_hal_pin_config_alt(can_txd_pin, mode, AF_FN_CAN, self->can_id);
		mp_hal_pin_config_alt(can_rxd_pin, mode, AF_FN_CAN, self->can_id);
	}
	else{
		mp_hal_pin_config(can_txd_pin, GPIO_MODE_INPUT, 0);
		mp_hal_pin_config(can_rxd_pin, GPIO_MODE_INPUT, 0);
	}
}

/**
  * @brief Get the message object is configured or not.
  * @param[in] tCAN The pointer to CAN module base address.
  * @param[in] u8MsgObj Specifies the Message object number, from 0 to 31.
  * @retval non-zero The corresponding message object is configured.
  * @retval 0 message object is free.
  * @details This function is used to check mesage object is configured or free.
  */
STATIC uint32_t CAN_IsMessageObjConfigured(CAN_T *tCAN, uint8_t u8MsgObj)
{
    return (u8MsgObj < 16ul ? tCAN->MVLD1 & (1ul << u8MsgObj) : tCAN->MVLD2 & (1ul << (u8MsgObj - 16ul)));
}


STATIC bool get_free_message_obj(CAN_T *tCAN, uint32_t timeout, uint32_t *pu32MessageObj) {
    uint32_t start = mp_hal_ticks_ms();
	int i;

    for (;;) {
		for(i = 0; i < 32; i ++){
			if(CAN_IsMessageObjConfigured(tCAN, i) == 0){
				*pu32MessageObj = i;
				return true;
			}
	
		}

        if (mp_hal_ticks_ms() - start >= timeout) {
            return false; // timeout
        }
        MICROPY_EVENT_POLL_HOOK
    }
}


STATIC bool can_receive_with_timeout(CAN_T *tCAN, uint32_t u32MsgNum, STR_CANMSG_T* pCanMsg, uint32_t timeout) {
    uint32_t start = mp_hal_ticks_ms();

    for (;;) {
		if(CAN_Receive(tCAN, u32MsgNum, pCanMsg))
			return true;

        if (mp_hal_ticks_ms() - start >= timeout) {
            return false; // timeout
        }
        MICROPY_EVENT_POLL_HOOK
    }
}

// init(mode, extframe=False, baudrate = 500000)
STATIC mp_obj_t pyb_can_init_helper(pyb_can_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args){
    enum { ARG_mode, ARG_extframe, ARG_baudrate };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,         MP_ARG_REQUIRED | MP_ARG_INT,   {.u_int  = CAN_NORMAL_MODE} },
        { MP_QSTR_extframe,     MP_ARG_BOOL,                    {.u_bool = false} },
        { MP_QSTR_baudrate,		MP_ARG_INT,                     {.u_int  = 500000} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->extframe = args[ARG_extframe].u_bool;

	enable_can_clock(self, true);
	switch_pinfun(self, true);

	self->mode = args[ARG_mode].u_int;
	int32_t i32Baudrate = args[ARG_baudrate].u_int;
	
	if(self->mode == CAN_NORMAL_MODE){
		CAN_Open(self->can,  i32Baudrate, CAN_NORMAL_MODE);
	}
	else{
		CAN_Open(self->can,  i32Baudrate, CAN_BASIC_MODE);
		CAN_EnterTestMode(self->can, self->mode);
	}

    return mp_const_none;
}

STATIC mp_obj_t pyb_can_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_can_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_init_obj, 1, pyb_can_init);

/// \method deinit()
/// Turn off the CAN bus.
STATIC mp_obj_t pyb_can_deinit(mp_obj_t self_in) {
    pyb_can_obj_t *self = self_in;

	CAN_DisableInt(self->can, CAN_CON_IE_Msk);
	self->callback = NULL;

	if(self->mode != CAN_NORMAL_MODE){
		CAN_LeaveTestMode(self->can);
	}

	CAN_Close(self->can);
	
	enable_can_clock(self, false);
	switch_pinfun(self, false);

    self->num_error_warning = 0;
    self->num_error_passive = 0;
    self->num_bus_off = 0;

    self->is_enabled = false;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_can_deinit_obj, pyb_can_deinit);



STATIC mp_obj_t pyb_can_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
	// check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

	// work out port
    mp_uint_t can_idx = mp_obj_get_int(args[0]);

    // check if the can exists
    if (can_idx < 0 || can_idx >= M48X_MAX_CAN_INST) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "CAN(%d) doesn't exist", can_idx));
    }

    pyb_can_obj_t *self = &pyb_can_obj[can_idx];

	if(self->can == NULL){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "CAN(%d) is not suuport ", can_idx));
	}

	if(self->is_enabled){
		// The caller is requesting a reconfiguration of the hardware
		// this can only be done if the hardware is in init mode
		pyb_can_deinit(self);
	}

    // configure the peripheral
	if (n_args > 1 || n_kw > 0) {
		mp_map_t kw_args;
		mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
		pyb_can_init_helper(self, n_args - 1, args + 1, &kw_args);
	}

    // return object
	return self;
}

STATIC void pyb_can_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_can_obj_t *self = self_in;
    if (!self->is_enabled) {
        mp_printf(print, "CAN(%u)", self->can_id);
    } else {
        qstr mode;
        switch (self->mode) {
            case CAN_NORMAL_MODE: mode = MP_QSTR_NORMAL; break;
            case CAN_TEST_LBACK_Msk: mode = MP_QSTR_LOOPBACK; break;
            case CAN_TEST_SILENT_Msk: mode = MP_QSTR_SILENT; break;
            case (CAN_TEST_LBACK_Msk | CAN_TEST_SILENT_Msk): default: mode = MP_QSTR_SILENT_LOOPBACK; break;
        }
        mp_printf(print, "CAN(%u, CAN.%q, extframe=%q)",
            self->can_id,
            mode,
            self->extframe ? MP_QSTR_True : MP_QSTR_False);
    }
}

// Force a software restart of the controller, to allow transmission after a bus error
STATIC mp_obj_t pyb_can_restart(mp_obj_t self_in) {
    pyb_can_obj_t *self = MP_OBJ_TO_PTR(self_in);

	CAN_EnterInitMode(self->can, 0);
	mp_hal_delay_ms(1000);
	CAN_LeaveInitMode(self->can);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_can_restart_obj, pyb_can_restart);

// Get the state of the controller
STATIC mp_obj_t pyb_can_state(mp_obj_t self_in) {
    pyb_can_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t state = CAN_STATE_STOPPED;
    if (self->is_enabled) {
		uint32_t u32Status = CAN_GET_INT_STATUS(self->can);

        if (u32Status & CAN_STATUS_BOFF_Msk) {
            state = CAN_STATE_BUS_OFF;
        } else if (u32Status & CAN_STATUS_EPASS_Msk) {
            state = CAN_STATE_ERROR_PASSIVE;
        } else if (u32Status & CAN_STATUS_EWARN_Msk) {
            state = CAN_STATE_ERROR_WARNING;
        } else {
            state = CAN_STATE_ERROR_ACTIVE;
        }
    }
    return MP_OBJ_NEW_SMALL_INT(state);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_can_state_obj, pyb_can_state);


/// \method any(fifo)
/// Return `True` if any message waiting on the FIFO, else `False`.
STATIC mp_obj_t pyb_can_any(mp_obj_t self_in, mp_obj_t fifo_in) {
    pyb_can_obj_t *self = self_in;
    mp_int_t fifo = mp_obj_get_int(fifo_in);
 
	if((fifo >= 0) && (fifo < 32)){
		if(CAN_IsNewDataReceived(self->can, fifo))
			return mp_const_true;
	}
    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_can_any_obj, pyb_can_any);

/// \method send(send, addr, *, timeout=5000)
/// Send a message on the bus:
///
///   - `send` is the data to send (an integer to send, or a buffer object).
///   - `addr` is the address to send to
///   - `timeout` is the timeout in milliseconds to wait for the send.
///
/// Return value: `None`.
STATIC mp_obj_t pyb_can_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_data, ARG_id, ARG_timeout, ARG_rtr };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_data,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_id,      MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_rtr,     MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };


    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[ARG_data].u_obj, &bufinfo, data);

    if (bufinfo.len > 8) {
        mp_raise_ValueError("CAN data field too long");
    }

	uint32_t u32MessageObj;
	
	if(get_free_message_obj(self->can, args[ARG_timeout].u_int, &u32MessageObj) == false){
       mp_hal_raise(HAL_TIMEOUT);
	}

    STR_CANMSG_T tMsg;
	int i;

	if(self->extframe)
		tMsg.IdType   = CAN_EXT_ID;
	else
		tMsg.IdType   = CAN_STD_ID;

	if(args[ARG_rtr].u_bool)
		tMsg.FrameType= CAN_REMOTE_FRAME;
	else
		tMsg.FrameType= CAN_DATA_FRAME;

    tMsg.Id       = args[ARG_id].u_int;
    tMsg.DLC      = bufinfo.len;

	for(i = 0; i < bufinfo.len; i ++)
		tMsg.Data[i] = ((byte*)bufinfo.buf)[i];

    if(CAN_Transmit(self->can, MSG(u32MessageObj),&tMsg) == FALSE){
 		mp_hal_raise(HAL_ERROR);
    }

    return mp_const_none;

}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_send_obj, 1, pyb_can_send);

/// \method recv(fifo, list=None, *, timeout=5000)
///
/// Receive data on the bus:
///
///   - `fifo` is an integer, which is the FIFO to receive on
///   - `list` if not None is a list with at least 4 elements
///   - `timeout` is the timeout in milliseconds to wait for the receive.
///
/// Return value: buffer of data bytes.
STATIC mp_obj_t pyb_can_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_fifo, ARG_list, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_fifo,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_list,    MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	uint32_t fifo = args[ARG_fifo].u_int;
	
	if(fifo >=  32){
        mp_raise_ValueError("fifo index is invalid");
	} 

	STR_CANMSG_T sCanMsg;

	if(can_receive_with_timeout(self->can, fifo, &sCanMsg, args[ARG_timeout].u_int) == false){
       mp_hal_raise(HAL_TIMEOUT);
	}

    // Create the tuple, or get the list, that will hold the return values
    // Also populate the fourth element, either a new bytes or reuse existing memoryview
    mp_obj_t ret_obj = args[ARG_list].u_obj;
    mp_obj_t *items;
    if (ret_obj == mp_const_none) {
        ret_obj = mp_obj_new_tuple(4, NULL);
        items = ((mp_obj_tuple_t*)MP_OBJ_TO_PTR(ret_obj))->items;
        items[3] = mp_obj_new_bytes(&sCanMsg.Data[0], sCanMsg.DLC);
    } else {
        // User should provide a list of length at least 4 to hold the values
        if (!MP_OBJ_IS_TYPE(ret_obj, &mp_type_list)) {
            mp_raise_TypeError(NULL);
        }
        mp_obj_list_t *list = MP_OBJ_TO_PTR(ret_obj);
        if (list->len < 4) {
            mp_raise_ValueError(NULL);
        }
        items = list->items;
        // Fourth element must be a memoryview which we assume points to a
        // byte-like array which is large enough, and then we resize it inplace
        if (!MP_OBJ_IS_TYPE(items[3], &mp_type_memoryview)) {
            mp_raise_TypeError(NULL);
        }
        mp_obj_array_t *mv = MP_OBJ_TO_PTR(items[3]);
        if (!(mv->typecode == (0x80 | BYTEARRAY_TYPECODE)
            || (mv->typecode | 0x20) == (0x80 | 'b'))) {
            mp_raise_ValueError(NULL);
        }
        mv->len = sCanMsg.DLC;
        memcpy(mv->items, &sCanMsg.Data[0], sCanMsg.DLC);
	}

    // Populate the first 3 values of the tuple/list
    items[0] = MP_OBJ_NEW_SMALL_INT(sCanMsg.Id);
    items[1] = sCanMsg.FrameType == CAN_REMOTE_FRAME ? mp_const_true : mp_const_false;
    items[2] = MP_OBJ_NEW_SMALL_INT(0);

    // Return the result
    return ret_obj;

}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_recv_obj, 1, pyb_can_recv);

/// Configures a filter mask
/// Return value: `None`.
STATIC mp_obj_t pyb_can_setfilter(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_id, ARG_fifo, ARG_mask };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,      	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_fifo,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_mask,   	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    };

    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);


	uint32_t fifo = args[ARG_fifo].u_int;
	
	if(fifo >=  32){
        mp_raise_ValueError("fifo index is invalid");
	} 

	uint32_t u32IDType = CAN_STD_ID;
	
	if(self->extframe)
		u32IDType = CAN_EXT_ID;

	if(!CAN_SetRxMsgAndMsk(self->can, fifo, u32IDType, args[ARG_id].u_int, args[ARG_mask].u_int)){
		mp_raise_ValueError("CAN filter parameter error");
	}

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_setfilter_obj, 1, pyb_can_setfilter);

STATIC mp_obj_t pyb_can_rxcallback(mp_obj_t self_in, mp_obj_t callback_in) {
    pyb_can_obj_t *self = self_in;

	if (callback_in == mp_const_none) {
		CAN_DisableInt(self->can, CAN_CON_IE_Msk);
		self->callback = mp_const_none;
	} else if (self->callback != mp_const_none) {
        // Rx call backs has already been initialized
        // only the callback function should be changed
        self->callback = callback_in;
    } else if (mp_obj_is_callable(callback_in)) {
		CAN_DisableInt(self->can, CAN_CON_IE_Msk);
		self->callback = callback_in;
		CAN_EnableInt(self->can, CAN_CON_IE_Msk);
		NVIC_EnableIRQ(self->irqn);
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_can_rxcallback_obj, pyb_can_rxcallback);

// Get info about error states and TX/RX buffers
STATIC mp_obj_t pyb_can_info(size_t n_args, const mp_obj_t *args) {

    pyb_can_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_list_t *list;
    if (n_args == 1) {
        list = MP_OBJ_TO_PTR(mp_obj_new_list(5, NULL));
    } else {
        if (!MP_OBJ_IS_TYPE(args[1], &mp_type_list)) {
            mp_raise_TypeError(NULL);
        }
        list = MP_OBJ_TO_PTR(args[1]);
        if (list->len < 5) {
            mp_raise_ValueError(NULL);
        }
    }

	uint32_t can_err = (self->can)->ERR;

    list->items[0] = MP_OBJ_NEW_SMALL_INT(can_err >> CAN_ERR_TEC_Pos & 0xff);
    list->items[1] = MP_OBJ_NEW_SMALL_INT(can_err >> CAN_ERR_REC_Pos & 0xff);
    list->items[2] = MP_OBJ_NEW_SMALL_INT(self->num_error_warning);
    list->items[3] = MP_OBJ_NEW_SMALL_INT(self->num_error_passive);
    list->items[4] = MP_OBJ_NEW_SMALL_INT(self->num_bus_off);

    return MP_OBJ_FROM_PTR(list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_can_info_obj, 1, 2, pyb_can_info);


STATIC const mp_rom_map_elem_t pyb_can_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_can_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_can_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_restart), MP_ROM_PTR(&pyb_can_restart_obj) },
    { MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&pyb_can_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&pyb_can_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&pyb_can_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_setfilter), MP_ROM_PTR(&pyb_can_setfilter_obj) },
    { MP_ROM_QSTR(MP_QSTR_rxcallback), MP_ROM_PTR(&pyb_can_rxcallback_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&pyb_can_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&pyb_can_recv_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_NORMAL), MP_ROM_INT(CAN_NORMAL_MODE) },
    { MP_ROM_QSTR(MP_QSTR_LOOPBACK), MP_ROM_INT(CAN_TEST_LBACK_Msk) },
    { MP_ROM_QSTR(MP_QSTR_SILENT), MP_ROM_INT(CAN_TEST_SILENT_Msk) },
    { MP_ROM_QSTR(MP_QSTR_SILENT_LOOPBACK), MP_ROM_INT(CAN_TEST_SILENT_Msk | CAN_TEST_LBACK_Msk) },

    // values for CAN.state()
    { MP_ROM_QSTR(MP_QSTR_STOPPED), MP_ROM_INT(CAN_STATE_STOPPED) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_ACTIVE), MP_ROM_INT(CAN_STATE_ERROR_ACTIVE) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_WARNING), MP_ROM_INT(CAN_STATE_ERROR_WARNING) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_PASSIVE), MP_ROM_INT(CAN_STATE_ERROR_PASSIVE) },
    { MP_ROM_QSTR(MP_QSTR_BUS_OFF), MP_ROM_INT(CAN_STATE_BUS_OFF) },

	// value for CAN.cb.reason
    { MP_ROM_QSTR(MP_QSTR_CB_REASON_RX), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_CB_REASON_ERROR_WARNING), MP_ROM_INT(CAN_STATE_ERROR_WARNING) },
    { MP_ROM_QSTR(MP_QSTR_CB_REASON_ERROR_PASSIVE), MP_ROM_INT(CAN_STATE_ERROR_PASSIVE) },
    { MP_ROM_QSTR(MP_QSTR_CB_REASON_ERROR_BUS_OFF), MP_ROM_INT(CAN_STATE_BUS_OFF) },

#if 0
    { MP_ROM_QSTR(MP_QSTR_initfilterbanks), MP_ROM_PTR(&pyb_can_initfilterbanks_obj) },
    { MP_ROM_QSTR(MP_QSTR_clearfilter), MP_ROM_PTR(&pyb_can_clearfilter_obj) },

    // class constants
    // Note: we use the ST constants >> 4 so they fit in a small-int.  The
    // right-shift is undone when the constants are used in the init function.
    { MP_ROM_QSTR(MP_QSTR_MASK16), MP_ROM_INT(MASK16) },
    { MP_ROM_QSTR(MP_QSTR_LIST16), MP_ROM_INT(LIST16) },
    { MP_ROM_QSTR(MP_QSTR_MASK32), MP_ROM_INT(MASK32) },
    { MP_ROM_QSTR(MP_QSTR_LIST32), MP_ROM_INT(LIST32) },

#endif
};

STATIC MP_DEFINE_CONST_DICT(pyb_can_locals_dict, pyb_can_locals_dict_table);

mp_uint_t can_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    pyb_can_obj_t *self = self_in;
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if (flags & MP_STREAM_POLL_RD) {
			int i;
			for(i = 0; i < 32; i++){
				if(CAN_IsNewDataReceived(self->can, i)){
					ret |= MP_STREAM_POLL_RD;
					break;
				}
			}			
        }
        if (flags & MP_STREAM_POLL_WR) {
			uint32_t u32MessageObj;
			if(get_free_message_obj(self->can, 0, &u32MessageObj)){
				ret |= MP_STREAM_POLL_WR;
			}
        }
    } else {
        *errcode = MP_EINVAL;
        ret = -1;
    }
    return ret;
}

STATIC const mp_stream_p_t can_stream_p = {
    //.read = can_read, // is read sensible for CAN?
    //.write = can_write, // is write sensible for CAN?
    .ioctl = can_ioctl,
    .is_text = false,
};

const mp_obj_type_t pyb_can_type = {
    { &mp_type_type },
    .name = MP_QSTR_CAN,
    .print = pyb_can_print,
    .make_new = pyb_can_make_new,
    .protocol = &can_stream_p,
    .locals_dict = (mp_obj_t)&pyb_can_locals_dict,
};

STATIC void can_handle_irq_callback(pyb_can_obj_t *self, uint32_t cb_reason, uint32_t fifo) {

		mp_obj_t callback = self->callback;
	
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
				mp_obj_t args[3];
				args[0] = self;
				args[1] = MP_OBJ_NEW_SMALL_INT(cb_reason);
				args[2] = MP_OBJ_NEW_SMALL_INT(fifo);				
				mp_call_function_n_kw(callback, 3, 0, args);

				nlr_pop();
			} else {
				// Uncaught exception; disable the callback so it doesn't run again.
				self->callback = mp_const_none;
				CAN_DisableInt(self->can, CAN_CON_IE_Msk);
				mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
			}
#if  MICROPY_PY_THREAD
			mp_sched_unlock();
#else
			gc_unlock();
#endif
		}
}


void Handle_CAN_Irq(int32_t i32CanID , uint32_t u32IIDRStatus)
{
	pyb_can_obj_t *self = &pyb_can_obj[i32CanID];
	uint32_t u32CBReason = 0;


	if (self == NULL) {
		return;
	}
	
	if(u32IIDRStatus == 0x00008000){
		uint32_t u32Status;
		u32Status = CAN_GET_INT_STATUS(self->can);
		
        if(u32Status & CAN_STATUS_RXOK_Msk){
            (self->can)->STATUS &= ~CAN_STATUS_RXOK_Msk;   /* Clear Rx Ok status*/
        }

        if(u32Status & CAN_STATUS_TXOK_Msk){
            (self->can)->STATUS &= ~CAN_STATUS_TXOK_Msk;    /* Clear Tx Ok status*/
        }

        if(u32Status & CAN_STATUS_EPASS_Msk){
			u32CBReason |= CAN_STATE_ERROR_PASSIVE;
			self->num_error_passive++;
        }

        if(u32Status & CAN_STATUS_EWARN_Msk){
			u32CBReason |= CAN_STATE_ERROR_WARNING;
			self->num_error_warning++;
        }

        if(u32Status & CAN_STATUS_BOFF_Msk){
			u32CBReason |= CAN_STATE_BUS_OFF;
			self->num_bus_off++;
        }

		if(u32CBReason){
			can_handle_irq_callback(self, u32CBReason, 0);
		}

	}
	else if(u32IIDRStatus != 0){
		
		can_handle_irq_callback(self, u32CBReason, (u32IIDRStatus - 1));
        CAN_CLR_INT_PENDING_BIT(self->can, (u32IIDRStatus -1));      /* Clear Interrupt Pending */
	}
}


#endif
