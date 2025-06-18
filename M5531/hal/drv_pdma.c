/**************************************************************************//**
 * @file     drv_pdma.c
 * @brief    PDMA high level driver for M5531 series
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nu_bitutil.h"
#include "drv_pdma.h"

#if defined(OS_FREERTOS)
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#endif

#ifndef NU_PDMA_MEMFUN_ACTOR_MAX
#define NU_PDMA_MEMFUN_ACTOR_MAX (4)
#endif

#define PDMA_CH_Msk                 ((1ul<<PDMA_CH_MAX)-1)
#define PDMA_INTSTS_REQTOFn_Pos     (8)
#define PDMA_INTSTS_REQTOFn_Msk     (PDMA_CH_Msk << PDMA_INTSTS_REQTOFn_Pos)
#define PDMA_BASE_OFFSET            (0x1000UL)

#define NU_PDMA_SG_TBL_MAXSIZE      (NU_PDMA_SG_LIMITED_DISTANCE/sizeof(DSCT_T))
#define NU_PDMA_CH_MAX              (PDMA_CNT*PDMA_CH_MAX)     /* Specify maximum channels of PDMA */
#define NU_PDMA_CH_Pos              (0)                        /* Specify first channel number of PDMA */
#define NU_PDMA_CH_Msk              (PDMA_CH_Msk << NU_PDMA_CH_Pos)
#define NU_PDMA_GET_BASE(ch)        (PDMA_T *)((((ch)/PDMA_CH_MAX)*PDMA_BASE_OFFSET) + PDMA0_BASE)
#define NU_PDMA_GET_MOD_IDX(ch)     ((ch)/PDMA_CH_MAX)
#define NU_PDMA_GET_MOD_CHIDX(ch)   ((ch)%PDMA_CH_MAX)

/* Private typedef --------------------------------------------------------------*/
struct nu_pdma_periph_ctl {
    uint32_t     m_u32Peripheral;
    nu_pdma_memctrl_t  m_eMemCtl;
};
typedef struct nu_pdma_periph_ctl nu_pdma_periph_ctl_t;

struct nu_pdma_chn {
    struct nu_pdma_chn_cb  m_sCB_Event;
    struct nu_pdma_chn_cb  m_sCB_Trigger;
    struct nu_pdma_chn_cb  m_sCB_Disable;

    nu_pdma_desc_t        *m_ppsSgtbl;
    nu_pdma_desc_t        *m_ppsSgtbl_unalign;
    uint32_t               m_u32WantedSGTblNum;

    uint32_t               m_u32EventFilter;
    uint32_t               m_u32IdleTimeout_us;
    nu_pdma_periph_ctl_t   m_spPeripCtl;
};
typedef struct nu_pdma_chn nu_pdma_chn_t;

struct nu_pdma_memfun_actor {
    int         m_i32ChannID;
    uint32_t    m_u32Result;
#if defined(OS_FREERTOS)
    SemaphoreHandle_t m_psSemMemFun;
#else
    volatile uint32_t m_psSemMemFun;
#endif
} ;
typedef struct nu_pdma_memfun_actor *nu_pdma_memfun_actor_t;

/* Private functions ------------------------------------------------------------*/
static int nu_pdma_peripheral_set(uint32_t u32PeriphType);
static void nu_pdma_init(void);
static void nu_pdma_channel_enable(int i32ChannID);
static void nu_pdma_channel_disable(int i32ChannID);
static void nu_pdma_channel_reset(int i32ChannID);
static int nu_pdma_timeout_set(int i32ChannID, int i32Timeout_us);
static void nu_pdma_periph_ctrl_fill(int i32ChannID, int i32CtlPoolIdx);
static int nu_pdma_memfun(void *dest, void *src, uint32_t u32DataWidth, unsigned int u32TransferCnt, nu_pdma_memctrl_t eMemCtl);
static void nu_pdma_memfun_cb(void *pvUserData, uint32_t u32Events);
static void nu_pdma_memfun_actor_init(void);
static int nu_pdma_memfun_employ(void);
static int nu_pdma_non_transfer_count_get(int32_t i32ChannID);

/* Public functions -------------------------------------------------------------*/


/* Private variables ------------------------------------------------------------*/
static volatile int nu_pdma_inited = 0;
static volatile uint32_t nu_pdma_chn_mask_arr[PDMA_CNT] = {0};
static nu_pdma_chn_t nu_pdma_chn_arr[NU_PDMA_CH_MAX];
static volatile uint32_t nu_pdma_memfun_actor_mask = 0;
static volatile uint32_t nu_pdma_memfun_actor_maxnum = 0;

const static struct nu_module nu_pdma_arr[] = {
    {
        .name = "pdma0",
        .m_pvBase = (void *)PDMA0,
        .u32RstId = SYS_PDMA0RST,
        .eIRQn = PDMA0_IRQn
    },
    {
        .name = "pdma1",
        .m_pvBase = (void *)PDMA1,
        .u32RstId = SYS_PDMA1RST,
        .eIRQn = PDMA1_IRQn
    },
};

static void *nvt_malloc_align(uint32_t size, uint32_t align, void **unalign_ptr)
{
    void *ptr;
    uint32_t align_size;

    align = NVT_ALIGN(align, sizeof(void *));

    align_size = NVT_ALIGN(size, sizeof(void *)) + align;

    if ((ptr = malloc(align_size)) != NULL) {
        void *align_ptr;

        if (((uintptr_t)ptr & (align - 1)) == 0) {
            align_ptr = (void *)((uintptr_t)ptr + align);
        } else {
            align_ptr = (void *)NVT_ALIGN((uintptr_t)ptr, align);
        }

        *unalign_ptr = ptr;
        ptr = align_ptr;
    }


    return ptr;
}

static const nu_pdma_periph_ctl_t g_nu_pdma_peripheral_ctl_pool[ ] = {
    // M2M
    { PDMA_MEM, eMemCtl_SrcInc_DstInc },

    // P2M, M2P
    { PDMA_UART0_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_UART0_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_UART1_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_UART1_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_UART2_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_UART2_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_UART3_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_UART3_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_UART4_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_UART4_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_UART5_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_UART5_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_UART6_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_UART6_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_UART7_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_UART7_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_UART8_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_UART8_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_UART9_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_UART9_RX, eMemCtl_SrcFix_DstInc },

    { PDMA_DAC0_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_DAC1_TX, eMemCtl_SrcInc_DstFix },

    { PDMA_SPI0_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_SPI0_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_SPI1_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_SPI1_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_SPI2_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_SPI2_RX, eMemCtl_SrcFix_DstInc },
    { PDMA_SPI3_TX, eMemCtl_SrcInc_DstFix },
    { PDMA_SPI3_RX, eMemCtl_SrcFix_DstInc },
};
#define NU_PERIPHERAL_SIZE ( sizeof(g_nu_pdma_peripheral_ctl_pool) / sizeof(g_nu_pdma_peripheral_ctl_pool[0]) )

static struct nu_pdma_memfun_actor nu_pdma_memfun_actor_arr[NU_PDMA_MEMFUN_ACTOR_MAX];

/* SG table pool */
static DSCT_T nu_pdma_sgtbl_arr[NU_PDMA_SGTBL_POOL_SIZE] = { 0 };
static uint32_t nu_pdma_sgtbl_token[NVT_ALIGN(NU_PDMA_SGTBL_POOL_SIZE, 32) / 32];

static int nu_pdma_check_is_nonallocated(uint32_t u32ChnId)
{
    uint32_t mod_idx = NU_PDMA_GET_MOD_IDX(u32ChnId);
    PDMA_ASSERT(mod_idx < PDMA_CNT);
    return !(nu_pdma_chn_mask_arr[mod_idx] & (1 << NU_PDMA_GET_MOD_CHIDX(u32ChnId)));
}

static int nu_pdma_peripheral_set(uint32_t u32PeriphType)
{
    int idx = 0;

    while (idx < NU_PERIPHERAL_SIZE) {
        if (g_nu_pdma_peripheral_ctl_pool[idx].m_u32Peripheral == u32PeriphType)
            return idx;

        idx++;
    }

    // Not such peripheral
    return -1;
}

static void nu_pdma_periph_ctrl_fill(int i32ChannID, int i32CtlPoolIdx)
{
    nu_pdma_chn_t *psPdmaChann = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos];
    psPdmaChann->m_spPeripCtl.m_u32Peripheral = g_nu_pdma_peripheral_ctl_pool[i32CtlPoolIdx].m_u32Peripheral;
    psPdmaChann->m_spPeripCtl.m_eMemCtl = g_nu_pdma_peripheral_ctl_pool[i32CtlPoolIdx].m_eMemCtl;
}

/**
 * Hardware PDMA Initialization
 */
static void nu_pdma_init(void)
{
    int i, latest = 0;

    if (nu_pdma_inited)
        return;

    memset(&nu_pdma_sgtbl_arr[0], 0x00, sizeof(nu_pdma_sgtbl_arr));
    memset(nu_pdma_chn_arr, 0x00, sizeof(nu_pdma_chn_arr));

    for (i = (PDMA_START + 1); i < PDMA_CNT; i++) {
        PDMA_T *psPDMA = (PDMA_T *)nu_pdma_arr[i].m_pvBase;
        nu_pdma_chn_mask_arr[i] = ~(NU_PDMA_CH_Msk);

        SYS_ResetModule(nu_pdma_arr[i].u32RstId);

        /* Initialize PDMA setting */
        PDMA_Open(psPDMA, PDMA_CH_Msk);

#if defined(OS_FREERTOS)
        NVIC_SetPriority((IRQn_Type)nu_pdma_arr[i].eIRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
#endif
        /* Enable PDMA interrupt */
        NVIC_EnableIRQ((IRQn_Type)nu_pdma_arr[i].eIRQn);

    }

    /* Initialize token pool. */
    memset(&nu_pdma_sgtbl_token[0], 0xff, sizeof(nu_pdma_sgtbl_token));

    if (NU_PDMA_SGTBL_POOL_SIZE % 32) {
        latest = (NU_PDMA_SGTBL_POOL_SIZE) / 32;
        nu_pdma_sgtbl_token[latest] ^= ~((1 << (NU_PDMA_SGTBL_POOL_SIZE % 32)) - 1) ;
    }

    nu_pdma_inited = 1;
}

static inline void nu_pdma_channel_enable(int i32ChannID)
{
    PDMA_T *PDMA = NU_PDMA_GET_BASE(i32ChannID);
    int u32ModChannId = NU_PDMA_GET_MOD_CHIDX(i32ChannID);

    /* Clean descriptor table control register. */
    PDMA->DSCT[u32ModChannId].CTL = 0UL;

    /* Enable the channel */
    PDMA->CHCTL |= (1 << u32ModChannId);
}

static inline void nu_pdma_channel_disable(int i32ChannID)
{
    PDMA_T *PDMA = NU_PDMA_GET_BASE(i32ChannID);
    int u32ModChannId = NU_PDMA_GET_MOD_CHIDX(i32ChannID);
    PDMA->CHCTL &= ~(1 << NU_PDMA_GET_MOD_CHIDX(i32ChannID));
}

static inline void nu_pdma_channel_reset(int i32ChannID)
{
    PDMA_T *PDMA = NU_PDMA_GET_BASE(i32ChannID);
    int u32ModChannId = NU_PDMA_GET_MOD_CHIDX(i32ChannID);


    PDMA->CHRST = (1 << u32ModChannId);

    /* Wait for cleared channel CHCTL. */
    while ((PDMA->CHCTL & (1 << u32ModChannId)));
}

void nu_pdma_channel_terminate(int i32ChannID)
{
    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_pdma_channel_terminate;

    //printf("[%s] %d\n", __func__, i32ChannID);

    /* Reset specified channel. */
    nu_pdma_channel_reset(i32ChannID);

    /* Enable specified channel after reset. */
    nu_pdma_channel_enable(i32ChannID);

exit_pdma_channel_terminate:

    return;
}

static int nu_pdma_timeout_set(int i32ChannID, int i32Timeout_us)
{
    int ret = 1;
    PDMA_T *PDMA = NULL;
    uint32_t u32ModChannId;

    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_timeout_set;

    PDMA = NU_PDMA_GET_BASE(i32ChannID);

    u32ModChannId = NU_PDMA_GET_MOD_CHIDX(i32ChannID);

    nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_u32IdleTimeout_us = i32Timeout_us;

    if (i32Timeout_us) {
        uint32_t u32ToClk_Max = 1000000 / (CLK_GetHCLK0Freq() / (1 << 8));
        uint32_t u32Divider     = (i32Timeout_us / u32ToClk_Max) / (1 << 16);
        uint32_t u32TOutCnt     = (i32Timeout_us / u32ToClk_Max) % (1 << 16);

        PDMA_DisableTimeout(PDMA,  1 << u32ModChannId);
        PDMA_EnableInt(PDMA, u32ModChannId, PDMA_INT_TIMEOUT);    // Interrupt type

        if (u32Divider > 7) {
            u32Divider = 7;
            u32TOutCnt = (1 << 16);
        }

        if (u32ModChannId < 8)
            PDMA->TOUTPSC0_7 = (PDMA->TOUTPSC0_7 & ~(0x7ul << (PDMA_TOUTPSC0_7_TOUTPSC0_Pos * u32ModChannId))) | (u32Divider << (PDMA_TOUTPSC0_7_TOUTPSC0_Pos * u32ModChannId));
        else
            PDMA->TOUTPSC8_15 = (PDMA->TOUTPSC8_15 & ~(0x7ul << (PDMA_TOUTPSC8_15_TOUTPSC8_Pos * (u32ModChannId % 8)))) | (u32Divider << (PDMA_TOUTPSC8_15_TOUTPSC8_Pos * (u32ModChannId % 8)));

        PDMA_SetTimeOut(PDMA,  u32ModChannId, 1, u32TOutCnt);

        ret = 0;
    } else {
        PDMA_DisableInt(PDMA, u32ModChannId, PDMA_INT_TIMEOUT);    // Interrupt type
        PDMA_DisableTimeout(PDMA,  1 << u32ModChannId);
    }

exit_nu_pdma_timeout_set:

    return -(ret);
}

int nu_pdma_channel_dynamic_allocate(int32_t i32PeripType)
{
    int ChnId, i32PeripCtlIdx, j;

    nu_pdma_init();

    if ((i32PeripCtlIdx = nu_pdma_peripheral_set(i32PeripType)) < 0)
        goto exit_nu_pdma_channel_dynamic_allocate;

    for (j = (PDMA_START + 1); j < PDMA_CNT; j++) {
        /* Find the position of first '0' in nu_pdma_chn_mask_arr[j]. */
        ChnId = nu_cto(nu_pdma_chn_mask_arr[j]);

        if (ChnId < PDMA_CH_MAX) {
            nu_pdma_chn_mask_arr[j] |= (1 << ChnId);
            ChnId += (j * PDMA_CH_MAX);
            memset(nu_pdma_chn_arr + ChnId - NU_PDMA_CH_Pos, 0x00, sizeof(nu_pdma_chn_t));

            /* Set idx number of g_nu_pdma_peripheral_ctl_pool */
            nu_pdma_periph_ctrl_fill(ChnId, i32PeripCtlIdx);

            /* Reset channel */
            nu_pdma_channel_terminate(ChnId);

            return ChnId;
        }
    }

exit_nu_pdma_channel_dynamic_allocate:
    // No channel available
    return NU_PDMA_OUT_OF_CHANNELS;
}


// i32PDMAId: 0, 1
// i32Channel: 0~15

int nu_pdma_channel_allocate(int32_t i32PeripType, int32_t i32PDMAId, int32_t i32Channel)
{
    int ChnId, i32PeripCtlIdx, j;

    nu_pdma_init();

    if ((i32PeripCtlIdx = nu_pdma_peripheral_set(i32PeripType)) < 0)
        goto exit_nu_pdma_channel_allocate;

    if ((i32PDMAId <= PDMA_START) || (i32PDMAId >= PDMA_CNT))
        goto exit_nu_pdma_channel_allocate;

    if (i32Channel >= PDMA_CH_MAX)
        goto exit_nu_pdma_channel_allocate;

    j = i32PDMAId;
    ChnId = i32Channel;

    //Check channel in used or not
    if ((nu_pdma_chn_mask_arr[j] & (1 << ChnId)) == 0) {
        nu_pdma_chn_mask_arr[j] |= (1 << ChnId);
        ChnId += (j * PDMA_CH_MAX);
        memset(nu_pdma_chn_arr + ChnId - NU_PDMA_CH_Pos, 0x00, sizeof(nu_pdma_chn_t));

        /* Set idx number of g_nu_pdma_peripheral_ctl_pool */
        nu_pdma_periph_ctrl_fill(ChnId, i32PeripCtlIdx);

        /* Reset channel */
        nu_pdma_channel_terminate(ChnId);

        return ChnId;
    }

exit_nu_pdma_channel_allocate:
    // No channel available
    return NU_PDMA_OUT_OF_CHANNELS;
}


int nu_pdma_channel_free(int i32ChannID)
{
    int ret = 1;

    if (! nu_pdma_inited)
        goto exit_nu_pdma_channel_free;

    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_channel_free;

    if ((i32ChannID < NU_PDMA_CH_MAX) && (i32ChannID >= NU_PDMA_CH_Pos)) {
        nu_pdma_chn_mask_arr[NU_PDMA_GET_MOD_IDX(i32ChannID)] &= ~(1 << NU_PDMA_GET_MOD_CHIDX(i32ChannID));
        nu_pdma_channel_disable(i32ChannID);
        ret =  0;
    }

exit_nu_pdma_channel_free:

    return -(ret);
}

int nu_pdma_filtering_set(int i32ChannID, uint32_t u32EventFilter)
{
    int ret = 1;

    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_filtering_set;

    nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_u32EventFilter = u32EventFilter;

    ret = 0;

exit_nu_pdma_filtering_set:

    return -(ret) ;
}

uint32_t nu_pdma_filtering_get(int i32ChannID)
{
    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_filtering_get;

    return nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_u32EventFilter;

exit_nu_pdma_filtering_get:

    return 0;
}

int nu_pdma_callback_register(int i32ChannID, nu_pdma_chn_cb_t psChnCb)
{
    int ret = 1;
    nu_pdma_chn_cb_t psChnCb_Current = NULL;

    PDMA_ASSERT(psChnCb != NULL);

    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_callback_register;

    switch (psChnCb->m_eCBType) {
    case eCBType_Event:
        psChnCb_Current = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_sCB_Event;
        break;

    case eCBType_Trigger:
        psChnCb_Current = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_sCB_Trigger;
        break;

    case eCBType_Disable:
        psChnCb_Current = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_sCB_Disable;
        break;

    default:
        goto exit_nu_pdma_callback_register;
    }

    psChnCb_Current->m_pfnCBHandler = psChnCb->m_pfnCBHandler;
    psChnCb_Current->m_pvUserData = psChnCb->m_pvUserData;

    ret = 0;

exit_nu_pdma_callback_register:

    return -(ret) ;
}

nu_pdma_cb_handler_t nu_pdma_callback_hijack(int i32ChannID, nu_pdma_cbtype_t eCBType, nu_pdma_chn_cb_t psChnCb_Hijack)
{
    nu_pdma_chn_cb_t psChnCb_Current = NULL;
    struct nu_pdma_chn_cb sChnCB_Tmp;

    PDMA_ASSERT(psChnCb_Hijack != NULL);

    sChnCB_Tmp.m_pfnCBHandler = NULL;

    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_callback_hijack;

    switch (eCBType) {
    case eCBType_Event:
        psChnCb_Current = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_sCB_Event;
        break;

    case eCBType_Trigger:
        psChnCb_Current = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_sCB_Trigger;
        break;

    case eCBType_Disable:
        psChnCb_Current = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_sCB_Disable;
        break;

    default:
        goto exit_nu_pdma_callback_hijack;
    }

    /* Backup */
    sChnCB_Tmp.m_pfnCBHandler = psChnCb_Current->m_pfnCBHandler;
    sChnCB_Tmp.m_pvUserData = psChnCb_Current->m_pvUserData;

    /* Update */
    psChnCb_Current->m_pfnCBHandler = psChnCb_Hijack->m_pfnCBHandler;
    psChnCb_Current->m_pvUserData = psChnCb_Hijack->m_pvUserData;

    /* Restore */
    psChnCb_Hijack->m_pfnCBHandler = sChnCB_Tmp.m_pfnCBHandler;
    psChnCb_Hijack->m_pvUserData = sChnCB_Tmp.m_pvUserData;

exit_nu_pdma_callback_hijack:

    return sChnCB_Tmp.m_pfnCBHandler;
}

static int nu_pdma_non_transfer_count_get(int32_t i32ChannID)
{
    PDMA_T *PDMA = NU_PDMA_GET_BASE(i32ChannID);
    return ((PDMA->DSCT[NU_PDMA_GET_MOD_CHIDX(i32ChannID)].CTL & PDMA_DSCT_CTL_TXCNT_Msk) >> PDMA_DSCT_CTL_TXCNT_Pos) + 1;
}

int nu_pdma_transferred_byte_get(int32_t i32ChannID, int32_t i32TriggerByteLen)
{
    int i32BitWidth = 0;
    int cur_txcnt = 0;
    PDMA_T *PDMA;

    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_transferred_byte_get;

    PDMA = NU_PDMA_GET_BASE(i32ChannID);

    i32BitWidth = PDMA->DSCT[NU_PDMA_GET_MOD_CHIDX(i32ChannID)].CTL & PDMA_DSCT_CTL_TXWIDTH_Msk;
    i32BitWidth = (i32BitWidth == PDMA_WIDTH_8) ? 1 : (i32BitWidth == PDMA_WIDTH_16) ? 2 : (i32BitWidth == PDMA_WIDTH_32) ? 4 : 0;

    cur_txcnt = nu_pdma_non_transfer_count_get(i32ChannID);

    return (i32TriggerByteLen - (cur_txcnt) * i32BitWidth);

exit_nu_pdma_transferred_byte_get:

    return -1;
}

nu_pdma_memctrl_t nu_pdma_channel_memctrl_get(int i32ChannID)
{
    nu_pdma_memctrl_t eMemCtrl = eMemCtl_Undefined;

    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_channel_memctrl_get;

    eMemCtrl = nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_spPeripCtl.m_eMemCtl;

exit_nu_pdma_channel_memctrl_get:

    return eMemCtrl;
}

int nu_pdma_channel_memctrl_set(int i32ChannID, nu_pdma_memctrl_t eMemCtrl)
{
    int ret = 1;
    nu_pdma_chn_t *psPdmaChann = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos];

    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_channel_memctrl_set;
    else if ((eMemCtrl < eMemCtl_SrcFix_DstFix) || (eMemCtrl > eMemCtl_SrcInc_DstInc))
        goto exit_nu_pdma_channel_memctrl_set;

    /* PDMA_MEM/SAR_FIX/BURST mode is not supported. */
    if ((psPdmaChann->m_spPeripCtl.m_u32Peripheral == PDMA_MEM) &&
        ((eMemCtrl == eMemCtl_SrcFix_DstInc) || (eMemCtrl == eMemCtl_SrcFix_DstFix)))
        goto exit_nu_pdma_channel_memctrl_set;

    nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_spPeripCtl.m_eMemCtl = eMemCtrl;

    ret = 0;

exit_nu_pdma_channel_memctrl_set:

    return -(ret);
}

static void nu_pdma_channel_memctrl_fill(nu_pdma_memctrl_t eMemCtl, uint32_t *pu32SrcCtl, uint32_t *pu32DstCtl)
{
    switch ((int)eMemCtl) {
    case eMemCtl_SrcFix_DstFix:
        *pu32SrcCtl = PDMA_SAR_FIX;
        *pu32DstCtl = PDMA_DAR_FIX;
        break;

    case eMemCtl_SrcFix_DstInc:
        *pu32SrcCtl = PDMA_SAR_FIX;
        *pu32DstCtl = PDMA_DAR_INC;
        break;

    case eMemCtl_SrcInc_DstFix:
        *pu32SrcCtl = PDMA_SAR_INC;
        *pu32DstCtl = PDMA_DAR_FIX;
        break;

    case eMemCtl_SrcInc_DstInc:
        *pu32SrcCtl = PDMA_SAR_INC;
        *pu32DstCtl = PDMA_DAR_INC;
        break;

    default:
        break;
    }
}

/* This is for Scatter-gather DMA. */
int nu_pdma_desc_setup(int i32ChannID, nu_pdma_desc_t dma_desc, uint32_t u32DataWidth, uint32_t u32AddrSrc,
                       uint32_t u32AddrDst, int32_t i32TransferCnt, nu_pdma_desc_t next, uint32_t u32BeSilent)
{
    nu_pdma_periph_ctl_t *psPeriphCtl = NULL;

    uint32_t u32SrcCtl = 0;
    uint32_t u32DstCtl = 0;

    int ret = 0;

    if (!dma_desc) {
        ret = -1;
        goto exit_nu_pdma_desc_setup;
    } else if (nu_pdma_check_is_nonallocated(i32ChannID)) {
        ret = -2;
        goto exit_nu_pdma_desc_setup;
    } else if (!(u32DataWidth == 8 || u32DataWidth == 16 || u32DataWidth == 32)) {
        ret = -3;
        goto exit_nu_pdma_desc_setup;
    } else if ((u32AddrSrc % (u32DataWidth / 8)) || (u32AddrDst % (u32DataWidth / 8))) {
        ret = -4;
        goto exit_nu_pdma_desc_setup;
    } else if (i32TransferCnt > NU_PDMA_MAX_TXCNT) {
        ret = -5;
        goto exit_nu_pdma_desc_setup;
    }

    psPeriphCtl = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_spPeripCtl;

    nu_pdma_channel_memctrl_fill(psPeriphCtl->m_eMemCtl, &u32SrcCtl, &u32DstCtl);

    dma_desc->CTL = ((i32TransferCnt - 1) << PDMA_DSCT_CTL_TXCNT_Pos) |
                    ((u32DataWidth == 8) ? PDMA_WIDTH_8 : (u32DataWidth == 16) ? PDMA_WIDTH_16 : PDMA_WIDTH_32) |
                    u32SrcCtl |
                    u32DstCtl |
                    PDMA_OP_BASIC;

    dma_desc->SA = u32AddrSrc;
    dma_desc->DA = u32AddrDst;
    dma_desc->NEXT = 0;  /* Terminating node by default. */


    if (psPeriphCtl->m_u32Peripheral == PDMA_MEM) {
        /* For M2M transfer */
        dma_desc->CTL |= (PDMA_REQ_BURST | PDMA_BURST_32);
    } else {
        /* For P2M and M2P transfer */
        dma_desc->CTL |= (PDMA_REQ_SINGLE);
    }

    if (next) {
        /* Link to Next and modify to scatter-gather DMA mode. */
        dma_desc->CTL = (dma_desc->CTL & ~PDMA_DSCT_CTL_OPMODE_Msk) | PDMA_OP_SCATTER;
        dma_desc->NEXT = (uint32_t)next;
    }

#if 0
    printf("DDDD nu_pdma_desc_setup dma_desc %p \n", dma_desc);
    printf("DDDD nu_pdma_desc_setup dma_desc->CTL %x \n", dma_desc->CTL);
    printf("DDDD nu_pdma_desc_setup dma_desc->SA %x \n", dma_desc->SA);
    printf("DDDD nu_pdma_desc_setup dma_desc->DA %x \n", dma_desc->DA);
    printf("DDDD nu_pdma_desc_setup dma_desc->NEXT %x \n", dma_desc->NEXT);
    printf("DDDD nu_pdma_desc_setup i32TransferCnt %d \n", i32TransferCnt);
#endif

    /* Be silent */
    if (u32BeSilent)
        dma_desc->CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    ret = 0;

exit_nu_pdma_desc_setup:

    return ret;
}

/* This is for M2M Scatter-gather descriptor. */
int nu_pdma_m2m_desc_setup(nu_pdma_desc_t dma_desc, uint32_t u32DataWidth, uint32_t u32AddrSrc,
                           uint32_t u32AddrDst, int32_t i32TransferCnt, nu_pdma_memctrl_t evMemCtrl, nu_pdma_desc_t next, uint32_t u32BeSilent)
{
    uint32_t u32SrcCtl = 0;
    uint32_t u32DstCtl = 0;

    int ret = 1;

    if (!dma_desc)
        goto exit_nu_pdma_desc_setup;
    else if (!(u32DataWidth == 8 || u32DataWidth == 16 || u32DataWidth == 32))
        goto exit_nu_pdma_desc_setup;
    else if ((u32AddrSrc % (u32DataWidth / 8)) || (u32AddrDst % (u32DataWidth / 8)))
        goto exit_nu_pdma_desc_setup;
    else if (i32TransferCnt > NU_PDMA_MAX_TXCNT)
        goto exit_nu_pdma_desc_setup;


    nu_pdma_channel_memctrl_fill(evMemCtrl, &u32SrcCtl, &u32DstCtl);

    dma_desc->CTL = ((i32TransferCnt - 1) << PDMA_DSCT_CTL_TXCNT_Pos) |
                    ((u32DataWidth == 8) ? PDMA_WIDTH_8 : (u32DataWidth == 16) ? PDMA_WIDTH_16 : PDMA_WIDTH_32) |
                    u32SrcCtl |
                    u32DstCtl |
                    PDMA_OP_BASIC;

    dma_desc->SA = u32AddrSrc;
    dma_desc->DA = u32AddrDst;
    dma_desc->NEXT = 0;  /* Terminating node by default. */

    /* For M2M transfer */
    dma_desc->CTL |= (PDMA_REQ_BURST | PDMA_BURST_32);

    if (next) {
        /* Link to Next and modify to scatter-gather DMA mode. */
        dma_desc->CTL = (dma_desc->CTL & ~PDMA_DSCT_CTL_OPMODE_Msk) | PDMA_OP_SCATTER;
        dma_desc->NEXT = (uint32_t)next;
    }

    /* Be silent */
    if (u32BeSilent)
        dma_desc->CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    ret = 0;

exit_nu_pdma_desc_setup:

    return -(ret);
}


static int nu_pdma_sgtbls_token_allocate(void)
{
    int idx, i;

    int pool_size = sizeof(nu_pdma_sgtbl_token) / sizeof(uint32_t);

    for (i = 0; i < pool_size; i++) {
        if ((idx = nu_ctz(nu_pdma_sgtbl_token[i])) != 32) {
            nu_pdma_sgtbl_token[i] &= ~(1 << idx);
            idx += i * 32;
            return idx;
        }
    }

    /* No available */
    return -1;
}

static void nu_pdma_sgtbls_token_free(nu_pdma_desc_t psSgtbls)
{
    int idx = (int)(psSgtbls - &nu_pdma_sgtbl_arr[0]);
    PDMA_ASSERT(idx >= 0);
    PDMA_ASSERT((idx + 1) <= NU_PDMA_SGTBL_POOL_SIZE);
    nu_pdma_sgtbl_token[idx / 32] |= (1 << (idx % 32));
}

void nu_pdma_sgtbls_free(nu_pdma_desc_t *ppsSgtbls, int num)
{
    int i;

    PDMA_ASSERT(ppsSgtbls != NULL);
    PDMA_ASSERT(num <= NU_PDMA_SG_TBL_MAXSIZE);

    for (i = 0; i < num; i++) {
        if (ppsSgtbls[i] != NULL) {
            nu_pdma_sgtbls_token_free(ppsSgtbls[i]);
        }

        ppsSgtbls[i] = NULL;
    }
}

int nu_pdma_sgtbls_allocate(nu_pdma_desc_t *ppsSgtbls, int num)
{
    int i, idx;

    PDMA_ASSERT(ppsSgtbls);
    PDMA_ASSERT(num <= NU_PDMA_SG_TBL_MAXSIZE);

    for (i = 0; i < num; i++) {
        ppsSgtbls[i] = NULL;

        /* Get token. */
        if ((idx = nu_pdma_sgtbls_token_allocate()) < 0) {
            printf("No available sgtbl.\n");
            goto fail_nu_pdma_sgtbls_allocate;
        }

        ppsSgtbls[i] = (nu_pdma_desc_t)&nu_pdma_sgtbl_arr[idx];
    }

    return 0;

fail_nu_pdma_sgtbls_allocate:

    /* Release allocated tables. */
    nu_pdma_sgtbls_free(ppsSgtbls, i);

    return -1;
}


static void _nu_pdma_transfer(int i32ChannID, uint32_t u32Peripheral, nu_pdma_desc_t head, uint32_t u32IdleTimeout_us)
{
    PDMA_T *PDMA = NU_PDMA_GET_BASE(i32ChannID);
    nu_pdma_chn_t *psPdmaChann = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos];

#if (NVT_DCACHE_ON == 1)
    /* Writeback data in dcache to memory before transferring. */
    {
        static uint32_t bNonCacheAlignedWarning = 1;
        nu_pdma_desc_t next = head;

        while (next != NULL) {
            uint32_t u32TxCnt     = ((next->CTL & PDMA_DSCT_CTL_TXCNT_Msk) >> PDMA_DSCT_CTL_TXCNT_Pos) + 1;
            uint32_t u32DataWidth = (1 << ((next->CTL & PDMA_DSCT_CTL_TXWIDTH_Msk) >> PDMA_DSCT_CTL_TXWIDTH_Pos));
            uint32_t u32SrcCtl    = (next->CTL & PDMA_DSCT_CTL_SAINC_Msk);
            uint32_t u32DstCtl    = (next->CTL & PDMA_DSCT_CTL_DAINC_Msk);
            uint32_t u32FlushLen  = u32TxCnt * u32DataWidth;

            /* Flush Src buffer into memory. */
            if ((u32SrcCtl == PDMA_SAR_INC)) // for M2P, M2M
                SCB_CleanInvalidateDCache_by_Addr((volatile void *)next->SA, (int32_t)u32FlushLen);

            /* Flush Dst buffer into memory. */
            if ((u32DstCtl == PDMA_DAR_INC)) // for P2M, M2M
                SCB_CleanInvalidateDCache_by_Addr((volatile void *)next->DA, (int32_t)u32FlushLen);

            /* Flush descriptor into memory */
            if (!((uint32_t)next & BIT31))
                SCB_CleanInvalidateDCache_by_Addr((volatile void *)next, sizeof(DSCT_T));

            if (bNonCacheAlignedWarning) {
                if ((u32FlushLen & (DCACHE_LINE_SIZE - 1)) ||
                    (next->SA & (DCACHE_LINE_SIZE - 1)) ||
                    (next->DA & (DCACHE_LINE_SIZE - 1)) ||
                    ((uint32_t)next & (DCACHE_LINE_SIZE - 1))) {
                    /*
                        Race-condition avoidance between DMA-transferring and DCache write-back:
                        Source, destination, DMA descriptor address and length should be aligned at len(CACHE_LINE_SIZE)
                    */
                    bNonCacheAlignedWarning = 0;
                    printf("[PDMA-W]\n");
                }
            }

            next = (nu_pdma_desc_t)next->NEXT;

            if (next == head) break;
        }
    }
#endif

    PDMA_DisableTimeout(PDMA,  1 << NU_PDMA_GET_MOD_CHIDX(i32ChannID));

    PDMA_EnableInt(PDMA, NU_PDMA_GET_MOD_CHIDX(i32ChannID), PDMA_INT_TRANS_DONE);

    nu_pdma_timeout_set(i32ChannID, u32IdleTimeout_us);

    /* Set scatter-gather mode and head */
    /* Take care the head structure, you should make sure cache-coherence. */
    PDMA_SetTransferMode(PDMA,
                         NU_PDMA_GET_MOD_CHIDX(i32ChannID),
                         u32Peripheral,
                         (head->NEXT != 0) ? 1 : 0,
                         (uint32_t)head);

    /* If peripheral is M2M, trigger it. */
    if (u32Peripheral == PDMA_MEM) {
        PDMA_Trigger(PDMA, NU_PDMA_GET_MOD_CHIDX(i32ChannID));
    } else if (psPdmaChann->m_sCB_Trigger.m_pfnCBHandler) {
        psPdmaChann->m_sCB_Trigger.m_pfnCBHandler(psPdmaChann->m_sCB_Trigger.m_pvUserData, psPdmaChann->m_sCB_Trigger.m_u32Reserved);
    }
}

static void _nu_pdma_free_sgtbls(nu_pdma_chn_t *psPdmaChann)
{
    if (psPdmaChann->m_ppsSgtbl) {
        nu_pdma_sgtbls_free(psPdmaChann->m_ppsSgtbl, psPdmaChann->m_u32WantedSGTblNum);
        psPdmaChann->m_ppsSgtbl = NULL;
        free(psPdmaChann->m_ppsSgtbl_unalign);
        psPdmaChann->m_ppsSgtbl_unalign = NULL;
        psPdmaChann->m_u32WantedSGTblNum = 0;
    }
}

static int _nu_pdma_transfer_chain(int i32ChannID, uint32_t u32DataWidth, uint32_t u32AddrSrc, uint32_t u32AddrDst, uint32_t u32TransferCnt, uint32_t u32IdleTimeout_us)
{
    int i = 0;
    int ret = 1;
    nu_pdma_periph_ctl_t *psPeriphCtl = NULL;
    nu_pdma_chn_t *psPdmaChann = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos];

    nu_pdma_memctrl_t eMemCtl = nu_pdma_channel_memctrl_get(i32ChannID);

    uint32_t u32Offset = 0;
    uint32_t u32TxCnt = 0;

    psPeriphCtl = &psPdmaChann->m_spPeripCtl;

    if (psPdmaChann->m_u32WantedSGTblNum != (u32TransferCnt / NU_PDMA_MAX_TXCNT + 1)) {
        if (psPdmaChann->m_u32WantedSGTblNum > 0)
            _nu_pdma_free_sgtbls(psPdmaChann);

        psPdmaChann->m_u32WantedSGTblNum = u32TransferCnt / NU_PDMA_MAX_TXCNT + 1;

        psPdmaChann->m_ppsSgtbl = (nu_pdma_desc_t *)nvt_malloc_align(sizeof(nu_pdma_desc_t) * psPdmaChann->m_u32WantedSGTblNum, 4, (void **)&psPdmaChann->m_ppsSgtbl_unalign);

        if (!psPdmaChann->m_ppsSgtbl)
            goto exit__nu_pdma_transfer_chain;

        ret = nu_pdma_sgtbls_allocate(psPdmaChann->m_ppsSgtbl, psPdmaChann->m_u32WantedSGTblNum);

        if (ret != 0)
            goto exit__nu_pdma_transfer_chain;
    }

    for (i = 0; i < psPdmaChann->m_u32WantedSGTblNum; i++) {
        u32TxCnt = (u32TransferCnt > NU_PDMA_MAX_TXCNT) ? NU_PDMA_MAX_TXCNT : u32TransferCnt;

        ret = nu_pdma_desc_setup(i32ChannID,
                                 psPdmaChann->m_ppsSgtbl[i],
                                 u32DataWidth,
                                 (eMemCtl & 0x2ul) ? u32AddrSrc + u32Offset : u32AddrSrc, /* Src address is Inc or not. */
                                 (eMemCtl & 0x1ul) ? u32AddrDst + u32Offset : u32AddrDst, /* Dst address is Inc or not. */
                                 u32TxCnt,
                                 ((i + 1) == psPdmaChann->m_u32WantedSGTblNum) ? NULL : psPdmaChann->m_ppsSgtbl[i + 1],
                                 ((i + 1) == psPdmaChann->m_u32WantedSGTblNum) ? 0 : 1); // Silent, w/o TD interrupt

        if (ret != 0)
            goto exit__nu_pdma_transfer_chain;

        u32TransferCnt -= u32TxCnt;
        u32Offset += (u32TxCnt * u32DataWidth / 8);
    }

    _nu_pdma_transfer(i32ChannID, psPeriphCtl->m_u32Peripheral, psPdmaChann->m_ppsSgtbl[0], u32IdleTimeout_us);

    ret = 0;

    return ret;

exit__nu_pdma_transfer_chain:

    _nu_pdma_free_sgtbls(psPdmaChann);

    return -(ret);
}

int nu_pdma_transfer(int i32ChannID, uint32_t u32DataWidth, uint32_t u32AddrSrc, uint32_t u32AddrDst, uint32_t u32TransferCnt, uint32_t u32IdleTimeout_us)
{
    int ret = 1;
    PDMA_T *PDMA = NU_PDMA_GET_BASE(i32ChannID);
    nu_pdma_desc_t head;
    nu_pdma_chn_t *psPdmaChann;

    nu_pdma_periph_ctl_t *psPeriphCtl = NULL;

    if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_transfer;
    else if (!u32TransferCnt)
        goto exit_nu_pdma_transfer;
    else if (u32TransferCnt > NU_PDMA_MAX_TXCNT)
        return _nu_pdma_transfer_chain(i32ChannID, u32DataWidth, u32AddrSrc, u32AddrDst, u32TransferCnt, u32IdleTimeout_us);

    psPdmaChann = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos];
    psPeriphCtl = &psPdmaChann->m_spPeripCtl;

    head = &PDMA->DSCT[NU_PDMA_GET_MOD_CHIDX(i32ChannID)];

#if 0
    printf("DDDD PDMA %p \n", PDMA);
    printf("DDDD psPdmaChann %p \n", psPdmaChann);
    printf("DDDD head %p \n", head);
    printf("DDDD u32DataWidth %d\n", u32DataWidth);
    printf("DDDD u32AddrSrc %x\n", u32AddrSrc);
    printf("DDDD u32AddrDst %x \n", u32AddrDst);
    printf("DDDD u32TransferCnt %d \n", u32TransferCnt);
    printf("DDDD i32ChannID %d \n", i32ChannID);
#endif

    ret = nu_pdma_desc_setup(i32ChannID,
                             head,
                             u32DataWidth,
                             u32AddrSrc,
                             u32AddrDst,
                             u32TransferCnt,
                             NULL,
                             0);

    if (ret != 0)
        goto exit_nu_pdma_transfer;

    _nu_pdma_transfer(i32ChannID, psPeriphCtl->m_u32Peripheral, head, u32IdleTimeout_us);

    ret = 0;

exit_nu_pdma_transfer:

    return -(ret);
}

int nu_pdma_sg_transfer(int i32ChannID, nu_pdma_desc_t head, uint32_t u32IdleTimeout_us)
{
    int ret = 1;
    nu_pdma_periph_ctl_t *psPeriphCtl = NULL;

    if (!head)
        goto exit_nu_pdma_sg_transfer;
    else if (nu_pdma_check_is_nonallocated(i32ChannID))
        goto exit_nu_pdma_sg_transfer;

    psPeriphCtl = &nu_pdma_chn_arr[i32ChannID - NU_PDMA_CH_Pos].m_spPeripCtl;

    _nu_pdma_transfer(i32ChannID, psPeriphCtl->m_u32Peripheral, head, u32IdleTimeout_us);

    ret = 0;

exit_nu_pdma_sg_transfer:

    return -(ret);
}

void Handle_PDMA_Irq(PDMA_T *PDMA)
{
    int i;

    uint32_t intsts = PDMA_GET_INT_STATUS(PDMA);
    uint32_t abtsts = PDMA_GET_ABORT_STS(PDMA);
    uint32_t tdsts  = PDMA_GET_TD_STS(PDMA);
    uint32_t unalignsts  = PDMA_GET_ALIGN_STS(PDMA);
    uint32_t reqto  = intsts & PDMA_INTSTS_REQTOFn_Msk;
    uint32_t reqto_ch = (reqto >> PDMA_INTSTS_REQTOFn_Pos);

    int allch_sts = (reqto_ch | tdsts | abtsts | unalignsts);

    // Abort
    if (intsts & PDMA_INTSTS_ABTIF_Msk) {
        // Clear all Abort flags
        PDMA_CLR_ABORT_FLAG(PDMA, abtsts);
    }

    // Transfer done
    if (intsts & PDMA_INTSTS_TDIF_Msk) {
        // Clear all transfer done flags
        PDMA_CLR_TD_FLAG(PDMA, tdsts);
    }

    // Unaligned
    if (intsts & PDMA_INTSTS_ALIGNF_Msk) {
        // Clear all Unaligned flags
        PDMA_CLR_ALIGN_FLAG(PDMA, unalignsts);
    }

    // Timeout
    if (reqto) {
        // Clear all Timeout flags
        PDMA->INTSTS = reqto;
    }

    // Find the position of first '1' in allch_sts.
    while ((i = nu_ctz(allch_sts)) < PDMA_CH_MAX) {
        int module_id = ((uint32_t)PDMA - PDMA0_BASE) / PDMA_BASE_OFFSET;
        int j = i + (module_id * PDMA_CH_MAX);
        int ch_mask = (1 << i);

        if (nu_pdma_chn_mask_arr[module_id] & ch_mask) {
            int ch_event = 0;
            nu_pdma_chn_t *dma_chn = nu_pdma_chn_arr + j - NU_PDMA_CH_Pos;

            if (dma_chn->m_sCB_Event.m_pfnCBHandler) {
                if (abtsts & ch_mask) {
                    ch_event |= NU_PDMA_EVENT_ABORT;
                }

                if (tdsts & ch_mask) {
                    ch_event |= NU_PDMA_EVENT_TRANSFER_DONE;
                }

                if (unalignsts & ch_mask) {
                    ch_event |= NU_PDMA_EVENT_ALIGNMENT;
                }

                if (reqto_ch & ch_mask) {
                    PDMA_DisableTimeout(PDMA,  ch_mask);
                    ch_event |= NU_PDMA_EVENT_TIMEOUT;
                }

                if (dma_chn->m_sCB_Disable.m_pfnCBHandler)
                    dma_chn->m_sCB_Disable.m_pfnCBHandler(dma_chn->m_sCB_Disable.m_pvUserData, dma_chn->m_sCB_Disable.m_u32Reserved);

                if (dma_chn->m_u32EventFilter & ch_event)
                    dma_chn->m_sCB_Event.m_pfnCBHandler(dma_chn->m_sCB_Event.m_pvUserData, ch_event);

                if (reqto_ch & ch_mask)
                    nu_pdma_timeout_set(j, nu_pdma_chn_arr[j - NU_PDMA_CH_Pos].m_u32IdleTimeout_us);

            }//if(dma_chn->handler)

        } //if (nu_pdma_chn_mask & ch_mask)

        // Clear the served bit.
        allch_sts &= ~ch_mask;

    } //while
}

static void nu_pdma_memfun_actor_init(void)
{
    int i = 0 ;
    nu_pdma_init();

    for (i = 0; i < NU_PDMA_MEMFUN_ACTOR_MAX; i++) {
        memset(&nu_pdma_memfun_actor_arr[i], 0, sizeof(struct nu_pdma_memfun_actor));

        if (-(1) != (nu_pdma_memfun_actor_arr[i].m_i32ChannID = nu_pdma_channel_allocate(PDMA_MEM, PDMA_CNT - 1, PDMA_CH_MAX - 1 - i))) {
#if defined(OS_FREERTOS)
            nu_pdma_memfun_actor_arr[i].m_psSemMemFun = xSemaphoreCreateBinary();
            PDMA_ASSERT(nu_pdma_memfun_actor_arr[i].m_psSemMemFun != NULL);
#else
            nu_pdma_memfun_actor_arr[i].m_psSemMemFun = 0;
#endif
        } else
            break;
    }

    if (i) {
        nu_pdma_memfun_actor_maxnum = i;
        nu_pdma_memfun_actor_mask = ~(((1 << i) - 1));
    }
}

static void nu_pdma_memfun_cb(void *pvUserData, uint32_t u32Events)
{
    nu_pdma_memfun_actor_t psMemFunActor = (nu_pdma_memfun_actor_t)pvUserData;
    psMemFunActor->m_u32Result = u32Events;

#if defined(OS_FREERTOS)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(psMemFunActor->m_psSemMemFun, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#else
    psMemFunActor->m_psSemMemFun = 1;
#endif
}

static int nu_pdma_memfun_employ(void)
{
    int idx = -1;

    /* Headhunter */
    {
        /* Find the position of first '0' in nu_pdma_memfun_actor_mask. */
        idx = nu_cto(nu_pdma_memfun_actor_mask);

        if (idx != 32) {
            nu_pdma_memfun_actor_mask |= (1 << idx);
        } else {
            idx = -1;
        }
    }

    return idx;
}

static int nu_pdma_memfun(void *dest, void *src, uint32_t u32DataWidth, unsigned int u32TransferCnt, nu_pdma_memctrl_t eMemCtl)
{
    static int i32memActorInited = 0;

    nu_pdma_memfun_actor_t psMemFunActor = NULL;
    struct nu_pdma_chn_cb sChnCB;
    int idx, ret = 0;

    if (!i32memActorInited) {
        nu_pdma_memfun_actor_init();
        i32memActorInited = 1;
    }

    /* Employ actor */
    while ((idx = nu_pdma_memfun_employ()) < 0);

    psMemFunActor = &nu_pdma_memfun_actor_arr[idx];

    /* Set PDMA memory control to eMemCtl. */
    nu_pdma_channel_memctrl_set(psMemFunActor->m_i32ChannID, eMemCtl);

    /* Register ISR callback function */
    sChnCB.m_eCBType = eCBType_Event;
    sChnCB.m_pfnCBHandler = nu_pdma_memfun_cb;
    sChnCB.m_pvUserData = (void *)psMemFunActor;

    nu_pdma_filtering_set(psMemFunActor->m_i32ChannID, NU_PDMA_EVENT_ABORT | NU_PDMA_EVENT_TRANSFER_DONE);
    nu_pdma_callback_register(psMemFunActor->m_i32ChannID, &sChnCB);

    psMemFunActor->m_u32Result = 0;

    /* Trigger it */
    nu_pdma_transfer(psMemFunActor->m_i32ChannID,
                     u32DataWidth,
                     (uint32_t)src,
                     (uint32_t)dest,
                     u32TransferCnt,
                     0);

    /* Wait it done. */
#if defined(OS_FREERTOS)
    xSemaphoreTake(psMemFunActor->m_psSemMemFun, portMAX_DELAY);
#else

    while (psMemFunActor->m_psSemMemFun == 0);

    psMemFunActor->m_psSemMemFun = 0;
#endif

    /* Give result if get NU_PDMA_EVENT_TRANSFER_DONE.*/
    if (psMemFunActor->m_u32Result & NU_PDMA_EVENT_TRANSFER_DONE) {
        ret +=  u32TransferCnt;
    } else {
        ret += (u32TransferCnt - nu_pdma_non_transfer_count_get(psMemFunActor->m_i32ChannID));
    }

    /* Terminate it if get ABORT event */
    if (psMemFunActor->m_u32Result & NU_PDMA_EVENT_ABORT) {
        nu_pdma_channel_terminate(psMemFunActor->m_i32ChannID);
    }

    nu_pdma_memfun_actor_mask &= ~(1 << idx);

    return ret;
}

int nu_pdma_mempush(void *dest, void *src, uint32_t data_width, unsigned int transfer_count)
{
    if (data_width == 8 || data_width == 16 || data_width == 32)
        return nu_pdma_memfun(dest, src, data_width, transfer_count, eMemCtl_SrcInc_DstFix);

    return 0;
}

void *nu_pdma_memcpy(void *dest, void *src, unsigned int count)
{
    int i = 0;
    uint32_t u32Offset = 0;
    uint32_t u32Remaining = count;

    for (i = 4; (i > 0) && (u32Remaining > 0) ; i >>= 1) {
        uint32_t u32src   = (uint32_t)src + u32Offset;
        uint32_t u32dest  = (uint32_t)dest + u32Offset;

        if (((u32src % i) == (u32dest % i)) &&
            ((u32src % i) == 0) &&
            (NVT_ALIGN_DOWN(u32Remaining, i) >= i)) {
            uint32_t u32TXCnt = u32Remaining / i;

            if (u32TXCnt != nu_pdma_memfun((void *)u32dest, (void *)u32src, i * 8, u32TXCnt, eMemCtl_SrcInc_DstInc))
                goto exit_nu_pdma_memcpy;

            u32Offset += (u32TXCnt * i);
            u32Remaining -= (u32TXCnt * i);
        }
    }

    if (count == u32Offset)
        return dest;

exit_nu_pdma_memcpy:

    return NULL;
}

