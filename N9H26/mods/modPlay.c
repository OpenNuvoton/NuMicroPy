/***************************************************************************//**
 * @file     modRecord.c
 * @brief    Reocrd python module function
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////

#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objarray.h"


#if MICROPY_ENABLE_PLAY

#include "Play.h"




///////////////////////////////////////////////////////////////////////////////////////////
// It is mp_record_struct_t object operations to handle record_vin_drv_t and record_ain_drv_t 
// It is following lvgl code
///////////////////////////////////////////////////////////////////////////////////////////

/*
 * Helper functions
 */

// Custom function mp object

typedef struct _mp_play_obj_fun_builtin_var_t {
    mp_obj_base_t base;
    mp_uint_t n_args;
    mp_fun_var_t mp_fun;
    void *play_fun;
} mp_play_obj_fun_builtin_var_t;

STATIC mp_obj_t play_fun_builtin_var_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
STATIC mp_int_t mp_func_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags);

STATIC const mp_obj_type_t mp_play_type_fun_builtin_var = {
    { &mp_type_type },
    .name = MP_QSTR_function,
    .call = play_fun_builtin_var_call,
    .unary_op = mp_generic_unary_op,
    .buffer_p = { .get_buffer = mp_func_get_buffer }
};

STATIC mp_obj_t play_fun_builtin_var_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    assert(MP_OBJ_IS_TYPE(self_in, &mp_lv_type_fun_builtin_var));
    mp_play_obj_fun_builtin_var_t *self = MP_OBJ_TO_PTR(self_in);
    mp_arg_check_num(n_args, n_kw, self->n_args, self->n_args, false);
    return self->mp_fun(n_args, args);
}

STATIC mp_int_t mp_func_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    assert(MP_OBJ_IS_TYPE(self_in, &mp_lv_type_fun_builtin_var));
    mp_play_obj_fun_builtin_var_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->play_fun;
    bufinfo->len = sizeof(self->play_fun);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}

#define MP_DEFINE_CONST_PLAY_FUN_OBJ_VAR(obj_name, n_args, mp_fun, play_fun) \
    const mp_play_obj_fun_builtin_var_t obj_name = \
        {{&mp_play_type_fun_builtin_var}, n_args, mp_fun, play_fun}

// Casting 
typedef struct mp_play_struct_t
{
    mp_obj_base_t base;
    void *data;
} mp_play_struct_t;

STATIC const mp_play_struct_t mp_play_null_obj;

STATIC mp_obj_t get_native_obj(mp_obj_t *mp_obj)
{
    if (!MP_OBJ_IS_OBJ(mp_obj)) return mp_obj;
    const mp_obj_type_t *native_type = ((mp_obj_base_t*)mp_obj)->type;
    if (native_type->parent == NULL) return mp_obj;
    while (native_type->parent) native_type = native_type->parent;
    return mp_instance_cast_to_native_base(mp_obj, MP_OBJ_FROM_PTR(native_type));
}

STATIC mp_obj_t dict_to_struct(mp_obj_t dict, const mp_obj_type_t *type);

STATIC mp_obj_t make_new_play_struct(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *args);

STATIC mp_obj_t *cast(mp_obj_t *mp_obj, const mp_obj_type_t *mp_type)
{
    mp_obj_t *res = NULL;
    if (mp_obj == mp_const_none && mp_type->make_new == &make_new_play_struct) {
        res = MP_OBJ_FROM_PTR(&mp_play_null_obj);
    } else if (MP_OBJ_IS_OBJ(mp_obj)) {
        res = get_native_obj(mp_obj);
        if (res){
            const mp_obj_type_t *res_type = ((mp_obj_base_t*)res)->type;
            if (res_type != mp_type){
                if (res_type == &mp_type_dict &&
                    mp_type->make_new == &make_new_play_struct)
                        res = dict_to_struct(res, mp_type);
                else res = NULL;
            }
        }
    }
    if (res == NULL) nlr_raise(
        mp_obj_new_exception_msg_varg(
            &mp_type_SyntaxError, MP_ERROR_TEXT("Can't convert %s to %s!"), mp_obj_get_type_str(mp_obj), qstr_str(mp_type->name)));
    return res;
}

// Convert dict to struct

STATIC mp_obj_t dict_to_struct(mp_obj_t dict, const mp_obj_type_t *type)
{
    mp_obj_t mp_struct = make_new_play_struct(type, 0, 0, NULL);
    mp_obj_t *native_dict = cast(dict, &mp_type_dict);
    mp_map_t *map = mp_obj_dict_get_map(native_dict);
    if (map == NULL) return mp_const_none;
    for (uint i = 0; i < map->alloc; i++) {
        mp_obj_t key = map->table[i].key;
        mp_obj_t value = map->table[i].value;
        if (key != MP_OBJ_NULL) {
            mp_obj_t dest[] = {MP_OBJ_SENTINEL, value};
            type->attr(mp_struct, mp_obj_str_get_qstr(key), dest);
            if (dest[0]) nlr_raise(
                mp_obj_new_exception_msg_varg(
                    &mp_type_SyntaxError, MP_ERROR_TEXT("Cannot set field %s on struct %s!"), qstr_str(mp_obj_str_get_qstr(key)), qstr_str(type->name)));
        }
    }
    return mp_struct;
}

// struct handling

STATIC inline mp_play_struct_t *mp_to_play_struct(mp_obj_t mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;
    if (!MP_OBJ_IS_OBJ(mp_obj)) nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Struct argument is not an object!")));
    mp_play_struct_t *mp_play_struct = MP_OBJ_TO_PTR(get_native_obj(mp_obj));
    return mp_play_struct;
}

STATIC inline size_t get_play_struct_size(const mp_obj_type_t *type)
{
    mp_obj_t size_obj = mp_obj_dict_get(type->locals_dict, MP_OBJ_NEW_QSTR(MP_QSTR_SIZE));
    return (size_t)mp_obj_get_int(size_obj);
}

STATIC const mp_obj_fun_builtin_var_t mp_play_dereference_obj;

// Reference an existing play struct (or part of it)

STATIC mp_obj_t play_to_mp_struct(const mp_obj_type_t *type, void *play_struct)
{
    if (play_struct == NULL) return mp_const_none;
    mp_play_struct_t *self = m_new_obj(mp_play_struct_t);
    *self = (mp_play_struct_t){
        .base = {type},
        .data = play_struct
    };
    return MP_OBJ_FROM_PTR(self);
}

// Convert mp object to ptr

STATIC void* mp_to_ptr(mp_obj_t self_in)
{
    mp_buffer_info_t buffer_info;
    if (self_in == mp_const_none)
        return NULL;

//    if (MP_OBJ_IS_INT(self_in))
//        return (void*)mp_obj_get_int(self_in);

    if (!mp_get_buffer(self_in, &buffer_info, MP_BUFFER_READ)) {
        // No buffer protocol - this is not a Struct or a Blob, it's some other mp object.
        // We only allow setting dict directly, since it's useful to setting user_data for passing data to C.
        // On other cases throw an exception, to avoid a crash later
        if (MP_OBJ_IS_TYPE(self_in, &mp_type_dict))
            return MP_OBJ_TO_PTR(self_in);
        else nlr_raise(
                mp_obj_new_exception_msg_varg(
                    &mp_type_SyntaxError, MP_ERROR_TEXT("Cannot convert '%s' to pointer!"), mp_obj_get_type_str(self_in)));
    }

    if (MP_OBJ_IS_STR_OR_BYTES(self_in) || 
        MP_OBJ_IS_TYPE(self_in, &mp_type_bytearray) ||
        MP_OBJ_IS_TYPE(self_in, &mp_type_memoryview))
            return buffer_info.buf;
    else
    {
        void *result;
        if (buffer_info.len != sizeof(result) || buffer_info.typecode != BYTEARRAY_TYPECODE){
            nlr_raise(
                mp_obj_new_exception_msg_varg(
                    &mp_type_SyntaxError, MP_ERROR_TEXT("Cannot convert %s to pointer! (buffer does not represent a pointer)"), mp_obj_get_type_str(self_in)));
        }
        memcpy(&result, buffer_info.buf, sizeof(result));
        return result;
    }
}

// Blob is a wrapper for void* 

STATIC void mp_blob_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{
    mp_printf(print, "Blob");
}

STATIC mp_int_t mp_blob_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    mp_play_struct_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->data;
    bufinfo->len = sizeof(self->data);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}

// Sometimes (but not always!) Blob represents a Micropython object.
// In such cases it's safe to cast the Blob back to the Micropython object
// cast argument is the underlying object type, and it's optional.

STATIC mp_obj_t mp_blob_cast(size_t argc, const mp_obj_t *argv)
{
    mp_obj_t self = argv[0];
    void *ptr = mp_to_ptr(self);
    if (argc == 1) return MP_OBJ_FROM_PTR(ptr);
    mp_obj_t type = argv[1];
    if (!MP_OBJ_IS_TYPE(type, &mp_type_type))
        nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Cast argument must be a type!")));
    return cast(MP_OBJ_FROM_PTR(ptr), type);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_blob_cast_obj, 1, 2, mp_blob_cast);

STATIC const mp_rom_map_elem_t mp_blob_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___dereference__), MP_ROM_PTR(&mp_play_dereference_obj) },
    { MP_ROM_QSTR(MP_QSTR_cast), MP_ROM_PTR(&mp_blob_cast_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_blob_locals_dict, mp_blob_locals_dict_table);

STATIC const mp_obj_type_t mp_blob_type = {
    { &mp_type_type },
    .name = MP_QSTR_Blob,
    .print = mp_blob_print,
    //.make_new = make_new_blob,
    .locals_dict = (mp_obj_dict_t*)&mp_blob_locals_dict,
    .buffer_p = { .get_buffer = mp_blob_get_buffer }
};

STATIC inline mp_obj_t ptr_to_mp(void *data)
{
    return play_to_mp_struct(&mp_blob_type, data);
}

///////////////////////////////////////////////////////////////////////

STATIC mp_obj_t make_new_play_struct(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *args)
{
    if ((!MP_OBJ_IS_TYPE(type, &mp_type_type)) || type->make_new != &make_new_play_struct)
        nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Argument is not a struct type!")));
    size_t size = get_play_struct_size(type);
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_play_struct_t *self = m_new_obj(mp_play_struct_t);
    *self = (mp_play_struct_t){
        .base = {type}, 
        .data = m_malloc(size)
    };
    mp_play_struct_t *other = n_args > 0? mp_to_play_struct(cast(args[0], type)): NULL;
    if (other) {
        memcpy(self->data, other->data, size);
    } else {
        memset(self->data, 0, size);
    }
    return MP_OBJ_FROM_PTR(self);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// For video-out driver operation
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Struct play_vout_drv_t
 */

STATIC inline const mp_obj_type_t *get_mp_play_vout_drv_t_type();

STATIC inline play_vout_drv_t* mp_write_ptr_play_vout_drv_t(mp_obj_t self_in)
{
    mp_play_struct_t *self = MP_OBJ_TO_PTR(cast(self_in, get_mp_play_vout_drv_t_type()));
    return (play_vout_drv_t*)self->data;
}

STATIC void mp_play_vout_drv_t_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_play_struct_t *self = MP_OBJ_TO_PTR(self_in);
    play_vout_drv_t *data = (play_vout_drv_t*)self->data;

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        switch(attr)
        {
			case MP_QSTR_width: dest[0]  = mp_obj_new_int_from_uint(data->u32OutputWidth); break;
			case MP_QSTR_height: dest[0]  = mp_obj_new_int_from_uint(data->u32OutputHeight); break;
			case MP_QSTR_vout_obj: dest[0]  = data->vout_obj; break;
            case MP_QSTR_flush_cb: dest[0] = ptr_to_mp((void*)data->pfn_vout_flush_cb); break;
        }
    } else {
        if (dest[1])
        {
            // store attribute
            switch(attr)
            {
                case MP_QSTR_width: data->u32OutputWidth = (uint32_t)mp_obj_get_int(dest[1]); break; // converting to uint32_t;
                case MP_QSTR_height: data->u32OutputHeight = (uint32_t)mp_obj_get_int(dest[1]); break; // converting to uint32_t;
				case MP_QSTR_vout_obj: data->vout_obj = dest[1]; break;
                case MP_QSTR_flush_cb: data->pfn_vout_flush_cb = (PFN_VOUT_FLUSH_CB)mp_to_ptr(dest[1]); break;
                default: return;
            }

            dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
}

STATIC void mp_play_vout_drv_t_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{
    mp_printf(print, "struct play_vout_drv_t");
}

STATIC const mp_rom_map_elem_t mp_play_vout_drv_t_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_SIZE), MP_ROM_PTR(MP_ROM_INT(sizeof(play_vout_drv_t))) },
//    { MP_ROM_QSTR(MP_QSTR_cast), MP_ROM_PTR(&mp_lv_cast_class_method) },
//    { MP_ROM_QSTR(MP_QSTR_cast_instance), MP_ROM_PTR(&mp_lv_cast_instance_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_play_vout_drv_t_locals_dict, mp_play_vout_drv_t_locals_dict_table);

STATIC const mp_obj_type_t mp_play_vout_drv_t_type = {
    { &mp_type_type },
    .name = MP_QSTR_play_vout_drv_t,
    .print = mp_play_vout_drv_t_print,
    .make_new = make_new_play_struct,
    .locals_dict = (mp_obj_dict_t*)&mp_play_vout_drv_t_locals_dict,
    .attr = mp_play_vout_drv_t_attr,
    .buffer_p = { .get_buffer = mp_blob_get_buffer }
};

STATIC inline const mp_obj_type_t *get_mp_play_vout_drv_t_type()
{
    return &mp_play_vout_drv_t_type;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// For audio-out driver object operation
////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Struct play_aout_drv_t
 */

STATIC inline const mp_obj_type_t *get_mp_play_aout_drv_t_type();

STATIC inline play_aout_drv_t* mp_write_ptr_play_aout_drv_t(mp_obj_t self_in)
{
    mp_play_struct_t *self = MP_OBJ_TO_PTR(cast(self_in, get_mp_play_aout_drv_t_type()));
    return (play_aout_drv_t*)self->data;
}

STATIC void mp_play_aout_drv_t_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_play_struct_t *self = MP_OBJ_TO_PTR(self_in);
    play_aout_drv_t *data = (play_aout_drv_t*)self->data;

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        switch(attr)
        {
            case MP_QSTR_flush_cb: dest[0] = ptr_to_mp((void*)data->pfn_aout_flush_cb); break;
        }
    } else {
        if (dest[1])
        {
            // store attribute
            switch(attr)
            {
                case MP_QSTR_flush_cb: data->pfn_aout_flush_cb = (PFN_AOUT_FLUSH_CB)mp_to_ptr(dest[1]); break;
                default: return;
            }

            dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
}

STATIC void mp_play_aout_drv_t_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{
    mp_printf(print, "struct record_ain_drv_t");
}

STATIC const mp_rom_map_elem_t mp_play_aout_drv_t_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_SIZE), MP_ROM_PTR(MP_ROM_INT(sizeof(play_aout_drv_t))) },
//    { MP_ROM_QSTR(MP_QSTR_cast), MP_ROM_PTR(&mp_lv_cast_class_method) },
//    { MP_ROM_QSTR(MP_QSTR_cast_instance), MP_ROM_PTR(&mp_lv_cast_instance_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_play_aout_drv_t_locals_dict, mp_play_aout_drv_t_locals_dict_table);

STATIC const mp_obj_type_t mp_play_aout_drv_t_type = {
    { &mp_type_type },
    .name = MP_QSTR_play_aout_drv_t,
    .print = mp_play_aout_drv_t_print,
    .make_new = make_new_play_struct,
    .locals_dict = (mp_obj_dict_t*)&mp_play_aout_drv_t_locals_dict,
    .attr = mp_play_aout_drv_t_attr,
    .buffer_p = { .get_buffer = mp_blob_get_buffer }
};

STATIC inline const mp_obj_type_t *get_mp_play_aout_drv_t_type()
{
    return &mp_play_aout_drv_t_type;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// For video information object operation
////////////////////////////////////////////////////////////////////////////////////////////

typedef struct{
    mp_obj_base_t base;
	play_video_info_t sVideoInfo;
} mp_obj_play_video_info_t;

STATIC void mp_play_video_info_t_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
#if 0
    mp_play_struct_t *self = MP_OBJ_TO_PTR(self_in);
    play_video_info_t *data = (play_video_info_t*)self->data;

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        switch(attr)
        {
			case MP_QSTR_width: dest[0] = mp_obj_new_int_from_uint(data->u32ImgWidth); break;
			case MP_QSTR_height: dest[0] = mp_obj_new_int_from_uint(data->u32ImgHeight); break;			
        }
    } else {
        if (dest[1])
        {
            // store attribute
            switch(attr)
            {
                case MP_QSTR_width: data->u32ImgWidth = (uint32_t)mp_obj_get_int(dest[1]); break;
                case MP_QSTR_height: data->u32ImgHeight = (uint32_t)mp_obj_get_int(dest[1]); break;
                default: return;
            }

            dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
#else
    if (dest[0] != MP_OBJ_NULL) {
        // not load attribute
        return;
    }
    mp_obj_play_video_info_t *self = MP_OBJ_TO_PTR(self_in);
    if (attr == MP_QSTR_width) {
        dest[0] = mp_obj_new_int_from_uint(self->sVideoInfo.u32ImgWidth);
    } else if (attr == MP_QSTR_height) {
        dest[0] = mp_obj_new_int_from_uint(self->sVideoInfo.u32ImgHeight);
    }
#endif

}

STATIC void mp_play_video_info_t_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{
    mp_printf(print, "struct play_video_info_t");
}

#if 0
STATIC const mp_rom_map_elem_t mp_play_video_info_t_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_SIZE), MP_ROM_PTR(MP_ROM_INT(sizeof(play_video_info_t))) },
};

STATIC MP_DEFINE_CONST_DICT(mp_play_video_info_t_locals_dict, mp_play_video_info_t_locals_dict_table);
#endif

STATIC const mp_obj_type_t mp_play_video_info_t_type = {
    { &mp_type_type },
    .name = MP_QSTR_play_video_info_t,
    .print = mp_play_video_info_t_print,
//    .make_new = make_new_play_struct,
//    .locals_dict = (mp_obj_dict_t*)&mp_play_video_info_t_locals_dict,
    .attr = mp_play_video_info_t_attr,
//    .buffer_p = { .get_buffer = mp_blob_get_buffer }
};

mp_obj_t mp_obj_new_play_video_info(void) {
    mp_obj_play_video_info_t *o = m_new_obj(mp_obj_play_video_info_t);
    o->base.type = &mp_play_video_info_t_type;
    o->sVideoInfo.u32ImgHeight = 0;
    o->sVideoInfo.u32ImgWidth = 0;    
    return MP_OBJ_FROM_PTR(o);
}


//////////////////////////////////////////////////////////////////////////////////////////////
// For audio information object operation
////////////////////////////////////////////////////////////////////////////////////////////

typedef struct{
    mp_obj_base_t base;
	play_audio_info_t sAudioInfo;
} mp_obj_play_audio_info_t;

STATIC void mp_play_audio_info_t_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
#if 0
    mp_play_struct_t *self = MP_OBJ_TO_PTR(self_in);
    play_audio_info_t *data = (play_audio_info_t*)self->data;

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        switch(attr)
        {
			case MP_QSTR_samplerate: dest[0] = mp_obj_new_int_from_uint(data->u32SampleRate); break;
			case MP_QSTR_channel: dest[0] = mp_obj_new_int_from_uint(data->u32Channel); break;			
        }
    } else {
        if (dest[1])
        {
            // store attribute
            switch(attr)
            {
                case MP_QSTR_samplerate: data->u32SampleRate = (uint32_t)mp_obj_get_int(dest[1]); break; // converting to uint16_t;
                case MP_QSTR_channel: data->u32Channel = (uint32_t)mp_obj_get_int(dest[1]); break; // converting to uint8_t;
                default: return;
            }

            dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
#else
    if (dest[0] != MP_OBJ_NULL) {
        // not load attribute
        return;
    }
    mp_obj_play_audio_info_t *self = MP_OBJ_TO_PTR(self_in);
    if (attr == MP_QSTR_samplerate) {
        dest[0] = mp_obj_new_int_from_uint(self->sAudioInfo.u32SampleRate);
    } else if (attr == MP_QSTR_channel) {
        dest[0] = mp_obj_new_int_from_uint(self->sAudioInfo.u32Channel);
    }

#endif
}

STATIC void mp_play_audio_info_t_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{
    mp_printf(print, "struct play_audio_info_t");
}

#if 0
STATIC const mp_rom_map_elem_t mp_play_audio_info_t_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_SIZE), MP_ROM_PTR(MP_ROM_INT(sizeof(play_audio_info_t))) },
};

STATIC MP_DEFINE_CONST_DICT(mp_play_audio_info_t_locals_dict, mp_play_audio_info_t_locals_dict_table);
#endif

STATIC const mp_obj_type_t mp_play_audio_info_t_type = {
    { &mp_type_type },
    .name = MP_QSTR_play_audio_info_t,
    .print = mp_play_audio_info_t_print,
//    .make_new = make_new_play_struct,
//    .locals_dict = (mp_obj_dict_t*)&mp_play_audio_info_t_locals_dict,
    .attr = mp_play_audio_info_t_attr,
//    .buffer_p = { .get_buffer = mp_blob_get_buffer }
};

mp_obj_t mp_obj_new_play_audio_info(void) {
    mp_obj_play_audio_info_t *o = m_new_obj(mp_obj_play_audio_info_t);
    o->base.type = &mp_play_audio_info_t_type;
    o->sAudioInfo.u32SampleRate = 0;
    o->sAudioInfo.u32Channel = 0;    
    return MP_OBJ_FROM_PTR(o);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Micropython binding function
///////////////////////////////////////////////////////////////////////////////////////////////

#define RAISE_OS_EXCEPTION(msg)     nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, msg))

static mp_obj_play_audio_info_t *s_psAudioInfoObj = NULL;
static mp_obj_play_video_info_t *s_psVideoInfoObj = NULL;
static mp_obj_t s_sMediaInfoCBObj = mp_const_none;

STATIC mp_obj_t mp_play_vout_drv_register(size_t mp_n_args, const mp_obj_t *mp_args)
{
    play_vout_drv_t *driver = mp_write_ptr_play_vout_drv_t(mp_args[0]);
    Play_RegisterVoutDrv(driver);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_PLAY_FUN_OBJ_VAR(mp_play_vout_drv_register_obj, 1, mp_play_vout_drv_register, Play_RegisterVoutDrv);

STATIC mp_obj_t mp_play_aout_drv_register(size_t mp_n_args, const mp_obj_t *mp_args)
{
    play_aout_drv_t *driver = mp_write_ptr_play_aout_drv_t(mp_args[0]);
    Play_RegisterAoutDrv(driver);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_PLAY_FUN_OBJ_VAR(mp_play_aout_drv_register_obj, 1, mp_play_aout_drv_register, Play_RegisterAoutDrv);

static mp_obj_t py_play_init(mp_obj_t callback_obj)
{
	//TODO: if callback_obj is null, we will use ptr obj for application to receive audio data
    s_sMediaInfoCBObj = callback_obj;

    if (!mp_obj_is_callable(s_sMediaInfoCBObj)) {
        s_sMediaInfoCBObj = mp_const_none;
        RAISE_OS_EXCEPTION("Invalid callback object!");
		goto py_play_init_done;
    }

	if(s_psAudioInfoObj == NULL){
		s_psAudioInfoObj = mp_obj_new_play_audio_info();
	}

	if(s_psVideoInfoObj == NULL){
		s_psVideoInfoObj = mp_obj_new_play_video_info();
	}

	//TODO: init player

py_play_init_done:

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_play_init_obj, py_play_init);


static const mp_rom_map_elem_t Play_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_ROM_QSTR(MP_QSTR_Play)               },
    { MP_ROM_QSTR(MP_QSTR_init),            MP_ROM_PTR(&py_play_init_obj)           },
//    { MP_ROM_QSTR(MP_QSTR_start_play),		MP_ROM_PTR(&py_play_start_play_obj)},
//    { MP_ROM_QSTR(MP_QSTR_stop_play),  		MP_ROM_PTR(&py_play_stop_play_obj) },
//    { MP_ROM_QSTR(MP_QSTR_deinit),			MP_ROM_PTR(&py_play_deinit_obj) },


	//audio-out and video-out driver callback
	{ MP_ROM_QSTR(MP_QSTR_vout_drv_t), MP_ROM_PTR(&mp_play_vout_drv_t_type) },
	{ MP_ROM_QSTR(MP_QSTR_aout_drv_t), MP_ROM_PTR(&mp_play_aout_drv_t_type) },

    { MP_ROM_QSTR(MP_QSTR_vout_drv_register), MP_ROM_PTR(&mp_play_vout_drv_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_aout_drv_register), MP_ROM_PTR(&mp_play_aout_drv_register_obj) },

};

STATIC MP_DEFINE_CONST_DICT(Play_module_globals, Play_globals_table);

const mp_obj_module_t mp_module_Play = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&Play_module_globals,
};

#endif
