/**************************************************************************//**
 * @file     nu_miscutil.h
 * @version  V0.01
 * @brief    misc utility
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef NU_MISC_UTIL_H
#define NU_MISC_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define NU_MAX(a,b) ((a)>(b)?(a):(b))
#define NU_MIN(a,b) ((a)<(b)?(a):(b))
#define NU_CLAMP(x, min, max)   NU_MIN(NU_MAX((x), (min)), (max))
#define NU_ALIGN_DOWN(X, ALIGN)     ((X) & ~((ALIGN) - 1))
#define NU_ALIGN_UP(X, ALIGN)       (((X) + (ALIGN) - 1) & ~((ALIGN) - 1))

void nu_nop(uint32_t n);
    
    
#ifdef __cplusplus
}
#endif

#endif
