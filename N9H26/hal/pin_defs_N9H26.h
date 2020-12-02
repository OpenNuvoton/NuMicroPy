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
// This file contains pin definitions that are specific to the m48x port.
// This file should only ever be #included by pin.h and not directly.

#include "wblib.h"
#include "N9H26_reg.h"
#include "N9H26_GPIO.h"
#include "N9H26_MFP.h"

enum {
  PORT_A = GPIO_PORTA,
  PORT_B = GPIO_PORTB,
  PORT_C = GPIO_PORTC,
  PORT_D = GPIO_PORTD,
  PORT_E = GPIO_PORTE,
  PORT_G = GPIO_PORTG,
  PORT_H = GPIO_PORTH,
};

enum {
  AF_FN_ADC,
  AF_FN_SPI,
  AF_FN_EMAC,
  AF_FN_SD,
  AF_FN_SDIO,
  AF_FN_UART,
  AF_FN_HUART,
  AF_FN_I2C,
  AF_FN_PWM,
  AF_FN_LCD,
  AF_FN_VIN,
  AF_FN_UHL,
  AF_FN_NAND,
  AF_FN_KPI,
  AF_FN_I2S,
  AF_FN_WDT,
};

enum {
  AF_PIN_TYPE_ADC_AIN1 = 0,
  AF_PIN_TYPE_ADC_AIN2,
  AF_PIN_TYPE_ADC_AIN3,

  AF_PIN_TYPE_SPI_DO = 0,
  AF_PIN_TYPE_SPI_DI,
  AF_PIN_TYPE_SPI_CLK,
  AF_PIN_TYPE_SPI_CS0,
  AF_PIN_TYPE_SPI_CS1,
  AF_PIN_TYPE_SPI_D2,
  AF_PIN_TYPE_SPI_D3,

  AF_PIN_TYPE_EMAC_REFCLK = 0,
  AF_PIN_TYPE_EMAC_RXERR,
  AF_PIN_TYPE_EMAC_CRSDV,
  AF_PIN_TYPE_EMAC_PPS,
  AF_PIN_TYPE_EMAC_RXD0,
  AF_PIN_TYPE_EMAC_RXD1,
  AF_PIN_TYPE_EMAC_TXEN,
  AF_PIN_TYPE_EMAC_TXD0,
  AF_PIN_TYPE_EMAC_TXD1,
  AF_PIN_TYPE_EMAC_MDC,
  AF_PIN_TYPE_EMAC_MDIO,

  AF_PIN_TYPE_SD_DATA0 = 0,
  AF_PIN_TYPE_SD_DATA1,
  AF_PIN_TYPE_SD_DATA2,
  AF_PIN_TYPE_SD_DATA3,
  AF_PIN_TYPE_SD_CLK,
  AF_PIN_TYPE_SD_CMD,
  AF_PIN_TYPE_SD_CD,

  AF_PIN_TYPE_SDIO_D0 = 0,
  AF_PIN_TYPE_SDIO_D1,
  AF_PIN_TYPE_SDIO_D2,
  AF_PIN_TYPE_SDIO_D3,
  AF_PIN_TYPE_SDIO_CMD,
  AF_PIN_TYPE_SDIO_CD,
  
  AF_PIN_TYPE_UART_RXD = 0,
  AF_PIN_TYPE_UART_RTS,
  AF_PIN_TYPE_UART_TXD,
  AF_PIN_TYPE_UART_CTS,
   
  AF_PIN_TYPE_HUART_RXD = 0,
  AF_PIN_TYPE_HUART_RTS,
  AF_PIN_TYPE_HUART_TXD,
  AF_PIN_TYPE_HUART_CTS,

  AF_PIN_TYPE_I2C_SDA = 0,
  AF_PIN_TYPE_I2C_SCL,

  AF_PIN_TYPE_PWM_CH0 = 0,
  AF_PIN_TYPE_PWM_CH1,
  AF_PIN_TYPE_PWM_CH2,
  AF_PIN_TYPE_PWM_CH3,
 
  AF_PIN_TYPE_LCD_LMVSYNC = 0,
  AF_PIN_TYPE_LCD_LPCLK,
  AF_PIN_TYPE_LCD_LHSYNC,
  AF_PIN_TYPE_LCD_LVSYNC,
  AF_PIN_TYPE_LCD_LVDEN,
  AF_PIN_TYPE_LCD_LVDATA0,
  AF_PIN_TYPE_LCD_LVDATA1,
  AF_PIN_TYPE_LCD_LVDATA2,
  AF_PIN_TYPE_LCD_LVDATA3,
  AF_PIN_TYPE_LCD_LVDATA4,
  AF_PIN_TYPE_LCD_LVDATA5,
  AF_PIN_TYPE_LCD_LVDATA6,
  AF_PIN_TYPE_LCD_LVDATA7,
  AF_PIN_TYPE_LCD_LVDATA8,
  AF_PIN_TYPE_LCD_LVDATA9,
  AF_PIN_TYPE_LCD_LVDATA10,
  AF_PIN_TYPE_LCD_LVDATA11,
  AF_PIN_TYPE_LCD_LVDATA12,
  AF_PIN_TYPE_LCD_LVDATA13,
  AF_PIN_TYPE_LCD_LVDATA14,
  AF_PIN_TYPE_LCD_LVDATA15,
  AF_PIN_TYPE_LCD_LVDATA16,
  AF_PIN_TYPE_LCD_LVDATA17,
  AF_PIN_TYPE_LCD_LVDATA18,
  AF_PIN_TYPE_LCD_LVDATA19,
  AF_PIN_TYPE_LCD_LVDATA20,
  AF_PIN_TYPE_LCD_LVDATA21,
  AF_PIN_TYPE_LCD_LVDATA22,
  AF_PIN_TYPE_LCD_LVDATA23,

  AF_PIN_TYPE_VIN_CLKO = 0,
  AF_PIN_TYPE_VIN_VSYNC,
  AF_PIN_TYPE_VIN_HSYNC,
  AF_PIN_TYPE_VIN_FILED,
  AF_PIN_TYPE_VIN_PCLK,
  AF_PIN_TYPE_VIN_PDATA0,
  AF_PIN_TYPE_VIN_PDATA1,
  AF_PIN_TYPE_VIN_PDATA2,
  AF_PIN_TYPE_VIN_PDATA3,
  AF_PIN_TYPE_VIN_PDATA4,
  AF_PIN_TYPE_VIN_PDATA5,
  AF_PIN_TYPE_VIN_PDATA6,
  AF_PIN_TYPE_VIN_PDATA7,

  AF_PIN_TYPE_UHL_DM = 0,
  AF_PIN_TYPE_UHL_DP,

  AF_PIN_TYPE_NAND_CS0 = 0,
  AF_PIN_TYPE_NAND_CS1,
  AF_PIN_TYPE_NAND_ALE,
  AF_PIN_TYPE_NAND_CLE,
  AF_PIN_TYPE_NAND_BUSY0,
  AF_PIN_TYPE_NAND_BUSY1,
  AF_PIN_TYPE_NAND_RE,
  AF_PIN_TYPE_NAND_WR,
  AF_PIN_TYPE_NAND_DATA0,
  AF_PIN_TYPE_NAND_DATA1,
  AF_PIN_TYPE_NAND_DATA2,
  AF_PIN_TYPE_NAND_DATA3,
  AF_PIN_TYPE_NAND_DATA4,
  AF_PIN_TYPE_NAND_DATA5,
  AF_PIN_TYPE_NAND_DATA6,
  AF_PIN_TYPE_NAND_DATA7,

  AF_PIN_TYPE_KPI_SO0 = 0,
  AF_PIN_TYPE_KPI_SO1,
  AF_PIN_TYPE_KPI_SO2,
  AF_PIN_TYPE_KPI_SO3,
  AF_PIN_TYPE_KPI_SO4,
  AF_PIN_TYPE_KPI_SO5,
  AF_PIN_TYPE_KPI_SO6,
  AF_PIN_TYPE_KPI_SO7,
  AF_PIN_TYPE_KPI_SO8or0,
  AF_PIN_TYPE_KPI_SO9or1,
  AF_PIN_TYPE_KPI_SO10or2,
  AF_PIN_TYPE_KPI_SO11or3,
  AF_PIN_TYPE_KPI_SO12or4,
  AF_PIN_TYPE_KPI_SO13or5,
  AF_PIN_TYPE_KPI_SO14or6,
  AF_PIN_TYPE_KPI_SO15or7,
  AF_PIN_TYPE_KPI_SI0,
  AF_PIN_TYPE_KPI_SI1,
  AF_PIN_TYPE_KPI_SI2,
  AF_PIN_TYPE_KPI_SI3,

  AF_PIN_TYPE_I2S_BCLK = 0,
  AF_PIN_TYPE_I2S_MCLK,
  AF_PIN_TYPE_I2S_WS,
  AF_PIN_TYPE_I2S_DIN,
  AF_PIN_TYPE_I2S_DOUT,

  AF_PIN_TYPE_WDT_RST = 0,
};

/*---------------------------------------------------------------------------------------------------------*/
/*  GPIO_MODE Constant Definitions                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define GPIO_MODE_INPUT          0x0UL /*!< Input Mode \hideinitializer */
#define GPIO_MODE_OUTPUT         0x1UL /*!< Output Mode \hideinitializer */
#define GPIO_MODE_AF 	 	     0x2UL /*!< Alternate function Mode \hideinitializer */

/*---------------------------------------------------------------------------------------------------------*/
/*  GPIO Interrupt Type Constant Definitions                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define GPIO_INT_RISING         0x00010000UL /*!< Interrupt enable by Input Rising Edge \hideinitializer */
#define GPIO_INT_FALLING        0x00000001UL /*!< Interrupt enable by Input Falling Edge \hideinitializer */
#define GPIO_INT_BOTH_EDGE      0x00010001UL /*!< Interrupt enable by both Rising Edge and Falling Edge \hideinitializer */

/*---------------------------------------------------------------------------------------------------------*/
/*  GPIO Pull-up And Pull-down Type Constant Definitions                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define GPIO_PUSEL_DISABLE          0x0UL           /*!< GPIO PUSEL setting for Disable Mode \hideinitializer */
#define GPIO_PUSEL_PULL_UP          0x1UL           /*!< GPIO PUSEL setting for Pull-up Mode \hideinitializer */
#define GPIO_PUSEL_PULL_DOWN        0x2UL           /*!< GPIO PUSEL setting for Pull-down Mode \hideinitializer */

#define IS_GPIO_MODE(MODE) (((MODE) == GPIO_MODE_INPUT)              ||\
                            ((MODE) == GPIO_MODE_OUTPUT)          	 ||\
                            ((MODE) == GPIO_MODE_AF))

#define IS_GPIO_PULL(PULL) (((PULL) == GPIO_PUSEL_DISABLE) || ((PULL) == GPIO_PUSEL_PULL_UP) || \
                            ((PULL) == GPIO_PUSEL_PULL_DOWN))

#define IS_GPIO_INT(ATTR) (((ATTR) == GPIO_INT_RISING)              ||\
                            ((ATTR) == GPIO_INT_FALLING)          	 ||\
                            ((ATTR) == GPIO_INT_BOTH_EDGE))


