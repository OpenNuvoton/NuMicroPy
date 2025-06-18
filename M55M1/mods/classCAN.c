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
#include "classCAN.h"
#include "hal/M55M1_CANFD.h"

#if MICROPY_HW_ENABLE_CAN

#define MAX_CANFD_INST 2
#define CAN_STATUS_INT_EVENT (CANFD_IE_BOE_Msk | CANFD_IE_EWE_Msk | CANFD_IE_EPE_Msk | CANFD_IE_RF0NE_Msk | CANFD_IE_RF1NE_Msk)

typedef struct _pyb_can_obj_t {
    mp_obj_base_t base;
    mp_uint_t can_id;
    canfd_t *obj;
    CANFD_InitTypeDef *InitDef;
    bool is_enabled;
    bool extframe;
    mp_obj_t callback;
    int32_t mode;
    uint16_t num_error_warning;
    uint16_t num_error_passive;
    uint16_t num_bus_off;
} pyb_can_obj_t;


enum {
    CAN_STATE_ERROR_ACTIVE = 0,
    CAN_STATE_ERROR_WARNING = 0x1,
    CAN_STATE_ERROR_PASSIVE = 0x2,
    CAN_STATE_BUS_OFF = 0x4,
    CAN_STATE_STOPPED = 0x8,
    CAN_STATE_RX_MSG = 0x10,
};

enum {
    CAN_NORMAL_MODE = 1,
    CAN_LOOPBACK_MODE = 2,
};

#if defined(MICROPY_HW_CANFD0_RXD)
static CANFD_InitTypeDef s_sCANFD0InitDef;
static canfd_t s_sCANFD0Obj = {.canfd = CANFD0, .i32FIFOIdx = -1};
#endif
#if defined(MICROPY_HW_CANFD1_RXD)
static CANFD_InitTypeDef s_sCANFD1InitDef;
static canfd_t s_sCANFD1Obj = {.canfd = CANFD1, .i32FIFOIdx = -1 };
#endif

static pyb_can_obj_t pyb_can_obj[MAX_CANFD_INST] = {
#if defined(MICROPY_HW_CANFD0_RXD)
    {{&machine_can_type}, 0, &s_sCANFD0Obj, &s_sCANFD0InitDef},
#else
    {{&machine_and_type}, -1, NULL, NULL},
#endif
#if defined(MICROPY_HW_CANFD1_RXD)
    {{&machine_can_type}, 1, &s_sCANFD1Obj, &s_sCANFD1InitDef},
#else
    {{&machine_can_type}, -1, NULL, NULL},
#endif
};

static void switch_pinfun(const pyb_can_obj_t *self, bool bCAN)
{

    const pin_obj_t *can_txd_pin;
    const pin_obj_t *can_rxd_pin;

    if (0) {
#if defined(MICROPY_HW_CANFD0_RXD)
    } else if (self->can_id == 0) {
        can_txd_pin = MICROPY_HW_CANFD0_TXD;
        can_rxd_pin = MICROPY_HW_CANFD0_RXD;
#endif
#if defined(MICROPY_HW_CANFD1_RXD)
    } else if (self->can_id == 1) {
        can_txd_pin = MICROPY_HW_CANFD1_TXD;
        can_rxd_pin = MICROPY_HW_CANFD1_RXD;
#endif
    } else {
        // CAN does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

    if(bCAN == true) {
        uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;
        printf("switch pin fun \n");

        mp_hal_pin_config_alt(can_txd_pin, mode, AF_FN_CANFD, self->can_id);
        mp_hal_pin_config_alt(can_rxd_pin, mode, AF_FN_CANFD, self->can_id);
    } else {
        mp_hal_pin_config(can_txd_pin, GPIO_MODE_INPUT, 0);
        mp_hal_pin_config(can_rxd_pin, GPIO_MODE_INPUT, 0);
    }
}




// init(mode, extframe=False, baudrate = 500000)
static mp_obj_t pyb_can_init_helper(pyb_can_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
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

    switch_pinfun(self, true);

    self->mode = args[ARG_mode].u_int;
    int32_t i32Baudrate = args[ARG_baudrate].u_int;
    int i32Ret;

    self->callback = mp_const_none;
    self->InitDef->NormalBitRate = i32Baudrate;
    self->InitDef->DataBitRate = i32Baudrate;
    self->InitDef->FDMode = FALSE;

    if(self->mode == CAN_LOOPBACK_MODE)
        self->InitDef->LoopBack = TRUE;
    else
        self->InitDef->LoopBack = FALSE;

    i32Ret = CANFD_Init(self->obj, self->InitDef);

    if(i32Ret != 0) {
        printf("Unable init canfd. Error %d \n", i32Ret);
    }

    return mp_const_none;
}

static mp_obj_t pyb_can_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    return pyb_can_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_init_obj, 1, pyb_can_init);


/// \method deinit()
/// Turn off the CAN bus.
static mp_obj_t pyb_can_deinit(mp_obj_t self_in)
{
    pyb_can_obj_t *self = self_in;

    self->callback = mp_const_none;

    CANFD_DisableStatusInt(self->obj, CAN_STATUS_INT_EVENT);
    CANFD_Final(self->obj);

    switch_pinfun(self, false);

    self->num_error_warning = 0;
    self->num_error_passive = 0;
    self->num_bus_off = 0;

    self->is_enabled = false;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_can_deinit_obj, pyb_can_deinit);


static mp_obj_t pyb_can_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // work out SPI bus
    int can_idx = -1;

    can_idx = mp_obj_get_int(args[0]);
    if (can_idx < 0 || can_idx >= MP_ARRAY_SIZE(pyb_can_obj)
        || pyb_can_obj[can_idx].obj == NULL) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
                                                "CAN(%d) doesn't exist", can_idx));
    }

    pyb_can_obj_t *self = (pyb_can_obj_t *)&pyb_can_obj[can_idx];

    if(self->is_enabled) {
        // The caller is requesting a reconfiguration of the hardware
        // this can only be done if the hardware is in init mode
        pyb_can_deinit(self);
    }

    // configure the peripheral
    if (n_args >= 1 || n_kw >= 3) {
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        pyb_can_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    // return object
    return self;
}

static void pyb_can_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_can_obj_t *self = self_in;
    if (!self->is_enabled) {
        mp_printf(print, "CAN(%u)", self->can_id);
    } else {
        qstr mode = MP_QSTR_NORMAL;
        switch (self->mode) {
        case CAN_NORMAL_MODE:
            mode = MP_QSTR_NORMAL;
            break;
        case CAN_LOOPBACK_MODE:
            mode = MP_QSTR_LOOPBACK;
            break;
        }
        mp_printf(print, "CAN(%u, CAN.%q, extframe=%q)",
                  self->can_id,
                  mode,
                  self->extframe ? MP_QSTR_True : MP_QSTR_False);
    }
}

// Force a software restart of the controller, to allow transmission after a bus error
static mp_obj_t pyb_can_restart(mp_obj_t self_in)
{
    pyb_can_obj_t *self = MP_OBJ_TO_PTR(self_in);

    CANFD_Restart(self->obj);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_can_restart_obj, pyb_can_restart);

// Get the state of the controller
static mp_obj_t pyb_can_state(mp_obj_t self_in)
{
    pyb_can_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t state = CAN_STATE_STOPPED;
    uint32_t u32IntTypeFlag = CANFD_IR_BO_Msk | CANFD_IR_EW_Msk | CANFD_IR_EP_Msk;


    if (self->is_enabled) {
        uint32_t u32Status = CANFD_GetStatusFlag(self->obj->canfd, u32IntTypeFlag);

        if (u32Status & CANFD_IR_BO_Msk) {
            state = CAN_STATE_BUS_OFF;
        } else if (u32Status & CANFD_IR_EP_Msk) {
            state = CAN_STATE_ERROR_PASSIVE;
        } else if (u32Status & CANFD_IR_EW_Msk) {
            state = CAN_STATE_ERROR_WARNING;
        } else {
            state = CAN_STATE_ERROR_ACTIVE;
        }
    }
    return MP_OBJ_NEW_SMALL_INT(state);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_can_state_obj, pyb_can_state);

/// \method any(fifo)
/// Return `True` if any message waiting on the FIFO, else `False`.
static mp_obj_t pyb_can_any(mp_obj_t self_in)
{
    pyb_can_obj_t *self = self_in;

    if(CANFD_AmountDataRecv(self->obj) > 0)
        return mp_const_true;

    return mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_can_any_obj, pyb_can_any);


/// Configures a filter mask
/// Return value: `None`.
static mp_obj_t pyb_can_setfilter(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_id, ARG_mask, ARG_extid };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,      	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_mask,   	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_extid,    MP_ARG_BOOL,                  {.u_bool = false} },
    };

    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);


    bool extid = args[ARG_extid].u_bool;

    E_CANFD_ID_TYPE eIDType = eCANFD_SID;

    if(extid)
        eIDType = eCANFD_XID;

    if(CANFD_SetRecvFilter(self->obj, eIDType, args[ARG_id].u_int, args[ARG_mask].u_int) != 0) {
        mp_raise_ValueError("CAN filter parameter error");
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_setfilter_obj, 1, pyb_can_setfilter);


// Get info about error states and TX/RX buffers
static mp_obj_t pyb_can_info(size_t n_args, const mp_obj_t *args)
{

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

    uint8_t u8TxBufErr = 0;
    uint8_t u8RxBufErr = 0;

    CANFD_GetBusErrCount(self->obj->canfd, &u8TxBufErr, &u8RxBufErr);

    list->items[0] = MP_OBJ_NEW_SMALL_INT(u8TxBufErr);
    list->items[1] = MP_OBJ_NEW_SMALL_INT(u8RxBufErr);
    list->items[2] = MP_OBJ_NEW_SMALL_INT(self->num_error_warning);
    list->items[3] = MP_OBJ_NEW_SMALL_INT(self->num_error_passive);
    list->items[4] = MP_OBJ_NEW_SMALL_INT(self->num_bus_off);

    return MP_OBJ_FROM_PTR(list);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_can_info_obj, 1, 2, pyb_can_info);


/// \method send(send, addr, *, timeout=5000)
/// Send a message on the bus:
///
///   - `send` is the data to send (an integer to send, or a buffer object).
///   - `addr` is the address to send to
///   - `timeout` is the timeout in milliseconds to wait for the send.
///
/// Return value: `None`.
static mp_obj_t pyb_can_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
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

    if ((bufinfo.len != 8) &&
        (bufinfo.len != 12) &&
        (bufinfo.len != 16) &&
        (bufinfo.len != 20) &&
        (bufinfo.len != 24) &&
        (bufinfo.len != 32) &&
        (bufinfo.len != 48) &&
        (bufinfo.len != 64)) {
        mp_raise_ValueError("CAN data field too long");
    }

    int availMsgObj;

    //Get available message object index. available: 0~31
    availMsgObj = CANFD_GetFreeMsgObjIdx(self->obj, args[ARG_timeout].u_int);
    if(availMsgObj < 0) {
        mp_hal_raise(HAL_TIMEOUT);
    }

    CANFD_FD_MSG_T tMsg;
    int i;

    if(self->extframe)
        tMsg.eIdType   = eCANFD_XID;
    else
        tMsg.eIdType   = eCANFD_SID;

    if(args[ARG_rtr].u_bool)
        tMsg.eFrmType= eCANFD_REMOTE_FRM;
    else
        tMsg.eFrmType= eCANFD_DATA_FRM;

    /* Set FD frame format attribute */
    tMsg.bFDFormat = 0;

    tMsg.u32Id       = args[ARG_id].u_int;
    tMsg.u32DLC      = bufinfo.len;

    for(i = 0; i < bufinfo.len; i ++)
        tMsg.au8Data[i] = ((byte*)bufinfo.buf)[i];

    if(CANFD_TransmitTxMsg(self->obj->canfd, availMsgObj, &tMsg) != eCANFD_TRANSMIT_SUCCESS) {
        mp_hal_raise(HAL_ERROR);
    }

    return mp_const_none;

}
static MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_send_obj, 1, pyb_can_send);


/// \method recv(fifo, list=None, *, timeout=5000)
///
/// Receive data on the bus:
///
///   - `fifo` is an integer, which is the FIFO to receive on
///   - `list` if not None is a list with at least 4 elements
///   - `timeout` is the timeout in milliseconds to wait for the receive.
///
/// Return value: buffer of data bytes.
static mp_obj_t pyb_can_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_list, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_list,    MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);


    CANFD_FD_MSG_T tCanMsg;

    if(CANFD_RecvMsg(self->obj, &tCanMsg, args[ARG_timeout].u_int) != 0) {
        mp_hal_raise(HAL_TIMEOUT);
    }

    // Create the tuple, or get the list, that will hold the return values
    // Also populate the fourth element, either a new bytes or reuse existing memoryview
    mp_obj_t ret_obj = args[ARG_list].u_obj;
    mp_obj_t *items;
    if (ret_obj == mp_const_none) {
        ret_obj = mp_obj_new_tuple(4, NULL);
        items = ((mp_obj_tuple_t*)MP_OBJ_TO_PTR(ret_obj))->items;
        items[3] = mp_obj_new_bytes(&tCanMsg.au8Data[0], tCanMsg.u32DLC);
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
        mv->len = tCanMsg.u32DLC;
        memcpy(mv->items, &tCanMsg.au8Data[0], tCanMsg.u32DLC);
    }

    // Populate the first 3 values of the tuple/list
    items[0] = MP_OBJ_NEW_SMALL_INT(tCanMsg.u32Id);
    items[1] = tCanMsg.eIdType == eCANFD_SID ? mp_const_true : mp_const_false;
    items[2] = tCanMsg.eFrmType == eCANFD_REMOTE_FRM ? mp_const_true : mp_const_false;

    // Return the result
    return ret_obj;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_recv_obj, 1, pyb_can_recv);


static void CAN_StatusInt_Handler(void *obj, uint32_t u32Status);

static mp_obj_t pyb_can_rxcallback(mp_obj_t self_in, mp_obj_t callback_in)
{
    pyb_can_obj_t *self = self_in;

    if (callback_in == mp_const_none) {
        CANFD_DisableStatusInt(self->obj, CAN_STATUS_INT_EVENT);
        self->callback = mp_const_none;
    } else if (self->callback != mp_const_none) {
        // Rx call backs has already been initialized
        // only the callback function should be changed
        self->callback = callback_in;
    } else if (mp_obj_is_callable(callback_in)) {
        CANFD_DisableStatusInt(self->obj, CAN_STATUS_INT_EVENT);
        self->callback = callback_in;
        CANFD_EnableStatusInt(self->obj, CAN_StatusInt_Handler, CAN_STATUS_INT_EVENT);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(pyb_can_rxcallback_obj, pyb_can_rxcallback);


static const mp_rom_map_elem_t pyb_can_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_can_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_can_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_restart), MP_ROM_PTR(&pyb_can_restart_obj) },
    { MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&pyb_can_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&pyb_can_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_setfilter), MP_ROM_PTR(&pyb_can_setfilter_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&pyb_can_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&pyb_can_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&pyb_can_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_rxcallback), MP_ROM_PTR(&pyb_can_rxcallback_obj) },


    // class constants
    { MP_ROM_QSTR(MP_QSTR_NORMAL), MP_ROM_INT(CAN_NORMAL_MODE) },
    { MP_ROM_QSTR(MP_QSTR_LOOPBACK), MP_ROM_INT(CAN_LOOPBACK_MODE) },

    // values for CAN.state()
    { MP_ROM_QSTR(MP_QSTR_STOPPED), MP_ROM_INT(CAN_STATE_STOPPED) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_ACTIVE), MP_ROM_INT(CAN_STATE_ERROR_ACTIVE) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_WARNING), MP_ROM_INT(CAN_STATE_ERROR_WARNING) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_PASSIVE), MP_ROM_INT(CAN_STATE_ERROR_PASSIVE) },
    { MP_ROM_QSTR(MP_QSTR_BUS_OFF), MP_ROM_INT(CAN_STATE_BUS_OFF) },

    // value for CAN.cb.reason
    { MP_ROM_QSTR(MP_QSTR_CB_REASON_RX), MP_ROM_INT(CAN_STATE_RX_MSG) },
    { MP_ROM_QSTR(MP_QSTR_CB_REASON_ERROR_WARNING), MP_ROM_INT(CAN_STATE_ERROR_WARNING) },
    { MP_ROM_QSTR(MP_QSTR_CB_REASON_ERROR_PASSIVE), MP_ROM_INT(CAN_STATE_ERROR_PASSIVE) },
    { MP_ROM_QSTR(MP_QSTR_CB_REASON_ERROR_BUS_OFF), MP_ROM_INT(CAN_STATE_BUS_OFF) },

};

static MP_DEFINE_CONST_DICT(pyb_can_locals_dict, pyb_can_locals_dict_table);


mp_uint_t can_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode)
{
    pyb_can_obj_t *self = self_in;
    mp_uint_t ret;

    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if (flags & MP_STREAM_POLL_RD) {
            if(CANFD_AmountDataRecv(self->obj) > 0) {
                ret |= MP_STREAM_POLL_RD;
            }
        }

        if (flags & MP_STREAM_POLL_WR) {
            if(CANFD_GetFreeMsgObjIdx(self->obj, 0) >= 0) {
                ret |= MP_STREAM_POLL_WR;
            }
        }
    } else {
        *errcode = MP_EINVAL;
        ret = -1;
    }

    return ret;
}

static const mp_stream_p_t can_stream_p = {
    //.read = can_read, // is read sensible for CAN?
    //.write = can_write, // is write sensible for CAN?
    .ioctl = can_ioctl,
    .is_text = false,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_can_type,
    MP_QSTR_CAN,
    MP_TYPE_FLAG_NONE,
    make_new, pyb_can_make_new,
    print, pyb_can_print,
    protocol, &can_stream_p,
    locals_dict, &pyb_can_locals_dict
);


static void can_handle_irq_callback(pyb_can_obj_t *self, uint32_t cb_reason)
{

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
            args[2] = MP_OBJ_NEW_SMALL_INT(self->obj->i32FIFOIdx);
            mp_call_function_n_kw(callback, 3, 0, args);

            nlr_pop();
        } else {
            // Uncaught exception; disable the callback so it doesn't run again.
            self->callback = mp_const_none;
            CANFD_DisableStatusInt(self->obj, CAN_STATUS_INT_EVENT);
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
#if  MICROPY_PY_THREAD
        mp_sched_unlock();
#else
        gc_unlock();
#endif
    }
}


static pyb_can_obj_t* find_pyb_can_obj(canfd_t *psCANFDObj)
{
    int i;
    int pyb_table_size = sizeof(pyb_can_obj) / sizeof(pyb_can_obj_t);

    for(i = 0; i < pyb_table_size; i++) {
        if(pyb_can_obj[i].obj == psCANFDObj)
            return (pyb_can_obj_t*)&pyb_can_obj[i];
    }

    return NULL;
}

static void CAN_StatusInt_Handler(void *obj, uint32_t u32Status)
{
    canfd_t *psCANFDObj = (canfd_t *)obj;
    uint32_t u32CBReason = 0;

    //search pyb_can_obj
    pyb_can_obj_t *self = find_pyb_can_obj(psCANFDObj);

    if (self == NULL) {
        return;
    }

    if(u32Status & CANFD_IR_EP_Msk) {
        u32CBReason |= CAN_STATE_ERROR_PASSIVE;
        self->num_error_passive++;
    }

    if(u32Status & CANFD_IR_EW_Msk) {
        u32CBReason |= CAN_STATE_ERROR_WARNING;
        self->num_error_warning++;
    }

    if(u32Status & CANFD_IR_BO_Msk) {
        u32CBReason |= CAN_STATE_BUS_OFF;
        self->num_bus_off++;
    }

    if(u32Status & CANFD_IR_RF0N_Msk) {
        u32CBReason |= CAN_STATE_RX_MSG;
    }

    if(u32Status & CANFD_IR_RF1N_Msk) {
        u32CBReason |= CAN_STATE_RX_MSG;
    }

    if(u32CBReason) {
        can_handle_irq_callback(self, u32CBReason);
    }
}

#endif



