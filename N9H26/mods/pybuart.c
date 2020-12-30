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
#include <stdarg.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#if MICROPY_HW_ENABLE_HW_UART

#include "pybuart.h"
#include "wblib.h"

typedef enum
{
	eUART_HWCONTROL_NONE,
	eUART_HWCONTROL_RTS,
	eUART_HWCONTROL_CTS
}E_UART_HWCONTROL;

typedef struct  {
    mp_obj_base_t base;
	int32_t uart_id;

    bool is_enabled;
	uint16_t char_mask;                 // 0x1f for 5 bit, 0x3f for 6 bit, 0x7f for 7 bit, 0xff for 8 bit
    uint16_t timeout;                   // timeout waiting for first char
    uint16_t timeout_char;              // timeout waiting between chars
    uint16_t read_buf_len;              // len in chars; buf can hold len-1 chars
    volatile uint16_t read_buf_head;    // indexes first empty slot
    uint16_t read_buf_tail;             // indexes first full slot (not full if equals head)
    byte *read_buf;                     // byte or uint16_t, depending on char size

	uint32_t u32BaudRate;
	uint32_t u32DataWidth;
	uint32_t u32Parity;
	uint32_t u32StopBits;
    uint32_t u32FlowControl;

//    IRQn_Type irqn;
//    bool attached_to_repl;              // whether the UART is attached to REPL

}pyb_uart_obj_t;

#define N9H26_MAX_UART_INST 1

STATIC pyb_uart_obj_t pyb_uart_obj[N9H26_MAX_UART_INST] = {

#if defined(MICROPY_HW_UART0_RXD)
    {{&pyb_uart_type}, 0, false},
#else
    {{&pyb_uart_type}, -1, false},
#endif

};


STATIC void switch_pinfun(const pyb_uart_obj_t *self, bool bUART, uint32_t u32FlowCtrl){

	const pin_obj_t *rxd_pin;
	const pin_obj_t *txd_pin;
	const pin_obj_t *cts_pin;
	const pin_obj_t *rts_pin;

    if (0) {
    #if defined(MICROPY_HW_UART0_RXD)
    } else if (self->uart_id == 0) {
		rxd_pin = MICROPY_HW_UART0_RXD;
		txd_pin = MICROPY_HW_UART0_TXD;
		cts_pin = MICROPY_HW_UART0_CTS;
		rts_pin = MICROPY_HW_UART0_RTS;
	#endif
    } else {
        // UART does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

	if(bUART == true){
		uint32_t mode = MP_HAL_PIN_MODE_ALT;
		printf("switch pin fun \n");
		mp_hal_pin_config_alt(rxd_pin, mode, AF_FN_UART, self->uart_id);
		mp_hal_pin_config_alt(txd_pin, mode, AF_FN_UART, self->uart_id);
		if(u32FlowCtrl & eUART_HWCONTROL_CTS)
			mp_hal_pin_config_alt(cts_pin, mode, AF_FN_UART, self->uart_id);
		if(u32FlowCtrl & eUART_HWCONTROL_RTS)
			mp_hal_pin_config_alt(rts_pin, mode, AF_FN_UART, self->uart_id);
	}
	else{
		mp_hal_pin_config(rxd_pin, GPIO_MODE_INPUT, 0);
		mp_hal_pin_config(txd_pin, GPIO_MODE_INPUT, 0);
		if(u32FlowCtrl & eUART_HWCONTROL_CTS)
			mp_hal_pin_config(cts_pin, GPIO_MODE_INPUT, 0);
		if(u32FlowCtrl & eUART_HWCONTROL_RTS)
			mp_hal_pin_config(rts_pin, GPIO_MODE_INPUT, 0);
	}
}

STATIC bool UART_IS_RX_READY(
	uint32_t u32Port
)
{
	uint32_t u32AddrOff = 0x100;
	
	return (! (inp32(REG_UART_FSR + (u32Port * u32AddrOff)) & Rx_Empty));
}

STATIC bool UART_IS_TX_EMPTY(
	uint32_t u32Port
)
{
	uint32_t u32AddrOff = 0x100;
	
	return ((inp32(REG_UART_FSR + (u32Port * u32AddrOff)) & Tx_Empty));
}

// Waits at most timeout milliseconds for TX register to become empty.
// Returns true if can write, false if can't.
STATIC bool uart_tx_wait(uint32_t u32Port, uint32_t timeout) {
    uint32_t start = mp_hal_ticks_ms();
		
    for (;;) {
        if (UART_IS_TX_EMPTY(u32Port)) {
            return true; // tx register is empty
        }
        if (mp_hal_ticks_ms() - start >= timeout) {
            return false; // timeout
        }
        MICROPY_EVENT_POLL_HOOK
    }
}

// Waits at most timeout milliseconds for at least 1 char to become ready for
// reading (from buf or for direct reading).
// Returns true if something available, false if not.
STATIC bool uart_rx_wait(pyb_uart_obj_t *self, uint32_t timeout) {
    uint32_t start = mp_hal_ticks_ms();

    for (;;) {
        if (self->read_buf_tail != self->read_buf_head || UART_IS_RX_READY(self->uart_id)) {
            return true; // have at least 1 char ready for reading
        }
        if (mp_hal_ticks_ms() - start >= timeout) {
            return false; // timeout
        }
        MICROPY_EVENT_POLL_HOOK
    }
}


// src - a pointer to the data to send (16-bit aligned for 9-bit chars)
// num_chars - number of characters to send (9-bit chars count for 2 bytes from src)
// *errcode - returns 0 for success, MP_Exxx on error
// returns the number of characters sent (valid even if there was an error)
STATIC size_t uart_tx_data(pyb_uart_obj_t *self, const void *src_in, size_t num_chars, int *errcode) {

    if (num_chars == 0) {
        *errcode = 0;
        return 0;
    }

    uint32_t timeout;
    if (self->u32FlowControl & eUART_HWCONTROL_CTS) {
        // CTS can hold off transmission for an arbitrarily long time. Apply
        // the overall timeout rather than the character timeout.
        timeout = self->timeout;
    } else {
        // The timeout specified here is for waiting for the TX data register to
        // become empty (ie between chars), as well as for the final char to be
        // completely transferred.  The default value for timeout_char is long
        // enough for 1 char, but we need to double it to wait for the last char
        // to be transferred to the data register, and then to be transmitted.
        timeout = 2 * self->timeout_char;
    }

	const uint8_t *src = (const uint8_t*)src_in;
    size_t num_tx = 0;

    while (num_tx < num_chars) {
        if (!uart_tx_wait(self->uart_id, timeout)) {
            *errcode = MP_ETIMEDOUT;
            return num_tx;
        }
        uint32_t data;
        data = *src++;

		if(self->uart_id == 0)
		{
			sysHuartTransfer((INT8*)&data, 1);
		}
		else if(self->uart_id == 1)
		{
			sysUartTransfer((INT8*)&data, 1);
		}
        ++num_tx;
    }
    
    *errcode = 0;
    return num_tx;

}

STATIC int uart_rx_char(pyb_uart_obj_t *self) {
	
    if (self->read_buf_tail != self->read_buf_head) {
        // buffering via IRQ
        int data;
        data = self->read_buf[self->read_buf_tail];

        self->read_buf_tail = (self->read_buf_tail + 1) % self->read_buf_len;
//        if (__HAL_UART_GET_FLAG(&self->uart, UART_FLAG_RXNE) != RESET) {
            // UART was stalled by flow ctrl: re-enable IRQ now we have room in buffer
//            __HAL_UART_ENABLE_IT(&self->uart, UART_IT_RXNE);
//        }
        return data;
    } else {
        // no buffering
		if(self->uart_id == 0)
			return (sysHuartReceive());
		else if(self->uart_id == 1)
			return (sysGetChar());		
    }

	return 0;
}

/// \method init(baudrate, bits=8, parity=None, stop=1, *, timeout=1000, timeout_char=0, flow=0, read_buf_len=64)
///
/// Initialise the UART bus with the given parameters:
///
///   - `baudrate` is the clock rate.
///   - `bits` is the number of bits per byte, 7, 8 or 9.
///   - `parity` is the parity, `None`, 0 (even) or 1 (odd).
///   - `stop` is the number of stop bits, 1 or 2.
///   - `timeout` is the timeout in milliseconds to wait for the first character.
///   - `timeout_char` is the timeout in milliseconds to wait between characters.
///   - `flow` is RTS | CTS where RTS == 256, CTS == 512
///   - `read_buf_len` is the character length of the read buffer (0 to disable).
STATIC mp_obj_t pyb_uart_init_helper(pyb_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 9600} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_flow, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = eUART_HWCONTROL_NONE} },
        { MP_QSTR_read_buf_len, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 64} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 2000} },
        { MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };

    // parse args
    struct {
        mp_arg_val_t baudrate, bits, parity, stop, flow, read_buf_len, timeout, timeout_char;
    } args;

    mp_arg_parse_all(n_args, pos_args, kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, (mp_arg_val_t*)&args);

	self->u32BaudRate = args.baudrate.u_int;

	if (args.parity.u_obj == mp_const_none) {
        self->u32Parity = WB_PARITY_NONE;
    } else {
        mp_int_t parity = mp_obj_get_int(args.parity.u_obj);
        self->u32Parity = (parity & 1) ? WB_PARITY_ODD : WB_PARITY_EVEN;
    }

	mp_int_t bits = args.bits.u_int;
	if(bits == 5){
		self->u32DataWidth = WB_DATA_BITS_5;
		self->char_mask = 0x1f;
	}
	else if (bits == 6){
		self->u32DataWidth = WB_DATA_BITS_6;
		self->char_mask = 0x3f;
	}
	else if (bits == 7){
		self->u32DataWidth = WB_DATA_BITS_7;
		self->char_mask = 0x7f;
	}
	else if (bits == 8){
		self->u32DataWidth = WB_DATA_BITS_8;
		self->char_mask = 0xff;
	} else {
        mp_raise_ValueError("unsupported combination of bits and parity");
    }

    // stop bits
    switch (args.stop.u_int) {
        case 1: self->u32StopBits = WB_STOP_BITS_1; break;
        default: self->u32StopBits = WB_STOP_BITS_2; break;
    }

   // flow control
    self->u32FlowControl = args.flow.u_int;

	//timeout
	self->timeout = args.timeout.u_int;

    // set timeout_char
    // make sure it is at least as long as a whole character (13 bits to be safe)
    // minimum value is 2ms because sys-tick has a resolution of only 1ms
    self->timeout_char = args.timeout_char.u_int;
    uint32_t min_timeout_char = 13000 / self->u32BaudRate + 2;
    if (self->timeout_char < min_timeout_char) {
        self->timeout_char = min_timeout_char;
    }

	m_del(byte, self->read_buf, self->read_buf_len);

    self->read_buf_head = 0;
    self->read_buf_tail = 0;
    if (args.read_buf_len.u_int <= 0) {
        // no read buffer
        self->read_buf_len = 0;
        self->read_buf = NULL;
    } else {
        // read buffer using interrupts
        self->read_buf_len = args.read_buf_len.u_int + 1; // +1 to adjust for usable length of buffer
        self->read_buf = m_new(byte, self->read_buf_len);
    }

	switch_pinfun(self, true, self->u32FlowControl);

	WB_UART_T sWB_UART;
	int32_t i32Ret;
	
	sWB_UART.uart_no = self->uart_id;
	sWB_UART.uiFreq = EXTERNAL_CRYSTAL_CLOCK;
	sWB_UART.uiBaudrate = self->u32BaudRate;
	sWB_UART.uiDataBits = self->u32DataWidth;
	sWB_UART.uiStopBits = self->u32StopBits;
	sWB_UART.uiParity = self->u32Parity;
	sWB_UART.uiRxTriggerLevel = LEVEL_1_BYTE;
	
	if(self->uart_id == 0)
		i32Ret = sysInitializeHUART(&sWB_UART);
	else
		i32Ret = sysInitializeUART(&sWB_UART);

	if(i32Ret != Successful)
	{
		switch_pinfun(self, false, self->u32FlowControl);
        mp_raise_ValueError("Unable open UART device");
	}

	if((self->u32FlowControl != eUART_HWCONTROL_NONE) && (self->uart_id == 0))
	{
		if(self->u32FlowControl & eUART_HWCONTROL_CTS)
		{
			outp32(REG_UART_IER+0, inp32(REG_UART_IER+0) | Auto_CTS_EN);
		}

		if(self->u32FlowControl & eUART_HWCONTROL_RTS)
		{
			outp32(REG_UART_IER+0, inp32(REG_UART_IER+0) | Auto_RTS_EN);
		}
	}

	self->is_enabled = true;

	return mp_const_none;
}

/******************************************************************************/
/* MicroPython bindings                                                       */

STATIC void pyb_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_uart_obj_t *self = self_in;
    if (!self->is_enabled) {
        mp_printf(print, "UART(%u)", self->uart_id);
    } else {
       mp_int_t bits;
        switch (self->u32DataWidth) {
            case WB_DATA_BITS_5: bits = 5; break;
            case WB_DATA_BITS_6: bits = 6; break;
            case WB_DATA_BITS_7: bits = 7; break;
            case WB_DATA_BITS_8: bits = 8; break;
            default: bits = 8; break;
        }
        mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=",
            self->uart_id, self->u32BaudRate, bits);

        if (self->u32Parity == WB_PARITY_NONE) {
            mp_print_str(print, "None");
        } else {
            mp_printf(print, "%u", self->u32Parity == WB_PARITY_EVEN ? 0 : 1);
        }
        if (self->u32FlowControl) {
            mp_printf(print, ", flow=");
            if (self->u32FlowControl & eUART_HWCONTROL_RTS) {
                mp_printf(print, "RTS%s", self->u32FlowControl & eUART_HWCONTROL_CTS ? "|" : "");
            }
            if (self->u32FlowControl & eUART_HWCONTROL_CTS) {
                mp_printf(print, "CTS");
            }
        }
        mp_printf(print, ", stop=%u, timeout_char=%u, read_buf_len=%u)",
            self->u32StopBits == WB_STOP_BITS_1 ? 1 : 2,
            self->timeout_char,
            self->read_buf_len == 0 ? 0 : self->read_buf_len - 1); // -1 to adjust for usable length of buffer
	}
}

STATIC mp_obj_t pyb_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the timer id
    mp_int_t uart_id = mp_obj_get_int(args[0]);

    // check if the timer exists
    if (uart_id < 0 || uart_id >= N9H26_MAX_UART_INST) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) doesn't exist", uart_id));
    }

    pyb_uart_obj_t *self = &pyb_uart_obj[uart_id];

	if(self->uart_id < 0){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) is not support ", uart_id));
	}

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        pyb_uart_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

	return (mp_obj_t)self;
}

STATIC mp_obj_t pyb_uart_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_uart_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_uart_init_obj, 1, pyb_uart_init);

/// \method deinit()
/// Turn off the UART bus.
STATIC mp_obj_t pyb_uart_deinit(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;

	if(self->uart_id == 0)
		outp32(REG_APBCLK, inp32(REG_APBCLK) & ~UART0_CKE);
	else if(self->uart_id == 1)
		outp32(REG_APBCLK, inp32(REG_APBCLK) & ~UART1_CKE);
	
    self->is_enabled = false;
	switch_pinfun(self, false, self->u32FlowControl);
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_deinit_obj, pyb_uart_deinit);

/// \method any()
/// Return `True` if any characters waiting, else `False`.
STATIC mp_obj_t pyb_uart_any(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;
	
	if(self->uart_id < 0){
		return MP_OBJ_NEW_SMALL_INT(0);
	}

    return MP_OBJ_NEW_SMALL_INT(UART_IS_RX_READY(self->uart_id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_any_obj, pyb_uart_any);

/// \method writechar(char)
/// Write a single character on the bus.  `char` is an integer to write.
/// Return value: `None`.
STATIC mp_obj_t pyb_uart_writechar(mp_obj_t self_in, mp_obj_t char_in) {
    pyb_uart_obj_t *self = self_in;

    // get the character to write
    uint16_t data = mp_obj_get_int(char_in);

    // write the character
    int errcode = 0;

    if (uart_tx_wait(self->uart_id, self->timeout)) {
        uart_tx_data(self, (uint8_t *)&data, 1, &errcode);
    } else {
        errcode = MP_ETIMEDOUT;
    }

    if (errcode != 0) {
        mp_raise_OSError(errcode);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_uart_writechar_obj, pyb_uart_writechar);

/// \method readchar()
/// Receive a single character on the bus.
/// Return value: The character read, as an integer.  Returns -1 on timeout.
STATIC mp_obj_t pyb_uart_readchar(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;
    if (uart_rx_wait(self, self->timeout)) {
		if(self->uart_id == 0)
			return MP_OBJ_NEW_SMALL_INT(sysHuartReceive());
		else if(self->uart_id == 1)
			return MP_OBJ_NEW_SMALL_INT(sysGetChar());		
    }

	// return -1 on timeout
	return MP_OBJ_NEW_SMALL_INT(-1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_readchar_obj, pyb_uart_readchar);


STATIC const mp_rom_map_elem_t pyb_uart_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_uart_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&pyb_uart_any_obj) },

    { MP_ROM_QSTR(MP_QSTR_writechar), MP_ROM_PTR(&pyb_uart_writechar_obj) },
    { MP_ROM_QSTR(MP_QSTR_readchar), MP_ROM_PTR(&pyb_uart_readchar_obj) },

    /// \method read([nbytes])
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    /// \method readline()
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    /// \method readinto(buf[, nbytes])
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    /// \method write(buf)
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },


    // class constants
    { MP_ROM_QSTR(MP_QSTR_RTS), MP_ROM_INT(eUART_HWCONTROL_RTS) },
    { MP_ROM_QSTR(MP_QSTR_CTS), MP_ROM_INT(eUART_HWCONTROL_CTS) },

};

STATIC MP_DEFINE_CONST_DICT(pyb_uart_locals_dict, pyb_uart_locals_dict_table);

STATIC mp_uint_t pyb_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) 
{
    pyb_uart_obj_t *self = self_in;
	byte *buf = buf_in;

    // wait for first char to become available
    if (!uart_rx_wait(self, self->timeout)) {
        // return EAGAIN error to indicate non-blocking (then read() method returns None)
        *errcode = 0; //MP_EAGAIN;
        return 0;
    }

    // read the data
    byte *orig_buf = buf;
    for (;;) {
        int data = uart_rx_char(self);

        *buf++ = data;
        if (--size == 0 || !uart_rx_wait(self, self->timeout_char)) {
            // return number of bytes read
			*errcode = 0;
            return buf - orig_buf;
        }
    }
}

STATIC mp_uint_t pyb_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    const byte *buf = buf_in;

    // wait to be able to write the first character. EAGAIN causes write to return None
    if (!uart_tx_wait(self->uart_id, self->timeout)) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

	*errcode = 0;
    // write the data
    size_t num_tx = uart_tx_data(self, buf, size, errcode);

    if (*errcode == 0 || *errcode == MP_ETIMEDOUT) {
        // return number of bytes written, even if there was a timeout
        return num_tx;
    } else {
        return MP_STREAM_ERROR;
    }
}

STATIC mp_uint_t pyb_uart_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    mp_uint_t ret;
	
	if(self->uart_id < 0){
        *errcode = MP_EINVAL;
        return MP_STREAM_ERROR;
	}
        
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && UART_IS_RX_READY(self->uart_id)) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && UART_IS_TX_EMPTY(self->uart_id)) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}


STATIC const mp_stream_p_t uart_stream_p = {
    .read = pyb_uart_read,
    .write = pyb_uart_write,
    .ioctl = pyb_uart_ioctl,
    .is_text = false,
};


const mp_obj_type_t pyb_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = pyb_uart_print,
    .make_new = pyb_uart_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &uart_stream_p,
    .locals_dict = (mp_obj_dict_t*)&pyb_uart_locals_dict,
};

#endif
