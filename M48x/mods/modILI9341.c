//////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////


#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#if MICROPY_LVGL

#include "NuMicro.h"
#include "lvgl/lvgl.h"
#include "driver/include/common.h"

typedef struct _ILI9341_obj_t {
    mp_obj_base_t base;
    bool initialized;
	int32_t i32Width;
} ILI9341_obj_t;


/* LCD Module "RESET" */
#define SET_RST PB6 = 1;
#define CLR_RST PB6 = 0;

/* LCD Module "RS" */
#define SET_RS  PH3 = 1;
#define CLR_RS  PH3 = 0;

/*-----------------------------------------------*/
// Write control registers of LCD module  
// 
/*-----------------------------------------------*/
static void LCD_WR_REG(uint16_t cmd)
{
    CLR_RS
    EBI0_WRITE_DATA16(0x00000000, cmd);
    SET_RS

}

/*-----------------------------------------------*/
// Write data to SRAM of LCD module  
// 
/*-----------------------------------------------*/
static void LCD_WR_DATA(uint16_t dat)
{
    EBI0_WRITE_DATA16(0x00030000, dat);

}

/*-----------------------------------------------*/
// Read data from SRAM of LCD module 
// 
/*-----------------------------------------------*/
static uint16_t LCD_RD_DATA(void)
{
    return EBI0_READ_DATA16(0x00030000);

}

/********************************************************************
*
*       LcdWriteDataMultiple
*
*   Function description:
*   Writes multiple values to a display register.
*/
static void LcdWriteDataMultiple(uint16_t * pData, int NumItems)
{
    while (NumItems--) {
        EBI0_WRITE_DATA16(0x00030000,*pData++);
    }
}


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
    GPIO_SetMode(PH, BIT3, GPIO_MODE_OUTPUT);
    PH3 = 1;

    /* EBI RD and WR pins on PE.4 and PE.5 */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE4MFP_Msk | SYS_GPE_MFPL_PE5MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE4MFP_EBI_nWR | SYS_GPE_MFPL_PE5MFP_EBI_nRD);

    /* EBI CS0 pin on PD.14 */
    SYS->GPD_MFPH &= ~SYS_GPD_MFPH_PD14MFP_Msk;
    SYS->GPD_MFPH |= SYS_GPD_MFPH_PD14MFP_EBI_nCS0;

    /* Configure PB.6 and PB.7 as Output mode for LCD_RST and LCD_Backlight */
    GPIO_SetMode(PB, BIT6, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PB, BIT7, GPIO_MODE_OUTPUT);
    PB6 = 1;
    PB7 = 0;
    
    GPIO_SetMode(PH, BIT5, GPIO_MODE_INPUT);
    GPIO_SetMode(PH, BIT6, GPIO_MODE_INPUT);
    GPIO_SetMode(PH, BIT7, GPIO_MODE_INPUT);
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
    LCD_WR_REG(0xCB);
    LCD_WR_DATA(0x39);
    LCD_WR_DATA(0x2C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x34);
    LCD_WR_DATA(0x02);

    LCD_WR_REG(0xCF);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xC1);
    LCD_WR_DATA(0x30);

    LCD_WR_REG(0xE8);
    LCD_WR_DATA(0x85);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x78);

    LCD_WR_REG(0xEA);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xED);
    LCD_WR_DATA(0x64);
    LCD_WR_DATA(0x03);
    LCD_WR_DATA(0x12);
    LCD_WR_DATA(0x81);

    LCD_WR_REG(0xF7);
    LCD_WR_DATA(0x20);

    LCD_WR_REG(0xC0);
    LCD_WR_DATA(0x23);

    LCD_WR_REG(0xC1);
    LCD_WR_DATA(0x10);

    LCD_WR_REG(0xC5);
    LCD_WR_DATA(0x3e);
    LCD_WR_DATA(0x28);

    LCD_WR_REG(0xC7);
    LCD_WR_DATA(0x86);

    LCD_WR_REG(0x36);

	if(self->i32Width == 240)
		LCD_WR_DATA(0x48); // for 240x320
	else
		LCD_WR_DATA(0xE8); // for 320x240

    LCD_WR_REG(0x3A);
    LCD_WR_DATA(0x55);
        
    LCD_WR_REG(0xB1);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x18);

    LCD_WR_REG(0xB6);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x82);
    LCD_WR_DATA(0x27);

    LCD_WR_REG(0xF2);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0x26);
    LCD_WR_DATA(0x01);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x31);
    LCD_WR_DATA(0x2B);
    LCD_WR_DATA(0x0C);
    LCD_WR_DATA(0x0E);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x4E);
    LCD_WR_DATA(0xF1);
    LCD_WR_DATA(0x37);
    LCD_WR_DATA(0x07);
    LCD_WR_DATA(0x10);
    LCD_WR_DATA(0x03);
    LCD_WR_DATA(0x0E);
    LCD_WR_DATA(0x09);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x0E);
    LCD_WR_DATA(0x14);
    LCD_WR_DATA(0x03);
    LCD_WR_DATA(0x11);
    LCD_WR_DATA(0x07);
    LCD_WR_DATA(0x31);
    LCD_WR_DATA(0xC1);
    LCD_WR_DATA(0x48);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x0C);
    LCD_WR_DATA(0x31);
    LCD_WR_DATA(0x36);
    LCD_WR_DATA(0x0F);

    LCD_WR_REG(0x11);
    mp_hal_delay_ms(200);     // Delay 200ms
    
    LCD_WR_REG(0x29);           //Display on

    LCD_WR_REG(0x0A);
    Reg = LCD_RD_DATA();
    Reg = LCD_RD_DATA();
    printf("0Ah = %02x.\n", Reg);
    
    LCD_WR_REG(0x0B);
    Reg = LCD_RD_DATA();
    Reg = LCD_RD_DATA();
    printf("0Bh = %02x.\n", Reg);
	
    LCD_WR_REG(0x0C);
    Reg = LCD_RD_DATA();
    Reg = LCD_RD_DATA();
    printf("0Ch = %02x.\n", Reg);

    printf("Initial ILI9341 LCD Module done.\n\n");

}

static void ILI9341Driver_Init(
	ILI9341_obj_t *self
)
{
    CLK_EnableModuleClock(EBI_MODULE);

    /* Configure DC/RESET/LED pins */
    EBI_FuncPinInit();

    /* Initialize EBI bank0 to access external LCD Module */
    EBI_Open(EBI_BANK0, EBI_BUSWIDTH_16BIT, EBI_TIMING_NORMAL, 0, EBI_CS_ACTIVE_LOW);
    EBI->CTL0 |= EBI_CTL0_CACCESS_Msk;
    EBI->TCTL0 |= (EBI_TCTL0_WAHDOFF_Msk | EBI_TCTL0_RAHDOFF_Msk);
//    printf("\n[EBI CTL0:0x%08X, TCLT0:0x%08X]\n\n", EBI->CTL0, EBI->TCTL0);
	
    /* Init LCD Module */
    ILI9341_Initial(self);
    
    /* PB.7 BL_CTRL pin */
    PB7 = 1;
}

static void ILI9341Driver_flush(
	struct _disp_drv_t *psDisp, 
	const lv_area_t *psArea, 
	lv_color_t *psColor_p
)
{
	uint16_t data[4];

//	printf("ILI9341Driver_flush %d, %d \n", psArea->x1, psArea->y1);
	/*Column addresses*/
	LCD_WR_REG(0x2A);
	data[0] = (psArea->x1 >> 8) & 0xFF;
	data[1] = psArea->x1 & 0xFF;
	data[2] = (psArea->x2 >> 8) & 0xFF;
	data[3] = psArea->x2 & 0xFF;
	LcdWriteDataMultiple(data, 4);
	
	/*Page addresses*/
	LCD_WR_REG(0x2B);
	data[0] = (psArea->y1 >> 8) & 0xFF;
	data[1] = psArea->y1 & 0xFF;
	data[2] = (psArea->y2 >> 8) & 0xFF;
	data[3] = psArea->y2 & 0xFF;
	LcdWriteDataMultiple(data, 4);

	/*Memory write*/
	LCD_WR_REG(0x2C);	

	uint32_t u32PixelSize = (psArea->x2 - psArea->x1 + 1) * (psArea->y2 - psArea->y1 + 1);

	LcdWriteDataMultiple((uint16_t *)psColor_p, u32PixelSize);
	
	lv_disp_flush_ready(psDisp);
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


