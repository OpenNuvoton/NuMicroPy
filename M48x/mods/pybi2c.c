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

/// \moduleref pyb
/// \class I2C - a two-wire serial protocol
///
/// I2C is a two-wire protocol for communicating between devices.  At the physical
/// level it consists of 2 wires: SCL and SDA, the clock and data lines respectively.
///
/// I2C objects are created attached to a specific bus.  They can be initialised
/// when created, or initialised later on:
///
///     from pyb import I2C
///
///     i2c = I2C(1)                         # create on bus 1
///     i2c = I2C(1, I2C.MASTER)             # create and init as a master
///     i2c.init(I2C.MASTER, baudrate=20000) # init as a master
///     i2c.init(I2C.SLAVE, addr=0x42)       # init as a slave with given address
///     i2c.deinit()                         # turn off the peripheral
///
/// Printing the i2c object gives you information about its configuration.
///
/// Basic methods for slave are send and recv:
///
///     i2c.send('abc')      # send 3 bytes
///     i2c.send(0x42)       # send a single byte, given by the number
///     data = i2c.recv(3)   # receive 3 bytes
///
/// To receive inplace, first create a bytearray:
///
///     data = bytearray(3)  # create a buffer
///     i2c.recv(data)       # receive 3 bytes, writing them into data
///
/// You can specify a timeout (in ms):
///
///     i2c.send(b'123', timeout=2000)   # timout after 2 seconds
///
/// A master must specify the recipient's address:
///
///     i2c.init(I2C.MASTER)
///     i2c.send('123', 0x42)        # send 3 bytes to slave with address 0x42
///     i2c.send(b'456', addr=0x42)  # keyword for address
///
/// Master also has other methods:
///
///     i2c.is_ready(0x42)           # check if slave 0x42 is ready
///     i2c.scan()                   # scan for slaves on the bus, returning
///                                  #   a list of valid addresses
///     i2c.mem_read(3, 0x42, 2)     # read 3 bytes from memory of slave 0x42,
///                                  #   starting at address 2 in the slave
///     i2c.mem_write('abc', 0x42, 2, timeout=1000)
///                                  #   starting at address 2 in the slave
///     i2c.mem_write('abc', 0x42, 2, timeout=1000)

#define PYB_I2C_MASTER (0)
#define PYB_I2C_SLAVE  (1)

#define PYB_I2C_SPEED_STANDARD (100000L)
#define PYB_I2C_SPEED_FULL     (400000L)
#define PYB_I2C_SPEED_FAST     (1000000L)

STATIC pyb_i2c_obj_t pyb_i2c_obj[] = {
    #if defined(MICROPY_HW_I2C0_SCL)
    {{&pyb_i2c_type}, I2C0},
    #else
    {{&pyb_i2c_type}, NULL},
    #endif
    #if defined(MICROPY_HW_I2C1_SCL)
    {{&pyb_i2c_type}, I2C1},
    #else
    {{&pyb_i2c_type}, NULL},
    #endif
    #if defined(MICROPY_HW_I2C2_SCL)
    {{&pyb_i2c_type}, I2C2},
    #else
    {{&pyb_i2c_type}, NULL},
    #endif
};


#define MICROPY_HW_I2C_BAUDRATE_DEFAULT (PYB_I2C_SPEED_STANDARD)
#define MICROPY_HW_I2C_BAUDRATE_MAX (PYB_I2C_SPEED_FAST)

STATIC void pyb_i2c_init(pyb_i2c_obj_t *self){
	int i2c_unit;
	const pin_obj_t *scl_pin;
	const pin_obj_t *sda_pin;


    if (0) {
    #if defined(MICROPY_HW_I2C0_SCL)
    } else if (self->i2c == I2C0) {
        i2c_unit = 0;
        scl_pin = MICROPY_HW_I2C0_SCL;
        sda_pin = MICROPY_HW_I2C0_SDA;
    #endif
    #if defined(MICROPY_HW_I2C1_SCL)
    } else if (self->i2c == I2C1) {
        i2c_unit = 1;
        scl_pin = MICROPY_HW_I2C1_SCL;
        sda_pin = MICROPY_HW_I2C1_SDA;
    #endif
    #if defined(MICROPY_HW_I2C2_SCL)
    } else if (self->i2c == I2C2) {
        i2c_unit = 2;
        scl_pin = MICROPY_HW_I2C2_SCL;
        sda_pin = MICROPY_HW_I2C2_SDA;
    #endif
    } else {
        // I2C does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

   // init the GPIO lines
//    uint32_t mode = MP_HAL_PIN_MODE_ALT_OPEN_DRAIN;
    uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;
    mp_hal_pin_config_alt(scl_pin, mode, AF_FN_I2C, i2c_unit);
    mp_hal_pin_config_alt(sda_pin, mode, AF_FN_I2C, i2c_unit);

	/* I2C clock pin enable schmitt trigger */
	//PA->SMTEN |= GPIO_SMTEN_SMTEN5_Msk;

    // init the I2C device
    if (I2C_Init(self->i2c, &self->init) != 0) {
        // init error
        // TODO should raise an exception, but this function is not necessarily going to be
        // called via Python, so may not be properly wrapped in an NLR handler
        printf("OSError: HAL_I2C_Init failed\n");
        return;
    }
}

STATIC void pyb_i2c_deinit(pyb_i2c_obj_t *self) {
	const pin_obj_t *scl_pin;
	const pin_obj_t *sda_pin;

	I2C_Final(self->i2c);
	
    if (0) {
    #if defined(MICROPY_HW_I2C0_SCL)
    } else if (self->i2c == I2C0) {
        scl_pin = MICROPY_HW_I2C0_SCL;
        sda_pin = MICROPY_HW_I2C0_SDA;
    #endif
    #if defined(MICROPY_HW_I2C1_SCL)
    } else if (self->i2c == I2C1) {
        scl_pin = MICROPY_HW_I2C1_SCL;
        sda_pin = MICROPY_HW_I2C1_SDA;
    #endif
    #if defined(MICROPY_HW_I2C2_SCL)
    } else if (self->i2c == I2C2) {
        scl_pin = MICROPY_HW_I2C2_SCL;
        sda_pin = MICROPY_HW_I2C2_SDA;
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
        { MP_QSTR_mode,     MP_ARG_INT, {.u_int = PYB_I2C_MASTER} },
        { MP_QSTR_addr,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0x12} },
        { MP_QSTR_baudrate, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MICROPY_HW_I2C_BAUDRATE_DEFAULT} },
        { MP_QSTR_gencall,  MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
//        { MP_QSTR_dma,      MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

   // set the I2C configuration values
    I2C_InitTypeDef *init = &self->init;

    if (args[0].u_int == PYB_I2C_MASTER) {
        // use a special address to indicate we are a master
		init->Mode = I2C_MODE_MASTER;
        init->OwnAddress = PYB_I2C_MASTER_ADDRESS;
    } else {
		init->Mode = I2C_MODE_SLAVE;
        init->OwnAddress = (args[1].u_int << 1) & 0xfe;
    }

	init->BaudRate = args[2].u_int;
	init->GeneralCallMode = args[3].u_bool ? I2C_GCMODE_ENABLE : I2C_GCMODE_DISABLE;

	// init the I2C bus
	pyb_i2c_deinit(self);
	pyb_i2c_init(self);

    return mp_const_none;
}


/******************************************************************************/
/* MicroPython bindings                                                       */

static inline bool in_master_mode(pyb_i2c_obj_t *self) { return self->init.OwnAddress == PYB_I2C_MASTER_ADDRESS; }

STATIC void pyb_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_i2c_obj_t *self = self_in;

    int i2c_num = -1;
    if (0) { }
    #if defined(MICROPY_HW_I2C0_SCL)
    else if (self->i2c == I2C0) { i2c_num = 0; }
    #endif
    #if defined(MICROPY_HW_I2C1_SCL)
    else if (self->i2c == I2C1) { i2c_num = 1; }
    #endif
    #if defined(MICROPY_HW_I2C2_SCL)
    else if (self->i2c == I2C2) { i2c_num = 2; }
    #endif

    if (i2c_num < 0) {
        mp_printf(print, "I2C(%u)", i2c_num);
    } else {
        if (in_master_mode(self)) {
            mp_printf(print, "I2C(%u, I2C.MASTER, baudrate=%u)", i2c_num, I2C_GetBusClockFreq(self->i2c));
        } else {
            mp_printf(print, "I2C(%u, I2C.SLAVE, addr=0x%02x)", i2c_num, (self->i2c->ADDR0 >> 1) & 0x7f);
        }
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
	if (i2c_id < 0 || i2c_id >= MP_ARRAY_SIZE(pyb_i2c_obj)
		|| pyb_i2c_obj[i2c_id].i2c == NULL) {
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

/// \method is_ready(addr)
/// Check if an I2C device responds to the given address.  Only valid when in master mode.
STATIC mp_obj_t pyb_i2c_is_ready(mp_obj_t self_in, mp_obj_t i2c_addr_o) {
    pyb_i2c_obj_t *self = self_in;

    if (!in_master_mode(self)) {
        mp_raise_TypeError("I2C must be a master");
    }

    mp_uint_t i2c_addr = mp_obj_get_int(i2c_addr_o);

    for (int i = 0; i < 10; i++) {
        int32_t status = I2C_DeviceReady(self->i2c, i2c_addr);
        if (status == I2C_DEVICE_OK) {
            return mp_const_true;
        }
    }

    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_i2c_is_ready_obj, pyb_i2c_is_ready);

/// \method scan()
/// Scan all I2C addresses from 0x08 to 0x77 and return a list of those that respond.
/// Only valid when in master mode.
STATIC mp_obj_t pyb_i2c_scan(mp_obj_t self_in) {
    pyb_i2c_obj_t *self = self_in;

    if (!in_master_mode(self)) {
        mp_raise_TypeError("I2C must be a master");
    }

    mp_obj_t list = mp_obj_new_list(0, NULL);

    for (uint addr = 0x08; addr <= 0x77; addr++) {
        int32_t status = I2C_DeviceReady(self->i2c, addr);
        if (status == I2C_DEVICE_OK) {
            mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(addr));
        }
    }

    return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_scan_obj, pyb_i2c_scan);

/// \method send(send, addr=0x00, timeout=5000)
/// Send data on the bus:
///
///   - `send` is the data to send (an integer to send, or a buffer object)
///   - `addr` is the address to send to (only required in master mode)
///   - `timeout` is the timeout in milliseconds to wait for the send
///
/// Return value: `None`.

STATIC mp_obj_t pyb_i2c_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_send,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_addr,    MP_ARG_INT, {.u_int = PYB_I2C_MASTER_ADDRESS} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[0].u_obj, &bufinfo, data);

	int32_t send_len = 0;
	if (in_master_mode(self)) {
        if (args[1].u_int == PYB_I2C_MASTER_ADDRESS) {
            mp_raise_TypeError("addr argument required");
        }
        mp_uint_t i2c_addr = args[1].u_int;

		send_len = I2C_WriteMultiBytes(self->i2c, i2c_addr, bufinfo.buf, bufinfo.len);
    } else {
		mp_raise_TypeError("I2C must be a master");
    }

	if(send_len != bufinfo.len){
		mp_hal_raise(HAL_ERROR);
	}

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_send_obj, 1, pyb_i2c_send);

/// \method recv(recv, addr=0x00, timeout=5000)
///
/// Receive data on the bus:
///
///   - `recv` can be an integer, which is the number of bytes to receive,
///     or a mutable buffer, which will be filled with received bytes
///   - `addr` is the address to receive from (only required in master mode)
///   - `timeout` is the timeout in milliseconds to wait for the receive
///
/// Return value: if `recv` is an integer then a new buffer of the bytes received,
/// otherwise the same buffer that was passed in to `recv`.

STATIC mp_obj_t pyb_i2c_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_recv,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_addr,    MP_ARG_INT, {.u_int = PYB_I2C_MASTER_ADDRESS} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    mp_obj_t o_ret = pyb_buf_get_for_recv(args[0].u_obj, &vstr);

	int32_t i32RecvLen = 0;
	if (in_master_mode(self)) {
        if (args[1].u_int == PYB_I2C_MASTER_ADDRESS) {
            mp_raise_TypeError("addr argument required");
        }
        mp_uint_t i2c_addr = args[1].u_int;
	
		i32RecvLen = I2C_ReadMultiBytes(self->i2c, i2c_addr, (uint8_t*)vstr.buf, vstr.len);
    } else {
		mp_raise_TypeError("I2C must be a master");
    }

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

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_recv_obj, 1, pyb_i2c_recv);

/// \method mem_read(data, addr, memaddr, timeout=5000, addr_size=8)
///
/// Read from the memory of an I2C device:
///
///   - `data` can be an integer or a buffer to read into
///   - `addr` is the I2C device address
///   - `memaddr` is the memory location within the I2C device
///   - `timeout` is the timeout in milliseconds to wait for the read
///   - `addr_size` selects width of memaddr: 8 or 16 bits
///
/// Returns the read data.
/// This is only valid in master mode.
STATIC const mp_arg_t pyb_i2c_mem_read_allowed_args[] = {
    { MP_QSTR_data,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_memaddr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    { MP_QSTR_addr_size, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
};


STATIC mp_obj_t pyb_i2c_mem_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args), pyb_i2c_mem_read_allowed_args, args);

    if (!in_master_mode(self)) {
        mp_raise_TypeError("I2C must be a master");
    }

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
    if (args[4].u_int != 8) {
        mem_addr_size = 16;
    }

	int32_t i32RecvLen = 0;

	if(mem_addr_size == 8)
		i32RecvLen = I2C_ReadMultiBytesOneReg(self->i2c, i2c_addr, mem_addr, (uint8_t*)vstr.buf, vstr.len);
	else
		i32RecvLen = I2C_ReadMultiBytesTwoRegs(self->i2c, i2c_addr, mem_addr, (uint8_t*)vstr.buf, vstr.len);
	
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

/// \method mem_write(data, addr, memaddr, timeout=5000, addr_size=8)
///
/// Write to the memory of an I2C device:
///
///   - `data` can be an integer or a buffer to write from
///   - `addr` is the I2C device address
///   - `memaddr` is the memory location within the I2C device
///   - `timeout` is the timeout in milliseconds to wait for the write
///   - `addr_size` selects width of memaddr: 8 or 16 bits
///
/// Returns `None`.
/// This is only valid in master mode.

STATIC mp_obj_t pyb_i2c_mem_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args (same as mem_read)
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_mem_read_allowed_args), pyb_i2c_mem_read_allowed_args, args);

    if (!in_master_mode(self)) {
        mp_raise_TypeError("I2C must be a master");
    }

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
    if (args[4].u_int != 8) {
        mem_addr_size = 16;
    }

	int32_t i32WriteLen = 0;

	if(mem_addr_size == 8)
		i32WriteLen = I2C_WriteMultiBytesOneReg(self->i2c, i2c_addr, mem_addr, bufinfo.buf, bufinfo.len);
	else
		i32WriteLen = I2C_WriteMultiBytesTwoRegs(self->i2c, i2c_addr, mem_addr, bufinfo.buf, bufinfo.len);

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
    { MP_ROM_QSTR(MP_QSTR_is_ready), MP_ROM_PTR(&pyb_i2c_is_ready_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&pyb_i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&pyb_i2c_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&pyb_i2c_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_read), MP_ROM_PTR(&pyb_i2c_mem_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_write), MP_ROM_PTR(&pyb_i2c_mem_write_obj) },

    // class constants
    /// \constant MASTER - for initialising the bus to master mode
    /// \constant SLAVE - for initialising the bus to slave mode
    { MP_ROM_QSTR(MP_QSTR_MASTER), MP_ROM_INT(PYB_I2C_MASTER) },
    { MP_ROM_QSTR(MP_QSTR_SLAVE), MP_ROM_INT(PYB_I2C_SLAVE) },
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
