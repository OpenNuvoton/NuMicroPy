/**************************************************************************//**
 * @file     N9H26_ISR.c
 * @version  V0.10
 * @brief   N9H26 Main interrupt server routines
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>

#include "wblib.h"
#include "N9H26_reg.h"
#include "N9H26_GPIO.h"

#include "mods/pybpin.h"
#include "mods/pybirq.h"

#include "hal/pin_int.h"

/**
 * @brief       GPIO PA and PB IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PA and PB default IRQ
 */
void EXTINT0_IRQHandler(void)
{
	int i;
	unsigned short u16TrigSrc;

	IRQ_ENTER(IRQ_EXTINT0);

	gpio_gettriggersrc(GPIO_PORTA, &u16TrigSrc);

	for(i = 0; i < 16; i ++){
		if(u16TrigSrc & (1 << i)){
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	
	gpio_cleartriggersrc(GPIO_PORTA);
	
	gpio_gettriggersrc(GPIO_PORTB, &u16TrigSrc);

	for(i = 0; i < 16; i ++){
		if(u16TrigSrc & (1 << i)){
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	
	gpio_cleartriggersrc(GPIO_PORTB);

	IRQ_EXIT(IRQ_EXTINT0);
}

/**
 * @brief       GPIO PC and PD IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PC and PD default IRQ
 */
void EXTINT1_IRQHandler(void)
{
	int i;
	unsigned short u16TrigSrc;

	IRQ_ENTER(IRQ_EXTINT1);

	gpio_gettriggersrc(GPIO_PORTC, &u16TrigSrc);

	for(i = 0; i < 16; i ++){
		if(u16TrigSrc & (1 << i)){
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	
	gpio_cleartriggersrc(GPIO_PORTC);
	
	gpio_gettriggersrc(GPIO_PORTD, &u16TrigSrc);

	for(i = 0; i < 16; i ++){
		if(u16TrigSrc & (1 << i)){
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	
	gpio_cleartriggersrc(GPIO_PORTD);
	
	IRQ_EXIT(IRQ_EXTINT1);
}

/**
 * @brief       GPIO PE and PG IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PE and PG default IRQ
 */
void EXTINT2_IRQHandler(void)
{
	int i;
	unsigned short u16TrigSrc;

	IRQ_ENTER(IRQ_EXTINT2);

	gpio_gettriggersrc(GPIO_PORTE, &u16TrigSrc);

	for(i = 0; i < 16; i ++){
		if(u16TrigSrc & (1 << i)){
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	
	gpio_cleartriggersrc(GPIO_PORTE);
	
	gpio_gettriggersrc(GPIO_PORTG, &u16TrigSrc);

	for(i = 0; i < 16; i ++){
		if(u16TrigSrc & (1 << i)){
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	
	gpio_cleartriggersrc(GPIO_PORTG);
	
	IRQ_EXIT(IRQ_EXTINT2);
}

/**
 * @brief       GPIO PH IRQ
 *
 * @param       None
 *
 * @return      None
 *
 * @details     The PH default IRQ
 */
void EXTINT3_IRQHandler(void)
{
	int i;
	unsigned short u16TrigSrc;

	IRQ_ENTER(IRQ_EXTINT3);

	gpio_gettriggersrc(GPIO_PORTH, &u16TrigSrc);

	for(i = 0; i < 16; i ++){
		if(u16TrigSrc & (1 << i)){
			//Call gpio irq handler
			Handle_GPIO_Irq(i);
		}
	}
	
	gpio_cleartriggersrc(GPIO_PORTH);
	
	IRQ_EXIT(IRQ_EXTINT3);
}



