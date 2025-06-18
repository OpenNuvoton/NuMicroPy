/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2023 Damien P. George
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

#include "py/obj.h"
#include "shared/timeutils/timeutils.h"
#include "rtc.h"

// This file is never compiled standalone, it's included directly from
// extmod/modtime.c via MICROPY_PY_TIME_INCLUDEFILE.

static bool bRTCEnabled = false;

static void EnableRTCClock(void)
{
    CLK_EnableModuleClock(RTC0_MODULE);
    /* Set LXT as RTC clock source */
    RTC_SetClockSource(RTC_CLOCK_SOURCE_LXT);

    RTC_Open(NULL);
    bRTCEnabled = true;
}

// Return the localtime as an 8-tuple.
static mp_obj_t mp_time_localtime_get(void)
{
    // get current date and time
    // note: need to call get time then get date to correctly access the registers
    S_RTC_TIME_DATA_T sRTCTimeData;

    if(!bRTCEnabled)
        EnableRTCClock();

    RTC_GetDateAndTime(&sRTCTimeData);
    mp_obj_t tuple[8];

    tuple[0] = mp_obj_new_int(sRTCTimeData.u32Year);
    tuple[1] = mp_obj_new_int(sRTCTimeData.u32Month);
    tuple[2] =    mp_obj_new_int(sRTCTimeData.u32Day);
    if(sRTCTimeData.u32DayOfWeek == RTC_SUNDAY)
        tuple[3] = mp_obj_new_int(7);
    else
        tuple[3] = mp_obj_new_int(sRTCTimeData.u32DayOfWeek);
    tuple[4] = mp_obj_new_int(sRTCTimeData.u32Hour);
    tuple[5] = mp_obj_new_int(sRTCTimeData.u32Minute);
    tuple[6] = mp_obj_new_int(sRTCTimeData.u32Second);
    tuple[7] = mp_obj_new_int(0);

    return mp_obj_new_tuple(8, tuple);
}



