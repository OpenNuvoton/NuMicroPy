/**
 * @file     M5531_CANFD.c
 * @version  V0.01
 * @brief    M5531 series CANFD HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "py/mphal.h"

#include "NuMicro.h"
#include "nu_modutil.h"
#include "nu_bitutil.h"
#include "nu_miscutil.h"

#include "M5531_CANFD.h"

#define MAX_CANFD_INST 2

typedef struct nu_canfd_var {
    canfd_t *     obj;
} s_nu_canfd_var;

static struct nu_canfd_var canfd0_var = {
    .obj                =   NULL,
};

static struct nu_canfd_var canfd1_var = {
    .obj                =   NULL,
};

static const struct nu_modinit_s canfd_modinit_tab[] = {
    {(uint32_t)CANFD0, CANFD0_MODULE, CLK_CANFDSEL_CANFD0SEL_APLL0_DIV2, CLK_CANFDDIV_CANFD0DIV(1), SYS_CANFD0RST, CANFD00_IRQn, &canfd0_var},
    {(uint32_t)CANFD1, CANFD1_MODULE, CLK_CANFDSEL_CANFD1SEL_APLL0_DIV2, CLK_CANFDDIV_CANFD1DIV(1), SYS_CANFD1RST, CANFD10_IRQn, &canfd1_var},

    {0, 0, 0, 0, 0, (IRQn_Type) 0, NULL}
};

int32_t CANFD_Init(
    canfd_t *psCANFDObj,
    CANFD_InitTypeDef *psInitDef
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psCANFDObj->canfd, canfd_modinit_tab);

    if(modinit == NULL)
        return -1;

    // Reset this module
    SYS_ResetModule(modinit->rsetidx);

    /* Select CAN FD0 clock source is APLL0/2 */
    CLK_SetModuleClock(modinit->clkidx, modinit->clksrc, modinit->clkdiv);
    CLK_EnableModuleClock(modinit->clkidx);

    NVIC_EnableIRQ(modinit->irq_n);

    CANFD_FD_T sCANFD_Config;

    /* Use defined configuration */
    sCANFD_Config.sElemSize.u32UserDef = 0;
    /* Get the CAN FD configuration value */
    CANFD_GetDefaultConfig(&sCANFD_Config, psInitDef->FDMode);
    sCANFD_Config.sBtConfig.bEnableLoopBack = psInitDef->LoopBack;
    sCANFD_Config.sBtConfig.sNormBitRate.u32BitRate = psInitDef->NormalBitRate;
    sCANFD_Config.sBtConfig.sDataBitRate.u32BitRate = psInitDef->DataBitRate;

    /* Open the CAN FD feature */
    CANFD_Open(psCANFDObj->canfd, &sCANFD_Config);

    /* CAN FD Run to Normal mode */
    CANFD_RunToNormal(psCANFDObj->canfd, TRUE);

    return 0;
}

void CANFD_Final(
    canfd_t *psCANFDObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psCANFDObj->canfd, canfd_modinit_tab);

    if(modinit == NULL)
        return;

    /* Enable I2C0 module clock */
    CLK_DisableModuleClock(modinit->clkidx);
    NVIC_DisableIRQ(modinit->irq_n);

    /* CAN FD Run to Normal mode */
    CANFD_RunToNormal(psCANFDObj->canfd, FALSE);

    uint32_t u32INTMask = CANFD_IE_BOE_Msk | CANFD_IE_EWE_Msk | CANFD_IE_EPE_Msk;

    CANFD_DisableInt(psCANFDObj->canfd, u32INTMask, u32INTMask, 0xFFFFFFFF, 0xFFFFFFFF);

    CANFD_Close(psCANFDObj->canfd);
    psCANFDObj->i32FIFOIdx = -1;
    psCANFDObj->u32StdFilterIdx = 0;
    psCANFDObj->u32ExtFilterIdx = 0;
}

void CANFD_Restart(
    canfd_t *psCANFDObj
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psCANFDObj->canfd, canfd_modinit_tab);

    if(modinit == NULL)
        return;

    CANFD_RunToNormal(psCANFDObj->canfd, FALSE);
    mp_hal_delay_ms(100);
    CANFD_RunToNormal(psCANFDObj->canfd, TRUE);
}

uint32_t CANFD_AmountDataRecv(
    canfd_t *psCANFDObj
)
{
    uint32_t u32RecvData = 0;
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psCANFDObj->canfd, canfd_modinit_tab);

    if(modinit == NULL)
        return 0;

    if(psCANFDObj->i32FIFOIdx == 0) {
        u32RecvData = ((psCANFDObj->canfd->RXF0C & CANFD_RXF0S_F0FL_Msk) >> CANFD_RXF0S_F0FL_Pos);
    } else if(psCANFDObj->i32FIFOIdx == 1) {
        u32RecvData = ((psCANFDObj->canfd->RXF1C & CANFD_RXF1S_F1FL_Msk) >> CANFD_RXF1S_F1FL_Pos);
    }

    return u32RecvData;
}

//Set receive filter
int32_t CANFD_SetRecvFilter(
    canfd_t *psCANFDObj,
    E_CANFD_ID_TYPE eIDType,
    uint32_t u32ID,
    uint32_t u32Mask
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psCANFDObj->canfd, canfd_modinit_tab);

    if(modinit == NULL)
        return -1;

    /* Reject Non-Matching Standard ID and Extended ID Filter(RX fifo1) */
    CANFD_SetGFC(CANFD0, eCANFD_REJ_NON_MATCH_FRM, eCANFD_REJ_NON_MATCH_FRM, 1, 1);

    if(psCANFDObj->canfd == CANFD0) {
        if(eIDType == eCANFD_SID) {
            CANFD_SetSIDFltr(CANFD0, psCANFDObj->u32StdFilterIdx, CANFD_RX_FIFO0_STD_MASK(u32ID, u32Mask));
            psCANFDObj->u32StdFilterIdx ++;
        } else {
            CANFD_SetXIDFltr(CANFD0, psCANFDObj->u32ExtFilterIdx, CANFD_RX_FIFO0_EXT_MASK_LOW(u32ID), CANFD_RX_FIFO0_EXT_MASK_HIGH(u32Mask));
            psCANFDObj->u32ExtFilterIdx ++;
        }

        psCANFDObj->i32FIFOIdx = 0;
        return 0;
    }

    if(eIDType == eCANFD_SID) {
        CANFD_SetSIDFltr(CANFD1, psCANFDObj->u32StdFilterIdx, CANFD_RX_FIFO1_STD_MASK(u32ID, u32Mask));
        psCANFDObj->u32StdFilterIdx ++;
    } else {
        CANFD_SetXIDFltr(CANFD1, psCANFDObj->u32ExtFilterIdx, CANFD_RX_FIFO1_EXT_MASK_LOW(u32ID), CANFD_RX_FIFO1_EXT_MASK_HIGH(u32Mask));
        psCANFDObj->u32ExtFilterIdx ++;
    }

    psCANFDObj->i32FIFOIdx = 1;
    return 0;
}


//Reture available message object index. available: 0~31
int32_t CANFD_GetFreeMsgObjIdx(
    canfd_t *psCANFDObj,
    uint32_t u32Timeout		//millisecond
)
{
    int32_t i32MsgIdx = -1;
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psCANFDObj->canfd, canfd_modinit_tab);

    if(modinit == NULL)
        return -1;


    uint32_t u32EndTick = mp_hal_ticks_ms() + u32Timeout;

    do {
        uint32_t u32TXBAR = psCANFDObj->canfd->TXBAR;

        for(int i = 0; i < 32; i ++) {
            if(!(u32TXBAR & (1UL << i))) {
                i32MsgIdx = i;
                return i32MsgIdx;
            }
        }

        MICROPY_THREAD_YIELD();
    } while(mp_hal_ticks_ms() <= u32EndTick);


    return i32MsgIdx;
}

//Receive message with timeout
int32_t CANFD_RecvMsg(
    canfd_t *psCANFDObj,
    CANFD_FD_MSG_T	*psMsgFrame,
    uint32_t u32Timeout		//millisecond
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psCANFDObj->canfd, canfd_modinit_tab);

    if(modinit == NULL)
        return -1;


    uint32_t u32EndTick = mp_hal_ticks_ms() + u32Timeout;
    uint32_t u32TotalMsg = 0;

    do {

        u32TotalMsg = CANFD_AmountDataRecv(psCANFDObj);

        if(u32TotalMsg > 0)
            break;

        MICROPY_THREAD_YIELD();
    } while(mp_hal_ticks_ms() <= u32EndTick);

    if(u32TotalMsg <= 0)
        return -2;		//Timeout

    if(CANFD_ReadRxFifoMsg(CANFD0, psCANFDObj->i32FIFOIdx, psMsgFrame) == 0)
        return -3;		//Rx FIFI empty

    return 0;
}

//Enable status interrupt
int32_t CANFD_EnableStatusInt(
    canfd_t *psCANFDObj,
    PFN_STATUS_INT_HANDLER pfnHandler,
    uint32_t u32INTMask
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psCANFDObj->canfd, canfd_modinit_tab);

    if(modinit == NULL)
        return -1;

    struct nu_canfd_var *var = (struct nu_canfd_var *) modinit->var;

    var->obj = psCANFDObj;
    psCANFDObj->pfnStatusHandler = pfnHandler;
    /* Fixed vector table is fixed in ROM and cannot be modified. */
    NVIC_EnableIRQ(modinit->irq_n);

    CANFD_EnableInt(psCANFDObj->canfd, u32INTMask, 0, 0, 0);
    return 0;
}

//Disable status interrupt
void CANFD_DisableStatusInt(
    canfd_t *psCANFDObj,
    uint32_t u32INTMask
)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)psCANFDObj->canfd, canfd_modinit_tab);

    if(modinit == NULL)
        return;

    struct nu_canfd_var *var = (struct nu_canfd_var *) modinit->var;

    CANFD_DisableInt(psCANFDObj->canfd, u32INTMask, 0, 0, 0);
    NVIC_DisableIRQ(modinit->irq_n);
    psCANFDObj->pfnStatusHandler = NULL;
    var->obj = NULL;
}

/**
 * Handle the CANFD interrupt
 * @param[in] obj The CANFD peripheral that generated the interrupt
 * @return
 */

void Handle_CANFD_Irq(CANFD_T *canfd, uint32_t u32Status)
{
    const struct nu_modinit_s *modinit = get_modinit((uint32_t)canfd, canfd_modinit_tab);

    if(modinit == NULL)
        return;

    struct nu_canfd_var *var = (struct nu_canfd_var *) modinit->var;

    canfd_t *psCANFDObj = var->obj;

    if((psCANFDObj) && (psCANFDObj->pfnStatusHandler)) {
        psCANFDObj->pfnStatusHandler(psCANFDObj, u32Status);
    }
}





