/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/mpthread.h"

#include "pybaccel.h"
#include "hal/M48x_I2C.h"


#if MICROPY_HW_HAS_BMX055

/* BMX055 address */
#define BMX055_DEFAULT_ACCEL_ADDR	(0x18)
#define BMX055_DEFAULT_GYRO_ADDR	(0x68)
#define BMX055_DEFAULT_MAG_ADDR		(0x10)


/*
 * BMX055 accelerometer registers and values
 * 
 */
#define BMX055_REG_ACCEL_CHIPID      (0x00U)
#define BMX055_REG_ACCEL_CHIPID_VAL  (0xFAU)
#define BMX055_REG_ACCEL_SHDW        (0x13U)
#define BMX055_REG_ACCEL_SHDW_ENABLE (0x00U)
#define BMX055_REG_ACCEL_DATA        (0x02U)


#define BMX055_REG_ACCEL_RANGE       (0x0FU)
#define BIT_ACC_RANGE_2G    (0x03U)
#define BIT_ACC_RANGE_4G    (0x05U)
#define BIT_ACC_RANGE_8G    (0x08U)
#define BIT_ACC_RANGE_16G   (0x0CU)

#define BMX055_REG_ACCEL_PMU_LPW	(0x11U)
#define BIT_ACC_SUSPEND		    (0x80U)

#define BMX055_REG_ACCEL_PMU_LOW_POWER	(0x12U)
#define BIT_ACC_LPM2		    (0x40U)


/*
 * BMX055 gyroscope registers and values
 * 
 */
#define BMX055_REG_GYRO_CHIPID      (0x00U)
#define BMX055_REG_GYRO_CHIPID_VAL  (0x0FU)

/* gyroscope power modes (LPM1)*/
#define BMX055_REG_GYRO_LPM1      	(0x11U)
#define BIT_GYRO_PWMD_NORM		    (0x00U)
#define BIT_GYRO_PWMD_SUSPEND	    (0x80U)

/* gyroscope power modes (LPM2)*/
#define BMX055_REG_GYRO_LPM2      	(0x12U)
#define BIT_GYRO_FAST_PWUP		    (0x80U)


/* gyroscope shadowing */
#define BMX055_REG_GYRO_RATE_HBW       (0x13U)
#define BIT_GYRO_SHADOW_DIS		 (0x40U)

/* gyroscope range*/
#define BMX055_REG_GYRO_RANGE       (0x0FU)
#define BIT_GYRO_RANGE_125DPS    (0x04U)
#define BIT_GYRO_RANGE_250DPS    (0x03U)
#define BIT_GYRO_RANGE_500DPS    (0x02U)
#define BIT_GYRO_RANGE_1000DPS   (0x01U)
#define BIT_GYRO_RANGE_2000DPS   (0x00U)

/* gyroscope bandwidth*/
#define BMX055_REG_GYRO_BW		(0x10U)
#define BIT_GYRO_BW_230HZ		(0x01U)
#define BIT_GYRO_BW_116HZ		(0x02U)
#define BIT_GYRO_BW_47HZ		(0x03U)
#define BIT_GYRO_BW_23HZ		(0x04U)
#define BIT_GYRO_BW_12HZ		(0x05U)
#define BIT_GYRO_BW_64HZ		(0x06U)
#define BIT_GYRO_BW_32HZ		(0x07U)


/*
 * BMX055 magnetometer registers and values
 * 
 */
#define BMX055_REG_MAG_CHIPID      (0x40U)
#define BMX055_REG_MAG_CHIPID_VAL  (0x32U)

/* magnetometer power control*/
#define BMX055_REG_MAG_PWCTRL      	(0x4BU)
#define BIT_MAG_PWCTRL_SLEEP	    (0x01U)

/* magnetometer operation mode, output data rate and self-test*/
#define BMX055_REG_MAG_OPMODE      	(0x4CU)
#define BIT_MAG_OPMODE_FORCED	    (0x02U)
#define BIT_MAG_OPMODE_SLEEP	    (0x06U)

//#define BMX055_ACCEL_SELF_TEST


typedef enum{
	eBMX055_OK,
	eBMX055_NODEV = -1,
	eBMX055_WRITE = -2,
}E_BMX055_ERRCODE;

typedef enum{
	eBMX055_ACCEL,
	eBMX055_GYRO,
	eBMX055_MAG
}E_BMX055_TYPE;

typedef struct {
	uint32_t u32Range;

}S_ACCEL_PARAM;


typedef struct {
	uint32_t u32Range;

}S_GYRO_PARAM;

typedef struct {
	uint32_t u32Reserved;

}S_MAG_PARAM;

#if  MICROPY_PY_THREAD
STATIC mp_thread_mutex_t s_I2C_mutex;
#endif

static E_BMX055_ERRCODE bmx055_init(
	I2C_T* i2c_bus,
	E_BMX055_TYPE eType,
	void *pvParam
)
{
	uint8_t u8Temp;
	E_BMX055_ERRCODE errCode = eBMX055_OK;

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif
	
	if(eType == eBMX055_ACCEL){
		S_ACCEL_PARAM *psAccelParam = (S_ACCEL_PARAM *)pvParam;

		//Check accelerometer chip ID
		u8Temp = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, BMX055_REG_ACCEL_CHIPID);

		if(u8Temp != BMX055_REG_ACCEL_CHIPID_VAL){
			errCode = eBMX055_NODEV;
			goto bmx055_init_done;
		}

		/* softreset to bring module to normal mode*/
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, 0x14, 0xB6) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}
#if defined(BMX055_ACCEL_SELF_TEST)
		/* setting accelerometer range */
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, BMX055_REG_ACCEL_RANGE, BIT_ACC_RANGE_8G) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}
#else
		/* setting accelerometer range */
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, BMX055_REG_ACCEL_RANGE, (uint8_t)psAccelParam->u32Range) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}
#endif

		/* enable accelerometer shadowing */
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, BMX055_REG_ACCEL_SHDW, BMX055_REG_ACCEL_SHDW_ENABLE) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}

	}

	if(eType == eBMX055_GYRO){
		S_GYRO_PARAM *psGyroParam = (S_GYRO_PARAM *)pvParam;

		//Check gyroscope chip ID
		u8Temp = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_CHIPID);

		if(u8Temp != BMX055_REG_GYRO_CHIPID_VAL){
			errCode = eBMX055_NODEV;
			goto bmx055_init_done;
		}

		/* set power mode to normal*/
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_LPM1, BIT_GYRO_PWMD_NORM) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}

		/* set power mode to normal*/
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_LPM2, 0x0) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}


		/* set gyroscope range */
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_RANGE, (uint8_t)psGyroParam->u32Range) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}

		/* set gyroscope bandwidth */
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_BW, BIT_GYRO_BW_230HZ) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}


		/* enable gyroscope shadowing */
		u8Temp = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_RATE_HBW);
		u8Temp &= ~BIT_GYRO_SHADOW_DIS; 
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_RATE_HBW, u8Temp) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}

	}

	if(eType == eBMX055_MAG){
//		S_MAG_PARAM *psMagParam = (S_MAG_PARAM *)pvParam;

		/* set power control to sleep*/
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, BMX055_REG_MAG_PWCTRL, BIT_MAG_PWCTRL_SLEEP) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}

		//Check magnetometer chip ID
		u8Temp = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, BMX055_REG_MAG_CHIPID);

		if(u8Temp != BMX055_REG_MAG_CHIPID_VAL){
			errCode = eBMX055_NODEV;
			goto bmx055_init_done;
		}

		/* change sleep mode to normal*/
		if(I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, BMX055_REG_MAG_OPMODE, 0x00) != 0){
			errCode = eBMX055_WRITE;
			goto bmx055_init_done;
		}

	}

bmx055_init_done:

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

	return errCode;

}

static void bmx055_deinit(
	I2C_T* i2c_bus,
	E_BMX055_TYPE eType
)
{
	uint8_t u8Temp;

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

	if(eType == eBMX055_ACCEL){
		//Set accelerometer to standby

		//Read low power mode register
		u8Temp = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, BMX055_REG_ACCEL_PMU_LOW_POWER);
		u8Temp |= BIT_ACC_LPM2;
		//set low power mode to LPM2
		I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, BMX055_REG_ACCEL_PMU_LOW_POWER, u8Temp);

		//Read low power enable register
		u8Temp = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, BMX055_REG_ACCEL_PMU_LPW);
		u8Temp |= BIT_ACC_SUSPEND;
		//set low power enable to suspend
		I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, BMX055_REG_ACCEL_PMU_LPW, u8Temp);
	}

	if(eType == eBMX055_GYRO){
	
		//Read lpm2 register
		u8Temp = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_LPM2);
		u8Temp &= ~BIT_GYRO_FAST_PWUP;
		//disable fast power up
		I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_LPM2, u8Temp);

		//Read lpm2 register
		u8Temp = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_LPM1);
		u8Temp |= BIT_GYRO_PWMD_SUSPEND;
		//enable suspend
		I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, BMX055_REG_GYRO_LPM1, u8Temp);

	}

	if(eType == eBMX055_MAG){
	
		//Read opemode register
		u8Temp = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, BMX055_REG_MAG_OPMODE);
		u8Temp |= BIT_MAG_OPMODE_SLEEP;
		//set sleep mode
		I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, BMX055_REG_MAG_OPMODE, u8Temp);

		//Read power control register
		u8Temp = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, BMX055_REG_MAG_PWCTRL);
		u8Temp &= ~BIT_MAG_PWCTRL_SLEEP;
		//enable suspend
		I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, BMX055_REG_MAG_PWCTRL, u8Temp);

	}
#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

}

static int32_t twos_comp_to_int(
	int32_t i32Val,
	uint8_t u8Bits
)
{
    if ((i32Val & (1 << (u8Bits - 1))) != 0)  //if sign bit is set
        i32Val = i32Val - (1 << u8Bits);     // compute negative value
    return i32Val;                         // return positive value as is
}


static float bmx055_read_x_accel(
	I2C_T* i2c_bus,
	S_ACCEL_PARAM *psAccelParam
)
{
	uint8_t u8LSB;
	uint8_t u8MSB;
	int32_t i32AccelVal;

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

#if defined(BMX055_ACCEL_SELF_TEST)
	I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, 0x32, 0x05);
#endif

	u8LSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, 0x02);
	u8MSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, 0x03);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

	i32AccelVal =  u8MSB;
	i32AccelVal = i32AccelVal << 4;
	i32AccelVal |=  (u8LSB >> 4);

	i32AccelVal = twos_comp_to_int(i32AccelVal, 12);

	if(psAccelParam->u32Range == BIT_ACC_RANGE_2G)
		return i32AccelVal * 0.98 / 1000;
	else if(psAccelParam->u32Range == BIT_ACC_RANGE_4G)
		return i32AccelVal * 1.95 / 1000;
	else if(psAccelParam->u32Range == BIT_ACC_RANGE_8G)
		return i32AccelVal * 3.91 / 1000;
	else
		return i32AccelVal * 7.81 / 1000;
	
}

static float bmx055_read_y_accel(
	I2C_T* i2c_bus,
	S_ACCEL_PARAM *psAccelParam
)
{
	uint8_t u8LSB;
	uint8_t u8MSB;
	int32_t i32AccelVal;

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

#if defined(BMX055_ACCEL_SELF_TEST)
	I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, 0x32, 0x06);
#endif
	
	u8LSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, 0x04);
	u8MSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, 0x05);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

	i32AccelVal =  u8MSB;
	i32AccelVal = i32AccelVal << 4;
	i32AccelVal |=  (u8LSB >> 4);

	i32AccelVal = twos_comp_to_int(i32AccelVal, 12);

	if(psAccelParam->u32Range == BIT_ACC_RANGE_2G)
		return i32AccelVal * 0.98 / 1000;
	else if(psAccelParam->u32Range == BIT_ACC_RANGE_4G)
		return i32AccelVal * 1.95 / 1000;
	else if(psAccelParam->u32Range == BIT_ACC_RANGE_8G)
		return i32AccelVal * 3.91 / 1000;
	else
		return i32AccelVal * 7.81 / 1000;
	
}

static float bmx055_read_z_accel(
	I2C_T* i2c_bus,
	S_ACCEL_PARAM *psAccelParam
)
{
	uint8_t u8LSB;
	uint8_t u8MSB;
	int32_t i32AccelVal;

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

#if defined(BMX055_ACCEL_SELF_TEST)
	I2C_WriteByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, 0x32, 0x07);
#endif
	
	u8LSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, 0x06);
	u8MSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_ACCEL_ADDR, 0x07);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

	i32AccelVal =  u8MSB;
	i32AccelVal = i32AccelVal << 4;
	i32AccelVal |=  (u8LSB >> 4);

	i32AccelVal = twos_comp_to_int(i32AccelVal, 12);

	if(psAccelParam->u32Range == BIT_ACC_RANGE_2G)
		return i32AccelVal * 0.98 / 1000;
	else if(psAccelParam->u32Range == BIT_ACC_RANGE_4G)
		return i32AccelVal * 1.95 / 1000;
	else if(psAccelParam->u32Range == BIT_ACC_RANGE_8G)
		return i32AccelVal * 3.91 / 1000;
	else
		return i32AccelVal * 7.81 / 1000;
	
}

static float bmx055_read_x_gyro(
	I2C_T* i2c_bus,
	S_GYRO_PARAM *psGyroParam
)
{
	uint8_t u8LSB;
	uint8_t u8MSB;
	int32_t i32GyroVal;
	float fResolution;

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif
	
	u8LSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, 0x02);
	u8MSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, 0x03);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

	i32GyroVal =  u8MSB;
	i32GyroVal = i32GyroVal << 8;
	i32GyroVal |=  u8LSB;
	i32GyroVal = twos_comp_to_int(i32GyroVal, 16);


	if(psGyroParam->u32Range == BIT_GYRO_RANGE_125DPS){
		fResolution = 0.0038; //(2 * 125) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_250DPS){
		fResolution = 0.0076; //(2 * 250) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_500DPS){
		fResolution = 0.0153; //(2 * 500) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_1000DPS){
		fResolution = 0.0305; //(2 * 1000) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_2000DPS){
		fResolution = 0.061; //(2.0 * 2000.0) / 65536.0;
		return i32GyroVal * fResolution;
	}
	
	return 0;
}


static float bmx055_read_y_gyro(
	I2C_T* i2c_bus,
	S_GYRO_PARAM *psGyroParam
)
{
	uint8_t u8LSB;
	uint8_t u8MSB;
	int32_t i32GyroVal;
	float fResolution;
	
#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif
	
	u8LSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, 0x04);
	u8MSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, 0x05);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

	i32GyroVal =  u8MSB;
	i32GyroVal = i32GyroVal << 8;
	i32GyroVal |=  u8LSB;
	i32GyroVal = twos_comp_to_int(i32GyroVal, 16);

	if(psGyroParam->u32Range == BIT_GYRO_RANGE_125DPS){
		fResolution = 0.0038; //(2 * 125) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_250DPS){
		fResolution = 0.0076; //(2 * 250) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_500DPS){
		fResolution = 0.0153; //(2 * 500) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_1000DPS){
		fResolution = 0.0305; //(2 * 1000) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_2000DPS){
		fResolution = 0.061; //(2 * 2000) / 65536;
		return i32GyroVal * fResolution;
	}
	
	return 0;
}


static float bmx055_read_z_gyro(
	I2C_T* i2c_bus,
	S_GYRO_PARAM *psGyroParam
)
{
	uint8_t u8LSB;
	uint8_t u8MSB;
	int32_t i32GyroVal;
	float fResolution;
	
#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

	u8LSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, 0x06);
	u8MSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_GYRO_ADDR, 0x07);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

	i32GyroVal =  u8MSB;
	i32GyroVal = i32GyroVal << 8;
	i32GyroVal |=  u8LSB;
	i32GyroVal = twos_comp_to_int(i32GyroVal, 16);

	if(psGyroParam->u32Range == BIT_GYRO_RANGE_125DPS){
		fResolution = 0.0038; //(2 * 125) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_250DPS){
		fResolution = 0.0076; //(2 * 250) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_500DPS){
		fResolution = 0.0153; //(2 * 500) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_1000DPS){
		fResolution = 0.0305; //(2 * 1000) / 65536;
		return i32GyroVal * fResolution;
	}
	else if(psGyroParam->u32Range == BIT_GYRO_RANGE_2000DPS){
		fResolution = 0.061; //(2 * 2000) / 65536;
		return i32GyroVal * fResolution;
	}
	
	return 0;
}

static float bmx055_read_x_mag(
	I2C_T* i2c_bus,
	S_MAG_PARAM *psMagParam
)
{
	uint8_t u8LSB;
	uint8_t u8MSB;
	int32_t i32MagVal;
	
#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

	u8LSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, 0x42);
	u8MSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, 0x43);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

	i32MagVal =  u8MSB;
	i32MagVal = i32MagVal << 5;
	i32MagVal |=  (u8LSB >> 3);
	i32MagVal = twos_comp_to_int(i32MagVal, 13);

	return i32MagVal;
}

static float bmx055_read_y_mag(
	I2C_T* i2c_bus,
	S_MAG_PARAM *psMagParam
)
{
	uint8_t u8LSB;
	uint8_t u8MSB;
	int32_t i32MagVal;

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif
	
	u8LSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, 0x44);
	u8MSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, 0x45);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

	i32MagVal =  u8MSB;
	i32MagVal = i32MagVal << 5;
	i32MagVal |=  (u8LSB >> 3);
	i32MagVal = twos_comp_to_int(i32MagVal, 13);

	return i32MagVal;
}

static float bmx055_read_z_mag(
	I2C_T* i2c_bus,
	S_MAG_PARAM *psMagParam
)
{
	uint8_t u8LSB;
	uint8_t u8MSB;
	int32_t i32MagVal;
	
#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

	u8LSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, 0x46);
	u8MSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, 0x47);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

	i32MagVal =  u8MSB;
	i32MagVal = i32MagVal << 7;
	i32MagVal |=  (u8LSB >> 1);
	i32MagVal = twos_comp_to_int(i32MagVal, 15);

	return i32MagVal;
}

static float bmx055_read_hall_mag(
	I2C_T* i2c_bus,
	S_MAG_PARAM *psMagParam
)
{
	uint8_t u8LSB;
	uint8_t u8MSB;
	int32_t i32MagVal;
	
#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif
	u8LSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, 0x48);
	u8MSB = I2C_ReadByteOneReg(i2c_bus, BMX055_DEFAULT_MAG_ADDR, 0x49);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif


	i32MagVal =  u8MSB;
	i32MagVal = i32MagVal << 6;
	i32MagVal |=  (u8LSB >> 2);
	i32MagVal = twos_comp_to_int(i32MagVal, 14);

	return i32MagVal;
}



STATIC bool bmx_i2c_bus_init(I2C_T *i2c){
	int i2c_unit;

    if (i2c == I2C0) {
        i2c_unit = 0;
    } else if (i2c == I2C1) {
        i2c_unit = 1;
    } else if (i2c == I2C2) {
        i2c_unit = 2;
    } else {
        // I2C does not exist for this board (shouldn't get here, should be checked by caller)
        return false;
    }

#if  MICROPY_PY_THREAD
	mp_thread_mutex_init(&s_I2C_mutex);
#endif

   // init the GPIO lines
    uint32_t mode = MP_HAL_PIN_MODE_ALT_PUSH_PULL;
    mp_hal_pin_config_alt(MICROPY_BMX055_SCL, mode, AF_FN_I2C, i2c_unit);
    mp_hal_pin_config_alt(MICROPY_BMX055_SDA, mode, AF_FN_I2C, i2c_unit);

	I2C_InitTypeDef init;

	init.Mode = I2C_MODE_MASTER;	//Master/Slave mode: I2C_MODE_MASTER/I2C_MODE_SLAVE
	init.BaudRate = 100000;			//Baud rate
	init.GeneralCallMode = 0;		//Support general call mode: I2C_GCMODE_ENABLE/I2C_GCMODE_DISABLE
	init.OwnAddress = 0;			//Slave own address 


    // init the I2C device
    if (I2C_Init(i2c, &init) != 0) {
        // init error
        // TODO should raise an exception, but this function is not necessarily going to be
        // called via Python, so may not be properly wrapped in an NLR handler
        printf("OSError: HAL_I2C_Init failed\n");
        return false;
    }
	
	return true;
}


STATIC void bmx_i2c_bus_deinit(I2C_T *i2c) {

	I2C_Final(i2c);
	
	mp_hal_pin_config(MICROPY_BMX055_SCL, GPIO_MODE_INPUT, 0);
	mp_hal_pin_config(MICROPY_BMX055_SDA, GPIO_MODE_INPUT, 0);
	
}



/******************************************************************************/
/* MicroPython bindings                                                      */

typedef struct _pyb_accel_obj_t {
    mp_obj_base_t base;
    bool bBusEnable;
    bool bAccelEnable;
	I2C_T* i2c;
	S_ACCEL_PARAM sAccelParam;
} pyb_accel_obj_t;

STATIC pyb_accel_obj_t pyb_accel_obj = {
    .base = {&pyb_accel_type},
    .bBusEnable = false,
    .bAccelEnable = false,
    .i2c = I2C0,
};

typedef struct _pyb_gyro_obj_t {
    mp_obj_base_t base;
    bool bBusEnable;
    bool bGyroEnable;
	I2C_T* i2c;
	S_GYRO_PARAM sGyroParam;
} pyb_gyro_obj_t;

STATIC pyb_gyro_obj_t pyb_gyro_obj = {
    .base = {&pyb_gyro_type},
    .bBusEnable = false,
    .bGyroEnable = false,
    .i2c = I2C0,
};

typedef struct _pyb_mag_obj_t {
    mp_obj_base_t base;
    bool bBusEnable;
    bool bMagEnable;
	I2C_T* i2c;
	S_MAG_PARAM sMagParam;
} pyb_mag_obj_t;

STATIC pyb_mag_obj_t pyb_mag_obj = {
    .base = {&pyb_mag_type},
    .bBusEnable = false,
    .bMagEnable = false,
    .i2c = I2C0,
};


STATIC mp_obj_t pyb_accel_init_helper(pyb_accel_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_range,         MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = BIT_ACC_RANGE_8G} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

#if defined(BMX055_ACCEL_SELF_TEST)
	self->sAccelParam.u32Range = args[0].u_int;
#else
	self->sAccelParam.u32Range = BIT_ACC_RANGE_8G;
#endif

	if(bmx055_init(self->i2c, eBMX055_ACCEL, &self->sAccelParam) != eBMX055_OK){
		printf("Unable init bmx055 accelerometer \n");
	}
	else{
		self->bAccelEnable = true;
	}    
	return mp_const_none;
}

/// \classmethod \constructor()
/// Create and return an accelerometer object.
///
/// Note: if you read accelerometer values immediately after creating this object
/// you will get 0.  It takes around 20ms for the first sample to be ready, so,
/// unless you have some other code between creating this object and reading its
/// values, you should put a `pyb.delay(20)` after creating it.  For example:
///
///     accel = pyb.Accel()
///     pyb.delay(20)
///     print(accel.x())
STATIC mp_obj_t pyb_accel_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 1, true);

	const pin_af_obj_t *af_scl = pin_find_af_by_fn_type(MICROPY_BMX055_SCL, AF_FN_I2C, AF_PIN_TYPE_I2C_SCL);
	const pin_af_obj_t *af_sda = pin_find_af_by_fn_type(MICROPY_BMX055_SDA, AF_FN_I2C, AF_PIN_TYPE_I2C_SDA);
	
	if((af_scl == NULL) || (af_sda == NULL)){
       nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Invalid I2C SCL and SDA pin"));
	}

	if(pyb_accel_obj.bAccelEnable){
		return MP_OBJ_FROM_PTR(&pyb_accel_obj);
	}

    pyb_accel_obj.i2c = (I2C_T *)af_scl->reg;

	if((pyb_accel_obj.bBusEnable == false) && (pyb_gyro_obj.bBusEnable == false) &&  (pyb_mag_obj.bBusEnable == false)){
		if(bmx_i2c_bus_init(pyb_accel_obj.i2c) == false){
		   nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Unable init I2C bus"));
		}
	}

	pyb_accel_obj.bBusEnable = true;

	mp_map_t kw_args;
	mp_map_init_fixed_table(&kw_args, n_kw, args);
	pyb_accel_init_helper(&pyb_accel_obj, 0, args, &kw_args);

    return MP_OBJ_FROM_PTR(&pyb_accel_obj);
}

STATIC mp_obj_t pyb_accel_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_accel_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(pyb_accel_init_obj, 1, pyb_accel_init);

// acced.deinit()
STATIC mp_obj_t pyb_accel_deinit(mp_obj_t self_in) {
    pyb_accel_obj_t *self = self_in;

	if(self->bAccelEnable == true){
		bmx055_deinit(self->i2c, eBMX055_ACCEL); 
		self->bAccelEnable = false;
		
		if((pyb_gyro_obj.bBusEnable == false) && (pyb_mag_obj.bBusEnable == false))
			bmx_i2c_bus_deinit(self->i2c);
		self->bBusEnable = false;
	}

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(pyb_accel_deinit_obj, pyb_accel_deinit);

/// \method x()
/// Get the x-axis value.
STATIC mp_obj_t pyb_accel_x(mp_obj_t self_in) {
	pyb_accel_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(bmx055_read_x_accel(self->i2c, &self->sAccelParam));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_accel_x_obj, pyb_accel_x);

/// \method y()
/// Get the y-axis value.
STATIC mp_obj_t pyb_accel_y(mp_obj_t self_in) {
	pyb_accel_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(bmx055_read_y_accel(self->i2c, &self->sAccelParam));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_accel_y_obj, pyb_accel_y);

/// \method z()
/// Get the z-axis value.
STATIC mp_obj_t pyb_accel_z(mp_obj_t self_in) {
	pyb_accel_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(bmx055_read_z_accel(self->i2c, &self->sAccelParam));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_accel_z_obj, pyb_accel_z);

STATIC mp_obj_t pyb_accel_read(mp_obj_t self_in, mp_obj_t reg) {
    uint8_t data;
	uint8_t u8Reg = mp_obj_get_int(reg);
	pyb_accel_obj_t *self = MP_OBJ_TO_PTR(self_in);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

	data = I2C_ReadByteOneReg(self->i2c, BMX055_DEFAULT_ACCEL_ADDR, u8Reg);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

    return mp_obj_new_int(data);
}
MP_DEFINE_CONST_FUN_OBJ_2(pyb_accel_read_obj, pyb_accel_read);

STATIC mp_obj_t pyb_accel_write(mp_obj_t self_in, mp_obj_t reg, mp_obj_t val) {
	uint8_t u8Reg = mp_obj_get_int(reg);
	pyb_accel_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t data = mp_obj_get_int(val);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

	I2C_WriteByteOneReg(self->i2c, BMX055_DEFAULT_ACCEL_ADDR, u8Reg, data);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(pyb_accel_write_obj, pyb_accel_write);



STATIC const mp_rom_map_elem_t pyb_accel_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_accel_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_accel_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&pyb_accel_x_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&pyb_accel_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_z), MP_ROM_PTR(&pyb_accel_z_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&pyb_accel_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&pyb_accel_write_obj) },

    { MP_ROM_QSTR(MP_QSTR_RANGE_2G), MP_ROM_INT(BIT_ACC_RANGE_2G) },
    { MP_ROM_QSTR(MP_QSTR_RANGE_4G), MP_ROM_INT(BIT_ACC_RANGE_4G) },
    { MP_ROM_QSTR(MP_QSTR_RANGE_8G), MP_ROM_INT(BIT_ACC_RANGE_8G) },
    { MP_ROM_QSTR(MP_QSTR_RANGE_16G), MP_ROM_INT(BIT_ACC_RANGE_16G) },

};

STATIC MP_DEFINE_CONST_DICT(pyb_accel_locals_dict, pyb_accel_locals_dict_table);

const mp_obj_type_t pyb_accel_type = {
    { &mp_type_type },
    .name = MP_QSTR_Accel,
    .make_new = pyb_accel_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_accel_locals_dict,
};



STATIC mp_obj_t pyb_gyro_init_helper(pyb_gyro_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_range,         MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = BIT_GYRO_RANGE_2000DPS} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	self->sGyroParam.u32Range = args[0].u_int;

	if(bmx055_init(self->i2c, eBMX055_GYRO, &self->sGyroParam) != eBMX055_OK){
		printf("Unable init bmx055 gyroscope \n");
	}
	else{
		self->bGyroEnable = true;
	}    
	return mp_const_none;
}



/// \classmethod \constructor()
/// Create and return a gyroscope object.
///
/// Note: if you read gyroscope values immediately after creating this object
/// you will get 0.  It takes around 20ms for the first sample to be ready, so,
/// unless you have some other code between creating this object and reading its
/// values, you should put a `pyb.delay(20)` after creating it.  For example:
///
///     gyro = pyb.Gyro()
///     pyb.delay(20)
///     print(gyro.x())
STATIC mp_obj_t pyb_gyro_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 1, true);

	const pin_af_obj_t *af_scl = pin_find_af_by_fn_type(MICROPY_BMX055_SCL, AF_FN_I2C, AF_PIN_TYPE_I2C_SCL);
	const pin_af_obj_t *af_sda = pin_find_af_by_fn_type(MICROPY_BMX055_SDA, AF_FN_I2C, AF_PIN_TYPE_I2C_SDA);
	
	if((af_scl == NULL) || (af_sda == NULL)){
       nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Invalid I2C SCL and SDA pin"));
	}

	if(pyb_gyro_obj.bGyroEnable){
		return MP_OBJ_FROM_PTR(&pyb_gyro_obj);
	}

    pyb_gyro_obj.i2c = (I2C_T *)af_scl->reg;

	if((pyb_accel_obj.bBusEnable == false) && (pyb_gyro_obj.bBusEnable == false) && (pyb_mag_obj.bBusEnable == false) ){
		if(bmx_i2c_bus_init(pyb_gyro_obj.i2c) == false){
		   nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Unable init I2C bus"));
		}
	}

	pyb_gyro_obj.bBusEnable = true;

	mp_map_t kw_args;
	mp_map_init_fixed_table(&kw_args, n_kw, args);
	pyb_gyro_init_helper(&pyb_gyro_obj, 0, args, &kw_args);

    return MP_OBJ_FROM_PTR(&pyb_gyro_obj);


}

STATIC mp_obj_t pyb_gyro_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_gyro_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(pyb_gyro_init_obj, 1, pyb_gyro_init);

// gyro.deinit()
STATIC mp_obj_t pyb_gyro_deinit(mp_obj_t self_in) {
    pyb_gyro_obj_t *self = self_in;

	if(self->bGyroEnable == true){
		bmx055_deinit(self->i2c, eBMX055_GYRO); 
		self->bGyroEnable = false;
		
		if((pyb_accel_obj.bBusEnable == false) && (pyb_mag_obj.bBusEnable == false))
			bmx_i2c_bus_deinit(self->i2c);
		self->bBusEnable = false;
	}

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(pyb_gyro_deinit_obj, pyb_gyro_deinit);

/// \method x()
/// Get the x-axis value.
STATIC mp_obj_t pyb_gyro_x(mp_obj_t self_in) {
	pyb_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(bmx055_read_x_gyro(self->i2c, &self->sGyroParam));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_gyro_x_obj, pyb_gyro_x);

/// \method y()
/// Get the y-axis value.
STATIC mp_obj_t pyb_gyro_y(mp_obj_t self_in) {
	pyb_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(bmx055_read_y_gyro(self->i2c, &self->sGyroParam));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_gyro_y_obj, pyb_gyro_y);

/// \method z()
/// Get the z-axis value.
STATIC mp_obj_t pyb_gyro_z(mp_obj_t self_in) {
	pyb_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(bmx055_read_z_gyro(self->i2c, &self->sGyroParam));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_gyro_z_obj, pyb_gyro_z);

STATIC mp_obj_t pyb_gyro_read(mp_obj_t self_in, mp_obj_t reg) {
    uint8_t data;
	uint8_t u8Reg = mp_obj_get_int(reg);
	pyb_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

	data = I2C_ReadByteOneReg(self->i2c, BMX055_DEFAULT_GYRO_ADDR, u8Reg);
#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

    return mp_obj_new_int(data);
}
MP_DEFINE_CONST_FUN_OBJ_2(pyb_gyro_read_obj, pyb_gyro_read);

STATIC mp_obj_t pyb_gyro_write(mp_obj_t self_in, mp_obj_t reg, mp_obj_t val) {
	uint8_t u8Reg = mp_obj_get_int(reg);
	pyb_gyro_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t data = mp_obj_get_int(val);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

	I2C_WriteByteOneReg(self->i2c, BMX055_DEFAULT_GYRO_ADDR, u8Reg, data);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(pyb_gyro_write_obj, pyb_gyro_write);


STATIC const mp_rom_map_elem_t pyb_gyro_locals_dict_table[] = {

    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_gyro_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_gyro_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&pyb_gyro_x_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&pyb_gyro_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_z), MP_ROM_PTR(&pyb_gyro_z_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&pyb_gyro_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&pyb_gyro_write_obj) },

    { MP_ROM_QSTR(MP_QSTR_RANGE_125DPS), MP_ROM_INT(BIT_GYRO_RANGE_125DPS) },
    { MP_ROM_QSTR(MP_QSTR_RANGE_250DPS), MP_ROM_INT(BIT_GYRO_RANGE_250DPS) },
    { MP_ROM_QSTR(MP_QSTR_RANGE_500DPS), MP_ROM_INT(BIT_GYRO_RANGE_500DPS) },
    { MP_ROM_QSTR(MP_QSTR_RANGE_1000DPS), MP_ROM_INT(BIT_GYRO_RANGE_1000DPS) },
    { MP_ROM_QSTR(MP_QSTR_RANGE_2000DPS), MP_ROM_INT(BIT_GYRO_RANGE_2000DPS) },

};

STATIC MP_DEFINE_CONST_DICT(pyb_gyro_locals_dict, pyb_gyro_locals_dict_table);


const mp_obj_type_t pyb_gyro_type = {
    { &mp_type_type },
    .name = MP_QSTR_Gyro,
    .make_new = pyb_gyro_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_gyro_locals_dict,
};


STATIC mp_obj_t pyb_mag_init_helper(pyb_mag_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	if(bmx055_init(self->i2c, eBMX055_MAG, &self->sMagParam) != eBMX055_OK){
		printf("Unable init bmx055 magnetometer \n");
	}
	else{
		self->bMagEnable = true;
	}    
	return mp_const_none;
}

/// \classmethod \constructor()
/// Create and return a magnetometer object.
///
/// Note: if you read magnetometer values immediately after creating this object
/// you will get 0.  It takes around 20ms for the first sample to be ready, so,
/// unless you have some other code between creating this object and reading its
/// values, you should put a `pyb.delay(20)` after creating it.  For example:
///
///     mag = pyb.Mag()
///     pyb.delay(20)
///     print(mag.x())
STATIC mp_obj_t pyb_mag_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 1, true);

	const pin_af_obj_t *af_scl = pin_find_af_by_fn_type(MICROPY_BMX055_SCL, AF_FN_I2C, AF_PIN_TYPE_I2C_SCL);
	const pin_af_obj_t *af_sda = pin_find_af_by_fn_type(MICROPY_BMX055_SDA, AF_FN_I2C, AF_PIN_TYPE_I2C_SDA);
	
	if((af_scl == NULL) || (af_sda == NULL)){
       nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Invalid I2C SCL and SDA pin"));
	}

	if(pyb_mag_obj.bMagEnable){
		return MP_OBJ_FROM_PTR(&pyb_mag_obj);
	}

    pyb_mag_obj.i2c = (I2C_T *)af_scl->reg;

	if((pyb_accel_obj.bBusEnable == false) && (pyb_gyro_obj.bBusEnable == false) &&  (pyb_mag_obj.bBusEnable == false)){
		if(bmx_i2c_bus_init(pyb_mag_obj.i2c) == false){
		   nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Unable init I2C bus"));
		}
	}

	pyb_mag_obj.bBusEnable = true;

	mp_map_t kw_args;
	mp_map_init_fixed_table(&kw_args, n_kw, args);
	pyb_mag_init_helper(&pyb_mag_obj, 0, args, &kw_args);

    return MP_OBJ_FROM_PTR(&pyb_mag_obj);


}

STATIC mp_obj_t pyb_mag_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_mag_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(pyb_mag_init_obj, 1, pyb_mag_init);

// mag.deinit()
STATIC mp_obj_t pyb_mag_deinit(mp_obj_t self_in) {
    pyb_mag_obj_t *self = self_in;

	if(self->bMagEnable == true){
		bmx055_deinit(self->i2c, eBMX055_MAG); 
		self->bMagEnable = false;
		
		if((pyb_accel_obj.bBusEnable == false) && (pyb_gyro_obj.bBusEnable == false))
			bmx_i2c_bus_deinit(self->i2c);
		self->bBusEnable = false;
	}

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(pyb_mag_deinit_obj, pyb_mag_deinit);

/// \method x()
/// Get the x-axis value.
STATIC mp_obj_t pyb_mag_x(mp_obj_t self_in) {
	pyb_mag_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(bmx055_read_x_mag(self->i2c, &self->sMagParam));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_mag_x_obj, pyb_mag_x);

/// \method y()
/// Get the y-axis value.
STATIC mp_obj_t pyb_mag_y(mp_obj_t self_in) {
	pyb_mag_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(bmx055_read_y_mag(self->i2c, &self->sMagParam));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_mag_y_obj, pyb_mag_y);

/// \method z()
/// Get the z-axis value.
STATIC mp_obj_t pyb_mag_z(mp_obj_t self_in) {
	pyb_mag_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(bmx055_read_z_mag(self->i2c, &self->sMagParam));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_mag_z_obj, pyb_mag_z);

/// \method hall()
/// Get the hall value.
STATIC mp_obj_t pyb_mag_hall(mp_obj_t self_in) {
	pyb_mag_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_float(bmx055_read_hall_mag(self->i2c, &self->sMagParam));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_mag_hall_obj, pyb_mag_hall);

STATIC mp_obj_t pyb_mag_read(mp_obj_t self_in, mp_obj_t reg) {
    uint8_t data;
	uint8_t u8Reg = mp_obj_get_int(reg);
	pyb_mag_obj_t *self = MP_OBJ_TO_PTR(self_in);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

	data = I2C_ReadByteOneReg(self->i2c, BMX055_DEFAULT_MAG_ADDR, u8Reg);
#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

    return mp_obj_new_int(data);
}
MP_DEFINE_CONST_FUN_OBJ_2(pyb_mag_read_obj, pyb_mag_read);

STATIC mp_obj_t pyb_mag_write(mp_obj_t self_in, mp_obj_t reg, mp_obj_t val) {
	uint8_t u8Reg = mp_obj_get_int(reg);
	pyb_mag_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t data = mp_obj_get_int(val);

#if  MICROPY_PY_THREAD
	mp_thread_mutex_lock(&s_I2C_mutex, 1);
#endif

	I2C_WriteByteOneReg(self->i2c, BMX055_DEFAULT_MAG_ADDR, u8Reg, data);
#if  MICROPY_PY_THREAD
	mp_thread_mutex_unlock(&s_I2C_mutex);
#endif

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(pyb_mag_write_obj, pyb_mag_write);


STATIC const mp_rom_map_elem_t pyb_mag_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_mag_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_mag_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_PTR(&pyb_mag_x_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_PTR(&pyb_mag_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_z), MP_ROM_PTR(&pyb_mag_z_obj) },
    { MP_ROM_QSTR(MP_QSTR_hall), MP_ROM_PTR(&pyb_mag_hall_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&pyb_gyro_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&pyb_gyro_write_obj) },

};

STATIC MP_DEFINE_CONST_DICT(pyb_mag_locals_dict, pyb_mag_locals_dict_table);


const mp_obj_type_t pyb_mag_type = {
    { &mp_type_type },
    .name = MP_QSTR_Mag,
    .make_new = pyb_mag_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_mag_locals_dict,
};



#endif
