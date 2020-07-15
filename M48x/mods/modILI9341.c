//////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////


#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#if MICROPY_LVGL

#include "mpconfigboard.h"

#include "NuMicro.h"
#include "lvgl/lvgl.h"
#include "driver/include/common.h"

#include "hal/M48x_SPI.h"

typedef struct _ILI9341_obj_t {
    mp_obj_base_t base;
    bool initialized;
	int32_t i32Width;
	spi_t sSPIObj;
} ILI9341_obj_t;


#if defined (MICROPY_HW_BOARD_NUMAKER_PFM_M487)

#define _ILI9341_USING_EBI_

//LCD RS pin
#define ILI9341_RS_PIN			PH3
#define ILI9341_RS_PIN_PORT		PH
#define ILI9341_RS_PIN_BIT		BIT3

//LCD RESET pin
#define ILI9341_RST_PIN			PB6
#define ILI9341_RST_PIN_PORT	PB
#define ILI9341_RST_PIN_BIT	BIT6

//LCD backlight pin
#define ILI9341_BL_PIN			PB7
#define ILI9341_BL_PIN_PORT		PB
#define ILI9341_BL_PIN_BIT		BIT7

#endif

#if defined (MICROPY_HW_BOARD_NUMAKER_IOT_M487)

#define _ILI9341_USING_SPI_

//LCD RS pin
#define ILI9341_RS_PIN			PB2
#define ILI9341_RS_PIN_PORT		PB
#define ILI9341_RS_PIN_BIT		BIT2

//LCD RESET pin
#define ILI9341_RST_PIN			PB3
#define ILI9341_RST_PIN_PORT	PB
#define ILI9341_RST_PIN_BIT		BIT3

//LCD backlight pin
#define ILI9341_BL_PIN			PE5
#define ILI9341_BL_PIN_PORT		PE
#define ILI9341_BL_PIN_BIT		BIT5

//LCD SPI pin
#define ILI9341_SPI_SS			MICROPY_HW_SPI2_NSS
#define ILI9341_SPI_CLK			MICROPY_HW_SPI2_SCK
#define ILI9341_SPI_MISO		MICROPY_HW_SPI2_MISO
#define ILI9341_SPI_MOSI		MICROPY_HW_SPI2_MOSI
#endif

/* LCD Module "RESET" */
#define SET_RST ILI9341_RST_PIN = 1;
#define CLR_RST ILI9341_RST_PIN = 0;

/* LCD Module "RS" */
#define SET_RS  ILI9341_RS_PIN = 1;
#define CLR_RS  ILI9341_RS_PIN = 0;

/*-----------------------------------------------*/
// Write control registers of LCD module  
// 
/*-----------------------------------------------*/
static void LCD_WR_REG(
	ILI9341_obj_t *self,
	uint16_t cmd
)
{
    CLR_RS
#if defined(_ILI9341_USING_EBI_)
    EBI0_WRITE_DATA16(0x00000000, cmd);
#endif
#if defined(_ILI9341_USING_SPI_)
	SPI_SetDataWidth(&self->sSPIObj, 8);
	SPI_SET_SS_LOW(self->sSPIObj.spi);
	SPI_MasterBlockWriteRead(&self->sSPIObj, (const char *)&cmd, 1, NULL, 0, 0);
	while(!(SPI_GET_TX_FIFO_EMPTY_FLAG(self->sSPIObj.spi)));
	SPI_SET_SS_HIGH(self->sSPIObj.spi);
#endif
    SET_RS

}

/*-----------------------------------------------*/
// Write data to SRAM of LCD module  
// 
/*-----------------------------------------------*/
static void LCD_WR_DATA(
	ILI9341_obj_t *self,
	uint16_t dat
)
{
#if defined(_ILI9341_USING_EBI_)
    EBI0_WRITE_DATA16(0x00030000, dat);
#endif
#if defined(_ILI9341_USING_SPI_)
	SPI_SetDataWidth(&self->sSPIObj, 8);
	SPI_SET_SS_LOW(self->sSPIObj.spi);
	SPI_MasterBlockWriteRead(&self->sSPIObj, (const char *)&dat, 1, NULL, 0, 0);
	while(!(SPI_GET_TX_FIFO_EMPTY_FLAG(self->sSPIObj.spi)));
	SPI_SET_SS_HIGH(self->sSPIObj.spi);
#endif
}

/*-----------------------------------------------*/
// Read data from SRAM of LCD module 
// 
/*-----------------------------------------------*/

#if defined(_ILI9341_USING_EBI_)
static uint16_t LCD_EBI_RD_DATA(
	ILI9341_obj_t *self
)
{
    return EBI0_READ_DATA16(0x00030000);
}
#endif

#if defined(_ILI9341_USING_SPI_)

static void LCD_SPI_RD_REG(
	ILI9341_obj_t *self,
	uint16_t cmd
)
{
    CLR_RS
	SPI_SetDataWidth(&self->sSPIObj, 8);
	SPI_SET_SS_LOW(self->sSPIObj.spi);
	SPI_MasterBlockWriteRead(&self->sSPIObj, (const char *)&cmd, 1, NULL, 0, 0);
    SET_RS

}

static uint16_t LCD_SPI_RD_DATA(
	ILI9341_obj_t *self
)
{
	uint16_t u16Data = 0;

	SPI_SetDataWidth(&self->sSPIObj, 8);
	SPI_MasterBlockWriteRead(&self->sSPIObj, NULL, 0, (char *)&u16Data, 1, 0);
	while(!(SPI_GET_TX_FIFO_EMPTY_FLAG(self->sSPIObj.spi)));
	SPI_SET_SS_HIGH(self->sSPIObj.spi);
	return u16Data;
}

static void LCD_SPI_WR_DATA16(
	ILI9341_obj_t *self,
	uint16_t u16Data
)
{
	SPI_SetDataWidth(&self->sSPIObj, 16);
	SPI_MasterBlockWriteRead(&self->sSPIObj, (char *)&u16Data, 2, NULL, 0, 0);
	while(!(SPI_GET_TX_FIFO_EMPTY_FLAG(self->sSPIObj.spi)));
}

uint16_t ILI9341_ReadRegister(
	ILI9341_obj_t *self,
	uint8_t Addr, 
	uint8_t xParameter
)
{
    uint16_t data=0;

	LCD_WR_REG(self, 0xD9);
	LCD_WR_DATA(self, 0x10 + xParameter);
	LCD_SPI_RD_REG(self, Addr);
	data = LCD_SPI_RD_DATA(self);

	return data;
}
#endif

/********************************************************************
*
*       LcdWriteDataMultiple
*
*   Function description:
*   Writes multiple values to a display register.
*/
static void LcdWriteDataMultiple(
	ILI9341_obj_t *self,
	uint16_t * pData, 
	int NumItems
)
{
#if defined(_ILI9341_USING_EBI_)
    while (NumItems--) {
        EBI0_WRITE_DATA16(0x00030000,*pData++);
    }
#endif

#if defined(_ILI9341_USING_SPI_)
	
	SPI_SET_SS_LOW(self->sSPIObj.spi);

	SPI_SetDataWidth(&self->sSPIObj, 16);
	SPI_MasterBlockWriteRead(&self->sSPIObj, (char *)pData, (NumItems * sizeof(uint16_t)), NULL, 0, 0);
	while(!(SPI_GET_TX_FIFO_EMPTY_FLAG(self->sSPIObj.spi)));

	SPI_SET_SS_HIGH(self->sSPIObj.spi);

#endif
}

#if defined(_ILI9341_USING_EBI_)

static void EBI_FuncPinInit(void)
{
/*=== EBI (LCD module) mult-function pins ===*/ 	
    /* EBI AD0~5 pins on PG.9~14 */
    SYS->GPG_MFPH &= ~(SYS_GPG_MFPH_PG9MFP_Msk  | SYS_GPG_MFPH_PG10MFP_Msk |
                       SYS_GPG_MFPH_PG11MFP_Msk | SYS_GPG_MFPH_PG12MFP_Msk |
                       SYS_GPG_MFPH_PG13MFP_Msk | SYS_GPG_MFPH_PG14MFP_Msk);
    SYS->GPG_MFPH |= (SYS_GPG_MFPH_PG9MFP_EBI_AD0  | SYS_GPG_MFPH_PG10MFP_EBI_AD1 |
                      SYS_GPG_MFPH_PG11MFP_EBI_AD2 | SYS_GPG_MFPH_PG12MFP_EBI_AD3 |
                      SYS_GPG_MFPH_PG13MFP_EBI_AD4 | SYS_GPG_MFPH_PG14MFP_EBI_AD5);

    /* EBI AD6, AD7 pins on PD.8, PD.9 */
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD8MFP_Msk | SYS_GPD_MFPH_PD9MFP_Msk);
    SYS->GPD_MFPH |= (SYS_GPD_MFPH_PD8MFP_EBI_AD6 | SYS_GPD_MFPH_PD9MFP_EBI_AD7);

    /* EBI AD8, AD9 pins on PE.14, PE.15 */
    SYS->GPE_MFPH &= ~(SYS_GPE_MFPH_PE14MFP_Msk | SYS_GPE_MFPH_PE15MFP_Msk);
    SYS->GPE_MFPH |= (SYS_GPE_MFPH_PE14MFP_EBI_AD8 | SYS_GPE_MFPH_PE15MFP_EBI_AD9);

    /* EBI AD10, AD11 pins on PE.1, PE.0 */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE1MFP_Msk | SYS_GPE_MFPL_PE0MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE1MFP_EBI_AD10 | SYS_GPE_MFPL_PE0MFP_EBI_AD11);

    /* EBI AD12~15 pins on PH.8~11 */
    SYS->GPH_MFPH &= ~(SYS_GPH_MFPH_PH8MFP_Msk  | SYS_GPH_MFPH_PH9MFP_Msk |
    SYS_GPH_MFPH_PH10MFP_Msk | SYS_GPH_MFPH_PH11MFP_Msk);
    SYS->GPH_MFPH |= (SYS_GPH_MFPH_PH8MFP_EBI_AD12  | SYS_GPH_MFPH_PH9MFP_EBI_AD13 |
                      SYS_GPH_MFPH_PH10MFP_EBI_AD14 | SYS_GPH_MFPH_PH11MFP_EBI_AD15);

    /* Configure PH.3 as Output mode for LCD_RS (use GPIO PH.3 to control LCD_RS) */
    GPIO_SetMode(ILI9341_RS_PIN_PORT, ILI9341_RS_PIN_BIT, GPIO_MODE_OUTPUT);
    ILI9341_RS_PIN = 1;

    /* EBI RD and WR pins on PE.4 and PE.5 */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE4MFP_Msk | SYS_GPE_MFPL_PE5MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE4MFP_EBI_nWR | SYS_GPE_MFPL_PE5MFP_EBI_nRD);

    /* EBI CS0 pin on PD.14 */
    SYS->GPD_MFPH &= ~SYS_GPD_MFPH_PD14MFP_Msk;
    SYS->GPD_MFPH |= SYS_GPD_MFPH_PD14MFP_EBI_nCS0;

    /* Configure PB.6 and PB.7 as Output mode for LCD_RST and LCD_Backlight */
    GPIO_SetMode(ILI9341_RST_PIN_PORT, ILI9341_RST_PIN_BIT, GPIO_MODE_OUTPUT);
    ILI9341_RST_PIN = 1;

    GPIO_SetMode(ILI9341_BL_PIN_PORT, ILI9341_BL_PIN_BIT, GPIO_MODE_OUTPUT);
    ILI9341_BL_PIN = 0;
    
	//FIXME: check pin usage for PH5,6,7
    GPIO_SetMode(PH, BIT5, GPIO_MODE_INPUT);
    GPIO_SetMode(PH, BIT6, GPIO_MODE_INPUT);
    GPIO_SetMode(PH, BIT7, GPIO_MODE_INPUT);
}
#endif

#if defined(_ILI9341_USING_SPI_)

static int32_t SPI_FuncPinInit(
	ILI9341_obj_t *self
)
{
	SPI_InitTypeDef sSPIInit;
	
	//check spi pin
	const pin_af_obj_t *af_spi_ss = pin_find_af_by_fn_type(ILI9341_SPI_SS, AF_FN_SPI, AF_PIN_TYPE_SPI_SS);
	const pin_af_obj_t *af_spi_clk = pin_find_af_by_fn_type(ILI9341_SPI_CLK, AF_FN_SPI, AF_PIN_TYPE_SPI_CLK);
	const pin_af_obj_t *af_spi_miso = pin_find_af_by_fn_type(ILI9341_SPI_MISO, AF_FN_SPI, AF_PIN_TYPE_SPI_MISO);
	const pin_af_obj_t *af_spi_mosi = pin_find_af_by_fn_type(ILI9341_SPI_MOSI, AF_FN_SPI, AF_PIN_TYPE_SPI_MOSI);

	if((af_spi_ss == NULL) || (af_spi_clk == NULL) || (af_spi_miso == NULL) ||(af_spi_mosi == NULL)){
       nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "ILI9341 SPI pins"));
	}

	self->sSPIObj.spi = (SPI_T *)af_spi_clk->reg;
	
	uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;
	mp_hal_pin_config_alt(ILI9341_SPI_SS, mode, AF_FN_SPI, af_spi_ss->unit);
	mp_hal_pin_config_alt(ILI9341_SPI_CLK, mode, AF_FN_SPI, af_spi_clk->unit);
	mp_hal_pin_config_alt(ILI9341_SPI_MISO, mode, AF_FN_SPI, af_spi_miso->unit);
	mp_hal_pin_config_alt(ILI9341_SPI_MOSI, mode, AF_FN_SPI, af_spi_mosi->unit);
	
	sSPIInit.Mode =	SPI_MASTER;
	sSPIInit.BaudRate =	36000000;	//over 40M is not work
	sSPIInit.ClockPolarity =	0;
	sSPIInit.Direction = SPI_DIRECTION_2LINES;
	sSPIInit.Bits = 8;
	sSPIInit.FirstBit = SPI_FIRSTBIT_MSB;
	sSPIInit.ClockPhase = SPI_PHASE_1EDGE;

	SPI_Init(&self->sSPIObj, &sSPIInit);

	SPI_DisableAutoSS(self->sSPIObj.spi);

    /* Configure PB.2 as Output mode for LCD_RS (use GPIO PB.2 to control LCD_RS) */
    GPIO_SetMode(ILI9341_RS_PIN_PORT, ILI9341_RS_PIN_BIT, GPIO_MODE_OUTPUT);
    ILI9341_RS_PIN = 1;

    /* Configure PB.3 and PE.5 as Output mode for LCD_RST and LCD_Backlight */
    GPIO_SetMode(ILI9341_RST_PIN_PORT, ILI9341_RST_PIN_BIT, GPIO_MODE_OUTPUT);
    ILI9341_RST_PIN = 1;

    GPIO_SetMode(ILI9341_BL_PIN_PORT, ILI9341_BL_PIN_BIT, GPIO_MODE_OUTPUT);
    ILI9341_BL_PIN = 0;

	return 0;
}

#endif

static void ILI9341_FillScreeen(
	ILI9341_obj_t *self,
	uint16_t u16Color
)
{

	/*Column addresses*/
	LCD_WR_REG(self, 0x2A);
	SPI_SET_SS_LOW(self->sSPIObj.spi);
	LCD_SPI_WR_DATA16(self, 0);
	SPI_SET_SS_HIGH(self->sSPIObj.spi);
	SPI_SET_SS_LOW(self->sSPIObj.spi);
	LCD_SPI_WR_DATA16(self, 319);
	SPI_SET_SS_HIGH(self->sSPIObj.spi);


	LCD_WR_REG(self, 0x2B);
	SPI_SET_SS_LOW(self->sSPIObj.spi);
	LCD_SPI_WR_DATA16(self, 0);
	SPI_SET_SS_HIGH(self->sSPIObj.spi);
	SPI_SET_SS_LOW(self->sSPIObj.spi);
	LCD_SPI_WR_DATA16(self, 239);
	SPI_SET_SS_HIGH(self->sSPIObj.spi);

	/*Memory write*/
	LCD_WR_REG(self, 0x2C);	

	int i;
	SPI_SET_SS_LOW(self->sSPIObj.spi);
	
	for(i = 0 ; i < (240 * 320); i ++){
		LCD_SPI_WR_DATA16(self, u16Color);
	}

	SPI_SET_SS_HIGH(self->sSPIObj.spi);
}

/*-----------------------------------------------*/
// Initial LIL9341 LCD driver chip 
// 
/*-----------------------------------------------*/
static void ILI9341_Initial(
	ILI9341_obj_t *self
)
{
    uint16_t Reg = 0;
    
    /* Hardware reset */
    SET_RST;
    mp_hal_delay_ms(5);     // Delay 5ms
    
    CLR_RST;
    mp_hal_delay_ms(20);    // Delay 20ms
    
    SET_RST;
    mp_hal_delay_ms(40);    // Delay 40ms

    /* Initial control registers */
    LCD_WR_REG(self, 0xCB);
    LCD_WR_DATA(self, 0x39);
    LCD_WR_DATA(self, 0x2C);
    LCD_WR_DATA(self, 0x00);
    LCD_WR_DATA(self, 0x34);
    LCD_WR_DATA(self, 0x02);

    LCD_WR_REG(self, 0xCF);
    LCD_WR_DATA(self, 0x00);
    LCD_WR_DATA(self, 0xC1);
    LCD_WR_DATA(self, 0x30);

    LCD_WR_REG(self, 0xE8);
    LCD_WR_DATA(self, 0x85);
    LCD_WR_DATA(self, 0x00);
    LCD_WR_DATA(self, 0x78);

    LCD_WR_REG(self, 0xEA);
    LCD_WR_DATA(self, 0x00);
    LCD_WR_DATA(self, 0x00);

    LCD_WR_REG(self, 0xED);
    LCD_WR_DATA(self, 0x64);
    LCD_WR_DATA(self, 0x03);
    LCD_WR_DATA(self, 0x12);
    LCD_WR_DATA(self, 0x81);

    LCD_WR_REG(self, 0xF7);
    LCD_WR_DATA(self, 0x20);

    LCD_WR_REG(self, 0xC0);
    LCD_WR_DATA(self, 0x23);

    LCD_WR_REG(self, 0xC1);
    LCD_WR_DATA(self, 0x10);

    LCD_WR_REG(self, 0xC5);
    LCD_WR_DATA(self, 0x3e);
    LCD_WR_DATA(self, 0x28);

    LCD_WR_REG(self, 0xC7);
    LCD_WR_DATA(self, 0x86);

    LCD_WR_REG(self, 0x36);

	if(self->i32Width == 240)
		LCD_WR_DATA(self, 0x48); // for 240x320
	else
		LCD_WR_DATA(self, 0xE8); // for 320x240

    LCD_WR_REG(self, 0x3A);
    LCD_WR_DATA(self, 0x55);
        
    LCD_WR_REG(self, 0xB1);
    LCD_WR_DATA(self, 0x00);
    LCD_WR_DATA(self, 0x18);

    LCD_WR_REG(self, 0xB6);
    LCD_WR_DATA(self, 0x08);
    LCD_WR_DATA(self, 0x82);
    LCD_WR_DATA(self, 0x27);

    LCD_WR_REG(self, 0xF2);
    LCD_WR_DATA(self, 0x00);

    LCD_WR_REG(self, 0x26);
    LCD_WR_DATA(self, 0x01);

    LCD_WR_REG(self, 0xE0);
    LCD_WR_DATA(self, 0x0F);
    LCD_WR_DATA(self, 0x31);
    LCD_WR_DATA(self, 0x2B);
    LCD_WR_DATA(self, 0x0C);
    LCD_WR_DATA(self, 0x0E);
    LCD_WR_DATA(self, 0x08);
    LCD_WR_DATA(self, 0x4E);
    LCD_WR_DATA(self, 0xF1);
    LCD_WR_DATA(self, 0x37);
    LCD_WR_DATA(self, 0x07);
    LCD_WR_DATA(self, 0x10);
    LCD_WR_DATA(self, 0x03);
    LCD_WR_DATA(self, 0x0E);
    LCD_WR_DATA(self, 0x09);
    LCD_WR_DATA(self, 0x00);

    LCD_WR_REG(self, 0xE1);
    LCD_WR_DATA(self, 0x00);
    LCD_WR_DATA(self, 0x0E);
    LCD_WR_DATA(self, 0x14);
    LCD_WR_DATA(self, 0x03);
    LCD_WR_DATA(self, 0x11);
    LCD_WR_DATA(self, 0x07);
    LCD_WR_DATA(self, 0x31);
    LCD_WR_DATA(self, 0xC1);
    LCD_WR_DATA(self, 0x48);
    LCD_WR_DATA(self, 0x08);
    LCD_WR_DATA(self, 0x0F);
    LCD_WR_DATA(self, 0x0C);
    LCD_WR_DATA(self, 0x31);
    LCD_WR_DATA(self, 0x36);
    LCD_WR_DATA(self, 0x0F);

    LCD_WR_REG(self, 0x11);
    mp_hal_delay_ms(200);     // Delay 200ms
    
    LCD_WR_REG(self, 0x29);           //Display on

#if defined(_ILI9341_USING_EBI_)

    LCD_WR_REG(self, 0x0A);
    Reg = LCD_EBI_RD_DATA(self);
    Reg = LCD_EBI_RD_DATA(self);
    printf("0Ah = %02x.\n", Reg);
    
    LCD_WR_REG(self, 0x0B);
    Reg = LCD_EBI_RD_DATA(self);
    Reg = LCD_EBI_RD_DATA(self);
    printf("0Bh = %02x.\n", Reg);
	
    LCD_WR_REG(self, 0x0C);
    Reg = LCD_EBI_RD_DATA(self);
    Reg = LCD_EBI_RD_DATA(self);
    printf("0Ch = %02x.\n", Reg);
#else
#if 1
	uint16_t u16Data;


	u16Data = ILI9341_ReadRegister(self, 0x0A, 1);
    printf("0Ah = %02x.\n", u16Data);

	u16Data = ILI9341_ReadRegister(self, 0x0B, 1);
    printf("0Bh = %02x.\n", u16Data);

	u16Data = ILI9341_ReadRegister(self, 0x0C, 1);
    printf("0Ch = %02x.\n", u16Data);


	u16Data = ILI9341_ReadRegister(self, 0xD3, 1);
    printf("D3h = %02x.\n", u16Data);
	u16Data = ILI9341_ReadRegister(self, 0xD3, 2);
    printf("D3h = %02x.\n", u16Data);
	u16Data = ILI9341_ReadRegister(self, 0xD3, 3);
    printf("D3h = %02x.\n", u16Data);

#else
	ILI9341_FillScreeen(self, 0xC0C0);
#endif
#endif

    printf("Initial ILI9341 LCD Module done.\n\n");

}

static void ILI9341Driver_Init(
	ILI9341_obj_t *self
)
{
#if defined(_ILI9341_USING_EBI_)

    CLK_EnableModuleClock(EBI_MODULE);

    /* Configure DC/RESET/LED pins */
    EBI_FuncPinInit();

    /* Initialize EBI bank0 to access external LCD Module */
    EBI_Open(EBI_BANK0, EBI_BUSWIDTH_16BIT, EBI_TIMING_NORMAL, 0, EBI_CS_ACTIVE_LOW);
    EBI->CTL0 |= EBI_CTL0_CACCESS_Msk;
    EBI->TCTL0 |= (EBI_TCTL0_WAHDOFF_Msk | EBI_TCTL0_RAHDOFF_Msk);
//    printf("\n[EBI CTL0:0x%08X, TCLT0:0x%08X]\n\n", EBI->CTL0, EBI->TCTL0);

#endif

#if defined(_ILI9341_USING_SPI_)
	SPI_FuncPinInit(self);
#endif	
    /* Init LCD Module */
    ILI9341_Initial(self);
    
    /* PB.7 BL_CTRL pin */
    ILI9341_BL_PIN = 1;
}


/******************************************************************************/
/* MicroPython bindings                                                       */


STATIC const mp_obj_type_t ILI9341_type;

STATIC ILI9341_obj_t ili9341_obj = {
	.base = {(mp_obj_type_t *)&ILI9341_type},
	.initialized = false,
};


STATIC mp_obj_t ILI9341_init(
	mp_obj_t self_in
)
{
	ILI9341_obj_t *self = (ILI9341_obj_t *)self_in;
	if(!self->initialized){
		ILI9341Driver_Init(self);
		self->initialized = true;
	}

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(ILI9341_init_obj, ILI9341_init);

static void ILI9341Driver_flush(
	struct _disp_drv_t *psDisp, 
	const lv_area_t *psArea, 
	lv_color_t *psColor_p
)
{
	uint16_t data[4];

	/*Column addresses*/
	LCD_WR_REG(&ili9341_obj, 0x2A);
#if defined(_ILI9341_USING_EBI_)
	data[0] = (psArea->x1 >> 8) & 0xFF;
	data[1] = psArea->x1 & 0xFF;
	data[2] = (psArea->x2 >> 8) & 0xFF;
	data[3] = psArea->x2 & 0xFF;
	LcdWriteDataMultiple(&ili9341_obj, data, 4);
#endif
#if defined(_ILI9341_USING_SPI_)
	data[0] = psArea->x1;
	data[1] = psArea->x2;
	LcdWriteDataMultiple(&ili9341_obj, &data[0], 1);
	LcdWriteDataMultiple(&ili9341_obj, &data[1], 1);
#endif

	/*Page addresses*/
	LCD_WR_REG(&ili9341_obj, 0x2B);
#if defined(_ILI9341_USING_EBI_)
	data[0] = (psArea->y1 >> 8) & 0xFF;
	data[1] = psArea->y1 & 0xFF;
	data[2] = (psArea->y2 >> 8) & 0xFF;
	data[3] = psArea->y2 & 0xFF;
	LcdWriteDataMultiple(&ili9341_obj, data, 4);
#endif
#if defined(_ILI9341_USING_SPI_)
	data[0] = psArea->y1;
	data[1] = psArea->y2;
	LcdWriteDataMultiple(&ili9341_obj, &data[0], 1);
	LcdWriteDataMultiple(&ili9341_obj, &data[1], 1);
#endif	

	/*Memory write*/
	LCD_WR_REG(&ili9341_obj, 0x2C);	

	uint32_t u32PixelSize = (psArea->x2 - psArea->x1 + 1) * (psArea->y2 - psArea->y1 + 1);

	LcdWriteDataMultiple(&ili9341_obj, (uint16_t *)psColor_p, u32PixelSize);
	
	lv_disp_flush_ready(psDisp);
}


DEFINE_PTR_OBJ(ILI9341Driver_flush);

STATIC const mp_rom_map_elem_t ILI9341_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&ILI9341_init_obj) },
//    { MP_ROM_QSTR(MP_QSTR_activate), MP_ROM_PTR(&ILI9341_activate_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&PTR_OBJ(ILI9341Driver_flush)) },
};

STATIC MP_DEFINE_CONST_DICT(ILI9341_locals_dict, ILI9341_locals_dict_table);


STATIC mp_obj_t ILI9341_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	ILI9341_obj_t *self = &ili9341_obj;

    if (self->initialized) {
        return (mp_obj_t)self;
    }

    enum{
         ARG_width,
    };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_width,MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int=320}},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	self->i32Width = args[ARG_width].u_int;
	return (mp_obj_t)self;
}

STATIC const mp_obj_type_t ILI9341_type = {
    { &mp_type_type },
    .name = MP_QSTR_ILI9341,
    //.print = ILI9341_print,
    .make_new = ILI9341_make_new,
    .locals_dict = (mp_obj_dict_t*)&ILI9341_locals_dict,
};

STATIC const mp_rom_map_elem_t ILI9341_globals_table[] = {

	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ILI9341) },
	{ MP_ROM_QSTR(MP_QSTR_display), (mp_obj_t)&ILI9341_type},
};

STATIC MP_DEFINE_CONST_DICT(ILI9341_module_globals, ILI9341_globals_table);

const mp_obj_module_t mp_module_ILI9341 = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&ILI9341_module_globals,
};

#endif


