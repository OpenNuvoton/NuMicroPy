
// I2C busses
#define MICROPY_HW_I2C0_SCL (pin_G0)
#define MICROPY_HW_I2C0_SDA (pin_G1)
#define MICROPY_HW_I2C1_SCL (pin_A3)
#define MICROPY_HW_I2C1_SDA (pin_A2)


// SPI busses
#define MICROPY_HW_SPI0_NSS  (pin_A3) //D10
#define MICROPY_HW_SPI0_SCK  (pin_A2) //D13
#define MICROPY_HW_SPI0_MISO (pin_A1) //D12
#define MICROPY_HW_SPI0_MOSI (pin_A0) //D11

#define MICROPY_HW_SPI3_NSS  (pin_C9) //D2
#define MICROPY_HW_SPI3_SCK  (pin_C10) //D3
#define MICROPY_HW_SPI3_MISO (pin_B9) //A3
#define MICROPY_HW_SPI3_MOSI (pin_B8) //A2

//ADC(using EADC pin to implement ADC class) 
#define MICROPY_HW_EADC0_CH0  (pin_B0) //10 
#define MICROPY_HW_EADC0_CH1  (pin_B1) // 9
#define MICROPY_HW_EADC0_CH2  (pin_B2) // 4
#define MICROPY_HW_EADC0_CH3  (pin_B3) // 3
//#define MICROPY_HW_EADC0_CH4  (pin_B0)//not implement yet
//#define MICROPY_HW_EADC0_CH5  (pin_B0)//not implement yet
#define MICROPY_HW_EADC0_CH6  (pin_B6)//144
#define MICROPY_HW_EADC0_CH7  (pin_B7)//143
#define MICROPY_HW_EADC0_CH8  (pin_B8)//142 //A2
#define MICROPY_HW_EADC0_CH9  (pin_B9)//141 //A3
//#define MICROPY_HW_EADC0_CH10 (pin_B0)//
//#define MICROPY_HW_EADC0_CH11 (pin_B0)//
//#define MICROPY_HW_EADC0_CH12 (pin_B0)//
//#define MICROPY_HW_EADC0_CH13 (pin_B0)//
//#define MICROPY_HW_EADC0_CH14 (pin_B0)//

//RTC
#define MICROPY_HW_ENABLE_RTC       (1)

//UART
#define MICROPY_HW_UART1_RXD  (pin_B2) //D0
#define MICROPY_HW_UART1_TXD  (pin_B3) //D1
#define MICROPY_HW_UART1_CTS  (pin_A1) //D12
#define MICROPY_HW_UART1_RTS  (pin_A0) //D11

#define MICROPY_HW_UART5_RXD  (pin_A4) //D9
#define MICROPY_HW_UART5_TXD  (pin_A5) //D8
#define MICROPY_HW_UART5_CTS  (pin_B2) //D0
#define MICROPY_HW_UART5_RTS  (pin_B3) //D1

//DAC
//#define MICROPY_HW_DAC0_OUT   (pin_B12) //DAC0_OUT

//CAN
#define MICROPY_HW_CAN0_RXD  (pin_A4) //D9
#define MICROPY_HW_CAN0_TXD  (pin_A5) //D8
#define MICROPY_HW_CAN1_RXD  (pin_C9) //D2
#define MICROPY_HW_CAN1_TXD  (pin_C10) //D3

//USB
#define MICROPY_HW_ENABLE_USBD		(1)

//Switch
#define MICROPY_HW_USRSW_PRESSED		(0)
#define MICROPY_HW_USRSW_EXTI_MODE  	(GPIO_INT_FALLING)

#define MICROPY_HW_USRSW_SW2_NAME		"sw2"
#define MICROPY_HW_USRSW_SW3_NAME		"sw3"
#define MICROPY_HW_USRSW_SW2_PIN        (pin_F11)
#define MICROPY_HW_USRSW_SW3_PIN        (pin_G5)

// The NuMaker has 3 LEDs
#define MICROPY_HW_LED0_NAME		"led0"
#define MICROPY_HW_LED1_NAME		"led1"
#define MICROPY_HW_LED2_NAME		"led2"

#define MICROPY_HW_LED0             (pin_H0) // red
#define MICROPY_HW_LED1             (pin_H1) // yellow
#define MICROPY_HW_LED2             (pin_H2) // green

#define MICROPY_HW_LED_OFF(pin)     (mp_hal_pin_high(pin))
#define MICROPY_HW_LED_ON(pin)     	(mp_hal_pin_low(pin))

//RNG (hardware random number generate)
#define MICROPY_HW_ENABLE_RNG       (1)

#define MICROPY_HW_HAS_BMX055		(1)
#define MICROPY_BMX055_SCL			(pin_D1)
#define MICROPY_BMX055_SDA			(pin_D0)


