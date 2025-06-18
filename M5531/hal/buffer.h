/**************************************************************************//**
 * @file     buffer.h
 * @version  V0.10
 * @brief   dma buffer function
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef NU_BUFFER_H
#define NU_BUFFER_H

#include <stddef.h>

/** Generic buffer structure
 */
typedef struct buffer_s {
    void    *buffer; /**< the pointer to a buffer */
    size_t   length; /**< the buffer length */
    size_t   pos;    /**< actual buffer position */
    uint8_t  width;  /**< The buffer unit width (8, 16, 32, 64), used for proper *buffer casting */
} buffer_t;

#endif
