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

#include <stdarg.h>
#include <string.h>


#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "bufhelper.h"

#include "hal/M48x_USBD.h"
#include "pybusb.h"

#if MICROPY_HW_ENABLE_USBD


typedef struct _usb_device_t {
    bool enabled;
//    USBD_HandleTypeDef hUSBDDevice;
    S_USBDEV_STATE *psUSBDev_vdp_hid_state;
//    usbd_cdc_itf_t usbd_cdc_itf;
//    usbd_hid_itf_t usbd_hid_itf;
} usb_device_t;

usb_device_t usb_device = {
	false,
	NULL,
};

/******************************************************************************/
// MicroPython bindings for USB

/*
  Philosophy of USB driver and Python API: pyb.usb_mode(...) configures the USB
  on the board.  The USB itself is not an entity, rather the interfaces are, and
  can be accessed by creating objects, such as pyb.USB_VCP() and pyb.USB_HID().

  We have:

    pyb.usb_mode()          # return the current usb mode
    pyb.usb_mode(None)      # disable USB
    pyb.usb_mode('VCP')     # enable with VCP interface
    pyb.usb_mode('VCP+HID') # enable with VCP and HID, defaulting to mouse protocol
    pyb.usb_mode('VCP+HID', vid=0xf055, pid=0x9800) # specify VID and PID
    pyb.usb_mode('VCP+HID', hid=pyb.hid_mouse)
    pyb.usb_mode('VCP+HID', hid=pyb.hid_keyboard)
    pyb.usb_mode('VCP+HID', pid=0x1234, hid=(subclass, protocol, max_packet_len, polling_interval, report_desc))

    vcp = pyb.USB_VCP() # get the VCP device for read/write
    hid = pyb.USB_HID() # get the HID device for write/poll

  Possible extensions:
    pyb.usb_mode('host', ...)
    pyb.usb_mode('OTG', ...)
    pyb.usb_mode(..., port=2) # for second USB port
*/

STATIC mp_obj_t pyb_usb_mode(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_vid, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MP_USBD_VID} },
        { MP_QSTR_pid, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
//        { MP_QSTR_hid, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = (mp_obj_t)&pyb_usb_hid_mouse_obj} },
    };

   // fetch the current usb mode -> pyb.usb_mode()
    if (n_args == 0) {
        E_USBDEV_MODE mode = USBDEV_GetMode(usb_device.psUSBDev_vdp_hid_state);
        switch (mode) {
#if 0
            case eUSBDEV_MODE_VCP:
                return MP_OBJ_NEW_QSTR(MP_QSTR_VCP);
#endif
            case eUSBDEV_MODE_HID:
                return MP_OBJ_NEW_QSTR(MP_QSTR_HID);
#if 0
            case eUSBDEV_MODE_HID_VCP:
                return MP_OBJ_NEW_QSTR(MP_QSTR_VCP_plus_HID);
#endif
            default:
                return mp_const_none;
        }
    }

	// parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // check if user wants to disable the USB
    if (args[0].u_obj == mp_const_none) {
        // disable usb
		USBDEV_Deinit(usb_device.psUSBDev_vdp_hid_state);
		usb_device.psUSBDev_vdp_hid_state = NULL;
		usb_device.enabled = false;
        return mp_const_none;
    }

	if(usb_device.enabled){
		mp_raise_ValueError("USB device already enabled");
	}

    // get mode string
    const char *mode_str = mp_obj_str_get_str(args[0].u_obj);

    // hardware configured for USB device mode

    // get the VID, PID and USB mode
    // note: PID=-1 means select PID based on mode
    // note: we support CDC as a synonym for VCP for backward compatibility
    uint16_t vid = args[1].u_int;
    uint16_t pid = args[2].u_int;
    E_USBDEV_MODE mode;

#if 0
	if (strcmp(mode_str, "CDC+HID") == 0 || strcmp(mode_str, "VCP+HID") == 0) {
        if (args[2].u_int == -1) {
            pid = MP_USBD_PID_VCP_HID;
        }
        mode = eUSBDEV_MODE_HID_VCP;
    } else if (strcmp(mode_str, "CDC") == 0 || strcmp(mode_str, "VCP") == 0) {
        if (args[2].u_int == -1) {
            pid = MP_USBD_PID_VCP;
        }
		mode = eUSBDEV_MODE_VCP;
    } else if (strcmp(mode_str, "HID") == 0) {
#else
    if (strcmp(mode_str, "HID") == 0) {
#endif
        if (args[2].u_int == -1) {
            pid = MP_USBD_PID_HID;
        }
		mode = eUSBDEV_MODE_HID;
    } else {
        mp_raise_ValueError("bad USB mode");
    }

	usb_device.psUSBDev_vdp_hid_state = USBDEV_Init(vid, pid, mode);
	if(usb_device.psUSBDev_vdp_hid_state == NULL){
		mp_raise_ValueError("bad USB mode");
	}

    /* Start transaction */
    printf("Waiting USB attached \n");
    while(1)
    {
        if (USBD_IS_ATTACHED())
        {
            USBDEV_Start(usb_device.psUSBDev_vdp_hid_state);
			printf("Start USB device HID/VCP class \n");
            break;
        }
    }

	usb_device.enabled = true;
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_mode_obj, 0, pyb_usb_mode);

/******************************************************************************/
// MicroPython bindings for USB HID
typedef struct _pyb_usb_hid_obj_t {
    mp_obj_base_t base;
    usb_device_t *usb_dev;
} pyb_usb_hid_obj_t;

STATIC const pyb_usb_hid_obj_t pyb_usb_hid_obj = {{&pyb_usb_hid_type}, &usb_device};

STATIC mp_obj_t pyb_usb_hid_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // TODO raise exception if USB is not configured for HID

    // return the USB HID object
    return (mp_obj_t)&pyb_usb_hid_obj;
}

/// \method recv(data, *, timeout=5000)
///
/// Receive data on the bus:
///
///   - `data` can be an integer, which is the number of bytes to receive,
///     or a mutable buffer, which will be filled with received bytes.
///   - `timeout` is the timeout in milliseconds to wait for the receive.
///
/// Return value: if `data` is an integer then a new buffer of the bytes received,
/// otherwise the number of bytes read into `data` is returned.
STATIC mp_obj_t pyb_usb_hid_recv(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_data,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_usb_hid_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);

    // get the buffer to receive into
    vstr_t vstr;
    mp_obj_t o_ret = pyb_buf_get_for_recv(vals[0].u_obj, &vstr);

    // receive the data
    int ret = USBDEV_HIDRecvData((uint8_t*)vstr.buf, vstr.len, vals[1].u_int, self->usb_dev->psUSBDev_vdp_hid_state);

    // return the received data
    if (o_ret != MP_OBJ_NULL) {
        return mp_obj_new_int(ret); // number of bytes read into given buffer
    } else {
        vstr.len = ret; // set actual number of bytes read
        return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr); // create a new buffer
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_hid_recv_obj, 1, pyb_usb_hid_recv);

STATIC mp_obj_t pyb_usb_hid_send(mp_obj_t self_in, mp_obj_t report_in) {

    pyb_usb_hid_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;
    byte temp_buf[64];

    // get the buffer to send from
    // we accept either a byte array, or a tuple/list of integers
    if (!mp_get_buffer(report_in, &bufinfo, MP_BUFFER_READ)) {
        mp_obj_t *items;
        mp_obj_get_array(report_in, &bufinfo.len, &items);
        if (bufinfo.len > sizeof(temp_buf)) {
            mp_raise_ValueError("tuple/list too large for HID report; use bytearray instead");
        }
        for (int i = 0; i < bufinfo.len; i++) {
            temp_buf[i] = mp_obj_get_int(items[i]);
        }
        bufinfo.buf = temp_buf;
    }
    
    // send the data
    int ret = USBDEV_HIDSendData(bufinfo.buf, bufinfo.len, 5000, self->usb_dev->psUSBDev_vdp_hid_state); 

    if (ret >= 0) {
        return mp_obj_new_int(ret);
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_usb_hid_send_obj, pyb_usb_hid_send);

STATIC mp_obj_t pyb_usb_hid_send_packet_size(mp_obj_t self_in){
	return mp_obj_new_int(USBDEV_HIDInReportPacketSize());
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_usb_hid_send_packet_size_obj, pyb_usb_hid_send_packet_size);

STATIC mp_obj_t pyb_usb_hid_recv_packet_size(mp_obj_t self_in) {
	return mp_obj_new_int(USBDEV_HIDOutReportPacketSize());
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_usb_hid_recv_packet_size_obj, pyb_usb_hid_recv_packet_size);


STATIC const mp_rom_map_elem_t pyb_usb_hid_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&pyb_usb_hid_send_obj) },
	{ MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&pyb_usb_hid_recv_obj) },
	{ MP_ROM_QSTR(MP_QSTR_send_packet_size), MP_ROM_PTR(&pyb_usb_hid_send_packet_size_obj) },
	{ MP_ROM_QSTR(MP_QSTR_recv_packet_size), MP_ROM_PTR(&pyb_usb_hid_recv_packet_size_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_usb_hid_locals_dict, pyb_usb_hid_locals_dict_table);

STATIC mp_uint_t pyb_usb_hid_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    pyb_usb_hid_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && USBDEV_HIDCanRecv(self->usb_dev->psUSBDev_vdp_hid_state) > 0) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && USBDEV_HIDCanSend(self->usb_dev->psUSBDev_vdp_hid_state)) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

STATIC const mp_stream_p_t pyb_usb_hid_stream_p = {
    .ioctl = pyb_usb_hid_ioctl,
};

const mp_obj_type_t pyb_usb_hid_type = {
    { &mp_type_type },
    .name = MP_QSTR_USB_HID,
    .make_new = pyb_usb_hid_make_new,
    .protocol = &pyb_usb_hid_stream_p,
    .locals_dict = (mp_obj_dict_t*)&pyb_usb_hid_locals_dict,
};

/******************************************************************************/
// MicroPython bindings for USB VCP

/// \moduleref pyb
/// \class USB_VCP - USB virtual comm port
///
/// The USB_VCP class allows creation of an object representing the USB
/// virtual comm port.  It can be used to read and write data over USB to
/// the connected host.

typedef struct _pyb_usb_vcp_obj_t {
    mp_obj_base_t base;
    usb_device_t *usb_dev;
} pyb_usb_vcp_obj_t;

STATIC const pyb_usb_vcp_obj_t pyb_usb_vcp_obj = {{&pyb_usb_vcp_type}, &usb_device};

STATIC void pyb_usb_vcp_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_print_str(print, "USB_VCP()");
}

/// \classmethod \constructor()
/// Create a new USB_VCP object.
STATIC mp_obj_t pyb_usb_vcp_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t
 *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // TODO raise exception if USB is not configured for VCP

    // return the USB VCP object
    return (mp_obj_t)&pyb_usb_vcp_obj;
}

STATIC mp_obj_t pyb_usb_vcp_setinterrupt(mp_obj_t self_in, mp_obj_t int_chr_in) {
    mp_hal_set_interrupt_char(mp_obj_get_int(int_chr_in));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_usb_vcp_setinterrupt_obj, pyb_usb_vcp_setinterrupt);

STATIC mp_obj_t pyb_usb_vcp_isconnected(mp_obj_t self_in) {
//    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(USBD_IS_ATTACHED());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_usb_vcp_isconnected_obj, pyb_usb_vcp_isconnected);

/// \method any()
/// Return `True` if any characters waiting, else `False`.
STATIC mp_obj_t pyb_usb_vcp_any(mp_obj_t self_in) {
    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (USBDEV_VCPCanRecv(self->usb_dev->psUSBDev_vdp_hid_state) > 0) {
        return mp_const_true;
    } else {
        return mp_const_false;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_usb_vcp_any_obj, pyb_usb_vcp_any);

/// \method send(data, *, timeout=5000)
/// Send data over the USB VCP:
///
///   - `data` is the data to send (an integer to send, or a buffer object).
///   - `timeout` is the timeout in milliseconds to wait for the send.
///
/// Return value: number of bytes sent.
STATIC const mp_arg_t pyb_usb_vcp_send_args[] = {
    { MP_QSTR_data,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
};
#define PYB_USB_VCP_SEND_NUM_ARGS MP_ARRAY_SIZE(pyb_usb_vcp_send_args)

STATIC mp_obj_t pyb_usb_vcp_send(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    // parse args
    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_arg_val_t vals[PYB_USB_VCP_SEND_NUM_ARGS];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, PYB_USB_VCP_SEND_NUM_ARGS, pyb_usb_vcp_send_args, vals);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(vals[0].u_obj, &bufinfo, data);

    // send the data
    int ret = USBDEV_VCPSendData(bufinfo.buf, bufinfo.len, vals[1].u_int, self->usb_dev->psUSBDev_vdp_hid_state);

    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_vcp_send_obj, 1, pyb_usb_vcp_send);

/// \method recv(data, *, timeout=5000)
///
/// Receive data on the bus:
///
///   - `data` can be an integer, which is the number of bytes to receive,
///     or a mutable buffer, which will be filled with received bytes.
///   - `timeout` is the timeout in milliseconds to wait for the receive.
///
/// Return value: if `data` is an integer then a new buffer of the bytes received,
/// otherwise the number of bytes read into `data` is returned.
STATIC mp_obj_t pyb_usb_vcp_recv(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    // parse args
    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_arg_val_t vals[PYB_USB_VCP_SEND_NUM_ARGS];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, PYB_USB_VCP_SEND_NUM_ARGS, pyb_usb_vcp_send_args, vals);

    // get the buffer to receive into
    vstr_t vstr;
    mp_obj_t o_ret = pyb_buf_get_for_recv(vals[0].u_obj, &vstr);

    // receive the data
    int ret = USBDEV_VCPRecvData((uint8_t*)vstr.buf, vstr.len, vals[1].u_int, self->usb_dev->psUSBDev_vdp_hid_state);

    // return the received data
    if (o_ret != MP_OBJ_NULL) {
        return mp_obj_new_int(ret); // number of bytes read into given buffer
    } else {
        vstr.len = ret; // set actual number of bytes read
        return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr); // create a new buffer
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_vcp_recv_obj, 1, pyb_usb_vcp_recv);

mp_obj_t pyb_usb_vcp___exit__(size_t n_args, const mp_obj_t *args) {
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_usb_vcp___exit___obj, 4, 4, pyb_usb_vcp___exit__);

STATIC const mp_rom_map_elem_t pyb_usb_vcp_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_setinterrupt), MP_ROM_PTR(&pyb_usb_vcp_setinterrupt_obj) },
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&pyb_usb_vcp_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&pyb_usb_vcp_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&pyb_usb_vcp_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&pyb_usb_vcp_recv_obj) },

    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj)},
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_identity_obj) },

    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&pyb_usb_vcp___exit___obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_usb_vcp_locals_dict, pyb_usb_vcp_locals_dict_table);

STATIC mp_uint_t pyb_usb_vcp_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int ret = USBDEV_VCPRecvData((byte*)buf, size, 0, self->usb_dev->psUSBDev_vdp_hid_state);
    if (ret == 0) {
        // return EAGAIN error to indicate non-blocking
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }
    return ret;
}

STATIC mp_uint_t pyb_usb_vcp_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int ret = USBDEV_VCPSendData((uint8_t *)buf, size, 0, self->usb_dev->psUSBDev_vdp_hid_state);
    if (ret == 0) {
        // return EAGAIN error to indicate non-blocking
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }
    return ret;
}

STATIC mp_uint_t pyb_usb_vcp_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    mp_uint_t ret;
    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && USBDEV_VCPCanRecv(self->usb_dev->psUSBDev_vdp_hid_state) > 0) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && USBDEV_VCPCanSend(self->usb_dev->psUSBDev_vdp_hid_state)) {
            ret |= MP_STREAM_POLL_WR;
        }

    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}


STATIC const mp_stream_p_t pyb_usb_vcp_stream_p = {
    .read = pyb_usb_vcp_read,
    .write = pyb_usb_vcp_write,
    .ioctl = pyb_usb_vcp_ioctl,
};

const mp_obj_type_t pyb_usb_vcp_type = {
    { &mp_type_type },
    .name = MP_QSTR_USB_VCP,
    .print = pyb_usb_vcp_print,
    .make_new = pyb_usb_vcp_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &pyb_usb_vcp_stream_p,
    .locals_dict = (mp_obj_dict_t*)&pyb_usb_vcp_locals_dict,
};


#endif


STATIC mp_obj_t pyb_msc_enable(void) {
	USBDEV_MSCEnDisable(1);
	return mp_const_none;
}

STATIC mp_obj_t pyb_msc_disable(void) {
	USBDEV_MSCEnDisable(0);
	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(pyb_msc_enable_obj, pyb_msc_enable);
MP_DEFINE_CONST_FUN_OBJ_0(pyb_msc_disable_obj, pyb_msc_disable);




