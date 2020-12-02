/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "extmod/misc.h"

// this table converts from HAL_StatusTypeDef to POSIX errno
const byte mp_hal_status_to_errno_table[4] = {
    [HAL_OK] = 0,
    [HAL_ERROR] = MP_EIO,
    [HAL_BUSY] = MP_EBUSY,
    [HAL_TIMEOUT] = MP_ETIMEDOUT,
};

NORETURN void mp_hal_raise(HAL_StatusTypeDef status) {
    mp_raise_OSError(mp_hal_status_to_errno_table[status]);
}


#if MICROPY_PY_OS_DUPTERM
static int call_dupterm_read(size_t idx) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t read_m[3];
        mp_load_method(MP_STATE_VM(dupterm_objs[idx]), MP_QSTR_read, read_m);
        read_m[2] = MP_OBJ_NEW_SMALL_INT(1);
        mp_obj_t res = mp_call_method_n_kw(1, 0, read_m);
        if (res == mp_const_none) {
            return -2;
        }
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(res, &bufinfo, MP_BUFFER_READ);
        if (bufinfo.len == 0) {
            mp_printf(&mp_plat_print, "dupterm: EOF received, deactivating\n");
            MP_STATE_VM(dupterm_objs[idx]) = MP_OBJ_NULL;
            return -1;
        }
        nlr_pop();
        return *(byte*)bufinfo.buf;
    } else {
        // Temporarily disable dupterm to avoid infinite recursion
        mp_obj_t save_term = MP_STATE_VM(dupterm_objs[idx]);
        MP_STATE_VM(dupterm_objs[idx]) = NULL;
        mp_printf(&mp_plat_print, "dupterm: ");
        mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
        MP_STATE_VM(dupterm_objs[idx]) = save_term;
    }

    return -1;
}
#endif

/////////////////////////////////////////////////////////////////////////////////////
//	REPL read write to USB
///////////////////////////////////////////////////////////////////////////////////////
#define REPL_TO_USB

#if defined(REPL_TO_USB)

#include "N9H26_USBDev.h"

static void SendStr_ToUSB(char *ptr, int len)
{
	S_USBDEV_STATE *psUSBState;
	int i32CanSend = 0;
	
	psUSBState = USBDEV_UpdateState();
	
	if(psUSBState->eUSBMode != eUSBDEV_MODE_MSC_VCP)
		return;
		
	do{
		i32CanSend = USBDEV_VCPCanSend(psUSBState);
		if(i32CanSend == -1)
			return;
		if(i32CanSend)
			break;
	}while(1);
	
	USBDEV_VCPSendData((uint8_t *)ptr, len, 0, psUSBState);

//	if(ptr[len] == '\n')
//	{
//		char chTemp;
//		chTemp = '\r';
//		USBDEV_VCPSendData((uint8_t *)&chTemp, 1, 10, psUSBState);		
//	}
}

static char RecvChar_FromUSB(void)
{
	S_USBDEV_STATE *psUSBState;
	
	psUSBState = USBDEV_UpdateState();
	
	if(psUSBState->eUSBMode != eUSBDEV_MODE_MSC_VCP)
		return 0;
	
	while(1){
		if(USBDEV_VCPCanRecv(psUSBState)){
			char chTemp;
			USBDEV_VCPRecvData((uint8_t *)&chTemp,1 ,10, psUSBState);
			return chTemp;
		}
		vTaskDelay(1);
	}
}


int REPL_write (int fd, char *ptr, int len)
{
	SendStr_ToUSB(ptr, len);
    return len;
}

int REPL_read(int fd, char *ptr, int len)
{
	*ptr = RecvChar_FromUSB();
    return 1;
}

#endif

int mp_hal_stdin_rx_chr(void) {
    unsigned char c;
#if MICROPY_PY_OS_DUPTERM
    // TODO only support dupterm one slot at the moment
    if (MP_STATE_VM(dupterm_objs[0]) != MP_OBJ_NULL) {
        int c;
        do {
             c = call_dupterm_read(0);
        } while (c == -2);
        if (c == -1) {
            goto main_term;
        }
        if (c == '\n') {
            c = '\r';
        }
        return c;
    } else {
        main_term:;
#endif
#if defined (REPL_TO_USB)
        int ret = REPL_read(0, (char *)&c, 1);
#else
        int ret = read(0, &c, 1);
#endif
        if (ret == 0) {
            c = 4; // EOF, ctrl-D
        } else if (c == '\n') {
            c = '\r';
        }

        return c;
#if MICROPY_PY_OS_DUPTERM
    }
#endif
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {

#if defined (REPL_TO_USB)
    int ret = REPL_write(1, (char *)str, len);
#else
    int ret = write(1, str, len);
#endif

    mp_uos_dupterm_tx_strn(str, len);
    (void)ret; // to suppress compiler warning
}

// Efficiently convert "\n" to "\r\n"
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    const char *last = str;
    while (len--) {
        if (*str == '\n') {
            if (str > last) {
                mp_hal_stdout_tx_strn(last, str - last);
            }
            mp_hal_stdout_tx_strn("\r\n", 2);
            ++str;
            last = str;
        } else {
            ++str;
        }
    }
    if (str > last) {
        mp_hal_stdout_tx_strn(last, str - last);
    }
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

