/**************************************************************************//**
*
* @copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of Nuvoton Technology Corp. nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Change Logs:
* Date            Author           Notes
* 2021-4-13        CHChen        First version
*
******************************************************************************/


#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "pybpin.h"

#include "NuMicro.h"

/*---------------------------------------------------------------------------------------------------------*/
/*  Set Wake up source by GPIO GPA0 Wake-up pin                    */
/*---------------------------------------------------------------------------------------------------------*/
static void WakeUpPinFunction()
{

	const pin_obj_t *wakeup_pin;
	
	wakeup_pin = MICROPY_HW_POWER_STANDBY_WAKEUP;

	GPIO_SetPullCtl(wakeup_pin->gpio, wakeup_pin->pin_mask, GPIO_PUSEL_PULL_UP);

    /* Configure wakeup pin as Input mode */
	mp_hal_pin_config(wakeup_pin, GPIO_MODE_INPUT, 0);

    // GPIO SPD GPA0 Power-down Wake-up Pin Select and Debounce Disable
    CLK_EnableSPDWKPin(wakeup_pin->port, wakeup_pin->pin, CLK_SPDWKPIN_RISING, CLK_SPDWKPIN_DEBOUNCEDIS);

}


void PowerManager_EnterStandbyPowerDown(void)
{
#if MICROPY_PY_THREAD
	vTaskSuspendAll();
#endif
	__disable_irq();
	
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set IO State and all IPs clock disable for power consumption */
    CLK->APBCLK1 = 0x00000000;
    CLK->APBCLK0 = 0x00000000;

    /* Clear all wake-up flag */
    CLK->PMUSTS |= CLK_PMUSTS_CLRWK_Msk;

   /* Select Power-down mode */
    CLK_SetPowerDownMode(CLK_PMUCTL_PDMSEL_SPD0);

	WakeUpPinFunction();
	
    /* Enable RTC wake-up */
    CLK_ENABLE_RTCWK();
	
    /* Enter to Power-down mode */
    CLK_PowerDown();

    /* Wait for Power-down mode wake-up reset happen */
    while(1);
}

void PowerManager_EnterNormalPowerDown(void)
{
	USBD_SET_SE0();

#if MICROPY_PY_THREAD
	vTaskSuspendAll();
#endif

	__disable_irq();

    /* Unlock protected registers */
    SYS_UnlockReg();

   /* Select Power-down mode */
    CLK_SetPowerDownMode(CLK_PMUCTL_PDMSEL_PD);

    /* Enter to Power-down mode */
    CLK_PowerDown();

	__enable_irq();

#if MICROPY_PY_THREAD
	xTaskResumeAll();
#endif

	USBD_CLR_SE0();
}
