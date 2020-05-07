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

#include <string.h>

#include "py/mperrno.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs_fat.h"

#include "mpconfigboard_common.h"

#include "hal/StorIF.h"
#include "pybflash.h"

static void *s_pvStorFlashRes;


void flash_init(void)
{
	g_STORIF_sFlash.pfnStorInit(0, &s_pvStorFlashRes);
}

static mp_uint_t flash_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks) {

    if(g_STORIF_sFlash.pfnReadSector(dest, block_num, num_blocks, s_pvStorFlashRes) < 0){
        return MP_EIO;
    }

    return 0;
}

static mp_uint_t flash_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks) {

    if(g_STORIF_sFlash.pfnWriteSector((uint8_t *) src, block_num, num_blocks, s_pvStorFlashRes) < 0){
        return MP_EIO;
    }

    return 0;
}

/******************************************************************************/
// MicroPython bindings
//
// Expose the internal flash as an object with the block protocol.

// there is a singleton internal flash object
STATIC const mp_obj_base_t pyb_flash_obj = {&pyb_flash_type};

STATIC mp_obj_t pyb_flash_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // return singleton object
    return (mp_obj_t)&pyb_flash_obj;
}

STATIC mp_obj_t pyb_flash_readblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
	S_STORIF_INFO sFlashInfo;
	
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);

	g_STORIF_sFlash.pfnGetInfo(&sFlashInfo, &s_pvStorFlashRes);
    mp_uint_t ret = flash_read_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / sFlashInfo.u32SectorSize);
    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_flash_readblocks_obj, pyb_flash_readblocks);

STATIC mp_obj_t pyb_flash_writeblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
	S_STORIF_INFO sFlashInfo;

    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);

	g_STORIF_sFlash.pfnGetInfo(&sFlashInfo, &s_pvStorFlashRes);
    mp_uint_t ret = flash_write_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / sFlashInfo.u32SectorSize);
    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_flash_writeblocks_obj, pyb_flash_writeblocks);

STATIC mp_obj_t pyb_flash_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mp_int_t cmd = mp_obj_get_int(cmd_in);

    switch (cmd) {
        case BP_IOCTL_INIT:
        {
			return MP_OBJ_NEW_SMALL_INT(0);
		}
        case BP_IOCTL_DEINIT:
        {
			return MP_OBJ_NEW_SMALL_INT(0); // TODO properly
		}
        case BP_IOCTL_SYNC:
        {
			return MP_OBJ_NEW_SMALL_INT(0);
		}
        case BP_IOCTL_SEC_COUNT:
        {
			S_STORIF_INFO sFlashInfo;
			g_STORIF_sFlash.pfnGetInfo(&sFlashInfo, &s_pvStorFlashRes);
			return MP_OBJ_NEW_SMALL_INT(sFlashInfo.u32TotalSector);
		}
        case BP_IOCTL_SEC_SIZE:
        {
			S_STORIF_INFO sFlashInfo;
			g_STORIF_sFlash.pfnGetInfo(&sFlashInfo, &s_pvStorFlashRes);
			return MP_OBJ_NEW_SMALL_INT(sFlashInfo.u32SectorSize);
		}
        default: return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_flash_ioctl_obj, pyb_flash_ioctl);


STATIC const mp_rom_map_elem_t pyb_flash_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&pyb_flash_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&pyb_flash_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&pyb_flash_ioctl_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_flash_locals_dict, pyb_flash_locals_dict_table);

const mp_obj_type_t pyb_flash_type = {
    { &mp_type_type },
    .name = MP_QSTR_Flash,
    .make_new = pyb_flash_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_flash_locals_dict,
};

void pyb_flash_init_vfs(fs_user_mount_t *vfs) {
    vfs->base.type = &mp_fat_vfs_type;
    vfs->flags |= FSUSER_NATIVE | FSUSER_HAVE_IOCTL;
    vfs->fatfs.drv = vfs;
    vfs->fatfs.part = 1; // flash filesystem lives on first partition
    vfs->readblocks[0] = (mp_obj_t)&pyb_flash_readblocks_obj;
    vfs->readblocks[1] = (mp_obj_t)&pyb_flash_obj;
    vfs->readblocks[2] = (mp_obj_t)flash_read_blocks; // native version
    vfs->writeblocks[0] = (mp_obj_t)&pyb_flash_writeblocks_obj;
    vfs->writeblocks[1] = (mp_obj_t)&pyb_flash_obj;
    vfs->writeblocks[2] = (mp_obj_t)flash_write_blocks; // native version
    vfs->u.ioctl[0] = (mp_obj_t)&pyb_flash_ioctl_obj;
    vfs->u.ioctl[1] = (mp_obj_t)&pyb_flash_obj;
}


