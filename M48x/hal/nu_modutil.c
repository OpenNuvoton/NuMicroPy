/**************************************************************************//**
 * @file     nu_modutil.c
 * @version  V0.01
 * @brief    module utility
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "nu_modutil.h"

const struct nu_modinit_s *get_modinit(uint32_t modname, const struct nu_modinit_s *modprop_tab)
{
	if(modprop_tab == NULL)
		return NULL;

    const struct nu_modinit_s *modprop_ind = modprop_tab;
    while (modprop_ind->modname != 0) {
        if ((int) modname == modprop_ind->modname) {
            return modprop_ind;
        }
        else {
            modprop_ind ++;
        }
    }
    
    return NULL;
}
