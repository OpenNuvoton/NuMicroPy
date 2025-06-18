/**************************************************************************//**
 * @file     pin_defs_m46x.c
 * @version  V0.01
 * @brief    pin control for M460
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "py/obj.h"
#include "mods/classPin.h"

// Returns the pin mode. This value returned by this macro should be one of:
// GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_OPEN_DRAIN, GPIO_MODE_QUASI

uint32_t pin_get_mode(const pin_obj_t *pin)
{
    GPIO_T *gpio = pin->gpio;
    uint32_t u32RegVal;
    uint32_t u32PinOffset = pin->pin % 4;

    u32RegVal = *pin->mfp_reg;

    u32RegVal = u32RegVal & (0x1F << (u32PinOffset * 8));

    if(u32RegVal == 0) {
        uint32_t mode = (gpio->MODE >> (pin->pin * 2)) & 3;
        return mode;
    }

    u32RegVal = *pin->mfos_reg;
    if((u32RegVal >> pin->pin) & 0x1) {
        return GPIO_MODE_AF_OD;
    }

    return GPIO_MODE_AF_PP;
}

// Returns the pin pullup/pulldown. The value returned by this macro should
// be one of GPIO_PUSEL_DISABLE, GPIO_PUSEL_PULL_UP, or GPIO_PUSEL_PULL_DOWN.

uint32_t pin_get_pull(const pin_obj_t *pin)
{
    return (pin->gpio->PUSEL >> (pin->pin * 2)) & 3;
}


// Set the pin pullup/pulldown. The mode should
// be one of GPIO_PUSEL_DISABLE, GPIO_PUSEL_PULL_UP, or GPIO_PUSEL_PULL_DOWN.

void pin_set_pull(const pin_obj_t *pin, uint32_t mode)
{

    uint32_t u32PullPinMask;
    uint32_t u32PuselValue;

    u32PullPinMask = 0x3 << (pin->pin * 2);
    mode = (mode & 0x3) << (pin->pin * 2);

    u32PuselValue = pin->gpio->PUSEL;

    u32PuselValue &= ~(u32PullPinMask);
    u32PuselValue |= mode;

    pin->gpio->PUSEL = u32PuselValue;
}


// Returns the af (alternate function) value currently set for a pin.
uint32_t pin_get_af(const pin_obj_t *pin)
{
    uint32_t u32RegVal;
    uint32_t u32PinOffset = pin->pin % 4;

    u32RegVal = *pin->mfp_reg;
    u32RegVal = u32RegVal & (0x1F << (u32PinOffset * 8));

    return u32RegVal;
}

#if 0
* @var SYS_T::GPA_MFOS
* Offset: 0x80  GPIOA Multiple Function Output Select Register
* ---------------------------------------------------------------------------------------------------
* |Bits    |Field     |Descriptions
* | :
----:
| :
----:
| :
---- |
* |[0]     |MFOS0     |GPIOA Pin[x] Multiple Function Pin Output Mode Select
* |        |          |This bit used to select multiple function pin output mode type for PA.x pin.
* |        |          |0 = Multiple function pin output mode type is Push-pull mode.
                           * |        |          |1 = Multiple function pin output mode type is Open-drain mode.
                                   * |        |          |Note 1:
                                   These bits are not retained when D2 power is turned off.
                                   * |[1]     |MFOS1     |GPIOA Pin[x] Multiple Function Pin Output Mode Select
                                       * |        |          |This bit used to select multiple function pin output mode type for PA.x pin.
                                       * |        |          |0 = Multiple function pin output mode type is Push-pull mode.
                                               * |        |          |1 = Multiple function pin output mode type is Open-drain mode.
                                                   * |        |          |Note 1:
                                                       These bits are not retained when D2 power is turned off.


                                                       * Offset: 0x300  GPIOA Multiple Function Control Register 0
                                                       * ---------------------------------------------------------------------------------------------------
                                                       * |Bits    |Field     |Descriptions
                                                   * | :
                                                   ----:
                                                   | :
                                                   ----:
                                                   | :
                                                       ---- |
                                                       * |[4:0]   |PA0MFP    |PA.0 Multi-function Pin Selection
                                                   * |        |          |Note:
                                                           These bits are not retained when D2 power is turned off.
                                                           * |[12:8]  |PA1MFP    |PA.1 Multi-function Pin Selection
                                                   * |        |          |Note:
                                                           These bits are not retained when D2 power is turned off.
                                                           * |[20:16] |PA2MFP    |PA.2 Multi-function Pin Selection
                                                   * |        |          |Note:
                                                           These bits are not retained when D2 power is turned off.
                                                           * |[28:24] |PA3MFP    |PA.3 Multi-function Pin Selection
                                                   * |        |          |Note:
                                                           These bits are not retained when D2 power is turned off.
#endif

                                                           void pin_set_af(const pin_obj_t *pin, const pin_af_obj_t *af_obj, int mode)
    {
        uint32_t u32MFPPinOffset = pin->pin % 4;
        uint32_t u32RegVal;

        u32RegVal = *pin->mfp_reg;

        //0x1F:[4:0]/[12:8]/[20:16]/[28:24] 5 bits
        //*8: offset 0/8/16/24
        u32RegVal &= ~(0x1F << (u32MFPPinOffset * 8));	//clear mfp, set GPIO mode

        if((mode != GPIO_MODE_AF_PP) && (mode != GPIO_MODE_AF_OD)) {
            *pin->mfp_reg = u32RegVal;
            GPIO_SetMode(pin->gpio, 1 << pin->pin, mode);
            return;
        }

        if(af_obj == NULL)
            return;

        u32RegVal |= af_obj->mfp_val;
        *pin->mfp_reg = u32RegVal;

        u32RegVal =  *pin->mfos_reg;
        if(mode == GPIO_MODE_AF_PP) {
            u32RegVal &= ~(1 << pin->pin);
        } else {
            u32RegVal |= (1 << pin->pin);
        }

        *pin->mfos_reg = u32RegVal;
    }

