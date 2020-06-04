/**************************************************************************//**
 * @file     NAU88L25.c
 * @version  V0.01
 * @brief    Driver for NAU88L25
 *
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "NuMicro.h"
#include "NAU88L25.h"

#define DEV_ADDR_ID 		(0x1A)         /* NAU88L25 Device ID */
#define NAU88L25_ROLE_MASTER               /* NAU88L25 is master, target chip is slave */

static S_NAU88L25_CONFIG s_sNAU99L25Config;

static uint8_t 
I2cWrite_MultiByteforNAU88L25(
	I2C_T *i2c, 
	uint8_t chipadd,
	uint16_t subaddr, 
	const uint8_t *p,
	uint32_t len
)
{
    /* Send START */
    I2C_START(i2c);
    I2C_WAIT_READY(i2c);

    /* Send device address */
    I2C_SET_DATA(i2c, chipadd);
    I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
    I2C_WAIT_READY(i2c);

    /* Send register number and MSB of data */
    I2C_SET_DATA(i2c, (uint8_t)(subaddr>>8));
    I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
    I2C_WAIT_READY(i2c);

    /* Send register number and MSB of data */
    I2C_SET_DATA(i2c, (uint8_t)(subaddr));
    I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
    I2C_WAIT_READY(i2c);

    /* Send data */
    I2C_SET_DATA(i2c, p[0]);
    I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
    I2C_WAIT_READY(i2c);

    /* Send data */
    I2C_SET_DATA(i2c, p[1]);
    I2C_SET_CONTROL_REG(i2c, I2C_CTL_SI);
    I2C_WAIT_READY(i2c);

    /* Send STOP */
    I2C_STOP(i2c);

    return  0;
}

static uint8_t 
I2C_WriteNAU88L25(
	I2C_T *i2c,
	uint16_t addr,
	uint16_t dat
)
{
    uint8_t Tx_Data0[2];

    Tx_Data0[0] = (uint8_t)(dat >> 8);
    Tx_Data0[1] = (uint8_t)(dat & 0x00FF);

    return ( I2cWrite_MultiByteforNAU88L25(i2c, DEV_ADDR_ID << 1,addr,&Tx_Data0[0],2) );
}

void NAU88L25_Reset(
	I2C_T *i2c
)
{
    I2C_WriteNAU88L25(i2c, 0,  0x1);
    I2C_WriteNAU88L25(i2c, 0,  0);   // Reset all registers
    mp_hal_delay_ms(30);

    printf("NAU88L25 Software Reset.\n");
}

#if 1
int32_t NAU88L25_DSPConfig(
	I2C_T *i2c,
	uint32_t u32SamplRate,
	uint8_t u8Channel,
	uint8_t u8DataWidth
)
{
    int  clkDivider;
    int  i2sPcmCtrl2;
    int  lrClkDiv;
    char bClkDiv;
    char mClkDiv;

    if (u8Channel > 1) {
        /* FIXME */
        u8Channel = 2;
    } else {
        u8Channel = 1;
    }

    I2C_WriteNAU88L25(i2c, REG_I2S_PCM_CTRL1, AIFMT0_STANDI2S | ((u8DataWidth<=24) ? ((u8DataWidth-16)>>2) : WLEN0_32BIT) );
    u8DataWidth = (u8DataWidth > 16) ? 32 : 16;

    if (u32SamplRate % 11025) {
        /* 48000 series 12.288Mhz */
        I2C_WriteNAU88L25(i2c, REG_FLL2, 0x3126 );
        I2C_WriteNAU88L25(i2c, REG_FLL3, 0x0008 );

        mClkDiv = 49152000 / (u32SamplRate * 256);
    } else {
        /* 44100 series 11.2896Mhz */
        I2C_WriteNAU88L25(i2c, REG_FLL2, 0x86C2 );
        I2C_WriteNAU88L25(i2c, REG_FLL3, 0x0007 );

        /* FIXME */
        if (u32SamplRate > 44100)
            u32SamplRate = 11025;

        mClkDiv = 45158400 / (u32SamplRate * 256);
    }

    lrClkDiv = u8Channel * u8DataWidth;
    bClkDiv = 256/lrClkDiv;

    switch (mClkDiv) {
    case 1:
        mClkDiv = 0;
        break;
    case 2:
        mClkDiv = 2;
        break;
    case 4:
        mClkDiv = 3;
        break;
    case 8:
        mClkDiv = 4;
        break;
    case 16:
        mClkDiv = 5;
        break;
    case 32:
        mClkDiv = 6;
        break;
    case 3:
        mClkDiv = 7;
        break;
    case 6:
        mClkDiv = 10;
        break;
    case 12:
        mClkDiv = 11;
        break;
    case 24:
        mClkDiv = 12;
        break;
    case 48:
        mClkDiv = 13;
        break;
    case 96:
        mClkDiv = 14;
        break;
    case 5:
        mClkDiv = 15;
        break;
    default:
        printf("mclk divider not match!\n");
        mClkDiv = 0;
        return -1;
    }

    clkDivider = CLK_SYSCLK_SRC_VCO | CLK_ADC_SRC_DIV2 | CLK_DAC_SRC_DIV2 | mClkDiv;
    I2C_WriteNAU88L25(i2c, REG_CLK_DIVIDER, clkDivider );

    switch (bClkDiv) {
    case 2:
        bClkDiv = 0;
        break;
    case 4:
        bClkDiv = 1;
        break;
    case 8:
        bClkDiv = 2;
        break;
    case 16:
        bClkDiv = 3;
        break;
    case 32:
        bClkDiv = 4;
        break;
    case 64:
        bClkDiv = 5;
        break;
    default:
        printf("bclk divider not match!\n");
        bClkDiv = 0;
        return -2;
    }

    switch (lrClkDiv) {
    case 256:
        lrClkDiv = 0;
        break;
    case 128:
        lrClkDiv = 1;
        break;
    case 64:
        lrClkDiv = 2;
        break;
    case 32:
        lrClkDiv = 3;
        break;
    default:
        printf("lrclk divider not match!\n");
        lrClkDiv = 0;
        return -3;
    }

    i2sPcmCtrl2 =  ADCDAT0_OE | MS0_MASTER | (lrClkDiv << 12) | bClkDiv;

    I2C_WriteNAU88L25(i2c, REG_I2S_PCM_CTRL2, i2sPcmCtrl2 );

	return 0;
}

#else
int32_t NAU88L25_DSPConfig(
	I2C_T *i2c,
	uint32_t u32SamplRate,
	uint8_t u8Channel,
	uint8_t u8DataWidth
)
{
    printf("[NAU88L25] Configure Sampling Rate to %d\n", u32SamplRate);
    if((u32SamplRate % 8) == 0)
    {
        I2C_WriteNAU88L25(i2c,0x0005, 0x3126); //12.288Mhz
        I2C_WriteNAU88L25(i2c,0x0006, 0x0008);
    }
    else
    {
        I2C_WriteNAU88L25(i2c,0x0005, 0x86C2); //11.2896Mhz
        I2C_WriteNAU88L25(i2c,0x0006, 0x0007);
    }

    switch (u32SamplRate)
    {
    case 16000:
        I2C_WriteNAU88L25(i2c,0x0003,  0x801B); //MCLK = SYSCLK_SRC/12
        I2C_WriteNAU88L25(i2c,0x0004,  0x0001);
        I2C_WriteNAU88L25(i2c,0x0005,  0x3126); //MCLK = 4.096MHz
        I2C_WriteNAU88L25(i2c,0x0006,  0x0008);
        I2C_WriteNAU88L25(i2c,0x001D,  0x301A); //301A:Master, BCLK_DIV=MCLK/8=512K, LRC_DIV=512K/32=16K
        I2C_WriteNAU88L25(i2c,0x002B,  0x0002);
        I2C_WriteNAU88L25(i2c,0x002C,  0x0082);
        break;

    case 44100:
        I2C_WriteNAU88L25(i2c,0x001D,  0x301A); //301A:Master, BCLK_DIV=11.2896M/8=1.4112M, LRC_DIV=1.4112M/32=44.1K
        I2C_WriteNAU88L25(i2c,0x002B,  0x0012);
        I2C_WriteNAU88L25(i2c,0x002C,  0x0082);
        break;

    case 48000:
        I2C_WriteNAU88L25(i2c,0x001D,  0x301A); //301A:Master, BCLK_DIV=12.288M/8=1.536M, LRC_DIV=1.536M/32=48K
        I2C_WriteNAU88L25(i2c,0x002B,  0x0012);
        I2C_WriteNAU88L25(i2c,0x002C,  0x0082);
        break;

    case 96000:
        I2C_WriteNAU88L25(i2c,0x0003,  0x80A2); //MCLK = SYSCLK_SRC/2
        I2C_WriteNAU88L25(i2c,0x0004,  0x1801);
        I2C_WriteNAU88L25(i2c,0x0005,  0x3126); //MCLK = 24.576MHz
        I2C_WriteNAU88L25(i2c,0x0006,  0xF008);
        I2C_WriteNAU88L25(i2c,0x001D,  0x301A); //3019:Master, BCLK_DIV=MCLK/8=3.072M, LRC_DIV=3.072M/32=96K
        I2C_WriteNAU88L25(i2c,0x002B,  0x0001);
        I2C_WriteNAU88L25(i2c,0x002C,  0x0080);
        break;
    }
}
#endif



#if 1
int32_t NAU88L25_Init(
	I2C_T *i2c,
	S_NAU88L25_CONFIG *psConfig
)
{
	memcpy(&s_sNAU99L25Config, psConfig, sizeof(S_NAU88L25_CONFIG));

    I2C_WriteNAU88L25(i2c, REG_CLK_DIVIDER,  CLK_SYSCLK_SRC_VCO | CLK_ADC_SRC_DIV2 | CLK_DAC_SRC_DIV2 | MCLK_SRC_DIV4);
    I2C_WriteNAU88L25(i2c, REG_FLL1,  FLL_RATIO_512K);
    I2C_WriteNAU88L25(i2c, REG_FLL2,  0x3126);
    I2C_WriteNAU88L25(i2c, REG_FLL3,  0x0008);
    I2C_WriteNAU88L25(i2c, REG_FLL4,  0x0010);
    I2C_WriteNAU88L25(i2c, REG_FLL5,  PDB_DACICTRL | CHB_FILTER_EN);
    I2C_WriteNAU88L25(i2c, REG_FLL6,  SDM_EN | CUTOFF500);
    I2C_WriteNAU88L25(i2c, REG_FLL_VCO_RSV,  0xF13C);
    I2C_WriteNAU88L25(i2c, REG_HSD_CTRL,  HSD_AUTO_MODE | MANU_ENGND1_GND);
    I2C_WriteNAU88L25(i2c, REG_SAR_CTRL,  RES_SEL_70K_OHMS | COMP_SPEED_1US | SAMPLE_SPEED_4US);
    I2C_WriteNAU88L25(i2c, REG_I2S_PCM_CTRL1,  AIFMT0_STANDI2S);

#if defined(NAU88L25_ROLE_MASTER)
	I2C_WriteNAU88L25(i2c, REG_I2S_PCM_CTRL2,  LRC_DIV_DIV32 | ADCDAT0_OE | MS0_MASTER | BLCKDIV_DIV8);   //301A:Master 3012:Slave
#else
	I2C_WriteNAU88L25(i2c, REG_I2S_PCM_CTRL2,  LRC_DIV_DIV32 | ADCDAT0_OE | MS0_SLAVE | BLCKDIV_DIV8);
	I2C_WriteNAU88L25(i2c, REG_LEFT_TIME_SLOT,  DIS_FS_SHORT_DET);
#endif

    I2C_WriteNAU88L25(i2c, REG_ADC_RATE,  0x10 | ADC_RATE_128);
    I2C_WriteNAU88L25(i2c, REG_DAC_CTRL1,  0x80 | DAC_RATE_128);

    I2C_WriteNAU88L25(i2c, REG_MUTE_CTRL,  0x0000); // 0x10000
    I2C_WriteNAU88L25(i2c, REG_ADC_DGAIN_CTRL,  DGAINL_ADC0(0xEF));
    I2C_WriteNAU88L25(i2c, REG_DACL_CTRL,  DGAINL_DAC(0xAE));
    I2C_WriteNAU88L25(i2c, REG_DACR_CTRL,  DGAINR_DAC(0xAE) | DAC_CH_SEL1_RIGHT);

    I2C_WriteNAU88L25(i2c, REG_CLASSG_CTRL,  CLASSG_TIMER_64MS | CLASSG_CMP_EN_R_DAC | CLASSG_CMP_EN_L_DAC | CLASSG_EN);
    I2C_WriteNAU88L25(i2c, REG_BIAS_ADJ,  VMIDEN | VMIDSEL_125K_OHM);
    I2C_WriteNAU88L25(i2c, REG_TRIM_SETTINGS,  DRV_IBCTRHS | DRV_ICUTHS | INTEG_IBCTRHS | INTEG_ICUTHS);
    I2C_WriteNAU88L25(i2c, REG_ANALOG_CONTROL_2,  AB_ADJ | CAP_1 | CAP_0);
    I2C_WriteNAU88L25(i2c, REG_ANALOG_ADC_1,  CHOPRESETN | CHOPF0_DIV4);
    I2C_WriteNAU88L25(i2c, REG_ANALOG_ADC_2,  VREFSEL_VMIDE_P5DB | PDNOTL | LFSRRESETN);
    I2C_WriteNAU88L25(i2c, REG_RDAC,  DAC_EN_L | DAC_EN_R | CLK_DAC_EN_L | CLK_DAC_EN_R | CLK_DAC_DELAY_2NSEC | DACVREFSEL(0x3));
    I2C_WriteNAU88L25(i2c, REG_MIC_BIAS,  INT2KB | LOWNOISE | POWERUP | MICBIASLVL1_1P1x);
    I2C_WriteNAU88L25(i2c, REG_BOOST,  PDVMDFST | BIASEN | BOOSTGDIS | EN_SHRT_SHTDWN);
    I2C_WriteNAU88L25(i2c, REG_POWER_UP_CONTROL,  PUFEPGA | FEPGA_GAIN(21) | PUP_INTEG_LEFT_HP | PUP_INTEG_RIGHT_HP | PUP_DRV_INSTG_RIGHT_HP | PUP_DRV_INSTG_LEFT_HP | PUP_MAIN_DRV_RIGHT_HP | PUP_MAIN_DRV_LEFT_HP);
    I2C_WriteNAU88L25(i2c, REG_CHARGE_PUMP_AND_DOWN_CONTROL, JAMNODCLOW | RNIN);
    I2C_WriteNAU88L25(i2c, REG_ENA_CTRL,  RDAC_EN | LDAC_EN | ADC_EN | DCLK_ADC_EN | DCLK_DAC_EN| CLK_I2S_EN | 0x4);

	printf("Initialized done.\n");

	return 0;
}
#else
int32_t NAU88L25_Init(
	I2C_T *i2c,
	S_NAU88L25_CONFIG *psConfig
)
{
 	memcpy(&s_sNAU99L25Config, psConfig, sizeof(S_NAU88L25_CONFIG));

    I2C_WriteNAU88L25(i2c, 0x0003,  0x8053);
    I2C_WriteNAU88L25(i2c,0x0004,  0x0001);
    I2C_WriteNAU88L25(i2c,0x0005,  0x3126);
    I2C_WriteNAU88L25(i2c,0x0006,  0x0008);
    I2C_WriteNAU88L25(i2c,0x0007,  0x0010);
    I2C_WriteNAU88L25(i2c,0x0008,  0xC000);
    I2C_WriteNAU88L25(i2c,0x0009,  0x6000);
    I2C_WriteNAU88L25(i2c,0x000A,  0xF13C);
    I2C_WriteNAU88L25(i2c,0x000C,  0x0048);
    I2C_WriteNAU88L25(i2c,0x000D,  0x0000);
    I2C_WriteNAU88L25(i2c,0x000F,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0010,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0011,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0012,  0xFFFF);
    I2C_WriteNAU88L25(i2c,0x0013,  0x0015);
    I2C_WriteNAU88L25(i2c,0x0014,  0x0110);
    I2C_WriteNAU88L25(i2c,0x0015,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0016,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0017,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0018,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0019,  0x0000);
    I2C_WriteNAU88L25(i2c,0x001A,  0x0000);
    I2C_WriteNAU88L25(i2c,0x001B,  0x0000);
    I2C_WriteNAU88L25(i2c,0x001C,  0x0002);
    I2C_WriteNAU88L25(i2c,0x001D,  0x301a);   //301A:Master, BCLK_DIV=12.288M/8=1.536M, LRC_DIV=1.536M/32=48K
    I2C_WriteNAU88L25(i2c,0x001E,  0x0000);
    I2C_WriteNAU88L25(i2c,0x001F,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0020,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0021,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0022,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0023,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0024,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0025,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0026,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0027,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0028,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0029,  0x0000);
    I2C_WriteNAU88L25(i2c,0x002A,  0x0000);
    I2C_WriteNAU88L25(i2c,0x002B,  0x0012);
    I2C_WriteNAU88L25(i2c,0x002C,  0x0082);
    I2C_WriteNAU88L25(i2c,0x002D,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0030,  0x00CF);
    I2C_WriteNAU88L25(i2c,0x0031,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0032,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0033,  0x009E);
    I2C_WriteNAU88L25(i2c,0x0034,  0x029E);
    I2C_WriteNAU88L25(i2c,0x0038,  0x1486);
    I2C_WriteNAU88L25(i2c,0x0039,  0x0F12);
    I2C_WriteNAU88L25(i2c,0x003A,  0x25FF);
    I2C_WriteNAU88L25(i2c,0x003B,  0x3457);
    I2C_WriteNAU88L25(i2c,0x0045,  0x1486);
    I2C_WriteNAU88L25(i2c,0x0046,  0x0F12);
    I2C_WriteNAU88L25(i2c,0x0047,  0x25F9);
    I2C_WriteNAU88L25(i2c,0x0048,  0x3457);
    I2C_WriteNAU88L25(i2c,0x004C,  0x0000);
    I2C_WriteNAU88L25(i2c,0x004D,  0x0000);
    I2C_WriteNAU88L25(i2c,0x004E,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0050,  0x2007);
    I2C_WriteNAU88L25(i2c,0x0051,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0053,  0xC201);
    I2C_WriteNAU88L25(i2c,0x0054,  0x0C95);
    I2C_WriteNAU88L25(i2c,0x0055,  0x0000);
    I2C_WriteNAU88L25(i2c,0x0058,  0x1A14);
    I2C_WriteNAU88L25(i2c,0x0059,  0x00FF);
    I2C_WriteNAU88L25(i2c,0x0066,  0x0060);
    I2C_WriteNAU88L25(i2c,0x0068,  0xC300);
    I2C_WriteNAU88L25(i2c,0x0069,  0x0000);
    I2C_WriteNAU88L25(i2c,0x006A,  0x0083);
    I2C_WriteNAU88L25(i2c,0x0071,  0x0011);
    I2C_WriteNAU88L25(i2c,0x0072,  0x0260);
    I2C_WriteNAU88L25(i2c,0x0073,  0x332C);
    I2C_WriteNAU88L25(i2c,0x0074,  0x4502);
    I2C_WriteNAU88L25(i2c,0x0076,  0x3140);
    I2C_WriteNAU88L25(i2c,0x0077,  0x0000);
    I2C_WriteNAU88L25(i2c,0x007F,  0x553F);
    I2C_WriteNAU88L25(i2c,0x0080,  0x0420);
    I2C_WriteNAU88L25(i2c,0x0001,  0x07D4);

    printf("NAU88L25 Configured done.\n");
}

#endif

int32_t NAU88L25_MixerCtrl(
	I2C_T *i2c,
	uint32_t u32Cmd,
	uint32_t u32Value
)
{
    switch (u32Cmd)
	{
    case NAUL8825_MIXER_MUTE:
        if (u32Value)
        {
            I2C_WriteNAU88L25(i2c, REG_MUTE_CTRL,  SMUTE_EN);
			mp_hal_pin_high(s_sNAU99L25Config.pin_phonejack_en);
        }
        else
        {
            I2C_WriteNAU88L25(i2c, REG_MUTE_CTRL,  0x0000);
			mp_hal_pin_low(s_sNAU99L25Config.pin_phonejack_en);
        }
        break;
    case NAUL8825_MIXER_VOLUME:
        //I2C_WriteNAU88L25(REG_DACL_CTRL,  DGAINL_DAC(ui32Value*2));
        //I2C_WriteNAU88L25(REG_DACR_CTRL,  DGAINR_DAC(ui32Value*2) | DAC_CH_SEL1_RIGHT);
        break;
    default:
        return -1;
	}
    return 0;
}


