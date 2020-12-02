/**************************************************************************//**
 * @file     pin_defs_m48x.c
 * @version  V0.01
 * @brief    pin control for M480
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "py/obj.h"
#include "mods/pybpin.h"

void mp_hal_pin_high(const pin_obj_t *pin)
{
	uint32_t u32PinBitMask = 1 << (pin->pin);
	gpio_setportval(pin->port, u32PinBitMask, u32PinBitMask);
}

void mp_hal_pin_low(const pin_obj_t *pin)
{
	uint32_t u32PinBitMask = 1 << (pin->pin);
	gpio_setportval(pin->port, u32PinBitMask, ~u32PinBitMask);
}

int mp_hal_pin_read(const pin_obj_t *pin)
{
	unsigned short u16RegVal;
	
	gpio_readport(pin->port, &u16RegVal);
	return ((u16RegVal >> pin->pin) & 1);
}

#define mp_hal_pin_read(p)      (((p)->gpio->PIN >> (p)->pin) & 1)

// Returns the pin mode. This value returned by this macro should be one of:
// GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_AF

uint32_t pin_get_mode(const pin_obj_t *pin) {

	uint32_t u32RegVal;
	uint32_t u32PinOffset = pin->pin % 8;	
	uint32_t u32PinBitMask = 1 << (pin->pin);
	uint32_t u32ModeReg = REG_GPIOA_DOUT;

	u32RegVal = inp32(pin->mfp_reg);
	u32RegVal = u32RegVal & (0xF << (u32PinOffset * 4));

	if(pin->port == GPIO_PORTA){
		if(pin->pin >= 12){
			u32RegVal = u32RegVal >> (( pin->pin - 8) * 4);
			
			if(u32RegVal != 0x2)
				return GPIO_MODE_AF;

			u32ModeReg = REG_GPIOA_DOUT;

			//Read GPIO output mode enable register
			u32RegVal = inp32(u32ModeReg);
			
			if(u32RegVal & u32PinBitMask)
				return GPIO_MODE_OUTPUT;
			return GPIO_MODE_INPUT;			
		}
	}

	if(u32RegVal == 0)
	{
		//GPIO mode, check input/output		
		if(pin->port == GPIO_PORTA){
			u32ModeReg = REG_GPIOA_DOUT;
		}
		else if(pin->port == GPIO_PORTB){
			u32ModeReg = REG_GPIOB_DOUT;
		}
		else if(pin->port == GPIO_PORTC){
			u32ModeReg = REG_GPIOC_DOUT;
		}
		else if(pin->port == GPIO_PORTD){
			u32ModeReg = REG_GPIOD_DOUT;
		}
		else if(pin->port == GPIO_PORTE){
			u32ModeReg = REG_GPIOE_DOUT;
		}
		else if(pin->port == GPIO_PORTG){
			u32ModeReg = REG_GPIOG_DOUT;
		}
		else if(pin->port == GPIO_PORTH){
			u32ModeReg = REG_GPIOH_DOUT;
		}
		
		//Read GPIO output mode enable register
		u32RegVal = inp32(u32ModeReg);
		
		if(u32RegVal & u32PinBitMask)
			return GPIO_MODE_OUTPUT;
		return GPIO_MODE_INPUT;
	}

	return GPIO_MODE_AF;
}

// Returns the pin pullup/pulldown. The value returned by this macro should
// be one of GPIO_PUSEL_DISABLE, GPIO_PUSEL_PULL_UP, or GPIO_PUSEL_PULL_DOWN.

uint32_t pin_get_pull(const pin_obj_t *pin) {

	uint32_t u32PullReg = REG_GPIOA_PUEN;
	uint32_t u32RegVal;
	uint32_t u32PinBitMask = 1 << (pin->pin);
	
	if(pin->port == GPIO_PORTA){
		u32PullReg = REG_GPIOA_PUEN;
	}
	else if(pin->port == GPIO_PORTB){
		u32PullReg = REG_GPIOB_PUEN;
	}
	else if(pin->port == GPIO_PORTC){
		u32PullReg = REG_GPIOC_PUEN;
	}
	else if(pin->port == GPIO_PORTD){
		u32PullReg = REG_GPIOD_PUEN;
	}
	else if(pin->port == GPIO_PORTE){
		u32PullReg = REG_GPIOE_PUEN;
	}
	else if(pin->port == GPIO_PORTG){
		u32PullReg = REG_GPIOG_PUEN;
	}
	else if(pin->port == GPIO_PORTH){
		u32PullReg = REG_GPIOH_PUEN;
	}

	//Read GPIO pull up/down register
	u32RegVal = inp32(u32PullReg);

	if(pin->port == GPIO_PORTG){
		if(u32RegVal & u32PinBitMask)
			return GPIO_PUSEL_DISABLE;
		else
			return GPIO_PUSEL_PULL_UP;
	}

	if(u32RegVal & u32PinBitMask){
		if(pin->port == GPIO_PORTB){
			return GPIO_PUSEL_PULL_DOWN;
		}
		
		if(pin->port == GPIO_PORTD){
			if((pin->pin == 0) || (pin->pin == 12) || (pin->pin == 14) || (pin->pin == 15))
				return GPIO_PUSEL_PULL_DOWN;
		}

		if(pin->port == GPIO_PORTE){
			if(pin->pin == 7)
				return GPIO_PUSEL_PULL_DOWN;
		}
			
		return GPIO_PUSEL_PULL_UP;
	}

	return GPIO_PUSEL_DISABLE;
}

// Set the pin pullup/pulldown type. The type by this macro should
// be one of GPIO_PUSEL_DISABLE, GPIO_PUSEL_PULL_UP, or GPIO_PUSEL_PULL_DOWN.
void pin_set_pull(const pin_obj_t *pin, int type)
{
	uint32_t u32PullReg;
	uint32_t u32RegVal;
	uint32_t u32PinBitMask = 1 << (pin->pin);
	
	if(pin->port == GPIO_PORTA){
		u32PullReg = REG_GPIOA_PUEN;
	}
	else if(pin->port == GPIO_PORTB){
		u32PullReg = REG_GPIOB_PUEN;
	}
	else if(pin->port == GPIO_PORTC){
		u32PullReg = REG_GPIOC_PUEN;
	}
	else if(pin->port == GPIO_PORTD){
		u32PullReg = REG_GPIOD_PUEN;
	}
	else if(pin->port == GPIO_PORTE){
		u32PullReg = REG_GPIOE_PUEN;
	}
	else if(pin->port == GPIO_PORTG){
		u32PullReg = REG_GPIOG_PUEN;
	}
	else if(pin->port == GPIO_PORTH){
		u32PullReg = REG_GPIOH_PUEN;
	}

	//Read GPIO pull up/down register
	u32RegVal = inp32(u32PullReg);
	u32RegVal &= ~u32PinBitMask;

	if(pin->port == GPIO_PORTG){
		if(type == GPIO_PUSEL_DISABLE)
			u32RegVal |= u32PinBitMask;
	}

	if(pin->port == GPIO_PORTB){
		if(type == GPIO_PUSEL_PULL_DOWN)
			u32RegVal |= u32PinBitMask;
	}

	if(pin->port == GPIO_PORTD){
		if((pin->pin == 0) || (pin->pin == 12) || (pin->pin == 14) || (pin->pin == 15)){
			if(type == GPIO_PUSEL_PULL_DOWN)
				u32RegVal |= u32PinBitMask;
		}
		else if(type == GPIO_PUSEL_PULL_UP){
			u32RegVal |= u32PinBitMask;
		}
	}

	if(pin->port == GPIO_PORTE){
		if(pin->pin == 7){
			if(type == GPIO_PUSEL_PULL_DOWN)
				u32RegVal |= u32PinBitMask;
		}
		else if(type == GPIO_PUSEL_PULL_UP){
			u32RegVal |= u32PinBitMask;
		}
	}

	if((pin->port == GPIO_PORTA) || (pin->port == GPIO_PORTC) || (pin->port == GPIO_PORTH)){
		if(type == GPIO_PUSEL_PULL_UP)
			u32RegVal |= u32PinBitMask;
	}

	outp32(u32PullReg, u32RegVal);
	
}


// Returns the af (alternate function) value currently set for a pin.
uint32_t pin_get_af(const pin_obj_t *pin) {

	uint32_t u32RegVal;
	uint32_t u32PinOffset = pin->pin % 8;	

	u32RegVal = inp32(pin->mfp_reg);
	u32RegVal = u32RegVal & (0xF << (u32PinOffset * 4));

	return u32RegVal;
}

void pin_set_af(const pin_obj_t *pin, const pin_af_obj_t *af_obj, int mode){
	uint32_t u32PinBitMask = 1 << (pin->pin);
	uint32_t u32MFPPinOffset = pin->pin % 8;
	uint32_t u32RegVal;

	u32RegVal = inp32(pin->mfp_reg);
	u32RegVal &= ~(0xF << (u32MFPPinOffset * 4));	//clear mfp, set GPIO mode

	if(mode != GPIO_MODE_AF){

		if(pin->port == GPIO_PORTA){
			if(pin->pin >= 12){
				u32RegVal |= (0x2 << ((pin->pin - 8) * 4)); 
			}
		}
	
		outp32(pin->mfp_reg, u32RegVal);
		if(mode == GPIO_MODE_OUTPUT)
			gpio_setportdir(pin->port, u32PinBitMask, u32PinBitMask);
		else
			gpio_setportdir(pin->port, u32PinBitMask, ~u32PinBitMask);
		return;
	}

	if(af_obj == NULL)
		return;
		
	u32RegVal |= af_obj->mfp_val;
	outp32(pin->mfp_reg, u32RegVal);
	
}
