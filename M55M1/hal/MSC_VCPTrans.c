/***************************************************************************//**
 * @file     MSC_VCPTrans.c
 * @brief    M480 series USB class transfer code for VCP and MSC
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NuMicro.h"
#include "M55M1_USBD.h"
#include "MSC_VCPTrans.h"
#include "StorIF.h"

#include "mpconfigboard.h"
#include "mpconfigboard_common.h"

//EP2 for VCP bulk in and address  0x2
//EP3 for VCP bulk out and address 0x3
//EP4 for VCP interrupt in and address 0x4
//EP5 for MSC bulk in and address 0x5
//EP6 for MSC bulk out and address 0x6

/*--------------------------------------------------------------------------*/
/* Global variables for all */

static S_USBD_INFO_T *s_psUSBDevInfo =  NULL;

/*--------------------------------------------------------------------------*/
/* Global variables for MSC */

static uint8_t volatile g_u8EP5Ready = 0;
static uint8_t volatile g_u8EP6Ready = 0;

static uint8_t g_au8SenseKey[4];

static uint32_t g_u32DataFlashStartAddr;
static uint32_t g_u32Length;
static uint32_t g_u32Address;
static uint32_t g_u32BytesInStorageBuf;
static uint32_t g_u32LbaAddress;
static uint8_t g_u8Size;

/* USB flow control variables */
static uint8_t g_u8BulkState;
uint8_t g_u8Prevent = 0;
static uint32_t g_u32BulkBuf0, g_u32BulkBuf1;

/* CBW/CSW variables */
struct CBW g_sCBW;
struct CSW g_sCSW;

static uint32_t MassBlock[DCACHE_ALIGN_LINE_SIZE(MASS_BUFFER_SIZE) / 4]__attribute__((aligned(DCACHE_LINE_SIZE)));
static uint32_t Storage_Block[DCACHE_ALIGN_LINE_SIZE(STORAGE_BUFFER_SIZE) / 4]__attribute__((aligned(DCACHE_LINE_SIZE)));

#if 0 //(NVT_DCACHE_ON == 1)
//USBD_MemCopy is CPU memory copy operation(not DMA), so we don't need to care DCache coherence issue
static uint8_t DCacheTempBuf[DCACHE_ALIGN_LINE_SIZE(EP6_MSC_MAX_PKT_SIZE)]__attribute__((aligned(DCACHE_LINE_SIZE)));
#endif

#define DEF_MAX_LUN 4

static S_STORIF_IF *s_apsLUNStorIf[DEF_MAX_LUN];
static PFN_USBDEV_VCPRecvSignal s_pfnVCPRecvSignel = NULL;

#define MassCMD_BUF        ((uint32_t)&MassBlock[0])
#define STORAGE_DATA_BUF   ((uint32_t)&Storage_Block[0])

//static uint32_t volatile g_u32MSCOutToggle = 0, g_u32MSCOutSkip = 0;
extern uint32_t volatile g_u32MSCOutToggle;
extern uint32_t volatile g_u32MSCOutSkip;
//uint8_t volatile g_u8Remove = 0;
extern uint8_t volatile g_u8MSCRemove;
extern uint32_t volatile g_u32VCPConnect;
extern uint32_t volatile g_u32MSCConnect;

static bool s_bMSCWriteProtect = false;

/*--------------------------------------------------------------------------*/
static uint8_t g_au8InquiryID[36] = {
    0x00,                   /* Peripheral Device Type */
    0x80,                   /* RMB */
    0x00,                   /* ISO/ECMA, ANSI Version */
    0x00,                   /* Response Data Format */
    0x1F, 0x00, 0x00, 0x00, /* Additional Length */

    /* Vendor Identification */
    'N', 'u', 'v', 'o', 't', 'o', 'n', ' ',

    /* Product Identification */
    'U', 'S', 'B', ' ', 'M', 'a', 's', 's', ' ', 'S', 't', 'o', 'r', 'a', 'g', 'e',

    /* Product Revision */
    '1', '.', '0', '0'
};


// code = 5Ah, Mode Sense 10
static uint8_t g_au8ModePage_01[12] = {
    0x01, 0x0A, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00
};

static uint8_t g_au8ModePage_05[32] = {
    0x05, 0x1E, 0x13, 0x88, 0x08, 0x20, 0x02, 0x00,
    0x01, 0xF4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x1E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x68, 0x00, 0x00
};

static uint8_t g_au8ModePage_1B[12] = {
    0x1B, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static uint8_t g_au8ModePage_1C[8] = {
    0x1C, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00
};

/*--------------------------------------------------------------------------*/
/* Global variables for VCP */
static STR_VCOM_LINE_CODING gLineCoding = {115200, 0, 0, 8};   /* Baud rate : 115200    */
/* Stop bit     */
/* parity       */
/* data bits    */
static uint16_t gCtrlSignal = 0;     /* BIT0: DTR(Data Terminal Ready) , BIT1: RTS(Request To Send) */

static uint32_t volatile g_u32VCPOutToggle = 0;

static int32_t volatile g_i32MSCConnectCheck = 0;
static int32_t volatile g_i32MSCMaxLun;

/*-------------------------------------------------------------------------------*/
/* Functions for VCP*/


#define MAX_VCP_SEND_BUF_LEN	1024
static uint8_t s_au8VCPSendBuf[DCACHE_ALIGN_LINE_SIZE(MAX_VCP_SEND_BUF_LEN)]__attribute__((aligned(DCACHE_LINE_SIZE)));
static volatile uint32_t s_u32VCPSendBufIn = 0;
static volatile uint32_t s_u32VCPSendBufOut = 0;
static volatile int32_t s_b32VCPSendTrans = 0;


int32_t VCPTrans_BulkInHandler()
{
    int32_t i32Len;
    uint8_t *pu8EPBufAddr;

    if((s_b32VCPSendTrans) && (s_u32VCPSendBufIn == s_u32VCPSendBufOut)) {
        s_b32VCPSendTrans = 0;
        return 0;
    }

    if(s_u32VCPSendBufOut > s_u32VCPSendBufIn) {
        s_u32VCPSendBufOut = s_u32VCPSendBufIn;
        s_b32VCPSendTrans = 0;
        return 0;
    }

    i32Len = s_u32VCPSendBufIn - s_u32VCPSendBufOut;

    if(i32Len > EP2_VCP_MAX_PKT_SIZE)
        i32Len = EP2_VCP_MAX_PKT_SIZE;

    if(i32Len == 0) {
        s_b32VCPSendTrans = 0;
        return 0;
    }

    s_b32VCPSendTrans = 1;
    pu8EPBufAddr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2));

#if 0 //(NVT_DCACHE_ON == 1)
    // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
    SCB_CleanDCache_by_Addr(s_au8VCPSendBuf + s_u32VCPSendBufOut, i32Len);
#endif
    //USBD_MemCopy is CPU memory copy operation(not DMA), so we don't need to care DCache coherence issue
    USBD_MemCopy(pu8EPBufAddr, (void *)s_au8VCPSendBuf + s_u32VCPSendBufOut, i32Len);
    USBD_SET_PAYLOAD_LEN(EP2, i32Len);

    s_u32VCPSendBufOut += i32Len;

    return i32Len;
}


int32_t VCPTrans_BulkInSend(
    uint8_t *pu8DataBuf,
    uint32_t u32DataBufLen
)
{
    uint32_t u32MaxLen;

    u32MaxLen = u32DataBufLen;

    if(s_b32VCPSendTrans == 0) {
        s_u32VCPSendBufIn = 0;
        s_u32VCPSendBufOut = 0;

        if(u32MaxLen > MAX_VCP_SEND_BUF_LEN)
            u32MaxLen = MAX_VCP_SEND_BUF_LEN;
    } else {
        if(u32MaxLen > (MAX_VCP_SEND_BUF_LEN - s_u32VCPSendBufIn))
            u32MaxLen = MAX_VCP_SEND_BUF_LEN - s_u32VCPSendBufIn;
    }

    memcpy(s_au8VCPSendBuf + s_u32VCPSendBufIn, pu8DataBuf, u32MaxLen);
    s_u32VCPSendBufIn += u32MaxLen;

    if(s_b32VCPSendTrans == 0) {
        VCPTrans_BulkInHandler();
    }

    return u32MaxLen;
}

int32_t VCPTrans_BulkInCanSend()
{
    if(s_u32VCPSendBufIn < MAX_VCP_SEND_BUF_LEN)
        return 1;
    return !s_b32VCPSendTrans;
}



#define MAX_VCP_RECV_BUF_LEN (EP3_VCP_MAX_PKT_SIZE * 3)
static uint8_t s_au8VCPRecvBuf[DCACHE_ALIGN_LINE_SIZE(MAX_VCP_RECV_BUF_LEN)]__attribute__((aligned(DCACHE_LINE_SIZE)));
static volatile uint32_t s_u32VCPRecvBufIn = 0;
static volatile uint32_t s_u32VCPRecvBufOut = 0;

extern int mp_interrupt_char;

void VCPTrans_RegisterSingal(
    PFN_USBDEV_VCPRecvSignal pfnSignal
)
{
    s_pfnVCPRecvSignel = pfnSignal;
}

int32_t VCPTrans_BulkOutHandler(
    uint8_t *pu8EPBuf,
    uint32_t u32Size
)
{
    int32_t i32BufFreeLen;
    uint32_t u32LimitLen;
    uint32_t u32NewInIdx = s_u32VCPRecvBufIn;
    uint32_t u32OutIdx = s_u32VCPRecvBufOut;

    if(u32NewInIdx >= u32OutIdx) {
        i32BufFreeLen = MAX_VCP_RECV_BUF_LEN - u32NewInIdx +  u32OutIdx;
        u32LimitLen = MAX_VCP_RECV_BUF_LEN - u32NewInIdx;
    } else {
        i32BufFreeLen = u32OutIdx - u32NewInIdx;
        u32LimitLen = i32BufFreeLen;
    }

    if(i32BufFreeLen < u32Size) {
        printf("USBD receive buffer size overflow, lost packet \n");
        return 0;
    }

    if(s_pfnVCPRecvSignel) {
        if(s_pfnVCPRecvSignel(pu8EPBuf, u32Size) == 0) {
            USBD_SET_PAYLOAD_LEN(EP3, EP3_VCP_MAX_PKT_SIZE);
            return 0;
        }
    }

    if(u32LimitLen < u32Size) {
        USBD_MemCopy(s_au8VCPRecvBuf + u32NewInIdx, pu8EPBuf, u32LimitLen);

#if 0 //(NVT_DCACHE_ON == 1)
        // Invalidate the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
        SCB_InvalidateDCache_by_Addr(s_au8VCPRecvBuf + u32NewInIdx, u32LimitLen);
#endif

        USBD_MemCopy(s_au8VCPRecvBuf, pu8EPBuf + u32LimitLen, u32Size - u32LimitLen);

#if 0 //(NVT_DCACHE_ON == 1)
        // Invalidate the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
        SCB_InvalidateDCache_by_Addr(s_au8VCPRecvBuf, u32Size - u32LimitLen);
#endif

    } else {
        USBD_MemCopy(s_au8VCPRecvBuf + u32NewInIdx, pu8EPBuf, u32Size);
#if 0 //(NVT_DCACHE_ON == 1)
        // Invalidate the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
        if((u32NewInIdx % DCACHE_LINE_SIZE) == 0)
            SCB_InvalidateDCache_by_Addr((s_au8VCPRecvBuf + u32NewInIdx), u32Size);
        else
            printf("u32NewInIdx %d \n", u32NewInIdx);
#endif
    }


    u32NewInIdx = (u32NewInIdx + u32Size) % MAX_VCP_RECV_BUF_LEN;
    s_u32VCPRecvBufIn = u32NewInIdx;
//	USBD_SET_PAYLOAD_LEN(EP3, EP3_VCP_MAX_PKT_SIZE); //TODO:delete it! It will be enabled if application read it

    return u32Size;
}

int32_t VCPTrans_BulkOutCanRecv()
{
    uint32_t u32InIdx;
    uint32_t u32OutIdx;
    int i32RecvLen = 0;

    u32InIdx = s_u32VCPRecvBufIn;
    u32OutIdx = s_u32VCPRecvBufOut;

    if(u32InIdx >= u32OutIdx) {
        i32RecvLen = u32InIdx - u32OutIdx;
    } else {
        i32RecvLen = MAX_VCP_RECV_BUF_LEN - u32OutIdx +  u32InIdx;
    }

    return i32RecvLen;
}

int32_t VCPTrans_BulkOutRecv(
    uint8_t *pu8DataBuf,
    uint32_t u32DataBufLen
)
{
    int i32CopyLen = 0;
    uint32_t u32InIdx;
    uint32_t u32NewOutIdx;
    uint32_t u32LimitLen;

    if(pu8DataBuf == NULL)
        return 0;

    u32InIdx = s_u32VCPRecvBufIn;
    u32NewOutIdx = s_u32VCPRecvBufOut;

    if(u32InIdx >= u32NewOutIdx) {
        i32CopyLen = u32InIdx - u32NewOutIdx;
        u32LimitLen = i32CopyLen;
    } else {
        i32CopyLen = MAX_VCP_RECV_BUF_LEN - u32NewOutIdx +  u32InIdx;
        u32LimitLen = MAX_VCP_RECV_BUF_LEN - u32NewOutIdx;
    }

    if(i32CopyLen > u32DataBufLen) {
        i32CopyLen = u32DataBufLen;
        u32LimitLen = i32CopyLen;
    }

    if(i32CopyLen) {

        if((i32CopyLen + u32NewOutIdx) > MAX_VCP_RECV_BUF_LEN) {
            memcpy(pu8DataBuf, s_au8VCPRecvBuf + u32NewOutIdx, u32LimitLen);
            memcpy(pu8DataBuf + u32LimitLen, s_au8VCPRecvBuf, i32CopyLen - u32LimitLen);
        } else {
            memcpy(pu8DataBuf, s_au8VCPRecvBuf + u32NewOutIdx, i32CopyLen);
        }

        u32NewOutIdx = (u32NewOutIdx + i32CopyLen) % MAX_VCP_RECV_BUF_LEN;
        s_u32VCPRecvBufOut = u32NewOutIdx;
        USBD_SET_PAYLOAD_LEN(EP3, EP3_VCP_MAX_PKT_SIZE);
    }

    return i32CopyLen;
}


/*-------------------------------------------------------------------------------*/
/* Functions for MSC*/

static void MSC_ReadMedia(uint32_t u32Addr, uint32_t u32Size, uint8_t *pu8Buffer)
{
    uint32_t u32SectorAlignAddr;
    uint32_t u32Offset;
    uint8_t *pu8TempBuf = NULL;
    int32_t i32EachReadLen;

    S_STORIF_IF *psStorIF = s_apsLUNStorIf[g_sCBW.bCBWLUN];
    S_STORIF_INFO sStorInfo;

//	printf("MSC_ReadMedia enter address %x , size %d \n", u32Addr, u32Size);
    psStorIF->pfnGetInfo(&sStorInfo, psStorIF->pvStorPriv);
    u32SectorAlignAddr = u32Addr & (~(sStorInfo.u32SectorSize - 1));

    /* read none sector-aligned data */
    if(u32SectorAlignAddr != u32Addr) {
        /* Get the sector offset*/
        u32Offset = (u32Addr & (sStorInfo.u32SectorSize - 1));

        pu8TempBuf = malloc(sStorInfo.u32SectorSize);
        if(pu8TempBuf == NULL)
            return;

        psStorIF->pfnReadSector(pu8TempBuf, u32SectorAlignAddr / sStorInfo.u32SectorSize, 1, psStorIF->pvStorPriv);
        i32EachReadLen = sStorInfo.u32SectorSize - u32Offset;

        if(i32EachReadLen > u32Size)
            i32EachReadLen = u32Size;

        memcpy(pu8Buffer, pu8TempBuf + u32Offset, i32EachReadLen);

        u32Addr += i32EachReadLen;
        pu8Buffer += i32EachReadLen;
        u32Size -= i32EachReadLen;
    }

    /*read sector aligned data */
    i32EachReadLen = u32Size & (~(sStorInfo.u32SectorSize - 1));

    if(i32EachReadLen > 0) {
        psStorIF->pfnReadSector(pu8Buffer, u32Addr / sStorInfo.u32SectorSize, i32EachReadLen / sStorInfo.u32SectorSize, psStorIF->pvStorPriv);

        u32Addr += i32EachReadLen;
        pu8Buffer += i32EachReadLen;
        u32Size -= i32EachReadLen;
    }

    /*read remain data */
    if(u32Size > 0) {
        if(pu8TempBuf == NULL) {
            pu8TempBuf = malloc(sStorInfo.u32SectorSize);
            if(pu8TempBuf == NULL)
                return;
        }

        psStorIF->pfnReadSector(pu8TempBuf, u32Addr / sStorInfo.u32SectorSize, 1, psStorIF->pvStorPriv);
        memcpy(pu8Buffer, pu8TempBuf, u32Size);
    }

    if(pu8TempBuf)
        free(pu8TempBuf);

//	printf("MSC_ReadMedia leave \n");
}

static void MSC_Read(void)
{
    uint32_t u32Len;

//	printf("MSC_Read \n");

    if (USBD_GET_EP_BUF_ADDR(EP5) == g_u32BulkBuf1)
        USBD_SET_EP_BUF_ADDR(EP5, g_u32BulkBuf0);
    else
        USBD_SET_EP_BUF_ADDR(EP5, g_u32BulkBuf1);

    /* Trigger to send out the data packet */
    USBD_SET_PAYLOAD_LEN(EP5, g_u8Size);

    g_u32Length -= g_u8Size;
    g_u32BytesInStorageBuf -= g_u8Size;

    if (g_u32Length) {
        if (g_u32BytesInStorageBuf) {
            /* Prepare next data packet */
            g_u8Size = EP5_MSC_MAX_PKT_SIZE;
            if (g_u8Size > g_u32Length)
                g_u8Size = g_u32Length;

#if 0 //(NVT_DCACHE_ON == 1)
            // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
            SCB_CleanDCache_by_Addr((void *)g_u32Address, g_u8Size);
#endif

            if (USBD_GET_EP_BUF_ADDR(EP5) == g_u32BulkBuf1)
                USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf0), (uint8_t *)g_u32Address, g_u8Size);
            else
                USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)g_u32Address, g_u8Size);
            g_u32Address += g_u8Size;
        } else {
            u32Len = g_u32Length;
            if (u32Len > STORAGE_BUFFER_SIZE)
                u32Len = STORAGE_BUFFER_SIZE;

            MSC_ReadMedia(g_u32LbaAddress, u32Len, (uint8_t *)STORAGE_DATA_BUF);
            g_u32BytesInStorageBuf = u32Len;
            g_u32LbaAddress += u32Len;
            g_u32Address = STORAGE_DATA_BUF;

            /* Prepare next data packet */
            g_u8Size = EP5_MSC_MAX_PKT_SIZE;
            if (g_u8Size > g_u32Length)
                g_u8Size = g_u32Length;

#if 0 //(NVT_DCACHE_ON == 1)
            // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
            SCB_CleanDCache_by_Addr((void *)g_u32Address, g_u8Size);
#endif

            if (USBD_GET_EP_BUF_ADDR(EP5) == g_u32BulkBuf1)
                USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf0), (uint8_t *)g_u32Address, g_u8Size);
            else
                USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)g_u32Address, g_u8Size);
            g_u32Address += g_u8Size;
        }
    }
}

static void MSC_ReadTrig(void)
{
    uint32_t u32Len;

//	printf("MSC_ReadTrig \n");
    if (g_u32Length) {
        if (g_u32BytesInStorageBuf) {
            /* Prepare next data packet */
            g_u8Size = EP5_MSC_MAX_PKT_SIZE;
            if (g_u8Size > g_u32Length)
                g_u8Size = g_u32Length;

#if 0 //(NVT_DCACHE_ON == 1)
            // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
            SCB_CleanDCache_by_Addr((void *)g_u32Address, g_u8Size);
#endif


            if (USBD_GET_EP_BUF_ADDR(EP5) == g_u32BulkBuf1)
                USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf0), (uint8_t *)g_u32Address, g_u8Size);
            else
                USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)g_u32Address, g_u8Size);
            g_u32Address += g_u8Size;
        } else {
            u32Len = g_u32Length;
            if (u32Len > STORAGE_BUFFER_SIZE)
                u32Len = STORAGE_BUFFER_SIZE;

            MSC_ReadMedia(g_u32LbaAddress, u32Len, (uint8_t *)STORAGE_DATA_BUF);
            g_u32BytesInStorageBuf = u32Len;
            g_u32LbaAddress += u32Len;
            g_u32Address = STORAGE_DATA_BUF;

            /* Prepare next data packet */
            g_u8Size = EP5_MSC_MAX_PKT_SIZE;
            if (g_u8Size > g_u32Length)
                g_u8Size = g_u32Length;

#if 0 //(NVT_DCACHE_ON == 1)
            // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
            SCB_CleanDCache_by_Addr((void *)g_u32Address, g_u8Size);
#endif

            if (USBD_GET_EP_BUF_ADDR(EP5) == g_u32BulkBuf1)
                USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf0), (uint8_t *)g_u32Address, g_u8Size);
            else
                USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)g_u32Address, g_u8Size);
            g_u32Address += g_u8Size;
        }

        /* DATA0/DATA1 Toggle */
        if (USBD_GET_EP_BUF_ADDR(EP5) == g_u32BulkBuf1)
            USBD_SET_EP_BUF_ADDR(EP5, g_u32BulkBuf0);
        else
            USBD_SET_EP_BUF_ADDR(EP5, g_u32BulkBuf1);

        /* Trigger to send out the data packet */
        USBD_SET_PAYLOAD_LEN(EP5, g_u8Size);

        g_u32Length -= g_u8Size;
        g_u32BytesInStorageBuf -= g_u8Size;
    } else
        USBD_SET_PAYLOAD_LEN(EP5, 0);
}


static void MSC_AckCmd(void)
{
    /* Bulk IN */
    if (g_u8BulkState == BULK_CSW) {
        /* Prepare to receive the CBW */
        g_u8BulkState = BULK_CBW;

        USBD_SET_EP_BUF_ADDR(EP6, g_u32BulkBuf0);
        USBD_SET_PAYLOAD_LEN(EP6, 31);
    } else if (g_u8BulkState == BULK_IN) {
        switch (g_sCBW.u8OPCode) {
        case UFI_READ_12:
        case UFI_READ_10: {
            if (g_u32Length > 0) {
//				printf("BBBBBBBBBBBBB MSC_AckCmd MSC_ReadTrig enter\n");
                MSC_ReadTrig();
//				printf("BBBBBBBBBBBBB MSC_AckCmd MSC_ReadTrig leave\n");
                return;
            }
            break;
        }
        case UFI_READ_FORMAT_CAPACITY:
        case UFI_READ_CAPACITY:
        case UFI_MODE_SENSE_10: {
            if (g_u32Length > 0) {
                MSC_Read();
                return;
            }
            g_sCSW.dCSWDataResidue = 0;
            g_sCSW.bCSWStatus = 0;
            break;
        }

        case UFI_WRITE_12:
        case UFI_WRITE_10:
            break;
        case UFI_PREVENT_ALLOW_MEDIUM_REMOVAL:
        case UFI_VERIFY_10:
        case UFI_START_STOP: {
            int32_t tmp;

            tmp = g_sCBW.dCBWDataTransferLength - STORAGE_BUFFER_SIZE;
            if (tmp < 0)
                tmp = 0;

            g_sCSW.dCSWDataResidue = tmp;
            g_sCSW.bCSWStatus = 0;
            break;
        }
        case UFI_INQUIRY:
        case UFI_MODE_SENSE_6:
        case UFI_REQUEST_SENSE:
        case UFI_TEST_UNIT_READY: {
            break;
        }
        default: {
            /* Unsupported command. Return command fail status */
            g_sCSW.dCSWDataResidue = g_sCBW.dCBWDataTransferLength;
            g_sCSW.bCSWStatus = 0x01;
            break;
        }
        }

        /* Return the CSW */
        USBD_SET_EP_BUF_ADDR(EP5, g_u32BulkBuf1);

        /* Bulk IN buffer */

#if 0 //(NVT_DCACHE_ON == 1)
        // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
        memcpy(DCacheTempBuf, &g_sCSW.dCSWSignature, 16);
        SCB_CleanDCache_by_Addr(DCacheTempBuf, 16);
        USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)DCacheTempBuf, 16);
#else
        USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)&g_sCSW.dCSWSignature, 16);
#endif

        g_u8BulkState = BULK_CSW;
        USBD_SET_PAYLOAD_LEN(EP5, 13);
    }
}

static void MSC_WriteMedia(uint32_t u32Addr, uint32_t u32Size, uint8_t *pu8Buffer)
{
    uint32_t u32SectorAlignAddr;
    uint32_t u32Offset;
    uint8_t *pu8TempBuf = NULL;
    int32_t i32EachWriteLen;

    S_STORIF_IF *psStorIF = s_apsLUNStorIf[g_sCBW.bCBWLUN];
    S_STORIF_INFO sStorInfo;

    psStorIF->pfnGetInfo(&sStorInfo, psStorIF->pvStorPriv);
    u32SectorAlignAddr = u32Addr & (~(sStorInfo.u32SectorSize - 1));
//	printf("MSC_WriteMedia enter %x , size %d \n", u32Addr, u32Size);

    /* write none sector-aligned data */
    if(u32SectorAlignAddr != u32Addr) {
        /* Get the sector offset*/
        u32Offset = (u32Addr & (sStorInfo.u32SectorSize - 1));

        pu8TempBuf = malloc(sStorInfo.u32SectorSize);
        if(pu8TempBuf == NULL)
            return;

        psStorIF->pfnReadSector(pu8TempBuf, u32SectorAlignAddr / sStorInfo.u32SectorSize, 1, psStorIF->pvStorPriv);
        i32EachWriteLen = sStorInfo.u32SectorSize - u32Offset;

        if(i32EachWriteLen > u32Size)
            i32EachWriteLen = u32Size;

        memcpy(pu8TempBuf + u32Offset, pu8Buffer, i32EachWriteLen);
        psStorIF->pfnWriteSector(pu8TempBuf, u32SectorAlignAddr / sStorInfo.u32SectorSize, 1, psStorIF->pvStorPriv);

        u32Addr += i32EachWriteLen;
        pu8Buffer += i32EachWriteLen;
        u32Size -= i32EachWriteLen;
    }

    /*write sector aligned data */
    i32EachWriteLen = u32Size & (~(sStorInfo.u32SectorSize - 1));

    if(i32EachWriteLen > 0) {
        psStorIF->pfnWriteSector(pu8Buffer, u32Addr / sStorInfo.u32SectorSize, i32EachWriteLen / sStorInfo.u32SectorSize, psStorIF->pvStorPriv);

        u32Addr += i32EachWriteLen;
        pu8Buffer += i32EachWriteLen;
        u32Size -= i32EachWriteLen;
    }

    /*write remain data */
    if(u32Size > 0) {
        if(pu8TempBuf == NULL) {
            pu8TempBuf = malloc(sStorInfo.u32SectorSize);
            if(pu8TempBuf == NULL)
                return;
        }

        psStorIF->pfnReadSector(pu8TempBuf, u32Addr / sStorInfo.u32SectorSize, 1, psStorIF->pvStorPriv);
        memcpy(pu8TempBuf, pu8Buffer, u32Size);
        psStorIF->pfnWriteSector(pu8TempBuf, u32Addr / sStorInfo.u32SectorSize, 1, psStorIF->pvStorPriv);
    }

//	printf("MSC_WriteMedia leave \n");

    if(pu8TempBuf)
        free(pu8TempBuf);
}


static void MSC_Write(void)
{
    uint32_t lba, len;

    if (g_u32MSCOutSkip == 0) {
        if (g_u32Length > EP6_MSC_MAX_PKT_SIZE) {
            if (USBD_GET_EP_BUF_ADDR(EP6) == g_u32BulkBuf0) {
                USBD_SET_EP_BUF_ADDR(EP6, g_u32BulkBuf1);
                USBD_SET_PAYLOAD_LEN(EP6, EP6_MSC_MAX_PKT_SIZE);
                USBD_MemCopy((uint8_t *)g_u32Address, (uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf0), EP6_MSC_MAX_PKT_SIZE);
            } else {
                USBD_SET_EP_BUF_ADDR(EP6, g_u32BulkBuf0);
                USBD_SET_PAYLOAD_LEN(EP6, EP6_MSC_MAX_PKT_SIZE);
                USBD_MemCopy((uint8_t *)g_u32Address, (uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), EP6_MSC_MAX_PKT_SIZE);
            }

#if 0 //(NVT_DCACHE_ON == 1)
            // Invalidate the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
            SCB_InvalidateDCache_by_Addr((void *)g_u32Address, EP6_MSC_MAX_PKT_SIZE);
#endif

            g_u32Address += EP6_MSC_MAX_PKT_SIZE;
            g_u32Length -= EP6_MSC_MAX_PKT_SIZE;

            /* Buffer full. Writer it to storage first. */
            if (g_u32Address >= (STORAGE_DATA_BUF + STORAGE_BUFFER_SIZE)) {
                MSC_WriteMedia(g_u32DataFlashStartAddr, STORAGE_BUFFER_SIZE, (uint8_t *)STORAGE_DATA_BUF);

                g_u32Address = STORAGE_DATA_BUF;
                g_u32DataFlashStartAddr += STORAGE_BUFFER_SIZE;
            }
        } else {
            if (USBD_GET_EP_BUF_ADDR(EP6) == g_u32BulkBuf0)
                USBD_MemCopy((uint8_t *)g_u32Address, (uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf0), g_u32Length);
            else
                USBD_MemCopy((uint8_t *)g_u32Address, (uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), g_u32Length);

#if 0 //(NVT_DCACHE_ON == 1)
            // Invalidate the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
            SCB_InvalidateDCache_by_Addr((void *)g_u32Address, g_u32Length);
#endif

            g_u32Address += g_u32Length;
            g_u32Length = 0;


            if ((g_sCBW.u8OPCode == UFI_WRITE_10) || (g_sCBW.u8OPCode == UFI_WRITE_12)) {
                lba = get_be32(&g_sCBW.au8Data[0]);
                len = g_sCBW.dCBWDataTransferLength;

                len = lba * UDC_SECTOR_SIZE + g_sCBW.dCBWDataTransferLength - g_u32DataFlashStartAddr;
                if (len)
                    MSC_WriteMedia(g_u32DataFlashStartAddr, len, (uint8_t *)STORAGE_DATA_BUF);
            }

            g_u8BulkState = BULK_IN;
            MSC_AckCmd();
        }
    }
}


/* Mass Storage class request */
static void MSC_RequestSense(void)
{
    uint8_t tmp[20];

    memset(tmp, 0, 18);
    if (g_u8Prevent) {
        g_u8Prevent = 0;
        tmp[0]= 0x70;
    } else
        tmp[0] = 0xf0;

    tmp[2] = g_au8SenseKey[0];
    tmp[7] = 0x0a;
    tmp[12] = g_au8SenseKey[1];
    tmp[13] = g_au8SenseKey[2];

#if 0 //(NVT_DCACHE_ON == 1)
    // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
    memcpy(DCacheTempBuf, tmp, 20);
    SCB_CleanDCache_by_Addr(DCacheTempBuf, 20);
    USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP5)), DCacheTempBuf, 20);
#else

    USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP5)), tmp, 20);
#endif

    g_au8SenseKey[0] = 0;
    g_au8SenseKey[1] = 0;
    g_au8SenseKey[2] = 0;
}

static void MSC_ReadFormatCapacity(void)
{
    uint8_t *pu8Desc;
    S_STORIF_IF *psStorIF = s_apsLUNStorIf[g_sCBW.bCBWLUN];
    S_STORIF_INFO sStorInfo;
    uint32_t u32TotalSector;

    psStorIF->pfnGetInfo(&sStorInfo, psStorIF->pvStorPriv);
    u32TotalSector = sStorInfo.u32DiskSize * (1024 / UDC_SECTOR_SIZE);

    pu8Desc = (uint8_t *)MassCMD_BUF;
    memset(pu8Desc, 0, 36);

    /*---------- Capacity List Header ----------*/
    // Capacity List Length
    pu8Desc[3] = 0x10;

    /*---------- Current/Maximum Capacity Descriptor ----------*/
    // Number of blocks (MSB first)
    pu8Desc[4] = *((uint8_t *)&u32TotalSector+3);
    pu8Desc[5] = *((uint8_t *)&u32TotalSector+2);
    pu8Desc[6] = *((uint8_t *)&u32TotalSector+1);
    pu8Desc[7] = *((uint8_t *)&u32TotalSector+0);

    // Descriptor Code:
    // 01b = Unformatted Media - Maximum formattable capacity for this cartridge
    // 10b = Formatted Media - Current media capacity
    // 11b = No Cartridge in Drive - Maximum formattable capacity for any cartridge
    pu8Desc[8] = 0x02;


    // Block Length. Fixed to be 512 (MSB first)
    pu8Desc[ 9] = 0;
    pu8Desc[10] = 0x02;
    pu8Desc[11] = 0;

    /*---------- Formattable Capacity Descriptor ----------*/
    // Number of Blocks
    pu8Desc[12] = *((uint8_t *)&u32TotalSector+3);
    pu8Desc[13] = *((uint8_t *)&u32TotalSector+2);
    pu8Desc[14] = *((uint8_t *)&u32TotalSector+1);
    pu8Desc[15] = *((uint8_t *)&u32TotalSector+0);

    // Block Length. Fixed to be 512 (MSB first)
    pu8Desc[17] = 0;
    pu8Desc[18] = 0x02;
    pu8Desc[19] = 0;

}

static void MSC_ReadCapacity(void)
{
    uint32_t tmp;
    S_STORIF_IF *psStorIF = s_apsLUNStorIf[g_sCBW.bCBWLUN];
    S_STORIF_INFO sStorInfo;
    uint32_t u32TotalSector;

    psStorIF->pfnGetInfo(&sStorInfo, psStorIF->pvStorPriv);
    u32TotalSector = sStorInfo.u32DiskSize * (1024 / UDC_SECTOR_SIZE);

    memset((uint8_t *)MassCMD_BUF, 0, 36);

    tmp = u32TotalSector - 1;
    *((uint8_t *)(MassCMD_BUF+0)) = *((uint8_t *)&tmp+3);
    *((uint8_t *)(MassCMD_BUF+1)) = *((uint8_t *)&tmp+2);
    *((uint8_t *)(MassCMD_BUF+2)) = *((uint8_t *)&tmp+1);
    *((uint8_t *)(MassCMD_BUF+3)) = *((uint8_t *)&tmp+0);
    *((uint8_t *)(MassCMD_BUF+6)) = 0x02;
}

static void MSC_ModeSense10(void)
{
    uint8_t i,j;
    uint8_t NumHead,NumSector;
    uint16_t NumCyl=0;

    S_STORIF_IF *psStorIF = s_apsLUNStorIf[g_sCBW.bCBWLUN];
    S_STORIF_INFO sStorInfo;
    uint32_t u32TotalSector;

    psStorIF->pfnGetInfo(&sStorInfo, psStorIF->pvStorPriv);
    u32TotalSector = sStorInfo.u32DiskSize * (1024 / UDC_SECTOR_SIZE);

    /* Clear the command buffer */
    *((uint32_t *)MassCMD_BUF  ) = 0;
    *((uint32_t *)MassCMD_BUF + 1) = 0;

    //For write protect access
    if(s_bMSCWriteProtect)
        *((uint8_t *)(MassCMD_BUF + 2)) = 0x80;

    switch (g_sCBW.au8Data[0]) {
    case 0x01:
        *((uint8_t *)MassCMD_BUF) = 19;
        i = 8;
        for (j = 0; j<12; j++, i++)
            *((uint8_t *)(MassCMD_BUF+i)) = g_au8ModePage_01[j];
        break;

    case 0x05:
        *((uint8_t *)MassCMD_BUF) = 39;
        i = 8;
        for (j = 0; j<32; j++, i++)
            *((uint8_t *)(MassCMD_BUF+i)) = g_au8ModePage_05[j];

        NumHead = 2;
        NumSector = 64;
        NumCyl = u32TotalSector / 128;

        *((uint8_t *)(MassCMD_BUF+12)) = NumHead;
        *((uint8_t *)(MassCMD_BUF+13)) = NumSector;
        *((uint8_t *)(MassCMD_BUF+16)) = (uint8_t)(NumCyl >> 8);
        *((uint8_t *)(MassCMD_BUF+17)) = (uint8_t)(NumCyl & 0x00ff);
        break;

    case 0x1B:
        *((uint8_t *)MassCMD_BUF) = 19;
        i = 8;
        for (j = 0; j<12; j++, i++)
            *((uint8_t *)(MassCMD_BUF+i)) = g_au8ModePage_1B[j];
        break;

    case 0x1C:
        *((uint8_t *)MassCMD_BUF) = 15;
        i = 8;
        for (j = 0; j<8; j++, i++)
            *((uint8_t *)(MassCMD_BUF+i)) = g_au8ModePage_1C[j];
        break;

    case 0x3F:
        *((uint8_t *)MassCMD_BUF) = 0x47;
        i = 8;
        for (j = 0; j<12; j++, i++)
            *((uint8_t *)(MassCMD_BUF+i)) = g_au8ModePage_01[j];
        for (j = 0; j<32; j++, i++)
            *((uint8_t *)(MassCMD_BUF+i)) = g_au8ModePage_05[j];
        for (j = 0; j<12; j++, i++)
            *((uint8_t *)(MassCMD_BUF+i)) = g_au8ModePage_1B[j];
        for (j = 0; j<8; j++, i++)
            *((uint8_t *)(MassCMD_BUF+i)) = g_au8ModePage_1C[j];

        NumHead = 2;
        NumSector = 64;
        NumCyl = u32TotalSector / 128;

        *((uint8_t *)(MassCMD_BUF+24)) = NumHead;
        *((uint8_t *)(MassCMD_BUF+25)) = NumSector;
        *((uint8_t *)(MassCMD_BUF+28)) = (uint8_t)(NumCyl >> 8);
        *((uint8_t *)(MassCMD_BUF+29)) = (uint8_t)(NumCyl & 0x00ff);
        break;

    default:
        g_au8SenseKey[0] = 0x05;
        g_au8SenseKey[1] = 0x24;
        g_au8SenseKey[2] = 0x00;
    }
}


void MSCTrans_BulkInHandler(void)
{

    g_u8EP5Ready = 1;

//    MSC_AckCmd();
}


void MSCTrans_BulkOutHandler(void)
{
    /* Bulk OUT */
    if (g_u32MSCOutToggle == (USBD->EPSTS0 & 0xf000000)) {
        g_u32MSCOutSkip = 1;
        USBD_SET_PAYLOAD_LEN(EP6, EP6_MSC_MAX_PKT_SIZE);
    } else {
        g_u8EP6Ready = 1;
        g_u32MSCOutToggle = USBD->EPSTS0 & 0xf000000;
        g_u32MSCOutSkip = 0;
    }
}

void MSCVCPTrans_ClassRequest(void)
{
    uint8_t buf[8];

    USBD_GetSetupPacket(buf);

    if (buf[0] & 0x80) { /* request data transfer direction */
        // Device to host
        switch (buf[1]) {
        case GET_LINE_CODE: {
            if (buf[4] == 0) { /* VCOM-1 */
#if 0 //(NVT_DCACHE_ON == 1)
                // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
                memcpy(DCacheTempBuf, &gLineCoding, 7);
                SCB_CleanDCache_by_Addr(DCacheTempBuf, 7);
                USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)), DCacheTempBuf, 7);
#else
                USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)), (uint8_t *)&gLineCoding, 7);
#endif
            }
            /* Data stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 7);
            /* Status stage */
            USBD_PrepareCtrlOut(0,0);
            break;
        }
        case GET_MAX_LUN: {
            uint8_t u8MaxLUN = 0;

#if MICROPY_HW_HAS_FLASH
            if(u8MaxLUN < DEF_MAX_LUN) {
                s_apsLUNStorIf[u8MaxLUN] = (S_STORIF_IF *)&g_STORIF_sFlash;
                u8MaxLUN ++;
            }
#endif

#if MICROPY_HW_HAS_SPIFLASH
            if(u8MaxLUN < DEF_MAX_LUN) {
                s_apsLUNStorIf[u8MaxLUN] = (S_STORIF_IF *)&g_STORIF_sSPIFlash;
                u8MaxLUN ++;
            }
#endif

#if MICROPY_HW_HAS_SDCARD
            if(u8MaxLUN < DEF_MAX_LUN) {
                NVIC_SetPriority(USBD_IRQn, 1); // Because of USBD interrupt will make SDH read abnormal, so set USBD interrupt priority lower than SDH.
                s_apsLUNStorIf[u8MaxLUN] = (S_STORIF_IF *)&g_STORIF_sSDCard;
                u8MaxLUN ++;
            }
#endif

            g_i32MSCMaxLun = u8MaxLUN;
            g_i32MSCConnectCheck = g_i32MSCMaxLun * 3;

            /* Check interface number with cfg descriptor wIndex = interface number, check wValue = 0, wLength = 1 */
            if ((((buf[3]<<8)+buf[2]) == 0) && (((buf[5]<<8)+buf[4]) == 2) && (((buf[7]<<8)+buf[6]) == 1)) {
                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = (u8MaxLUN - 1);
                /* Data stage */
                USBD_SET_DATA1(EP0);
                USBD_SET_PAYLOAD_LEN(EP0, 1);
                /* Status stage */
                USBD_PrepareCtrlOut(0,0);
            } else { /* Invalid Get MaxLun command */
                USBD_SetStall(0);
            }
            break;
        }
        default: {
            /* Setup error, stall the device */
            USBD_SetStall(0);
            break;
        }
        }
    } else {
        // Host to device
        switch (buf[1]) {
        case SET_CONTROL_LINE_STATE: {
            if (buf[4] == 0) { /* VCOM-1 */
                gCtrlSignal = buf[3];
                gCtrlSignal = (gCtrlSignal << 8) | buf[2];
                //printf("RTS=%d  DTR=%d\n", (gCtrlSignal >> 1) & 1, gCtrlSignal & 1);
            }

            /* Status stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 0);
            break;
        }
        case SET_LINE_CODE: {
            if (buf[4] == 0) /* VCOM-1 */
                USBD_PrepareCtrlOut((uint8_t *)&gLineCoding, 7);

            /* Status stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 0);
            g_u32VCPConnect = 1;
            break;
        }
        case BULK_ONLY_MASS_STORAGE_RESET: {
            /* Check interface number with cfg descriptor and check wValue = 0, wLength = 0 */
            if((buf[4] == s_psUSBDevInfo->gu8ConfigDesc[LEN_CONFIG + 2]) && (buf[2] + buf[3] + buf[6] + buf[7] == 0)) {

                g_u32Length = 0; // Reset all read/write data transfer
                USBD_LockEpStall(0);

                /* Clear ready */
                USBD->EP[EP5].CFGP |= USBD_CFGP_CLRRDY_Msk;
                USBD->EP[EP6].CFGP |= USBD_CFGP_CLRRDY_Msk;

                /* Prepare to receive the CBW */
                g_u8EP5Ready = 0;
                g_u8EP6Ready = 0;
                g_u8BulkState = BULK_CBW;

                USBD_SET_DATA1(EP6);
                USBD_SET_EP_BUF_ADDR(EP6, g_u32BulkBuf0);
                USBD_SET_PAYLOAD_LEN(EP6, EP6_MSC_MAX_PKT_SIZE);
            } else { /* Invalid Reset command */
                USBD_SET_EP_STALL(EP1);
            }
            /* Status stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 0);
            break;
        }
        default: {
            // Stall
            /* Setup error, stall the device */
            USBD_SetStall(0);
            break;
        }
        }
    }
}


void MSCTrans_ProcessCmd(void)
{
    uint8_t u8Len;
    int32_t i;
    uint32_t Hcount, Dcount;


    if (g_u8EP5Ready) {
        g_u8EP5Ready = 0;
        MSC_AckCmd();
    }

    if (g_u8EP6Ready) {
        g_u8EP6Ready = 0;
        if (g_u8BulkState == BULK_CBW) {
            u8Len = USBD_GET_PAYLOAD_LEN(EP6);

            /* Check Signature & length of CBW */
            /* Bulk Out buffer */
            if ((*(uint32_t *) ((uint32_t)USBD_BUF_BASE + g_u32BulkBuf0) != CBW_SIGNATURE) || (u8Len != 31)) {
                /* Invalid CBW */
                g_u8Prevent = 1;
                USBD_SET_EP_STALL(EP5);
                USBD_SET_EP_STALL(EP6);
                USBD_LockEpStall((1 << EP5) | (1 << EP6));
                return;
            }

            /* Get the CBW */
            for (i = 0; i < u8Len; i++)
                *((uint8_t *) (&g_sCBW.dCBWSignature) + i) = *(uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf0 + i);

            /* Prepare to echo the tag from CBW to CSW */
            g_sCSW.dCSWTag = g_sCBW.dCBWTag;
            Hcount = g_sCBW.dCBWDataTransferLength;

            /* Parse Op-Code of CBW */
            switch (g_sCBW.u8OPCode) {
            case UFI_PREVENT_ALLOW_MEDIUM_REMOVAL: {
                if (g_sCBW.au8Data[2] & 0x01) {
                    g_au8SenseKey[0] = 0x05;  //INVALID COMMAND
                    g_au8SenseKey[1] = 0x24;
                    g_au8SenseKey[2] = 0;
                    g_u8Prevent = 1;
                } else
                    g_u8Prevent = 0;
                g_u8BulkState = BULK_IN;
                MSC_AckCmd();
                return;
            }
            case UFI_TEST_UNIT_READY: {
                if(g_i32MSCConnectCheck > 0)
                    g_i32MSCConnectCheck --;
                else
                    g_u32MSCConnect = 1;

                if (Hcount != 0) {
                    if (g_sCBW.bmCBWFlags == 0) {   /* Ho > Dn (Case 9) */
                        g_u8Prevent = 1;
                        USBD_SET_EP_STALL(EP6);
                        g_sCSW.bCSWStatus = 0x1;
                        g_sCSW.dCSWDataResidue = Hcount;
                    }
                } else { /* Hn == Dn (Case 1) */
                    S_STORIF_IF *psStorIF = s_apsLUNStorIf[g_sCBW.bCBWLUN];
                    if((!psStorIF->pfnDetect(psStorIF->pvStorPriv)) || (g_u8MSCRemove)) {
                        g_sCSW.dCSWDataResidue = 0;
                        g_sCSW.bCSWStatus = 1;
                        g_au8SenseKey[0] = 0x02;    /* Not ready */
                        g_au8SenseKey[1] = 0x3A;
                        g_au8SenseKey[2] = 0;
                        g_u8Prevent = 1;
                    } else {
                        g_sCSW.dCSWDataResidue = 0;
                        g_sCSW.bCSWStatus = 0;
                    }
                }
                g_u8BulkState = BULK_IN;
                MSC_AckCmd();
                return;
            }
            case UFI_START_STOP: {
                if ((g_sCBW.au8Data[2] & 0x03) == 0x2) {
                    g_u8MSCRemove = 1;
                }
                g_u8BulkState = BULK_IN;
                MSC_AckCmd();
                return;
            }
            case UFI_VERIFY_10: {
                g_u8BulkState = BULK_IN;
                MSC_AckCmd();
                return;
            }
            case UFI_REQUEST_SENSE: {
                /* Special case : Allocation Length is 24 on specific PC, it caused VCOM can't work.*/
                if ((Hcount > 0) && (Hcount <= 18))
                    //if (Hcount > 0)
                {
                    MSC_RequestSense();
                    /* CV Test must consider reserved bits.
                       Devices shall be capable of returning at least 18 bytes of data in response to a REQUEST SENSE command. */
                    USBD_SET_PAYLOAD_LEN(EP5, Hcount);
                    g_u8BulkState = BULK_IN;
                    g_sCSW.bCSWStatus = 0;
                    g_sCSW.dCSWDataResidue = 0;
                    return;
                } else {
                    USBD_SET_EP_STALL(EP5);
                    g_u8Prevent = 1;
                    g_sCSW.bCSWStatus = 0x01;
                    g_sCSW.dCSWDataResidue = 0;
                    g_u8BulkState = BULK_IN;
                    MSC_AckCmd();
                    return;
                }
            }
            case UFI_READ_FORMAT_CAPACITY: {
                if (g_u32Length == 0) {
                    g_u32Length = g_sCBW.dCBWDataTransferLength;
                    g_u32Address = MassCMD_BUF;
                }
                MSC_ReadFormatCapacity();
                g_u8BulkState = BULK_IN;
                if (g_u32Length > 0) {
                    if (g_u32Length > EP5_MSC_MAX_PKT_SIZE)
                        g_u8Size = EP5_MSC_MAX_PKT_SIZE;
                    else
                        g_u8Size = g_u32Length;

                    /* Bulk IN buffer */
#if 0 //(NVT_DCACHE_ON == 1)
                    // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
                    SCB_CleanDCache_by_Addr((void *)g_u32Address, g_u8Size);
#endif
                    USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)g_u32Address, g_u8Size);

                    g_u32Address += g_u8Size;
                    USBD_SET_EP_BUF_ADDR(EP5, g_u32BulkBuf0);
                    MSC_Read();
                }
                return;
            }
            case UFI_READ_CAPACITY: {
                if (g_u32Length == 0) {
                    g_u32Length = g_sCBW.dCBWDataTransferLength;
                    g_u32Address = MassCMD_BUF;
                }

                MSC_ReadCapacity();
                g_u8BulkState = BULK_IN;
                if (g_u32Length > 0) {
                    if (g_u32Length > EP5_MSC_MAX_PKT_SIZE)
                        g_u8Size = EP5_MSC_MAX_PKT_SIZE;
                    else
                        g_u8Size = g_u32Length;

                    /* Bulk IN buffer */
#if 0 //(NVT_DCACHE_ON == 1)
                    // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
                    SCB_CleanDCache_by_Addr((void *)g_u32Address, g_u8Size);
#endif
                    USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)g_u32Address, g_u8Size);

                    g_u32Address += g_u8Size;
                    USBD_SET_EP_BUF_ADDR(EP5, g_u32BulkBuf0);
                    MSC_Read();
                }
                return;
            }
            case UFI_MODE_SELECT_6:
            case UFI_MODE_SELECT_10: {
                g_u32Length = g_sCBW.dCBWDataTransferLength;
                g_u32Address = MassCMD_BUF;

                if (g_u32Length > 0) {
                    USBD_SET_PAYLOAD_LEN(EP6, EP6_MSC_MAX_PKT_SIZE);
                    g_u8BulkState = BULK_OUT;
                }
                return;
            }
            case UFI_MODE_SENSE_6: {
                *(uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1+0) = 0x3;
                *(uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1+1) = 0x0;

                //For write protect access
                if(s_bMSCWriteProtect)
                    *(uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1+2) = 0x80;
                else
                    *(uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1+2) = 0x0;

                *(uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1+3) = 0x0;

                USBD_SET_PAYLOAD_LEN(EP5, 4);
                g_u8BulkState = BULK_IN;
                g_sCSW.bCSWStatus = 0;
                g_sCSW.dCSWDataResidue = Hcount - 4;;
                return;
            }
            case UFI_MODE_SENSE_10: {
                if (g_u32Length == 0) {
                    g_u32Length = g_sCBW.dCBWDataTransferLength;
                    g_u32Address = MassCMD_BUF;
                }

                MSC_ModeSense10();
                g_u8BulkState = BULK_IN;
                if (g_u32Length > 0) {
                    if (g_u32Length > EP5_MSC_MAX_PKT_SIZE)
                        g_u8Size = EP5_MSC_MAX_PKT_SIZE;
                    else
                        g_u8Size = g_u32Length;
                    /* Bulk IN buffer */
#if 0 //(NVT_DCACHE_ON == 1)
                    // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
                    SCB_CleanDCache_by_Addr((void *)g_u32Address, g_u8Size);
#endif
                    USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)g_u32Address, g_u8Size);

                    g_u32Address += g_u8Size;

                    USBD_SET_EP_BUF_ADDR(EP5, g_u32BulkBuf0);
                    MSC_Read();
                }
                return;
            }
            case UFI_INQUIRY: {

                if ((Hcount > 0) && (Hcount <= 36)) {
                    /* Bulk IN buffer */
#if 0 //(NVT_DCACHE_ON == 1)
                    // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
                    memcpy(DCacheTempBuf, g_au8InquiryID, Hcount);
                    SCB_CleanDCache_by_Addr(DCacheTempBuf, Hcount);
                    USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), DCacheTempBuf, Hcount);
#else
                    USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)g_au8InquiryID, Hcount);
#endif
                    USBD_SET_PAYLOAD_LEN(EP5, Hcount);
                    g_u8BulkState = BULK_IN;
                    g_sCSW.bCSWStatus = 0;
                    g_sCSW.dCSWDataResidue = 0;
                    return;
                } else {
                    USBD_SET_EP_STALL(EP5);
                    g_u8Prevent = 1;
                    g_sCSW.bCSWStatus = 0x01;
                    g_sCSW.dCSWDataResidue = 0;
                    g_u8BulkState = BULK_IN;
                    MSC_AckCmd();
                    return;
                }
            }
            case UFI_READ_12:
            case UFI_READ_10: {
                g_i32MSCConnectCheck = g_i32MSCMaxLun * 3;

                /* Check if it is a new transfer */
                if(g_u32Length == 0) {

                    Dcount = (get_be32(&g_sCBW.au8Data[4])>>8) * 512;
                    if (g_sCBW.bmCBWFlags == 0x80) {    /* IN */
                        if (Hcount == Dcount) { /* Hi == Di (Case 6)*/
                        } else if (Hcount < Dcount) { /* Hn < Di (Case 2) || Hi < Di (Case 7) */
                            if (Hcount) {   /* Hi < Di (Case 7) */
                                g_u8Prevent = 1;
                                g_sCSW.bCSWStatus = 0x01;
                                g_sCSW.dCSWDataResidue = 0;
                            } else { /* Hn < Di (Case 2) */
                                g_u8Prevent = 1;
                                g_sCSW.bCSWStatus = 0x01;
                                g_sCSW.dCSWDataResidue = 0;
                                g_u8BulkState = BULK_IN;
                                MSC_AckCmd();
                                return;
                            }
                        } else if (Hcount > Dcount) { /* Hi > Dn (Case 4) || Hi > Di (Case 5) */
                            g_u8Prevent = 1;
                            g_sCSW.bCSWStatus = 0x01;
                            g_sCSW.dCSWDataResidue = 0;
                        }
                    } else { /* Ho <> Di (Case 10) */
                        g_u8Prevent = 1;
                        USBD_SET_EP_STALL(EP6);
                        g_sCSW.bCSWStatus = 0x01;
                        g_sCSW.dCSWDataResidue = Hcount;
                        g_u8BulkState = BULK_IN;
                        MSC_AckCmd();
                        return;
                    }
                }

                /* Get LBA address */
                g_u32Address = get_be32(&g_sCBW.au8Data[0]);
                g_u32LbaAddress = g_u32Address * UDC_SECTOR_SIZE;
                g_u32Length = g_sCBW.dCBWDataTransferLength;
                g_u32BytesInStorageBuf = g_u32Length;

                i = g_u32Length;
                if (i > STORAGE_BUFFER_SIZE)
                    i = STORAGE_BUFFER_SIZE;

                MSC_ReadMedia(g_u32Address * UDC_SECTOR_SIZE, i, (uint8_t *)STORAGE_DATA_BUF);
                g_u32BytesInStorageBuf = i;
                g_u32LbaAddress += i;

                g_u32Address = STORAGE_DATA_BUF;

                /* Indicate the next packet should be Bulk IN Data packet */
                g_u8BulkState = BULK_IN;
                if (g_u32BytesInStorageBuf > 0) {
                    /* Set the packet size */
                    if (g_u32BytesInStorageBuf > EP5_MSC_MAX_PKT_SIZE)
                        g_u8Size = EP5_MSC_MAX_PKT_SIZE;
                    else
                        g_u8Size = g_u32BytesInStorageBuf;

                    /* Prepare the first data packet (DATA1) */
                    /* Bulk IN buffer */
#if 0 //(NVT_DCACHE_ON == 1)
                    // Clean the D-Cache for the programmed region to ensure data consistency when D-Cache is enabled
                    SCB_CleanDCache_by_Addr((void *)g_u32Address, g_u8Size);
#endif
                    USBD_MemCopy((uint8_t *)((uint32_t)USBD_BUF_BASE + g_u32BulkBuf1), (uint8_t *)g_u32Address, g_u8Size);
                    g_u32Address += g_u8Size;

                    /* kick - start */
                    USBD_SET_EP_BUF_ADDR(EP5, g_u32BulkBuf1);
                    /* Trigger to send out the data packet */
                    USBD_SET_PAYLOAD_LEN(EP5, g_u8Size);
                    g_u32Length -= g_u8Size;
                    g_u32BytesInStorageBuf -= g_u8Size;
                }
                return;
            }
            case UFI_WRITE_12:
            case UFI_WRITE_10: {
                if (g_u32Length == 0) {
                    Dcount = (get_be32(&g_sCBW.au8Data[4])>>8) * 512;
                    if (g_sCBW.bmCBWFlags == 0x00) {    /* OUT */
                        if (Hcount == Dcount) { /* Ho == Do (Case 12)*/
                            g_sCSW.dCSWDataResidue = 0;
                            g_sCSW.bCSWStatus = 0;
                        } else if (Hcount < Dcount) { /* Hn < Do (Case 3) || Ho < Do (Case 13) */
                            g_u8Prevent = 1;
                            g_sCSW.dCSWDataResidue = 0;
                            g_sCSW.bCSWStatus = 0x1;
                            if (Hcount == 0) {  /* Hn < Do (Case 3) */
                                g_u8BulkState = BULK_IN;
                                MSC_AckCmd();
                                return;
                            }
                        } else if (Hcount > Dcount) { /* Ho > Do (Case 11) */
                            g_u8Prevent = 1;
                            g_sCSW.dCSWDataResidue = 0;
                            g_sCSW.bCSWStatus = 0x1;
                        }
                        g_u32Length = g_sCBW.dCBWDataTransferLength;
                        g_u32Address = STORAGE_DATA_BUF;
                        g_u32DataFlashStartAddr = get_be32(&g_sCBW.au8Data[0]) * UDC_SECTOR_SIZE;
                    } else { /* Hi <> Do (Case 8) */
                        g_u8Prevent = 1;
                        g_sCSW.dCSWDataResidue = Hcount;
                        g_sCSW.bCSWStatus = 0x1;
                        USBD_SET_EP_STALL(EP5);
                        g_u8BulkState = BULK_IN;
                        MSC_AckCmd();
                        return;
                    }
                }

                if ((g_u32Length > 0)) {
                    USBD_SET_PAYLOAD_LEN(EP6, EP6_MSC_MAX_PKT_SIZE);
                    g_u8BulkState = BULK_OUT;
                }
                return;
            }
            case UFI_READ_16: {
                USBD_SET_EP_STALL(EP5);
                g_u8Prevent = 1;
                g_sCSW.bCSWStatus = 0x01;
                g_sCSW.dCSWDataResidue = 0;
                g_u8BulkState = BULK_IN;
                MSC_AckCmd();
                return;
            }
            default: {
                /* Unsupported command */
                g_au8SenseKey[0] = 0x05;
                g_au8SenseKey[1] = 0x20;
                g_au8SenseKey[2] = 0x00;

                /* If CBW request for data phase, just return zero packet to end data phase */
                if (g_sCBW.dCBWDataTransferLength > 0) {
                    /* Data Phase, zero/short packet */
                    if ((g_sCBW.bmCBWFlags & 0x80) != 0) {
                        /* Data-In */
                        g_u8BulkState = BULK_IN;
                        USBD_SET_PAYLOAD_LEN(EP5, 0);
                    }
                } else {
                    /* Status Phase */
                    g_u8BulkState = BULK_IN;
                    MSC_AckCmd();
                }
                return;
            }
            }
        } else if (g_u8BulkState == BULK_OUT) {
            switch (g_sCBW.u8OPCode) {
            case UFI_WRITE_12:
            case UFI_WRITE_10:
            case UFI_MODE_SELECT_6:
            case UFI_MODE_SELECT_10: {
                MSC_Write();
                return;
            }
            }
        }
    }
}


void MSCTrans_SetConfig(void)
{
    // Clear stall status and ready
    USBD->EP[5].CFGP = 1;
    USBD->EP[6].CFGP = 1;
    /*****************************************************/
    /* EP5 ==> Bulk IN endpoint, address 5 */
    USBD_CONFIG_EP(EP5, USBD_CFG_EPMODE_IN | MSC_BULK_IN_EP_NUM);
    /* Buffer range for EP5 */
    USBD_SET_EP_BUF_ADDR(EP5, EP5_MSC_BUF_BASE);

    /* EP6 ==> Bulk Out endpoint, address 6 */
    USBD_CONFIG_EP(EP6, USBD_CFG_EPMODE_OUT | MSC_BULK_OUT_EP_NUM);
    /* Buffer range for EP6 */
    USBD_SET_EP_BUF_ADDR(EP6, EP6_MSC_BUF_BASE);

    /* trigger to receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP6, EP6_MSC_MAX_PKT_SIZE);


    USBD_LockEpStall(0);

    g_u8BulkState = BULK_CBW;

}

/*--------------------------------------------------------------------------*/
/**
  * @brief  USBD Endpoint Config.
  * @param  None.
  * @retval None.
  */
void MSCVCPTrans_Init(
    S_USBD_INFO_T *psUSBDevInfo,
    bool bMSCWriteProtect
)
{
    /* Init setup packet buffer */
    /* Buffer for setup packet -> [0 ~ 0x7] */
    USBD->STBUFSEG = SETUP_MSC_VCP_BUF_BASE;

    /*****************************************************/
    /* EP0 ==> control IN endpoint, address 0 */
    USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
    /* Buffer range for EP0 */
    USBD_SET_EP_BUF_ADDR(EP0, EP0_MSC_VCP_BUF_BASE);

    /* EP1 ==> control OUT endpoint, address 0 */
    USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
    /* Buffer range for EP1 */
    USBD_SET_EP_BUF_ADDR(EP1, EP1_MSC_VCP_BUF_BASE);

    /*****************************************************/
    /* EP2 ==> Bulk IN endpoint, address 2 */
    USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_IN | VCP_BULK_IN_EP_NUM);
    /* Buffer offset for EP2 */
    USBD_SET_EP_BUF_ADDR(EP2, EP2_VCP_BUF_BASE);

    /* EP3 ==> Bulk Out endpoint, address 3 */
    USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_OUT | VCP_BULK_OUT_EP_NUM);
    /* Buffer offset for EP3 */
    USBD_SET_EP_BUF_ADDR(EP3, EP3_VCP_BUF_BASE);
    /* trigger receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP3, EP3_VCP_MAX_PKT_SIZE);

    /* EP4 ==> Interrupt IN endpoint, address 4 */
    USBD_CONFIG_EP(EP4, USBD_CFG_EPMODE_IN | VCP_INT_IN_EP_NUM);
    /* Buffer offset for EP4 ->  */
    USBD_SET_EP_BUF_ADDR(EP4, EP4_VCP_BUF_BASE);

    /*****************************************************/
    /* EP5 ==> Bulk IN endpoint, address 5 */
    USBD_CONFIG_EP(EP5, USBD_CFG_EPMODE_IN | MSC_BULK_IN_EP_NUM);
    /* Buffer range for EP5 */
    USBD_SET_EP_BUF_ADDR(EP5, EP5_MSC_BUF_BASE);

    /* EP6 ==> Bulk Out endpoint, address 6 */
    USBD_CONFIG_EP(EP6, USBD_CFG_EPMODE_OUT | MSC_BULK_OUT_EP_NUM);
    /* Buffer range for EP6 */
    USBD_SET_EP_BUF_ADDR(EP6, EP6_MSC_BUF_BASE);
    /* trigger to receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP6, EP6_MSC_MAX_PKT_SIZE);

    /*****************************************************/
    g_u32BulkBuf0 = EP6_MSC_BUF_BASE;
    g_u32BulkBuf1 = EP5_MSC_BUF_BASE;

    g_sCSW.dCSWSignature = CSW_SIGNATURE;

    s_psUSBDevInfo = psUSBDevInfo;
    s_bMSCWriteProtect = bMSCWriteProtect;
}

