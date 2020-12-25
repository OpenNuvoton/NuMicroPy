/**************************************************************************//**

 * @file     W55FA92_VideoIn.h

 * @version  V3.00

 * @brief    N3292x series VideoIn driver header file

 *

 * SPDX-License-Identifier: Apache-2.0

 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.

*****************************************************************************/



#ifndef __W55FA92_VIDEOIN_H__

#define __W55FA92_VIDEOIN_H__



// #include header file

#ifdef  __cplusplus

extern "C"

{

#endif



/* Define data type (struct, union¡K) */

// #define Constant

#include "wblib.h"



//Error message

// E_VIDEOIN_INVALID_INT					Invalid interrupt

// E_VIDEOIN_INVALID_BUF					Invalid buffer

// E_VIDEOIN_INVALID_PIPE					Invalid pipe

// E_VIDEOIN_INVALID_COLOR_MODE			Invalid color mode



#define E_VIDEOIN_INVALID_INT   				(VIN_ERR_ID | 0x01)

#define E_VIDEOIN_INVALID_BUF   				(VIN_ERR_ID | 0x02)

#define E_VIDEOIN_INVALID_PIPE  				(VIN_ERR_ID | 0x03)

#define E_VIDEOIN_INVALID_COLOR_MODE  		(VIN_ERR_ID | 0x04)

#define E_VIDEOIN_WRONG_COLOR_PARAMETER  	(VIN_ERR_ID | 0x05)

typedef void (*PFN_VIDEOIN_CALLBACK)(UINT8 u8PacketBufID,

								UINT8 u8PlanarBufID, 

								UINT8 u8FrameRate,

								UINT8 u8Filed);

/* Input device  */

typedef enum

{

	eVIDEOIN_SNR_CCIR656=0,		/* 1st port from GPB*/

	eVIDEOIN_SNR_CCIR601,

	eVIDEOIN_TVD_CCIR656,

	eVIDEOIN_TVD_CCIR601,

		

	eVIDEOIN_2ND_SNR_CCIR656,		/* 2nd port: Data form GPC/Control from GPA */	

	eVIDEOIN_2ND_SNR_CCIR601,		/* 2nd port: Data form GPC/Control from GPA and GPE*/		

	eVIDEOIN_2ND_TVD_CCIR656,		/* 2nd port: Data form GPC/Control from GPA */

	eVIDEOIN_2ND_TVD_CCIR601,		/* 2nd port: Data form GPC/Control from GPA and GPE*/	



		

	eVIDEOIN_3RD_SNR_CCIR656,		/* 3rd port: Data form GPG and GPD/Control from GPD */

	eVIDEOIN_3RD_SNR_CCIR601,		/* 3rd port: Data form GPG and GPD/Control from GPD */

	eVIDEOIN_3RD_TVD_CCIR656,		/* 3rd port: Data form GPG and GPD/Control from GPD */

	eVIDEOIN_3RD_TVD_CCIR601		/* 3rd port: Data form GPG and GPD/Control from GPD */

}E_VIDEOIN_DEV_TYPE;



/* Interrupt type */

typedef enum

{

	eVIDEOIN_MDINT = 0x100000,

	eVIDEOIN_ADDRMINT = 0x80000,

	eVIDEOIN_MEINT = 0x20000,

	eVIDEOIN_VINT = 0x10000	

}E_VIDEOIN_INT_TYPE;





/* Pipe enable */

typedef enum

{

	eVIDEOIN_BOTH_PIPE_DISABLE = 0,

	eVIDEOIN_PLANAR = 1,

	eVIDEOIN_PACKET = 2,

	eVIDEOIN_BOTH_PIPE_ENABLE = 3	

}E_VIDEOIN_PIPE;



/* Base address */

typedef enum

{

	eVIDEOIN_BUF0 =0,

	eVIDEOIN_BUF1,	

	eVIDEOIN_BUF2

}E_VIDEOIN_BUFFER;



/* For DrvVideoIn_SetOperationMode */

#define VIDEOIN_CONTINUE   1   



/* Input Data Order For YCbCr */

typedef enum

{

	eVIDEOIN_IN_UYVY =0,

	eVIDEOIN_IN_YUYV,

	eVIDEOIN_IN_VYUY,		

	eVIDEOIN_IN_YVYU

}E_VIDEOIN_ORDER;





typedef enum

{

	eVIDEOIN_IN_YUV422 = 0,

	eVIDEOIN_IN_RGB565

}E_VIDEOIN_IN_FORMAT;                                  

                                                                

typedef enum

{

	eVIDEOIN_OUT_YUV422 = 0,

	eVIDEOIN_OUT_ONLY_Y,

	eVIDEOIN_OUT_RGB555,		

	eVIDEOIN_OUT_RGB565

}E_VIDEOIN_OUT_FORMAT;	



typedef enum

{

	eVIDEOIN_PLANAR_YUV422 = 0,

	eVIDEOIN_PLANAR_YUV420,

	eVIDEOIN_MACRO_PLANAR_YUV420

}E_VIDEOIN_PLANAR_FORMAT;	





typedef enum

{

	eVIDEOIN_TYPE_CCIR601 = 0,

	eVIDEOIN_TYPE_CCIR656

}E_VIDEOIN_TYPE;     



typedef enum

{

	eVIDEOIN_SNR_APLL = 2,

	eVIDEOIN_SNR_UPLL = 3

}E_VIDEOIN_SNR_SRC;  



typedef enum

{

	eVIDEOIN_CEF_NORMAL = 0,

	eVIDEOIN_CEF_SEPIA = 1,

	eVIDEOIN_CEF_NEGATIVE = 2,

	eVIDEOIN_CEF_POSTERIZE = 3

}E_VIDEOIN_CEF;  







typedef struct

{

	void (*Init)(BOOL bIsEnableSnrClock, E_VIDEOIN_SNR_SRC eSnrSrc, UINT32 u32SensorFreqKHz, E_VIDEOIN_DEV_TYPE eDevType);

	INT32 (*Open)(UINT32 u32EngFreqKHz, UINT32 u32SensorFreqKHz);

	void (*Close)(void);

	void (*SetPipeEnable)(BOOL bEngEnable, E_VIDEOIN_PIPE ePipeEnable);

	void (*SetPlanarFormat)(E_VIDEOIN_PLANAR_FORMAT ePlanarFmt);

	void (*SetCropWinSize)(UINT32 u32height, UINT32 u32width);

	void (*SetCropWinStartAddr)(UINT32 u32VerticalStart, UINT32 u32HorizontalStart);

	INT32 (*PreviewPipeSize)(UINT16 u16height, UINT16 u16width);

	INT32 (*EncodePipeSize)(UINT16 u16height, UINT16 u16width);

	void (*SetStride)(UINT32 u16packetstride, UINT32 u32planarstride);  								

	void (*GetStride)(PUINT32 pu32PacketStride, PUINT32 pu32PlanarStride);

	INT32 (*EnableInt)(E_VIDEOIN_INT_TYPE eIntType);

	INT32 (*DisableInt)(E_VIDEOIN_INT_TYPE eIntType);

	INT32 (*InstallCallback)(E_VIDEOIN_INT_TYPE eIntType, PFN_VIDEOIN_CALLBACK pfnCallback, PFN_VIDEOIN_CALLBACK *pfnOldCallback);

	INT32 (*SetBaseStartAddress)(E_VIDEOIN_PIPE ePipe, E_VIDEOIN_BUFFER eBuf, UINT32 u32BaseStartAddr);					

	void (*SetOperationMode)(BOOL bIsOneSutterMode);

	BOOL (*GetOperationMode)(void);

	void (*SetPacketFrameBufferControl)(BOOL bFrameSwitch, BOOL bFrameBufferSel);

	void (*SetSensorPolarity)(BOOL bVsync, BOOL bHsync, BOOL bPixelClk);	

	

	INT32 (*SetColorEffectParameter)(UINT8 u8YComp, UINT8 u8UComp, UINT8 u8VComp);

	void (*SetDataFormatAndOrder)(E_VIDEOIN_ORDER eInputOrder, E_VIDEOIN_IN_FORMAT eInputFormat, E_VIDEOIN_OUT_FORMAT eOutputFormat);

	void (*SetMotionDet)(BOOL bEnable, BOOL bBlockSize,	BOOL bSaveMode);

	void (*SetMotionDetEx)(UINT32 u32Threshold, UINT32 u32OutBuffer, UINT32 u32LumBuffer);					

	void (*SetInputType)(UINT32 u32FieldEnable, E_VIDEOIN_TYPE eInputType,	BOOL bFieldSwap);

	void (*SetStandardCCIR656)(BOOL);

	void (*SetShadowRegister)(void);

        void (*SetFieldDetection)(BOOL bDetPosition,BOOL bFieldDetMethod);

	void (*GetFieldDetection)(PBOOL pbDetPosition,PBOOL pbFieldDetMethod);

}VINDEV_T;



INT32 register_vin_device(UINT32 u32port, VINDEV_T* pVinDev );



#ifdef __cplusplus

}

#endif



#endif



