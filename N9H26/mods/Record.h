/***************************************************************************//**
 * @file     Record.h
 * @brief    Media record header file
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __RECORD_H__
#define __RECORD_H__

#include <stdio.h>

#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"

#include "NVTMedia.h"

#define DEF_REC_RESERVE_STORE_SPACE	(16*1024*1024)
#define DEF_RECORD_FILE_MGR_TYPE			eFILEMGR_TYPE_MP4
#define DEF_REC_FOLDER_PATH "/sd/DCIM/"
#define DEF_NM_MEDIA_FILE_TYPE				eNM_MEDIA_MP4

#if MICROPY_OPENMV
#include "imlib.h"

#else

typedef struct image {
    int w;
    int h;
    int bpp;
    union {
        uint8_t *pixels;
        uint8_t *data;
    };
} image_t;

#endif

typedef enum {
    ePLANAR_PIXFORMAT_INVALID = 0,
    ePLANAR_PIXFORMAT_YUV422,	//for JPEG encode
    ePLANAR_PIXFORMAT_YUV420,	//for JPEG encode
    ePLANAR_PIXFORMAT_YUV420MB,	//for H264 encode source
} E_PLANAR_PIXFORMAT;

typedef bool (*PFN_VIN_FILL_CB)(image_t *psImg, uint64_t *pu64ImgTime);
typedef bool (*PFN_AIN_FILL_CB)(uint32_t u32ReadSamples, uint8_t *pu8DataBuf, uint64_t *pu64FrameTime);
typedef void (*PFN_VOUT_FLUSH_CB)(mp_obj_t vout_obj, uint8_t *pu8FrameBufAddr);

typedef struct{
	uint32_t u32Width;
	uint32_t u32Height;
	E_PLANAR_PIXFORMAT ePixelFormat;
	PFN_VIN_FILL_CB pfn_vin_packet_img_fill_cb;
	PFN_VIN_FILL_CB pfn_vin_planar_img_fill_cb;
}record_vin_drv_t;

typedef struct{
	uint32_t u32SampleRate;
	uint32_t u32Channel;
	PFN_AIN_FILL_CB pfn_ain_fill_cb;
}record_ain_drv_t;

//It is for preview output
typedef struct{
	mp_obj_t vout_obj;
	PFN_VOUT_FLUSH_CB pfn_vout_flush_cb;
}record_vout_drv_t;

void Record_RegisterVinDrv(record_vin_drv_t *psVinDrv);
void Record_RegisterAinDrv(record_ain_drv_t *psAinDrv);
void Record_RegisterVoutDrv(record_vout_drv_t *psVoutDrv);

int Record_Init(
	bool bEnablePreview
);

void Record_DeInit(void);

int Record_Start(
	const char *szRecFolderPath,
	uint32_t u32FileDuration
);

int Record_Stop(void);

#endif


