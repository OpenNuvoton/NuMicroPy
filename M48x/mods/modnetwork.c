/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2015 Daniel Campora
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

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "modnetwork.h"

/// \module network - network configuration
///
/// This module provides network drivers and routing configuration.

void mod_network_init(void) {
    mp_obj_list_init(&MP_STATE_PORT(mod_network_nic_list), 0);
}

void mod_network_deinit(void) {
}

void mod_network_register_nic(mp_obj_t nic) {
    for (mp_uint_t i = 0; i < MP_STATE_PORT(mod_network_nic_list).len; i++) {
        if (MP_STATE_PORT(mod_network_nic_list).items[i] == nic) {
            // nic already registered
            return;
        }
    }
    // nic not registered so add to list
    mp_obj_list_append(MP_OBJ_FROM_PTR(&MP_STATE_PORT(mod_network_nic_list)), nic);
}

mp_obj_t mod_network_find_nic(const uint8_t *ip) {
    // find a NIC that is suited to given IP address
    for (mp_uint_t i = 0; i < MP_STATE_PORT(mod_network_nic_list).len; i++) {
        mp_obj_t nic = MP_STATE_PORT(mod_network_nic_list).items[i];
        // TODO check IP suitability here
        //mod_network_nic_type_t *nic_type = (mod_network_nic_type_t*)mp_obj_get_type(nic);
        return nic;
    }

    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "no available NIC"));
}

STATIC const mp_rom_map_elem_t mp_module_network_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),	MP_ROM_QSTR(MP_QSTR_network) },
#if MICROPY_NVT_LWIP
    { MP_ROM_QSTR(MP_QSTR_LAN),		MP_ROM_PTR(&mod_network_nic_type_lan) },
#endif
#if MICROPY_WLAN_ESP8266
    { MP_ROM_QSTR(MP_QSTR_WLAN),	MP_ROM_PTR(&mod_network_nic_type_esp8266) },
#endif

};

STATIC MP_DEFINE_CONST_DICT(network_module_globals, mp_module_network_globals_table);

const mp_obj_module_t mp_module_network = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&network_module_globals,
};


