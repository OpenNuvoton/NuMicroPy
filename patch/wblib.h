/**************************************************************************//**
 * @file     wblib.h
 * @version  V3.00
 * @brief    N9H26 series SYS driver header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     WBLIB.h
 *
 * VERSION
 *     1.1
 *
 * DESCRIPTION
 *     The header file of N9H26K system library.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 * HISTORY
 *     2008-06-26  Ver 1.0 draft by Min-Nan Cheng
 *     2009-02-26  Ver 1.1 Changed for N9H26K MCU
 *
 * REMARK
 *     None
 **************************************************************************/
#ifndef _WBLIB_H
#define _WBLIB_H

#ifdef __cplusplus
extern "C"{
#endif

#include "N9H26_reg.h"
#include "wberrcode.h"
#include "wbio.h"

//#define PD_RAM_BASE	0xFF000000
//#define PD_RAM_START	0xFF001000
//#define PD_RAM_SIZE	0x2000	/* 8KB Tmp Buffer */
#define PD_RAM_BASE		0xFF000000
#define PD_RAM_START	0xFF000800
#define PD_RAM_SIZE		0x2000	/* 8KB Tmp Buffer */

/* Error code return value */
#define WB_INVALID_PARITY       	(SYSLIB_ERR_ID +1) 
#define WB_INVALID_DATA_BITS    	(SYSLIB_ERR_ID +2)
#define WB_INVALID_STOP_BITS    	(SYSLIB_ERR_ID +3)
#define WB_INVALID_BAUD         	(SYSLIB_ERR_ID +4)
#define WB_PM_PD_IRQ_Fail			(SYSLIB_ERR_ID +5)
#define WB_PM_Type_Fail			    (SYSLIB_ERR_ID +6)
#define WB_PM_INVALID_IRQ_NUM		(SYSLIB_ERR_ID +7)
#define WB_MEM_INVALID_MEM_SIZE		(SYSLIB_ERR_ID +8)
#define WB_INVALID_TIME				(SYSLIB_ERR_ID +9)
#define WB_INVALID_CLOCK			(SYSLIB_ERR_ID +10)
#define E_ERR_CLK                    WB_INVALID_CLOCK 
//-- function return value
#define	   Successful  0
#define	   Fail        1

#define EXTERNAL_CRYSTAL_CLOCK  12000000
//#define EXTERNAL_CRYSTAL_CLOCK  27000000

/* Define the vector numbers associated with each interrupt */
typedef enum int_source_e
{
	IRQ_WDT = 1, 
	IRQ_EXTINT0 = 2, 
	IRQ_EXTINT1 = 3, 
	IRQ_EXTINT2 = 4, 
   	IRQ_EXTINT3 = 5, 
	
    IRQ_IPSEC = 6, 
   	IRQ_SPU = 7, 
    IRQ_I2S = 8, 
    IRQ_VPOST = 9, 
    IRQ_VIN = 10, 
	
    IRQ_MDCT = 11, 
    IRQ_BLT = 12, 
    IRQ_VPE = 13, 
    IRQ_HUART = 14, 
    IRQ_TMR0 = 15, 
	
    IRQ_TMR1 = 16, 
    IRQ_UDC = 17, 
    IRQ_SIC = 18, 
    IRQ_SDIO = 19,
    IRQ_UHC = 20, 
	
    IRQ_EHCI = 21, 
    IRQ_OHCI = 22, 
    IRQ_EDMA0 = 23, 
    IRQ_EDMA1 = 24, 
    IRQ_SPIMS0 = 25, 
	
    IRQ_SPIMS1 = 26, 
    IRQ_AUDIO = 27, 
    IRQ_TOUCH = 28, 
    IRQ_RTC = 29,	
    IRQ_UART = 30, 
	
    IRQ_PWM = 31, 
    IRQ_JPG = 32,  
    IRQ_VDE = 33, 
    IRQ_VEN = 34, 
    IRQ_SDIC = 35, 
	
    IRQ_EMCTX = 36,
    IRQ_EMCRX = 37,
    IRQ_I2C = 38,
    IRQ_KPI = 39,
    IRQ_RSC = 40,
	
    IRQ_VTB = 41,
    IRQ_ROT = 42,
    IRQ_PWR = 43,                	
    IRQ_LVD = 44, 
    IRQ_VIN1 = 45,
	
    IRQ_TMR2 = 46,
    IRQ_TMR3 = 47
} INT_SOURCE_E;


typedef enum
{
	WE_EMAC = 0x1,
	WE_UHC20 = 02,
	WE_GPIO = 0x100,
	WE_RTC = 0x0200,
	//WE_SDH = 0x0400,
	WE_UART = 0x0800,
	WE_UDC = 0x1000,
	WE_UHC = 0x2000,
	WE_ADC = 0x4000,
	WE_KPI = 0x8000
}WAKEUP_SOURCE_E;

typedef struct datetime_t
{
	UINT32	year;
	UINT32	mon;
	UINT32	day;
	UINT32	hour;
	UINT32	min;
	UINT32	sec;
} DateTime_T;

/* Define constants for use timer in service parameters.  */
#define TIMER0            0
#define TIMER1            1
#define TIMER2            2
#define TIMER3            3
#define WDTIMER           4

#define ONE_SHOT_MODE     0
#define PERIODIC_MODE     1
#define TOGGLE_MODE       2
#define UNINTERRUPT_MODE  3

#define WDT_INTERVAL_0 	   0
#define WDT_INTERVAL_1     1
#define WDT_INTERVAL_2     2
#define WDT_INTERVAL_3     3

/* Define constants for use UART in service parameters.  */
#define WB_UART_0		0
#define WB_UART_1		1

#define WB_DATA_BITS_5    0x00
#define WB_DATA_BITS_6    0x01
#define WB_DATA_BITS_7    0x02
#define WB_DATA_BITS_8    0x03

#define WB_STOP_BITS_1    0x00
#define WB_STOP_BITS_2    0x04


#define WB_PARITY_NONE   0x00
#define WB_PARITY_ODD     0x08
#define WB_PARITY_EVEN    0x18

#define LEVEL_1_BYTE      0x0
#define LEVEL_4_BYTES     0x1
#define LEVEL_8_BYTES     0x2
#define LEVEL_14_BYTES    0x3
#define LEVEL_30_BYTES    0x4	/* High speed UART only */
#define LEVEL_46_BYTES    0x5
#define LEVEL_62_BYTES    0x6 

/* Define constants for use AIC in service parameters.  */
#define WB_SWI            0
#define WB_D_ABORT        1
#define WB_I_ABORT        2
#define WB_UNDEFINE       3

/* The parameters for sysSetInterruptPriorityLevel() and 
   sysInstallISR() use */
#define FIQ_LEVEL_0        0
#define IRQ_LEVEL_1        1
#define IRQ_LEVEL_2        2
#define IRQ_LEVEL_3        3
#define IRQ_LEVEL_4        4
#define IRQ_LEVEL_5        5
#define IRQ_LEVEL_6        6
#define IRQ_LEVEL_7        7

/* The parameters for sysSetGlobalInterrupt() use */
#define ENABLE_ALL_INTERRUPTS      0
#define DISABLE_ALL_INTERRUPTS     1

/* The parameters for sysSetInterruptType() use */
#define LOW_LEVEL_SENSITIVE        0x00
#define HIGH_LEVEL_SENSITIVE       0x01
#define NEGATIVE_EDGE_TRIGGER      0x02
#define POSITIVE_EDGE_TRIGGER      0x03

/* The parameters for sysSetLocalInterrupt() use */
#define ENABLE_IRQ                 0x7F
#define ENABLE_FIQ                 0xBF
#define ENABLE_FIQ_IRQ             0x3F
#define DISABLE_IRQ                0x80
#define DISABLE_FIQ                0x40
#define DISABLE_FIQ_IRQ            0xC0

/* Define Cache type  */
#define CACHE_WRITE_BACK		0
#define CACHE_WRITE_THROUGH		1
#define CACHE_DISABLE			-1

#define MMU_DIRECT_MAPPING	0
#define MMU_INVERSE_MAPPING	1




/* Define system clock come from */
typedef enum 
{
	eSYS_EXT 	= 0,
	eSYS_MPLL 	= 1,	/* Generally, for memory clock */
	eSYS_APLL  	= 2, /* Generally, for audio clock */
	eSYS_UPLL  	= 3	/* Generally, for system/engine clock */
}E_SYS_SRC_CLK;

/* Define constants for use Cache in service parameters.  */
#define CACHE_4M		2
#define CACHE_8M		3
#define CACHE_16M		4 
#define CACHE_32M		5
#define I_CACHE		    6
#define D_CACHE		    7
#define I_D_CACHE		8


/* Define UART initialization data structure */
typedef struct UART_INIT_STRUCT
{
	    UINT32		uart_no;
    	UINT32		uiFreq;
    	UINT32		uiBaudrate;
    	UINT8		uiDataBits;
    	UINT8		uiStopBits;
    	UINT8		uiParity;
    	UINT8		uiRxTriggerLevel;
}WB_UART_T;


/* Define the constant values of PM */
#define WB_PM_IDLE		    1
#define WB_PM_PD		    2
#define WB_PM_MIDLE	        5


#ifdef __FreeRTOS__
#include <FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wbtypes.h"
#include "wbio.h"
#endif

#if 0 //def __FreeRTOS__
extern SemaphoreHandle_t 			_uart_mutex;
extern int 							_uart_refcnt;
#define UART_MUTEX_INIT()		_uart_mutex=xSemaphoreCreateRecursiveMutex()
#define UART_MUTEX_FINI()		vSemaphoreDelete(_uart_mutex)
#define UART_MUTEX_LOCK()       xSemaphoreTakeRecursive(_uart_mutex, portMAX_DELAY)
#define UART_MUTEX_UNLOCK()     xSemaphoreGiveRecursive(_uart_mutex)
//#define UART_MUTEX_LOCK()   { xSemaphoreTakeRecursive(_uart_mutex, portMAX_DELAY); _uart_refcnt++; sysprintf("[%s]%s lock - %d\n", pcTaskGetName(xTaskGetCurrentTaskHandle()), __func__, _uart_refcnt); }
//#define UART_MUTEX_UNLOCK() {  _uart_refcnt--; sysprintf("[%s]%s unlock -%d\n", pcTaskGetName(xTaskGetCurrentTaskHandle()) ,__func__, _uart_refcnt); xSemaphoreGiveRecursive(_uart_mutex); }
#else
#define UART_MUTEX_INIT()	
#define UART_MUTEX_FINI()	
#define UART_MUTEX_LOCK()   
#define UART_MUTEX_UNLOCK() 
#endif /* __FreeRTOS__ */




/* Define system library Timer functions */
UINT32 sysGetTicks (INT32 nTimeNo);
INT32  sysResetTicks (INT32 nTimeNo);
INT32  sysUpdateTickCount(INT32 nTimeNo, UINT32 uCount);
INT32  sysSetTimerReferenceClock (INT32 nTimeNo, UINT32 uClockRate);
INT32  sysStartTimer (INT32 nTimeNo, UINT32 uTicksPerSecond, INT32 nOpMode);
INT32  sysStopTimer (INT32 nTimeNo);
INT32  sysSetTimerEvent(INT32 nTimeNo, UINT32 uTimeTick, PVOID pvFun);
VOID   sysClearTimerEvent(INT32 nTimeNo, UINT32 uTimeEventNo);
VOID   sysSetLocalTime(DateTime_T ltime);
VOID   sysGetCurrentTime(DateTime_T *curTime);
VOID   sysDelay(UINT32 uTicks);
VOID   sysClearWatchDogTimerCount (VOID);
VOID   sysClearWatchDogTimerInterruptStatus(VOID);
VOID   sysDisableWatchDogTimer (VOID);
VOID   sysDisableWatchDogTimerReset(VOID);
VOID   sysEnableWatchDogTimer (VOID);
VOID   sysEnableWatchDogTimerReset(VOID);
PVOID sysInstallWatchDogTimerISR (INT32 nIntTypeLevel, PVOID pvNewISR);
INT32 sysSetWatchDogTimerInterval (INT32 nWdtInterval);


/* Define system library UART functions */
#define UART_INT_RDA		0
#define UART_INT_RDTO		1
#define UART_INT_NONE		255

typedef VOID (*PFN_SYS_UART_CALLBACK)(UINT8* u8Buf, UINT32 u32Len);
VOID 	sysUartPort(UINT32 u32Port);
INT8	sysGetChar (VOID);
INT32	sysInitializeUART (WB_UART_T *uart);
VOID	sysPrintf (PINT8 pcStr,...);
VOID	sysprintf (PINT8 pcStr,...);
VOID	sysPutChar (UINT8 ucCh);
VOID    sysUartInstallcallback(UINT32 u32IntType,  PFN_SYS_UART_CALLBACK pfnCallback);
VOID 	sysUartEnableInt(INT32 eIntType);
VOID 	sysUartTransfer(INT8* pu8buf, UINT32 u32Len);

/*--------------------------------------------------------------------------*/
/* Define Function Prototypes for HUART                                     */
/*--------------------------------------------------------------------------*/
INT32  sysInitializeHUART(WB_UART_T *uart);
VOID   sysHuartInstallcallback(UINT32 u32IntType, PFN_SYS_UART_CALLBACK pfnCallback);
VOID   sysHuartEnableInt(INT32 eIntType);
INT8   sysHuartReceive(VOID);
VOID   sysHuartTransfer(INT8* pu8buf, UINT32 u32Len);

/* Define system library AIC functions */
ERRCODE sysDisableInterrupt (INT_SOURCE_E eIntNo);
ERRCODE sysEnableInterrupt (INT_SOURCE_E eIntNo);
BOOL sysGetIBitState(VOID);
UINT32 sysGetInterruptEnableStatus(VOID);
UINT32 sysGetInterruptHighEnableStatus(VOID);
PVOID sysInstallExceptionHandler (INT32 nExceptType, 
								PVOID pvNewHandler);
PVOID sysInstallFiqHandler (PVOID pvNewISR);	/* Almost is not used, sysInstallISR() can cover the job */
PVOID sysInstallIrqHandler (PVOID pvNewISR);	/* Almost is not used, sysInstallISR() can cover the job */
PVOID sysInstallISR (INT32 nIntTypeLevel, INT_SOURCE_E eIntNo, PVOID pvNewISR);
ERRCODE sysSetGlobalInterrupt (INT32 nIntState);
ERRCODE sysSetInterruptPriorityLevel (INT_SOURCE_E eIntNo, UINT32 uIntLevel);
ERRCODE sysSetInterruptType (INT_SOURCE_E eIntNo, UINT32 uIntSourceType);
ERRCODE sysSetLocalInterrupt (INT32 nIntState);
ERRCODE sysSetAIC2SWMode(VOID);


/* Define system library Cache functions */
VOID	sysDisableCache(VOID);
INT32   sysEnableCache(UINT32 uCacheOpMode);
VOID	sysFlushCache(INT32 nCacheType);
BOOL    sysGetCacheState(VOID);
INT32   sysGetCacheMode(VOID);
INT32   sysGetSdramSizebyMB(VOID);
VOID	sysInvalidCache(VOID);
INT32   sysSetCachePages(UINT32 addr, INT32 size, INT32 cache_mode);



/* Define system power management functions */
INT32   sysPowerDown(UINT32 u32WakeUpSrc);

/* Define system clock functions */
UINT32  sysGetExternalClock(VOID);
UINT32  sysSetPllClock(E_SYS_SRC_CLK eSrcClk, UINT32 u32TargetHz);
VOID 	sysCheckPllConstraint(BOOL bIsCheck);
ERRCODE sysSetSystemClock(E_SYS_SRC_CLK eSrcClk, UINT32 u32PllHz, UINT32 u32SysHz);	/* Unit HZ */
INT32   sysSetCPUClock(UINT32 u32CPUClock);/* Unit HZ */
INT32   sysSetAPBClock(UINT32 u32APBClock);/* Unit HZ */
UINT32  sysGetPLLOutputHz(E_SYS_SRC_CLK eSysPll, UINT32 u32FinHz);
UINT32  sysGetSystemClock(VOID);	/* Unit HZ */
UINT32  sysGetCPUClock(VOID);	/* Unit HZ */
UINT32  sysGetHCLK1Clock(VOID);	/* Unit HZ */
UINT32  sysGetAPBClock(VOID);	/* Unit HZ */


UINT32 sysSetDramClock(E_SYS_SRC_CLK eSrcClk, UINT32 u32PLLClockHz, UINT32 u32DramClock); /* Unit HZ */
UINT32 sysGetDramClock(VOID);
	
#ifdef __cplusplus
}
#endif

#endif  /* _WBLIB_H */

