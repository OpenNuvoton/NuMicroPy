// m48x_prefix.c becomes the initial portion of the generated pins file.

#include <stdio.h>

#include "py/obj.h"
#include "py/mphal.h"
#include "mods/pybpin.h"

#define AF(p_port, p_pin, p_mfp, af_idx, af_fn, af_unit, af_type, af_ptr) \
{ \
    { &pin_af_type }, \
    .name = MP_QSTR_AF_P##p_port##p_pin##_ ## af_fn ## af_unit ##_ ## af_type, \
    .idx = (af_idx), \
    .fn = AF_FN_ ## af_fn, \
    .unit = (af_unit), \
    .type = AF_PIN_TYPE_ ## af_fn ## _ ## af_type, \
    .reg = (af_ptr), \
    .mfp_val = SYS_GP##p_port##_##p_mfp##_P##p_port##p_pin##MFP_## af_fn ## af_unit##_##af_type, \
}

#define PIN(p_port, p_pin, p_mfp, p_af, p_adc_num, p_adc_channel) \
{ \
    { &pin_type }, \
    .name = MP_QSTR_ ## p_port ## p_pin, \
    .port = PORT_ ## p_port, \
    .pin = (p_pin), \
    .num_af = (sizeof(p_af) / sizeof(pin_af_obj_t)), \
    .pin_mask = (1 << ((p_pin) & 0x0f)), \
    .gpio = P ## p_port, \
    .af = p_af, \
    .adc_num = p_adc_num, \
    .adc_channel = p_adc_channel, \
    .mfp_reg = &SYS->GP##p_port##_##p_mfp, \
    .mfos_reg = &SYS->GP##p_port##_MFOS, \
}
