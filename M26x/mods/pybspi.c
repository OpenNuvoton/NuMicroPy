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
#include "bufhelper.h"

#include "extmod/machine_spi.h"

#include "pybirq.h"
#include "pybspi.h"

#if MICROPY_HW_ENABLE_HW_SPI

/// \moduleref pyb
/// \class SPI - a master-driven serial protocol
///
/// SPI is a serial protocol that is driven by a master.  At the physical level
/// there are 3 lines: SCK, MOSI, MISO.
///
/// See usage model of I2C; SPI is very similar.  Main difference is
/// parameters to init the SPI bus:
///
///     from pyb import SPI
///     spi = SPI(1, SPI.MASTER, baudrate=600000, polarity=1, phase=0, crc=0x7)
///
/// Only required parameter is mode, SPI.MASTER or SPI.SLAVE.  Polarity can be
/// 0 or 1, and is the level the idle clock line sits at.  Phase can be 0 or 1
/// to sample data on the first or second clock edge respectively.  Crc can be
/// None for no CRC, or a polynomial specifier.
///
/// Additional method for SPI:
///
///     data = spi.send_recv(b'1234')        # send 4 bytes and receive 4 bytes
///     buf = bytearray(4)
///     spi.send_recv(b'1234', buf)          # send 4 bytes and receive 4 into buf
///     spi.send_recv(buf, buf)              # send/recv 4 bytes from/to buf

#define PYB_SPI_MASTER (0)
#define PYB_SPI_SLAVE  (1)


#if defined(MICROPY_HW_SPI0_SCK)
STATIC SPI_InitTypeDef s_sSPI0InitDef;
STATIC spi_t s_sSPI0Obj = {.spi = SPI0};
#endif
#if defined(MICROPY_HW_SPI1_SCK)
STATIC SPI_InitTypeDef s_sSPI1InitDef;
STATIC spi_t s_sSPI1Obj = {.spi = SPI1};
#endif
#if defined(MICROPY_HW_SPI2_SCK)
STATIC SPI_InitTypeDef s_sSPI2InitDef;
STATIC spi_t s_sSPI2Obj = {.spi = SPI2};
#endif
#if defined(MICROPY_HW_SPI3_SCK)
STATIC SPI_InitTypeDef s_sSPI3InitDef;
STATIC spi_t s_sSPI3Obj = {.spi = SPI3};
#endif


STATIC const pyb_spi_obj_t pyb_spi_obj[4] = {
    #if defined(MICROPY_HW_SPI0_SCK)
    {{&pyb_spi_type}, &s_sSPI0Obj, &s_sSPI0InitDef},
    #else
    {{&pyb_spi_type},NULL, NULL},
    #endif
    #if defined(MICROPY_HW_SPI1_SCK)
    {{&pyb_spi_type}, &s_sSPI1Obj, &s_sSPI1InitDef},
    #else
    {{&pyb_spi_type},NULL, NULL},
    #endif
    #if defined(MICROPY_HW_SPI2_SCK)
    {{&pyb_spi_type}, &s_sSPI2Obj, &s_sSPI2InitDef},
    #else
    {{&pyb_spi_type},NULL, NULL},
    #endif
    #if defined(MICROPY_HW_SPI3_SCK)
    {{&pyb_spi_type}, &s_sSPI3Obj, &s_sSPI3InitDef},
    #else
    {{&pyb_spi_type},NULL, NULL},
    #endif
};


STATIC void switch_pinfun(const pyb_spi_obj_t *self, bool bSPI){
	int spi_unit;
	const pin_obj_t *nss_pin;
	const pin_obj_t *sck_pin;
	const pin_obj_t *miso_pin;
	const pin_obj_t *mosi_pin;

    if (0) {
    #if defined(MICROPY_HW_SPI0_SCK)
    } else if (self->obj->spi == SPI0) {
        spi_unit = 0;
        nss_pin = MICROPY_HW_SPI0_NSS;
        sck_pin = MICROPY_HW_SPI0_SCK;
        miso_pin = MICROPY_HW_SPI0_MISO;
        mosi_pin = MICROPY_HW_SPI0_MOSI;
    #endif
    #if defined(MICROPY_HW_SPI1_SCK)
    } else if (self->obj->spi == SPI1) {
        spi_unit = 1;
        nss_pin = MICROPY_HW_SPI1_NSS;
        sck_pin = MICROPY_HW_SPI1_SCK;
        miso_pin = MICROPY_HW_SPI1_MISO;
        mosi_pin = MICROPY_HW_SPI1_MOSI;
    #endif
    #if defined(MICROPY_HW_SPI2_SCK)
    } else if (self->obj->spi == SPI2) {
        spi_unit = 2;
        nss_pin = MICROPY_HW_SPI2_NSS;
        sck_pin = MICROPY_HW_SPI2_SCK;
        miso_pin = MICROPY_HW_SPI2_MISO;
        mosi_pin = MICROPY_HW_SPI2_MOSI;
    #endif
    #if defined(MICROPY_HW_SPI3_SCK)
    } else if (self->obj->spi == SPI3) {
        spi_unit = 3;
        nss_pin = MICROPY_HW_SPI3_NSS;
        sck_pin = MICROPY_HW_SPI3_SCK;
        miso_pin = MICROPY_HW_SPI3_MISO;
        mosi_pin = MICROPY_HW_SPI3_MOSI;
    #endif
    } else {
        // SPI does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

	if(bSPI == true){
		uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;
		printf("switch pin fun \n");
		mp_hal_pin_config_alt(nss_pin, mode, AF_FN_SPI, spi_unit);
		mp_hal_pin_config_alt(sck_pin, mode, AF_FN_SPI, spi_unit);
		mp_hal_pin_config_alt(miso_pin, mode, AF_FN_SPI, spi_unit);
		mp_hal_pin_config_alt(mosi_pin, mode, AF_FN_SPI, spi_unit);
	}
	else{
		mp_hal_pin_config(nss_pin, GPIO_MODE_INPUT, 0);
		mp_hal_pin_config(sck_pin, GPIO_MODE_INPUT, 0);
		mp_hal_pin_config(miso_pin, GPIO_MODE_INPUT, 0);
		mp_hal_pin_config(mosi_pin, GPIO_MODE_INPUT, 0);
	}
}

// A transfer of "len" bytes should take len*8*1000/baudrate milliseconds.
// To simplify the calculation we assume the baudrate is never less than 8kHz
// and use that value for the baudrate in the formula, plus a small constant.
#define SPI_TRANSFER_TIMEOUT(len) ((len) + 100)

STATIC void spi_transfer(pyb_spi_obj_t *self, size_t len, const uint8_t *src, uint8_t *dest, uint32_t timeout) {

	spi_t *obj = self->obj;
	int TotalTrans = 0;
	
//	printf("len:%d, src:%x, dest:%x, timeout:%d \n", len, src, dest, timeout);

//	if((src == NULL) || (dest == NULL) || (query_irq() == IRQ_STATE_DISABLED)){
	if(query_irq() == IRQ_STATE_DISABLED){
		int src_len = 0;
		int dest_len = 0;
		
		if (src)
			src_len = len;
		if(dest)
			dest_len = len;
		
		if(self->InitDef->Mode == SPI_MASTER){
			TotalTrans = SPI_MasterBlockWriteRead(obj, (char *)src, src_len, (char *)dest, dest_len, 0xFF);
		}
		else{
			TotalTrans = SPI_SlaveBlockWriteRead(obj, (char *)src, src_len, (char *)dest, dest_len, 0xFF);
		}
	}
	else{
		spi_master_transfer(obj, (char *)src, len, (char *)dest, len, self->InitDef->Bits, (uint32_t)spi_irq_handler_asynch, SPI_EVENT_ALL, DMA_USAGE_NEVER);

		// check event flag for complete
		while(obj->event == 0){
//		while(is_spi_trans_done(obj) == 0){
			mp_hal_delay_ms(timeout / 10);
		}
		
		if(obj->event & SPI_EVENT_INTERNAL_TRANSFER_COMPLETE)
			TotalTrans = len;
	}

	if(TotalTrans == 0){
		mp_hal_raise(HAL_ERROR);
	}
}


STATIC void spi_print(const mp_print_t *print, pyb_spi_obj_t *self, bool legacy) {

    uint spi_num = -1; // default to SPI1

    if (self->obj->spi == SPI0) {
		spi_num = 0;
	}
	else if(self->obj->spi == SPI1){
		spi_num = 1;
	}
	else if(self->obj->spi == SPI2){
		spi_num = 2;
	}
	else if(self->obj->spi == SPI3){
		spi_num = 3;
	}

    mp_printf(print, "SPI(%u", spi_num);
//    if (spi->State != HAL_SPI_STATE_RESET) {
    if (spi_num >= 0) {

        if (self->InitDef->Mode == SPI_MASTER) {
            // compute baudrate
            uint spi_clock;
            spi_clock = SPI_GetBusClock(self->obj->spi);

            if (legacy) {
                mp_printf(print, ", SPI.MASTER");
            }
            mp_printf(print, ", baudrate=%u", spi_clock);
        } else {
            mp_printf(print, ", SPI.SLAVE");
        }
        mp_printf(print, ", polarity=%u, phase=%u, bits=%u", self->InitDef->ClockPolarity, self->InitDef->ClockPhase == SPI_PHASE_1EDGE ? 0 : 1, self->InitDef->Bits);
    }
    mp_print_str(print, ")");
}



STATIC void pyb_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_spi_obj_t *self = self_in;
    spi_print(print, self, true);
}

/// \method init(mode, baudrate=328125, *, polarity=0, phase=0, bits=8, firstbit=SPI.MSB)
///
/// Initialise the SPI bus with the given parameters:
///
///   - `mode` must be either `SPI.MASTER` or `SPI.SLAVE`.
///   - `baudrate` is the SCK clock rate (only sensible for a master).

STATIC mp_obj_t pyb_spi_init_helper(const pyb_spi_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 328125} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_dir,      MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = SPI_DIRECTION_2LINES} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = 8} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = SPI_FIRSTBIT_MSB} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = 0} },
//        { MP_QSTR_prescaler, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
//        { MP_QSTR_nss,      MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = SPI_NSS_SOFT} },
//        { MP_QSTR_ti,       MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
//        { MP_QSTR_crc,      MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    };
    
    
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
  
	if(args[0].u_int == PYB_SPI_MASTER)
		self->InitDef->Mode = SPI_MASTER;
	else
		self->InitDef->Mode = SPI_SLAVE;
	
	self->InitDef->BaudRate = args[1].u_int;
	self->InitDef->ClockPolarity = args[2].u_int;
	self->InitDef->Direction = args[3].u_int;
    
	if(args[4].u_int < 8 || args[4].u_int > 32){
		mp_raise_TypeError("data bits argument incorrect");
	}

	self->InitDef->Bits = args[4].u_int;
	
	if(args[5].u_int == SPI_FIRSTBIT_MSB)
		self->InitDef->FirstBit = SPI_FIRSTBIT_MSB;
	else
		self->InitDef->FirstBit = SPI_FIRSTBIT_LSB;
	
	if(args[6].u_int == SPI_PHASE_1EDGE)
		self->InitDef->ClockPhase = SPI_PHASE_1EDGE;
	else
		self->InitDef->ClockPhase = SPI_PHASE_2EDGE;

	switch_pinfun(self, true);
	SPI_Init(self->obj, self->InitDef);

    return mp_const_none;
}



/// \classmethod \constructor(bus, ...)
///
/// Construct an SPI object on the given bus.  `bus` can be 1 or 2.
/// With no additional parameters, the SPI object is created but not
/// initialised (it has the settings from the last initialisation of
/// the bus, if any).  If extra arguments are given, the bus is initialised.
/// See `init` for parameters of initialisation.
///
/// The physical pins of the SPI busses are:
///
///   - `SPI(0)`: `(NSS, SCK, MISO, MOSI) = (D10, D13, D12, D11) = (PA3, PA2, PA1, PA0)`
///   - `SPI(3)`: `(NSS, SCK, MISO, MOSI) = (D2, D3, A3, A3) = (PC10, PC9, PB9, PB8)`
///
/// At the moment, the NSS pin is not used by the SPI driver and is free
/// for other use.
STATIC mp_obj_t pyb_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // work out SPI bus
    int spi_id = -1;

	spi_id = mp_obj_get_int(args[0]);
	if (spi_id < 0 || spi_id >= MP_ARRAY_SIZE(pyb_spi_obj)
		|| pyb_spi_obj[spi_id].obj == NULL) {
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
			"SPI(%d) doesn't exist", spi_id));
	}
	
    // get SPI object
    const pyb_spi_obj_t *spi_obj = &pyb_spi_obj[spi_id];

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        pyb_spi_init_helper(spi_obj, n_args - 1, args + 1, &kw_args);
    }

    return (mp_obj_t)spi_obj;
}

STATIC mp_obj_t pyb_spi_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_spi_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_spi_init_obj, 1, pyb_spi_init);

/// \method deinit()
/// Turn off the SPI bus.
STATIC mp_obj_t pyb_spi_deinit(mp_obj_t self_in) {
    pyb_spi_obj_t *self = self_in;

	switch_pinfun(self, false);
	SPI_Final(self->obj);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_spi_deinit_obj, pyb_spi_deinit);

STATIC const mp_rom_map_elem_t pyb_spi_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_spi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_spi_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_machine_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_machine_spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_machine_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&mp_machine_spi_write_readinto_obj) },

    // legacy methods
//    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&pyb_spi_send_obj) },
//    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&pyb_spi_recv_obj) },
//    { MP_ROM_QSTR(MP_QSTR_send_recv), MP_ROM_PTR(&pyb_spi_send_recv_obj) },
    // class constants
    /// \constant MASTER - for initialising the bus to master mode
    /// \constant SLAVE - for initialising the bus to slave mode
    /// \constant MSB - set the first bit to MSB
    /// \constant LSB - set the first bit to LSB
    { MP_ROM_QSTR(MP_QSTR_MASTER), MP_ROM_INT(PYB_SPI_MASTER) },
    { MP_ROM_QSTR(MP_QSTR_SLAVE),  MP_ROM_INT(PYB_SPI_SLAVE) },
    { MP_ROM_QSTR(MP_QSTR_MSB),    MP_ROM_INT(SPI_FIRSTBIT_MSB) },
    { MP_ROM_QSTR(MP_QSTR_LSB),    MP_ROM_INT(SPI_FIRSTBIT_LSB) },

    /* TODO
    { MP_ROM_QSTR(MP_QSTR_DIRECTION_2LINES             ((uint32_t)0x00000000)
    { MP_ROM_QSTR(MP_QSTR_DIRECTION_2LINES_RXONLY      SPI_CR1_RXONLY
    { MP_ROM_QSTR(MP_QSTR_DIRECTION_1LINE              SPI_CR1_BIDIMODE
    { MP_ROM_QSTR(MP_QSTR_NSS_SOFT                    SPI_CR1_SSM
    { MP_ROM_QSTR(MP_QSTR_NSS_HARD_INPUT              ((uint32_t)0x00000000)
    { MP_ROM_QSTR(MP_QSTR_NSS_HARD_OUTPUT             ((uint32_t)0x00040000)
    */
};

STATIC MP_DEFINE_CONST_DICT(pyb_spi_locals_dict, pyb_spi_locals_dict_table);

STATIC void spi_transfer_pyb(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    pyb_spi_obj_t *self = (pyb_spi_obj_t*)self_in;
    spi_transfer(self, len, src, dest, SPI_TRANSFER_TIMEOUT(len));
}

STATIC const mp_machine_spi_p_t pyb_spi_p = {
    .transfer = spi_transfer_pyb,
};

const mp_obj_type_t pyb_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = pyb_spi_print,
    .make_new = pyb_spi_make_new,
    .protocol = &pyb_spi_p,
    .locals_dict = (mp_obj_dict_t*)&pyb_spi_locals_dict,
};
 
#endif
