/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2018 Damien P. George
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

#include "rng.h"
#include "NuMicro.h"

#if MICROPY_HW_ENABLE_RNG

#define RNG_TIMEOUT_MS (10)

uint32_t rng_get(void) {
    uint32_t    au32PrngData[8];

    // Enable the RNG peripheral if it's not already enabled
	PRNG_Start(CRPT);
    // Wait for a new random number to be ready, takes on the order of 10us
    uint32_t start = mp_hal_ticks_ms();
    while (CRPT->PRNG_CTL & CRPT_PRNG_CTL_START_Msk) {
        if (mp_hal_ticks_ms() - start >= RNG_TIMEOUT_MS) {
            return 0;
        }
    }

    // Get and return the new random number
    PRNG_Read(CRPT, au32PrngData);
    return au32PrngData[0];
}


// Return a 30-bit hardware generated random number.
STATIC mp_obj_t pyb_rng_get(void) {
    return mp_obj_new_int(rng_get() >> 2);
}
MP_DEFINE_CONST_FUN_OBJ_0(pyb_rng_get_obj, pyb_rng_get);

#endif
