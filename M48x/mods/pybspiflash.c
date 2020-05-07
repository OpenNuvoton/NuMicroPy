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
#include <stdlib.h>
#include <string.h>

#include "py/mperrno.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs_fat.h"

#include "mpconfigboard_common.h"

#include "hal/StorIF.h"
#include "pybspiflash.h"

#define DEF_FAT_SECTOR_SIZE 512

static void *s_pvStorSPIFlashRes = NULL;

void spiflash_init(void){
	
	//TODO:set multi-function pins according board configure header file
	/* Enable SPI0 peripheral clock */
    CLK_EnableModuleClock(QSPI0_MODULE);


    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC5MFP_Msk | SYS_GPC_MFPL_PC4MFP_Msk | SYS_GPC_MFPL_PC3MFP_Msk | SYS_GPC_MFPL_PC2MFP_Msk | SYS_GPC_MFPL_PC1MFP_Msk | SYS_GPC_MFPL_PC0MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC5MFP_QSPI0_MISO1 | SYS_GPC_MFPL_PC4MFP_QSPI0_MOSI1 | SYS_GPC_MFPL_PC3MFP_QSPI0_SS | SYS_GPC_MFPL_PC2MFP_QSPI0_CLK | SYS_GPC_MFPL_PC1MFP_QSPI0_MISO0 | SYS_GPC_MFPL_PC0MFP_QSPI0_MOSI0);

    /* Enable QSPI0 clock pin (PC2) schmitt trigger */
    PC->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;

	//SPI flash D2D3 pin switch to normalmode
    SYS->GPC_MFPL =  SYS->GPC_MFPL & ~(SYS_GPC_MFPL_PC4MFP_Msk | SYS_GPC_MFPL_PC5MFP_Msk);
    GPIO_SetMode(PC, BIT4, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT5, GPIO_MODE_OUTPUT);
    PC4 = 1;
    PC5 = 1;	

	g_STORIF_sSPIFlash.pfnStorInit(0, &s_pvStorSPIFlashRes);
}



static mp_uint_t spiflash_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks) {

	uint32_t u32Addr;	
    S_STORIF_INFO sStorInfo;
    uint32_t u32Size;
	uint32_t u32SectorAlignAddr;
	uint32_t u32Offset;
	uint8_t *pu8TempBuf = NULL;
	int32_t i32EachReadLen;
    uint8_t *pu8Buffer = dest;
    
	g_STORIF_sSPIFlash.pfnGetInfo(&sStorInfo, s_pvStorSPIFlashRes);

	u32Addr = block_num * DEF_FAT_SECTOR_SIZE;
	u32Size = num_blocks * DEF_FAT_SECTOR_SIZE;

	u32SectorAlignAddr = u32Addr & (~(sStorInfo.u32SectorSize - 1));

	/* read none sector-aligned data */
	if(u32SectorAlignAddr != u32Addr){
		/* Get the sector offset*/
		u32Offset = (u32Addr & (sStorInfo.u32SectorSize - 1));

		pu8TempBuf = malloc(sStorInfo.u32SectorSize);
		if(pu8TempBuf == NULL)
			return MP_EIO;

		g_STORIF_sSPIFlash.pfnReadSector(pu8TempBuf, u32SectorAlignAddr / sStorInfo.u32SectorSize, 1, s_pvStorSPIFlashRes);
		i32EachReadLen = sStorInfo.u32SectorSize - u32Offset;

		if(i32EachReadLen > u32Size)
			i32EachReadLen = u32Size;
		
		memcpy(pu8Buffer, pu8TempBuf + u32Offset, i32EachReadLen);

		u32Addr += i32EachReadLen;
		pu8Buffer += i32EachReadLen;
		u32Size -= i32EachReadLen;
	} 

	/*read sector aligned data */
	i32EachReadLen = u32Size & (~(sStorInfo.u32SectorSize - 1));

	if(i32EachReadLen > 0){
		g_STORIF_sSPIFlash.pfnReadSector(pu8Buffer, u32Addr / sStorInfo.u32SectorSize, i32EachReadLen / sStorInfo.u32SectorSize, s_pvStorSPIFlashRes);
		
		u32Addr += i32EachReadLen;
		pu8Buffer += i32EachReadLen;
		u32Size -= i32EachReadLen;
	}

	/*read remain data */
	if(u32Size > 0){
		if(pu8TempBuf == NULL){
			pu8TempBuf = malloc(sStorInfo.u32SectorSize);
			if(pu8TempBuf == NULL)
				return MP_EIO;
		}

		g_STORIF_sSPIFlash.pfnReadSector(pu8TempBuf, u32Addr / sStorInfo.u32SectorSize, 1, s_pvStorSPIFlashRes);
		memcpy(pu8Buffer, pu8TempBuf, u32Size);
	}
	
	if(pu8TempBuf)
		free(pu8TempBuf);

    return 0;
}

static mp_uint_t spiflash_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks) {
	uint32_t u32Addr;	
    S_STORIF_INFO sStorInfo;
    uint32_t u32Size;
	uint32_t u32SectorAlignAddr;
	uint32_t u32Offset;
	uint8_t *pu8TempBuf = NULL;
	int32_t i32EachWriteLen;
    uint8_t *pu8Buffer = (uint8_t *)src;

	g_STORIF_sSPIFlash.pfnGetInfo(&sStorInfo, s_pvStorSPIFlashRes);

	u32Addr = block_num * DEF_FAT_SECTOR_SIZE;
	u32Size = num_blocks * DEF_FAT_SECTOR_SIZE;

	u32SectorAlignAddr = u32Addr & (~(sStorInfo.u32SectorSize - 1));

	/* write none sector-aligned data */
	if(u32SectorAlignAddr != u32Addr){
		/* Get the sector offset*/
		u32Offset = (u32Addr & (sStorInfo.u32SectorSize - 1));

		pu8TempBuf = malloc(sStorInfo.u32SectorSize);
		if(pu8TempBuf == NULL)
			return MP_EIO;

		g_STORIF_sSPIFlash.pfnReadSector(pu8TempBuf, u32SectorAlignAddr / sStorInfo.u32SectorSize, 1, s_pvStorSPIFlashRes);
		i32EachWriteLen = sStorInfo.u32SectorSize - u32Offset;

		if(i32EachWriteLen > u32Size)
			i32EachWriteLen = u32Size;

		memcpy(pu8TempBuf + u32Offset, pu8Buffer, i32EachWriteLen);
		g_STORIF_sSPIFlash.pfnWriteSector(pu8TempBuf, u32SectorAlignAddr / sStorInfo.u32SectorSize, 1, s_pvStorSPIFlashRes);
		
		u32Addr += i32EachWriteLen;
		pu8Buffer += i32EachWriteLen;
		u32Size -= i32EachWriteLen;
	} 

	/*write sector aligned data */
	i32EachWriteLen = u32Size & (~(sStorInfo.u32SectorSize - 1));

	if(i32EachWriteLen > 0){
		g_STORIF_sSPIFlash.pfnWriteSector(pu8Buffer, u32Addr / sStorInfo.u32SectorSize, i32EachWriteLen / sStorInfo.u32SectorSize, s_pvStorSPIFlashRes);
		
		u32Addr += i32EachWriteLen;
		pu8Buffer += i32EachWriteLen;
		u32Size -= i32EachWriteLen;
	}

	/*write remain data */
	if(u32Size > 0){
		if(pu8TempBuf == NULL){
			pu8TempBuf = malloc(sStorInfo.u32SectorSize);
			if(pu8TempBuf == NULL)
				return MP_EIO;
		}

		g_STORIF_sSPIFlash.pfnReadSector(pu8TempBuf, u32Addr / sStorInfo.u32SectorSize, 1, s_pvStorSPIFlashRes);
		memcpy(pu8TempBuf, pu8Buffer, u32Size);
		g_STORIF_sSPIFlash.pfnWriteSector(pu8TempBuf, u32Addr / sStorInfo.u32SectorSize, 1, s_pvStorSPIFlashRes);
	}
	
	if(pu8TempBuf)
		free(pu8TempBuf);

    return 0;
}

/******************************************************************************/
// MicroPython bindings
//
// Expose the external flash as an object with the block protocol.

// there is a singleton internal flash object
STATIC const mp_obj_base_t pyb_spiflash_obj = {&pyb_spiflash_type};

STATIC mp_obj_t pyb_spiflash_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // return singleton object
    return (mp_obj_t)&pyb_spiflash_obj;
}

STATIC mp_obj_t pyb_spiflash_readblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
	mp_buffer_info_t bufinfo;

	mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);

    mp_uint_t ret = spiflash_read_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / DEF_FAT_SECTOR_SIZE);
	return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_spiflash_readblocks_obj, pyb_spiflash_readblocks);

STATIC mp_obj_t pyb_spiflash_writeblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;

    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);

    mp_uint_t ret = spiflash_write_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / DEF_FAT_SECTOR_SIZE);
    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_spiflash_writeblocks_obj, pyb_spiflash_writeblocks);


STATIC mp_obj_t pyb_spiflash_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in) {
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
			S_STORIF_INFO sSPIFlashInfo;
			g_STORIF_sSPIFlash.pfnGetInfo(&sSPIFlashInfo, s_pvStorSPIFlashRes);
			return MP_OBJ_NEW_SMALL_INT((sSPIFlashInfo.u32DiskSize * 1024) / DEF_FAT_SECTOR_SIZE);		//512:FAT sector size
		}
        case BP_IOCTL_SEC_SIZE:
        {
			//Using FAT sector size, not SPI flash sector size
			return MP_OBJ_NEW_SMALL_INT(DEF_FAT_SECTOR_SIZE);
		}
        default: return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_spiflash_ioctl_obj, pyb_spiflash_ioctl);

STATIC const mp_rom_map_elem_t pyb_spiflash_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&pyb_spiflash_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&pyb_spiflash_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&pyb_spiflash_ioctl_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_spiflash_locals_dict, pyb_spiflash_locals_dict_table);

const mp_obj_type_t pyb_spiflash_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPIFlash,
    .make_new = pyb_spiflash_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_spiflash_locals_dict,
};

void pyb_spiflash_init_vfs(fs_user_mount_t *vfs) {
    vfs->base.type = &mp_fat_vfs_type;
    vfs->flags |= FSUSER_NATIVE | FSUSER_HAVE_IOCTL;
    vfs->fatfs.drv = vfs;
    vfs->fatfs.part = 1; // flash filesystem lives on first partition
    vfs->readblocks[0] = (mp_obj_t)&pyb_spiflash_readblocks_obj;
    vfs->readblocks[1] = (mp_obj_t)&pyb_spiflash_obj;
    vfs->readblocks[2] = (mp_obj_t)spiflash_read_blocks; // native version
    vfs->writeblocks[0] = (mp_obj_t)&pyb_spiflash_writeblocks_obj;
    vfs->writeblocks[1] = (mp_obj_t)&pyb_spiflash_obj;
    vfs->writeblocks[2] = (mp_obj_t)spiflash_write_blocks; // native version
    vfs->u.ioctl[0] = (mp_obj_t)&pyb_spiflash_ioctl_obj;
    vfs->u.ioctl[1] = (mp_obj_t)&pyb_spiflash_obj;
}
