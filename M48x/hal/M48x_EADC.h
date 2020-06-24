/**************************************************************************//**
 * @file     M48x_EADC.h
 * @version  V1.00
 * @brief    M480 EADC HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M48X_EADC_H__
#define __M48X_EADC_H__


/* ADC defintions */

typedef struct
{
  uint32_t ClockPrescaler;        /*!< Select ADC clock prescaler. The clock is common for
                                       all the ADCs.
                                       This parameter can be a value of @ref ADC_ClockPrescaler */
  uint32_t Resolution;            /*!< Configures the ADC resolution.
                                       This parameter can be a value of @ref ADC_Resolution */
  uint32_t DataAlign;             /*!< Specifies ADC data alignment to right (MSB on register bit 11 and LSB on register bit 0) (default setting)
                                       or to left (if regular group: MSB on register bit 15 and LSB on register bit 4, if injected group (MSB kept as signed value due to potential negative value after offset application): MSB on register bit 14 and LSB on register bit 3).
                                       This parameter can be a value of @ref ADC_Data_Align */
  uint32_t ScanConvMode;          /*!< Configures the sequencer of regular and injected groups.
                                       This parameter can be associated to parameter 'DiscontinuousConvMode' to have main sequence subdivided in successive parts.
                                       If disabled: Conversion is performed in single mode (one channel converted, the one defined in rank 1).
                                                    Parameters 'NbrOfConversion' and 'InjectedNbrOfConversion' are discarded (equivalent to set to 1).
                                       If enabled:  Conversions are performed in sequence mode (multiple ranks defined by 'NbrOfConversion'/'InjectedNbrOfConversion' and each channel rank).
                                                    Scan direction is upward: from rank1 to rank 'n'.
                                       This parameter can be a value of @ref ADC_Scan_mode.
                                       This parameter can be set to ENABLE or DISABLE */
  uint32_t EOCSelection;          /*!< Specifies what EOC (End Of Conversion) flag is used for conversion by polling and interruption: end of conversion of each rank or complete sequence.
                                       This parameter can be a value of @ref ADC_EOCSelection.
                                       Note: For injected group, end of conversion (flag&IT) is raised only at the end of the sequence.
                                             Therefore, if end of conversion is set to end of each conversion, injected group should not be used with interruption (HAL_ADCEx_InjectedStart_IT)
                                             or polling (HAL_ADCEx_InjectedStart and HAL_ADCEx_InjectedPollForConversion). By the way, polling is still possible since driver will use an estimated timing for end of injected conversion.
                                       Note: If overrun feature is intended to be used, use ADC in mode 'interruption' (function HAL_ADC_Start_IT() ) with parameter EOCSelection set to end of each conversion or in mode 'transfer by DMA' (function HAL_ADC_Start_DMA()).
                                             If overrun feature is intended to be bypassed, use ADC in mode 'polling' or 'interruption' with parameter EOCSelection must be set to end of sequence */
  uint32_t ContinuousConvMode;    /*!< Specifies whether the conversion is performed in single mode (one conversion) or continuous mode for regular group,
                                       after the selected trigger occurred (software start or external trigger).
                                       This parameter can be set to ENABLE or DISABLE. */
  uint32_t NbrOfConversion;       /*!< Specifies the number of ranks that will be converted within the regular group sequencer.
                                       To use regular group sequencer and convert several ranks, parameter 'ScanConvMode' must be enabled.
                                       This parameter must be a number between Min_Data = 1 and Max_Data = 16. */
  uint32_t DiscontinuousConvMode; /*!< Specifies whether the conversions sequence of regular group is performed in Complete-sequence/Discontinuous-sequence (main sequence subdivided in successive parts).
                                       Discontinuous mode is used only if sequencer is enabled (parameter 'ScanConvMode'). If sequencer is disabled, this parameter is discarded.
                                       Discontinuous mode can be enabled only if continuous mode is disabled. If continuous mode is enabled, this parameter setting is discarded.
                                       This parameter can be set to ENABLE or DISABLE. */
  uint32_t NbrOfDiscConversion;   /*!< Specifies the number of discontinuous conversions in which the  main sequence of regular group (parameter NbrOfConversion) will be subdivided.
                                       If parameter 'DiscontinuousConvMode' is disabled, this parameter is discarded.
                                       This parameter must be a number between Min_Data = 1 and Max_Data = 8. */
  uint32_t ExternalTrigConv;      /*!< Selects the external event used to trigger the conversion start of regular group.
                                       If set to ADC_SOFTWARE_START, external triggers are disabled.
                                       If set to external trigger source, triggering is on event rising edge by default.
                                       This parameter can be a value of @ref ADC_External_trigger_Source_Regular */
  uint32_t ExternalTrigConvEdge;  /*!< Selects the external trigger edge of regular group.
                                       If trigger is set to ADC_SOFTWARE_START, this parameter is discarded.
                                       This parameter can be a value of @ref ADC_External_trigger_edge_Regular */
  uint32_t DMAContinuousRequests; /*!< Specifies whether the DMA requests are performed in one shot mode (DMA transfer stop when number of conversions is reached)
                                       or in Continuous mode (DMA transfer unlimited, whatever number of conversions).
                                       Note: In continuous mode, DMA must be configured in circular mode. Otherwise an overrun will be triggered when DMA buffer maximum pointer is reached.
                                       Note: This parameter must be modified when no conversion is on going on both regular and injected groups (ADC disabled, or ADC enabled without continuous mode or external trigger that could launch a conversion).
                                       This parameter can be set to ENABLE or DISABLE. */
}ADC_InitTypeDef;

/**
  * @brief  ADC handle Structure definition
  */
typedef struct
{
  EADC_T                   *Instance;                   /*!< Register base address */

  //ADC_InitTypeDef               Init;                        /*!< ADC required parameters */

  //__IO uint32_t                 NbrOfCurrentConversionRank;  /*!< ADC number of current conversion rank */

  //DMA_HandleTypeDef             *DMA_Handle;                 /*!< Pointer DMA Handler */

  //HAL_LockTypeDef               Lock;                        /*!< ADC locking object */

  //__IO uint32_t                 State;                       /*!< ADC communication state */

  //__IO uint32_t                 ErrorCode;                   /*!< ADC Error code */
}ADC_HandleTypeDef;

typedef struct _pyb_obj_adc_t {
    mp_obj_base_t base;
    mp_obj_t pin_name;
    int channel;
    EADC_T* eadc_base;
} pyb_obj_adc_t;


/*ADC PIN def*/
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

/*internal sample module related definition*/
#define EADC_CHANNEL_BANDGAP      EADC_CHANNEL_16
#define EADC_CHANNEL_TEMPSENSOR   EADC_CHANNEL_17
#define EADC_CHANNEL_VBAT         EADC_CHANNEL_18
#define EADC_INTERNAL_CHANNEL_DUMMY 0xFFFFFFFF    


#define EADC_RESOLUTION_12B (12UL)
#define EADC_FIRST_GPIO_CHANNEL  (0)
#define EADC_LAST_GPIO_CHANNEL   (15)
#define EADC_LAST_ADC_CHANNEL    (15+3)    


#define IS_ADC_CHANNEL(ch) (((ch) <= 16UL)  || \
                            ((ch) == EADC_CHANNEL_TEMPSENSOR)||\
                            ((ch) == EADC_CHANNEL_BANDGAP)||\
                            ((ch) == EADC_CHANNEL_VBAT)\
                           )
/*
#define IS_ADC_CHANNEL(__HANDLE__, __CHANNEL__) ((((__HANDLE__)) == EADC0)  && \
                                                (((__CHANNEL__) == EADC_CHANNEL_VREFINT)     || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_1)           || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_2)           || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_3)           || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_4)           || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_5)           || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_6)           || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_7)           || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_8)           || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_9)           || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_10)          || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_11)          || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_12)          || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_13)          || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_14)          || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_15)          || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_16)          || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_17)          || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_18)          || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_TEMPSENSOR)  || \
                                                 ((__CHANNEL__) == EADC_CHANNEL_VBAT)))
*/

#endif //__M48X_EADC_H__
