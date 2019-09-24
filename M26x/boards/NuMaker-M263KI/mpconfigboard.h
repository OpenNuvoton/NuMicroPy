#define MICROPY_HW_BOARD_NAME "NuMaker-M263KI"

// I2C busses
#define MICROPY_HW_I2C0_SCL (pin_C1) //SCL
#define MICROPY_HW_I2C0_SDA (pin_C0) //SDA

#define MICROPY_HW_I2C1_SCL (pin_B1) //A5
#define MICROPY_HW_I2C1_SDA (pin_B0) //A4

#define MICROPY_HW_I2C2_SCL (pin_A1) //D12
#define MICROPY_HW_I2C2_SDA (pin_A0) //D11

// SPI busses
#define MICROPY_HW_SPI0_NSS  (pin_A3) //D10
#define MICROPY_HW_SPI0_SCK  (pin_A2) //D13
#define MICROPY_HW_SPI0_MISO (pin_A1) //D12
#define MICROPY_HW_SPI0_MOSI (pin_A0) //D11

#define MICROPY_HW_SPI1_NSS  (pin_C0) //SDA
#define MICROPY_HW_SPI1_SCK  (pin_C1) //SCL
#define MICROPY_HW_SPI1_MISO (pin_C3) //D4
#define MICROPY_HW_SPI1_MOSI (pin_C2) //D5

//ADC(using EADC pin to implement ADC class) 
#define MICROPY_HW_EADC0_CH0  (pin_B0) //A4
#define MICROPY_HW_EADC0_CH1  (pin_B1) //A5
#define MICROPY_HW_EADC0_CH2  (pin_B2) //D0
#define MICROPY_HW_EADC0_CH3  (pin_B3) //D1
#define MICROPY_HW_EADC0_CH4  (pin_B4) //A3
#define MICROPY_HW_EADC0_CH5  (pin_B5) //A2
#define MICROPY_HW_EADC0_CH6  (pin_B6) //A1
#define MICROPY_HW_EADC0_CH7  (pin_B7) //A0

//RTC
#define MICROPY_HW_ENABLE_RTC       (1)

//UART
#define MICROPY_HW_UART1_RXD  (pin_B2) //D0
#define MICROPY_HW_UART1_TXD  (pin_B3) //D1
#define MICROPY_HW_UART1_CTS  (pin_A1) //D12
#define MICROPY_HW_UART1_RTS  (pin_A0) //D11

#define MICROPY_HW_UART2_RXD  (pin_B0) //A4
#define MICROPY_HW_UART2_TXD  (pin_B1) //A5
#define MICROPY_HW_UART2_CTS  (pin_C2) //D5
#define MICROPY_HW_UART2_RTS  (pin_C3) //D4

#define MICROPY_HW_UART5_RXD  (pin_A4) //D9
#define MICROPY_HW_UART5_TXD  (pin_A5) //D8
#define MICROPY_HW_UART5_CTS  (pin_B2) //D0
#define MICROPY_HW_UART5_RTS  (pin_B3) //D1

//DAC
//#define MICROPY_HW_DAC0_OUT   (pin_B12) //DAC0_OUT

//CAN
#define MICROPY_HW_CAN0_RXD  (pin_A4) //D9
#define MICROPY_HW_CAN0_TXD  (pin_A5) //D8

//USB
#define MICROPY_HW_ENABLE_USBD		(1)

//Switch
#define MICROPY_HW_USRSW_PRESSED		(0)
#define MICROPY_HW_USRSW_EXTI_MODE  	(GPIO_INT_FALLING)

#define MICROPY_HW_USRSW_SW2_NAME		"sw2"
#define MICROPY_HW_USRSW_SW3_NAME		"sw3"
#define MICROPY_HW_USRSW_SW2_PIN        (pin_G15)
#define MICROPY_HW_USRSW_SW3_PIN        (pin_F11)

// The NuMaker has 3 LEDs
#define MICROPY_HW_LED0_NAME		"led0"
//#define MICROPY_HW_LED1_NAME		"led1"
//#define MICROPY_HW_LED2_NAME		"led2"

#define MICROPY_HW_LED0             (pin_B10) // red
//#define MICROPY_HW_LED1             (pin_H1) // yellow
//#define MICROPY_HW_LED2             (pin_H2) // green

#define MICROPY_HW_LED_OFF(pin)     (mp_hal_pin_high(pin))
#define MICROPY_HW_LED_ON(pin)     	(mp_hal_pin_low(pin))

//RNG (hardware random number generate)
#define MICROPY_HW_ENABLE_RNG       (1)

