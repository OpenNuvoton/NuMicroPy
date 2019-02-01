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

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/binary.h"
#include "py/mphal.h"
#include "pybadc.h"
#include "pybpin.h"
//#include "dddebug.h"
//#include "timer.h"

#if MICROPY_HW_ENABLE_HW_EADC

static const eadc_channel_obj_t eadc_channel_obj_list[16+3] = {

#define INIT_ELEMENT(element0) {element0}  //init a single element
	INIT_ELEMENT(MICROPY_HW_EADC0_CH0),     //Channel0
	INIT_ELEMENT(MICROPY_HW_EADC0_CH1),     //Channel1
	INIT_ELEMENT(MICROPY_HW_EADC0_CH2),     //Channel2
	INIT_ELEMENT(MICROPY_HW_EADC0_CH3),     //Channel3
	INIT_ELEMENT(NULL),                     //Channel4
	INIT_ELEMENT(NULL),                     //Channel5
	INIT_ELEMENT(MICROPY_HW_EADC0_CH6),     //Channel6
	INIT_ELEMENT(MICROPY_HW_EADC0_CH7),     //Channel7
	INIT_ELEMENT(MICROPY_HW_EADC0_CH8),     //Channel8
	INIT_ELEMENT(MICROPY_HW_EADC0_CH9),     //Channel9
	INIT_ELEMENT(NULL),                     //Channel10
	INIT_ELEMENT(NULL),                     //Channel11
	INIT_ELEMENT(NULL),                     //Channel12
	INIT_ELEMENT(NULL),                     //Channel13
	INIT_ELEMENT(NULL),                     //Channel14
	INIT_ELEMENT(NULL),                     //Channel15
	INIT_ELEMENT(NULL),                                //Channel16: internal band-gap
	INIT_ELEMENT(EADC_INTERNAL_CHANNEL_DUMMY),         //Channel17: internal temperature sensor
    INIT_ELEMENT(EADC_INTERNAL_CHANNEL_DUMMY),         //Channel18: internal VBat
#undef INIT_ELEMENT
};

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
     *  Sample module16  <----> Band-gap (ExtChannel) 
     *  Sample module17  <----> Temperature sensor (ExtChannel)  					 
	 *  Sample module18  <----> VBat (ExtChannel)	
	 * */
	
	/* Configure the sample module 0 for analog input channel number and software trigger source.*/
	#if defined(__M480_H__)
        if(     (0<=u32ChannelNum) 
            &&  (u32ChannelNum<16) ) {
            EADC_ConfigSampleModule(eadc_base, u32ChannelNum, EADC_SOFTWARE_TRIGGER, u32ChannelNum);
        
        } else if (( 16 <= u32ChannelNum) 
            &&  ( u32ChannelNum <= 18 ) ) { 
            /*Using external channel: Band-gap, temperature, Vbat*/
            /* Set sample module u32ChannelNum external sampling time to 0xF */
            EADC_SetExtendSampleTime(eadc_base, u32ChannelNum, 0xF);
        } else {
            printf("error:%s, %d\n", __FILE__, __LINE__);
            while(1);
        }
    #else
        #error "not implement yet"
    #endif
	/* Clear the A/D ADINT0 interrupt flag for safe */
	EADC_CLR_INT_FLAG(eadc_base, EADC_STATUS2_ADIF0_Msk);
      
    /* Enable Sample Module 0 */
	EADC_START_CONV(eadc_base, u32ChannelMsk);

	/* Wait conversion done */
	while(EADC_GET_DATA_VALID_FLAG(eadc_base, u32ChannelMsk) != (u32ChannelMsk));
    
    return EADC_GET_CONV_DATA(eadc_base, u32ChannelNum);
}

/// \moduleref pyb
/// \class ADC - analog to digital conversion: read analog values on a pin
///
/// Usage:
///
///     adc = pyb.ADC(pin)              # create an analog object from a pin
///     val = adc.read()                # read an analog value
///
///     adc = pyb.ADCAll(resolution)    # creale an ADCAll object
///     val = adc.read_channel(channel) # read the given channel
///     val = adc.read_core_temp()      # read MCU temperature
///     val = adc.read_core_vbat()      # read MCU VBAT
///     val = adc.read_core_vref()      # read MCU VREF

//adc_get_internal_channel()
// convert user-facing channel number into internal channel number
static inline uint32_t adc_get_internal_channel(uint32_t channel) {
    #if defined(__M480_H__)
    // on M480 MCU we want channel 16 to always be the TEMPSENSOR
    // (on some MCUs ADC_CHANNEL_TEMPSENSOR=16, on others it doesn't)
    
    if (channel ==16) {
        channel = EADC_CHANNEL_BANDGAP;
    } else if (channel == 17) {
        channel = EADC_CHANNEL_TEMPSENSOR;
    } else if (channel == 18) {
        channel = EADC_CHANNEL_VBAT;
    } 
    #endif
    return channel;
}

 //is_adcx_channel()
STATIC bool is_adcx_channel(int channel) {

#if defined(__M480_H__)
    return IS_ADC_CHANNEL(channel);
#else
    #error "Unsupported processor"
#endif

}

/* //adc_wait_for_eoc_or_timeout()
STATIC void adc_wait_for_eoc_or_timeout(int32_t timeout) {
    uint32_t tickstart = HAL_GetTick();
#if defined(STM32F4) || defined(STM32F7)
    while ((ADCx->SR & ADC_FLAG_EOC) != ADC_FLAG_EOC) {
#elif defined(STM32H7) || defined(STM32L4)
    while (READ_BIT(ADCx->ISR, ADC_FLAG_EOC) != ADC_FLAG_EOC) {
#else
    #error Unsupported processor
#endif
        if (((HAL_GetTick() - tickstart ) > timeout)) {
            break; // timeout
        }
    }
}
*/
/* //adcx_clock_enable();
STATIC void adcx_clock_enable(void) {
#if defined(STM32F4) || defined(STM32F7)
    ADCx_CLK_ENABLE();
#elif defined(STM32H7)
    __HAL_RCC_ADC3_CLK_ENABLE();
    __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);
#elif defined(STM32L4)
    __HAL_RCC_ADC_CLK_ENABLE();
#else
    #error Unsupported processor
#endif
}
*/

#if 0 //STM32: origin
STATIC void adcx_init_periph(ADC_HandleTypeDef *adch, uint32_t resolution) {
    adcx_clock_enable();

    adch->Instance                   = ADCx;
    adch->Init.Resolution            = resolution;
    adch->Init.ContinuousConvMode    = DISABLE;
    adch->Init.DiscontinuousConvMode = DISABLE;
    adch->Init.NbrOfDiscConversion   = 0;
    adch->Init.NbrOfConversion       = 1;
    adch->Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    adch->Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    adch->Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    #if defined(STM32F4) || defined(STM32F7)
    adch->Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV2;
    adch->Init.ScanConvMode          = DISABLE;
    adch->Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    adch->Init.DMAContinuousRequests = DISABLE;
    #elif defined(STM32H7)
    adch->Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    adch->Init.BoostMode             = ENABLE;
    adch->Init.ScanConvMode          = DISABLE;
    adch->Init.LowPowerAutoWait      = DISABLE;
    adch->Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    adch->Init.OversamplingMode      = DISABLE;
    adch->Init.LeftBitShift          = ADC_LEFTBITSHIFT_NONE;
    adch->Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    #elif defined(STM32L4)
    adch->Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;
    adch->Init.ScanConvMode          = ADC_SCAN_DISABLE;
    adch->Init.LowPowerAutoWait      = DISABLE;
    adch->Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    adch->Init.OversamplingMode      = DISABLE;
    adch->Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    adch->Init.DMAContinuousRequests = DISABLE;
    #else
    #error Unsupported processor
    #endif

    HAL_ADC_Init(adch);

    #if defined(STM32H7)
    HAL_ADCEx_Calibration_Start(adch, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    #endif
}

STATIC void adc_init_single(pyb_obj_adc_t *adc_obj) {
    if (!is_adcx_channel(adc_obj->channel)) {
        return;
    }

    if (ADC_FIRST_GPIO_CHANNEL <= adc_obj->channel && adc_obj->channel <= ADC_LAST_GPIO_CHANNEL) {
        // Channels 0-16 correspond to real pins. Configure the GPIO pin in ADC mode.
        const pin_obj_t *pin = pin_adc1[adc_obj->channel];
        mp_hal_pin_config(pin, MP_HAL_PIN_MODE_ADC, MP_HAL_PIN_PULL_NONE, 0);
    }

    adcx_init_periph(&adc_obj->handle, ADC_RESOLUTION_12B);
    

#if defined(STM32L4)
    ADC_MultiModeTypeDef multimode;
    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(&adc_obj->handle, &multimode) != HAL_OK)
    {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Can not set multimode on ADC1 channel: %d", adc_obj->channel));
    }
#endif
}

STATIC void adc_config_channel(ADC_HandleTypeDef *adc_handle, uint32_t channel) {
//    ADC_ChannelConfTypeDef sConfig;
//
//    sConfig.Channel = channel;
//    sConfig.Rank = 1;
/*
#if defined(STM32F4) || defined(STM32F7)
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
#elif defined(STM32H7)
    sConfig.SamplingTime = ADC_SAMPLETIME_8CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.OffsetRightShift = DISABLE;
    sConfig.OffsetSignedSaturation = DISABLE;
#elif defined(STM32L4)
    sConfig.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
#else
    #error Unsupported processor
#endif
*/
    sConfig.Offset = 0;

    HAL_ADC_ConfigChannel(adc_handle, &sConfig);
}

STATIC uint32_t adc_read_channel(ADC_HandleTypeDef *adcHandle) {
    HAL_ADC_Start(adcHandle);
    adc_wait_for_eoc_or_timeout(EOC_TIMEOUT);
    uint32_t value = ADCx->DR;
    HAL_ADC_Stop(adcHandle);
    return value;
}

STATIC uint32_t adc_config_and_read_channel(ADC_HandleTypeDef *adcHandle, uint32_t channel) {
    adc_config_channel(adcHandle, channel);
    return adc_read_channel(adcHandle);
}


/// \method read_timed(buf, timer)
///
/// Read analog values into `buf` at a rate set by the `timer` object.
///
/// `buf` can be bytearray or array.array for example.  The ADC values have
/// 12-bit resolution and are stored directly into `buf` if its element size is
/// 16 bits or greater.  If `buf` has only 8-bit elements (eg a bytearray) then
/// the sample resolution will be reduced to 8 bits.
///
/// `timer` should be a Timer object, and a sample is read each time the timer
/// triggers.  The timer must already be initialised and running at the desired
/// sampling frequency.
///
/// To support previous behaviour of this function, `timer` can also be an
/// integer which specifies the frequency (in Hz) to sample at.  In this case
/// Timer(6) will be automatically configured to run at the given frequency.
///
/// Example using a Timer object (preferred way):
///
///     adc = pyb.ADC(pyb.Pin.board.X19)    # create an ADC on pin X19
///     tim = pyb.Timer(6, freq=10)         # create a timer running at 10Hz
///     buf = bytearray(100)                # creat a buffer to store the samples
///     adc.read_timed(buf, tim)            # sample 100 values, taking 10s
///
/// Example using an integer for the frequency:
///
///     adc = pyb.ADC(pyb.Pin.board.X19)    # create an ADC on pin X19
///     buf = bytearray(100)                # create a buffer of 100 bytes
///     adc.read_timed(buf, 10)             # read analog values into buf at 10Hz
///                                         #   this will take 10 seconds to finish
///     for val in buf:                     # loop over all values
///         print(val)                      # print the value out
///
/// This function does not allocate any memory.
STATIC mp_obj_t adc_read_timed(mp_obj_t self_in, mp_obj_t buf_in, mp_obj_t freq_in) {
    pyb_obj_adc_t *self = self_in;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_WRITE);
    size_t typesize = mp_binary_get_size('@', bufinfo.typecode, NULL);

    TIM_HandleTypeDef *tim;
    #if defined(TIM6)
    if (mp_obj_is_integer(freq_in)) {
        // freq in Hz given so init TIM6 (legacy behaviour)
        tim = timer_tim6_init(mp_obj_get_int(freq_in));
        HAL_TIM_Base_Start(tim);
    } else
    #endif
    {
        // use the supplied timer object as the sampling time base
        tim = pyb_timer_get_handle(freq_in);
    }

    // configure the ADC channel
    adc_config_channel(&self->handle, self->channel);

    // This uses the timer in polling mode to do the sampling
    // TODO use DMA

    uint nelems = bufinfo.len / typesize;
    for (uint index = 0; index < nelems; index++) {
        // Wait for the timer to trigger so we sample at the correct frequency
        while (__HAL_TIM_GET_FLAG(tim, TIM_FLAG_UPDATE) == RESET) {
        }
        __HAL_TIM_CLEAR_FLAG(tim, TIM_FLAG_UPDATE);

        if (index == 0) {
            // for the first sample we need to turn the ADC on
            HAL_ADC_Start(&self->handle);
        } else {
            // for subsequent samples we can just set the "start sample" bit
#if defined(STM32F4) || defined(STM32F7)
            ADCx->CR2 |= (uint32_t)ADC_CR2_SWSTART;
#elif defined(STM32H7) || defined(STM32L4)
            SET_BIT(ADCx->CR, ADC_CR_ADSTART);
#else
            #error Unsupported processor
#endif
        }

        // wait for sample to complete
        adc_wait_for_eoc_or_timeout(EOC_TIMEOUT);

        // read value
        uint value = ADCx->DR;

        // store value in buffer
        if (typesize == 1) {
            value >>= 4;
        }
        mp_binary_set_val_array_from_int(bufinfo.typecode, bufinfo.buf, index, value);
    }

    // turn the ADC off
    HAL_ADC_Stop(&self->handle);

    #if defined(TIM6)
    if (mp_obj_is_integer(freq_in)) {
        // stop timer if we initialised TIM6 in this function (legacy behaviour)
        HAL_TIM_Base_Stop(tim);
    }
    #endif

    return mp_obj_new_int(bufinfo.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(adc_read_timed_obj, adc_read_timed);

// read_timed_multi((adcx, adcy, ...), (bufx, bufy, ...), timer)
//
// Read analog values from multiple ADC's into buffers at a rate set by the
// timer.  The ADC values have 12-bit resolution and are stored directly into
// the corresponding buffer if its element size is 16 bits or greater, otherwise
// the sample resolution will be reduced to 8 bits.
//
// This function should not allocate any heap memory.
STATIC mp_obj_t adc_read_timed_multi(mp_obj_t adc_array_in, mp_obj_t buf_array_in, mp_obj_t tim_in) {
    size_t nadcs, nbufs;
    mp_obj_t *adc_array, *buf_array;
    mp_obj_get_array(adc_array_in, &nadcs, &adc_array);
    mp_obj_get_array(buf_array_in, &nbufs, &buf_array);

    if (nadcs < 1) {
        mp_raise_ValueError("need at least 1 ADC");
    }
    if (nadcs != nbufs) {
        mp_raise_ValueError("length of ADC and buffer lists differ");
    }

    // Get buf for first ADC, get word size, check other buffers match in type
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_array[0], &bufinfo, MP_BUFFER_WRITE);
    size_t typesize = mp_binary_get_size('@', bufinfo.typecode, NULL);
    void *bufptrs[nbufs];
    for (uint array_index = 0; array_index < nbufs; array_index++) {
        mp_buffer_info_t bufinfo_curr;
        mp_get_buffer_raise(buf_array[array_index], &bufinfo_curr, MP_BUFFER_WRITE);
        if ((bufinfo.len != bufinfo_curr.len) || (bufinfo.typecode != bufinfo_curr.typecode)) {
            mp_raise_ValueError("size and type of buffers must match");
        }
        bufptrs[array_index] = bufinfo_curr.buf;
    }

    // Use the supplied timer object as the sampling time base
    TIM_HandleTypeDef *tim;
    tim = pyb_timer_get_handle(tim_in);

    // Start adc; this is slow so wait for it to start
    pyb_obj_adc_t *adc0 = adc_array[0];
    adc_config_channel(&adc0->handle, adc0->channel);
    HAL_ADC_Start(&adc0->handle);
    // Wait for sample to complete and discard
    adc_wait_for_eoc_or_timeout(EOC_TIMEOUT);
    // Read (and discard) value
    uint value = ADCx->DR;

    // Ensure first sample is on a timer tick
    __HAL_TIM_CLEAR_FLAG(tim, TIM_FLAG_UPDATE);
    while (__HAL_TIM_GET_FLAG(tim, TIM_FLAG_UPDATE) == RESET) {
    }
    __HAL_TIM_CLEAR_FLAG(tim, TIM_FLAG_UPDATE);

    // Overrun check: assume success
    bool success = true;
    size_t nelems = bufinfo.len / typesize;
    for (size_t elem_index = 0; elem_index < nelems; elem_index++) {
        if (__HAL_TIM_GET_FLAG(tim, TIM_FLAG_UPDATE) != RESET) {
            // Timer has already triggered
            success = false;
        } else {
            // Wait for the timer to trigger so we sample at the correct frequency
            while (__HAL_TIM_GET_FLAG(tim, TIM_FLAG_UPDATE) == RESET) {
            }
        }
        __HAL_TIM_CLEAR_FLAG(tim, TIM_FLAG_UPDATE);

        for (size_t array_index = 0; array_index < nadcs; array_index++) {
            pyb_obj_adc_t *adc = adc_array[array_index];
            // configure the ADC channel
            adc_config_channel(&adc->handle, adc->channel);
            // for the first sample we need to turn the ADC on
            // ADC is started: set the "start sample" bit
            #if defined(STM32F4) || defined(STM32F7)
            ADCx->CR2 |= (uint32_t)ADC_CR2_SWSTART;
            #elif defined(STM32H7) || defined(STM32L4)
            SET_BIT(ADCx->CR, ADC_CR_ADSTART);
            #else
            #error Unsupported processor
            #endif
            // wait for sample to complete
            adc_wait_for_eoc_or_timeout(EOC_TIMEOUT);

            // read value
            value = ADCx->DR;

            // store values in buffer
            if (typesize == 1) {
                value >>= 4;
            }
            mp_binary_set_val_array_from_int(bufinfo.typecode, bufptrs[array_index], elem_index, value);
        }
    }

    // Turn the ADC off
    adc0 = adc_array[0];
    HAL_ADC_Stop(&adc0->handle);

    return mp_obj_new_bool(success);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(adc_read_timed_multi_fun_obj, adc_read_timed_multi);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(adc_read_timed_multi_obj, MP_ROM_PTR(&adc_read_timed_multi_fun_obj));





#endif //STM32 pyb_adc_type


static inline eadc_TurnOnClk(EADC_T *eadc_base)
{
	SYS_UnlockReg();
	
	/* Enable EADC module clock */
    CLK_EnableModuleClock(EADC_MODULE);
	
    /* EADC clock source is 96MHz, set divider to 255, EADC clock is 96/255 MHz */
    CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(255));
	
    EADC_Open(eadc_base, EADC_CTL_DIFFEN_SINGLE_END);
    
	SYS_LockReg();	
}

STATIC void adc_init_single(pyb_obj_adc_t *adc_obj) {
    if (!is_adcx_channel(adc_obj->channel)) {
        return;
    }

    if (EADC_FIRST_GPIO_CHANNEL <= adc_obj->channel && adc_obj->channel <= EADC_LAST_GPIO_CHANNEL) {
        // Channels 0-16 correspond to real pins. Configure the GPIO pin in ADC mode.
        const pin_obj_t *pin = eadc_channel_obj_list[adc_obj->channel].eadc_channel_pin_obj;
        mp_hal_pin_config_alt(pin, MP_HAL_PIN_MODE_ALT_PUSH_PULL, AF_FN_EADC, 0);
        GPIO_SetMode(pin->gpio, 1 << pin->pin, MP_HAL_PIN_MODE_INPUT);  //Set Analog pin as input mode 
        GPIO_DISABLE_DIGITAL_PATH(pin->gpio, 1 << pin->pin);            /* Disable the digital input path to avoid the leakage current. */
    }
    
    eadc_TurnOnClk(adc_obj->eadc_base);
    
    /* Configure the sample module 0 for analog input channel 2 and software trigger source.*/
	EADC_ConfigSampleModule(adc_obj->eadc_base, adc_obj->channel, EADC_SOFTWARE_TRIGGER, adc_obj->channel);
	
	/* Clear the A/D ADINT0 interrupt flag for safe */
	EADC_CLR_INT_FLAG(adc_obj->eadc_base, EADC_STATUS2_ADIF0_Msk);
}


/******************************************************************************/
/* MicroPython bindings : adc object (single channel)                         */

STATIC void adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_obj_adc_t *self = self_in;
    mp_print_str(print, "<ADC on ");
    mp_obj_print_helper(print, self->pin_name, PRINT_STR);
    mp_printf(print, " channel=%u>", self->channel);
}

/// \classmethod \constructor(pin)
/// Create an ADC object associated with the given pin.
/// This allows you to then read analog values on that pin.
STATIC mp_obj_t adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
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
        
        if(channel==0xffffffff){ 
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
    o->base.type = &pyb_adc_type;
    o->pin_name = pin_obj;
    o->channel = channel;
    //o->eadc_base =eadc_channel_obj_list[channel].eadc_channel_pin_obj->af->reg;
    o->eadc_base = EADC;
    //ASSERT(EADC==o->eadc_base);
    adc_init_single(o);

    return o;
}

/// \method read()
/// Read the value on the analog pin and return it.  The returned value
/// will be between 0 and 4095.

STATIC mp_obj_t adc_read(mp_obj_t self_in) {
    pyb_obj_adc_t *self = self_in;
    return mp_obj_new_int(eadc_config_and_read_channel(self->eadc_base, self->channel));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(adc_read_obj, adc_read);


STATIC const mp_rom_map_elem_t adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&adc_read_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_read_timed), MP_ROM_PTR(&adc_read_timed_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_read_timed_multi), MP_ROM_PTR(&adc_read_timed_multi_obj) },
};

STATIC MP_DEFINE_CONST_DICT(adc_locals_dict, adc_locals_dict_table);


const mp_obj_type_t pyb_adc_type = {
    { &mp_type_type },
    .name = MP_QSTR_ADC,
    .print = adc_print,
    .make_new = adc_make_new,
    .locals_dict = (mp_obj_dict_t*)&adc_locals_dict,
};

/******************************************************************************/
/* adc all object                                                             */
void adc_init_all(pyb_adc_all_obj_t *adc_all, uint32_t resolution, uint32_t en_mask) {

    switch (resolution) {
        //case 8:  resolution = ADC_RESOLUTION_8B;  break;
        //case 10: resolution = ADC_RESOLUTION_10B; break;
        case 12: resolution = EADC_RESOLUTION_12B; break;
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
                SYS->IVSCTL |= SYS_IVSCTL_VTEMPEN_Msk; /* Enable temperature sensor for pin 17 */
                SYS_LockReg();
            } else if ((EADC_INTERNAL_CHANNEL_DUMMY==pin)&&(channel == EADC_CHANNEL_VBAT)) {
                SYS_UnlockReg();
                /* Enable VBAT unity gain buffer */
                SYS->IVSCTL |= SYS_IVSCTL_VBATUGEN_Msk;
                SYS_LockReg();
            } else if(channel > EADC_LAST_ADC_CHANNEL){
                nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
                "Paramemter error"));
            } 
        }
    }
}
/* //get resolution()
int adc_get_resolution(ADC_HandleTypeDef *adcHandle) {
    uint32_t res_reg = ADC_GET_RESOLUTION(adcHandle);

    switch (res_reg) {
       #if !defined(STM32H7)
        case ADC_RESOLUTION_6B:  return 6;
        #endif
        case ADC_RESOLUTION_8B:  return 8;
        case ADC_RESOLUTION_10B: return 10;
    }
    return 12;
}
*/

//get core_temp()
int adc_read_core_temp(pyb_adc_all_obj_t *self_in) {
    int32_t raw_value = eadc_config_and_read_channel(self_in->eadc_base, EADC_CHANNEL_TEMPSENSOR);

    // Note: constants assume 12-bit resolution, so we scale the raw value to
    //       be 12-bits.
    //raw_value <<= (12 - adc_get_resolution(self_in));

    //return ((raw_value - CORE_TEMP_V25) / CORE_TEMP_AVG_SLOPE) + 25;
    
    return raw_value; //I cannot find transform function so just return raw data.
}

//adc_read_core_temp_float()
#if MICROPY_PY_BUILTINS_FLOAT
// correction factor for reference value
STATIC volatile float adc_refcor = 1.0f;

float eadc_read_core_temp_float(EADC_T *adcHandle) {
    int32_t raw_value = eadc_config_and_read_channel(adcHandle, EADC_CHANNEL_TEMPSENSOR);

    // constants assume 12-bit resolution so we scale the raw value to 12-bits
    //raw_value <<= (12 - adc_get_resolution(adcHandle));

    //float core_temp_avg_slope = (*ADC_CAL2 - *ADC_CAL1) / 80.0;
    //return (((float)raw_value * adc_refcor - *ADC_CAL1) / core_temp_avg_slope) + 30.0f;
    
    return (float)raw_value; //Cannot get transform function, just return raw data.
}

float eadc_read_core_vbat(EADC_T *adcHandle) {
    uint32_t raw_value = eadc_config_and_read_channel(adcHandle, EADC_CHANNEL_VBAT);

    // Note: constants assume 12-bit resolution, so we scale the raw value to
    //       be 12-bits.
    //raw_value <<= (12 - adc_get_resolution(adcHandle));

    #if defined(STM32F4) || defined(STM32F7)
    // ST docs say that (at least on STM32F42x and STM32F43x), VBATE must
    // be disabled when TSVREFE is enabled for TEMPSENSOR and VREFINT
    // conversions to work.  VBATE is enabled by the above call to read
    // the channel, and here we disable VBATE so a subsequent call for
    // TEMPSENSOR or VREFINT works correctly.
    //ADC->CCR &= ~ADC_CCR_VBATE;
    #endif

    //return raw_value * VBAT_DIV * ADC_SCALE * adc_refcor;
    return raw_value; //raw data is Vbat/4 in M48x
}

float adc_read_core_vref(ADC_HandleTypeDef *adcHandle) {
    
    //uint32_t raw_value = adc_config_and_read_channel(adcHandle, EADC_CHANNEL_VREFINT);
    
    // Note: constants assume 12-bit resolution, so we scale the raw value to
    //       be 12-bits.
    //raw_value <<= (12 - adc_get_resolution(adcHandle));

    // update the reference correction factor
    //adc_refcor = ((float)(*VREFIN_CAL)) / ((float)raw_value);

    //return (*VREFIN_CAL) * ADC_SCALE;
    nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
                "The function is not support\n" ));
    
    return 0;
}
#endif

/******************************************************************************/
/* MicroPython bindings : adc_all object                                      */

STATIC void adc_all_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_adc_all_obj_t *self = self_in;
    mp_print_str(print, "<List Eanbled Channel: \n");
    //printf("==>%d\n", sizeof(eadc_channel_obj_t.eadc_channel_pin_obj.eadc_channel_obj)/sizeof(eadc_channel_obj_t));
    /*TODO from here*/
    //for(uint32_t i=0; i<sizeof(pyb_eadc_all_obj->eadc_channel_obj)/sizeof(pyb_adc_all_obj_t); i++){
    for(uint32_t i=0; i < 18; i++){
        if(eadc_channel_obj_list[i].eadc_channel_pin_obj)
            mp_printf(print, " <channel %u>\n", i); 
    }
   
   
}

STATIC mp_obj_t adc_all_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check number of arguments
    mp_arg_check_num(n_args, n_kw, 1, 2, false);

    // make ADCAll object
    pyb_adc_all_obj_t *o = m_new_obj(pyb_adc_all_obj_t);
    o->eadc_base = EADC0;
    o->base.type = &pyb_adc_all_type;
    
    mp_int_t res = mp_obj_get_int(args[0]);
    //uint32_t en_mask = 0xffffffff; //default all channel open
    if(o==NULL) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
                "Class allocate fail\n" ));
    }
    
    uint32_t en_mask = 0x703CF; //default partial port open
    if (n_args > 1) {
        en_mask =  mp_obj_get_int(args[1]);
    }
    adc_init_all(o, res, en_mask);

    return o;
}

//adc_all_read_channel()
STATIC mp_obj_t adc_all_read_channel(mp_obj_t self_in, mp_obj_t channel) {
    pyb_adc_all_obj_t *self = self_in;
    uint32_t chan = adc_get_internal_channel(mp_obj_get_int(channel));
    uint32_t data = eadc_config_and_read_channel(self->eadc_base, chan);

    return mp_obj_new_int(data);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(adc_all_read_channel_obj, adc_all_read_channel);

 //adc_all_read_core_temp()
STATIC mp_obj_t adc_all_read_core_temp(mp_obj_t self_in) {
    pyb_adc_all_obj_t *self = self_in;
    #if MICROPY_PY_BUILTINS_FLOAT
    float data = eadc_read_core_temp_float(self->eadc_base);
    return mp_obj_new_float(data);
    #else
    int data  = adc_read_core_temp(self);
    return mp_obj_new_int(data);
    #endif
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(adc_all_read_core_temp_obj, adc_all_read_core_temp);

#if MICROPY_PY_BUILTINS_FLOAT
STATIC mp_obj_t adc_all_read_core_vbat(mp_obj_t self_in) {
    pyb_adc_all_obj_t *self = self_in;
    float data = eadc_read_core_vbat(self->eadc_base);
    return mp_obj_new_float(data);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(adc_all_read_core_vbat_obj, adc_all_read_core_vbat);

STATIC mp_obj_t adc_all_read_core_vref(mp_obj_t self_in) {
    pyb_adc_all_obj_t *self = self_in;
    //float data  = adc_read_core_vref(&self->handle);
    return mp_obj_new_float(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(adc_all_read_core_vref_obj, adc_all_read_core_vref);

STATIC mp_obj_t adc_all_read_vref(mp_obj_t self_in) {
    pyb_adc_all_obj_t *self = self_in;
    //adc_read_core_vref(&self->handle);
    //return mp_obj_new_float(3.3 * adc_refcor);
    return mp_obj_new_float(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(adc_all_read_vref_obj, adc_all_read_vref);
#endif

STATIC const mp_rom_map_elem_t adc_all_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_channel), MP_ROM_PTR(&adc_all_read_channel_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_core_temp), MP_ROM_PTR(&adc_all_read_core_temp_obj) },
#if MICROPY_PY_BUILTINS_FLOAT
    { MP_ROM_QSTR(MP_QSTR_read_core_vbat), MP_ROM_PTR(&adc_all_read_core_vbat_obj) },
    
    /* Below functions are not supported on M48X */
    //{ MP_ROM_QSTR(MP_QSTR_read_core_vref), MP_ROM_PTR(&adc_all_read_core_vref_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_read_vref), MP_ROM_PTR(&adc_all_read_vref_obj) },
#endif
};

STATIC MP_DEFINE_CONST_DICT(adc_all_locals_dict, adc_all_locals_dict_table);

const mp_obj_type_t pyb_adc_all_type = {
    { &mp_type_type },
    .name = MP_QSTR_ADCAll,
    .print = adc_all_print,
    .make_new = adc_all_make_new,
    .locals_dict = (mp_obj_dict_t*)&adc_all_locals_dict,
};

#endif // MICROPY_HW_ENABLE_ADC
