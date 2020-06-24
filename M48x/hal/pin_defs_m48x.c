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

// Returns the pin mode. This value returned by this macro should be one of:
// GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_OPEN_DRAIN, GPIO_MODE_QUASI

uint32_t pin_get_mode(const pin_obj_t *pin) {
    GPIO_T *gpio = pin->gpio;
	uint32_t u32RegVal;
	uint32_t u32PinOffset = pin->pin % 8;	

	u32RegVal = *pin->mfp_reg;

	u32RegVal = u32RegVal & (0xF << (u32PinOffset * 4));
		
	if(u32RegVal == 0){
		uint32_t mode = (gpio->MODE >> (pin->pin * 2)) & 3;
		return mode;
	}

	u32RegVal = *pin->mfos_reg;
	if((u32RegVal >> pin->pin) & 0x1){
		return GPIO_MODE_AF_OD;
	}

	return GPIO_MODE_AF_PP;
}

// Returns the pin pullup/pulldown. The value returned by this macro should
// be one of GPIO_PUSEL_DISABLE, GPIO_PUSEL_PULL_UP, or GPIO_PUSEL_PULL_DOWN.

uint32_t pin_get_pull(const pin_obj_t *pin) {
    return (pin->gpio->PUSEL >> (pin->pin * 2)) & 3;
}

// Returns the af (alternate function) value currently set for a pin.
uint32_t pin_get_af(const pin_obj_t *pin) {
	uint32_t u32RegVal;
	uint32_t u32PinOffset = pin->pin % 8;
	
	u32RegVal = *pin->mfp_reg;
	u32RegVal = u32RegVal & (0xF << (u32PinOffset * 4));

	return u32RegVal;
}

void pin_set_af(const pin_obj_t *pin, const pin_af_obj_t *af_obj, int mode){
	uint32_t u32MFPPinOffset = pin->pin % 8;
	uint32_t u32RegVal;

	u32RegVal = *pin->mfp_reg;
	u32RegVal &= ~(0xF << (u32MFPPinOffset * 4));	//clear mfp, set GPIO mode

	if((mode != GPIO_MODE_AF_PP) && (mode != GPIO_MODE_AF_OD)){
		*pin->mfp_reg = u32RegVal;
		GPIO_SetMode(pin->gpio, 1 << pin->pin, mode);
		return;
	}

	if(af_obj == NULL)
		return;
		
	u32RegVal |= af_obj->mfp_val;
	*pin->mfp_reg = u32RegVal;

	u32RegVal =  *pin->mfos_reg;
	if(mode == GPIO_MODE_AF_PP){
		u32RegVal &= ~(1 << pin->pin);
	}
	else{
		u32RegVal |= (1 << pin->pin);
	}

	*pin->mfos_reg = u32RegVal;
}

