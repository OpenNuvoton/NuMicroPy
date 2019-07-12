/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Original template from ST Cube library.  See below for header.
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
 
/**
  ******************************************************************************
  * @file    Templates/hal/m26x_ISR.c
  * @author  MCD Application Team
  * @version V1.0.1
  * @date    26-February-2014
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
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
  ******************************************************************************
  */
  
#include <stdio.h>

#include "NuMicro.h"

#include "mods/pybpin.h"
#include "mods/pybirq.h"
#include "hal/pin_int.h"
#include "hal/M26x_USBD.h"
#

/**
 * @brief       GPIO PA IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PA default IRQ, declared in startup_M261.s.
 */
void GPA_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPA_IRQn);

	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PA, 1 << i)){
			GPIO_CLR_INT_FLAG(PA, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	
	IRQ_EXIT(GPA_IRQn);
}


/**
 * @brief       GPIO PB IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PB default IRQ, declared in startup_M261.s.
 */
void GPB_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPB_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PB, 1 << i)){
			GPIO_CLR_INT_FLAG(PB, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPB_IRQn);
}

/**
 * @brief       GPIO PC IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PC default IRQ, declared in startup_M261.s.
 */
void GPC_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPC_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PC, 1 << i)){
			GPIO_CLR_INT_FLAG(PC, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPC_IRQn);
}

/**
 * @brief       GPIO PD IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PD default IRQ, declared in startup_M261.s.
 */
void GPD_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPD_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PD, 1 << i)){
			GPIO_CLR_INT_FLAG(PD, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPD_IRQn);
}

/**
 * @brief       GPIO PE IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PE default IRQ, declared in startup_M261.s.
 */
void GPE_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPE_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PE, 1 << i)){
			GPIO_CLR_INT_FLAG(PE, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPE_IRQn);
}

/**
 * @brief       GPIO PF IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PF default IRQ, declared in startup_M261.s.
 */
void GPF_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPF_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PF, 1 << i)){
			GPIO_CLR_INT_FLAG(PF, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPF_IRQn);
}

/**
 * @brief       GPIO PG IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PG default IRQ, declared in startup_M261.s.
 */
void GPG_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPG_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PG, 1 << i)){
			GPIO_CLR_INT_FLAG(PG, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPG_IRQn);
}

/**
 * @brief       GPIO PH IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PH default IRQ, declared in startup_M261.s.
 */
void GPH_IRQHandler(void)
{
	int i;

	IRQ_ENTER(GPH_IRQn);
	for(i = 0; i < 16; i ++){
		if(GPIO_GET_INT_FLAG(PH, 1 << i)){
			GPIO_CLR_INT_FLAG(PH, 1 << i);
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	IRQ_EXIT(GPH_IRQn);
}

#if !MICROPY_PY_THREAD

__IO uint32_t uwTick;

void SysTick_Handler(void) {
    // Instead of calling HAL_IncTick we do the increment here of the counter.
    // This is purely for efficiency, since SysTick is called 1000 times per
    // second at the highest interrupt priority.
    // Note: we don't need uwTick to be declared volatile here because this is
    // the only place where it can be modified, and the code is more efficient
    // without the volatile specifier.
    uwTick += 1;

    // Read the systick control regster. This has the side effect of clearing
    // the COUNTFLAG bit, which makes the logic in mp_hal_ticks_us
    // work properly.
    SysTick->CTRL;
}

#endif
  
void USBD_IRQHandler(void)
{

    uint32_t u32IntStatus = USBD_GET_INT_FLAG();
    uint32_t u32BusState = USBD_GET_BUS_STATE();

	IRQ_ENTER(USBD_IRQn);
	Handle_USBDEV_Irq(u32IntStatus, u32BusState);
	IRQ_EXIT(USBD_IRQn);
}
  
  
