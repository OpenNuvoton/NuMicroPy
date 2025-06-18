/**************************************************************************//**
 * @file     M5531_DAC.c
 * @version  V0.01
 * @brief    M5531 series DAC HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "NuMicro.h"

#include "M5531_DAC.h"
#include "nu_modutil.h"
#include "drv_pdma.h"

struct nu_dac_var {
    dac_t *     psObj;
    uint8_t     pdma_perp_tx;

    int         dma_chn_id_tx;
    uint32_t	dma_data_width;
    uint32_t	dma_trans_len;
    uint8_t		*dma_tarns_buf;

};

static struct nu_dac_var dac0_var = {
    .psObj              =   NULL,
    .pdma_perp_tx       =   PDMA_DAC0_TX,
    .dma_chn_id_tx		=	NU_PDMA_OUT_OF_CHANNELS,
};

static struct nu_dac_var dac1_var = {
    .psObj                =   NULL,
    .pdma_perp_tx       =   PDMA_DAC1_TX,
    .dma_chn_id_tx		=	NU_PDMA_OUT_OF_CHANNELS,
};

static const struct nu_modinit_s dac_modinit_tab[] = {
    {(uint32_t)DAC0, DAC01_MODULE, MODULE_NoMsk, MODULE_NoMsk, SYS_DAC01RST, DAC01_IRQn, &dac0_var},
    {(uint32_t)DAC1, DAC01_MODULE, MODULE_NoMsk, MODULE_NoMsk, SYS_DAC01RST, DAC01_IRQn, &dac1_var},

    {0, 0, 0, 0, 0, (IRQn_Type) 0, NULL}
};

int32_t DAC_Init(
    dac_t *psObj,
    DAC_InitTypeDef *psInitDef
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->dac, dac_modinit_tab);

    if(modinit == NULL)
        return -1;

    struct nu_dac_var *dac_var = (struct nu_dac_var *)modinit->var;

    // Reset this module
    SYS_ResetModule(modinit->rsetidx);

    // Enable IP clock
    CLK_EnableModuleClock(modinit->clkidx);

    dac_var->psObj = psObj;
    dac_var->dma_chn_id_tx = NU_PDMA_OUT_OF_CHANNELS;
    dac_var->dma_data_width = 8;
    dac_var->dma_trans_len = 0;
    dac_var->dma_tarns_buf = NULL;

    /* Set the software trigger DAC and enable D/A converter */
    DAC_Open(psObj->dac, 0, psInitDef->u32TriggerMode);

    /* The DAC conversion settling time is 1us */
    DAC_SetDelayTime(psObj->dac, 1);

    /* Clear the DAC conversion complete finish flag for safe */
    DAC_CLR_INT_FLAG(psObj->dac, 0);

    if(psInitDef->eBitWidth == eDAC_BITWIDTH_8) {
        (psObj->dac)->CTL |= DAC_CTL_BWSEL_Msk;
    } else {
        (psObj->dac)->CTL &= ~DAC_CTL_BWSEL_Msk;
    }

    return 0;
}

void DAC_Final(
    dac_t *psObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->dac, dac_modinit_tab);

    if(modinit == NULL)
        return;

    struct nu_dac_var *dac_var = (struct nu_dac_var *)modinit->var;

    dac_var->psObj = NULL;

    if (dac_var->dma_chn_id_tx != NU_PDMA_OUT_OF_CHANNELS) {
        nu_pdma_channel_free(dac_var->dma_chn_id_tx);
        dac_var->dma_chn_id_tx = NU_PDMA_OUT_OF_CHANNELS;
    }

    DAC_Close(psObj->dac, 0);

    /* Disable module clock */
    CLK_DisableModuleClock(modinit->clkidx);
}

int32_t DAC_SingleConv(
    dac_t *psObj,
    uint32_t u32Data
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->dac, dac_modinit_tab);

    if(modinit == NULL)
        return -1;

    DAC_WRITE_DATA(psObj->dac, 0, u32Data);

    /* Start A/D conversion */
    DAC_START_CONV(psObj->dac);
    return 0;
}

static void DAC_DMA_Handler_TX(void* id, uint32_t event_dma)
{
    dac_t *psObj = (dac_t *) id;

    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->dac, dac_modinit_tab);
    if(modinit == NULL)
        return;

    struct nu_dac_var *dac_var = (struct nu_dac_var *)modinit->var;

    if(dac_var->psObj != psObj)
        return;

    // TODO: Pass this error to caller
    if (event_dma & NU_PDMA_EVENT_ABORT) {
    }

    // Expect UART IRQ will catch this transfer done event
    if (event_dma & NU_PDMA_EVENT_TRANSFER_DONE) {
        //retrigger transfer
        if(dac_var->dma_tarns_buf) {
            nu_pdma_transfer( dac_var->dma_chn_id_tx,
                              dac_var->dma_data_width,
                              (uint32_t)dac_var->dma_tarns_buf,
                              (uint32_t)&(psObj->dac)->DAT,
                              dac_var->dma_trans_len,
                              0
                            );

        }
    }
    // TODO: Pass this error to caller
    if (event_dma & NU_PDMA_EVENT_TIMEOUT) {

    }


}

int32_t DAC_TimerPDMAConv(
    dac_t *psObj,
    TIMER_T *timer,
    uint32_t u32TriggerMode,
    E_DAC_BITWIDTH eBitWidth,
    uint8_t *pu8Buf,
    uint32_t u32BufLen,
    bool bCircular
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->dac, dac_modinit_tab);

    if(modinit == NULL)
        return -1;

    struct nu_dac_var *dac_var = (struct nu_dac_var *)modinit->var;

    if(dac_var->dma_chn_id_tx != NU_PDMA_OUT_OF_CHANNELS)
        nu_pdma_channel_free(dac_var->dma_chn_id_tx);

    DAC_InitTypeDef dacInit;

    dacInit.u32TriggerMode = u32TriggerMode;
    dacInit.eBitWidth = eBitWidth;

    //Re-init dac
    DAC_Init(psObj, &dacInit);

    dac_var->dma_chn_id_tx = nu_pdma_channel_dynamic_allocate(dac_var->pdma_perp_tx);

    if(dac_var->dma_chn_id_tx == NU_PDMA_OUT_OF_CHANNELS)
        return -1;

    nu_pdma_filtering_set(dac_var->dma_chn_id_tx, NU_PDMA_EVENT_TRANSFER_DONE | NU_PDMA_EVENT_TIMEOUT);

    if(bCircular) {
        struct nu_pdma_chn_cb pdma_tx_chn_cb;

        pdma_tx_chn_cb.m_eCBType = eCBType_Event;
        pdma_tx_chn_cb.m_pfnCBHandler = DAC_DMA_Handler_TX;
        pdma_tx_chn_cb.m_pvUserData = psObj;

        nu_pdma_callback_register(dac_var->dma_chn_id_tx, &pdma_tx_chn_cb);
    }

    dac_var->dma_tarns_buf = pu8Buf;

    if(eBitWidth == eDAC_BITWIDTH_8) {
        dac_var->dma_data_width = 8;
        dac_var->dma_trans_len = u32BufLen;
    } else {
        dac_var->dma_data_width = 16;
        dac_var->dma_trans_len = u32BufLen / sizeof(uint16_t);
    }

    nu_pdma_transfer( dac_var->dma_chn_id_tx,
                      dac_var->dma_data_width,
                      (uint32_t)dac_var->dma_tarns_buf,
                      (uint32_t)&(psObj->dac)->DAT,
                      dac_var->dma_trans_len,
                      0
                    );


    /* Enable the PDMA Mode */
    DAC_ENABLE_PDMA(psObj->dac);

    // timer have been opened, just stop and reset it
    TIMER_Stop(timer);
    TIMER_ResetCounter(timer);
    TIMER_SetTriggerTarget(timer, TIMER_TRG_TO_DAC);
    TIMER_Start(timer);

    return 0;
}


int32_t DAC_StopTimerPDMAConv(
    dac_t *psObj,
    TIMER_T *timer
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psObj->dac, dac_modinit_tab);

    if(modinit == NULL)
        return -1;

    struct nu_dac_var *dac_var = (struct nu_dac_var *)modinit->var;

    TIMER_Stop(timer);
    DAC_DISABLE_PDMA(psObj->dac);

    if (dac_var->dma_chn_id_tx != NU_PDMA_OUT_OF_CHANNELS) {
        nu_pdma_channel_free(dac_var->dma_chn_id_tx);
        dac_var->dma_chn_id_tx = NU_PDMA_OUT_OF_CHANNELS;
        dac_var->dma_tarns_buf = NULL;
        dac_var->dma_data_width = 8;
        dac_var->dma_trans_len = 0;
    }

    return 0;
}





