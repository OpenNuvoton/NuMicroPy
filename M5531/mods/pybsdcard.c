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

#include "pybsdcard.h"
#include "hal/StorIF.h"

#if MICROPY_HW_HAS_SDCARD

void sdcard_init(void)
{
    g_STORIF_sSDCard.pfnStorInit(0, &g_STORIF_sSDCard.pvStorPriv);
}

bool sdcard_is_present(void)
{
    return g_STORIF_sSDCard.pfnDetect(g_STORIF_sSDCard.pvStorPriv);
}

mp_int_t sdcard_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks)
{
    mp_int_t read_blocks = 0;

    // check that SD card is initialised
    if (g_STORIF_sSDCard.pfnDetect(g_STORIF_sSDCard.pvStorPriv) == false) {
        return -(MP_EIO);
    }

    if((read_blocks = g_STORIF_sSDCard.pfnReadSector(dest, block_num, num_blocks, g_STORIF_sSDCard.pvStorPriv)) < 0) {
        return -(MP_EIO);
    }

    return read_blocks;
}

mp_int_t sdcard_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks)
{
    mp_int_t write_blocks = 0;

    // check that SD card is initialised
    if (g_STORIF_sSDCard.pfnDetect(g_STORIF_sSDCard.pvStorPriv) == false) {
        return -(MP_EIO);
    }

    if((write_blocks = g_STORIF_sSDCard.pfnWriteSector((uint8_t *)src, block_num, num_blocks, g_STORIF_sSDCard.pvStorPriv)) < 0) {
        return -(MP_EIO);
    }

    return write_blocks;
}


static mp_int_t vfs_sdcard_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks)
{
    mp_int_t read_blocks;

    read_blocks = sdcard_read_blocks(dest, block_num, num_blocks);

    if(read_blocks < 0)
        return read_blocks;

    return 0;
}

static mp_int_t vfs_sdcard_write_blocks(uint8_t *src, uint32_t block_num, uint32_t num_blocks)
{
    mp_int_t write_blocks;

    write_blocks = sdcard_write_blocks(src, block_num, num_blocks);

    if(write_blocks < 0)
        return write_blocks;

    return 0;
}

/******************************************************************************/
// MicroPython bindings
//
// Expose the SD card as an object with the block protocol.

// there is a singleton SDCard object
static const mp_obj_base_t pyb_sdcard_obj = {&pyb_sdcard_type};

static mp_obj_t pyb_sdcard_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // return singleton object
    return (mp_obj_t)&pyb_sdcard_obj;
}

static mp_obj_t sd_present(mp_obj_t self)
{
    return mp_obj_new_bool(g_STORIF_sSDCard.pfnDetect(g_STORIF_sSDCard.pvStorPriv));
}
static MP_DEFINE_CONST_FUN_OBJ_1(sd_present_obj, sd_present);

static mp_obj_t sd_info(mp_obj_t self)
{
    if (g_STORIF_sSDCard.pfnDetect(g_STORIF_sSDCard.pvStorPriv)  == false) {
        return mp_const_none;
    }
    S_STORIF_INFO cardinfo;
    g_STORIF_sSDCard.pfnGetInfo(&cardinfo, g_STORIF_sSDCard.pvStorPriv);

    mp_obj_t tuple[3] = {
        mp_obj_new_int_from_ull((uint64_t)cardinfo.u32DiskSize * 1024),
        mp_obj_new_int_from_uint(cardinfo.u32SectorSize),
        mp_obj_new_int(cardinfo.u32SubType),
    };
    return mp_obj_new_tuple(3, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_1(sd_info_obj, sd_info);

static mp_obj_t pyb_sdcard_readblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf)
{
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);
    mp_uint_t ret = g_STORIF_sSDCard.pfnReadSector(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDH_BLOCK_SIZE, g_STORIF_sSDCard.pvStorPriv);
    return MP_OBJ_NEW_SMALL_INT(ret);
}
static MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_readblocks_obj, pyb_sdcard_readblocks);

static mp_obj_t pyb_sdcard_writeblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf)
{
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);
    mp_uint_t ret = g_STORIF_sSDCard.pfnWriteSector(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDH_BLOCK_SIZE, g_STORIF_sSDCard.pvStorPriv);
    return MP_OBJ_NEW_SMALL_INT(ret);
}
static MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_writeblocks_obj, pyb_sdcard_writeblocks);

static mp_obj_t pyb_sdcard_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in)
{
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    switch (cmd) {
    case MP_BLOCKDEV_IOCTL_INIT:
        return MP_OBJ_NEW_SMALL_INT(0); // success

    case MP_BLOCKDEV_IOCTL_DEINIT:
        return MP_OBJ_NEW_SMALL_INT(0); // success
    case MP_BLOCKDEV_IOCTL_SYNC:
        // nothing to do
        return MP_OBJ_NEW_SMALL_INT(0); // success

    case MP_BLOCKDEV_IOCTL_BLOCK_COUNT: {
        S_STORIF_INFO cardinfo;
        g_STORIF_sSDCard.pfnGetInfo(&cardinfo, g_STORIF_sSDCard.pvStorPriv);

        return MP_OBJ_NEW_SMALL_INT(cardinfo.u32TotalSector);
    }
    case MP_BLOCKDEV_IOCTL_BLOCK_SIZE:
        return MP_OBJ_NEW_SMALL_INT(SDH_BLOCK_SIZE);

    default: // unknown command
        return MP_OBJ_NEW_SMALL_INT(-1); // error
    }
}
static MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_ioctl_obj, pyb_sdcard_ioctl);


static const mp_rom_map_elem_t pyb_sdcard_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_present), MP_ROM_PTR(&sd_present_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&sd_info_obj) },

    // block device protocol
    { MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&pyb_sdcard_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&pyb_sdcard_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&pyb_sdcard_ioctl_obj) },
};

static MP_DEFINE_CONST_DICT(pyb_sdcard_locals_dict, pyb_sdcard_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pyb_sdcard_type,
    MP_QSTR_SDCard,
    MP_TYPE_FLAG_NONE,
    make_new, pyb_sdcard_make_new,
    locals_dict, &pyb_sdcard_locals_dict
);

void sdcard_init_vfs(fs_user_mount_t *vfs, int part)
{
    vfs->base.type = &mp_fat_vfs_type;
    vfs->blockdev.flags |= MP_BLOCKDEV_FLAG_NATIVE | MP_BLOCKDEV_FLAG_HAVE_IOCTL;
    vfs->fatfs.drv = vfs;
    vfs->fatfs.part = part;
    vfs->blockdev.readblocks[0] = (mp_obj_t)&pyb_sdcard_readblocks_obj;
    vfs->blockdev.readblocks[1] = (mp_obj_t)&pyb_sdcard_obj;
    vfs->blockdev.readblocks[2] = (mp_obj_t)vfs_sdcard_read_blocks; // native version
    vfs->blockdev.writeblocks[0] = (mp_obj_t)&pyb_sdcard_writeblocks_obj;
    vfs->blockdev.writeblocks[1] = (mp_obj_t)&pyb_sdcard_obj;
    vfs->blockdev.writeblocks[2] = (mp_obj_t)vfs_sdcard_write_blocks; // native version
    vfs->blockdev.u.ioctl[0] = (mp_obj_t)&pyb_sdcard_ioctl_obj;
    vfs->blockdev.u.ioctl[1] = (mp_obj_t)&pyb_sdcard_obj;
}

#endif
