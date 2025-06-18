#define MICROPY_HW_BOARD_NAME "NuMaker-M55M1"
#define MICROPY_HW_BOARD_NUMAKER_M55M1

// I2C busses
#define MICROPY_HW_I2C0_SCL (pin_H2)	//CCAP connector
#define MICROPY_HW_I2C0_SDA (pin_H3)	//CCAP connector

#define MICROPY_HW_I2C3_SCL (pin_G0) 	//UNO_SCL
#define MICROPY_HW_I2C3_SDA (pin_G1) 	//UNO_SDA

#define MICROPY_HW_I2C4_SCL (pin_C12) 	//D5
#define MICROPY_HW_I2C4_SDA (pin_C11) 	//D4

//UART
//#define MICROPY_HW_UART1_RXD  (pin_B2) //D0
//#define MICROPY_HW_UART1_TXD  (pin_B3) //D1
//#define MICROPY_HW_UART1_CTS  (pin_A1) //D12
//#define MICROPY_HW_UART1_RTS  (pin_A0) //D11

//#define MICROPY_HW_UART5_RXD  (pin_B4) //D6
//#define MICROPY_HW_UART5_TXD  (pin_B5) //D7
//#define MICROPY_HW_UART5_CTS  (pin_B2) //D0
//#define MICROPY_HW_UART5_RTS  (pin_B3) //D1

#define MICROPY_HW_UART6_RXD  (pin_C11) //D4
#define MICROPY_HW_UART6_TXD  (pin_C12) //D5
#define MICROPY_HW_UART6_CTS  (pin_C9)  //D2
#define MICROPY_HW_UART6_RTS  (pin_C10) //D3


//For LPUART testing
//#define MICROPY_HW_UART10_RXD  (pin_C11) //D4
//#define MICROPY_HW_UART10_TXD  (pin_C12) //D5
//#define MICROPY_HW_UART10_CTS  (pin_C9)  //D2
//#define MICROPY_HW_UART10_RTS  (pin_C10) //D3


// SPI busses
#define MICROPY_HW_SPI0_NSS  (pin_A3)	//D10
#define MICROPY_HW_SPI0_SCK  (pin_A2)	//D13
#define MICROPY_HW_SPI0_MISO (pin_A1)	//D12
#define MICROPY_HW_SPI0_MOSI (pin_A0)	//D11

#define MICROPY_HW_SPI2_NSS  (pin_A11)	//SS
#define MICROPY_HW_SPI2_SCK  (pin_A10)	//CLK
#define MICROPY_HW_SPI2_MISO (pin_A9)	//D4
#define MICROPY_HW_SPI2_MOSI (pin_A8)	//D5

//CAN
#define MICROPY_HW_CANFD0_RXD  (pin_J11) //CANFD_RX(J8)
#define MICROPY_HW_CANFD0_TXD  (pin_J10) //CANFD_TX(J8)

//ADC(using EADC pin to implement ADC class)
#define MICROPY_HW_EADC0_CH0   (pin_B0)	//A4
#define MICROPY_HW_EADC0_CH1   (pin_B1)	//A5
#define MICROPY_HW_EADC0_CH2   (pin_B2)	//D0
#define MICROPY_HW_EADC0_CH3   (pin_B3)	//D1
#define MICROPY_HW_EADC0_CH4   (pin_B4)	//D6
#define MICROPY_HW_EADC0_CH5   (pin_B5)	//D7
#define MICROPY_HW_EADC0_CH6   (pin_B6)	//A0
#define MICROPY_HW_EADC0_CH7   (pin_B7)	//A1
#define MICROPY_HW_EADC0_CH8   (pin_B8)	//A2
#define MICROPY_HW_EADC0_CH9   (pin_B9)	//A3
#define MICROPY_HW_EADC0_CH20  (pin_A8)	//MOSI
#define MICROPY_HW_EADC0_CH21  (pin_A9)	//MISO
#define MICROPY_HW_EADC0_CH22  (pin_A10)	//CLK
#define MICROPY_HW_EADC0_CH23  (pin_A11)	//SS

//DAC
//#define MICROPY_HW_DAC0_OUT   (pin_B12) //DAC0_OUT
//#define MICROPY_HW_DAC1_OUT   (pin_B13) //DAC1_OUT

#if 0
//RTC
#define MICROPY_HW_ENABLE_RTC       (1)


//DAC
//#define MICROPY_HW_DAC0_OUT   (pin_B12) //DAC0_OUT


//RNG (hardware random number generate)
#define MICROPY_HW_ENABLE_RNG       (1)

#endif

//Switch
#define MICROPY_HW_USRSW_PRESSED		(0)
#define MICROPY_HW_USRSW_EXTI_MODE  	(GPIO_INT_FALLING)

#define MICROPY_HW_USRSW_SW0_NAME		"BTN0"
#define MICROPY_HW_USRSW_SW1_NAME		"BTN1"
#define MICROPY_HW_USRSW_SW0_PIN        (pin_I11)
#define MICROPY_HW_USRSW_SW1_PIN        (pin_H1)

// The NuMaker has 3 LEDs
#define MICROPY_HW_LED0_NAME		"LEDR"
#define MICROPY_HW_LED1_NAME		"LEDY"
#define MICROPY_HW_LED2_NAME		"LEDG"

#define MICROPY_HW_LED0             (pin_H4)  // red
#define MICROPY_HW_LED1             (pin_D6)  // yellow
#define MICROPY_HW_LED2             (pin_D5)  // green

#define MICROPY_HW_LED_OFF(pin)     (mp_hal_pin_high(pin))
#define MICROPY_HW_LED_ON(pin)     	(mp_hal_pin_low(pin))

//SD card
#define MICROPY_HW_SD_nCD()			SET_SD0_nCD_PD13()
#define MICROPY_HW_SD_CLK()			SET_SD0_CLK_PE6()
#define MICROPY_HW_SD_CMD()			SET_SD0_CMD_PE7()
#define MICROPY_HW_SD_DAT0()		SET_SD0_DAT0_PE2()
#define MICROPY_HW_SD_DAT1()		SET_SD0_DAT1_PE3()
#define MICROPY_HW_SD_DAT2()		SET_SD0_DAT2_PE4()
#define MICROPY_HW_SD_DAT3()		SET_SD0_DAT3_PE5()

