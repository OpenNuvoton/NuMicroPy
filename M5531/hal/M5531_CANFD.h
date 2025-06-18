/**************************************************************************//**
 * @file     M5531_CANFD.h
 * @version  V1.00
 * @brief    M5531 CANFD HAL header file for micropython
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2025 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __M5531_CANFD_H__
#define __M5531_CANFD_H__

typedef void (*PFN_STATUS_INT_HANDLER)(void *obj, uint32_t u32Status);

typedef struct {
    CANFD_T *canfd;
    int32_t i32FIFOIdx;		//assigned >=0
    uint32_t u32StdFilterIdx; 	//filter index. start from 0;
    uint32_t u32ExtFilterIdx; 	//filter index. start from 0;
    PFN_STATUS_INT_HANDLER pfnStatusHandler;
} canfd_t;

typedef struct {
    bool FDMode;			//can/canfd mode
    bool LoopBack;			//loopback
    uint32_t NormalBitRate;	//Normal bit rate
    uint32_t DataBitRate;	//Data bit rate
} CANFD_InitTypeDef;

//Init CANFD object
int32_t CANFD_Init(
    canfd_t *psCANFDObj,
    CANFD_InitTypeDef *psInitDef
);

//Final CANFD object
void CANFD_Final(
    canfd_t *psCANFDObj
);

//Restart CANFD object. Force a software restart of the controller, to allow transmission after a bus error
void CANFD_Restart(
    canfd_t *psCANFDObj
);

//Return amount of data in receive fifo
uint32_t CANFD_AmountDataRecv(
    canfd_t *psCANFDObj
);

//Get available message object index. available: 0~31
int32_t CANFD_GetFreeMsgObjIdx(
    canfd_t *psCANFDObj,
    uint32_t u32Timeout		//millisecond
);

//Set receive filter
int32_t CANFD_SetRecvFilter(
    canfd_t *psCANFDObj,
    E_CANFD_ID_TYPE eIDType,
    uint32_t u32ID,
    uint32_t u32Mask
);

//Receive message with timeout
int32_t CANFD_RecvMsg(
    canfd_t *psCANFDObj,
    CANFD_FD_MSG_T	*psMsgFrame,
    uint32_t u32Timeout		//millisecond
);

//Enable status interrupt
int32_t CANFD_EnableStatusInt(
    canfd_t *psCANFDObj,
    PFN_STATUS_INT_HANDLER pfnHandler,
    uint32_t u32INTMask
);

//Disable status interrupt
void CANFD_DisableStatusInt(
    canfd_t *psCANFDObj,
    uint32_t u32INTMask
);

/**
 * Handle the CANFD interrupt
 * @param[in] obj The CANFD peripheral that generated the interrupt
 * @return
 */

void Handle_CANFD_Irq(CANFD_T *canfd, uint32_t u32Status);


#endif
