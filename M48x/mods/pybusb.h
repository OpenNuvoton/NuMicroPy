/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014, 2015 Damien P. George
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
#ifndef MICROPY_INCLUDED_M48X_USB_H
#define MICROPY_INCLUDED_M48X_USB_H


// Windows needs a different PID to distinguish different device configurations
#define MP_USBD_VID         	(0x0416)
#define MP_USBD_PID_HID	 	(0x5021)
#define MP_USBD_PID_VCP	 	(0x5011)
#define MP_USBD_PID_VCP_HID	(0xDC00)

extern const mp_obj_type_t pyb_usb_hid_type;
extern const mp_obj_type_t pyb_usb_vcp_type;

MP_DECLARE_CONST_FUN_OBJ_KW(pyb_usb_mode_obj);
#endif //MICROPY_INCLUDED_M48X_USB_H
