/*
 * This file is part of the MicroPython project, http://micropython.org/
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/objtuple.h"
#include "py/objarray.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/binary.h"
#include "py/stream.h"
#include "py/mphal.h"

#include "classADC.h"


#if MICROPY_HW_ENABLE_HW_EADC

#define EADC_EXTERNAL_CHANNELS 24
#define EADC_INTERNAL_CHANNELS 4

/*EADC channel def*/
#define EADC_CHANNEL_0            ( 0UL)
#define EADC_CHANNEL_1            ( 1UL)
#define EADC_CHANNEL_2            ( 2UL)
#define EADC_CHANNEL_3            ( 3UL)
#define EADC_CHANNEL_4            ( 4UL)
#define EADC_CHANNEL_5            ( 5UL)
#define EADC_CHANNEL_6            ( 6UL)
#define EADC_CHANNEL_7            ( 7UL)
#define EADC_CHANNEL_8            ( 8UL)
#define EADC_CHANNEL_9            ( 9UL)
#define EADC_CHANNEL_10           (10UL)
#define EADC_CHANNEL_11           (11UL)
#define EADC_CHANNEL_12           (12UL)
#define EADC_CHANNEL_13           (13UL)
#define EADC_CHANNEL_14           (14UL)
#define EADC_CHANNEL_15           (15UL)
#define EADC_CHANNEL_16           (16UL)
#define EADC_CHANNEL_17           (17UL)
#define EADC_CHANNEL_18           (18UL)
#define EADC_CHANNEL_19           (19UL)
#define EADC_CHANNEL_20           (20UL)
#define EADC_CHANNEL_21           (21UL)
#define EADC_CHANNEL_22           (22UL)
#define EADC_CHANNEL_23           (23UL)
#define EADC_CHANNEL_24           (24UL)
#define EADC_CHANNEL_25           (25UL)
#define EADC_CHANNEL_26           (26UL)
#define EADC_CHANNEL_27           (27UL)


/*internal sample module related definition*/
#define EADC_CHANNEL_BANDGAP      	EADC_CHANNEL_24
#define EADC_CHANNEL_TEMPSENSOR   	EADC_CHANNEL_25
#define EADC_CHANNEL_VBAT        	EADC_CHANNEL_26
#define EADC_INTERNAL_CHANNEL_DUMMY (const pin_obj_t*)0xFFFFFFFF

#define EADC_RESOLUTION_12B (12UL)
#define EADC_FIRST_GPIO_CHANNEL  (0)
#define EADC_LAST_GPIO_CHANNEL   (EADC_EXTERNAL_CHANNELS - 1)
#define EADC_LAST_ADC_CHANNEL    ( EADC_EXTERNAL_CHANNELS + EADC_INTERNAL_CHANNELS - 1)

typedef struct _pyb_obj_adc_t {
    mp_obj_base_t base;
    mp_obj_t pin_name;
    int channel;
    EADC_T* eadc_base;
} pyb_obj_adc_t;

typedef struct _pyb_adc_all_obj_t {
    mp_obj_base_t base;
    EADC_T *eadc_base;
} pyb_adc_all_obj_t;

typedef struct _pyb_eadc_channel_obj {
    const pin_obj_t* eadc_channel_pin_obj;
} eadc_channel_obj_t;

static const eadc_channel_obj_t eadc_channel_obj_list[EADC_EXTERNAL_CHANNELS + EADC_INTERNAL_CHANNELS] = {

#define INIT_ELEMENT(element0) {element0}  //init a single element

#if defined(MICROPY_HW_EADC0_CH0)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH0),     //Channel0
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH1)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH1),     //Channel1
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH2)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH2),     //Channel2
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH3)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH3),     //Channel3
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH4)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH4),     //Channel4
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH5)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH5),     //Channel5
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH6)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH6),     //Channel6
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH7)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH7),     //Channel7
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH8)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH8),     //Channel8
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH9)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH9),     //Channel9
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH10)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH10),     //Channel10
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH11)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH11),     //Channel11
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH12)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH12),     //Channel12
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH13)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH13),     //Channel13
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH14)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH14),     //Channel14
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH15)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH15),     //Channel15
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH16)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH16),     //Channel16
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH17)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH17),     //Channel17
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH18)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH18),     //Channel18
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH19)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH19),     //Channel19
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH20)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH20),     //Channel20
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH21)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH21),     //Channel21
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH22)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH22),     //Channel22
#else
    INIT_ELEMENT(NULL),
#endif
#if defined(MICROPY_HW_EADC0_CH23)
    INIT_ELEMENT(MICROPY_HW_EADC0_CH23),     //Channel23
#else
    INIT_ELEMENT(NULL),
#endif
    INIT_ELEMENT(EADC_INTERNAL_CHANNEL_DUMMY),						//Internal Vbg(band-gap voltage)
    INIT_ELEMENT(EADC_INTERNAL_CHANNEL_DUMMY),						//Internal Vtemp(tempteratures sensor)
    INIT_ELEMENT(EADC_INTERNAL_CHANNEL_DUMMY),						//Internal Vbat/4(batter power/4)
    INIT_ELEMENT(NULL),												//Internal AVdd/4
};

//adc_get_internal_channel()
// convert user-facing channel number into internal channel number
static inline uint32_t adc_get_internal_channel(uint32_t channel)
{

    if (channel ==EADC_CHANNEL_24) {
        channel = EADC_CHANNEL_BANDGAP;
    } else if (channel == EADC_CHANNEL_25) {
        channel = EADC_CHANNEL_TEMPSENSOR;
    } else if (channel == EADC_CHANNEL_26) {
        channel = EADC_CHANNEL_VBAT;
    }

    return channel;
}

#define IS_ADC_CHANNEL(ch) (((ch) < EADC_EXTERNAL_CHANNELS)  || \
                            ((ch) == EADC_CHANNEL_TEMPSENSOR)||\
                            ((ch) == EADC_CHANNEL_BANDGAP)||\
                            ((ch) == EADC_CHANNEL_VBAT)\
                           )

//is_adcx_channel()
static bool is_adcx_channel(int channel)
{
    return IS_ADC_CHANNEL(channel);
}

static inline void eadc_TurnOnClk(EADC_T *eadc_base)
{

    /* Enable EADC peripheral clock */
    CLK_SetModuleClock(EADC0_MODULE, CLK_EADCSEL_EADC0SEL_APLL1_DIV2,  CLK_EADCDIV_EADC0DIV(10));

    /* Enable EADC module clock */
    CLK_EnableModuleClock(EADC0_MODULE);

    EADC_Open(eadc_base, EADC_CTL_DIFFEN_SINGLE_END);
}

static void adc_init_single(pyb_obj_adc_t *adc_obj)
{
    if (!is_adcx_channel(adc_obj->channel)) {
        return;
    }

    if (EADC_FIRST_GPIO_CHANNEL <= adc_obj->channel && adc_obj->channel <= EADC_LAST_GPIO_CHANNEL) {
        // Channels 0-23 correspond to real pins. Configure the GPIO pin in ADC mode.
        const pin_obj_t *pin = eadc_channel_obj_list[adc_obj->channel].eadc_channel_pin_obj;
        mp_hal_pin_config_alt(pin, MP_HAL_PIN_MODE_ALT_PUSH_PULL, AF_FN_EADC, 0);
        GPIO_SetMode(pin->gpio, 1 << pin->pin, MP_HAL_PIN_MODE_INPUT);  //Set Analog pin as input mode
        GPIO_DISABLE_DIGITAL_PATH(pin->gpio, 1 << pin->pin);            /* Disable the digital input path to avoid the leakage current. */
    }

    eadc_TurnOnClk(adc_obj->eadc_base);

    /* Configure the sample module 0 for analog input channel and software trigger source.*/
    EADC_ConfigSampleModule(adc_obj->eadc_base, adc_obj->channel, EADC_SOFTWARE_TRIGGER, adc_obj->channel);

    /* Clear the A/D ADINT0 interrupt flag for safe */
    EADC_CLR_INT_FLAG(adc_obj->eadc_base, EADC_STATUS2_ADIF0_Msk);
}

static inline uint32_t eadc_config_and_read_channel(EADC_T *eadc_base, uint32_t u32ChannelNum)
{
    uint32_t u32ChannelMsk = 0x1 << u32ChannelNum;
    //ASSERT(eadc_base==EADC);
    /*
     *  Sample module0   <----> Channel 0
     *  Sample module1   <----> Channel 1
     * 					 .
     * 					 .
     *
     *  Sample module24  <----> Band-gap (ExtChannel)
     *  Sample module25  <----> Temperature sensor (ExtChannel)
     *  Sample module26  <----> VBat (ExtChannel)
     * */

    /* Configure the sample module 0 for analog input channel number and software trigger source.*/
    if(     (EADC_FIRST_GPIO_CHANNEL <= u32ChannelNum)
            &&  (u32ChannelNum <= EADC_LAST_GPIO_CHANNEL) ) {
        EADC_ConfigSampleModule(eadc_base, u32ChannelNum, EADC_SOFTWARE_TRIGGER, u32ChannelNum);

    } else if (( EADC_LAST_GPIO_CHANNEL < u32ChannelNum)
               &&  ( u32ChannelNum <= EADC_LAST_ADC_CHANNEL ) ) {
        /*Using external channel: Band-gap, temperature, Vbat*/
        /* Set sample module u32ChannelNum external sampling time to 0xFF */
        EADC_SetExtendSampleTime(eadc_base, u32ChannelNum, 0xFF);
    } else {
        printf("error:%s, %d\n", __FILE__, __LINE__);
        while(1);
    }

    /* Clear the A/D ADINT0 interrupt flag for safe */
    EADC_CLR_INT_FLAG(eadc_base, EADC_STATUS2_ADIF0_Msk);

    /* Enable Sample Module 0 */
    EADC_START_CONV(eadc_base, u32ChannelMsk);

    /* Wait conversion done */
    while(EADC_GET_DATA_VALID_FLAG(eadc_base, u32ChannelMsk) != (u32ChannelMsk));

    return EADC_GET_CONV_DATA(eadc_base, u32ChannelNum);
}

/* adc all object                                                             */
void adc_init_all(pyb_adc_all_obj_t *adc_all, uint32_t resolution, uint32_t en_mask)
{

    switch (resolution) {
    //case 8:  resolution = ADC_RESOLUTION_8B;  break;
    //case 10: resolution = ADC_RESOLUTION_10B; break;
    case 12:
        resolution = EADC_RESOLUTION_12B;
        break;
    default:
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
                                                "resolution %d not supported. only support 12", resolution));
    }

    eadc_TurnOnClk(adc_all->eadc_base);

    for (uint32_t channel = EADC_FIRST_GPIO_CHANNEL; channel <= EADC_LAST_ADC_CHANNEL; ++channel) {
        // only initialise those channels that are selected with the en_mask
        if (en_mask & (1 << channel)) {
            // Channels 0-16 correspond to real pins. Configure the GPIO pin in
            // ADC mode.

            //const pin_obj_t *pin = adc_all->eadc_channel_obj[channel].eadc_channel_pin_obj;
            const pin_obj_t *pin = eadc_channel_obj_list[channel].eadc_channel_pin_obj;
            if (pin && (pin != EADC_INTERNAL_CHANNEL_DUMMY)) {
                mp_hal_pin_config_alt(pin, MP_HAL_PIN_MODE_ALT_PUSH_PULL, AF_FN_EADC, 0); //FIXME: using PUSH_PULL to trigger MFP switch function
                GPIO_SetMode(pin->gpio, 1 << pin->pin, MP_HAL_PIN_MODE_INPUT);            //Set Analog pin as input mode
                GPIO_DISABLE_DIGITAL_PATH(pin->gpio, 1 << pin->pin);       /* Disable the digital input path to avoid the leakage current. */
            } else if ((EADC_INTERNAL_CHANNEL_DUMMY==pin)&&(channel == EADC_CHANNEL_TEMPSENSOR)) {
                SYS_UnlockReg();
                SYS->IVSCTL |= SYS_IVSCTL_VTEMPEN_Msk; /* Enable temperature sensor */
                /* Set reference voltage to external pin */
                SYS_SetVRef(SYS_VREFCTL_VREF_PIN);
                //SYS_LockReg();
            } else if ((EADC_INTERNAL_CHANNEL_DUMMY==pin)&&(channel == EADC_CHANNEL_VBAT)) {
                SYS_UnlockReg();
                /* Enable VBAT unity gain buffer */
                SYS->IVSCTL |= SYS_IVSCTL_VBATUGEN_Msk;
                //SYS_LockReg();
            } else if(channel > EADC_LAST_ADC_CHANNEL) {
                nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
                                                        "Paramemter error"));
            }
        }
    }
}

float eadc_read_core_vbat(EADC_T *adcHandle)
{
    uint32_t raw_value = eadc_config_and_read_channel(adcHandle, EADC_CHANNEL_VBAT);

    return raw_value; //raw data is Vbat/4 in M48x
}

float eadc_read_core_temp_float(EADC_T *adcHandle)
{
    int32_t raw_value = eadc_config_and_read_channel(adcHandle, EADC_CHANNEL_TEMPSENSOR);
    float temp;
    // constants assume 12-bit resolution so we scale the raw value to 12-bits
    //raw_value <<= (12 - adc_get_resolution(adcHandle));

    //float core_temp_avg_slope = (*ADC_CAL2 - *ADC_CAL1) / 80.0;
    //return (((float)raw_value * adc_refcor - *ADC_CAL1) / core_temp_avg_slope) + 30.0f;
    temp = 25+(((float)raw_value/4095*3300)-1030)/(-2.7);
    return (float)temp;
}



/******************************************************************************/
/* MicroPython bindings : adc object (single channel)                         */

static void adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_obj_adc_t *self = self_in;
    mp_print_str(print, "<ADC on ");
    mp_obj_print_helper(print, self->pin_name, PRINT_STR);
    mp_printf(print, " channel=%u>", self->channel);
}

/// \classmethod \constructor(pin)
/// Create an ADC object associated with the given pin.
/// This allows you to then read analog values on that pin.
static mp_obj_t adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    // check number of arguments
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    // 1st argument is the pin name
    mp_obj_t pin_obj = args[0];

    uint32_t channel=0xffffffff;

    if (MP_OBJ_IS_INT(pin_obj)) {
        channel = adc_get_internal_channel(mp_obj_get_int(pin_obj));
    } else {
        const pin_obj_t *pin = pin_find(pin_obj);

        if (pin_find_af(pin, AF_FN_EADC, 0) == NULL) { //check if the pin has EADC channel
            // No ADC1 function on that pin
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "pin %q does not have ADC capabilities", pin->name));
        }

        for(uint32_t i =0 ; i <= EADC_LAST_GPIO_CHANNEL; i++) {
            if(pin == eadc_channel_obj_list[i].eadc_channel_pin_obj) {
                channel = i; //FIXME: workaround to get ADC channel because I cannot get correct pin->adc_channel value.
                break;
            }
        }

        if(channel==0xffffffff) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "%q: pin mapping error!", pin->name));
        }
    }

    if (!is_adcx_channel(channel)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "not a valid ADC Channel: %d", channel));
    }

    if (EADC_FIRST_GPIO_CHANNEL <= channel && channel <= EADC_LAST_ADC_CHANNEL) {
        // these channels correspond to physical GPIO ports so make sure they exist
        if (eadc_channel_obj_list[channel].eadc_channel_pin_obj == NULL) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
                                                    "channel %d not available on this board", channel));
        }
    }

    pyb_obj_adc_t *o = m_new_obj(pyb_obj_adc_t);
    memset(o, 0, sizeof(*o));
    o->base.type = &machine_adc_type;
    o->pin_name = pin_obj;
    o->channel = channel;
    //o->eadc_base =eadc_channel_obj_list[channel].eadc_channel_pin_obj->af->reg;
    o->eadc_base = EADC0;
    //ASSERT(EADC==o->eadc_base);
    adc_init_single(o);

    return o;
}

/// \method read()
/// Read the value on the analog pin and return it.  The returned value
/// will be between 0 and 4095.

static mp_obj_t adc_read(mp_obj_t self_in)
{
    pyb_obj_adc_t *self = self_in;
    return mp_obj_new_int(eadc_config_and_read_channel(self->eadc_base, self->channel));
}
static MP_DEFINE_CONST_FUN_OBJ_1(adc_read_obj, adc_read);


static const mp_rom_map_elem_t adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&adc_read_obj) },
};

static MP_DEFINE_CONST_DICT(adc_locals_dict, adc_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_adc_type,
    MP_QSTR_ADC,
    MP_TYPE_FLAG_NONE,
    make_new, adc_make_new,
    print, adc_print,
    locals_dict, &adc_locals_dict
);


/******************************************************************************/
/* MicroPython bindings : adc_all object                                      */

static void adc_all_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    mp_print_str(print, "<List Eanbled Channel: \n");

    for(uint32_t i= EADC_FIRST_GPIO_CHANNEL; i <= EADC_LAST_ADC_CHANNEL; i++) {
        if(eadc_channel_obj_list[i].eadc_channel_pin_obj)
            mp_printf(print, " <channel %u>\n", i);
    }
}

static mp_obj_t adc_all_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_int_t res;
    // check number of arguments
    mp_arg_check_num(n_args, n_kw, 1, 2, false);

    if (n_args == 0) {
        res = 12;
    } else {
        res = mp_obj_get_int(args[0]);
    }

    // make ADCAll object
    pyb_adc_all_obj_t *o = m_new_obj(pyb_adc_all_obj_t);
    o->eadc_base = EADC0;
    o->base.type = &machine_adc_all_type;

    //uint32_t en_mask = 0xffffffff; //default all channel open
    if(o==NULL) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
                                                "Class allocate fail\n" ));
    }

    uint32_t en_mask = 0x7F003FF; //default partial port open
    if (n_args > 1) {
        en_mask =  mp_obj_get_int(args[1]);
    }
    adc_init_all(o, res, en_mask);

    return o;
}

static mp_obj_t adc_all_read_channel(mp_obj_t self_in, mp_obj_t channel)
{
    pyb_adc_all_obj_t *self = self_in;
    uint32_t chan = adc_get_internal_channel(mp_obj_get_int(channel));
    uint32_t data = eadc_config_and_read_channel(self->eadc_base, chan);

    return mp_obj_new_int(data);
}
static MP_DEFINE_CONST_FUN_OBJ_2(adc_all_read_channel_obj, adc_all_read_channel);

//adc_all_read_core_temp()
static mp_obj_t adc_all_read_core_temp(mp_obj_t self_in)
{
    pyb_adc_all_obj_t *self = self_in;

    float data = eadc_read_core_temp_float(self->eadc_base);
    return mp_obj_new_float(data);
}
static MP_DEFINE_CONST_FUN_OBJ_1(adc_all_read_core_temp_obj, adc_all_read_core_temp);

static mp_obj_t adc_all_read_core_vbat(mp_obj_t self_in)
{
    pyb_adc_all_obj_t *self = self_in;
    float data = eadc_read_core_vbat(self->eadc_base);
    return mp_obj_new_float(data);
}
static MP_DEFINE_CONST_FUN_OBJ_1(adc_all_read_core_vbat_obj, adc_all_read_core_vbat);


static const mp_rom_map_elem_t adc_all_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_channel), MP_ROM_PTR(&adc_all_read_channel_obj) },
#if MICROPY_PY_BUILTINS_FLOAT
    { MP_ROM_QSTR(MP_QSTR_read_core_temp), MP_ROM_PTR(&adc_all_read_core_temp_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_core_vbat), MP_ROM_PTR(&adc_all_read_core_vbat_obj) },

    /* Below functions are not supported on M48X */
    //{ MP_ROM_QSTR(MP_QSTR_read_core_vref), MP_ROM_PTR(&adc_all_read_core_vref_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_read_vref), MP_ROM_PTR(&adc_all_read_vref_obj) },
#endif
};

static MP_DEFINE_CONST_DICT(adc_all_locals_dict, adc_all_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    machine_adc_all_type,
    MP_QSTR_ADCAll,
    MP_TYPE_FLAG_NONE,
    make_new, adc_all_make_new,
    print, adc_all_print,
    locals_dict, &adc_all_locals_dict
);


#endif



