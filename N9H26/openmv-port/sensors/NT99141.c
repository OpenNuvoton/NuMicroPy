//      Copyright (c) 2021 Nuvoton Technology Corp. All rights reserved.
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either m_uiVersion 2 of the License, or
//      (at your option) any later m_uiVersion.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

#include <stdio.h>
#include <inttypes.h>

#include "py/runtime.h"
#include "py/mphal.h"

#include "wblib.h"
#include "N9H26_I2C.h"

#include "omv_boardconfig.h"
#include "sensor.h"

#define REG_TABLE_INIT  0
#define REG_TABLE_VGA   1   //640X480
#define REG_TABLE_SVGA  2   //800X600
#define REG_TABLE_HD720 3   //1280X720

struct NT_RegValue
{
    uint16_t    u16RegAddr;     //Register Address
    uint8_t u8Value;            //Register Data
};

#define _REG_TABLE_SIZE(nTableName) sizeof(nTableName)/sizeof(struct NT_RegValue)

struct NT_RegTable
{
    struct NT_RegValue *psRegTable;
    uint16_t u16TableSize;
};

static struct NT_RegValue s_sNT99141_Init[] =
{
#include "NT99141/NT99141_Init.dat"
};

#if !defined(SENSOR_PCLK_48MHZ) && !defined(SENSOR_PCLK_60MHZ)&&!defined(SENSOR_PCLK_74MHZ)
static struct NT_RegValue s_sNT99141_HD720[] =
{
    0x0, 0x0
};
static struct NT_RegValue s_sNT99141_SVGA[] =
{
    0x0, 0x0
};
static struct NT_RegValue s_sNT99141_VGA[] =
{
    0x0, 0x0
};
#endif

#if (SENSOR_PCLK == SENSOR_PCLK_48MHZ)
static struct NT_RegValue s_sNT99141_HD720[] =
{
#include "NT99141/NT99141_HD720_PCLK_48MHz.dat"
};

static struct NT_RegValue s_sNT99141_SVGA[] =
{
#include "NT99141/NT99141_SVGA_PCLK_48MHz.dat"
};

static struct NT_RegValue s_sNT99141_VGA[] =
{
#include "NT99141/NT99141_VGA_PCLK_48MHz.dat"
};

#endif

#if  (SENSOR_PCLK == SENSOR_PCLK_60MHZ)
static struct NT_RegValue s_sNT99141_HD720[] =
{
#include "NT99141/NT99141_HD720_PCLK_60MHz.dat"
};

static struct NT_RegValue s_sNT99141_SVGA[] =
{
#include "NT99141/NT99141_SVGA_PCLK_60MHz.dat"
};

static struct NT_RegValue s_sNT99141_VGA[] =
{
#include "NT99141/NT99141_VGA_PCLK_60MHz.dat"
};

#endif

#if (SENSOR_PCLK == SENSOR_PCLK_74MHZ)
static struct NT_RegValue s_sNT99141_HD720[] =
{
#include "NT99141/NT99141_HD720_PCLK_74MHz.dat"
};

static struct NT_RegValue s_sNT99141_SVGA[] =
{
#include "NT99141/NT99141_SVGA_PCLK_74MHz.dat"
};

static struct NT_RegValue s_sNT99141_VGA[] =
{
#include "NT99141/NT99141_VGA_PCLK_74MHz.dat"
};

#endif

static struct NT_RegTable s_NT99141SnrTable[] =
{

    {s_sNT99141_Init, _REG_TABLE_SIZE(s_sNT99141_Init)},
    {s_sNT99141_VGA, _REG_TABLE_SIZE(s_sNT99141_VGA)},
    {s_sNT99141_SVGA, _REG_TABLE_SIZE(s_sNT99141_SVGA)},
    {s_sNT99141_HD720, _REG_TABLE_SIZE(s_sNT99141_HD720)},

    {0, 0}
};

typedef struct
{
    uint16_t u16ImageWidth;
    uint16_t u16ImageHeight;
    int8_t i8ResolIdx;
} S_NTSuppResol;

#define NT_RESOL_SUPP_CNT  4

static S_NTSuppResol s_asNTSuppResolTable[NT_RESOL_SUPP_CNT] =
{
    {0, 0, REG_TABLE_INIT},
    {640, 480, REG_TABLE_VGA},
    {800, 600, REG_TABLE_SVGA},
    {1280, 720, REG_TABLE_HD720}
};

static UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
{
    UINT8 u8Data;

    i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, uAddr >> 1, 0);
    i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0);
    i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, uRegAddr, 2);
    i2cRead(&u8Data, 1);

    return u8Data;
}

static BOOL I2C_Write_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr, UINT8 uData)
{
    i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, uAddr >> 1, 0);
    i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0);
    i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, uRegAddr, 2);
    i2cWrite(&uData, 1);

    return TRUE;
}

static int reset(sensor_t *sensor)
{
	//Soft reset sensor
	uint16_t u16RegAddr = 0x3021;
	uint8_t u8RegData;
	uint8_t u8RegVal;
    uint16_t u16TableSize;
    struct NT_RegValue *psRegValue;
	int i;
	
	u8RegData = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, u16RegAddr);
		
	u8RegData |= 0x01;
	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, u16RegAddr, u8RegData);
	 
	/* delay n ms */
	mp_hal_delay_ms(10);

	//read sensor chip id
	u8RegData = 0;
	u8RegData = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3000);
	if(u8RegData != 0x14){
		printf("sensor nt99141 reset failed\n");
		return -1;
	}

	//Set init table
    u16TableSize = s_NT99141SnrTable[REG_TABLE_INIT].u16TableSize;
    psRegValue = s_NT99141SnrTable[REG_TABLE_INIT].psRegTable;

    for (i = 0; i < u16TableSize; i++, psRegValue++)
    {
		/* delay n ms */
		mp_hal_delay_ms(1);
        I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, (psRegValue->u16RegAddr), (psRegValue->u8Value));

		u8RegVal = 0;
		u8RegVal = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, psRegValue->u16RegAddr);

		if (u8RegVal != psRegValue->u8Value)
			printf("Write sensor address error = 0x%x 0x%x\n", psRegValue->u16RegAddr, u8RegVal);

    }

	return 0;
}

static int set_pixformat(sensor_t *sensor, pixformat_t pixformat)
{
	//Using VIN engine to change pixformat. Implement in sensor.c
	return 0;
}

static int read_reg(sensor_t *sensor, uint16_t reg_addr)
{
    uint8_t u8RegData = 0;

	u8RegData = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, reg_addr);

    return u8RegData;
}

static int write_reg(sensor_t *sensor, uint16_t reg_addr, uint16_t reg_data)
{
	BOOL bRet;
	bRet = I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, reg_addr, (uint8_t)reg_data);
	return bRet;
}

static int set_framesize(sensor_t *sensor, framesize_t framesize)
{
    uint16_t w = resolution[framesize][0];
    uint16_t h = resolution[framesize][1];

    int8_t i = 0;
    int8_t i8WidthIdx;
    int8_t i8HeightIdx;
    int8_t i8SensorIdx;

    uint16_t u16TableSize;
    struct NT_RegValue *psRegValue;

	if((w > 1280) || (h > 720))
		return  -1;
	
    for(i = 0; i < NT_RESOL_SUPP_CNT ; i ++)
    {
        if (w <= s_asNTSuppResolTable[i].u16ImageWidth)
            break;
    }
    
    if(i == NT_RESOL_SUPP_CNT)
		return -2;	

	i8WidthIdx = i;

    for (i = 0; i < NT_RESOL_SUPP_CNT ; i ++)
    {
        if (h <= s_asNTSuppResolTable[i].u16ImageHeight)
            break;
    }
    
    if (i == NT_RESOL_SUPP_CNT)
        return -3;

    i8HeightIdx = i;

    if (i8HeightIdx >= i8WidthIdx)
    {
        i8SensorIdx = i8HeightIdx;
    }
    else
    {
        i8SensorIdx = i8WidthIdx;
    }

	//Set resolution registers

	u16TableSize = s_NT99141SnrTable[i8SensorIdx].u16TableSize;
	psRegValue = s_NT99141SnrTable[i8SensorIdx].psRegTable;

	uint8_t u8RegVal;

	for (i = 0; i < u16TableSize; i++, psRegValue++)
	{
		mp_hal_delay_ms(1);
		I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, (psRegValue->u16RegAddr), (psRegValue->u8Value));

		u8RegVal = 0;
		u8RegVal = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, psRegValue->u16RegAddr);

		if (u8RegVal != psRegValue->u8Value)
			printf("Write sensor address error = 0x%x 0x%x\n", psRegValue->u16RegAddr, u8RegVal);

	}

	//return actual resolutaion. sensor.c will cropping image to fit application requirment.
	if(s_asNTSuppResolTable[i8SensorIdx].i8ResolIdx == REG_TABLE_VGA)
		return FRAMESIZE_VGA;

	if(s_asNTSuppResolTable[i8SensorIdx].i8ResolIdx == REG_TABLE_SVGA)
		return FRAMESIZE_SVGA;

	if(s_asNTSuppResolTable[i8SensorIdx].i8ResolIdx == REG_TABLE_HD720)
		return FRAMESIZE_HD;

	return 0;
}

#define NUM_CONTRAST_LEVELS (9)
static const uint8_t contrast_regs[NUM_CONTRAST_LEVELS][1] = {
    {0x40}, /* -4 */
    {0x50}, /* -3 */
    {0x60}, /* -2 */
    {0x70}, /* -1 */
    {0x80}, /*  0 */
    {0x90}, /* +1 */
    {0xA0}, /* +2 */
    {0xB0}, /* +3 */
    {0xC0}, /* +4 */
};

#define NUM_BRIGHTNESS_LEVELS (9)
static const uint8_t brightness_regs[NUM_BRIGHTNESS_LEVELS][1] = {
    {0x40}, /* -4 */
    {0x50}, /* -3 */
    {0x60}, /* -2 */
    {0x70}, /* -1 */
    {0x80}, /*  0 */
    {0x90}, /* +1 */
    {0xA0}, /* +2 */
    {0xB0}, /* +3 */
    {0xC0}, /* +4 */
};

static int set_contrast(sensor_t *sensor, int level)
{
    level += (NUM_CONTRAST_LEVELS / 2);
    if (level < 0 || level >= NUM_CONTRAST_LEVELS) {
        return -1;
    }

	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x32F6, 0x04);
	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x32F2, contrast_regs[level][0]);
	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3060, 0x01);

    return 0;
}

static int set_brightness(sensor_t *sensor, int level)
{
    level += (NUM_BRIGHTNESS_LEVELS / 2);
    if (level < 0 || level >= NUM_BRIGHTNESS_LEVELS) {
        return -1;
    }

	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x32F6, 0x04);
	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x32FC, brightness_regs[level][0]);
	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3060, 0x01);

    return 0;
}

static int set_hmirror(sensor_t *sensor, int enable)
{
	uint8_t u8CurVal;
	uint8_t u8BitMask = 0x02;
	
	u8CurVal = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3022);

	if(enable)
	{
		u8CurVal |= u8BitMask;
	}
	else
	{
		u8CurVal &= ~u8BitMask;
	}

	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3022, u8CurVal);
	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3060, 0x01);

	return 0;
}

static int set_vflip(sensor_t *sensor, int enable)
{
	uint8_t u8CurVal;
	uint8_t u8BitMask = 0x01;
	
	u8CurVal = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3022);

	if(enable)
	{
		u8CurVal |= u8BitMask;
	}
	else
	{
		u8CurVal &= ~u8BitMask;
	}

	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3022, u8CurVal);
	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3060, 0x01);

	return 0;
}

static int set_special_effect(sensor_t *sensor, sde_t sde)
{
    int ret=0;
	uint8_t u8CurVal;

    switch (sde) {
		case SDE_NEGATIVE:
			//Enable special effect
			u8CurVal = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3201);
			u8CurVal |= 0x01;
			I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3201, u8CurVal);

			u8CurVal = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x32F1);
			u8CurVal &= ~0x07;
			u8CurVal |= 0x03;  //Set Negative mode
			I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x32F1, u8CurVal);

            break;
        case SDE_NORMAL:
			u8CurVal = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x32F1);
			u8CurVal &= ~0x07;	//Set Normal 
			I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x32F1, u8CurVal);

			//Disable special effect
			u8CurVal = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3201);
			u8CurVal &= ~0x01;
			I2C_Write_8bitSlaveAddr_16bitReg_8bitData(sensor->slv_addr, 0x3201, u8CurVal);
            break;
        default:
            return -1;
    }

    return ret;
}

int NT99141_init(sensor_t *sensor)
{
    // Initialize sensor structure.
	int32_t i32Ret;
	int32_t i2c_unit = 0;

   // init the GPIO lines
    uint32_t mode = MP_HAL_PIN_MODE_ALT;
    mp_hal_pin_config_alt(SENSOR_I2C_SCL_PIN, mode, AF_FN_I2C, i2c_unit);
    mp_hal_pin_config_alt(SENSOR_I2C_SDA_PIN, mode, AF_FN_I2C, i2c_unit);

	outpw(REG_APBIPRST, inpw(REG_APBIPRST) | I2CRST);	//reset i2c
	outpw(REG_APBIPRST, inpw(REG_APBIPRST) & ~I2CRST);	
	
	i32Ret = i2cOpen();

	if(i32Ret == I2C_ERR_BUSY)
	{
		printf("Unable open I2C bus: busy \n");
		return -1;
	}

	i2cIoctl(I2C_IOC_SET_SPEED, 100, 0);

	sensor->gs_bpp              = 2;
	sensor->reset               = reset;
	sensor->set_pixformat       = set_pixformat;
	sensor->read_reg            = read_reg;
	sensor->write_reg           = write_reg;
	sensor->set_framesize       = set_framesize;
    sensor->set_contrast        = set_contrast;
    sensor->set_brightness      = set_brightness;
    sensor->set_hmirror         = set_hmirror;
    sensor->set_vflip           = set_vflip;
	sensor->set_special_effect  = set_special_effect;

	//TODO: CHChen59
#if 0
    sensor->set_saturation      = set_saturation;
    sensor->set_gainceiling     = set_gainceiling;
    sensor->set_quality         = set_quality;
    sensor->set_colorbar        = set_colorbar;
    sensor->set_auto_gain       = set_auto_gain;
    sensor->get_gain_db         = get_gain_db;
    sensor->set_auto_exposure   = set_auto_exposure;
    sensor->get_exposure_us     = get_exposure_us;
    sensor->set_auto_whitebal   = set_auto_whitebal;
    sensor->get_rgb_gain_db     = get_rgb_gain_db;
    sensor->set_lens_correction = set_lens_correction;
#endif

    // Set sensor flags
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_VSYNC, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_HSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_PIXCK, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_FSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_JPEGE, 0);
    SENSOR_HW_FLAGS_SET(sensor, SWNSOR_HW_FLAGS_RGB565_REV, 1);

    return 0;
}
