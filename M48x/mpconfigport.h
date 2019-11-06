#include <stdint.h>

#include "mpconfigboard.h"
#include "mpconfigboard_common.h"
#include "NuMicro.h"
// options to control how MicroPython is built

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.
#define MICROPY_ENABLE_COMPILER     (1)

#define MICROPY_QSTR_BYTES_IN_HASH  (1)
#define MICROPY_QSTR_EXTRA_POOL     mp_qstr_frozen_const_pool
#define MICROPY_ALLOC_PATH_MAX      (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT (16)
#define MICROPY_EMIT_X64            (0)
#define MICROPY_EMIT_THUMB          (0)
#define MICROPY_EMIT_INLINE_THUMB   (0)
#define MICROPY_COMP_MODULE_CONST   (0)
#define MICROPY_COMP_CONST          (0)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (0)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (0)
#define MICROPY_MEM_STATS           (0)
#define MICROPY_DEBUG_PRINTERS      (0)
#define MICROPY_GC_ALLOC_THRESHOLD  (0)
#define MICROPY_REPL_EVENT_DRIVEN   (0)
#define MICROPY_HELPER_LEXER_UNIX   (0)
#define MICROPY_ENABLE_SOURCE_LINE  (0)
#define MICROPY_ENABLE_DOC_STRING   (0)
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_TERSE)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)
#define MICROPY_PY_ASYNC_AWAIT      (0)
#define MICROPY_PY___FILE__         (0)
#define MICROPY_PY_GC               (1)
#define MICROPY_PY_ARRAY            (1)
#define MICROPY_PY_ATTRTUPLE        (1)
#define MICROPY_PY_COLLECTIONS      (1)
#define MICROPY_PY_COLLECTIONS_DEQUE (1)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT (1)
#define MICROPY_PY_MATH             (1)
#define MICROPY_PY_CMATH            (1)
#define MICROPY_PY_IO               (1)
#define MICROPY_PY_IO_FILEIO        (1)
#define MICROPY_PY_STRUCT           (1)
//#define MICROPY_PY_SYS              (0)
#define MICROPY_PY_SYS_MAXSIZE      (1)
#define MICROPY_PY_SYS_EXIT         (1)
#define MICROPY_PY_SYS_STDFILES     (1)
#define MICROPY_PY_SYS_STDIO_BUFFER (1)
#ifndef MICROPY_PY_SYS_PLATFORM     // let boards override it if they want
#define MICROPY_PY_SYS_PLATFORM     "pyboard"
#endif

#define MICROPY_MODULE_FROZEN_MPY   (1)
#define MICROPY_CPYTHON_COMPAT      (0)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_NONE)
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_PY_UERRNO           (1)

// control over Python builtins
#define MICROPY_PY_BUILTINS_BYTEARRAY (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (1)
#define MICROPY_PY_BUILTINS_ENUMERATE (0)
#define MICROPY_PY_BUILTINS_FILTER  (0)
#define MICROPY_PY_BUILTINS_FROZENSET (0)
#define MICROPY_PY_BUILTINS_REVERSED (0)
#define MICROPY_PY_BUILTINS_SET     (1)
#define MICROPY_PY_BUILTINS_SLICE   (0)
#define MICROPY_PY_BUILTINS_PROPERTY (0)
#define MICROPY_PY_BUILTINS_MIN_MAX (0)
#define MICROPY_PY_BUILTINS_INPUT   (1)


// Python internal features
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_READER_VFS          (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_VFS                 (1)
#define MICROPY_VFS_FAT             (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_REPL_EMACS_KEYS     (1)
#define MICROPY_REPL_AUTO_INDENT    (1)
#define MICROPY_STREAMS_POSIX_API   (1)
#define MICROPY_ENABLE_SCHEDULER    (1)
#define MICROPY_SCHEDULER_DEPTH     (8)

// extended modules
#define MICROPY_PY_UCTYPES          (1)
#define MICROPY_PY_UZLIB            (1)
#define MICROPY_PY_UJSON            (1)
#define MICROPY_PY_URE              (1)
#define MICROPY_PY_UHEAPQ           (1)
#define MICROPY_PY_UHASHLIB         (1)
#define MICROPY_PY_UBINASCII        (1)
#define MICROPY_PY_USELECT          (1)
#define MICROPY_PY_UTIME_MP_HAL     (1)
#define MICROPY_PY_MACHINE_I2C      (1)

#define MICROPY_PY_MACHINE_SPI      (1)
#define MICROPY_PY_MACHINE_SPI_MSB  (0)
#define MICROPY_PY_MACHINE_SPI_LSB  (1)

// fatfs configuration used in ffconf.h
#define MICROPY_FATFS_ENABLE_LFN       (1)
#define MICROPY_FATFS_LFN_CODE_PAGE    (437) /* 1=SFN/ANSI 437=LFN/U.S.(OEM) */
#define MICROPY_FATFS_USE_LABEL        (1)
#define MICROPY_FATFS_RPATH            (2)
#define MICROPY_FATFS_MULTI_PARTITION  (1)

// TODO these should be generic, not bound to fatfs
#define mp_type_fileio mp_type_vfs_fat_fileio

// use vfs's functions for import stat and builtin open
#define mp_import_stat mp_vfs_import_stat
#define mp_builtin_open mp_vfs_open
#define mp_builtin_open_obj mp_vfs_open_obj

#define MICROPY_PY_MACHINE          (1)

#if MICROPY_NVT_LWIP || MICROPY_WLAN_ESP8266
#define MICROPY_PY_NETWORK           (1)
#else
#define MICROPY_PY_NETWORK           (0)
#endif

#ifndef MICROPY_PY_THREAD
#define MICROPY_PY_THREAD           (0)
#endif

#if MICROPY_PY_THREAD
//Disable GIL mutex access. Because py/runtime.c enter the GIL mutex but without exit
#define MICROPY_PY_THREAD_GIL		(0)
#endif

 
#define MICROPY_USE_INTERNAL_PRINTF         (0)

// type definitions for the specific machine

#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void*)((mp_uint_t)(p) | 1))

// This port is intended to be 32-bit, but unfortunately, int32_t for
// different targets may be defined in different ways - either as int
// or as long. This requires different printf formatting specifiers
// to print such value. So, we avoid int32_t and use int directly.
#define UINT_FMT "%u"
#define INT_FMT "%d"
typedef int mp_int_t; // must be pointer size
typedef unsigned mp_uint_t; // must be pointer size

typedef long mp_off_t;

#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)

// extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
   { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) },



// extra built in modules to add to the list of known ones
extern const struct _mp_obj_module_t machine_module;
extern const struct _mp_obj_module_t pyb_module;
extern const struct _mp_obj_module_t mp_module_network;
extern const struct _mp_obj_module_t mp_module_usocket;
extern const struct _mp_obj_module_t mp_module_utime;
extern const struct _mp_obj_module_t mp_module_uos;

//littlevgl modules
extern const struct _mp_obj_module_t mp_module_lvgl;
extern const struct _mp_obj_module_t mp_module_ILI9341;
extern const struct _mp_obj_module_t mp_module_TouchADC;


#if MICROPY_PY_NETWORK
#define NETWORK_BUILTIN_MODULE              \
	{ MP_ROM_QSTR(MP_QSTR_network), MP_ROM_PTR(&mp_module_network) }, \
	{ MP_ROM_QSTR(MP_QSTR_usocket), MP_ROM_PTR(&mp_module_usocket) },
#else
#define NETWORK_BUILTIN_MODULE
#endif

#if MICROPY_LVGL
#include "lvgl/src/lv_misc/lv_gc.h"
// TODO: [MUST] by chchen59
//    { MP_OBJ_NEW_QSTR(MP_QSTR_lvindev), (mp_obj_t)&mp_module_lvindev},
//    { MP_OBJ_NEW_QSTR(MP_QSTR_SDL), (mp_obj_t)&mp_module_SDL },
//    { MP_OBJ_NEW_QSTR(MP_QSTR_lodepng), (mp_obj_t)&mp_module_lodepng },

#define MICROPY_PY_LVGL_DEF \
    { MP_OBJ_NEW_QSTR(MP_QSTR_lvgl), (mp_obj_t)&mp_module_lvgl }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_ILI9341), (mp_obj_t)&mp_module_ILI9341 }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_TouchADC), (mp_obj_t)&mp_module_TouchADC },
#else
#define LV_ROOTS 
#define MICROPY_PY_LVGL_DEF 
#endif



#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_umachine), (mp_obj_t)&machine_module }, \
    { MP_ROM_QSTR(MP_QSTR_pyb), MP_ROM_PTR(&pyb_module) }, \
    { MP_ROM_QSTR(MP_QSTR_uos), MP_ROM_PTR(&mp_module_uos) }, \
    { MP_ROM_QSTR(MP_QSTR_utime), MP_ROM_PTR(&mp_module_utime) }, \
    NETWORK_BUILTIN_MODULE \
    MICROPY_PY_LVGL_DEF \

#define MICROPY_PORT_BUILTIN_MODULE_WEAK_LINKS \
    { MP_ROM_QSTR(MP_QSTR_binascii), MP_ROM_PTR(&mp_module_ubinascii) }, \
    { MP_ROM_QSTR(MP_QSTR_collections), MP_ROM_PTR(&mp_module_collections) }, \
    { MP_ROM_QSTR(MP_QSTR_re), MP_ROM_PTR(&mp_module_ure) }, \
    { MP_ROM_QSTR(MP_QSTR_zlib), MP_ROM_PTR(&mp_module_uzlib) }, \
    { MP_ROM_QSTR(MP_QSTR_json), MP_ROM_PTR(&mp_module_ujson) }, \
    { MP_ROM_QSTR(MP_QSTR_heapq), MP_ROM_PTR(&mp_module_uheapq) }, \
    { MP_ROM_QSTR(MP_QSTR_hashlib), MP_ROM_PTR(&mp_module_uhashlib) }, \
    { MP_ROM_QSTR(MP_QSTR_io), MP_ROM_PTR(&mp_module_io) }, \
    { MP_ROM_QSTR(MP_QSTR_os), MP_ROM_PTR(&mp_module_uos) }, \
    { MP_ROM_QSTR(MP_QSTR_random), MP_ROM_PTR(&mp_module_urandom) }, \
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&mp_module_utime) }, \
    { MP_ROM_QSTR(MP_QSTR_select), MP_ROM_PTR(&mp_module_uselect) }, \
    SOCKET_BUILTIN_MODULE_WEAK_LINKS \
    { MP_ROM_QSTR(MP_QSTR_struct), MP_ROM_PTR(&mp_module_ustruct) }, \
    { MP_ROM_QSTR(MP_QSTR_machine), MP_ROM_PTR(&machine_module) }, \
    { MP_ROM_QSTR(MP_QSTR_errno), MP_ROM_PTR(&mp_module_uerrno) }, \


// We need to provide a declaration/definition of alloca()
#include <alloca.h>

#define MICROPY_HW_MCU_NAME "Nuvoton-M48x"

#ifdef __linux__
#define MICROPY_MIN_USE_STDOUT (1)
#endif

#ifdef __thumb__
#define MICROPY_MIN_USE_CORTEX_CPU (1)
//#define MICROPY_MIN_USE_STM32_MCU (1)
#endif

#define MP_STATE_PORT MP_STATE_VM

#define MICROPY_PORT_ROOT_POINTERS \
    const char *readline_hist[8]; \
    LV_ROOTS \
    void *mp_lv_user_data; \
    \
    mp_obj_t pin_class_mapper; \
    mp_obj_t pin_class_map_dict; \
    \
    mp_obj_t pyb_extint_callback[PYB_EXTI_NUM_VECTORS]; \
    /* list of registered NICs */ \
    mp_obj_list_t mod_network_nic_list; \
    

// We have inlined IRQ functions for efficiency (they are generally
// 1 machine instruction).
//
// Note on IRQ state: you should not need to know the specific
// value of the state variable, but rather just pass the return
// value from disable_irq back to enable_irq.  If you really need
// to know the machine-specific values, see irq.h.

static inline void enable_irq(mp_uint_t state) {
    __set_PRIMASK(state);
}

static inline mp_uint_t disable_irq(void) {
    mp_uint_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

#if MICROPY_PY_THREAD
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(void); \
        mp_handle_pending(); \
		MP_THREAD_GIL_EXIT(); \
		taskYIELD(); \
		MP_THREAD_GIL_ENTER(); \
    } while (0);

#define MICROPY_THREAD_YIELD() taskYIELD()

#else
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(void); \
        mp_handle_pending(); \
        __WFI(); \
    } while (0);

#define MICROPY_THREAD_YIELD()
#endif

