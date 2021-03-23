/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2019 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2019 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Sensor abstraction layer.
 */
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "py/mphal.h"

#include "framebuffer.h"

#include "omv_boardconfig.h"
#include "sensor.h"
#include "NT99141.h"

#include "hal/N9H26_VideoIn.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#if MICROPY_NVTMEDIA
#include "NVTMedia.h"
#endif

#if VIN_CONFIG_PORT0_PLANAR_MAX_FRAME_CNT == 1
    #define __VIN_ONE_SHOT_MODE__
#endif

#define VIN_TASK_PRIORITY        (configMAX_PRIORITIES - 1)

typedef struct
{
    struct S_VIN_CTRL *psVinCtrl;

    uint32_t u32PortNo;
    uint32_t u32PacketMaxFrameCnt;
    uint32_t u32PlanarMaxFrameCnt;
    BOOL    *pabIsPacketBufFull;
    BOOL    *pabIsPlanarBufFull;

    uint32_t u32PacketInIdx;
    uint32_t u32PacketProcIdx;
    uint32_t u32PacketEncIdx;

    uint32_t u32PlanarInIdx;
    uint32_t u32PlanarProcIdx;
    uint32_t u32PlanarEncIdx;

    uint8_t **apu8PlanarFrameBufPtr;
    uint8_t **apu8PacketFrameBufPtr;
//    S_VIN_PIPE_INFO *psPlanarPipeInfo;
//    S_VIN_PIPE_INFO *psPacketPipeInfo;
	framebuffer_t *psPlanarPipeFrame;
	framebuffer_t *psPacketPipeFrame;

    VINDEV_T sVinDev;

    SemaphoreHandle_t tVinISRSem;   //video-in interrupt semaphore
    xSemaphoreHandle tFrameIndexMutex;
} S_VIN_PORT_OP;

// static varaiable
static S_VIN_PORT_OP s_sVinPortOP;
static bool s_bSensorRun = false;

#if (SENSOR_PCLK == SENSOR_PCLK_48MHZ)
static uint32_t s_u32SnrClk = 16000;    //frame rate: 20
static uint32_t s_u32InputFrameRate = 20;
#endif

#if (SENSOR_PCLK == SENSOR_PCLK_60MHZ)
static uint32_t s_u32SnrClk = 20000;
static uint32_t s_u32InputFrameRate = 24;
#endif

#if (SENSOR_PCLK == SENSOR_PCLK_74MHZ)
static uint32_t s_u32SnrClk = 24000;
static uint32_t s_u32InputFrameRate = 30;
#endif

static BOOL s_abIsPacketBufFull[VIN_CONFIG_PORT0_PACKET_FRAME_CNT];
static BOOL s_abIsPlanarBufFull[VIN_CONFIG_PORT0_PLANAR_FRAME_CNT];

//#pragma arm section zidata = "non_initialized"

#if VIN_CONFIG_PORT0_PLANAR_FRAME_CNT >= 1
#if defined (__GNUC__) && !(__CC_ARM)
static __attribute__((section("no_init"), aligned(32))) uint8_t s_au8PlanarFrameBuffer0[(VIN_CONFIG_PORT0_PLANAR_WIDTH * VIN_CONFIG_PORT0_PLANAR_HEIGHT * 2) + 256];
#else
static __align(32) uint8_t s_au8PlanarFrameBuffer0[(VIN_CONFIG_PORT0_PLANAR_WIDTH * VIN_CONFIG_PORT0_PLANAR_HEIGHT * 2) + 256];
#endif
#endif

#if VIN_CONFIG_PORT0_PLANAR_FRAME_CNT >= 2
#if defined (__GNUC__) && !(__CC_ARM)
static __attribute__((section("no_init"), aligned(32))) uint8_t s_au8PlanarFrameBuffer1[(VIN_CONFIG_PORT0_PLANAR_WIDTH * VIN_CONFIG_PORT0_PLANAR_HEIGHT * 2) + 256];
#else
static __align(32) uint8_t s_au8PlanarFrameBuffer1[(VIN_CONFIG_PORT0_PLANAR_WIDTH * VIN_CONFIG_PORT0_PLANAR_HEIGHT * 2) + 256];
#endif
#endif

#if VIN_CONFIG_PORT0_PLANAR_FRAME_CNT >= 3
#if defined (__GNUC__) && !(__CC_ARM)
static __attribute__((section("no_init"), aligned(32))) uint8_t s_au8PlanarFrameBuffer2[(VIN_CONFIG_PORT0_PLANAR_WIDTH * VIN_CONFIG_PORT0_PLANAR_HEIGHT * 2) + 256];
#else
static __align(32) uint8_t s_au8PlanarFrameBuffer2[(VIN_CONFIG_PORT0_PLANAR_WIDTH * VIN_CONFIG_PORT0_PLANAR_HEIGHT * 2) + 256];
#endif
#endif

#if VIN_CONFIG_PORT0_PACKET_FRAME_CNT >= 1
    #if defined (__GNUC__) && !(__CC_ARM)
        static __attribute__((section("no_init"), aligned(32))) uint8_t s_au8PacketFrameBuffer0[(VIN_CONFIG_PORT0_PACKET_WIDTH * VIN_CONFIG_PORT0_PACKET_HEIGHT * 2) + 256];
    #else
        static __align(32) uint8_t s_au8PacketFrameBuffer0[(VIN_CONFIG_PORT0_PACKET_WIDTH * VIN_CONFIG_PORT0_PACKET_HEIGHT * 2) + 256];
    #endif
#endif

#if VIN_CONFIG_PORT0_PACKET_FRAME_CNT >= 2
    #if defined (__GNUC__) && !(__CC_ARM)
        static __attribute__((section("no_init"), aligned(32))) uint8_t s_au8PacketFrameBuffer1[(VIN_CONFIG_PORT0_PACKET_WIDTH * VIN_CONFIG_PORT0_PACKET_HEIGHT * 2) + 256];
    #else
        static __align(32) uint8_t s_au8PacketFrameBuffer1[(VIN_CONFIG_PORT0_PACKET_WIDTH * VIN_CONFIG_PORT0_PACKET_HEIGHT * 2) + 256];
    #endif
#endif

#if VIN_CONFIG_PORT0_PACKET_FRAME_CNT >= 3
    #if defined (__GNUC__) && !(__CC_ARM)
        static __attribute__((section("no_init"), aligned(32))) uint8_t s_au8PacketFrameBuffer2[(VIN_CONFIG_PORT0_PACKET_WIDTH * VIN_CONFIG_PORT0_PACKET_HEIGHT * 2) + 256];
    #else
        static __align(32) uint8_t s_au8PacketFrameBuffer2[(VIN_CONFIG_PORT0_PACKET_WIDTH * VIN_CONFIG_PORT0_PACKET_HEIGHT * 2) + 256];
    #endif
#endif

//#pragma arm section zidata // back to default (.bss section)

static uint8_t *s_apu8PlanarFrameBufPtr[VIN_CONFIG_PORT0_PLANAR_FRAME_CNT] =
{
#if VIN_CONFIG_PORT0_PLANAR_FRAME_CNT >= 1
    s_au8PlanarFrameBuffer0,
#endif
#if VIN_CONFIG_PORT0_PLANAR_FRAME_CNT >= 2
    s_au8PlanarFrameBuffer1,
#endif
#if VIN_CONFIG_PORT0_PLANAR_FRAME_CNT >= 3
    s_au8PlanarFrameBuffer2,
#endif

};

static uint8_t *s_apu8PacketFrameBufPtr[VIN_CONFIG_PORT0_PACKET_FRAME_CNT] =
{
#if VIN_CONFIG_PORT0_PACKET_FRAME_CNT >= 1
    s_au8PacketFrameBuffer0,
#endif
#if VIN_CONFIG_PORT0_PACKET_FRAME_CNT >= 2
    s_au8PacketFrameBuffer1,
#endif
#if VIN_CONFIG_PORT0_PACKET_FRAME_CNT >= 3
    s_au8PacketFrameBuffer2,
#endif
};

static uint32_t s_u32PlanarFrameBufSize = sizeof(s_au8PlanarFrameBuffer0);
static uint32_t s_u32PacketFrameBufSize = sizeof(s_au8PacketFrameBuffer0);


// global variable
sensor_t sensor = {0};

framebuffer_t g_sPlanarPipeFrame;
framebuffer_t g_sPacketPipeFrame;

const int resolution[][2] = {
    {0,    0   },
    // C/SIF Resolutions
    {88,   72  },    /* QQCIF     */
    {176,  144 },    /* QCIF      */
    {352,  288 },    /* CIF       */
    {88,   60  },    /* QQSIF     */
    {176,  120 },    /* QSIF      */
    {352,  240 },    /* SIF       */
    // VGA Resolutions
    {40,   30  },    /* QQQQVGA   */
    {80,   60  },    /* QQQVGA    */
    {160,  120 },    /* QQVGA     */
    {320,  240 },    /* QVGA      */
    {640,  480 },    /* VGA       */
    {60,   40  },    /* HQQQVGA   */
    {120,  80  },    /* HQQVGA    */
    {240,  160 },    /* HQVGA     */
    // FFT Resolutions
    {64,   32  },    /* 64x32     */
    {64,   64  },    /* 64x64     */
    {128,  64  },    /* 128x64    */
    {128,  128 },    /* 128x128   */
    // Other
    {128,  160 },    /* LCD       */
    {128,  160 },    /* QQVGA2    */
    {720,  480 },    /* WVGA      */
    {752,  480 },    /* WVGA2     */
    {800,  600 },    /* SVGA      */
    {1024, 768 },    /* XGA       */
    {1280, 1024},    /* SXGA      */
    {1600, 1200},    /* UXGA      */
    {1280, 720 },    /* HD        */
    {1920, 1080},    /* FHD       */
    {2560, 1440},    /* QHD       */
    {2048, 1536},    /* QXGA      */
    {2560, 1600},    /* WQXGA     */
    {2592, 1944},    /* WQXGA2    */
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Static functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void VideoInFrameBufSwitch(
    S_VIN_PORT_OP *psPortOP
)
{
    uint16_t u16PlanarWidth, u16PlanarHeight;
    framebuffer_t *psPlanarPipeInfo;

    switch (psPortOP->u32PacketInIdx)
    {
    case 0:
        psPortOP->pabIsPacketBufFull[psPortOP->u32PacketInIdx] = TRUE;
        if (psPortOP->u32PacketMaxFrameCnt > 1)
        {
            if (psPortOP->pabIsPacketBufFull[psPortOP->u32PacketInIdx + 1] == FALSE)        //If packet buffer 1 is empty change to it.
            {
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PACKET, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PacketFrameBufPtr[psPortOP->u32PacketInIdx + 1]));
                psPortOP->u32PacketInIdx = psPortOP->u32PacketInIdx + 1;
            }
        }
        break;
    case 1:
        psPortOP->pabIsPacketBufFull[psPortOP->u32PacketInIdx] = TRUE;
        if (psPortOP->u32PacketMaxFrameCnt > 2)
        {
            if (psPortOP->pabIsPacketBufFull[psPortOP->u32PacketInIdx + 1] == FALSE)        //If packet buffer 2 is empty change to it.
            {
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PACKET, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PacketFrameBufPtr[psPortOP->u32PacketInIdx + 1]));
                psPortOP->u32PacketInIdx = psPortOP->u32PacketInIdx + 1;
            }
        }
        else
        {
            if (psPortOP->pabIsPacketBufFull[0] == FALSE)       //If packet buffer 0 is empty change to it.
            {
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PACKET, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PacketFrameBufPtr[0]));
                psPortOP->u32PacketInIdx = 0;
            }
        }
        break;
    case 2:
        psPortOP->pabIsPacketBufFull[psPortOP->u32PacketInIdx] = TRUE;
        if (psPortOP->pabIsPacketBufFull[0] == FALSE)       //If packet buffer 0 is empty change to it.
        {
            psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PACKET, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PacketFrameBufPtr[0]));
            psPortOP->u32PacketInIdx = 0;
        }
        break;
    }

    psPlanarPipeInfo = psPortOP->psPlanarPipeFrame;
    u16PlanarWidth = psPlanarPipeInfo->u;
    u16PlanarHeight = psPlanarPipeInfo->v;

    switch (psPortOP->u32PlanarInIdx)
    {
    case 0:
        psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarInIdx] = TRUE;
        if (psPortOP->u32PlanarMaxFrameCnt > 1)
        {
            if (psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarInIdx + 1] == FALSE)        //If planar buffer 1 is empty change to it.
            {
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1]));
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)1, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + u16PlanarWidth * u16PlanarHeight));

                if (sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV420MB)
                    psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + u16PlanarWidth * u16PlanarHeight));
                else if ((sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV422))
                    psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + ((u16PlanarWidth * u16PlanarHeight) + ((u16PlanarWidth * u16PlanarHeight) / 2))));
                else if ((sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV420))
                    psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + ((u16PlanarWidth * u16PlanarHeight) + ((u16PlanarWidth * u16PlanarHeight) / 4))));

                psPortOP->u32PlanarInIdx = psPortOP->u32PlanarInIdx + 1;
            }
        }
        break;
    case 1:
        psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarInIdx] = TRUE;
        if (psPortOP->u32PlanarMaxFrameCnt > 2)
        {
            if (psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarInIdx + 1] == FALSE)        //If packet buffer 2 is empty change to it.
            {
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1]));
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)1, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + u16PlanarWidth * u16PlanarHeight));

                if (sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV420MB)
                    psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + u16PlanarWidth * u16PlanarHeight));
                else if ((sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV422))
                    psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + ((u16PlanarWidth * u16PlanarHeight) + ((u16PlanarWidth * u16PlanarHeight) / 2))));
                else if ((sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV420))
                    psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarInIdx + 1] + ((u16PlanarWidth * u16PlanarHeight) + ((u16PlanarWidth * u16PlanarHeight) / 4))));

                psPortOP->u32PlanarInIdx = psPortOP->u32PlanarInIdx + 1;
            }
        }
        else
        {
            if (psPortOP->pabIsPlanarBufFull[0] == FALSE)       //If packet buffer 0 is empty change to it.
            {
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0]));
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)1, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + u16PlanarWidth * u16PlanarHeight));

                if (sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV420MB)
                    psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + u16PlanarWidth * u16PlanarHeight));
                else if ((sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV422))
                    psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + ((u16PlanarWidth * u16PlanarHeight) + ((u16PlanarWidth * u16PlanarHeight) / 2))));
                else if ((sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV420))
                    psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + ((u16PlanarWidth * u16PlanarHeight) + ((u16PlanarWidth * u16PlanarHeight) / 4))));

                psPortOP->u32PlanarInIdx = 0;
            }
        }
        break;
    case 2:
        psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarInIdx] = TRUE;
        if (psPortOP->pabIsPlanarBufFull[0] == FALSE)       //If packet buffer 0 is empty change to it.
        {
            psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)0, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0]));
            psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)1, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + u16PlanarWidth * u16PlanarHeight));

            if (sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV420MB)
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + u16PlanarWidth * u16PlanarHeight));
            else if ((sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV422))
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + (u16PlanarWidth * u16PlanarHeight + u16PlanarWidth * u16PlanarHeight / 2)));
            else if ((sensor.pixformat_planar == PLANAR_PIXFORMAT_YUV420))
                psPortOP->sVinDev.SetBaseStartAddress(eVIDEOIN_PLANAR, (E_VIDEOIN_BUFFER)2, (UINT32)(psPortOP->apu8PlanarFrameBufPtr[0] + (u16PlanarWidth * u16PlanarHeight + u16PlanarWidth * u16PlanarHeight / 4)));

            psPortOP->u32PlanarInIdx = 0;
        }

        break;
    }
}


//Video-In task
static void VinFrameLoop(
    S_VIN_PORT_OP   *psPortOP
)
{
    int32_t i32MaxPlanarFrameCnt = psPortOP->u32PlanarMaxFrameCnt;
    int32_t i32MaxPacketFrameCnt = psPortOP->u32PacketMaxFrameCnt;
    int32_t i;

    uint64_t u64CalFPSStartTime = mp_hal_ticks_ms();
    uint64_t u64CurTime;
    uint64_t u64CalFPSTime;
    uint32_t u32Frames = 0;

    while (1)
    {
		//Wait Video-In ISR semaphore
        xSemaphoreTake(psPortOP->tVinISRSem, portMAX_DELAY);
		//Lock frame index mutex
        xSemaphoreTake(psPortOP->tFrameIndexMutex, portMAX_DELAY);

        for (i = 0; i < i32MaxPlanarFrameCnt; i ++)
        {
            if (((i + 1) % i32MaxPlanarFrameCnt)  == psPortOP->u32PlanarInIdx)
            {
                if (psPortOP->pabIsPlanarBufFull[i] == TRUE)
                {
                    psPortOP->u32PlanarProcIdx = i;
                }
            }
            else if (i == psPortOP->u32PlanarInIdx)
            {
                //Nothing to do
            }
            else if (psPortOP->pabIsPlanarBufFull[i] == TRUE)
            {
                if ((i != psPortOP->u32PlanarProcIdx) && (i != psPortOP->u32PlanarEncIdx))
                    psPortOP->pabIsPlanarBufFull[i] = FALSE;
            }
        }

        for (i = 0; i < i32MaxPacketFrameCnt; i ++)
        {
            if (((i + 1) % i32MaxPacketFrameCnt)  == psPortOP->u32PacketInIdx)
            {
                if (psPortOP->pabIsPacketBufFull[i] == TRUE)
                {
                    psPortOP->u32PacketProcIdx = i;
                }
            }
            else if (i == psPortOP->u32PacketInIdx)
            {
                //Nothing to do
            }
            else if (psPortOP->pabIsPacketBufFull[i] == TRUE)
            {
                if ((i != psPortOP->u32PacketProcIdx) && (i != psPortOP->u32PacketEncIdx))
                    psPortOP->pabIsPacketBufFull[i] = FALSE;
            }
        }

		//Unlock frame index mutex
        xSemaphoreGive(psPortOP->tFrameIndexMutex);

        u64CurTime = mp_hal_ticks_ms();
        u64CalFPSTime = u64CurTime - u64CalFPSStartTime;
        u32Frames ++;

#if defined (__VIN_ONE_SHOT_MODE__)
        vTaskDelay(1 / portTICK_RATE_MS);
        psPortOP->sVinDev.SetOperationMode(TRUE);
        psPortOP->sVinDev.SetShadowRegister();
#endif

		//Calculate FPS
        if (u64CalFPSTime >= 5000)
        {
            uint32_t u32NewFPS;

            u32NewFPS = (u32Frames) / (u64CalFPSTime / 1000);

//            printf("Video_in port %d, %d fps \n", psPortOP->u32PortNo, u32NewFPS);
            u32Frames = 0;
            u64CalFPSStartTime = mp_hal_ticks_ms();
        }
    }
}

//Port 0 interrupt handle
static void VideoIn_InterruptHandler(
    uint8_t u8PacketBufID,
    uint8_t u8PlanarBufID,
    uint8_t u8FrameRate,
    uint8_t u8Filed
)
{
    //Handle the irq and use freeRTOS semaphore to wakeup vin thread
    S_VIN_PORT_OP *psPortOP = &s_sVinPortOP;
    portBASE_TYPE xHigherPrioTaskWoken = pdFALSE;

    if (psPortOP == NULL)
        return;

    VideoInFrameBufSwitch(psPortOP);

#if !defined (__VIN_ONE_SHOT_MODE__)
    psPortOP->sVinDev.SetShadowRegister();
#endif

    xSemaphoreGiveFromISR(psPortOP->tVinISRSem, &xHigherPrioTaskWoken);
    if (xHigherPrioTaskWoken == pdTRUE)
    {
        portEXIT_SWITCHING_ISR(xHigherPrioTaskWoken);
    }
}

static void VideoIn_TaskCreate(
    void *pvParameters
)
{
    VinFrameLoop(&s_sVinPortOP);
    vTaskDelete(NULL);
}

static int VideoIn_SetColorFormat(
	planar_pixformat_t planar_pixformat,
	pixformat_t packet_pixformat
)
{
	E_VIDEOIN_OUT_FORMAT ePacketFormat;
	E_VIDEOIN_PLANAR_FORMAT ePlanarFormat;
	VINDEV_T *psVinDev = &s_sVinPortOP.sVinDev;

	g_sPacketPipeFrame.bpp =2;
	g_sPlanarPipeFrame.bpp =2;
	
	if(planar_pixformat == PLANAR_PIXFORMAT_YUV420MB)
		ePlanarFormat = eVIDEOIN_MACRO_PLANAR_YUV420;
	else if(planar_pixformat == PLANAR_PIXFORMAT_YUV420)
		ePlanarFormat = eVIDEOIN_PLANAR_YUV420;
	else if (planar_pixformat == PLANAR_PIXFORMAT_YUV422)
		ePlanarFormat = eVIDEOIN_PLANAR_YUV422;
	else
		return -1;

	if(packet_pixformat == PIXFORMAT_GRAYSCALE)
	{
		g_sPacketPipeFrame.bpp =1;
		ePacketFormat = eVIDEOIN_OUT_ONLY_Y;
	}
	else if(packet_pixformat == PIXFORMAT_RGB565)
		ePacketFormat = eVIDEOIN_OUT_RGB565;
	else if(packet_pixformat == PIXFORMAT_YUV422)
		ePacketFormat = eVIDEOIN_OUT_YUV422;
	else
		return -2;

    psVinDev->SetDataFormatAndOrder(eVIDEOIN_IN_UYVY,               //NT99141
    
                                    eVIDEOIN_IN_YUV422,     //Intput format
                                    ePacketFormat);     //Output format for packet

    psVinDev->SetPlanarFormat(ePlanarFormat);               // Planar YUV422/420/macro
	return 0;
}

static uint16_t
Util_GCD(
	uint16_t m1,
	uint16_t m2
)
{
	uint16_t m;

	if (m1 < m2) {
		m = m1;
		m1 = m2;
		m2 = m;
	}

	if (m1 % m2 == 0)
		return m2;
	else
		return (Util_GCD(m2, m1 % m2));
}


static int VideoIn_SetFrameSize(
	framesize_t eSensorInputSize,
	framesize_t ePlanarFrameSize,
	framesize_t ePacketFrameSize
)
{
	VINDEV_T *psVinDev = &s_sVinPortOP.sVinDev;

	uint16_t u16SnrW = resolution[eSensorInputSize][0];
	uint16_t u16SnrH = resolution[eSensorInputSize][1];
	uint16_t u16PlanarFrameW = resolution[ePlanarFrameSize][0];
	uint16_t u16PlanarFrameH = resolution[ePlanarFrameSize][1];
	uint16_t u16PacketFrameW = resolution[ePacketFrameSize][0];
	uint16_t u16PacketFrameH = resolution[ePacketFrameSize][1];

	if((u16PlanarFrameW * u16PlanarFrameH * 2) > s_u32PlanarFrameBufSize)
		return -1;
		
	if((u16PacketFrameW * u16PacketFrameH * 2) > s_u32PacketFrameBufSize)
		return -2;

    uint32_t u32WidthFac;
    uint32_t u32HeightFac;
    uint32_t u32Fac;
    uint16_t u32GCD;
    uint32_t u32PlanarRatioW;
    uint32_t u32PlanarRatioH;

    u32GCD = Util_GCD(u16PlanarFrameW, u16PlanarFrameH);

    u32PlanarRatioW = u16PlanarFrameW / u32GCD;
    u32PlanarRatioH = u16PlanarFrameH / u32GCD;

    u32WidthFac = u16SnrW / u32PlanarRatioW;
    u32HeightFac = u16SnrH / u32PlanarRatioH;

    if (u32WidthFac < u32HeightFac)
        u32Fac = u32WidthFac;
    else
        u32Fac = u32HeightFac;

    uint32_t u32CapEngCropWinW = u32PlanarRatioW * u32Fac;
    uint32_t u32CapEngCropWinH = u32PlanarRatioH * u32Fac;

    uint32_t u32CapEngCropStartX = 0;
    uint32_t u32CapEngCropStartY = 0;

    if (u16SnrW > u32CapEngCropWinW)
    {
        u32CapEngCropStartX = (u16SnrW - u32CapEngCropWinW) / 2;
    }

    if (u16SnrH > u32CapEngCropWinH)
    {
        u32CapEngCropStartY = (u16SnrH - u32CapEngCropWinH) / 2;
    }

    psVinDev->SetCropWinStartAddr(u32CapEngCropStartY,       //Horizontal start position
                                  u32CapEngCropStartX);

    psVinDev->SetCropWinSize(u32CapEngCropWinH,      //UINT16 u16Height,
                             u32CapEngCropWinW);              //UINT16 u16Width

    psVinDev->EncodePipeSize(u16PlanarFrameH, u16PlanarFrameW);
    psVinDev->PreviewPipeSize(u16PacketFrameH, u16PacketFrameW);
    psVinDev->SetStride(u16PacketFrameW, u16PlanarFrameW);

	return 0;
}

static void VideoIn_ReadCurPlanarFrame(
	S_VIN_PORT_OP   *psPortOP,
    uint8_t **ppu8FrameData
)
{
    if (ppu8FrameData == NULL)
        return;

    if (psPortOP->tFrameIndexMutex == NULL)
        return;

    xSemaphoreTake(psPortOP->tFrameIndexMutex, portMAX_DELAY);
    *ppu8FrameData = psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarEncIdx];
    xSemaphoreGive(psPortOP->tFrameIndexMutex);
}

static BOOL
VideoIn_ReadNextPlanarFrame(
	S_VIN_PORT_OP   *psPortOP,
    uint8_t **ppu8FrameData,
    uint64_t *pu64FrameTime
)
{
    if (ppu8FrameData == NULL)
        return FALSE;

    if (psPortOP->tFrameIndexMutex == NULL)
        return FALSE;
	
    xSemaphoreTake(psPortOP->tFrameIndexMutex, portMAX_DELAY);

    if (psPortOP->u32PlanarProcIdx == psPortOP->u32PlanarEncIdx)
    {
        xSemaphoreGive(psPortOP->tFrameIndexMutex);
        return FALSE;
    }

    psPortOP->pabIsPlanarBufFull[psPortOP->u32PlanarEncIdx] = FALSE;

    psPortOP->u32PlanarEncIdx = psPortOP->u32PlanarProcIdx;
    *ppu8FrameData = psPortOP->apu8PlanarFrameBufPtr[psPortOP->u32PlanarEncIdx];
#if MICROPY_NVTMEDIA
	*pu64FrameTime = NMUtil_GetTimeMilliSec();
#else
	*pu64FrameTime = mp_hal_ticks_ms();
#endif
    xSemaphoreGive(psPortOP->tFrameIndexMutex);

    return TRUE;
}

static void VideoIn_ReadCurPacketFrame(
	S_VIN_PORT_OP   *psPortOP,
    uint8_t **ppu8FrameData
)
{
    if (ppu8FrameData == NULL)
        return;

    if (psPortOP->tFrameIndexMutex == NULL)
        return;

    xSemaphoreTake(psPortOP->tFrameIndexMutex, portMAX_DELAY);
    *ppu8FrameData = psPortOP->apu8PacketFrameBufPtr[psPortOP->u32PacketEncIdx];
    xSemaphoreGive(psPortOP->tFrameIndexMutex);
}

static BOOL
VideoIn_ReadNextPacketFrame(
	S_VIN_PORT_OP   *psPortOP,
    uint8_t **ppu8FrameData,
    uint64_t *pu64FrameTime
)
{
    if (ppu8FrameData == NULL)
        return FALSE;

    if (psPortOP->tFrameIndexMutex == NULL)
        return FALSE;

    xSemaphoreTake(psPortOP->tFrameIndexMutex, portMAX_DELAY);

    if (psPortOP->u32PacketProcIdx == psPortOP->u32PacketEncIdx)
    {
        xSemaphoreGive(psPortOP->tFrameIndexMutex);
        return FALSE;
    }

    psPortOP->pabIsPacketBufFull[psPortOP->u32PacketEncIdx] = FALSE;

    psPortOP->u32PacketEncIdx = psPortOP->u32PacketProcIdx;
    *ppu8FrameData = psPortOP->apu8PacketFrameBufPtr[psPortOP->u32PacketEncIdx];

#if MICROPY_NVTMEDIA
	*pu64FrameTime = NMUtil_GetTimeMilliSec();
#else
	*pu64FrameTime = mp_hal_ticks_ms();
#endif

//	printf("DDDDDDDD VideoIn_ReadNextPacketFrame in %d , %d, %d \n",  psPortOP->u32PacketInIdx,  psPortOP->u32PacketProcIdx,  psPortOP->u32PacketEncIdx);
    xSemaphoreGive(psPortOP->tFrameIndexMutex);

    return TRUE;
}

static int32_t VideoIn_StartRun(
	sensor_t *sensor,
	S_VIN_PORT_OP   *psPortOP
)
{
	int32_t i32Ret;
	VINDEV_T *psVinDev = &psPortOP->sVinDev;
	uint16_t u16PlanarFrameW = resolution[sensor->framesize_planar][0];
	uint16_t u16PlanarFrameH = resolution[sensor->framesize_planar][1];
	PFN_VIDEOIN_CALLBACK pfnOldCallback;

	
	i32Ret = sensor_set_framesize(sensor->framesize_planar, sensor->framesize_packet);
	
	if(i32Ret != 0)
		return -1;
		
    psVinDev->SetPacketFrameBufferControl(FALSE, FALSE);

	i32Ret = sensor_set_pixformat(sensor->pixformat_planar, sensor->pixformat_packet);
	
	if(i32Ret != 0)
		return -1;

    psVinDev->SetStandardCCIR656(TRUE);                     //standard CCIR656 mode
    psVinDev->SetSensorPolarity(FALSE,
                                FALSE,
                                FALSE);

    psVinDev->SetBaseStartAddress(eVIDEOIN_PACKET, (E_VIDEOIN_BUFFER)0, (UINT32)s_au8PacketFrameBuffer0);

    psVinDev->SetBaseStartAddress(eVIDEOIN_PLANAR,
                                  (E_VIDEOIN_BUFFER)0,                            //Planar buffer Y addrress
                                  (UINT32) s_au8PlanarFrameBuffer0);

    psVinDev->SetBaseStartAddress(eVIDEOIN_PLANAR,
                                  (E_VIDEOIN_BUFFER)1,                            //Planar buffer U addrress
                                  (UINT32)(s_au8PlanarFrameBuffer0 + (u16PlanarFrameW * u16PlanarFrameH)));


    if (sensor->pixformat_planar == PLANAR_PIXFORMAT_YUV420MB)
    {
        psVinDev->SetBaseStartAddress(eVIDEOIN_PLANAR,
                                      (E_VIDEOIN_BUFFER)2,                            //Planar buffer U addrress
                                      (UINT32)(s_au8PlanarFrameBuffer0 + (u16PlanarFrameW * u16PlanarFrameH)));
    }
    else if (sensor->pixformat_planar == PLANAR_PIXFORMAT_YUV422)
    {
        psVinDev->SetBaseStartAddress(eVIDEOIN_PLANAR,
                                      (E_VIDEOIN_BUFFER)2,                            //Planar buffer V addrress
                                      (UINT32)(s_au8PlanarFrameBuffer0 + (u16PlanarFrameW * u16PlanarFrameH) + ((u16PlanarFrameW * u16PlanarFrameH) / 2)));
    }
    else if (sensor->pixformat_planar == PLANAR_PIXFORMAT_YUV420)
    {
        psVinDev->SetBaseStartAddress(eVIDEOIN_PLANAR,
                                      (E_VIDEOIN_BUFFER)2,                            //Planar buffer V addrress
                                      (UINT32)(s_au8PlanarFrameBuffer0 + (u16PlanarFrameW * u16PlanarFrameH) + ((u16PlanarFrameW * u16PlanarFrameH) / 4)));
    }

    psVinDev->SetPipeEnable(TRUE,                                   // Engine enable?
                            eVIDEOIN_BOTH_PIPE_ENABLE);     // which packet was enabled.

    psVinDev->EnableInt(eVIDEOIN_VINT);

    psVinDev->InstallCallback(eVIDEOIN_VINT,
                              (PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
                              &pfnOldCallback);   //Frame End interrupt

    uint32_t u32FrameRateGCD;
    uint16_t u16InputFrameRate = s_u32InputFrameRate;
    uint16_t u16TargetFrameRate = 30;
    uint32_t u32RatioM;
    uint32_t u32RatioN;

    u32FrameRateGCD = Util_GCD(u16InputFrameRate, u16TargetFrameRate);
    u32RatioM = s_u32InputFrameRate / u32FrameRateGCD;
    u32RatioN = u16TargetFrameRate / u32FrameRateGCD;

    if (u32RatioM > u32RatioN)
    {
		outp32(REG_VPEFRC, u32RatioN << 8 | u32RatioM);
    }

    psVinDev->SetShadowRegister();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Global functions
//////////////////////////////////////////////////////////////////////////////////////////////////////
void sensor_init0()
{
	//Config sensor power down and reset pin to gpio output mode
#if defined (SENSOR_POWERDOWN_PIN)
	mp_hal_pin_config(SENSOR_POWERDOWN_PIN, GPIO_MODE_OUTPUT, 0);
#endif

#if defined (SENSOR_RESET_PIN)
	mp_hal_pin_config(SENSOR_RESET_PIN, GPIO_MODE_OUTPUT, 0);
#endif


	g_sPlanarPipeFrame.w = g_sPlanarPipeFrame.u = VIN_CONFIG_PORT0_PLANAR_WIDTH;
	g_sPlanarPipeFrame.h = g_sPlanarPipeFrame.v = VIN_CONFIG_PORT0_PLANAR_HEIGHT;
	g_sPlanarPipeFrame.bpp = 2;
	g_sPlanarPipeFrame.pixels = (uint8_t *)s_au8PlanarFrameBuffer0;

	g_sPacketPipeFrame.w = g_sPacketPipeFrame.u = VIN_CONFIG_PORT0_PACKET_WIDTH;
	g_sPacketPipeFrame.h = g_sPacketPipeFrame.v = VIN_CONFIG_PORT0_PACKET_HEIGHT;
	g_sPacketPipeFrame.bpp = 2;
	g_sPacketPipeFrame.pixels = (uint8_t *)s_au8PacketFrameBuffer0;

	memset(&s_sVinPortOP, 0, sizeof(S_VIN_PORT_OP));

    s_sVinPortOP.u32PortNo = 0;
	s_sVinPortOP.psPlanarPipeFrame = &g_sPlanarPipeFrame;
	s_sVinPortOP.psPacketPipeFrame = &g_sPacketPipeFrame;

	s_sVinPortOP.u32PacketMaxFrameCnt = VIN_CONFIG_PORT0_PACKET_FRAME_CNT;
	s_sVinPortOP.u32PlanarMaxFrameCnt = VIN_CONFIG_PORT0_PLANAR_FRAME_CNT;
	s_sVinPortOP.pabIsPacketBufFull = s_abIsPacketBufFull;
	s_sVinPortOP.pabIsPlanarBufFull = s_abIsPlanarBufFull;

	s_sVinPortOP.u32PacketInIdx = 0;
	s_sVinPortOP.u32PacketProcIdx = 0;
	s_sVinPortOP.u32PacketEncIdx = 0;

	s_sVinPortOP.u32PlanarInIdx = 0;
	s_sVinPortOP.u32PlanarProcIdx = 0;
	s_sVinPortOP.u32PlanarEncIdx = 0;

	s_sVinPortOP.apu8PlanarFrameBufPtr = s_apu8PlanarFrameBufPtr;
	s_sVinPortOP.apu8PacketFrameBufPtr = s_apu8PacketFrameBufPtr;
}

int sensor_init()
{
    int init_ret = 0;
	VINDEV_T *psVinDev = &s_sVinPortOP.sVinDev;

	s_sVinPortOP.tVinISRSem = xSemaphoreCreateBinary();

	if (s_sVinPortOP.tVinISRSem == NULL)
	{
		printf("Unable create video-in interrupt semaphore \n");
		return -1;
	}

	s_sVinPortOP.tFrameIndexMutex = xSemaphoreCreateMutex();
	if (s_sVinPortOP.tFrameIndexMutex == NULL)
	{
		printf("Unable create video-in frame index mutex \n");
		return -2;
	}

	init_ret = register_vin_device(1, psVinDev);

	if (init_ret < 0)
	{
		printf("Register vin 0 device fail\n");
		return -3;
	}

	psVinDev->Init(TRUE, (E_VIDEOIN_SNR_SRC)eSYS_UPLL, 24000, eVIDEOIN_SNR_CCIR601);
	psVinDev->Open(72000, s_u32SnrClk);

#if defined (SENSOR_POWERDOWN_PIN)
	mp_hal_pin_low(SENSOR_POWERDOWN_PIN);
	mp_hal_delay_ms(100);
#endif

#if defined (SENSOR_RESET_PIN)
	mp_hal_pin_high(SENSOR_RESET_PIN);
	mp_hal_delay_ms(100);
	mp_hal_pin_low(SENSOR_RESET_PIN);
	mp_hal_delay_ms(100);
	mp_hal_pin_high(SENSOR_RESET_PIN);
#endif

    /* Reset the sesnor state */
    memset(&sensor, 0, sizeof(sensor_t));

    /* Some sensors have different reset polarities, and we can't know which sensor
       is connected before initializing cambus and probing the sensor, which in turn
       requires pulling the sensor out of the reset state. So we try to probe the
       sensor with both polarities to determine line state. */
    sensor.pwdn_pol = ACTIVE_LOW;
    sensor.reset_pol = ACTIVE_LOW;

#if (OMV_ENABLE_NT99141 == 1)
	sensor.slv_addr = NT99141_SLV_ADDR;
    sensor.chip_id = NT99141_ID;
#endif

    // Set default snapshot function.
    sensor.snapshot = sensor_snapshot;

    switch (sensor.chip_id) {
        #if (OMV_ENABLE_NT99141 == 1)
        case NT99141_ID:
            init_ret = NT99141_init(&sensor);
            break;
        #endif // (OMV_ENABLE_OV2640 == 1)
        default:
            return -4;
            break;
    }

    // Call sensor-specific reset function
    if (sensor.reset(&sensor) != 0) {
		printf("Unable reset sensor\n");
        return -5;
    }

	xTaskCreate(VideoIn_TaskCreate, "VideoIn", configMINIMAL_STACK_SIZE, NULL, VIN_TASK_PRIORITY, NULL);

    // Set default color palette.
    sensor.color_palette = rainbow_table;
    sensor.detected = true;
	s_bSensorRun = false;
	return init_ret;
}

int sensor_shutdown(int enable)
{
	VINDEV_T *psVinDev = &s_sVinPortOP.sVinDev;

    if (enable) {
		psVinDev->Close();
#if defined (SENSOR_POWERDOWN_PIN)
		mp_hal_pin_high(SENSOR_POWERDOWN_PIN);
#endif
    } else {
#if defined (SENSOR_POWERDOWN_PIN)
		mp_hal_pin_low(SENSOR_POWERDOWN_PIN);
		mp_hal_delay_ms(100);
		psVinDev->Init(TRUE, (E_VIDEOIN_SNR_SRC)eSYS_UPLL, 24000, eVIDEOIN_SNR_CCIR601);
		psVinDev->Open(72000, s_u32SnrClk);
#endif
    }

	mp_hal_delay_ms(100);
    return 0;
}

int sensor_reset()
{
	VINDEV_T *psVinDev = &s_sVinPortOP.sVinDev;

	psVinDev->Close();

    // Reset the sensor state
    sensor.sde           = 0;
    sensor.pixformat_packet     = 0;
    sensor.pixformat_planar     = 0;
    sensor.framesize_packet     = 0;
    sensor.framesize_planar     = 0;
    sensor.gainceiling   = 0;
    sensor.hmirror       = false;
    sensor.vflip         = false;
    sensor.transpose     = false;
    sensor.auto_rotation = false;
//    sensor.vsync_gpio    = NULL;

    // Reset default color palette.
    sensor.color_palette = rainbow_table;

    // Restore shutdown state on reset.
    sensor_shutdown(false);

    // Hard-reset the sensor
    if (sensor.reset_pol == ACTIVE_HIGH) {
#if defined (SENSOR_RESET_PIN)
		mp_hal_pin_low(SENSOR_RESET_PIN);
		mp_hal_delay_ms(100);
		mp_hal_pin_high(SENSOR_RESET_PIN);
		mp_hal_delay_ms(100);
		mp_hal_pin_low(SENSOR_RESET_PIN);
#endif
    } else {
#if defined (SENSOR_RESET_PIN)
	mp_hal_pin_high(SENSOR_RESET_PIN);
	mp_hal_delay_ms(100);
	mp_hal_pin_low(SENSOR_RESET_PIN);
	mp_hal_delay_ms(100);
	mp_hal_pin_high(SENSOR_RESET_PIN);
#endif
    }

	mp_hal_delay_ms(100);

    // Call sensor-specific reset function
    if (sensor.reset(&sensor) != 0) {
        return -1;
    }

    s_bSensorRun = false;
	return 0;
}

int sensor_get_id()
{
    return sensor.chip_id;
}

BOOL sensor_is_detected()
{
    return sensor.detected;
}

int sensor_set_contrast(int level)
{
    if (sensor.set_contrast != NULL) {
        return sensor.set_contrast(&sensor, level);
    }
    return -1;
}

int sensor_set_brightness(int level)
{
    if (sensor.set_brightness != NULL) {
        return sensor.set_brightness(&sensor, level);
    }
    return -1;
}

int sensor_set_hmirror(int enable)
{
    if (sensor.hmirror == ((bool) enable)) {
        /* no change */
        return 0;
    }

    /* call the sensor specific function */
    if (sensor.set_hmirror == NULL
        || sensor.set_hmirror(&sensor, enable) != 0) {
        /* operation not supported */
        return -1;
    }
    sensor.hmirror = enable;
    mp_hal_delay_ms(100); // wait for the camera to settle
    return 0;
}

BOOL sensor_get_hmirror()
{
    return sensor.hmirror;
}

int sensor_set_vflip(int enable)
{
    if (sensor.vflip == ((bool) enable)) {
        /* no change */
        return 0;
    }

    /* call the sensor specific function */
    if (sensor.set_vflip == NULL
        || sensor.set_vflip(&sensor, enable) != 0) {
        /* operation not supported */
        return -1;
    }
    sensor.vflip = enable;
    mp_hal_delay_ms(100); // wait for the camera to settle
    return 0;
}

BOOL sensor_get_vflip()
{
    return sensor.vflip;
}

int sensor_set_special_effect(sde_t sde)
{
    if (sensor.sde == sde) {
        /* no change */
        return 0;
    }

    /* call the sensor specific function */
    if (sensor.set_special_effect == NULL
        || sensor.set_special_effect(&sensor, sde) != 0) {
        /* operation not supported */
        return -1;
    }

    sensor.sde = sde;
    return 0;
}

int sensor_read_reg(uint16_t reg_addr)
{
    if (sensor.read_reg == NULL) {
        // Operation not supported
        return -1;
    }
    return sensor.read_reg(&sensor, reg_addr);
}

int sensor_write_reg(uint16_t reg_addr, uint16_t reg_data)
{
    if (sensor.write_reg == NULL) {
        // Operation not supported
        return -1;
    }
    return sensor.write_reg(&sensor, reg_addr, reg_data);
}

int sensor_set_color_palette(const uint16_t *color_palette)
{
    sensor.color_palette = color_palette;
    return 0;
}

const uint16_t *sensor_get_color_palette()
{
    return sensor.color_palette;
}

int sensor_set_pixformat(
	planar_pixformat_t planar_pixformat,
	pixformat_t packet_pixformat
)
{
	int i32Ret;

	if((packet_pixformat == PIXFORMAT_BINARY) || 
		(packet_pixformat == PIXFORMAT_BAYER) ||
		(packet_pixformat == PIXFORMAT_JPEG)){
		printf("Pixel format is not support \n");
		return -1;
	}
		
    if (sensor.set_pixformat == NULL
        || sensor.set_pixformat(&sensor, packet_pixformat) != 0) {
        // Operation not supported
        return -2;
    }

    mp_hal_delay_ms(100); // wait for the camera to settle

	i32Ret = VideoIn_SetColorFormat(planar_pixformat, packet_pixformat);

	if(i32Ret != 0){
		printf("Pixel format is not support by vin engine \n");
		return -3;
	}

	sensor.pixformat_packet = packet_pixformat;
	sensor.pixformat_planar = planar_pixformat;

	return 0;
}

int sensor_set_framesize(
	framesize_t planar_framesize,
	framesize_t packet_framesize
)
{
	framesize_t eSensorInputSize;
	int i32Ret;
	uint16_t u16PlanarFrameW = resolution[planar_framesize][0];
	uint16_t u16PlanarFrameH = resolution[planar_framesize][1];
	uint16_t u16PacketFrameW = resolution[packet_framesize][0];
	uint16_t u16PacketFrameH = resolution[packet_framesize][1];
	
#if 0	
    if ((sensor.framesize_planar == planar_framesize) &&
		(sensor.framesize_packet == packet_framesize)) {
        // No change
        return 0;
    }
#endif
    
	if((u16PacketFrameW > u16PlanarFrameW) || (u16PacketFrameH > u16PlanarFrameH)){
		printf("planar frame size must large than packet frame size \n");
		return -1;
	}	

    // Call the sensor specific function
    if (sensor.set_framesize == NULL) {
        // Operation not supported
        return -2;
    }
	
	eSensorInputSize = sensor.set_framesize(&sensor, planar_framesize);
	
	if(eSensorInputSize <= 0) {
		printf("Unable set sensor frame size \n");
		return -3;
	}

	i32Ret = VideoIn_SetFrameSize(eSensorInputSize, planar_framesize, packet_framesize);

	if(i32Ret != 0){
		printf("Unable set VIN frame size \n");
		return -4;
	}
	
	sensor.framesize_packet = packet_framesize;
	sensor.framesize_planar = planar_framesize;
	
    // Set MAIN FB x offset, y offset, width, height, backup width, and backup height.
    MAIN_FB()->x = 0;
    MAIN_FB()->y = 0;
    MAIN_FB()->w = MAIN_FB()->u = resolution[packet_framesize][0];
    MAIN_FB()->h = MAIN_FB()->v = resolution[packet_framesize][1];

	return 0;
}

int sensor_get_fb(
	sensor_t *sensor,
	image_t *planar_image,
	image_t *packet_image
)
{
	if(planar_image)
	{
		planar_image->w = resolution[sensor->framesize_planar][0];
		planar_image->h = resolution[sensor->framesize_planar][1];
		planar_image->bpp  = g_sPlanarPipeFrame.bpp;
		VideoIn_ReadCurPlanarFrame(&s_sVinPortOP, &planar_image->pixels);
	}
	
	if(packet_image)
	{
		packet_image->w = resolution[sensor->framesize_packet][0];
		packet_image->h = resolution[sensor->framesize_packet][1];
		packet_image->bpp = g_sPacketPipeFrame.bpp;
		VideoIn_ReadCurPacketFrame(&s_sVinPortOP, &packet_image->pixels);
	}
	return 0;
}

BOOL sensor_ReadPlanarImage(
	sensor_t *sensor,
	image_t *psPlanarImage,
	uint64_t *pu64FrameTime
)
{
	if(s_bSensorRun == false){
		printf("Please run sensor snapshot/skip_frame first \n");
		return false;
	}

	if(psPlanarImage == NULL)
		return false;
		
	psPlanarImage->w = resolution[sensor->framesize_planar][0];
	psPlanarImage->h = resolution[sensor->framesize_planar][1];
	psPlanarImage->bpp  = g_sPlanarPipeFrame.bpp;

	return VideoIn_ReadNextPlanarFrame(&s_sVinPortOP, &psPlanarImage->pixels, pu64FrameTime);
}

BOOL sensor_ReadPacketImage(
	sensor_t *sensor,
	image_t *psPacketImage,
	uint64_t *pu64FrameTime
)
{
	if(s_bSensorRun == false){
		printf("Please run sensor snapshot/skip_frame first \n");
		return false;
	}

	if(psPacketImage == NULL)
		return false;
		
	psPacketImage->w = resolution[sensor->framesize_packet][0];
	psPacketImage->h = resolution[sensor->framesize_packet][1];
	psPacketImage->bpp  = g_sPacketPipeFrame.bpp;

	return VideoIn_ReadNextPacketFrame(&s_sVinPortOP, &psPacketImage->pixels, pu64FrameTime);
}


// This is the default snapshot function, which can be replaced in sensor_init functions.
int sensor_snapshot(
	sensor_t *sensor,
	image_t *planar_image,
	image_t *packet_image,
	streaming_cb_t streaming_cb
)
{
	int i32Ret;
    bool streaming = (streaming_cb != NULL); // Streaming mode.
	uint32_t u32CurTime;

	uint64_t u64SnapTime;

    // In streaming mode the image pointer must be valid.
    if (streaming) {
        if ((planar_image == NULL) && (packet_image == NULL)) {
			printf("image pointer is NULL \n");
            return -1;
        }
    }

	if(s_bSensorRun == false){
		int i32FlushFrames = 3;
		uint8_t *pu8PlanarFrame;
		uint8_t *pu8PacketFrame;
		
		i32Ret = VideoIn_StartRun(sensor, &s_sVinPortOP);
		if(i32Ret != 0){
			printf("Vin unable run \n");
			return -2;
		}

		//flush unstable image
		printf("start flush image\n");
		
		while(i32FlushFrames >= 0){
			while(VideoIn_ReadNextPlanarFrame(&s_sVinPortOP, &pu8PlanarFrame, &u64SnapTime) == FALSE);
			while(VideoIn_ReadNextPacketFrame(&s_sVinPortOP, &pu8PacketFrame, &u64SnapTime) == FALSE);			
			i32FlushFrames --;
		}
		
		printf("end flush image\n");
		s_bSensorRun = true;
	}

	if(planar_image){
		planar_image->w = resolution[sensor->framesize_planar][0];
		planar_image->h = resolution[sensor->framesize_planar][1];
		planar_image->bpp  = g_sPlanarPipeFrame.bpp;
	}

	if(packet_image){
		packet_image->w = resolution[sensor->framesize_packet][0];
		packet_image->h = resolution[sensor->framesize_packet][1];
		packet_image->bpp = g_sPacketPipeFrame.bpp;
	}

	do
	{
		if(planar_image){
			planar_image->pixels = NULL;
			u32CurTime = mp_hal_ticks_ms();

			while(VideoIn_ReadNextPlanarFrame(&s_sVinPortOP, &planar_image->pixels, &u64SnapTime) == FALSE)
			{
				mp_hal_delay_ms(2);
				if((mp_hal_ticks_ms() - u32CurTime) > 3000)	//timeout check
					break;
			}
//			printf("DDDDDDD sensor snapshot planar image address %x \n", planar_image->pixels);
		}

		if(packet_image){
			packet_image->pixels = NULL;
			u32CurTime = mp_hal_ticks_ms();

			while(VideoIn_ReadNextPacketFrame(&s_sVinPortOP, &packet_image->pixels, &u64SnapTime) == FALSE)
			{
				mp_hal_delay_ms(2);
				if((mp_hal_ticks_ms() - u32CurTime) > 3000)	//timeout check
					break;
			}
//			printf("DDDDDDD sensor snapshot packet image address %x \n", packet_image->pixels);
		}

        // Before we wait for the next frame try to get some work done. If we are in double buffer
        // mode then we can start processing the previous image buffer.
        if (streaming_cb) {
            // Call streaming callback function with previous frame.
            // Note: Image pointer should Not be NULL in streaming mode.
            streaming = streaming_cb(planar_image, packet_image);
        }
        
	}while(streaming == true);

	return 0;
}
