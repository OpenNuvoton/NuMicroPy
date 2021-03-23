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


#if MICROPY_ENABLE_RECORD

#include "Record.h"

///////////////////////////////////////////////////////////////////////////////////////////
// It is mp_record_struct_t object operations to handle record_vin_drv_t and record_ain_drv_t 
// It is following lvgl code
///////////////////////////////////////////////////////////////////////////////////////////

/*
 * Helper functions
 */

// Custom function mp object

typedef struct _mp_record_obj_fun_builtin_var_t {
    mp_obj_base_t base;
    mp_uint_t n_args;
    mp_fun_var_t mp_fun;
    void *rec_fun;
} mp_record_obj_fun_builtin_var_t;

STATIC mp_obj_t record_fun_builtin_var_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
STATIC mp_int_t mp_func_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags);

STATIC const mp_obj_type_t mp_record_type_fun_builtin_var = {
    { &mp_type_type },
    .name = MP_QSTR_function,
    .call = record_fun_builtin_var_call,
    .unary_op = mp_generic_unary_op,
    .buffer_p = { .get_buffer = mp_func_get_buffer }
};

STATIC mp_obj_t record_fun_builtin_var_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    assert(MP_OBJ_IS_TYPE(self_in, &mp_lv_type_fun_builtin_var));
    mp_record_obj_fun_builtin_var_t *self = MP_OBJ_TO_PTR(self_in);
    mp_arg_check_num(n_args, n_kw, self->n_args, self->n_args, false);
    return self->mp_fun(n_args, args);
}

STATIC mp_int_t mp_func_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    assert(MP_OBJ_IS_TYPE(self_in, &mp_lv_type_fun_builtin_var));
    mp_record_obj_fun_builtin_var_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->rec_fun;
    bufinfo->len = sizeof(self->rec_fun);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}

#define MP_DEFINE_CONST_RECORD_FUN_OBJ_VAR(obj_name, n_args, mp_fun, recrod_fun) \
    const mp_record_obj_fun_builtin_var_t obj_name = \
        {{&mp_record_type_fun_builtin_var}, n_args, mp_fun, recrod_fun}


// Casting 
typedef struct mp_record_struct_t
{
    mp_obj_base_t base;
    void *data;
} mp_record_struct_t;

STATIC const mp_record_struct_t mp_record_null_obj;

STATIC mp_obj_t get_native_obj(mp_obj_t *mp_obj)
{
    if (!MP_OBJ_IS_OBJ(mp_obj)) return mp_obj;
    const mp_obj_type_t *native_type = ((mp_obj_base_t*)mp_obj)->type;
    if (native_type->parent == NULL) return mp_obj;
    while (native_type->parent) native_type = native_type->parent;
    return mp_instance_cast_to_native_base(mp_obj, MP_OBJ_FROM_PTR(native_type));
}

STATIC mp_obj_t dict_to_struct(mp_obj_t dict, const mp_obj_type_t *type);

STATIC mp_obj_t make_new_record_struct(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *args);

STATIC mp_obj_t *cast(mp_obj_t *mp_obj, const mp_obj_type_t *mp_type)
{
    mp_obj_t *res = NULL;
    if (mp_obj == mp_const_none && mp_type->make_new == &make_new_record_struct) {
        res = MP_OBJ_FROM_PTR(&mp_record_null_obj);
    } else if (MP_OBJ_IS_OBJ(mp_obj)) {
        res = get_native_obj(mp_obj);
        if (res){
            const mp_obj_type_t *res_type = ((mp_obj_base_t*)res)->type;
            if (res_type != mp_type){
                if (res_type == &mp_type_dict &&
                    mp_type->make_new == &make_new_record_struct)
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
    mp_obj_t mp_struct = make_new_record_struct(type, 0, 0, NULL);
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

STATIC inline mp_record_struct_t *mp_to_record_struct(mp_obj_t mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;
    if (!MP_OBJ_IS_OBJ(mp_obj)) nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Struct argument is not an object!")));
    mp_record_struct_t *mp_reocrd_struct = MP_OBJ_TO_PTR(get_native_obj(mp_obj));
    return mp_reocrd_struct;
}

STATIC inline size_t get_record_struct_size(const mp_obj_type_t *type)
{
    mp_obj_t size_obj = mp_obj_dict_get(type->locals_dict, MP_OBJ_NEW_QSTR(MP_QSTR_SIZE));
    return (size_t)mp_obj_get_int(size_obj);
}

STATIC const mp_obj_fun_builtin_var_t mp_record_dereference_obj;

// Reference an existing record struct (or part of it)

STATIC mp_obj_t record_to_mp_struct(const mp_obj_type_t *type, void *record_struct)
{
    if (record_struct == NULL) return mp_const_none;
    mp_record_struct_t *self = m_new_obj(mp_record_struct_t);
    *self = (mp_record_struct_t){
        .base = {type},
        .data = record_struct
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
    mp_record_struct_t *self = MP_OBJ_TO_PTR(self_in);

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
    { MP_ROM_QSTR(MP_QSTR___dereference__), MP_ROM_PTR(&mp_record_dereference_obj) },
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
    return record_to_mp_struct(&mp_blob_type, data);
}


//////////////////////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t make_new_record_struct(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *args)
{
    if ((!MP_OBJ_IS_TYPE(type, &mp_type_type)) || type->make_new != &make_new_record_struct)
        nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Argument is not a struct type!")));
    size_t size = get_record_struct_size(type);
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_record_struct_t *self = m_new_obj(mp_record_struct_t);
    *self = (mp_record_struct_t){
        .base = {type}, 
        .data = m_malloc(size)
    };
    mp_record_struct_t *other = n_args > 0? mp_to_record_struct(cast(args[0], type)): NULL;
    if (other) {
        memcpy(self->data, other->data, size);
    } else {
        memset(self->data, 0, size);
    }
    return MP_OBJ_FROM_PTR(self);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// For video-in driver operation
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Struct record_vin_drv_t
 */

STATIC inline const mp_obj_type_t *get_mp_record_vin_drv_t_type();

STATIC inline record_vin_drv_t* mp_write_ptr_record_vin_drv_t(mp_obj_t self_in)
{
    mp_record_struct_t *self = MP_OBJ_TO_PTR(cast(self_in, get_mp_record_vin_drv_t_type()));
    return (record_vin_drv_t*)self->data;
}

STATIC void mp_record_vin_drv_t_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_record_struct_t *self = MP_OBJ_TO_PTR(self_in);
    record_vin_drv_t *data = (record_vin_drv_t*)self->data;

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        switch(attr)
        {
			case MP_QSTR_width: dest[0] = mp_obj_new_int_from_uint(data->u32Width); break;
			case MP_QSTR_height: dest[0] = mp_obj_new_int_from_uint(data->u32Height); break;			
			case MP_QSTR_pixel_format: dest[0] = mp_obj_new_int_from_uint(data->ePixelFormat); break;
            case MP_QSTR_planar_img_fill_cb: dest[0] = ptr_to_mp((void*)data->pfn_vin_planar_img_fill_cb); break;
            case MP_QSTR_packet_img_fill_cb: dest[0] = ptr_to_mp((void*)data->pfn_vin_packet_img_fill_cb); break;
        }
    } else {
        if (dest[1])
        {
            // store attribute
            switch(attr)
            {
                case MP_QSTR_width: data->u32Width = (uint32_t)mp_obj_get_int(dest[1]); break;
                case MP_QSTR_height: data->u32Height = (uint32_t)mp_obj_get_int(dest[1]); break;
				case MP_QSTR_pixel_format: data->ePixelFormat = (uint32_t)mp_obj_get_int(dest[1]); break;
                case MP_QSTR_planar_img_fill_cb: data->pfn_vin_planar_img_fill_cb = (PFN_VIN_FILL_CB)mp_to_ptr(dest[1]); break;
                case MP_QSTR_packet_img_fill_cb: data->pfn_vin_packet_img_fill_cb = (PFN_VIN_FILL_CB)mp_to_ptr(dest[1]); break;
                default: return;
            }

            dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
}

STATIC void mp_record_vin_drv_t_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{
    mp_printf(print, "struct record_vin_drv_t");
}

STATIC const mp_rom_map_elem_t mp_record_vin_drv_t_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_SIZE), MP_ROM_PTR(MP_ROM_INT(sizeof(record_vin_drv_t))) },
//    { MP_ROM_QSTR(MP_QSTR_cast), MP_ROM_PTR(&mp_lv_cast_class_method) },
//    { MP_ROM_QSTR(MP_QSTR_cast_instance), MP_ROM_PTR(&mp_lv_cast_instance_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_record_vin_drv_t_locals_dict, mp_record_vin_drv_t_locals_dict_table);

STATIC const mp_obj_type_t mp_record_vin_drv_t_type = {
    { &mp_type_type },
    .name = MP_QSTR_record_vin_drv_t,
    .print = mp_record_vin_drv_t_print,
    .make_new = make_new_record_struct,
    .locals_dict = (mp_obj_dict_t*)&mp_record_vin_drv_t_locals_dict,
    .attr = mp_record_vin_drv_t_attr,
    .buffer_p = { .get_buffer = mp_blob_get_buffer }
};

STATIC inline const mp_obj_type_t *get_mp_record_vin_drv_t_type()
{
    return &mp_record_vin_drv_t_type;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// For video-out driver operation
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Struct record_vout_drv_t
 */

STATIC inline const mp_obj_type_t *get_mp_record_vout_drv_t_type();

STATIC inline record_vout_drv_t* mp_write_ptr_record_vout_drv_t(mp_obj_t self_in)
{
    mp_record_struct_t *self = MP_OBJ_TO_PTR(cast(self_in, get_mp_record_vout_drv_t_type()));
    return (record_vout_drv_t*)self->data;
}

STATIC void mp_record_vout_drv_t_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_record_struct_t *self = MP_OBJ_TO_PTR(self_in);
    record_vout_drv_t *data = (record_vout_drv_t*)self->data;

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        switch(attr)
        {
			case MP_QSTR_vout_obj: dest[0]  = data->vout_obj; break;
            case MP_QSTR_flush_cb: dest[0] = ptr_to_mp((void*)data->pfn_vout_flush_cb); break;
        }
    } else {
        if (dest[1])
        {
            // store attribute
            switch(attr)
            {
				case MP_QSTR_vout_obj: data->vout_obj = dest[1]; break;
                case MP_QSTR_flush_cb: data->pfn_vout_flush_cb = (PFN_VOUT_FLUSH_CB)mp_to_ptr(dest[1]); break;
                default: return;
            }

            dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
}

STATIC void mp_record_vout_drv_t_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{
    mp_printf(print, "struct record_vout_drv_t");
}

STATIC const mp_rom_map_elem_t mp_record_vout_drv_t_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_SIZE), MP_ROM_PTR(MP_ROM_INT(sizeof(record_vout_drv_t))) },
//    { MP_ROM_QSTR(MP_QSTR_cast), MP_ROM_PTR(&mp_lv_cast_class_method) },
//    { MP_ROM_QSTR(MP_QSTR_cast_instance), MP_ROM_PTR(&mp_lv_cast_instance_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_record_vout_drv_t_locals_dict, mp_record_vout_drv_t_locals_dict_table);

STATIC const mp_obj_type_t mp_record_vout_drv_t_type = {
    { &mp_type_type },
    .name = MP_QSTR_record_vout_drv_t,
    .print = mp_record_vout_drv_t_print,
    .make_new = make_new_record_struct,
    .locals_dict = (mp_obj_dict_t*)&mp_record_vout_drv_t_locals_dict,
    .attr = mp_record_vout_drv_t_attr,
    .buffer_p = { .get_buffer = mp_blob_get_buffer }
};

STATIC inline const mp_obj_type_t *get_mp_record_vout_drv_t_type()
{
    return &mp_record_vout_drv_t_type;
}



//////////////////////////////////////////////////////////////////////////////////////////////
// For audio-in driver object operation
////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Struct record_ain_drv_t
 */

STATIC inline const mp_obj_type_t *get_mp_record_ain_drv_t_type();

STATIC inline record_ain_drv_t* mp_write_ptr_record_ain_drv_t(mp_obj_t self_in)
{
    mp_record_struct_t *self = MP_OBJ_TO_PTR(cast(self_in, get_mp_record_ain_drv_t_type()));
    return (record_ain_drv_t*)self->data;
}

STATIC void mp_record_ain_drv_t_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_record_struct_t *self = MP_OBJ_TO_PTR(self_in);
    record_ain_drv_t *data = (record_ain_drv_t*)self->data;

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        switch(attr)
        {
			case MP_QSTR_samplerate: dest[0] = mp_obj_new_int_from_uint(data->u32SampleRate); break;
			case MP_QSTR_channel: dest[0] = mp_obj_new_int_from_uint(data->u32Channel); break;			
            case MP_QSTR_fill_cb: dest[0] = ptr_to_mp((void*)data->pfn_ain_fill_cb); break;
        }
    } else {
        if (dest[1])
        {
            // store attribute
            switch(attr)
            {
                case MP_QSTR_samplerate: data->u32SampleRate = (uint32_t)mp_obj_get_int(dest[1]); break; // converting to uint16_t;
                case MP_QSTR_channel: data->u32Channel = (uint32_t)mp_obj_get_int(dest[1]); break; // converting to uint8_t;
                case MP_QSTR_fill_cb: data->pfn_ain_fill_cb = (PFN_AIN_FILL_CB)mp_to_ptr(dest[1]); break;
                default: return;
            }

            dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
}

STATIC void mp_record_ain_drv_t_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{
    mp_printf(print, "struct record_ain_drv_t");
}

STATIC const mp_rom_map_elem_t mp_record_ain_drv_t_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_SIZE), MP_ROM_PTR(MP_ROM_INT(sizeof(record_ain_drv_t))) },
//    { MP_ROM_QSTR(MP_QSTR_cast), MP_ROM_PTR(&mp_lv_cast_class_method) },
//    { MP_ROM_QSTR(MP_QSTR_cast_instance), MP_ROM_PTR(&mp_lv_cast_instance_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_record_ain_drv_t_locals_dict, mp_record_ain_drv_t_locals_dict_table);

STATIC const mp_obj_type_t mp_record_ain_drv_t_type = {
    { &mp_type_type },
    .name = MP_QSTR_record_ain_drv_t,
    .print = mp_record_ain_drv_t_print,
    .make_new = make_new_record_struct,
    .locals_dict = (mp_obj_dict_t*)&mp_record_ain_drv_t_locals_dict,
    .attr = mp_record_ain_drv_t_attr,
    .buffer_p = { .get_buffer = mp_blob_get_buffer }
};

STATIC inline const mp_obj_type_t *get_mp_record_ain_drv_t_type()
{
    return &mp_record_ain_drv_t_type;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Micropython binding function
///////////////////////////////////////////////////////////////////////////////////////////////

STATIC mp_obj_t mp_record_vin_drv_register(size_t mp_n_args, const mp_obj_t *mp_args)
{
    record_vin_drv_t *driver = mp_write_ptr_record_vin_drv_t(mp_args[0]);
    Record_RegisterVinDrv(driver);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_RECORD_FUN_OBJ_VAR(mp_record_vin_drv_register_obj, 1, mp_record_vin_drv_register, Record_RegisterVinDrv);

STATIC mp_obj_t mp_record_vout_drv_register(size_t mp_n_args, const mp_obj_t *mp_args)
{
    record_vout_drv_t *driver = mp_write_ptr_record_vout_drv_t(mp_args[0]);
    Record_RegisterVoutDrv(driver);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_RECORD_FUN_OBJ_VAR(mp_record_vout_drv_register_obj, 1, mp_record_vout_drv_register, Record_RegisterVoutDrv);

STATIC mp_obj_t mp_record_ain_drv_register(size_t mp_n_args, const mp_obj_t *mp_args)
{
    record_ain_drv_t *driver = mp_write_ptr_record_ain_drv_t(mp_args[0]);
    Record_RegisterAinDrv(driver);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_RECORD_FUN_OBJ_VAR(mp_record_ain_drv_register_obj, 1, mp_record_ain_drv_register, Record_RegisterAinDrv);


static mp_obj_t py_record_init(mp_obj_t preview_obj)
{
	Record_Init(mp_obj_is_true(preview_obj));
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_record_init_obj, py_record_init);

static mp_obj_t py_record_deinit()
{
	Record_DeInit();
	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_record_deinit_obj, py_record_deinit);

static mp_obj_t py_record_start_record(mp_obj_t path_obj, mp_obj_t duration_obj)
{
	const char *szRecFolderPath;
	uint32_t u32RecDuration;
	int i32Ret;

	printf("DDDDD py_record_start_record --- 0 \n");
	if(path_obj != mp_const_none){
		szRecFolderPath = mp_obj_str_get_str(path_obj);
	}
	else{
		szRecFolderPath = DEF_REC_FOLDER_PATH;
	}

	if(duration_obj != mp_const_none){
		u32RecDuration = mp_obj_get_int(duration_obj);
	}
	else{
		u32RecDuration = eNM_UNLIMIT_TIME;
	}

	printf("DDDDD py_record_start_record %s --- 1 \n", szRecFolderPath);
	i32Ret = Record_Start(szRecFolderPath, u32RecDuration);
	printf("DDDDD py_record_start_record --- 2 \n");
	
	if(i32Ret != 0){
        nlr_raise(mp_obj_new_exception_msg(&mp_type_RuntimeError,
                    "Unable start record"));
	}

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_record_start_record_obj, py_record_start_record);

static mp_obj_t py_record_stop_record()
{
	int i32Ret;

	i32Ret = Record_Stop();

	if(i32Ret != 0){
        nlr_raise(mp_obj_new_exception_msg(&mp_type_RuntimeError,
                    "Unable stop record"));
	}

	return mp_const_none;
}


STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_record_stop_record_obj, py_record_stop_record);


static const mp_rom_map_elem_t Record_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_ROM_QSTR(MP_QSTR_Record)               },
    { MP_ROM_QSTR(MP_QSTR_init),            MP_ROM_PTR(&py_record_init_obj)           },
    { MP_ROM_QSTR(MP_QSTR_start_record),	MP_ROM_PTR(&py_record_start_record_obj)},
    { MP_ROM_QSTR(MP_QSTR_stop_record),  	MP_ROM_PTR(&py_record_stop_record_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),			MP_ROM_PTR(&py_record_deinit_obj) },

	//constant
    { MP_OBJ_NEW_QSTR(MP_QSTR_YUV422P),             MP_OBJ_NEW_SMALL_INT(ePLANAR_PIXFORMAT_YUV422)},	/* 2BPP/YUV422 planar*/
    { MP_OBJ_NEW_QSTR(MP_QSTR_YUV420P),             MP_OBJ_NEW_SMALL_INT(ePLANAR_PIXFORMAT_YUV420)},   /* 2BPP/YUV420 planar*/
    { MP_OBJ_NEW_QSTR(MP_QSTR_YUV420MB),            MP_OBJ_NEW_SMALL_INT(ePLANAR_PIXFORMAT_YUV420MB)},  /* 2BPP/YUV420 marco block*/

	//audio-in, video-in and video-out driver callback
	{ MP_ROM_QSTR(MP_QSTR_vin_drv_t), MP_ROM_PTR(&mp_record_vin_drv_t_type) },
	{ MP_ROM_QSTR(MP_QSTR_vout_drv_t), MP_ROM_PTR(&mp_record_vout_drv_t_type) },
	{ MP_ROM_QSTR(MP_QSTR_ain_drv_t), MP_ROM_PTR(&mp_record_ain_drv_t_type) },

    { MP_ROM_QSTR(MP_QSTR_vin_drv_register), MP_ROM_PTR(&mp_record_vin_drv_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_vout_drv_register), MP_ROM_PTR(&mp_record_vout_drv_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_ain_drv_register), MP_ROM_PTR(&mp_record_ain_drv_register_obj) },

};

STATIC MP_DEFINE_CONST_DICT(Record_module_globals, Record_globals_table);

const mp_obj_module_t mp_module_Record = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t)&Record_module_globals,
};

#endif
