/***************************************************************************//**
 * @file     Play.c
 * @brief    Media play source file
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Play.h"

void Play_RegisterVoutDrv(play_vout_drv_t *psVoutDrv)
{
	printf("DDDDDD Play_RegisterVoutDrv psVoutDrv->pfn_vout_flush_cb %p \n", psVoutDrv->pfn_vout_flush_cb);
	printf("DDDDDD Play_RegisterVoutDrv psVoutDrv->vout_obj %p \n", psVoutDrv->vout_obj);
}


void Play_RegisterAoutDrv(play_aout_drv_t *psAoutDrv)
{

}
