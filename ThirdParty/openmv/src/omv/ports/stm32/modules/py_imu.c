/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2020 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2020 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * IMU Python module.
 */
#include "py/obj.h"
#include "py/nlr.h"
#include "py/mphal.h"
#include "systick.h"

#include "lsm6ds3tr_c_reg.h"
#include "omv_boardconfig.h"
#include "py_helper.h"
#include "py_imu.h"
#include STM32_HAL_H

#if MICROPY_PY_IMU

typedef union {
  int16_t i16bit[3];
  uint8_t u8bit[6];
} axis3bit16_t;

typedef union {
  int16_t i16bit;
  uint8_t u8bit[2];
} axis1bit16_t;

STATIC int32_t platform_write(void *handle, uint8_t Reg, uint8_t *Bufp,
                              uint16_t len)
{
    HAL_GPIO_WritePin(IMU_SPI_SSEL_PORT, IMU_SPI_SSEL_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle, &Reg, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(handle, Bufp, len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(IMU_SPI_SSEL_PORT, IMU_SPI_SSEL_PIN, GPIO_PIN_SET);
    return 0;
}

STATIC int32_t platform_read(void *handle, uint8_t Reg, uint8_t *Bufp,
                             uint16_t len)
{
    Reg |= 0x80;
    HAL_GPIO_WritePin(IMU_SPI_SSEL_PORT, IMU_SPI_SSEL_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle, &Reg, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(handle, Bufp, len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(IMU_SPI_SSEL_PORT, IMU_SPI_SSEL_PIN, GPIO_PIN_SET);
    return 0;
}

STATIC SPI_HandleTypeDef SPIHandle = {
    .Instance               = IMU_SPI,
    .Init.Mode              = SPI_MODE_MASTER,
    .Init.Direction         = SPI_DIRECTION_2LINES,
    .Init.DataSize          = SPI_DATASIZE_8BIT,
    .Init.CLKPolarity       = SPI_POLARITY_HIGH,
    .Init.CLKPhase          = SPI_PHASE_2EDGE,
    .Init.NSS               = SPI_NSS_SOFT,
    .Init.BaudRatePrescaler = IMU_SPI_PRESCALER,
    .Init.FirstBit          = SPI_FIRSTBIT_MSB,
};

STATIC stmdev_ctx_t dev_ctx = {
    .write_reg = platform_write,
    .read_reg = platform_read,
    .handle = &SPIHandle
};

STATIC bool test_not_ready()
{
    return HAL_SPI_GetState(&SPIHandle) != HAL_SPI_STATE_READY;
}

STATIC void error_on_not_ready()
{
    if (test_not_ready()) nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "IMU Not Ready!"));
}

STATIC mp_obj_t py_imu_tuple(float x, float y, float z)
{
    return mp_obj_new_tuple(3, (mp_obj_t [3]) {mp_obj_new_float(x),
                                               mp_obj_new_float(y),
                                               mp_obj_new_float(z)});
}

// For when the camera board is lying on a table face up.

// X points to the right of the camera sensor
// Y points down below the camera sensor
// Z points in the reverse direction of the camera sensor

// Thus (https://www.nxp.com/docs/en/application-note/AN3461.pdf):
//
// Roll = atan2(Y, Z)
// Pitch = atan2(-X, sqrt(Y^2, + Z^2)) -> assume Y=0 -> atan2(-X, Z)

// For when the camera board is standing right-side up.

// X points to the right of the camera sensor (still X)
// Y points down below the camera sensor (now Z)
// Z points in the reverse direction of the camera sensor (now -Y)

// So:
//
// Roll = atan2(-X, sqrt(Z^2, + Y^2)) -> assume Z=0 -> atan2(-X, Y)
// Pitch = atan2(Z, -Y)

STATIC float py_imu_get_roll()
{
    axis3bit16_t data_raw_acceleration = {};
    lsm6ds3tr_c_acceleration_raw_get(&dev_ctx, data_raw_acceleration.u8bit);
    float x = lsm6ds3tr_c_from_fs8g_to_mg(data_raw_acceleration.i16bit[0]);
    float y = lsm6ds3tr_c_from_fs8g_to_mg(data_raw_acceleration.i16bit[1]);
    return fmodf((IM_RAD2DEG(fast_atan2f(-x, y)) + 180), 360); // rotate 180
}

STATIC float py_imu_get_pitch()
{
    axis3bit16_t data_raw_acceleration = {};
    lsm6ds3tr_c_acceleration_raw_get(&dev_ctx, data_raw_acceleration.u8bit);
    float y = lsm6ds3tr_c_from_fs8g_to_mg(data_raw_acceleration.i16bit[1]);
    float z = lsm6ds3tr_c_from_fs8g_to_mg(data_raw_acceleration.i16bit[2]);
    return IM_RAD2DEG(fast_atan2f(z, -y));
}

STATIC mp_obj_t py_imu_acceleration_mg()
{
    error_on_not_ready();

    axis3bit16_t data_raw_acceleration = {};
    lsm6ds3tr_c_acceleration_raw_get(&dev_ctx, data_raw_acceleration.u8bit);
    return py_imu_tuple(lsm6ds3tr_c_from_fs8g_to_mg(data_raw_acceleration.i16bit[0]),
                        lsm6ds3tr_c_from_fs8g_to_mg(data_raw_acceleration.i16bit[1]),
                        lsm6ds3tr_c_from_fs8g_to_mg(data_raw_acceleration.i16bit[2]));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_imu_acceleration_mg_obj, py_imu_acceleration_mg);

STATIC mp_obj_t py_imu_angular_rate_mdps()
{
    error_on_not_ready();

    axis3bit16_t data_raw_angular_rate = {};
    lsm6ds3tr_c_angular_rate_raw_get(&dev_ctx, data_raw_angular_rate.u8bit);
    return py_imu_tuple(lsm6ds3tr_c_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[0]),
                        lsm6ds3tr_c_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[1]),
                        lsm6ds3tr_c_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[2]));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_imu_angular_rate_mdps_obj, py_imu_angular_rate_mdps);

STATIC mp_obj_t py_imu_temperature_c()
{
    error_on_not_ready();

    axis1bit16_t data_raw_temperature = {};
    lsm6ds3tr_c_temperature_raw_get(&dev_ctx, data_raw_temperature.u8bit);
    return mp_obj_new_float(lsm6ds3tr_c_from_lsb_to_celsius(data_raw_temperature.i16bit));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_imu_temperature_c_obj, py_imu_temperature_c);

STATIC mp_obj_t py_imu_roll()
{
    error_on_not_ready();

    return mp_obj_new_float(py_imu_get_roll());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_imu_roll_obj, py_imu_roll);

STATIC mp_obj_t py_imu_pitch()
{
    error_on_not_ready();

    return mp_obj_new_float(py_imu_get_pitch());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_imu_pitch_obj, py_imu_pitch);

STATIC mp_obj_t py_imu_sleep(mp_obj_t enable)
{
    error_on_not_ready();

    bool en = mp_obj_get_int(enable);
    lsm6ds3tr_c_xl_data_rate_set(&dev_ctx, en ? LSM6DS3TR_C_XL_ODR_OFF : LSM6DS3TR_C_XL_ODR_52Hz);
    lsm6ds3tr_c_gy_data_rate_set(&dev_ctx, en ? LSM6DS3TR_C_GY_ODR_OFF : LSM6DS3TR_C_GY_ODR_52Hz);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_imu_sleep_obj, py_imu_sleep);

STATIC mp_obj_t py_imu_write_reg(mp_obj_t addr, mp_obj_t val)
{
    error_on_not_ready();

    uint8_t v = mp_obj_get_int(val);
    lsm6ds3tr_c_write_reg(&dev_ctx, mp_obj_get_int(addr), &v, sizeof(v));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_imu_write_reg_obj, py_imu_write_reg);

STATIC mp_obj_t py_imu_read_reg(mp_obj_t addr)
{
    error_on_not_ready();

    uint8_t v;
    lsm6ds3tr_c_read_reg(&dev_ctx, mp_obj_get_int(addr), &v, sizeof(v));
    return mp_obj_new_int(v);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_imu_read_reg_obj, py_imu_read_reg);

STATIC const mp_rom_map_elem_t globals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_OBJ_NEW_QSTR(MP_QSTR_imu) },
    { MP_ROM_QSTR(MP_QSTR_acceleration_mg),     MP_ROM_PTR(&py_imu_acceleration_mg_obj) },
    { MP_ROM_QSTR(MP_QSTR_angular_rate_mdps),   MP_ROM_PTR(&py_imu_angular_rate_mdps_obj) },
    { MP_ROM_QSTR(MP_QSTR_temperature_c),       MP_ROM_PTR(&py_imu_temperature_c_obj) },
    { MP_ROM_QSTR(MP_QSTR_roll),                MP_ROM_PTR(&py_imu_roll_obj) },
    { MP_ROM_QSTR(MP_QSTR_pitch),               MP_ROM_PTR(&py_imu_pitch_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep),               MP_ROM_PTR(&py_imu_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR___write_reg),         MP_ROM_PTR(&py_imu_write_reg_obj) },
    { MP_ROM_QSTR(MP_QSTR___read_reg),          MP_ROM_PTR(&py_imu_read_reg_obj) },
};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t imu_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_t) &globals_dict
};

void py_imu_init()
{
    HAL_SPI_Init(&SPIHandle);

    uint8_t whoamI = 0, rst = 1;

    // Try to read device id...
    for (int i = 0; (i < 10) && (whoamI != LSM6DS3TR_C_ID); i++) {
        lsm6ds3tr_c_device_id_get(&dev_ctx, &whoamI);
    }

    if (whoamI != LSM6DS3TR_C_ID) {
        HAL_SPI_DeInit(&SPIHandle);
        return;
    }

    lsm6ds3tr_c_reset_set(&dev_ctx, PROPERTY_ENABLE);

    for (int i = 0; (i < 10000) && rst; i++) {
        lsm6ds3tr_c_reset_get(&dev_ctx, &rst);
    }

    if (rst) {
        HAL_SPI_DeInit(&SPIHandle);
        return;
    }

    lsm6ds3tr_c_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);

    lsm6ds3tr_c_xl_data_rate_set(&dev_ctx, LSM6DS3TR_C_XL_ODR_52Hz);
    lsm6ds3tr_c_gy_data_rate_set(&dev_ctx, LSM6DS3TR_C_GY_ODR_52Hz);

    lsm6ds3tr_c_xl_full_scale_set(&dev_ctx, LSM6DS3TR_C_8g);
    lsm6ds3tr_c_gy_full_scale_set(&dev_ctx, LSM6DS3TR_C_2000dps);
}

float py_imu_roll_rotation()
{
    if (test_not_ready()) {
        return 0; // output when the camera is mounted upright
    }

    return py_imu_get_roll();
}

float py_imu_pitch_rotation()
{
    if (test_not_ready()) {
        return 0; // output when the camera is mounted upright
    }

    return py_imu_get_pitch();
}
#endif // MICROPY_PY_IMU
