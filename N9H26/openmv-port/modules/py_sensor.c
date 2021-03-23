/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2019 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2019 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Sensor Python module.
 */
#include <stdarg.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/binary.h"

#if MICROPY_PY_SENSOR

#include "py_image.h"
#include "py_assert.h"
#include "sensor.h"
#include "imlib.h"
#include "omv_boardconfig.h"
#include "framebuffer.h"

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
STATIC mp_ptr_t PTR_OBJ(ptr_global) = {\
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
//////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	eSENSOR_PIPE_PLANAR,
	eSENSOR_PIPE_PACKET,	
}E_SENSOR_PIPE;

extern sensor_t sensor;

static mp_obj_t py_sensor__init__()
{
    // This is the module init function, not the sensor init function,
    // it gets called when the module is imported. This is good
    // place to check if the sensor was detected or not.
    if (sensor_is_detected() == false) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_RuntimeError,
                    "The image sensor is detached or failed to initialize!"));
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_sensor__init__obj,              py_sensor__init__);

static mp_obj_t py_sensor_reset() {
    PY_ASSERT_FALSE_MSG(sensor_reset() != 0, "Reset Failed");
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_sensor_reset_obj,               py_sensor_reset);

static mp_obj_t py_sensor_shutdown(mp_obj_t enable) {
    PY_ASSERT_FALSE_MSG(sensor_shutdown(mp_obj_is_true(enable)) != 0, "Shutdown Failed");
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_sensor_shutdown_obj,            py_sensor_shutdown);

#if 0
//Note: It is a function pointer data type. Used by media engine
int py_sensor_fill(
	image_t *planar_img,
	image_t *packet_img
)
{
    return sensor.snapshot(&sensor, planar_img, packet_img, NULL);
}

DEFINE_PTR_OBJ(py_sensor_fill);
#else
bool py_sensor_planar_fill(
	image_t *planar_img,
	uint64_t *pu64ImgTime
)
{
    return sensor_ReadPlanarImage(&sensor, planar_img, pu64ImgTime);
}

DEFINE_PTR_OBJ(py_sensor_planar_fill);

bool py_sensor_packet_fill(
	image_t *packet_img,
	uint64_t *pu64ImgTime
)
{
    return sensor_ReadPacketImage(&sensor, packet_img, pu64ImgTime);
}

DEFINE_PTR_OBJ(py_sensor_packet_fill);

#endif

static mp_obj_t py_sensor_snapshot(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_planar_image,	MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_packet_image, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

	mp_obj_t planar_img_obj = mp_const_none;
	mp_obj_t packet_img_obj = mp_const_none;
	
	//parse argument
	if(args)
	{
		mp_arg_val_t args_val[MP_ARRAY_SIZE(allowed_args)];
		mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args_val);

		planar_img_obj = args_val[0].u_obj;
		packet_img_obj = args_val[1].u_obj;
	}

	image_t *planar_img = NULL;
	image_t *packet_img = NULL;
	
	if(packet_img_obj == mp_const_none){
		packet_img_obj = py_image(0, 0, 0, 0);
	}
	
	if(planar_img_obj != mp_const_none){
		planar_img = (image_t *)py_image_cobj(planar_img_obj);
	}

	packet_img = (image_t *)py_image_cobj(packet_img_obj);

    int ret = sensor.snapshot(&sensor, planar_img, packet_img, NULL);

    if (ret < 0) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_RuntimeError, "Capture Failed: %d", ret));
    }
	
	return packet_img_obj;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_sensor_snapshot_obj, 0,        py_sensor_snapshot);

static mp_obj_t py_sensor_skip_frames(uint n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    mp_map_elem_t *kw_arg = mp_map_lookup(kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_time), MP_MAP_LOOKUP);
    mp_int_t time = 300;

    if (kw_arg != NULL) {
        time = mp_obj_get_int(kw_arg->value);
    }

    uint32_t millis = mp_hal_ticks_ms();

    if (!n_args) {
        while ((mp_hal_ticks_ms() - millis) < time) { // 32-bit math handles wrap around...
            py_sensor_snapshot(0, NULL, NULL);
        }
    } else {
        for (int i = 0, j = mp_obj_get_int(args[0]); i < j; i++) {
            if ((kw_arg != NULL) && ((mp_hal_ticks_ms() - millis) >= time)) {
                break;
            }

            py_sensor_snapshot(0, NULL, NULL);
        }
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_sensor_skip_frames_obj, 0,     py_sensor_skip_frames);

static mp_obj_t py_sensor_width()
{
    return mp_obj_new_int(resolution[sensor.framesize_packet][0]);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_sensor_width_obj,               py_sensor_width);

static mp_obj_t py_sensor_height()
{
    return mp_obj_new_int(resolution[sensor.framesize_packet][1]);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_sensor_height_obj,              py_sensor_height);

static mp_obj_t py_sensor_get_fb(mp_obj_t pipe)
{
	image_t image;
	int pipe_type;
	
	pipe_type = mp_obj_get_int(pipe);

	if(pipe_type == eSENSOR_PIPE_PLANAR)
	{
		sensor_get_fb(&sensor, &image, NULL);
	}
	else if(pipe_type == eSENSOR_PIPE_PACKET)
	{
		sensor_get_fb(&sensor, NULL, &image);
	}
	else
	{
		return mp_const_none;
	}

    return py_image_from_struct(&image);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_sensor_get_fb_obj,              py_sensor_get_fb);

static mp_obj_t py_sensor_get_id() {
    return mp_obj_new_int(sensor_get_id());
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_sensor_get_id_obj,              py_sensor_get_id);

static mp_obj_t py_sensor_set_pixformat(mp_obj_t pixformat_planar, mp_obj_t pixformat_packet) {
    if (sensor_set_pixformat(mp_obj_get_int(pixformat_planar), mp_obj_get_int(pixformat_packet)) != 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Pixel format is not supported!"));
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_sensor_set_pixformat_obj,       py_sensor_set_pixformat);

static mp_obj_t py_sensor_get_pixformat() {
    if (sensor.pixformat_packet == PIXFORMAT_INVALID) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Pixel format not set yet!"));
    }
    return mp_obj_new_int(sensor.pixformat_packet);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_sensor_get_pixformat_obj,       py_sensor_get_pixformat);

static mp_obj_t py_sensor_set_framesize(mp_obj_t framesize_planar, mp_obj_t framesize_packet) {
    if (sensor_set_framesize(mp_obj_get_int(framesize_planar), mp_obj_get_int(framesize_packet)) != 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Failed to set framesize!"));
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_sensor_set_framesize_obj,       py_sensor_set_framesize);

static mp_obj_t py_sensor_get_framesize() {
    if (sensor.framesize_packet == FRAMESIZE_INVALID) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Frame size not set yet!"));
    }
    return mp_obj_new_int(sensor.framesize_packet);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_sensor_get_framesize_obj,       py_sensor_get_framesize);

static mp_obj_t py_sensor_set_contrast(mp_obj_t contrast) {
    if (sensor_set_contrast(mp_obj_get_int(contrast)) != 0) {
        return mp_const_false;
    }
    return mp_const_true;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_sensor_set_contrast_obj,        py_sensor_set_contrast);

static mp_obj_t py_sensor_set_brightness(mp_obj_t brightness) {
    if (sensor_set_brightness(mp_obj_get_int(brightness)) != 0) {
        return mp_const_false;
    }
    return mp_const_true;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_sensor_set_brightness_obj,      py_sensor_set_brightness);

static mp_obj_t py_sensor_set_hmirror(mp_obj_t enable) {
    if (sensor_set_hmirror(mp_obj_is_true(enable)) != 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Sensor control failed!"));
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_sensor_set_hmirror_obj,         py_sensor_set_hmirror);

static mp_obj_t py_sensor_get_hmirror() {
    return mp_obj_new_bool(sensor_get_hmirror());
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_sensor_get_hmirror_obj,         py_sensor_get_hmirror);

static mp_obj_t py_sensor_set_vflip(mp_obj_t enable) {
    if (sensor_set_vflip(mp_obj_is_true(enable)) != 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Sensor control failed!"));
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_sensor_set_vflip_obj,           py_sensor_set_vflip);

static mp_obj_t py_sensor_get_vflip() {
    return mp_obj_new_bool(sensor_get_vflip());
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_sensor_get_vflip_obj,           py_sensor_get_vflip);

static mp_obj_t py_sensor_set_special_effect(mp_obj_t sde) {
    if (sensor_set_special_effect(mp_obj_get_int(sde)) != 0) {
        return mp_const_false;
    }
    return mp_const_true;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_sensor_set_special_effect_obj,  py_sensor_set_special_effect);

static mp_obj_t py_sensor_set_color_palette(mp_obj_t palette_obj) {
    int palette = mp_obj_get_int(palette_obj);
    switch (palette) {
        case COLOR_PALETTE_RAINBOW:
            sensor_set_color_palette(rainbow_table);
            break;
        case COLOR_PALETTE_IRONBOW:
            sensor_set_color_palette(ironbow_table);
            break;
        default:
            nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Invalid color palette!"));
            break;
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_sensor_set_color_palette_obj,   py_sensor_set_color_palette);

static mp_obj_t py_sensor_get_color_palette() {
    const uint16_t *palette = sensor_get_color_palette();
    if (palette == rainbow_table) {
        return mp_obj_new_int(COLOR_PALETTE_RAINBOW);
    } else if (palette == ironbow_table) {
        return mp_obj_new_int(COLOR_PALETTE_IRONBOW);
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_sensor_get_color_palette_obj,   py_sensor_get_color_palette);

static mp_obj_t py_sensor_write_reg(mp_obj_t addr, mp_obj_t val) {
    sensor_write_reg(mp_obj_get_int(addr), mp_obj_get_int(val));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_sensor_write_reg_obj,           py_sensor_write_reg);

static mp_obj_t py_sensor_read_reg(mp_obj_t addr) {
    return mp_obj_new_int(sensor_read_reg(mp_obj_get_int(addr)));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_sensor_read_reg_obj,            py_sensor_read_reg);

STATIC const mp_map_elem_t globals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),            MP_OBJ_NEW_QSTR(MP_QSTR_sensor)},

    { MP_OBJ_NEW_QSTR(MP_QSTR_GRAYSCALE),           MP_OBJ_NEW_SMALL_INT(PIXFORMAT_GRAYSCALE)},/* 1BPP/GRAYSCALE*/
    { MP_OBJ_NEW_QSTR(MP_QSTR_RGB565),              MP_OBJ_NEW_SMALL_INT(PIXFORMAT_RGB565)},   /* 2BPP/RGB565*/
    { MP_OBJ_NEW_QSTR(MP_QSTR_YUV422),              MP_OBJ_NEW_SMALL_INT(PIXFORMAT_YUV422)},   /* 2BPP/YUV422*/
    { MP_OBJ_NEW_QSTR(MP_QSTR_YUV422P),             MP_OBJ_NEW_SMALL_INT(PLANAR_PIXFORMAT_YUV422)},/* 2BPP/YUV422 planar*/
    { MP_OBJ_NEW_QSTR(MP_QSTR_YUV420P),             MP_OBJ_NEW_SMALL_INT(PLANAR_PIXFORMAT_YUV420)},   /* 2BPP/YUV420 planar*/
    { MP_OBJ_NEW_QSTR(MP_QSTR_YUV420MB),            MP_OBJ_NEW_SMALL_INT(PLANAR_PIXFORMAT_YUV420MB)},   /* 2BPP/YUV420 marco block*/

    { MP_OBJ_NEW_QSTR(MP_QSTR_NT99141),             MP_OBJ_NEW_SMALL_INT(NT99141_ID)},

    // Special effects
    { MP_OBJ_NEW_QSTR(MP_QSTR_NORMAL),              MP_OBJ_NEW_SMALL_INT(SDE_NORMAL)},          /* Normal/No SDE */
    { MP_OBJ_NEW_QSTR(MP_QSTR_NEGATIVE),            MP_OBJ_NEW_SMALL_INT(SDE_NEGATIVE)},        /* Negative image */

    // C/SIF Resolutions
    { MP_OBJ_NEW_QSTR(MP_QSTR_QQCIF),               MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QQCIF)},    /* 88x72     */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QCIF),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QCIF)},     /* 176x144   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_CIF),                 MP_OBJ_NEW_SMALL_INT(FRAMESIZE_CIF)},      /* 352x288   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QQSIF),               MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QQSIF)},    /* 88x60     */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QSIF),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QSIF)},     /* 176x120   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_SIF),                 MP_OBJ_NEW_SMALL_INT(FRAMESIZE_SIF)},      /* 352x240   */
    // VGA Resolutions
    { MP_OBJ_NEW_QSTR(MP_QSTR_QQQQVGA),             MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QQQQVGA)},  /* 40x30     */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QQQVGA),              MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QQQVGA)},   /* 80x60     */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QQVGA),               MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QQVGA)},    /* 160x120   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QVGA),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QVGA)},     /* 320x240   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_VGA),                 MP_OBJ_NEW_SMALL_INT(FRAMESIZE_VGA)},      /* 640x480   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_HQQQVGA),             MP_OBJ_NEW_SMALL_INT(FRAMESIZE_HQQQVGA)},  /* 80x40     */
    { MP_OBJ_NEW_QSTR(MP_QSTR_HQQVGA),              MP_OBJ_NEW_SMALL_INT(FRAMESIZE_HQQVGA)},   /* 160x80    */
    { MP_OBJ_NEW_QSTR(MP_QSTR_HQVGA),               MP_OBJ_NEW_SMALL_INT(FRAMESIZE_HQVGA)},    /* 240x160   */
    // FFT Resolutions
    { MP_OBJ_NEW_QSTR(MP_QSTR_B64X32),              MP_OBJ_NEW_SMALL_INT(FRAMESIZE_64X32)},    /* 64x32     */
    { MP_OBJ_NEW_QSTR(MP_QSTR_B64X64),              MP_OBJ_NEW_SMALL_INT(FRAMESIZE_64X64)},    /* 64x64     */
    { MP_OBJ_NEW_QSTR(MP_QSTR_B128X64),             MP_OBJ_NEW_SMALL_INT(FRAMESIZE_128X64)},   /* 128x64    */
    { MP_OBJ_NEW_QSTR(MP_QSTR_B128X128),            MP_OBJ_NEW_SMALL_INT(FRAMESIZE_128X128)},  /* 128x128   */
    // Other
    { MP_OBJ_NEW_QSTR(MP_QSTR_LCD),                 MP_OBJ_NEW_SMALL_INT(FRAMESIZE_LCD)},      /* 128x160   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QQVGA2),              MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QQVGA2)},   /* 128x160   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_WVGA),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_WVGA)},     /* 720x480   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_WVGA2),               MP_OBJ_NEW_SMALL_INT(FRAMESIZE_WVGA2)},    /* 752x480   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_SVGA),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_SVGA)},     /* 800x600   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_XGA),                 MP_OBJ_NEW_SMALL_INT(FRAMESIZE_XGA)},      /* 1024x768  */
    { MP_OBJ_NEW_QSTR(MP_QSTR_SXGA),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_SXGA)},     /* 1280x1024 */
    { MP_OBJ_NEW_QSTR(MP_QSTR_UXGA),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_UXGA)},     /* 1600x1200 */
    { MP_OBJ_NEW_QSTR(MP_QSTR_HD),                  MP_OBJ_NEW_SMALL_INT(FRAMESIZE_HD)},       /* 1280x720  */
    { MP_OBJ_NEW_QSTR(MP_QSTR_FHD),                 MP_OBJ_NEW_SMALL_INT(FRAMESIZE_FHD)},      /* 1920x1080 */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QHD),                 MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QHD)},      /* 2560x1440 */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QXGA),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QXGA)},     /* 2048x1536 */
    { MP_OBJ_NEW_QSTR(MP_QSTR_WQXGA),               MP_OBJ_NEW_SMALL_INT(FRAMESIZE_WQXGA)},    /* 2560x1600 */
    { MP_OBJ_NEW_QSTR(MP_QSTR_WQXGA2),              MP_OBJ_NEW_SMALL_INT(FRAMESIZE_WQXGA2)},   /* 2592x1944 */

    // Color Palettes
    { MP_OBJ_NEW_QSTR(MP_QSTR_PALETTE_RAINBOW),     MP_OBJ_NEW_SMALL_INT(COLOR_PALETTE_RAINBOW)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_PALETTE_IRONBOW),     MP_OBJ_NEW_SMALL_INT(COLOR_PALETTE_IRONBOW)},

	// Sensor Pipt type
    { MP_OBJ_NEW_QSTR(MP_QSTR_PIPE_PLANAR),     	MP_OBJ_NEW_SMALL_INT(eSENSOR_PIPE_PLANAR)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_PIPE_PACKET),     	MP_OBJ_NEW_SMALL_INT(eSENSOR_PIPE_PACKET)},

    // Sensor functions
    { MP_OBJ_NEW_QSTR(MP_QSTR___init__),            (mp_obj_t)&py_sensor__init__obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reset),               (mp_obj_t)&py_sensor_reset_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_shutdown),            (mp_obj_t)&py_sensor_shutdown_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_snapshot),            (mp_obj_t)&py_sensor_snapshot_obj },

    { MP_OBJ_NEW_QSTR(MP_QSTR_planar_fill),			MP_ROM_PTR(&PTR_OBJ(py_sensor_planar_fill)) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_packet_fill),			MP_ROM_PTR(&PTR_OBJ(py_sensor_packet_fill)) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_skip_frames),         (mp_obj_t)&py_sensor_skip_frames_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_width),               (mp_obj_t)&py_sensor_width_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_height),              (mp_obj_t)&py_sensor_height_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_fb),              (mp_obj_t)&py_sensor_get_fb_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_id),              (mp_obj_t)&py_sensor_get_id_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_pixformat),       (mp_obj_t)&py_sensor_set_pixformat_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_pixformat),       (mp_obj_t)&py_sensor_get_pixformat_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_framesize),       (mp_obj_t)&py_sensor_set_framesize_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_framesize),       (mp_obj_t)&py_sensor_get_framesize_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_contrast),        (mp_obj_t)&py_sensor_set_contrast_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_brightness),      (mp_obj_t)&py_sensor_set_brightness_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_hmirror),         (mp_obj_t)&py_sensor_set_hmirror_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_hmirror),         (mp_obj_t)&py_sensor_get_hmirror_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_vflip),           (mp_obj_t)&py_sensor_set_vflip_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_vflip),           (mp_obj_t)&py_sensor_get_vflip_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_special_effect),  (mp_obj_t)&py_sensor_set_special_effect_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_color_palette),   (mp_obj_t)&py_sensor_set_color_palette_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_color_palette),   (mp_obj_t)&py_sensor_get_color_palette_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR___write_reg),         (mp_obj_t)&py_sensor_write_reg_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR___read_reg),          (mp_obj_t)&py_sensor_read_reg_obj },

};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t sensor_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&globals_dict,
};

#endif  //MICROPY_PY_SENSOR
