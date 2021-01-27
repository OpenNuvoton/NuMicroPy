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
#include "pybi2c.h"
#include "bufhelper.h"

#if MICROPY_HW_ENABLE_HW_I2C

#include "wblib.h"
#include "N9H26_I2C.h"

typedef struct _pyb_i2c_obj_t {
    mp_obj_base_t base;
    int i32Inst;
	bool bInit;
	uint32_t u32BaudRate;
} pyb_i2c_obj_t;

//Enable it, if using NT99141 i2c test
#define USE_NT9941_I2C_TEST

#define PYB_I2C_SPEED_STANDARD (100000L)
#define PYB_I2C_SPEED_FULL     (400000L)
#define PYB_I2C_SPEED_FAST     (1000000L)

#define MICROPY_HW_I2C_BAUDRATE_DEFAULT (PYB_I2C_SPEED_STANDARD)
#define MICROPY_HW_I2C_BAUDRATE_MAX (PYB_I2C_SPEED_FAST)


STATIC pyb_i2c_obj_t pyb_i2c_obj[] = {
    #if defined(MICROPY_HW_I2C0_SCL)
    {{&pyb_i2c_type}, 0, false, MICROPY_HW_I2C_BAUDRATE_DEFAULT},
    #endif
};

#if defined (USE_NT9941_I2C_TEST)
#include "hal/N9H26_VideoIn.h"
extern void videoIn0_Init(
	BOOL bIsEnableSnrClock,
	E_VIDEOIN_SNR_SRC eSnrSrc,	
	UINT32 u32SensorFreqKHz,						//KHz unit
	E_VIDEOIN_DEV_TYPE eDevType
	);
#endif

STATIC void pyb_i2c_init(pyb_i2c_obj_t *self){
	int i2c_unit;
	const pin_obj_t *scl_pin;
	const pin_obj_t *sda_pin;

    if (0) {
    #if defined(MICROPY_HW_I2C0_SCL)
    } else if (self->i32Inst == 0) {
        i2c_unit = 0;
        scl_pin = MICROPY_HW_I2C0_SCL;
        sda_pin = MICROPY_HW_I2C0_SDA;
    #endif
    } else {
        // I2C does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

#if defined (USE_NT9941_I2C_TEST)
	videoIn0_Init(TRUE, eSYS_UPLL, 24000, eVIDEOIN_SNR_CCIR601);
#endif
   // init the GPIO lines
    uint32_t mode = MP_HAL_PIN_MODE_ALT;
    mp_hal_pin_config_alt(scl_pin, mode, AF_FN_I2C, i2c_unit);
    mp_hal_pin_config_alt(sda_pin, mode, AF_FN_I2C, i2c_unit);

	outpw(REG_APBIPRST, inpw(REG_APBIPRST) | I2CRST);	//reset i2c
	outpw(REG_APBIPRST, inpw(REG_APBIPRST) & ~I2CRST);	

	if(i2cOpen() == I2C_ERR_BUSY)
        mp_raise_ValueError("Unable open I2C bus: busy");
	
	i2cIoctl(I2C_IOC_SET_SPEED, self->u32BaudRate, 0);
}

STATIC void pyb_i2c_deinit(pyb_i2c_obj_t *self) {
	const pin_obj_t *scl_pin;
	const pin_obj_t *sda_pin;

	i2cClose();
	
    if (0) {
    #if defined(MICROPY_HW_I2C0_SCL)
    } else if (self->i32Inst == 0) {
        scl_pin = MICROPY_HW_I2C0_SCL;
        sda_pin = MICROPY_HW_I2C0_SDA;
    #endif
    } else {
        // I2C does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

	mp_hal_pin_config(scl_pin, GPIO_MODE_INPUT, 0);
	mp_hal_pin_config(sda_pin, GPIO_MODE_INPUT, 0);
}


/// \method init(mode, *, addr=0x12, baudrate=400000, gencall=False)
///
/// Initialise the I2C bus with the given parameters:
///
///   - `mode` must be either `I2C.MASTER` or `I2C.SLAVE`
///   - `addr` is the 7-bit address (only sensible for a slave)	
///   - `baudrate` is the SCL clock rate (only sensible for a master)
///   - `gencall` is whether to support general call mode
STATIC mp_obj_t pyb_i2c_init_helper(pyb_i2c_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MICROPY_HW_I2C_BAUDRATE_DEFAULT} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	// set the I2C configuration values
	self->u32BaudRate = args[0].u_int;

	// init the I2C bus
	pyb_i2c_deinit(self);
	pyb_i2c_init(self);

    return mp_const_none;
}


/******************************************************************************/
/* MicroPython bindings                                                       */

STATIC void pyb_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_i2c_obj_t *self = self_in;

    int i2c_num = -1;
    if (0) { }
    #if defined(MICROPY_HW_I2C0_SCL)
    else if (self->i32Inst == 0) { i2c_num = 0; }
    #endif

    if (i2c_num < 0) {
        mp_printf(print, "I2C(%u)", i2c_num);
    } else {
		mp_printf(print, "I2C(%u, I2C.MASTER, baudrate=%u)", i2c_num, self->u32BaudRate);
    }
}

/// \classmethod \constructor(bus, ...)
///
/// Construct an I2C object on the given bus.  `bus` can be 0, 1, 2.
/// With no additional parameters, the I2C object is created but not
/// initialised (it has the settings from the last initialisation of
/// the bus, if any).  If extra arguments are given, the bus is initialised.
/// See `init` for parameters of initialisation.
///
/// The physical pins of the I2C busses are:
///
///   - `I2C(0)` is on: `(SCL, SDA) = (D9, D8) = (PA4, PA5)`
///   - `I2C(1)` is on: `(SCL, SDA) = (D13, D10) = (PA2, PA3)`
STATIC mp_obj_t pyb_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
   // work out i2c bus
    int i2c_id = -1;

	i2c_id = mp_obj_get_int(args[0]);
	if (i2c_id < 0 || i2c_id >= MP_ARRAY_SIZE(pyb_i2c_obj)) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
			"I2C(%d) doesn't exist", i2c_id));
	}
	
    // get I2C object
    pyb_i2c_obj_t *i2c_obj = &pyb_i2c_obj[i2c_id];

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        pyb_i2c_init_helper(i2c_obj, n_args - 1, args + 1, &kw_args);
    }

    return (mp_obj_t)i2c_obj;
}

STATIC mp_obj_t pyb_i2c_init_(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_i2c_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_init_obj, 1, pyb_i2c_init_);

/// \method deinit()
/// Turn off the I2C bus.
STATIC mp_obj_t pyb_i2c_deinit_(mp_obj_t self_in) {
    pyb_i2c_obj_t *self = self_in;
    pyb_i2c_deinit(self);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_deinit_obj, pyb_i2c_deinit_);

/// \method send(send, addr=0x00)
/// Send data on the bus:
///
///   - `send` is the data to send (an integer to send, or a buffer object)
///   - `addr` is the address to send to (only required in master mode)
///
/// Return value: `None`.

STATIC mp_obj_t pyb_i2c_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_send,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_addr,    MP_ARG_INT, {.u_int = PYB_I2C_MASTER_ADDRESS} },
    };

    // parse args
    //pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[0].u_obj, &bufinfo, data);

	int32_t send_len = 0;

	if (args[1].u_int == PYB_I2C_MASTER_ADDRESS) {
		mp_raise_TypeError("addr argument required");
	}
	mp_uint_t i2c_addr = args[1].u_int;

    i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, i2c_addr >> 1, 0);
    i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0);
    send_len = i2cWrite(bufinfo.buf, bufinfo.len);
	
	if(send_len != bufinfo.len){
		mp_hal_raise(HAL_ERROR);
	}

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_send_obj, 1, pyb_i2c_send);


/// \method recv(recv, addr=0x00)
///
/// Receive data on the bus:
///
///   - `recv` can be an integer, which is the number of bytes to receive,
///     or a mutable buffer, which will be filled with received bytes
///   - `addr` is the address to receive from (only required in master mode)
///
/// Return value: if `recv` is an integer then a new buffer of the bytes received,
/// otherwise the same buffer that was passed in to `recv`.

STATIC mp_obj_t pyb_i2c_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_recv,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_addr,    MP_ARG_INT, {.u_int = PYB_I2C_MASTER_ADDRESS} },
    };

    // parse args
    //pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    mp_obj_t o_ret = pyb_buf_get_for_recv(args[0].u_obj, &vstr);

	int32_t i32RecvLen = 0;

	if (args[1].u_int == PYB_I2C_MASTER_ADDRESS) {
		mp_raise_TypeError("addr argument required");
	}
	mp_uint_t i2c_addr = args[1].u_int;

    i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, i2c_addr >> 1, 0);
    i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0);
	i32RecvLen = i2cRead((uint8_t*)vstr.buf, vstr.len);

	if(i32RecvLen <= 0){
        mp_hal_raise(HAL_ERROR);
	}

   // return the received data
    if (o_ret != MP_OBJ_NULL) {
        return o_ret;
    } else {
        return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_recv_obj, 1, pyb_i2c_recv);

/// \method mem_read(data, addr, memaddr, addr_size=8)
///
/// Read from the memory of an I2C device:
///
///   - `data` can be an integer or a buffer to read into
///   - `addr` is the I2C device address
///   - `memaddr` is the memory location within the I2C device
///   - `addr_size` selects width of memaddr: 8 or 16 bits
///
/// Returns the read data.
/// This is only valid in master mode.
STATIC const mp_arg_t pyb_i2c_mem_read_allowed_args[] = {
    { MP_QSTR_data,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_memaddr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_addr_size, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
};

STATIC mp_obj_t pyb_i2c_mem_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    //pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args), pyb_i2c_mem_read_allowed_args, args);

   // get the buffer to read into
    vstr_t vstr;
    mp_obj_t o_ret = pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    // get the addresses
    mp_uint_t i2c_addr = args[1].u_int;
    mp_uint_t mem_addr = args[2].u_int;

	if (i2c_addr == PYB_I2C_MASTER_ADDRESS) {
		mp_raise_TypeError("addr argument required");
	}

    // determine width of mem_addr; default is 8 bits, entering any other value gives 16 bit width
    mp_uint_t mem_addr_size = 8;
    if (args[3].u_int != 8) {
        mem_addr_size = 16;
    }

	int32_t i32RecvLen = 0;

    i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, i2c_addr >> 1, 0);
    i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0);

	if(mem_addr_size == 8)
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, mem_addr, 1);
	else
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, mem_addr, 2);
	
	i32RecvLen = i2cRead((uint8_t*)vstr.buf, vstr.len);
	
	if(i32RecvLen == 0){
        mp_hal_raise(HAL_ERROR);
	}

	// return the received data
    if (o_ret != MP_OBJ_NULL) {
        return o_ret;
    } else {
        return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_mem_read_obj, 1, pyb_i2c_mem_read);

/// \method mem_write(data, addr, memaddr, addr_size=8)
///
/// Write to the memory of an I2C device:
///
///   - `data` can be an integer or a buffer to write from
///   - `addr` is the I2C device address
///   - `memaddr` is the memory location within the I2C device
///   - `addr_size` selects width of memaddr: 8 or 16 bits
///
/// Returns `None`.
/// This is only valid in master mode.

STATIC mp_obj_t pyb_i2c_mem_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args (same as mem_read)
    //pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args), pyb_i2c_mem_read_allowed_args, args);

    // get the buffer to write from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[0].u_obj, &bufinfo, data);

    // get the addresses
    mp_uint_t i2c_addr = args[1].u_int;
    mp_uint_t mem_addr = args[2].u_int;
    
	if (i2c_addr == PYB_I2C_MASTER_ADDRESS) {
		mp_raise_TypeError("addr argument required");
	}
    
    // determine width of mem_addr; default is 8 bits, entering any other value gives 16 bit width
    mp_uint_t mem_addr_size = 8;
    if (args[3].u_int != 8) {
        mem_addr_size = 16;
    }

	int32_t i32WriteLen = 0;

    i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, i2c_addr >> 1, 0);
    i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0);

	if(mem_addr_size == 8)
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, mem_addr, 1);
	else
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, mem_addr, 2);
	
	i32WriteLen = i2cWrite(bufinfo.buf, bufinfo.len);

	if(i32WriteLen != bufinfo.len){
		mp_hal_raise(HAL_ERROR);
	}

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_mem_write_obj, 1, pyb_i2c_mem_write);


STATIC const mp_rom_map_elem_t pyb_i2c_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_i2c_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_i2c_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&pyb_i2c_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&pyb_i2c_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_read), MP_ROM_PTR(&pyb_i2c_mem_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_write), MP_ROM_PTR(&pyb_i2c_mem_write_obj) },

    // class constants
};

STATIC MP_DEFINE_CONST_DICT(pyb_i2c_locals_dict, pyb_i2c_locals_dict_table);


const mp_obj_type_t pyb_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = pyb_i2c_print,
    .make_new = pyb_i2c_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_i2c_locals_dict,
};

#endif
