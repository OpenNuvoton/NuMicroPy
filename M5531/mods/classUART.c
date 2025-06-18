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

#include "classUART.h"
#include "hal/M5531_UART.h"
/// \moduleref pyb
/// \class UART - duplex serial communication bus
///
/// UART implements the standard UART/USART duplex serial communications protocol.  At
/// the physical level it consists of 2 lines: RX and TX.  The unit of communication
/// is a character (not to be confused with a string character) which can be 8 or 9
/// bits wide.
///
/// UART objects can be created and initialised using:
///
///     from pyb import UART
///
///     uart = UART(1, 9600)                         # init with given baudrate
///     uart.init(9600, bits=8, parity=None, stop=1) # init with given parameters
///
/// Bits can be 8 or 9.  Parity can be None, 0 (even) or 1 (odd).  Stop can be 1 or 2.
///
/// A UART object acts like a stream object and reading and writing is done
/// using the standard stream methods:
///
///     uart.read(10)       # read 10 characters, returns a bytes object
///     uart.read()         # read all available characters
///     uart.readline()     # read a line
///     uart.readinto(buf)  # read and store into the given buffer
///     uart.write('abc')   # write the 3 characters
///
/// Individual characters can be read/written using:
///
///     uart.readchar()     # read 1 character and returns it as an integer
///     uart.writechar(42)  # write 1 character
///
/// To check if there is anything to be read, use:
///
///     uart.any()               # returns True if any characters waiting

typedef struct  {
    mp_obj_base_t base;
    uint32_t uart_id;
//	UART_T *uart;
    uart_t *psUARTObj;
    IRQn_Type irqn;
    bool is_enabled;
    bool attached_to_repl;              // whether the UART is attached to REPL
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

} pyb_uart_obj_t;


#define M5531_MAX_UART_INST 11

#if defined(MICROPY_HW_UART0_RXD)
static uart_t s_sUART0Obj = {.u_uart.uart = UART0, .bLPUART = false};
#endif
#if defined(MICROPY_HW_UART1_RXD)
static uart_t s_sUART1Obj = {.u_uart.uart = UART1, .bLPUART = false};
#endif
#if defined(MICROPY_HW_UART2_RXD)
static uart_t s_sUART2Obj = {.u_uart.uart = UART2, .bLPUART = false};
#endif
#if defined(MICROPY_HW_UART3_RXD)
static uart_t s_sUART3Obj = {.u_uart.uart = UART3, .bLPUART = false};
#endif
#if defined(MICROPY_HW_UART4_RXD)
static uart_t s_sUART4Obj = {.u_uart.uart = UART4, .bLPUART = false};
#endif
#if defined(MICROPY_HW_UART5_RXD)
static uart_t s_sUART5Obj = {.u_uart.uart = UART5, .bLPUART = false};
#endif
#if defined(MICROPY_HW_UART6_RXD)
static uart_t s_sUART6Obj = {.u_uart.uart = UART6, .bLPUART = false};
#endif
#if defined(MICROPY_HW_UART7_RXD)
static uart_t s_sUART7Obj = {.u_uart.uart = UART7, .bLPUART = false};
#endif
#if defined(MICROPY_HW_UART8_RXD)
static uart_t s_sUART8Obj = {.u_uart.uart = UART8, .bLPUART = false};
#endif
#if defined(MICROPY_HW_UART9_RXD)
static uart_t s_sUART9Obj = {.u_uart.uart = UART9, .bLPUART = false};
#endif

#if defined(MICROPY_HW_UART10_RXD)
static uart_t s_sUART10Obj = {.u_uart.lpuart = LPUART0, .bLPUART = true};
#endif

static pyb_uart_obj_t pyb_uart_obj[M5531_MAX_UART_INST] = {
#if defined(MICROPY_HW_UART0_RXD)
    {{&machine_uart_type}, 0, &s_sUART0Obj, UART0_IRQn, false, true},
#else
    {{&machine_uart_type}, 0, NULL, 0, false, false},
#endif

#if defined(MICROPY_HW_UART1_RXD)
    {{&machine_uart_type}, 1, &s_sUART1Obj, UART1_IRQn, false, false},
#else
    {{&machine_uart_type}, 1, NULL, 0, false, false},
#endif

#if defined(MICROPY_HW_UART2_RXD)
    {{&machine_uart_type}, 2, &s_sUART2Obj, UART2_IRQn, false, false},
#else
    {{&machine_uart_type}, 2, NULL, 0, false, false},
#endif

#if defined(MICROPY_HW_UART3_RXD)
    {{&machine_uart_type}, 3, &s_sUART3Obj, UART3_IRQn, false, false},
#else
    {{&machine_uart_type}, 3, NULL, 0, false, false},
#endif

#if defined(MICROPY_HW_UART4_RXD)
    {{&machine_uart_type}, 4, &s_sUART4Obj, UART4_IRQn, false, false},
#else
    {{&machine_uart_type}, 4, NULL, 0, false, false},
#endif

#if defined(MICROPY_HW_UART5_RXD)
    {{&machine_uart_type}, 5, &s_sUART5Obj, UART5_IRQn, false, false},
#else
    {{&machine_uart_type}, 5, NULL, 0, false, false},
#endif

#if defined(MICROPY_HW_UART6_RXD)
    {{&machine_uart_type}, 6, &s_sUART6Obj, UART6_IRQn, false, false},
#else
    {{&machine_uart_type}, 6, NULL, 0, false, false},
#endif

#if defined(MICROPY_HW_UART7_RXD)
    {{&machine_uart_type}, 7, &s_sUART7Obj, UART7_IRQn, false, false},
#else
    {{&machine_uart_type}, 7, NULL, 0, false, false},
#endif

#if defined(MICROPY_HW_UART8_RXD)
    {{&machine_uart_type}, 8, &s_sUART8Obj, UART8_IRQn, false, false},
#else
    {{&machine_uart_type}, 8, NULL, 0, false, false},
#endif

#if defined(MICROPY_HW_UART9_RXD)
    {{&machine_uart_type}, 9, &s_sUART9Obj, UART9_IRQn, false, false},
#else
    {{&machine_uart_type}, 9, NULL, 0, false, false},
#endif

#if defined(MICROPY_HW_UART10_RXD)
    {{&machine_uart_type}, 10, &s_sUART10Obj, LPUART0_IRQn, false, false},
#else
    {{&machine_uart_type}, 10, NULL, 0, false, false},
#endif

};

static void enable_uart_clock(const pyb_uart_obj_t *self, bool bEnable)
{

    if(bEnable == false) {
        if(self->uart_id == 0) {
            CLK_DisableModuleClock(UART0_MODULE);
        } else if(self->uart_id == 1) {
            CLK_DisableModuleClock(UART1_MODULE);
        } else if(self->uart_id == 2) {
            CLK_DisableModuleClock(UART2_MODULE);
        } else if(self->uart_id == 3) {
            CLK_DisableModuleClock(UART3_MODULE);
        } else if(self->uart_id == 4) {
            CLK_DisableModuleClock(UART4_MODULE);
        } else if(self->uart_id == 5) {
            CLK_DisableModuleClock(UART5_MODULE);
        } else if(self->uart_id == 6) {
            CLK_DisableModuleClock(UART6_MODULE);
        } else if(self->uart_id == 7) {
            CLK_DisableModuleClock(UART7_MODULE);
        } else if(self->uart_id == 8) {
            CLK_DisableModuleClock(UART8_MODULE);
        } else if(self->uart_id == 9) {
            CLK_DisableModuleClock(UART9_MODULE);
        } else if(self->uart_id == 10) {
            CLK_DisableModuleClock(LPUART0_MODULE);
        }
        return;
    }


    if(self->uart_id == 0) {
        CLK_EnableModuleClock(UART0_MODULE);
        CLK_SetModuleClock(UART0_MODULE, CLK_UARTSEL0_UART0SEL_HIRC, CLK_UARTDIV0_UART1DIV(1));
    } else if(self->uart_id == 1) {
        CLK_EnableModuleClock(UART1_MODULE);
        CLK_SetModuleClock(UART1_MODULE, CLK_UARTSEL0_UART1SEL_HIRC, CLK_UARTDIV0_UART1DIV(1));
    } else if(self->uart_id == 2) {
        CLK_EnableModuleClock(UART2_MODULE);
        CLK_SetModuleClock(UART2_MODULE, CLK_UARTSEL0_UART2SEL_HIRC, CLK_UARTDIV0_UART1DIV(1));
    } else if(self->uart_id == 3) {
        CLK_EnableModuleClock(UART3_MODULE);
        CLK_SetModuleClock(UART3_MODULE, CLK_UARTSEL0_UART3SEL_HIRC, CLK_UARTDIV0_UART1DIV(1));
    } else if(self->uart_id == 4) {
        CLK_EnableModuleClock(UART4_MODULE);
        CLK_SetModuleClock(UART4_MODULE, CLK_UARTSEL0_UART4SEL_HIRC, CLK_UARTDIV0_UART1DIV(1));
    } else if(self->uart_id == 5) {
        CLK_EnableModuleClock(UART5_MODULE);
        CLK_SetModuleClock(UART5_MODULE, CLK_UARTSEL0_UART5SEL_HIRC, CLK_UARTDIV0_UART1DIV(1));
    } else if(self->uart_id == 6) {
        CLK_EnableModuleClock(UART6_MODULE);
        CLK_SetModuleClock(UART6_MODULE, CLK_UARTSEL0_UART6SEL_HIRC, CLK_UARTDIV0_UART1DIV(1));
    } else if(self->uart_id == 7) {
        CLK_EnableModuleClock(UART7_MODULE);
        CLK_SetModuleClock(UART7_MODULE, CLK_UARTSEL0_UART7SEL_HIRC, CLK_UARTDIV0_UART1DIV(1));
    } else if(self->uart_id == 8) {
        CLK_EnableModuleClock(UART8_MODULE);
        CLK_SetModuleClock(UART8_MODULE, CLK_UARTSEL1_UART8SEL_HIRC, CLK_UARTDIV0_UART1DIV(1));
    } else if(self->uart_id == 9) {
        CLK_EnableModuleClock(UART9_MODULE);
        CLK_SetModuleClock(UART9_MODULE, CLK_UARTSEL1_UART9SEL_HIRC, CLK_UARTDIV0_UART1DIV(1));
    } else if(self->uart_id == 10) {
        CLK_EnableModuleClock(LPUART0_MODULE);
        CLK_SetModuleClock(LPUART0_MODULE, CLK_LPUARTSEL_LPUART0SEL_HIRC, CLK_LPUARTDIV_LPUART0DIV(1));
    }
}

static void switch_pinfun(const pyb_uart_obj_t *self, bool bUART, uint32_t u32FlowCtrl)
{

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
#if defined(MICROPY_HW_UART1_RXD)
    } else if (self->uart_id == 1) {
        rxd_pin = MICROPY_HW_UART1_RXD;
        txd_pin = MICROPY_HW_UART1_TXD;
        cts_pin = MICROPY_HW_UART1_CTS;
        rts_pin = MICROPY_HW_UART1_RTS;
#endif
#if defined(MICROPY_HW_UART2_RXD)
    } else if (self->uart_id == 2) {
        rxd_pin = MICROPY_HW_UART2_RXD;
        txd_pin = MICROPY_HW_UART2_TXD;
        cts_pin = MICROPY_HW_UART2_CTS;
        rts_pin = MICROPY_HW_UART2_RTS;
#endif
#if defined(MICROPY_HW_UART3_RXD)
    } else if (self->uart_id == 3) {
        rxd_pin = MICROPY_HW_UART3_RXD;
        txd_pin = MICROPY_HW_UART3_TXD;
        cts_pin = MICROPY_HW_UART3_CTS;
        rts_pin = MICROPY_HW_UART3_RTS;
#endif
#if defined(MICROPY_HW_UART4_RXD)
    } else if (self->uart_id == 4) {
        rxd_pin = MICROPY_HW_UART4_RXD;
        txd_pin = MICROPY_HW_UART4_TXD;
        cts_pin = MICROPY_HW_UART4_CTS;
        rts_pin = MICROPY_HW_UART4_RTS;
#endif
#if defined(MICROPY_HW_UART5_RXD)
    } else if (self->uart_id == 5) {
        rxd_pin = MICROPY_HW_UART5_RXD;
        txd_pin = MICROPY_HW_UART5_TXD;
        cts_pin = MICROPY_HW_UART5_CTS;
        rts_pin = MICROPY_HW_UART5_RTS;
#endif
#if defined(MICROPY_HW_UART6_RXD)
    } else if (self->uart_id == 6) {
        rxd_pin = MICROPY_HW_UART6_RXD;
        txd_pin = MICROPY_HW_UART6_TXD;
        cts_pin = MICROPY_HW_UART6_CTS;
        rts_pin = MICROPY_HW_UART6_RTS;
#endif
#if defined(MICROPY_HW_UART7_RXD)
    } else if (self->uart_id == 7) {
        rxd_pin = MICROPY_HW_UART7_RXD;
        txd_pin = MICROPY_HW_UART7_TXD;
        cts_pin = MICROPY_HW_UART7_CTS;
        rts_pin = MICROPY_HW_UART7_RTS;
#endif
#if defined(MICROPY_HW_UART8_RXD)
    } else if (self->uart_id == 8) {
        rxd_pin = MICROPY_HW_UART8_RXD;
        txd_pin = MICROPY_HW_UART8_TXD;
        cts_pin = MICROPY_HW_UART8_CTS;
        rts_pin = MICROPY_HW_UART8_RTS;
#endif
#if defined(MICROPY_HW_UART9_RXD)
    } else if (self->uart_id == 9) {
        rxd_pin = MICROPY_HW_UART9_RXD;
        txd_pin = MICROPY_HW_UART9_TXD;
        cts_pin = MICROPY_HW_UART9_CTS;
        rts_pin = MICROPY_HW_UART9_RTS;
#endif
#if defined(MICROPY_HW_UART10_RXD)	//UART10 is for LPUART0
    } else if (self->uart_id == 10) {
        rxd_pin = MICROPY_HW_UART10_RXD;
        txd_pin = MICROPY_HW_UART10_TXD;
        cts_pin = MICROPY_HW_UART10_CTS;
        rts_pin = MICROPY_HW_UART10_RTS;
#endif
    } else {
        // UART does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

    if(bUART == true) {
        uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;
        printf("switch pin fun \n");

        if(self->uart_id == 10) { // for LPUART0
            mp_hal_pin_config_alt(rxd_pin, mode, AF_FN_LPUART, 0);
            mp_hal_pin_config_alt(txd_pin, mode, AF_FN_LPUART, 0);

            if(u32FlowCtrl & eUART_HWCONTROL_CTS)
                mp_hal_pin_config_alt(cts_pin, mode, AF_FN_LPUART, 0);
            if(u32FlowCtrl & eUART_HWCONTROL_RTS)
                mp_hal_pin_config_alt(rts_pin, mode, AF_FN_LPUART, 0);
        } else {
            mp_hal_pin_config_alt(rxd_pin, mode, AF_FN_UART, self->uart_id);
            mp_hal_pin_config_alt(txd_pin, mode, AF_FN_UART, self->uart_id);

            if(u32FlowCtrl & eUART_HWCONTROL_CTS)
                mp_hal_pin_config_alt(cts_pin, mode, AF_FN_UART, self->uart_id);
            if(u32FlowCtrl & eUART_HWCONTROL_RTS)
                mp_hal_pin_config_alt(rts_pin, mode, AF_FN_UART, self->uart_id);
        }

    } else {
        mp_hal_pin_config(rxd_pin, GPIO_MODE_INPUT, 0);
        mp_hal_pin_config(txd_pin, GPIO_MODE_INPUT, 0);
        if(u32FlowCtrl & eUART_HWCONTROL_CTS)
            mp_hal_pin_config(cts_pin, GPIO_MODE_INPUT, 0);
        if(u32FlowCtrl & eUART_HWCONTROL_RTS)
            mp_hal_pin_config(rts_pin, GPIO_MODE_INPUT, 0);
    }
}

// Waits at most timeout milliseconds for TX register to become empty.
// Returns true if can write, false if can't.
static bool uart_tx_wait(pyb_uart_obj_t *self, uint32_t timeout)
{
    uint32_t start = mp_hal_ticks_ms();
    uart_t *psUARTObj = self->psUARTObj;

    if(psUARTObj == NULL)
        return false;

    for (;;) {
        if (psUARTObj->bLPUART) {
            if (LPUART_IS_TX_EMPTY(psUARTObj->u_uart.lpuart)) {
                return true; // tx register is empty
            }
        } else {
            if (UART_IS_TX_EMPTY(psUARTObj->u_uart.uart)) {
                return true; // tx register is empty
            }
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
static bool uart_rx_wait(pyb_uart_obj_t *self, uint32_t timeout)
{
    uint32_t start = mp_hal_ticks_ms();
    uart_t *psUARTObj = self->psUARTObj;

    if(psUARTObj == NULL)
        return false;

    for (;;) {
        if (psUARTObj->bLPUART) {
            if (self->read_buf_tail != self->read_buf_head || LPUART_IS_RX_READY(psUARTObj->u_uart.lpuart)) {
                return true; // have at least 1 char ready for reading
            }
        } else {
            if (self->read_buf_tail != self->read_buf_head || UART_IS_RX_READY(psUARTObj->u_uart.uart)) {
                return true; // have at least 1 char ready for reading
            }
        }
        if (mp_hal_ticks_ms() - start >= timeout) {
            return false; // timeout
        }
        MICROPY_EVENT_POLL_HOOK
    }
}

static int uart_rx_char(pyb_uart_obj_t *self)
{
    uart_t *psUARTObj = self->psUARTObj;

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
        if (psUARTObj->bLPUART) {
            return LPUART_READ(psUARTObj->u_uart.lpuart);
        } else {
            return UART_READ(psUARTObj->u_uart.uart);
        }
    }
}

// src - a pointer to the data to send (16-bit aligned for 9-bit chars)
// num_chars - number of characters to send (9-bit chars count for 2 bytes from src)
// *errcode - returns 0 for success, MP_Exxx on error
// returns the number of characters sent (valid even if there was an error)
static size_t uart_tx_data(pyb_uart_obj_t *self, const void *src_in, size_t num_chars, int *errcode)
{
    uart_t *psUARTObj = self->psUARTObj;

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
        if (!uart_tx_wait(self, timeout)) {
            *errcode = MP_ETIMEDOUT;
            return num_tx;
        }
        uint32_t data;
        data = *src++;
        if (psUARTObj->bLPUART) {
            LPUART_WRITE(psUARTObj->u_uart.lpuart, data);
        } else {
            UART_WRITE(psUARTObj->u_uart.uart, data);
        }
        ++num_tx;
    }

    *errcode = 0;
    return num_tx;

}

/******************************************************************************/
/* MicroPython bindings                                                       */
/******************************************************************************/

static void pyb_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_uart_obj_t *self = self_in;
    if (!self->is_enabled) {
        mp_printf(print, "UART(%u)", self->uart_id);
    } else {
        mp_int_t bits;
        switch (self->u32DataWidth) {
        case UART_WORD_LEN_5:
            bits = 5;
            break;
        case UART_WORD_LEN_6:
            bits = 6;
            break;
        case UART_WORD_LEN_7:
            bits = 7;
            break;
        case UART_WORD_LEN_8:
            bits = 8;
            break;
        default:
            bits = 8;
            break;
        }
        mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=",
                  self->uart_id, self->u32BaudRate, bits);

        if (self->u32Parity == UART_PARITY_NONE) {
            mp_print_str(print, "None");
        } else {
            mp_printf(print, "%u", self->u32Parity == UART_PARITY_EVEN ? 0 : 1);
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
        mp_printf(print, ", stop=%u, timeout=%u, timeout_char=%u, read_buf_len=%u)",
                  self->u32StopBits == UART_STOP_BIT_1 ? 1 : 2,
                  self->timeout, self->timeout_char,
                  self->read_buf_len == 0 ? 0 : self->read_buf_len - 1); // -1 to adjust for usable length of buffer
    }
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
static mp_obj_t pyb_uart_init_helper(pyb_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
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
        self->u32Parity = UART_PARITY_NONE;
    } else {
        mp_int_t parity = mp_obj_get_int(args.parity.u_obj);
        self->u32Parity = (parity & 1) ? UART_PARITY_ODD : UART_PARITY_EVEN;
    }

    mp_int_t bits = args.bits.u_int;
    if(bits == 5) {
        self->u32DataWidth = UART_WORD_LEN_5;
        self->char_mask = 0x1f;
    } else if (bits == 6) {
        self->u32DataWidth = UART_WORD_LEN_6;
        self->char_mask = 0x3f;
    } else if (bits == 7) {
        self->u32DataWidth = UART_WORD_LEN_7;
        self->char_mask = 0x7f;
    } else if (bits == 8) {
        self->u32DataWidth = UART_WORD_LEN_8;
        self->char_mask = 0xff;
    } else {
        mp_raise_ValueError("unsupported combination of bits and parity");
    }

    // stop bits
    switch (args.stop.u_int) {
    case 1:
        self->u32StopBits = UART_STOP_BIT_1;
        break;
    default:
        self->u32StopBits = UART_STOP_BIT_2;
        break;
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

    //enable uart clock
    enable_uart_clock(self, true);

    UART_InitTypeDef sUARTInit;

    //Open UART port
    sUARTInit.u32BaudRate = self->u32BaudRate;
    sUARTInit.u32DataWidth = self->u32DataWidth;
    sUARTInit.u32Parity = self->u32Parity;
    sUARTInit.u32StopBits = self->u32StopBits;
    sUARTInit.eFlowControl = self->u32FlowControl;

    self->psUARTObj->dma_usage = ePDMA_USAGE_NEVER;
    self->psUARTObj->dma_chn_id_tx = 0;
    self->psUARTObj->dma_chn_id_rx = 0;
    self->psUARTObj->event = 0;
    self->psUARTObj->hdlr_async = 0;
    self->psUARTObj->hdlr_dma_tx = NULL;
    self->psUARTObj->hdlr_dma_rx = NULL;

    if(UART_Init(self->psUARTObj, &sUARTInit, 0) != 0) {
        mp_raise_ValueError("Unable open UART device");
    } else {
        //switch uart pin
        switch_pinfun(self, true, self->u32FlowControl);
        self->is_enabled = true;
    }

    return mp_const_none;

}

/// \classmethod \constructor(bus, ...)
///
/// Construct a UART object on the given bus.
/// With no additional parameters, the UART object is created but not
/// initialised (it has the settings from the last initialisation of
/// the bus, if any).  If extra arguments are given, the bus is initialised.
/// See `init` for parameters of initialisation.
static mp_obj_t pyb_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the timer id
    mp_int_t uart_id = mp_obj_get_int(args[0]);

    // check if the timer exists
    if (uart_id < 0 || uart_id >= M5531_MAX_UART_INST) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) doesn't exist", uart_id));
    }

    pyb_uart_obj_t *self = &pyb_uart_obj[uart_id];

    if(self->attached_to_repl) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) is attached to REPL", uart_id));
    }

    if(self->psUARTObj == NULL) {
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

static mp_obj_t pyb_uart_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    return pyb_uart_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pyb_uart_init_obj, 1, pyb_uart_init);

/// \method deinit()
/// Turn off the UART bus.
static mp_obj_t pyb_uart_deinit(mp_obj_t self_in)
{
    pyb_uart_obj_t *self = self_in;

    if(self->psUARTObj == NULL)
        return mp_const_none;

    UART_Final(self->psUARTObj);
    self->is_enabled = false;

    switch_pinfun(self, false, self->u32FlowControl);

    enable_uart_clock(self, false);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_deinit_obj, pyb_uart_deinit);

/// \method any()
/// Return `True` if any characters waiting, else `False`.
static mp_obj_t pyb_uart_any(mp_obj_t self_in)
{
    pyb_uart_obj_t *self = self_in;
    uart_t *psUARTObj = self->psUARTObj;

    if(psUARTObj == NULL) {
        return MP_OBJ_NEW_SMALL_INT(0);
    }

    if (psUARTObj->bLPUART) {
        return MP_OBJ_NEW_SMALL_INT(LPUART_IS_RX_READY(psUARTObj->u_uart.lpuart));
    }

    return MP_OBJ_NEW_SMALL_INT(UART_IS_RX_READY(psUARTObj->u_uart.uart));
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_any_obj, pyb_uart_any);

/// \method writechar(char)
/// Write a single character on the bus.  `char` is an integer to write.
/// Return value: `None`.
static mp_obj_t pyb_uart_writechar(mp_obj_t self_in, mp_obj_t char_in)
{
    pyb_uart_obj_t *self = self_in;

    // get the character to write
    uint16_t data = mp_obj_get_int(char_in);

    // write the character
    int errcode = 0;

    if (uart_tx_wait(self, self->timeout)) {
        uart_tx_data(self, (uint8_t *)&data, 1, &errcode);
    } else {
        errcode = MP_ETIMEDOUT;
    }

    if (errcode != 0) {
        mp_raise_OSError(errcode);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(pyb_uart_writechar_obj, pyb_uart_writechar);

/// \method readchar()
/// Receive a single character on the bus.
/// Return value: The character read, as an integer.  Returns -1 on timeout.
static mp_obj_t pyb_uart_readchar(mp_obj_t self_in)
{
    pyb_uart_obj_t *self = self_in;

    if (uart_rx_wait(self, self->timeout)) {
        return MP_OBJ_NEW_SMALL_INT(uart_rx_char(self));
    } else {
        // return -1 on timeout
        return MP_OBJ_NEW_SMALL_INT(-1);
    }
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_readchar_obj, pyb_uart_readchar);

static const mp_rom_map_elem_t pyb_uart_locals_dict_table[] = {
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

#if 0
    { MP_ROM_QSTR(MP_QSTR_sendbreak), MP_ROM_PTR(&pyb_uart_sendbreak_obj) },

#endif
};


static MP_DEFINE_CONST_DICT(pyb_uart_locals_dict, pyb_uart_locals_dict_table);


static mp_uint_t pyb_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode)
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

static mp_uint_t pyb_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode)
{
    pyb_uart_obj_t *self = self_in;
    const byte *buf = buf_in;

    // wait to be able to write the first character. EAGAIN causes write to return None
    if (!uart_tx_wait(self, self->timeout)) {
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

static mp_uint_t pyb_uart_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode)
{
    pyb_uart_obj_t *self = self_in;
    mp_uint_t ret;
    uart_t *psUARTObj = self->psUARTObj;

    if(psUARTObj == NULL) {
        *errcode = MP_EINVAL;
        return MP_STREAM_ERROR;
    }

    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if (psUARTObj->bLPUART) {
            if ((flags & MP_STREAM_POLL_RD) && LPUART_IS_RX_READY(psUARTObj->u_uart.lpuart)) {
                ret |= MP_STREAM_POLL_RD;
            }
            if ((flags & MP_STREAM_POLL_WR) && LPUART_IS_TX_EMPTY(psUARTObj->u_uart.lpuart)) {
                ret |= MP_STREAM_POLL_WR;
            }
        } else {
            if ((flags & MP_STREAM_POLL_RD) && UART_IS_RX_READY(psUARTObj->u_uart.uart)) {
                ret |= MP_STREAM_POLL_RD;
            }
            if ((flags & MP_STREAM_POLL_WR) && UART_IS_TX_EMPTY(psUARTObj->u_uart.uart)) {
                ret |= MP_STREAM_POLL_WR;
            }
        }

    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

static const mp_stream_p_t uart_stream_p = {
    .read = pyb_uart_read,
    .write = pyb_uart_write,
    .ioctl = pyb_uart_ioctl,
    .is_text = false,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_uart_type,
    MP_QSTR_UART,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    make_new, pyb_uart_make_new,
    print, pyb_uart_print,
    protocol, &uart_stream_p,
    locals_dict, &pyb_uart_locals_dict
);
