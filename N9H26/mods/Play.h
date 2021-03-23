/***************************************************************************//**
 * @file     Play.h
 * @brief    Media play header file
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef __PLAY_H__
#define __PLAY_H__

#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"

typedef void (*PFN_AOUT_FLUSH_CB)(uint8_t *pu8DataBuf, uint32_t *pu8SendLen);
typedef void (*PFN_VOUT_FLUSH_CB)(mp_obj_t vout_obj, uint8_t *pu8FrameBufAddr);

//Video information
typedef struct{
	uint32_t u32ImgWidth;
	uint32_t u32ImgHeight;	
}play_video_info_t;

//Audio information
typedef struct{
	uint32_t u32SampleRate;
	uint32_t u32Channel;	
}play_audio_info_t;

//It is for video output driver
typedef struct{
	mp_obj_t vout_obj;
	uint32_t u32OutputWidth;
	uint32_t u32OutputHeight;
	PFN_VOUT_FLUSH_CB pfn_vout_flush_cb;
}play_vout_drv_t;

//It is for audio output driver
typedef struct{
	PFN_AOUT_FLUSH_CB pfn_aout_flush_cb;
}play_aout_drv_t;

void Play_RegisterAoutDrv(play_aout_drv_t *psAoutDrv);
void Play_RegisterVoutDrv(play_vout_drv_t *psVoutDrv);


#endif
