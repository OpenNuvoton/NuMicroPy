//////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/binary.h"

#if MICROPY_HW_ENABLE_VPOST

#include "wblib.h"
#include "N9H26_VPOST.h"

#if MICROPY_LVGL
#include "lvgl/lvgl.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// A read-only buffer that contains a C pointer
// Used to communicate function pointers to Micropython bindings
///////////////////////////////////////////////////////////////////////////////

typedef struct mp_ptr_t
{
    mp_obj_base_t base;
    void *ptr;
} mp_ptr_t;

STATIC mp_int_t mp_ptr_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags)
{
    mp_ptr_t *self = MP_OBJ_TO_PTR(self_in);

    if (flags & MP_BUFFER_WRITE) {
        // read-only ptr
        return 1;
    }

    bufinfo->buf = &self->ptr;
    bufinfo->len = sizeof(self->ptr);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}

#define PTR_OBJ(ptr_global) ptr_global ## _obj

#define DEFINE_PTR_OBJ_TYPE(ptr_obj_type, ptr_type_qstr)\
STATIC const mp_obj_type_t ptr_obj_type = {\
    { &mp_type_type },\
    .name = ptr_type_qstr,\
    .buffer_p = { .get_buffer = mp_ptr_get_buffer }\
}

#define DEFINE_PTR_OBJ(ptr_global)\
DEFINE_PTR_OBJ_TYPE(ptr_global ## _type, MP_QSTR_ ## ptr_global);\
STATIC const mp_ptr_t PTR_OBJ(ptr_global) = {\
    { &ptr_global ## _type },\
    &ptr_global\
}

#define NEW_PTR_OBJ(name, value)\
({\
    DEFINE_PTR_OBJ_TYPE(ptr_obj_type, MP_QSTR_ ## name);\
    mp_ptr_t *self = m_new_obj(mp_ptr_t);\
    self->base.type = &ptr_obj_type;\
    self->ptr = value;\
    MP_OBJ_FROM_PTR(self);\
})

///////////////////////////////////////////////////////////////////////////////////

#if defined (__HAVE_GIANTPLUS_GPM1006D0__)
#define LCD_HEIGHT 	640
#define LCD_WIDTH 	480
#define FRAME_BUF_SIZE (LCD_HEIGHT * LCD_WIDTH * 2)	
#endif

typedef struct _VPOST_obj_t {
    mp_obj_base_t base;
    bool initialized;
	bool bOSDMode;
	uint32_t u32ColorFormat;
	uint8_t *pu8ScreenBuf0;
	uint8_t *pu8ScreenBuf1;
	bool bFrameDirty;
} VPOST_obj_t;

typedef struct _VPOST_OSD_obj_t {
    mp_obj_base_t base;
    bool initialized;
	bool bOSDMode;
	uint32_t u32ColorFormat;
	uint8_t *pu8ScreenBuf0;
	uint8_t *pu8ScreenBuf1;
	bool bFrameDirty;

	int32_t i32ColorKey;
}VPOST_OSD_obj_t;

typedef VPOST_obj_t VPOST_Frame_obj_t ;

STATIC const mp_obj_type_t VPOST_type;

STATIC VPOST_Frame_obj_t VPOST_Frame_obj = {
	.base = {(mp_obj_type_t *)&VPOST_type},
	.initialized = false,
	.bOSDMode = false,
	.u32ColorFormat = DRVVPOST_FRAME_RGB565,
	.pu8ScreenBuf0 = NULL,
	.pu8ScreenBuf1 = NULL,
	.bFrameDirty = false,
};

STATIC VPOST_OSD_obj_t VPOST_OSD_obj = {
	.base = {(mp_obj_type_t *)&VPOST_type},
	.initialized = false,
	.bOSDMode = true,
	.u32ColorFormat = DRVVPOST_FRAME_RGB565,
	.pu8ScreenBuf0 = NULL,
	.pu8ScreenBuf1 = NULL,
	.bFrameDirty = false,
	
	.i32ColorKey = -1,
};


static volatile uint8_t *s_pu8NextScrFrameBufAddr;
static volatile uint8_t *s_pu8NextScrOSDBufAddr;

static void VPOST_InterruptServiceRiuntine()
{
	if(VPOST_Frame_obj.bFrameDirty){
		vpostSetFrameBuffer((uint32_t)s_pu8NextScrFrameBufAddr);
		VPOST_Frame_obj.bFrameDirty = false;
	}

	if(VPOST_OSD_obj.bFrameDirty){
		vpostSetOSD_BaseAddress((uint32_t)s_pu8NextScrOSDBufAddr);
		VPOST_OSD_obj.bFrameDirty = false;
	}
}

static int VPOST_FrameInit(VPOST_Frame_obj_t *self)
{
	uint8_t *pu8ScreenBuf0;
	uint8_t *pu8ScreenBuf1;
	
	pu8ScreenBuf0 = malloc(FRAME_BUF_SIZE);
	pu8ScreenBuf1 = malloc(FRAME_BUF_SIZE);

	if((pu8ScreenBuf0 == NULL) || (pu8ScreenBuf1 == NULL))
		goto frameinit_fail;

	memset(pu8ScreenBuf0, 0x0, FRAME_BUF_SIZE);
	memset(pu8ScreenBuf1, 0x0, FRAME_BUF_SIZE);

	PFN_DRVVPOST_INT_CALLBACK fun_ptr;
	LCDFORMATEX lcdFormat;	

	lcdFormat.ucVASrcFormat = self->u32ColorFormat;
	lcdFormat.nScreenWidth = LCD_WIDTH;
	lcdFormat.nScreenHeight = LCD_HEIGHT;	  
	vpostLCMInit(&lcdFormat, (UINT32*)pu8ScreenBuf0);
	
	vpostInstallCallBack(eDRVVPOST_VINT, (PFN_DRVVPOST_INT_CALLBACK)VPOST_InterruptServiceRiuntine,  (PFN_DRVVPOST_INT_CALLBACK*)&fun_ptr);
	vpostEnableInt(eDRVVPOST_VINT);	
	sysEnableInterrupt(IRQ_VPOST);	

	self->pu8ScreenBuf0 = pu8ScreenBuf0;
	self->pu8ScreenBuf1 = pu8ScreenBuf1;

	self->initialized = true;
	return 0;	

frameinit_fail:
	
	if(pu8ScreenBuf0)
		free(pu8ScreenBuf0);

	if(pu8ScreenBuf1)
		free(pu8ScreenBuf1);

	return -1;
}

static int VPOST_OSDInit(VPOST_OSD_obj_t *self)
{
	uint8_t *pu8ScreenBuf0;
	uint8_t *pu8ScreenBuf1;
	
	pu8ScreenBuf0 = malloc(FRAME_BUF_SIZE);
	pu8ScreenBuf1 = malloc(FRAME_BUF_SIZE);

	if((pu8ScreenBuf0 == NULL) || (pu8ScreenBuf1 == NULL))
		goto osdinit_fail;

	memset(pu8ScreenBuf0, 0x0, FRAME_BUF_SIZE);
	memset(pu8ScreenBuf1, 0x0, FRAME_BUF_SIZE);

	S_DRVVPOST_OSD_SIZE sSize;
	S_DRVVPOST_OSD_POS sPos;
	S_DRVVPOST_OSD sOSD;

	/* set OSD position */
	sPos.u16VStart_1st = 0; 						
	sPos.u16VEnd_1st = LCD_HEIGHT - 1; 							
	sPos.u16HStart_1st = 0; 						
	sPos.u16HEnd_1st = LCD_WIDTH - 1; 							

	sPos.u16VOffset_2nd = 0; 						
	sPos.u16HOffset_2nd = 0;
	sOSD.psPos = &sPos;

	/* set OSD size */
	sSize.u16HSize = LCD_WIDTH; 						
	sSize.u16VSize = LCD_HEIGHT; 
	sOSD.psSize = &sSize;	

	if(self->u32ColorFormat == DRVVPOST_FRAME_RGB555)
		sOSD.eType = eDRVVPOST_OSD_RGB555;
	else if(self->u32ColorFormat == DRVVPOST_FRAME_RGB565)
		sOSD.eType = eDRVVPOST_OSD_RGB565;
	else if(self->u32ColorFormat == DRVVPOST_FRAME_CBYCRY)
		sOSD.eType = eDRVVPOST_OSD_CB0Y0CR0Y1;
	else if(self->u32ColorFormat == DRVVPOST_FRAME_YCBYCR)
		sOSD.eType = eDRVVPOST_OSD_Y0CB0Y1CR0;
	else if(self->u32ColorFormat == DRVVPOST_FRAME_CRYCBY)
		sOSD.eType = eDRVVPOST_OSD_CR0Y0CB0Y1;
	else if(self->u32ColorFormat == DRVVPOST_FRAME_YCRYCB)
		sOSD.eType = eDRVVPOST_OSD_Y0CR0Y1CB0;
	else
		sOSD.eType = eDRVVPOST_OSD_RGB565;

	/* configure OSD */
	vpostSetOSD_DataType(sOSD.eType);
	vpostSetOSD_BaseAddress((UINT32)(UINT16*) pu8ScreenBuf0);
	vpostSetOSD_Size(sOSD.psSize);
	vpostSetOSD_Pos(sOSD.psPos);	
	vpostSetOSD_Enable();
	
	if(self->i32ColorKey != -1)
	{
		vpostSetOSD_Transparent_Enable();
		if((self->u32ColorFormat == DRVVPOST_FRAME_RGB555) || 
			(self->u32ColorFormat == DRVVPOST_FRAME_RGB565))
			vpostSetOSD_Transparent(eDRVVPOST_OSD_TRANSPARENT_RGB565, self->i32ColorKey);
		else
			vpostSetOSD_Transparent(eDRVVPOST_OSD_TRANSPARENT_YUV, self->i32ColorKey);
	}

	self->pu8ScreenBuf0 = pu8ScreenBuf0;
	self->pu8ScreenBuf1 = pu8ScreenBuf1;

	self->initialized = true;
	return 0;	

osdinit_fail:
	
	if(pu8ScreenBuf0)
		free(pu8ScreenBuf0);

	if(pu8ScreenBuf1)
		free(pu8ScreenBuf1);

	return -1;
}

#if MICROPY_LVGL

//Note: Must set self object in display driver at py code
static void VPOSTDriver_flush(
	struct _disp_drv_t *psDisp, 
	const lv_area_t *psArea, 
	lv_color_t *psColor_p
)
{
	uint8_t *pu8OffScrBufAddr = (uint8_t *)s_pu8NextScrFrameBufAddr;
	
	VPOST_obj_t *self = (VPOST_obj_t *)psDisp->user_data;
	
	if(self->bFrameDirty == false){
		if(s_pu8NextScrFrameBufAddr == self->pu8ScreenBuf0)
			pu8OffScrBufAddr = self->pu8ScreenBuf1;
		else
			pu8OffScrBufAddr = self->pu8ScreenBuf0;
	
		//Copy on-screen buffer data to off-screen buffer
		memcpy(pu8OffScrBufAddr, (const void*)s_pu8NextScrFrameBufAddr, FRAME_BUF_SIZE);
	}

	//draw new pixel data
	int y;
	int i32EachCopySize;
	int i32LineSize;
	uint8_t *pu8SrcAddr = (uint8_t *)psColor_p;	
	uint8_t *pu8CopyAddr = pu8OffScrBufAddr + ((psArea->y1 * LCD_WIDTH + psArea->x1) * sizeof(uint16_t));

	i32LineSize = LCD_WIDTH * sizeof(uint16_t);
	i32EachCopySize = (psArea->x2 - psArea->x1) * sizeof(uint16_t);
	
	for(y = psArea->y1; y < psArea->y2; y ++){
		memcpy(pu8CopyAddr, pu8SrcAddr, i32EachCopySize);
		pu8CopyAddr += i32LineSize;
		pu8SrcAddr += i32EachCopySize;
	} 
	
	//Update next frame to off screen buffer
	if(self->bOSDMode == false)
		s_pu8NextScrFrameBufAddr = pu8OffScrBufAddr;
	else
		s_pu8NextScrOSDBufAddr = pu8OffScrBufAddr;

	self->bFrameDirty = true;

	lv_disp_flush_ready(psDisp);
}

DEFINE_PTR_OBJ(VPOSTDriver_flush);

#endif


#define PTR_OBJ(ptr_global) ptr_global ## _obj

#define DEFINE_PTR_OBJ_TYPE(ptr_obj_type, ptr_type_qstr)\
STATIC const mp_obj_type_t ptr_obj_type = {\
    { &mp_type_type },\
    .name = ptr_type_qstr,\
    .buffer_p = { .get_buffer = mp_ptr_get_buffer }\
}

#define DEFINE_PTR_OBJ(ptr_global)\
DEFINE_PTR_OBJ_TYPE(ptr_global ## _type, MP_QSTR_ ## ptr_global);\
STATIC const mp_ptr_t PTR_OBJ(ptr_global) = {\
    { &ptr_global ## _type },\
    &ptr_global\
}

//Note: Must set self object at py code
static void VPOSTDriver_Render(
	mp_obj_t self_in,
	uint8_t *pu8FrameBufAddr
)
{
	VPOST_obj_t *self = (VPOST_obj_t *)self_in;
	

	//Update next frame to user frame buffer
	if(self->bOSDMode == false)
		s_pu8NextScrFrameBufAddr = pu8FrameBufAddr;
	else
		s_pu8NextScrOSDBufAddr = pu8FrameBufAddr;

	self->bFrameDirty = true;
}

DEFINE_PTR_OBJ(VPOSTDriver_Render);

/******************************************************************************/
/* MicroPython bindings                                                       */

STATIC mp_obj_t VPOST_init(
	mp_obj_t self_in
)
{
	int i32Ret = 0;
	VPOST_obj_t *self = (VPOST_obj_t *)self_in;


	if(!self->initialized){
		if(self->bOSDMode == true){
			if(VPOST_Frame_obj.initialized == false)
				nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "must init VPOST frame first"));

			i32Ret = VPOST_OSDInit((VPOST_OSD_obj_t *)self);
		}
		else{
			i32Ret = VPOST_FrameInit((VPOST_Frame_obj_t *)self);
		}
	}

	if(i32Ret != 0){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "unable init VPOST object"));
	}

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(VPOST_init_obj, VPOST_init);



STATIC const mp_rom_map_elem_t VPOST_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&VPOST_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&PTR_OBJ(VPOSTDriver_Render)) },

#if MICROPY_LVGL
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&PTR_OBJ(VPOSTDriver_flush)) },
#endif

	// class constants
	{ MP_ROM_QSTR(MP_QSTR_RGB555),	MP_ROM_INT(DRVVPOST_FRAME_RGB555) },
	{ MP_ROM_QSTR(MP_QSTR_RGB565),	MP_ROM_INT(DRVVPOST_FRAME_RGB565) },
	{ MP_ROM_QSTR(MP_QSTR_CBYCRY),	MP_ROM_INT(DRVVPOST_FRAME_CBYCRY) },
    { MP_ROM_QSTR(MP_QSTR_YCBYCR),	MP_ROM_INT(DRVVPOST_FRAME_YCBYCR) },
	{ MP_ROM_QSTR(MP_QSTR_CRYCBY),	MP_ROM_INT(DRVVPOST_FRAME_CRYCBY) },
	{ MP_ROM_QSTR(MP_QSTR_YCRYCB),	MP_ROM_INT(DRVVPOST_FRAME_YCRYCB) },
};

STATIC MP_DEFINE_CONST_DICT(VPOST_locals_dict, VPOST_locals_dict_table);


STATIC mp_obj_t VPOST_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	VPOST_obj_t *self;

	enum{
		ARG_OSDLayer,
		ARG_ColorKey,
		ARG_ColorFormat,				 
	};

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_OSDLayer, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool=false}},
        { MP_QSTR_OSDColorKey, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        { MP_QSTR_OSDColorFormat, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DRVVPOST_FRAME_RGB565}},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	if(args[ARG_OSDLayer].u_bool == true){
		VPOST_OSD_obj.i32ColorKey = args[ARG_ColorKey].u_int;
		self = (VPOST_obj_t *)&VPOST_OSD_obj;
		self->bOSDMode = true;
	}
	else{
		self = (VPOST_obj_t *)&VPOST_Frame_obj;
		self->bOSDMode = false;
	}

    if (self->initialized) {
        return (mp_obj_t)self;
    }

	self->u32ColorFormat = args[ARG_ColorFormat].u_int;

	return (mp_obj_t)self;
}

STATIC const mp_obj_type_t VPOST_type = {
    { &mp_type_type },
    .name = MP_QSTR_VPOST,
    .make_new = VPOST_make_new,
    .locals_dict = (mp_obj_dict_t*)&VPOST_locals_dict,
};

STATIC const mp_rom_map_elem_t VPOST_globals_table[] = {

	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_VPOST) },
	{ MP_ROM_QSTR(MP_QSTR_display), (mp_obj_t)&VPOST_type},
};

STATIC MP_DEFINE_CONST_DICT(VPOST_module_globals, VPOST_globals_table);

const mp_obj_module_t mp_module_VPOST = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&VPOST_module_globals,
};

#endif
