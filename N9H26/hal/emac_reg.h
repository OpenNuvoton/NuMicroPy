/**************************************************************************//**
 * @file     emac_reg.h
 * @version  V1.00
 * @brief    EMAC register definition header file
 *
 * @copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __EMAC_REG_H__
#define __EMAC_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/* IO definitions (access restrictions to peripheral registers) */
/**
    \defgroup CMSIS_glob_defs CMSIS Global Defines

    <strong>IO Type Qualifiers</strong> are used
    \li to specify the access to peripheral variables.
    \li for automatic generation of peripheral register debug information.
*/
#ifdef __cplusplus
  #define   __I     volatile             /*!< Defines 'read only' permissions */
#else
  #define   __I     volatile const       /*!< Defines 'read only' permissions */
#endif
#define     __O     volatile             /*!< Defines 'write only' permissions */
#define     __IO    volatile             /*!< Defines 'read / write' permissions */

/**
   @addtogroup REGISTER Control Register
   @{
*/

/**
    @addtogroup EMAC Ethernet MAC Controller(EMAC)
    Memory Mapped Structure for EMAC Controller
@{ */

typedef struct
{
    __IO uint32_t CAMCMR; /*CAMCTL; */    /*!< [0xE000] CAM Comparison Control Register                                  */
    __IO uint32_t CAMEN;                 /*!< [0xE004] CAM Enable Register                                              */
    __IO uint32_t CAM0M;                 /*!< [0xE008] CAM0 Most Significant Word Register                              */
    __IO uint32_t CAM0L;                 /*!< [0xE00c] CAM0 Least Significant Word Register                             */
    __IO uint32_t CAM1M;                 /*!< [0xE010] CAM1 Most Significant Word Register                              */
    __IO uint32_t CAM1L;                 /*!< [0xE014] CAM1 Least Significant Word Register                             */
    __IO uint32_t CAM2M;                 /*!< [0xE018] CAM2 Most Significant Word Register                              */
    __IO uint32_t CAM2L;                 /*!< [0xE01c] CAM2 Least Significant Word Register                             */
    __IO uint32_t CAM3M;                 /*!< [0xE020] CAM3 Most Significant Word Register                              */
    __IO uint32_t CAM3L;                 /*!< [0xE024] CAM3 Least Significant Word Register                             */
    __IO uint32_t CAM4M;                 /*!< [0xE028] CAM4 Most Significant Word Register                              */
    __IO uint32_t CAM4L;                 /*!< [0xE02c] CAM4 Least Significant Word Register                             */
    __IO uint32_t CAM5M;                 /*!< [0xE030] CAM5 Most Significant Word Register                              */
    __IO uint32_t CAM5L;                 /*!< [0xE034] CAM5 Least Significant Word Register                             */
    __IO uint32_t CAM6M;                 /*!< [0xE038] CAM6 Most Significant Word Register                              */
    __IO uint32_t CAM6L;                 /*!< [0xE03c] CAM6 Least Significant Word Register                             */
    __IO uint32_t CAM7M;                 /*!< [0xE040] CAM7 Most Significant Word Register                              */
    __IO uint32_t CAM7L;                 /*!< [0xE044] CAM7 Least Significant Word Register                             */
    __IO uint32_t CAM8M;                 /*!< [0xE048] CAM8 Most Significant Word Register                              */
    __IO uint32_t CAM8L;                 /*!< [0xE04c] CAM8 Least Significant Word Register                             */
    __IO uint32_t CAM9M;                 /*!< [0xE050] CAM9 Most Significant Word Register                              */
    __IO uint32_t CAM9L;                 /*!< [0xE054] CAM9 Least Significant Word Register                             */
    __IO uint32_t CAM10M;                /*!< [0xE058] CAM10 Most Significant Word Register                             */
    __IO uint32_t CAM10L;                /*!< [0xE05c] CAM10 Least Significant Word Register                            */
    __IO uint32_t CAM11M;                /*!< [0xE060] CAM11 Most Significant Word Register                             */
    __IO uint32_t CAM11L;                /*!< [0xE064] CAM11 Least Significant Word Register                            */
    __IO uint32_t CAM12M;                /*!< [0xE068] CAM12 Most Significant Word Register                             */
    __IO uint32_t CAM12L;                /*!< [0xE06c] CAM12 Least Significant Word Register                            */
    __IO uint32_t CAM13M;                /*!< [0xE070] CAM13 Most Significant Word Register                             */
    __IO uint32_t CAM13L;                /*!< [0xE074] CAM13 Least Significant Word Register                            */
    __IO uint32_t CAM14M;                /*!< [0xE078] CAM14 Most Significant Word Register                             */
    __IO uint32_t CAM14L;                /*!< [0xE07c] CAM14 Least Significant Word Register                            */
    __IO uint32_t CAM15M; /*CAM15MSB*/   /*!< [0xE080] CAM15 Most Significant Word Register                             */
    __IO uint32_t CAM15L; /*CAM15LSB*/   /*!< [0xE084] CAM15 Least Significant Word Register                            */
    __IO uint32_t TXDLSA; /*TXDSA*/      /*!< [0xE088] Transmit Descriptor Link List Start Address Register             */
    __IO uint32_t RXDLSA; /*RXDSA*/      /*!< [0xE08c] Receive Descriptor Link List Start Address Register              */
    __IO uint32_t MCMDR;  /*CTL*/        /*!< [0xE090] MAC Control Register                                             */
    __IO uint32_t MIID;   /*MIIMDAT*/    /*!< [0xE094] MII Management Data Register                                     */
    __IO uint32_t MIIDA;  /*MIIMCTL*/    /*!< [0xE098] MII Management Control and Address Register                      */
    __IO uint32_t FFTCR;  /*FIFOCTL*/    /*!< [0xE09c] FIFO Threshold Control Register                                  */
    __O  uint32_t TSDR;   /*TXST*/       /*!< [0xE0a0] Transmit Start Demand Register                                   */
    __O  uint32_t RSDR;   /*RXST*/       /*!< [0xE0a4] Receive Start Demand Register                                    */
    __IO uint32_t DMARFC; /*MRFL*/       /*!< [0xE0a8] Maximum Receive Frame Control Register                           */
    __IO uint32_t MIEN;	  /*INTEN*/      /*!< [0xE0ac] MAC Interrupt Enable Register                                    */
    __IO uint32_t MISTA;  /*INTSTS*/     /*!< [0xE0b0] MAC Interrupt Status Register                                    */
    __IO uint32_t MGSTA;  /*GENSTS*/     /*!< [0xE0b4] MAC General Status Register                                      */
    __IO uint32_t MPCNT;                 /*!< [0xE0b8] Missed Packet Count Register                                     */
    __I  uint32_t MRPC;   /*RPCNT*/      /*!< [0xE0bc] MAC Receive Pause Count Register                                 */
    __IO uint32_t MRPCC;                 /*!< [0xE0c0] MAC Receive Pause Current Count Register                         */
    __IO uint32_t MREPC;                 /*!< [0xE0c4] MAC Remote Pause Count Register                                  */
    __IO uint32_t DMARFS;  /*FRSTS*/     /*!< [0xE0c8] DMA Receive Frame Status Register                                */
    __I  uint32_t CTXDSA;                /*!< [0xE0cc] Current Transmit Descriptor Start Address Register               */
    __I  uint32_t CTXBSA;                /*!< [0xE0d0] Current Transmit Buffer Start Address Register                   */
    __I  uint32_t CRXDSA;                /*!< [0xE0d4] Current Receive Descriptor Start Address Register                */
    __I  uint32_t CRXBSA;                /*!< [0xE0d8] Current Receive Buffer Start Address Register                    */

} EMAC_T;

/**
    @addtogroup EMAC_CONST EMAC Bit Field Definition
    Constant Definitions for EMAC Controller
@{ */


//#define EMAC_CAMCTL_AUP_Pos              (0)                                               /*!< EMAC_T::CAMCTL: AUP Position           */
//#define EMAC_CAMCTL_AUP_Msk              (0x1ul << EMAC_CAMCTL_AUP_Pos)                    /*!< EMAC_T::CAMCTL: AUP Mask               */

//#define EMAC_CAMCTL_AMP_Pos              (1)                                               /*!< EMAC_T::CAMCTL: AMP Position           */
//#define EMAC_CAMCTL_AMP_Msk              (0x1ul << EMAC_CAMCTL_AMP_Pos)                    /*!< EMAC_T::CAMCTL: AMP Mask               */

//#define EMAC_CAMCTL_ABP_Pos              (2)                                               /*!< EMAC_T::CAMCTL: ABP Position           */
//#define EMAC_CAMCTL_ABP_Msk              (0x1ul << EMAC_CAMCTL_ABP_Pos)                    /*!< EMAC_T::CAMCTL: ABP Mask               */

//#define EMAC_CAMCTL_COMPEN_Pos           (3)                                               /*!< EMAC_T::CAMCTL: COMPEN Position        */
//#define EMAC_CAMCTL_COMPEN_Msk           (0x1ul << EMAC_CAMCTL_COMPEN_Pos)                 /*!< EMAC_T::CAMCTL: COMPEN Mask            */

//#define EMAC_CAMCTL_CMPEN_Pos            (4)                                               /*!< EMAC_T::CAMCTL: CMPEN Position         */
//#define EMAC_CAMCTL_CMPEN_Msk            (0x1ul << EMAC_CAMCTL_CMPEN_Pos)                  /*!< EMAC_T::CAMCTL: CMPEN Mask             */

#define EMAC_CAMCMR_AUP_Pos              (0)                                               /*!< EMAC_T::CAMCTL: AUP Position           */
#define EMAC_CAMCMR_AUP_Msk              (0x1ul << EMAC_CAMCMR_AUP_Pos)                    /*!< EMAC_T::CAMCTL: AUP Mask               */

#define EMAC_CAMCMR_AMP_Pos              (1)                                               /*!< EMAC_T::CAMCTL: AMP Position           */
#define EMAC_CAMCMR_AMP_Msk              (0x1ul << EMAC_CAMCMR_AMP_Pos)                    /*!< EMAC_T::CAMCTL: AMP Mask               */

#define EMAC_CAMCMR_ABP_Pos              (2)                                               /*!< EMAC_T::CAMCTL: ABP Position           */
#define EMAC_CAMCMR_ABP_Msk              (0x1ul << EMAC_CAMCMR_ABP_Pos)                    /*!< EMAC_T::CAMCTL: ABP Mask               */

#define EMAC_CAMCMR_CCAM_Pos             (3)                                               /*!< EMAC_T::CAMCTL: COMPEN Position        */
#define EMAC_CAMCMR_CCAM_Msk             (0x1ul << EMAC_CAMCMR_CCAM_Pos)                   /*!< EMAC_T::CAMCTL: COMPEN Mask            */

#define EMAC_CAMCMR_ECMP_Pos             (4)                                               /*!< EMAC_T::CAMCTL: CMPEN Position         */
#define EMAC_CAMCMR_ECMP_Msk             (0x1ul << EMAC_CAMCMR_ECMP_Pos)                   /*!< EMAC_T::CAMCTL: CMPEN Mask             */

#define EMAC_CAMEN_CAMxEN_Pos            (0)                                               /*!< EMAC_T::CAMEN: CAMxEN Position         */
#define EMAC_CAMEN_CAMxEN_Msk            (0x1ul << EMAC_CAMEN_CAMxEN_Pos)                  /*!< EMAC_T::CAMEN: CAMxEN Mask             */

#define EMAC_CAM0M_MACADDR2_Pos          (0)                                               /*!< EMAC_T::CAM0M: MACADDR2 Position       */
#define EMAC_CAM0M_MACADDR2_Msk          (0xfful << EMAC_CAM0M_MACADDR2_Pos)               /*!< EMAC_T::CAM0M: MACADDR2 Mask           */

#define EMAC_CAM0M_MACADDR3_Pos          (8)                                               /*!< EMAC_T::CAM0M: MACADDR3 Position       */
#define EMAC_CAM0M_MACADDR3_Msk          (0xfful << EMAC_CAM0M_MACADDR3_Pos)               /*!< EMAC_T::CAM0M: MACADDR3 Mask           */

#define EMAC_CAM0M_MACADDR4_Pos          (16)                                              /*!< EMAC_T::CAM0M: MACADDR4 Position       */
#define EMAC_CAM0M_MACADDR4_Msk          (0xfful << EMAC_CAM0M_MACADDR4_Pos)               /*!< EMAC_T::CAM0M: MACADDR4 Mask           */

#define EMAC_CAM0M_MACADDR5_Pos          (24)                                              /*!< EMAC_T::CAM0M: MACADDR5 Position       */
#define EMAC_CAM0M_MACADDR5_Msk          (0xfful << EMAC_CAM0M_MACADDR5_Pos)               /*!< EMAC_T::CAM0M: MACADDR5 Mask           */

#define EMAC_CAM0L_MACADDR0_Pos          (16)                                              /*!< EMAC_T::CAM0L: MACADDR0 Position       */
#define EMAC_CAM0L_MACADDR0_Msk          (0xfful << EMAC_CAM0L_MACADDR0_Pos)               /*!< EMAC_T::CAM0L: MACADDR0 Mask           */

#define EMAC_CAM0L_MACADDR1_Pos          (24)                                              /*!< EMAC_T::CAM0L: MACADDR1 Position       */
#define EMAC_CAM0L_MACADDR1_Msk          (0xfful << EMAC_CAM0L_MACADDR1_Pos)               /*!< EMAC_T::CAM0L: MACADDR1 Mask           */

#define EMAC_CAM1M_MACADDR2_Pos          (0)                                               /*!< EMAC_T::CAM1M: MACADDR2 Position       */
#define EMAC_CAM1M_MACADDR2_Msk          (0xfful << EMAC_CAM1M_MACADDR2_Pos)               /*!< EMAC_T::CAM1M: MACADDR2 Mask           */

#define EMAC_CAM1M_MACADDR3_Pos          (8)                                               /*!< EMAC_T::CAM1M: MACADDR3 Position       */
#define EMAC_CAM1M_MACADDR3_Msk          (0xfful << EMAC_CAM1M_MACADDR3_Pos)               /*!< EMAC_T::CAM1M: MACADDR3 Mask           */

#define EMAC_CAM1M_MACADDR4_Pos          (16)                                              /*!< EMAC_T::CAM1M: MACADDR4 Position       */
#define EMAC_CAM1M_MACADDR4_Msk          (0xfful << EMAC_CAM1M_MACADDR4_Pos)               /*!< EMAC_T::CAM1M: MACADDR4 Mask           */

#define EMAC_CAM1M_MACADDR5_Pos          (24)                                              /*!< EMAC_T::CAM1M: MACADDR5 Position       */
#define EMAC_CAM1M_MACADDR5_Msk          (0xfful << EMAC_CAM1M_MACADDR5_Pos)               /*!< EMAC_T::CAM1M: MACADDR5 Mask           */

#define EMAC_CAM1L_MACADDR0_Pos          (16)                                              /*!< EMAC_T::CAM1L: MACADDR0 Position       */
#define EMAC_CAM1L_MACADDR0_Msk          (0xfful << EMAC_CAM1L_MACADDR0_Pos)               /*!< EMAC_T::CAM1L: MACADDR0 Mask           */

#define EMAC_CAM1L_MACADDR1_Pos          (24)                                              /*!< EMAC_T::CAM1L: MACADDR1 Position       */
#define EMAC_CAM1L_MACADDR1_Msk          (0xfful << EMAC_CAM1L_MACADDR1_Pos)               /*!< EMAC_T::CAM1L: MACADDR1 Mask           */

#define EMAC_CAM2M_MACADDR2_Pos          (0)                                               /*!< EMAC_T::CAM2M: MACADDR2 Position       */
#define EMAC_CAM2M_MACADDR2_Msk          (0xfful << EMAC_CAM2M_MACADDR2_Pos)               /*!< EMAC_T::CAM2M: MACADDR2 Mask           */

#define EMAC_CAM2M_MACADDR3_Pos          (8)                                               /*!< EMAC_T::CAM2M: MACADDR3 Position       */
#define EMAC_CAM2M_MACADDR3_Msk          (0xfful << EMAC_CAM2M_MACADDR3_Pos)               /*!< EMAC_T::CAM2M: MACADDR3 Mask           */

#define EMAC_CAM2M_MACADDR4_Pos          (16)                                              /*!< EMAC_T::CAM2M: MACADDR4 Position       */
#define EMAC_CAM2M_MACADDR4_Msk          (0xfful << EMAC_CAM2M_MACADDR4_Pos)               /*!< EMAC_T::CAM2M: MACADDR4 Mask           */

#define EMAC_CAM2M_MACADDR5_Pos          (24)                                              /*!< EMAC_T::CAM2M: MACADDR5 Position       */
#define EMAC_CAM2M_MACADDR5_Msk          (0xfful << EMAC_CAM2M_MACADDR5_Pos)               /*!< EMAC_T::CAM2M: MACADDR5 Mask           */

#define EMAC_CAM2L_MACADDR0_Pos          (16)                                              /*!< EMAC_T::CAM2L: MACADDR0 Position       */
#define EMAC_CAM2L_MACADDR0_Msk          (0xfful << EMAC_CAM2L_MACADDR0_Pos)               /*!< EMAC_T::CAM2L: MACADDR0 Mask           */

#define EMAC_CAM2L_MACADDR1_Pos          (24)                                              /*!< EMAC_T::CAM2L: MACADDR1 Position       */
#define EMAC_CAM2L_MACADDR1_Msk          (0xfful << EMAC_CAM2L_MACADDR1_Pos)               /*!< EMAC_T::CAM2L: MACADDR1 Mask           */

#define EMAC_CAM3M_MACADDR2_Pos          (0)                                               /*!< EMAC_T::CAM3M: MACADDR2 Position       */
#define EMAC_CAM3M_MACADDR2_Msk          (0xfful << EMAC_CAM3M_MACADDR2_Pos)               /*!< EMAC_T::CAM3M: MACADDR2 Mask           */

#define EMAC_CAM3M_MACADDR3_Pos          (8)                                               /*!< EMAC_T::CAM3M: MACADDR3 Position       */
#define EMAC_CAM3M_MACADDR3_Msk          (0xfful << EMAC_CAM3M_MACADDR3_Pos)               /*!< EMAC_T::CAM3M: MACADDR3 Mask           */

#define EMAC_CAM3M_MACADDR4_Pos          (16)                                              /*!< EMAC_T::CAM3M: MACADDR4 Position       */
#define EMAC_CAM3M_MACADDR4_Msk          (0xfful << EMAC_CAM3M_MACADDR4_Pos)               /*!< EMAC_T::CAM3M: MACADDR4 Mask           */

#define EMAC_CAM3M_MACADDR5_Pos          (24)                                              /*!< EMAC_T::CAM3M: MACADDR5 Position       */
#define EMAC_CAM3M_MACADDR5_Msk          (0xfful << EMAC_CAM3M_MACADDR5_Pos)               /*!< EMAC_T::CAM3M: MACADDR5 Mask           */

#define EMAC_CAM3L_MACADDR0_Pos          (16)                                              /*!< EMAC_T::CAM3L: MACADDR0 Position       */
#define EMAC_CAM3L_MACADDR0_Msk          (0xfful << EMAC_CAM3L_MACADDR0_Pos)               /*!< EMAC_T::CAM3L: MACADDR0 Mask           */

#define EMAC_CAM3L_MACADDR1_Pos          (24)                                              /*!< EMAC_T::CAM3L: MACADDR1 Position       */
#define EMAC_CAM3L_MACADDR1_Msk          (0xfful << EMAC_CAM3L_MACADDR1_Pos)               /*!< EMAC_T::CAM3L: MACADDR1 Mask           */

#define EMAC_CAM4M_MACADDR2_Pos          (0)                                               /*!< EMAC_T::CAM4M: MACADDR2 Position       */
#define EMAC_CAM4M_MACADDR2_Msk          (0xfful << EMAC_CAM4M_MACADDR2_Pos)               /*!< EMAC_T::CAM4M: MACADDR2 Mask           */

#define EMAC_CAM4M_MACADDR3_Pos          (8)                                               /*!< EMAC_T::CAM4M: MACADDR3 Position       */
#define EMAC_CAM4M_MACADDR3_Msk          (0xfful << EMAC_CAM4M_MACADDR3_Pos)               /*!< EMAC_T::CAM4M: MACADDR3 Mask           */

#define EMAC_CAM4M_MACADDR4_Pos          (16)                                              /*!< EMAC_T::CAM4M: MACADDR4 Position       */
#define EMAC_CAM4M_MACADDR4_Msk          (0xfful << EMAC_CAM4M_MACADDR4_Pos)               /*!< EMAC_T::CAM4M: MACADDR4 Mask           */

#define EMAC_CAM4M_MACADDR5_Pos          (24)                                              /*!< EMAC_T::CAM4M: MACADDR5 Position       */
#define EMAC_CAM4M_MACADDR5_Msk          (0xfful << EMAC_CAM4M_MACADDR5_Pos)               /*!< EMAC_T::CAM4M: MACADDR5 Mask           */

#define EMAC_CAM4L_MACADDR0_Pos          (16)                                              /*!< EMAC_T::CAM4L: MACADDR0 Position       */
#define EMAC_CAM4L_MACADDR0_Msk          (0xfful << EMAC_CAM4L_MACADDR0_Pos)               /*!< EMAC_T::CAM4L: MACADDR0 Mask           */

#define EMAC_CAM4L_MACADDR1_Pos          (24)                                              /*!< EMAC_T::CAM4L: MACADDR1 Position       */
#define EMAC_CAM4L_MACADDR1_Msk          (0xfful << EMAC_CAM4L_MACADDR1_Pos)               /*!< EMAC_T::CAM4L: MACADDR1 Mask           */

#define EMAC_CAM5M_MACADDR2_Pos          (0)                                               /*!< EMAC_T::CAM5M: MACADDR2 Position       */
#define EMAC_CAM5M_MACADDR2_Msk          (0xfful << EMAC_CAM5M_MACADDR2_Pos)               /*!< EMAC_T::CAM5M: MACADDR2 Mask           */

#define EMAC_CAM5M_MACADDR3_Pos          (8)                                               /*!< EMAC_T::CAM5M: MACADDR3 Position       */
#define EMAC_CAM5M_MACADDR3_Msk          (0xfful << EMAC_CAM5M_MACADDR3_Pos)               /*!< EMAC_T::CAM5M: MACADDR3 Mask           */

#define EMAC_CAM5M_MACADDR4_Pos          (16)                                              /*!< EMAC_T::CAM5M: MACADDR4 Position       */
#define EMAC_CAM5M_MACADDR4_Msk          (0xfful << EMAC_CAM5M_MACADDR4_Pos)               /*!< EMAC_T::CAM5M: MACADDR4 Mask           */

#define EMAC_CAM5M_MACADDR5_Pos          (24)                                              /*!< EMAC_T::CAM5M: MACADDR5 Position       */
#define EMAC_CAM5M_MACADDR5_Msk          (0xfful << EMAC_CAM5M_MACADDR5_Pos)               /*!< EMAC_T::CAM5M: MACADDR5 Mask           */

#define EMAC_CAM5L_MACADDR0_Pos          (16)                                              /*!< EMAC_T::CAM5L: MACADDR0 Position       */
#define EMAC_CAM5L_MACADDR0_Msk          (0xfful << EMAC_CAM5L_MACADDR0_Pos)               /*!< EMAC_T::CAM5L: MACADDR0 Mask           */

#define EMAC_CAM5L_MACADDR1_Pos          (24)                                              /*!< EMAC_T::CAM5L: MACADDR1 Position       */
#define EMAC_CAM5L_MACADDR1_Msk          (0xfful << EMAC_CAM5L_MACADDR1_Pos)               /*!< EMAC_T::CAM5L: MACADDR1 Mask           */

#define EMAC_CAM6M_MACADDR2_Pos          (0)                                               /*!< EMAC_T::CAM6M: MACADDR2 Position       */
#define EMAC_CAM6M_MACADDR2_Msk          (0xfful << EMAC_CAM6M_MACADDR2_Pos)               /*!< EMAC_T::CAM6M: MACADDR2 Mask           */

#define EMAC_CAM6M_MACADDR3_Pos          (8)                                               /*!< EMAC_T::CAM6M: MACADDR3 Position       */
#define EMAC_CAM6M_MACADDR3_Msk          (0xfful << EMAC_CAM6M_MACADDR3_Pos)               /*!< EMAC_T::CAM6M: MACADDR3 Mask           */

#define EMAC_CAM6M_MACADDR4_Pos          (16)                                              /*!< EMAC_T::CAM6M: MACADDR4 Position       */
#define EMAC_CAM6M_MACADDR4_Msk          (0xfful << EMAC_CAM6M_MACADDR4_Pos)               /*!< EMAC_T::CAM6M: MACADDR4 Mask           */

#define EMAC_CAM6M_MACADDR5_Pos          (24)                                              /*!< EMAC_T::CAM6M: MACADDR5 Position       */
#define EMAC_CAM6M_MACADDR5_Msk          (0xfful << EMAC_CAM6M_MACADDR5_Pos)               /*!< EMAC_T::CAM6M: MACADDR5 Mask           */

#define EMAC_CAM6L_MACADDR0_Pos          (16)                                              /*!< EMAC_T::CAM6L: MACADDR0 Position       */
#define EMAC_CAM6L_MACADDR0_Msk          (0xfful << EMAC_CAM6L_MACADDR0_Pos)               /*!< EMAC_T::CAM6L: MACADDR0 Mask           */

#define EMAC_CAM6L_MACADDR1_Pos          (24)                                              /*!< EMAC_T::CAM6L: MACADDR1 Position       */
#define EMAC_CAM6L_MACADDR1_Msk          (0xfful << EMAC_CAM6L_MACADDR1_Pos)               /*!< EMAC_T::CAM6L: MACADDR1 Mask           */

#define EMAC_CAM7M_MACADDR2_Pos          (0)                                               /*!< EMAC_T::CAM7M: MACADDR2 Position       */
#define EMAC_CAM7M_MACADDR2_Msk          (0xfful << EMAC_CAM7M_MACADDR2_Pos)               /*!< EMAC_T::CAM7M: MACADDR2 Mask           */

#define EMAC_CAM7M_MACADDR3_Pos          (8)                                               /*!< EMAC_T::CAM7M: MACADDR3 Position       */
#define EMAC_CAM7M_MACADDR3_Msk          (0xfful << EMAC_CAM7M_MACADDR3_Pos)               /*!< EMAC_T::CAM7M: MACADDR3 Mask           */

#define EMAC_CAM7M_MACADDR4_Pos          (16)                                              /*!< EMAC_T::CAM7M: MACADDR4 Position       */
#define EMAC_CAM7M_MACADDR4_Msk          (0xfful << EMAC_CAM7M_MACADDR4_Pos)               /*!< EMAC_T::CAM7M: MACADDR4 Mask           */

#define EMAC_CAM7M_MACADDR5_Pos          (24)                                              /*!< EMAC_T::CAM7M: MACADDR5 Position       */
#define EMAC_CAM7M_MACADDR5_Msk          (0xfful << EMAC_CAM7M_MACADDR5_Pos)               /*!< EMAC_T::CAM7M: MACADDR5 Mask           */

#define EMAC_CAM7L_MACADDR0_Pos          (16)                                              /*!< EMAC_T::CAM7L: MACADDR0 Position       */
#define EMAC_CAM7L_MACADDR0_Msk          (0xfful << EMAC_CAM7L_MACADDR0_Pos)               /*!< EMAC_T::CAM7L: MACADDR0 Mask           */

#define EMAC_CAM7L_MACADDR1_Pos          (24)                                              /*!< EMAC_T::CAM7L: MACADDR1 Position       */
#define EMAC_CAM7L_MACADDR1_Msk          (0xfful << EMAC_CAM7L_MACADDR1_Pos)               /*!< EMAC_T::CAM7L: MACADDR1 Mask           */

#define EMAC_CAM8M_MACADDR2_Pos          (0)                                               /*!< EMAC_T::CAM8M: MACADDR2 Position       */
#define EMAC_CAM8M_MACADDR2_Msk          (0xfful << EMAC_CAM8M_MACADDR2_Pos)               /*!< EMAC_T::CAM8M: MACADDR2 Mask           */

#define EMAC_CAM8M_MACADDR3_Pos          (8)                                               /*!< EMAC_T::CAM8M: MACADDR3 Position       */
#define EMAC_CAM8M_MACADDR3_Msk          (0xfful << EMAC_CAM8M_MACADDR3_Pos)               /*!< EMAC_T::CAM8M: MACADDR3 Mask           */

#define EMAC_CAM8M_MACADDR4_Pos          (16)                                              /*!< EMAC_T::CAM8M: MACADDR4 Position       */
#define EMAC_CAM8M_MACADDR4_Msk          (0xfful << EMAC_CAM8M_MACADDR4_Pos)               /*!< EMAC_T::CAM8M: MACADDR4 Mask           */

#define EMAC_CAM8M_MACADDR5_Pos          (24)                                              /*!< EMAC_T::CAM8M: MACADDR5 Position       */
#define EMAC_CAM8M_MACADDR5_Msk          (0xfful << EMAC_CAM8M_MACADDR5_Pos)               /*!< EMAC_T::CAM8M: MACADDR5 Mask           */

#define EMAC_CAM8L_MACADDR0_Pos          (16)                                              /*!< EMAC_T::CAM8L: MACADDR0 Position       */
#define EMAC_CAM8L_MACADDR0_Msk          (0xfful << EMAC_CAM8L_MACADDR0_Pos)               /*!< EMAC_T::CAM8L: MACADDR0 Mask           */

#define EMAC_CAM8L_MACADDR1_Pos          (24)                                              /*!< EMAC_T::CAM8L: MACADDR1 Position       */
#define EMAC_CAM8L_MACADDR1_Msk          (0xfful << EMAC_CAM8L_MACADDR1_Pos)               /*!< EMAC_T::CAM8L: MACADDR1 Mask           */

#define EMAC_CAM9M_MACADDR2_Pos          (0)                                               /*!< EMAC_T::CAM9M: MACADDR2 Position       */
#define EMAC_CAM9M_MACADDR2_Msk          (0xfful << EMAC_CAM9M_MACADDR2_Pos)               /*!< EMAC_T::CAM9M: MACADDR2 Mask           */

#define EMAC_CAM9M_MACADDR3_Pos          (8)                                               /*!< EMAC_T::CAM9M: MACADDR3 Position       */
#define EMAC_CAM9M_MACADDR3_Msk          (0xfful << EMAC_CAM9M_MACADDR3_Pos)               /*!< EMAC_T::CAM9M: MACADDR3 Mask           */

#define EMAC_CAM9M_MACADDR4_Pos          (16)                                              /*!< EMAC_T::CAM9M: MACADDR4 Position       */
#define EMAC_CAM9M_MACADDR4_Msk          (0xfful << EMAC_CAM9M_MACADDR4_Pos)               /*!< EMAC_T::CAM9M: MACADDR4 Mask           */

#define EMAC_CAM9M_MACADDR5_Pos          (24)                                              /*!< EMAC_T::CAM9M: MACADDR5 Position       */
#define EMAC_CAM9M_MACADDR5_Msk          (0xfful << EMAC_CAM9M_MACADDR5_Pos)               /*!< EMAC_T::CAM9M: MACADDR5 Mask           */

#define EMAC_CAM9L_MACADDR0_Pos          (16)                                              /*!< EMAC_T::CAM9L: MACADDR0 Position       */
#define EMAC_CAM9L_MACADDR0_Msk          (0xfful << EMAC_CAM9L_MACADDR0_Pos)               /*!< EMAC_T::CAM9L: MACADDR0 Mask           */

#define EMAC_CAM9L_MACADDR1_Pos          (24)                                              /*!< EMAC_T::CAM9L: MACADDR1 Position       */
#define EMAC_CAM9L_MACADDR1_Msk          (0xfful << EMAC_CAM9L_MACADDR1_Pos)               /*!< EMAC_T::CAM9L: MACADDR1 Mask           */

#define EMAC_CAM10M_MACADDR2_Pos         (0)                                               /*!< EMAC_T::CAM10M: MACADDR2 Position      */
#define EMAC_CAM10M_MACADDR2_Msk         (0xfful << EMAC_CAM10M_MACADDR2_Pos)              /*!< EMAC_T::CAM10M: MACADDR2 Mask          */

#define EMAC_CAM10M_MACADDR3_Pos         (8)                                               /*!< EMAC_T::CAM10M: MACADDR3 Position      */
#define EMAC_CAM10M_MACADDR3_Msk         (0xfful << EMAC_CAM10M_MACADDR3_Pos)              /*!< EMAC_T::CAM10M: MACADDR3 Mask          */

#define EMAC_CAM10M_MACADDR4_Pos         (16)                                              /*!< EMAC_T::CAM10M: MACADDR4 Position      */
#define EMAC_CAM10M_MACADDR4_Msk         (0xfful << EMAC_CAM10M_MACADDR4_Pos)              /*!< EMAC_T::CAM10M: MACADDR4 Mask          */

#define EMAC_CAM10M_MACADDR5_Pos         (24)                                              /*!< EMAC_T::CAM10M: MACADDR5 Position      */
#define EMAC_CAM10M_MACADDR5_Msk         (0xfful << EMAC_CAM10M_MACADDR5_Pos)              /*!< EMAC_T::CAM10M: MACADDR5 Mask          */

#define EMAC_CAM10L_MACADDR0_Pos         (16)                                              /*!< EMAC_T::CAM10L: MACADDR0 Position      */
#define EMAC_CAM10L_MACADDR0_Msk         (0xfful << EMAC_CAM10L_MACADDR0_Pos)              /*!< EMAC_T::CAM10L: MACADDR0 Mask          */

#define EMAC_CAM10L_MACADDR1_Pos         (24)                                              /*!< EMAC_T::CAM10L: MACADDR1 Position      */
#define EMAC_CAM10L_MACADDR1_Msk         (0xfful << EMAC_CAM10L_MACADDR1_Pos)              /*!< EMAC_T::CAM10L: MACADDR1 Mask          */

#define EMAC_CAM11M_MACADDR2_Pos         (0)                                               /*!< EMAC_T::CAM11M: MACADDR2 Position      */
#define EMAC_CAM11M_MACADDR2_Msk         (0xfful << EMAC_CAM11M_MACADDR2_Pos)              /*!< EMAC_T::CAM11M: MACADDR2 Mask          */

#define EMAC_CAM11M_MACADDR3_Pos         (8)                                               /*!< EMAC_T::CAM11M: MACADDR3 Position      */
#define EMAC_CAM11M_MACADDR3_Msk         (0xfful << EMAC_CAM11M_MACADDR3_Pos)              /*!< EMAC_T::CAM11M: MACADDR3 Mask          */

#define EMAC_CAM11M_MACADDR4_Pos         (16)                                              /*!< EMAC_T::CAM11M: MACADDR4 Position      */
#define EMAC_CAM11M_MACADDR4_Msk         (0xfful << EMAC_CAM11M_MACADDR4_Pos)              /*!< EMAC_T::CAM11M: MACADDR4 Mask          */

#define EMAC_CAM11M_MACADDR5_Pos         (24)                                              /*!< EMAC_T::CAM11M: MACADDR5 Position      */
#define EMAC_CAM11M_MACADDR5_Msk         (0xfful << EMAC_CAM11M_MACADDR5_Pos)              /*!< EMAC_T::CAM11M: MACADDR5 Mask          */

#define EMAC_CAM11L_MACADDR0_Pos         (16)                                              /*!< EMAC_T::CAM11L: MACADDR0 Position      */
#define EMAC_CAM11L_MACADDR0_Msk         (0xfful << EMAC_CAM11L_MACADDR0_Pos)              /*!< EMAC_T::CAM11L: MACADDR0 Mask          */

#define EMAC_CAM11L_MACADDR1_Pos         (24)                                              /*!< EMAC_T::CAM11L: MACADDR1 Position      */
#define EMAC_CAM11L_MACADDR1_Msk         (0xfful << EMAC_CAM11L_MACADDR1_Pos)              /*!< EMAC_T::CAM11L: MACADDR1 Mask          */

#define EMAC_CAM12M_MACADDR2_Pos         (0)                                               /*!< EMAC_T::CAM12M: MACADDR2 Position      */
#define EMAC_CAM12M_MACADDR2_Msk         (0xfful << EMAC_CAM12M_MACADDR2_Pos)              /*!< EMAC_T::CAM12M: MACADDR2 Mask          */

#define EMAC_CAM12M_MACADDR3_Pos         (8)                                               /*!< EMAC_T::CAM12M: MACADDR3 Position      */
#define EMAC_CAM12M_MACADDR3_Msk         (0xfful << EMAC_CAM12M_MACADDR3_Pos)              /*!< EMAC_T::CAM12M: MACADDR3 Mask          */

#define EMAC_CAM12M_MACADDR4_Pos         (16)                                              /*!< EMAC_T::CAM12M: MACADDR4 Position      */
#define EMAC_CAM12M_MACADDR4_Msk         (0xfful << EMAC_CAM12M_MACADDR4_Pos)              /*!< EMAC_T::CAM12M: MACADDR4 Mask          */

#define EMAC_CAM12M_MACADDR5_Pos         (24)                                              /*!< EMAC_T::CAM12M: MACADDR5 Position      */
#define EMAC_CAM12M_MACADDR5_Msk         (0xfful << EMAC_CAM12M_MACADDR5_Pos)              /*!< EMAC_T::CAM12M: MACADDR5 Mask          */

#define EMAC_CAM12L_MACADDR0_Pos         (16)                                              /*!< EMAC_T::CAM12L: MACADDR0 Position      */
#define EMAC_CAM12L_MACADDR0_Msk         (0xfful << EMAC_CAM12L_MACADDR0_Pos)              /*!< EMAC_T::CAM12L: MACADDR0 Mask          */

#define EMAC_CAM12L_MACADDR1_Pos         (24)                                              /*!< EMAC_T::CAM12L: MACADDR1 Position      */
#define EMAC_CAM12L_MACADDR1_Msk         (0xfful << EMAC_CAM12L_MACADDR1_Pos)              /*!< EMAC_T::CAM12L: MACADDR1 Mask          */

#define EMAC_CAM13M_MACADDR2_Pos         (0)                                               /*!< EMAC_T::CAM13M: MACADDR2 Position      */
#define EMAC_CAM13M_MACADDR2_Msk         (0xfful << EMAC_CAM13M_MACADDR2_Pos)              /*!< EMAC_T::CAM13M: MACADDR2 Mask          */

#define EMAC_CAM13M_MACADDR3_Pos         (8)                                               /*!< EMAC_T::CAM13M: MACADDR3 Position      */
#define EMAC_CAM13M_MACADDR3_Msk         (0xfful << EMAC_CAM13M_MACADDR3_Pos)              /*!< EMAC_T::CAM13M: MACADDR3 Mask          */

#define EMAC_CAM13M_MACADDR4_Pos         (16)                                              /*!< EMAC_T::CAM13M: MACADDR4 Position      */
#define EMAC_CAM13M_MACADDR4_Msk         (0xfful << EMAC_CAM13M_MACADDR4_Pos)              /*!< EMAC_T::CAM13M: MACADDR4 Mask          */

#define EMAC_CAM13M_MACADDR5_Pos         (24)                                              /*!< EMAC_T::CAM13M: MACADDR5 Position      */
#define EMAC_CAM13M_MACADDR5_Msk         (0xfful << EMAC_CAM13M_MACADDR5_Pos)              /*!< EMAC_T::CAM13M: MACADDR5 Mask          */

#define EMAC_CAM13L_MACADDR0_Pos         (16)                                              /*!< EMAC_T::CAM13L: MACADDR0 Position      */
#define EMAC_CAM13L_MACADDR0_Msk         (0xfful << EMAC_CAM13L_MACADDR0_Pos)              /*!< EMAC_T::CAM13L: MACADDR0 Mask          */

#define EMAC_CAM13L_MACADDR1_Pos         (24)                                              /*!< EMAC_T::CAM13L: MACADDR1 Position      */
#define EMAC_CAM13L_MACADDR1_Msk         (0xfful << EMAC_CAM13L_MACADDR1_Pos)              /*!< EMAC_T::CAM13L: MACADDR1 Mask          */

#define EMAC_CAM14M_MACADDR2_Pos         (0)                                               /*!< EMAC_T::CAM14M: MACADDR2 Position      */
#define EMAC_CAM14M_MACADDR2_Msk         (0xfful << EMAC_CAM14M_MACADDR2_Pos)              /*!< EMAC_T::CAM14M: MACADDR2 Mask          */

#define EMAC_CAM14M_MACADDR3_Pos         (8)                                               /*!< EMAC_T::CAM14M: MACADDR3 Position      */
#define EMAC_CAM14M_MACADDR3_Msk         (0xfful << EMAC_CAM14M_MACADDR3_Pos)              /*!< EMAC_T::CAM14M: MACADDR3 Mask          */

#define EMAC_CAM14M_MACADDR4_Pos         (16)                                              /*!< EMAC_T::CAM14M: MACADDR4 Position      */
#define EMAC_CAM14M_MACADDR4_Msk         (0xfful << EMAC_CAM14M_MACADDR4_Pos)              /*!< EMAC_T::CAM14M: MACADDR4 Mask          */

#define EMAC_CAM14M_MACADDR5_Pos         (24)                                              /*!< EMAC_T::CAM14M: MACADDR5 Position      */
#define EMAC_CAM14M_MACADDR5_Msk         (0xfful << EMAC_CAM14M_MACADDR5_Pos)              /*!< EMAC_T::CAM14M: MACADDR5 Mask          */

#define EMAC_CAM14L_MACADDR0_Pos         (16)                                              /*!< EMAC_T::CAM14L: MACADDR0 Position      */
#define EMAC_CAM14L_MACADDR0_Msk         (0xfful << EMAC_CAM14L_MACADDR0_Pos)              /*!< EMAC_T::CAM14L: MACADDR0 Mask          */

#define EMAC_CAM14L_MACADDR1_Pos         (24)                                              /*!< EMAC_T::CAM14L: MACADDR1 Position      */
#define EMAC_CAM14L_MACADDR1_Msk         (0xfful << EMAC_CAM14L_MACADDR1_Pos)              /*!< EMAC_T::CAM14L: MACADDR1 Mask          */

#define EMAC_CAM15MSB_OPCODE_Pos         (0)                                               /*!< EMAC_T::CAM15MSB: OPCODE Position      */
#define EMAC_CAM15MSB_OPCODE_Msk         (0xfffful << EMAC_CAM15MSB_OPCODE_Pos)            /*!< EMAC_T::CAM15MSB: OPCODE Mask          */

#define EMAC_CAM15MSB_LENGTH_Pos         (16)                                              /*!< EMAC_T::CAM15MSB: LENGTH Position      */
#define EMAC_CAM15MSB_LENGTH_Msk         (0xfffful << EMAC_CAM15MSB_LENGTH_Pos)            /*!< EMAC_T::CAM15MSB: LENGTH Mask          */

#define EMAC_CAM15LSB_OPERAND_Pos        (24)                                              /*!< EMAC_T::CAM15LSB: OPERAND Position     */
#define EMAC_CAM15LSB_OPERAND_Msk        (0xfful << EMAC_CAM15LSB_OPERAND_Pos)             /*!< EMAC_T::CAM15LSB: OPERAND Mask         */

//#define EMAC_TXDSA_TXDSA_Pos             (0)                                               /*!< EMAC_T::TXDSA: TXDSA Position          */
//#define EMAC_TXDSA_TXDSA_Msk             (0xfffffffful << EMAC_TXDSA_TXDSA_Pos)            /*!< EMAC_T::TXDSA: TXDSA Mask              */

//#define EMAC_RXDSA_RXDSA_Pos             (0)                                               /*!< EMAC_T::RXDSA: RXDSA Position          */
//#define EMAC_RXDSA_RXDSA_Msk             (0xfffffffful << EMAC_RXDSA_RXDSA_Pos)            /*!< EMAC_T::RXDSA: RXDSA Mask              */

#define EMAC_TXDLSA_TXDLSA_Pos             (0)                                               /*!< EMAC_T::TXDSA: TXDSA Position          */
#define EMAC_TXDLSA_TXDLSA_Msk             (0xfffffffful << EMAC_TXDLSA_TXDLSA_Pos)            /*!< EMAC_T::TXDSA: TXDSA Mask              */

#define EMAC_RXDLSA_RXDLSA_Pos             (0)                                               /*!< EMAC_T::RXDSA: RXDSA Position          */
#define EMAC_RXDLSA_RXDLSA_Msk             (0xfffffffful << EMAC_RXDLSA_RXDLSA_Pos)            /*!< EMAC_T::RXDSA: RXDSA Mask              */

//#define EMAC_CTL_RXON_Pos                (0)                                               /*!< EMAC_T::CTL: RXON Position             */
//#define EMAC_CTL_RXON_Msk                (0x1ul << EMAC_CTL_RXON_Pos)                      /*!< EMAC_T::CTL: RXON Mask                 */

//#define EMAC_CTL_ALP_Pos                 (1)                                               /*!< EMAC_T::CTL: ALP Position              */
//#define EMAC_CTL_ALP_Msk                 (0x1ul << EMAC_CTL_ALP_Pos)                       /*!< EMAC_T::CTL: ALP Mask                  */

//#define EMAC_CTL_ARP_Pos                 (2)                                               /*!< EMAC_T::CTL: ARP Position              */
//#define EMAC_CTL_ARP_Msk                 (0x1ul << EMAC_CTL_ARP_Pos)                       /*!< EMAC_T::CTL: ARP Mask                  */

//#define EMAC_CTL_ACP_Pos                 (3)                                               /*!< EMAC_T::CTL: ACP Position              */
//#define EMAC_CTL_ACP_Msk                 (0x1ul << EMAC_CTL_ACP_Pos)                       /*!< EMAC_T::CTL: ACP Mask                  */

//#define EMAC_CTL_AEP_Pos                 (4)                                               /*!< EMAC_T::CTL: AEP Position              */
//#define EMAC_CTL_AEP_Msk                 (0x1ul << EMAC_CTL_AEP_Pos)                       /*!< EMAC_T::CTL: AEP Mask                  */

//#define EMAC_CTL_STRIPCRC_Pos            (5)                                               /*!< EMAC_T::CTL: STRIPCRC Position         */
//#define EMAC_CTL_STRIPCRC_Msk            (0x1ul << EMAC_CTL_STRIPCRC_Pos)                  /*!< EMAC_T::CTL: STRIPCRC Mask             */

//#define EMAC_CTL_WOLEN_Pos               (6)                                               /*!< EMAC_T::CTL: WOLEN Position            */
//#define EMAC_CTL_WOLEN_Msk               (0x1ul << EMAC_CTL_WOLEN_Pos)                     /*!< EMAC_T::CTL: WOLEN Mask                */

//#define EMAC_CTL_TXON_Pos                (8)                                               /*!< EMAC_T::CTL: TXON Position             */
//#define EMAC_CTL_TXON_Msk                (0x1ul << EMAC_CTL_TXON_Pos)                      /*!< EMAC_T::CTL: TXON Mask                 */

//#define EMAC_CTL_NODEF_Pos               (9)                                               /*!< EMAC_T::CTL: NODEF Position            */
//#define EMAC_CTL_NODEF_Msk               (0x1ul << EMAC_CTL_NODEF_Pos)                     /*!< EMAC_T::CTL: NODEF Mask                */

//#define EMAC_CTL_SDPZ_Pos                (16)                                              /*!< EMAC_T::CTL: SDPZ Position             */
//#define EMAC_CTL_SDPZ_Msk                (0x1ul << EMAC_CTL_SDPZ_Pos)                      /*!< EMAC_T::CTL: SDPZ Mask                 */

//#define EMAC_CTL_SQECHKEN_Pos            (17)                                              /*!< EMAC_T::CTL: SQECHKEN Position         */
//#define EMAC_CTL_SQECHKEN_Msk            (0x1ul << EMAC_CTL_SQECHKEN_Pos)                  /*!< EMAC_T::CTL: SQECHKEN Mask             */

//#define EMAC_CTL_FUDUP_Pos               (18)                                              /*!< EMAC_T::CTL: FUDUP Position            */
//#define EMAC_CTL_FUDUP_Msk               (0x1ul << EMAC_CTL_FUDUP_Pos)                     /*!< EMAC_T::CTL: FUDUP Mask                */

//#define EMAC_CTL_RMIIRXCTL_Pos           (19)                                              /*!< EMAC_T::CTL: RMIIRXCTL Position        */
//#define EMAC_CTL_RMIIRXCTL_Msk           (0x1ul << EMAC_CTL_RMIIRXCTL_Pos)                 /*!< EMAC_T::CTL: RMIIRXCTL Mask            */

//#define EMAC_CTL_OPMODE_Pos              (20)                                              /*!< EMAC_T::CTL: OPMODE Position           */
//#define EMAC_CTL_OPMODE_Msk              (0x1ul << EMAC_CTL_OPMODE_Pos)                    /*!< EMAC_T::CTL: OPMODE Mask               */

//#define EMAC_CTL_RMIIEN_Pos              (22)                                              /*!< EMAC_T::CTL: RMIIEN Position           */
//#define EMAC_CTL_RMIIEN_Msk              (0x1ul << EMAC_CTL_RMIIEN_Pos)                    /*!< EMAC_T::CTL: RMIIEN Mask               */

//#define EMAC_CTL_RST_Pos                 (24)                                              /*!< EMAC_T::CTL: RST Position              */
//#define EMAC_CTL_RST_Msk                 (0x1ul << EMAC_CTL_RST_Pos)                       /*!< EMAC_T::CTL: RST Mask                  */

#define EMAC_MCMDR_RXON_Pos                (0)                                               /*!< EMAC_T::CTL: RXON Position             */
#define EMAC_MCMDR_RXON_Msk                (0x1ul << EMAC_MCMDR_RXON_Pos)                      /*!< EMAC_T::CTL: RXON Mask                 */

#define EMAC_MCMDR_ALP_Pos                 (1)                                               /*!< EMAC_T::CTL: ALP Position              */
#define EMAC_MCMDR_ALP_Msk                 (0x1ul << EMAC_MCMDR_ALP_Pos)                       /*!< EMAC_T::CTL: ALP Mask                  */

#define EMAC_MCMDR_ARP_Pos                 (2)                                               /*!< EMAC_T::CTL: ARP Position              */
#define EMAC_MCMDR_ARP_Msk                 (0x1ul << EMAC_MCMDR_ARP_Pos)                       /*!< EMAC_T::CTL: ARP Mask                  */

#define EMAC_MCMDR_ACP_Pos                 (3)                                               /*!< EMAC_T::CTL: ACP Position              */
#define EMAC_MCMDR_ACP_Msk                 (0x1ul << EMAC_MCMDR_ACP_Pos)                       /*!< EMAC_T::CTL: ACP Mask                  */

#define EMAC_MCMDR_AEP_Pos                 (4)                                               /*!< EMAC_T::CTL: AEP Position              */
#define EMAC_MCMDR_AEP_Msk                 (0x1ul << EMAC_MCMDR_AEP_Pos)                       /*!< EMAC_T::CTL: AEP Mask                  */

#define EMAC_MCMDR_SPCRC_Pos               (5)                                               /*!< EMAC_T::CTL: STRIPCRC Position         */
#define EMAC_MCMDR_SPCRC_Msk               (0x1ul << EMAC_MCMDR_SPCRC_Pos)                  /*!< EMAC_T::CTL: STRIPCRC Mask             */

#define EMAC_MCMDR_AMGP_Pos                (6)                                               /*!< EMAC_T::CTL: AMGP Position            */
#define EMAC_MCMDR_AMGP_Msk                (0x1ul << EMAC_MCMDR_AMGP_Pos)                     /*!< EMAC_T::CTL: AMGP Mask                */

#define EMAC_MCMDR_TXON_Pos                (8)                                               /*!< EMAC_T::CTL: TXON Position             */
#define EMAC_MCMDR_TXON_Msk                (0x1ul << EMAC_MCMDR_TXON_Pos)                      /*!< EMAC_T::CTL: TXON Mask                 */

#define EMAC_MCMDR_NODEF_Pos               (9)                                               /*!< EMAC_T::CTL: NODEF Position            */
#define EMAC_MCMDR_NODEF_Msk               (0x1ul << EMAC_MCMDR_NODEF_Pos)                     /*!< EMAC_T::CTL: NODEF Mask                */

#define EMAC_MCMDR_SDPZ_Pos                (16)                                              /*!< EMAC_T::CTL: SDPZ Position             */
#define EMAC_MCMDR_SDPZ_Msk                (0x1ul << EMAC_MCMDR_SDPZ_Pos)                      /*!< EMAC_T::CTL: SDPZ Mask                 */

#define EMAC_MCMDR_ENSQE_Pos               (17)                                              /*!< EMAC_T::CTL: SQECHKEN Position         */
#define EMAC_MCMDR_EMSQE_Msk               (0x1ul << EMAC_MCMDR_ENSQE_Pos)                  /*!< EMAC_T::CTL: SQECHKEN Mask             */

#define EMAC_MCMDR_FDUP_Pos                (18)                                              /*!< EMAC_T::CTL: FUDUP Position            */
#define EMAC_MCMDR_FDUP_Msk                (0x1ul << EMAC_MCMDR_FDUP_Pos)                     /*!< EMAC_T::CTL: FUDUP Mask                */

#define EMAC_MCMDR_ENMDC_Pos               (19)                                              /*!< EMAC_T::CTL: RMIIRXCTL Position        */
#define EMAC_MCMDR_ENMDC_Msk               (0x1ul << EMAC_MCMDR_ENMDC_Pos)                 /*!< EMAC_T::CTL: RMIIRXCTL Mask            */

#define EMAC_MCMDR_OPMOD_Pos               (20)                                              /*!< EMAC_T::CTL: OPMODE Position           */
#define EMAC_MCMDR_OPMOD_Msk               (0x1ul << EMAC_MCMDR_OPMOD_Pos)                    /*!< EMAC_T::CTL: OPMODE Mask               */

#define EMAC_MCMDR_LBK_Pos                 (21)                                              /*!< EMAC_T::CTL: LBK Position           */
#define EMAC_MCMDR_LBK_Msk                 (0x1ul << EMAC_MCMDR_LBK_Pos)                    /*!< EMAC_T::CTL: LBK Mask               */

#define EMAC_MCMDR_ENRMII_Pos              (22)                                              /*!< EMAC_T::CTL: RMIIEN Position           */
#define EMAC_MCMDR_ENRMII_Msk              (0x1ul << EMAC_MCMDR_ENRMII_Pos)                    /*!< EMAC_T::CTL: RMIIEN Mask               */

#define EMAC_MCMDR_SWR_Pos                 (24)                                              /*!< EMAC_T::CTL: RST Position              */
#define EMAC_MCMDR_SWR_Msk                 (0x1ul << EMAC_MCMDR_SWR_Pos)                       /*!< EMAC_T::CTL: RST Mask                  */

//#define EMAC_MIIMDAT_DATA_Pos            (0)                                               /*!< EMAC_T::MIIMDAT: DATA Position         */
//#define EMAC_MIIMDAT_DATA_Msk            (0xfffful << EMAC_MIIMDAT_DATA_Pos)               /*!< EMAC_T::MIIMDAT: DATA Mask             */

#define EMAC_MIID_MIIDATA_Pos              (0)                                               /*!< EMAC_T::MIIMDAT: DATA Position         */
#define EMAC_MIID_MIIDATA_Msk              (0xfffful << EMAC_MIID_MIIDATA_Pos)               /*!< EMAC_T::MIIMDAT: DATA Mask             */

//#define EMAC_MIIMCTL_PHYREG_Pos          (0)                                               /*!< EMAC_T::MIIMCTL: PHYREG Position       */
//#define EMAC_MIIMCTL_PHYREG_Msk          (0x1ful << EMAC_MIIMCTL_PHYREG_Pos)               /*!< EMAC_T::MIIMCTL: PHYREG Mask           */

//#define EMAC_MIIMCTL_PHYADDR_Pos         (8)                                               /*!< EMAC_T::MIIMCTL: PHYADDR Position      */
//#define EMAC_MIIMCTL_PHYADDR_Msk         (0x1ful << EMAC_MIIMCTL_PHYADDR_Pos)              /*!< EMAC_T::MIIMCTL: PHYADDR Mask          */

//#define EMAC_MIIMCTL_WRITE_Pos           (16)                                              /*!< EMAC_T::MIIMCTL: WRITE Position        */
//#define EMAC_MIIMCTL_WRITE_Msk           (0x1ul << EMAC_MIIMCTL_WRITE_Pos)                 /*!< EMAC_T::MIIMCTL: WRITE Mask            */

//#define EMAC_MIIMCTL_BUSY_Pos            (17)                                              /*!< EMAC_T::MIIMCTL: BUSY Position         */
//#define EMAC_MIIMCTL_BUSY_Msk            (0x1ul << EMAC_MIIMCTL_BUSY_Pos)                  /*!< EMAC_T::MIIMCTL: BUSY Mask             */

//#define EMAC_MIIMCTL_PREAMSP_Pos         (18)                                              /*!< EMAC_T::MIIMCTL: PREAMSP Position      */
//#define EMAC_MIIMCTL_PREAMSP_Msk         (0x1ul << EMAC_MIIMCTL_PREAMSP_Pos)               /*!< EMAC_T::MIIMCTL: PREAMSP Mask          */

//#define EMAC_MIIMCTL_MDCON_Pos           (19)                                              /*!< EMAC_T::MIIMCTL: MDCON Position        */
//#define EMAC_MIIMCTL_MDCON_Msk           (0x1ul << EMAC_MIIMCTL_MDCON_Pos)                 /*!< EMAC_T::MIIMCTL: MDCON Mask            */

#define EMAC_MIIDA_PHYRAD_Pos              (0)                                               /*!< EMAC_T::MIIMCTL: PHYREG Position       */
#define EMAC_MIIDA_PHYRAD_Msk              (0x1ful << EMAC_MIIDA_PHYRAD_Pos)               /*!< EMAC_T::MIIMCTL: PHYREG Mask           */

#define EMAC_MIIDA_PHYAD_Pos               (8)                                               /*!< EMAC_T::MIIMCTL: PHYADDR Position      */
#define EMAC_MIIDA_PHYAD_Msk               (0x1ful << EMAC_MIIDA_PHYAD_Pos)              /*!< EMAC_T::MIIMCTL: PHYADDR Mask          */

#define EMAC_MIIDA_WRITE_Pos               (16)                                              /*!< EMAC_T::MIIMCTL: WRITE Position        */
#define EMAC_MIIDA_WRITE_Msk               (0x1ul << EMAC_MIIDA_WRITE_Pos)                 /*!< EMAC_T::MIIMCTL: WRITE Mask            */

#define EMAC_MIIDA_BUSY_Pos                (17)                                              /*!< EMAC_T::MIIMCTL: BUSY Position         */
#define EMAC_MIIDA_BUSY_Msk                (0x1ul << EMAC_MIIDA_BUSY_Pos)                  /*!< EMAC_T::MIIMCTL: BUSY Mask             */

#define EMAC_MIIDA_PRESP_Pos               (18)                                              /*!< EMAC_T::MIIMCTL: PREAMSP Position      */
#define EMAC_MIIDA_PRESP_Msk               (0x1ul << EMAC_MIIDA_PRESP_Pos)               /*!< EMAC_T::MIIMCTL: PREAMSP Mask          */

#define EMAC_MIIDA_MDCON_Pos               (19)                                              /*!< EMAC_T::MIIMCTL: MDCON Position        */
#define EMAC_MIIDA_MDCON_Msk               (0x1ul << EMAC_MIIDA_MDCON_Pos)                 /*!< EMAC_T::MIIMCTL: MDCON Mask            */

#define EMAC_MIIDA_MDCCR_Pos               (20)                                               /*!< EMAC_T::MIIMCTL: PHYREG Position       */
#define EMAC_MIIDA_MDCCR_Msk               (0xful << EMAC_MIIDA_MDCCR_Pos)               /*!< EMAC_T::MIIMCTL: PHYREG Mask           */

//#define EMAC_FIFOCTL_RXFIFOTH_Pos        (0)                                               /*!< EMAC_T::FIFOCTL: RXFIFOTH Position     */
//#define EMAC_FIFOCTL_RXFIFOTH_Msk        (0x3ul << EMAC_FIFOCTL_RXFIFOTH_Pos)              /*!< EMAC_T::FIFOCTL: RXFIFOTH Mask         */

//#define EMAC_FIFOCTL_TXFIFOTH_Pos        (8)                                               /*!< EMAC_T::FIFOCTL: TXFIFOTH Position     */
//#define EMAC_FIFOCTL_TXFIFOTH_Msk        (0x3ul << EMAC_FIFOCTL_TXFIFOTH_Pos)              /*!< EMAC_T::FIFOCTL: TXFIFOTH Mask         */

//#define EMAC_FIFOCTL_BURSTLEN_Pos        (20)                                              /*!< EMAC_T::FIFOCTL: BURSTLEN Position     */
//#define EMAC_FIFOCTL_BURSTLEN_Msk        (0x3ul << EMAC_FIFOCTL_BURSTLEN_Pos)              /*!< EMAC_T::FIFOCTL: BURSTLEN Mask         */

#define EMAC_FFTCR_RXTHD_Pos               (0)                                               /*!< EMAC_T::FIFOCTL: RXFIFOTH Position     */
#define EMAC_FFTCR_RXTHD_Msk               (0x3ul << EMAC_FFTCR_RXTHD_Pos)              /*!< EMAC_T::FIFOCTL: RXFIFOTH Mask         */

#define EMAC_FFTCR_TXTHD_Pos               (8)                                               /*!< EMAC_T::FIFOCTL: TXFIFOTH Position     */
#define EMAC_FFTCR_TXTHD_Msk               (0x3ul << EMAC_FFTCR_TXTHD_Pos)              /*!< EMAC_T::FIFOCTL: TXFIFOTH Mask         */

#define EMAC_FFTCR_BLENGTH_Pos             (20)                                              /*!< EMAC_T::FIFOCTL: BURSTLEN Position     */
#define EMAC_FFTCR_BLENGTH_Msk             (0x3ul << EMAC_FFTCR_BLENGTH_Pos)              /*!< EMAC_T::FIFOCTL: BURSTLEN Mask         */

//#define EMAC_TXST_TXST_Pos               (0)                                               /*!< EMAC_T::TXST: TXST Position            */
//#define EMAC_TXST_TXST_Msk               (0xfffffffful << EMAC_TXST_TXST_Pos)              /*!< EMAC_T::TXST: TXST Mask                */

#define EMAC_TSDR_TSDR_Pos                 (0)                                               /*!< EMAC_T::TXST: TXST Position            */
#define EMAC_TSDR_TSDR_Msk                 (0xfffffffful << EMAC_TSDR_TSDR_Pos)              /*!< EMAC_T::TXST: TXST Mask                */

//#define EMAC_RXST_RXST_Pos               (0)                                               /*!< EMAC_T::RXST: RXST Position            */
//#define EMAC_RXST_RXST_Msk               (0xfffffffful << EMAC_RXST_RXST_Pos)              /*!< EMAC_T::RXST: RXST Mask                */

#define EMAC_RSDR_RXDR_Pos                 (0)                                               /*!< EMAC_T::RXST: RXST Position            */
#define EMAC_RSDR_RXDR_Msk                 (0xfffffffful << EMAC_RSDR_RXDR_Pos)              /*!< EMAC_T::RXST: RXST Mask                */

//#define EMAC_MRFL_MRFL_Pos               (0)                                               /*!< EMAC_T::MRFL: MRFL Position            */
//#define EMAC_MRFL_MRFL_Msk               (0xfffful << EMAC_MRFL_MRFL_Pos)                  /*!< EMAC_T::MRFL: MRFL Mask                */

#define EMAC_DMARFC_RXMS_Pos               (0)                                               /*!< EMAC_T::MRFL: MRFL Position            */
#define EMAC_DMARFC_RXMS_Msk               (0xfffful << EMAC_DMARFC_RXMS_Pos)                  /*!< EMAC_T::MRFL: MRFL Mask                */

#define EMAC_MIEN_ENRXINTR_Pos             (0)                                               /*!< EMAC_T::MIEN: RXIEN Position          */
#define EMAC_MIEN_ENRXINTR_Msk             (0x1ul << EMAC_MIEN_ENRXINTR_Pos)                   /*!< EMAC_T::MIEN: RXIEN Mask              */

#define EMAC_MIEN_ENCRCE_Pos               (1)                                               /*!< EMAC_T::MIEN: CRCEIEN Position        */
#define EMAC_MIEN_ENCRCE_Msk               (0x1ul << EMAC_MIEN_ENCRCE_Pos)                 /*!< EMAC_T::MIEN: CRCEIEN Mask            */

#define EMAC_MIEN_ENERXOV_Pos              (2)                                               /*!< EMAC_T::MIEN: RXOVIEN Position        */
#define EMAC_MIEN_ENERXOV_Msk              (0x1ul << EMAC_MIEN_ENERXOV_Pos)                 /*!< EMAC_T::MIEN: RXOVIEN Mask            */

#define EMAC_MIEN_ENPTLE_Pos               (3)                                               /*!< EMAC_T::MIEN: LPIEN Position          */
#define EMAC_MIEN_ENPTLE_Msk               (0x1ul << EMAC_MIEN_ENPTLE_Pos)                   /*!< EMAC_T::MIEN: LPIEN Mask              */

#define EMAC_MIEN_ENRXGD_Pos               (4)                                               /*!< EMAC_T::MIEN: RXGDIEN Position        */
#define EMAC_MIEN_ENRXGD_Msk               (0x1ul << EMAC_MIEN_ENRXGD_Pos)                 /*!< EMAC_T::MIEN: RXGDIEN Mask            */

#define EMAC_MIEN_ENALIE_Pos               (5)                                               /*!< EMAC_T::MIEN: ALIEIEN Position        */
#define EMAC_MIEN_ENALIE_Msk               (0x1ul << EMAC_MIEN_ENALIE_Pos)                 /*!< EMAC_T::MIEN: ALIEIEN Mask            */

#define EMAC_MIEN_ENRP_Pos                 (6)                                               /*!< EMAC_T::MIEN: RPIEN Position          */
#define EMAC_MIEN_ENRP_Msk                 (0x1ul << EMAC_MIEN_ENRP_Pos)                   /*!< EMAC_T::MIEN: RPIEN Mask              */

#define EMAC_MIEN_ENMMP_Pos                (7)                                               /*!< EMAC_T::MIEN: MPCOVIEN Position       */
#define EMAC_MIEN_ENMMP_Msk                (0x1ul << EMAC_MIEN_ENMMP_Pos)                /*!< EMAC_T::MIEN: MPCOVIEN Mask           */

#define EMAC_MIEN_ENDFO_Pos                (8)                                               /*!< EMAC_T::MIEN: MFLEIEN Position        */
#define EMAC_MIEN_ENDFO_Msk                (0x1ul << EMAC_MIEN_ENDFO_Pos)                 /*!< EMAC_T::MIEN: MFLEIEN Mask            */

#define EMAC_MIEN_ENDEN_Pos                (9)                                               /*!< EMAC_T::MIEN: DENIEN Position         */
#define EMAC_MIEN_ENDEN_Msk                (0x1ul << EMAC_MIEN_ENDEN_Pos)                  /*!< EMAC_T::MIEN: DENIEN Mask             */

#define EMAC_MIEN_ENRDU_Pos                (10)                                              /*!< EMAC_T::MIEN: RDUIEN Position         */
#define EMAC_MIEN_ENRDU_Msk                (0x1ul << EMAC_MIEN_ENRDU_Pos)                  /*!< EMAC_T::MIEN: RDUIEN Mask             */

#define EMAC_MIEN_ENRXBERR_Pos             (11)                                              /*!< EMAC_T::MIEN: RXBEIEN Position        */
#define EMAC_MIEN_ENRXBERR_Msk             (0x1ul << EMAC_MIEN_ENRXBERR_Pos)                 /*!< EMAC_T::MIEN: RXBEIEN Mask            */

#define EMAC_MIEN_ENCFR_Pos                (14)                                              /*!< EMAC_T::MIEN: CFRIEN Position         */
#define EMAC_MIEN_ENCFR_Msk                (0x1ul << EMAC_MIEN_ENCFR_Pos)                  /*!< EMAC_T::MIEN: CFRIEN Mask             */

//#define EMAC_MIEN_WOLIEN_Pos             (15)                                              /*!< EMAC_T::MIEN: WOLIEN Position         */
//#define EMAC_MIEN_WOLIEN_Msk             (0x1ul << EMAC_MIEN_WOLIEN_Pos)                  /*!< EMAC_T::MIEN: WOLIEN Mask             */

#define EMAC_MIEN_ENTXINTR_Pos             (16)                                              /*!< EMAC_T::MIEN: TXIEN Position          */
#define EMAC_MIEN_ENTXINTR_Msk             (0x1ul << EMAC_MIEN_ENTXINTR_Pos)                   /*!< EMAC_T::MIEN: TXIEN Mask              */

#define EMAC_MIEN_ENTXEMP_Pos              (17)                                              /*!< EMAC_T::MIEN: TXUDIEN Position        */
#define EMAC_MIEN_ENTXEMP_Msk              (0x1ul << EMAC_MIEN_ENTXEMP_Pos)                 /*!< EMAC_T::MIEN: TXUDIEN Mask            */

#define EMAC_MIEN_ENTXCP_Pos               (18)                                              /*!< EMAC_T::MIEN: TXCPIEN Position        */
#define EMAC_MIEN_ENTXCP_Msk               (0x1ul << EMAC_MIEN_ENTXCP_Pos)                 /*!< EMAC_T::MIEN: TXCPIEN Mask            */

#define EMAC_MIEN_ENEXDEF_Pos              (19)                                              /*!< EMAC_T::MIEN: EXDEFIEN Position       */
#define EMAC_MIEN_ENEXDEF_Msk              (0x1ul << EMAC_MIEN_ENEXDEF_Pos)                /*!< EMAC_T::MIEN: EXDEFIEN Mask           */

#define EMAC_MIEN_ENNCS_Pos                (20)                                              /*!< EMAC_T::MIEN: NCSIEN Position         */
#define EMAC_MIEN_ENNCS_Msk                (0x1ul << EMAC_MIEN_ENNCS_Pos)                  /*!< EMAC_T::MIEN: NCSIEN Mask             */

#define EMAC_MIEN_ENTXABT_Pos              (21)                                              /*!< EMAC_T::MIEN: TXABTIEN Position       */
#define EMAC_MIEN_ENTXABT_Msk              (0x1ul << EMAC_MIEN_ENTXABT_Pos)                /*!< EMAC_T::MIEN: TXABTIEN Mask           */

#define EMAC_MIEN_ENLC_Pos                 (22)                                              /*!< EMAC_T::MIEN: LCIEN Position          */
#define EMAC_MIEN_ENLC_Msk                 (0x1ul << EMAC_MIEN_ENLC_Pos)                   /*!< EMAC_T::MIEN: LCIEN Mask              */

#define EMAC_MIEN_ENTDU_Pos                (23)                                              /*!< EMAC_T::MIEN: TDUIEN Position         */
#define EMAC_MIEN_ENTDU_Msk                (0x1ul << EMAC_MIEN_ENTDU_Pos)                  /*!< EMAC_T::MIEN: TDUIEN Mask             */

#define EMAC_MIEN_ENTXBERR_Pos             (24)                                              /*!< EMAC_T::MIEN: TXBEIEN Position        */
#define EMAC_MIEN_ENTXBERR_Msk             (0x1ul << EMAC_MIEN_ENTXBERR_Pos)                 /*!< EMAC_T::MIEN: TXBEIEN Mask            */

//#define EMAC_MIEN_TSALMIEN_Pos           (28)                                              /*!< EMAC_T::MIEN: TSALMIEN Position       */
//#define EMAC_MIEN_TSALMIEN_Msk           (0x1ul << EMAC_MIEN_TSALMIEN_Pos)                /*!< EMAC_T::MIEN: TSALMIEN Mask           */

#define EMAC_MISTA_RXINTR_Pos             (0)                                               /*!< EMAC_T::MISTA: RXIF Position          */
#define EMAC_MISTA_RXINTR_Msk             (0x1ul << EMAC_MISTA_RXINTR_Pos)                   /*!< EMAC_T::MISTA: RXIF Mask              */

#define EMAC_MISTA_CRCE_Pos               (1)                                               /*!< EMAC_T::MISTA: CRCEIF Position        */
#define EMAC_MISTA_CRCE_Msk               (0x1ul << EMAC_MISTA_CRCE_Pos)                 /*!< EMAC_T::MISTA: CRCEIF Mask            */

#define EMAC_MISTA_RXOV_Pos               (2)                                               /*!< EMAC_T::MISTA: RXOVIF Position        */
#define EMAC_MISTA_RXOV_Msk               (0x1ul << EMAC_MISTA_RXOV_Pos)                 /*!< EMAC_T::MISTA: RXOVIF Mask            */

#define EMAC_MISTA_PTLE_Pos               (3)                                               /*!< EMAC_T::MISTA: LPIF Position          */
#define EMAC_MISTA_PTLE_Msk               (0x1ul << EMAC_MISTA_PTLE_Pos)                   /*!< EMAC_T::MISTA: LPIF Mask              */

#define EMAC_MISTA_RXGD_Pos               (4)                                               /*!< EMAC_T::MISTA: RXGDIF Position        */
#define EMAC_MISTA_RXGD_Msk               (0x1ul << EMAC_MISTA_RXGD_Pos)                 /*!< EMAC_T::MISTA: RXGDIF Mask            */

#define EMAC_MISTA_ALIE_Pos               (5)                                               /*!< EMAC_T::MISTA: ALIEIF Position        */
#define EMAC_MISTA_ALIE_Msk               (0x1ul << EMAC_MISTA_ALIE_Pos)                 /*!< EMAC_T::MISTA: ALIEIF Mask            */

#define EMAC_MISTA_RP_Pos                 (6)                                               /*!< EMAC_T::MISTA: RPIF Position          */
#define EMAC_MISTA_RP_Msk                 (0x1ul << EMAC_MISTA_RP_Pos)                   /*!< EMAC_T::MISTA: RPIF Mask              */

#define EMAC_MISTA_MMP_Pos                (7)                                               /*!< EMAC_T::MISTA: MPCOVIF Position       */
#define EMAC_MISTA_MMP_Msk                (0x1ul << EMAC_MISTA_MMP_Pos)                /*!< EMAC_T::MISTA: MPCOVIF Mask           */

#define EMAC_MISTA_DFOI_Pos               (8)                                               /*!< EMAC_T::MISTA: MFLEIF Position        */
#define EMAC_MISTA_DFOI_Msk               (0x1ul << EMAC_MISTA_DFOI_Pos)                 /*!< EMAC_T::MISTA: MFLEIF Mask            */

#define EMAC_MISTA_DEN_Pos                (9)                                               /*!< EMAC_T::MISTA: DENIF Position         */
#define EMAC_MISTA_DEN_Msk                (0x1ul << EMAC_MISTA_DEN_Pos)                  /*!< EMAC_T::MISTA: DENIF Mask             */

#define EMAC_MISTA_RDU_Pos                (10)                                              /*!< EMAC_T::MISTA: RDUIF Position         */
#define EMAC_MISTA_RDU_Msk                (0x1ul << EMAC_MISTA_RDU_Pos)                  /*!< EMAC_T::MISTA: RDUIF Mask             */

#define EMAC_MISTA_RXBERR_Pos             (11)                                              /*!< EMAC_T::MISTA: RXBEIF Position        */
#define EMAC_MISTA_RXBERR_Msk             (0x1ul << EMAC_MISTA_RXBERR_Pos)                 /*!< EMAC_T::MISTA: RXBEIF Mask            */

#define EMAC_MISTA_CFR_Pos                (14)                                              /*!< EMAC_T::MISTA: CFRIF Position         */
#define EMAC_MISTA_CFR_Msk                (0x1ul << EMAC_MISTA_CFR_Pos)                  /*!< EMAC_T::MISTA: CFRIF Mask             */

//#define EMAC_MISTA_WOLIF_Pos            (15)                                              /*!< EMAC_T::MISTA: WOLIF Position         */
//#define EMAC_MISTA_WOLIF_Msk            (0x1ul << EMAC_MISTA_WOLIF_Pos)                  /*!< EMAC_T::MISTA: WOLIF Mask             */

#define EMAC_MISTA_TXINTR_Pos             (16)                                              /*!< EMAC_T::MISTA: TXIF Position          */
#define EMAC_MISTA_TXINTR_Msk             (0x1ul << EMAC_MISTA_TXINTR_Pos)                   /*!< EMAC_T::MISTA: TXIF Mask              */

#define EMAC_MISTA_TXEMP_Pos              (17)                                              /*!< EMAC_T::MISTA: TXUDIF Position        */
#define EMAC_MISTA_TXEMP_Msk              (0x1ul << EMAC_MISTA_TXEMP_Pos)                 /*!< EMAC_T::MISTA: TXUDIF Mask            */

#define EMAC_MISTA_TXCP_Pos               (18)                                              /*!< EMAC_T::MISTA: TXCPIF Position        */
#define EMAC_MISTA_TXCP_Msk               (0x1ul << EMAC_MISTA_TXCP_Pos)                 /*!< EMAC_T::MISTA: TXCPIF Mask            */

#define EMAC_MISTA_EXDEF_Pos              (19)                                              /*!< EMAC_T::MISTA: EXDEFIF Position       */
#define EMAC_MISTA_EXDEF_Msk              (0x1ul << EMAC_MISTA_EXDEF_Pos)                /*!< EMAC_T::MISTA: EXDEFIF Mask           */

#define EMAC_MISTA_NCS_Pos                (20)                                              /*!< EMAC_T::MISTA: NCSIF Position         */
#define EMAC_MISTA_NCS_Msk                (0x1ul << EMAC_MISTA_NCS_Pos)                  /*!< EMAC_T::MISTA: NCSIF Mask             */

#define EMAC_MISTA_TXABT_Pos              (21)                                              /*!< EMAC_T::MISTA: TXABTIF Position       */
#define EMAC_MISTA_TXABT_Msk              (0x1ul << EMAC_MISTA_TXABT_Pos)                /*!< EMAC_T::MISTA: TXABTIF Mask           */

#define EMAC_MISTA_LC_Pos                 (22)                                              /*!< EMAC_T::MISTA: LCIF Position          */
#define EMAC_MISTA_LC_Msk                 (0x1ul << EMAC_MISTA_LC_Pos)                   /*!< EMAC_T::MISTA: LCIF Mask              */

#define EMAC_MISTA_TDU_Pos                (23)                                              /*!< EMAC_T::MISTA: TDUIF Position         */
#define EMAC_MISTA_TDU_Msk                (0x1ul << EMAC_MISTA_TDU_Pos)                  /*!< EMAC_T::MISTA: TDUIF Mask             */

#define EMAC_MISTA_TXBERR_Pos             (24)                                              /*!< EMAC_T::MISTA: TXBEIF Position        */
#define EMAC_MISTA_TXBERR_Msk             (0x1ul << EMAC_MISTA_TXBERR_Pos)                 /*!< EMAC_T::MISTA: TXBEIF Mask            */

#define EMAC_MGSTA_CFR_Pos                (0)                                               /*!< EMAC_T::MGSTA: CFR Position           */
#define EMAC_MGSTA_CFR_Msk                (0x1ul << EMAC_MGSTA_CFR_Pos)                    /*!< EMAC_T::MGSTA: CFR Mask               */

#define EMAC_MGSTA_RXHA_Pos               (1)                                               /*!< EMAC_T::MGSTA: RXHALT Position        */
#define EMAC_MGSTA_RXHA_Msk               (0x1ul << EMAC_MGSTA_RXHALT_Pos)                 /*!< EMAC_T::MGSTA: RXHALT Mask            */

#define EMAC_MGSTA_RXFFULL_Pos            (2)                                               /*!< EMAC_T::MGSTA: RXFFULL Position       */
#define EMAC_MGSTA_RXFFULL_Msk            (0x1ul << EMAC_MGSTA_RXFFULL_Pos)                /*!< EMAC_T::MGSTA: RXFFULL Mask           */

#define EMAC_MGSTA_CCNT_Pos               (4)                                               /*!< EMAC_T::MGSTA: COLCNT Position        */
#define EMAC_MGSTA_CCNT_Msk               (0xful << EMAC_MGSTA_COLCNT_Pos)                 /*!< EMAC_T::MGSTA: COLCNT Mask            */

#define EMAC_MGSTA_DEF_Pos                (8)                                               /*!< EMAC_T::MGSTA: DEF Position           */
#define EMAC_MGSTA_DEF_Msk                (0x1ul << EMAC_MGSTA_DEF_Pos)                    /*!< EMAC_T::MGSTA: DEF Mask               */

#define EMAC_MGSTA_PAU_Pos                (9)                                               /*!< EMAC_T::MGSTA: TXPAUSED Position      */
#define EMAC_MGSTA_PAU_Msk                (0x1ul << EMAC_MGSTA_TXPAUSED_Pos)               /*!< EMAC_T::MGSTA: TXPAUSED Mask          */

#define EMAC_MGSTA_SQE_Pos                (10)                                              /*!< EMAC_T::MGSTA: SQE Position           */
#define EMAC_MGSTA_SQE_Msk                (0x1ul << EMAC_MGSTA_SQE_Pos)                    /*!< EMAC_T::MGSTA: SQE Mask               */

#define EMAC_MGSTA_TXHA_Pos               (11)                                              /*!< EMAC_T::MGSTA: TXHALT Position        */
#define EMAC_MGSTA_TXHA_Msk               (0x1ul << EMAC_MGSTA_TXHALT_Pos)                 /*!< EMAC_T::MGSTA: TXHALT Mask            */

//#define EMAC_MGSTA_RPSTS_Pos            (12)                                              /*!< EMAC_T::MGSTA: RPSTS Position         */
//#define EMAC_MGSTA_RPSTS_Msk            (0x1ul << EMAC_MGSTA_RPSTS_Pos)                  /*!< EMAC_T::MGSTA: RPSTS Mask             */

#define EMAC_MPCNT_MPC_Pos                (0)                                               /*!< EMAC_T::MPCNT: MPCNT Position          */
#define EMAC_MPCNT_MPC_Msk                (0xfffful << EMAC_MPCNT_MPC_Pos)                /*!< EMAC_T::MPCNT: MPCNT Mask              */

#define EMAC_MRPC_MRPC_Pos                (0)                                               /*!< EMAC_T::RPCNT: RPCNT Position          */
#define EMAC_MPRC_MRPC_Msk                (0xfffful << EMAC_MRPC_MRPC_Pos)                /*!< EMAC_T::RPCNT: RPCNT Mask              */

#define EMAC_MRPCC_MRPCC_Pos             (0)                                               /*!< EMAC_T::MRPCC: MRPCC Position          */
#define EMAC_MRPCC_MRPCC_Msk             (0xfffful << EMAC_MRPCC_MRPCC_Pos)                /*!< EMAC_T::MRPCC: MRPCC Mask              */

#define EMAC_MREPC_MREPC_Pos             (0)                                               /*!< EMAC_T::MRPCC: MRPCC Position          */
#define EMAC_MREPC_MREPC_Msk             (0xfffful << EMAC_MREPC_MREPC_Pos)                /*!< EMAC_T::MRPCC: MRPCC Mask              */

//#define EMAC_FRSTS_RXFLT_Pos            (0)                                               /*!< EMAC_T::FRSTS: RXFLT Position          */
//#define EMAC_FRSTS_RXFLT_Msk            (0xfffful << EMAC_FRSTS_RXFLT_Pos)                /*!< EMAC_T::FRSTS: RXFLT Mask              */

#define EMAC_DMARFS_RXFLT_Pos             (0)                                               /*!< EMAC_T::FRSTS: RXFLT Position          */
#define EMAC_DMARFS_RXFLT_Msk             (0xfffful << EMAC_DMARFS_RXFLT_Pos)                /*!< EMAC_T::FRSTS: RXFLT Mask              */

#define EMAC_CTXDSA_CTXDSA_Pos           (0)                                               /*!< EMAC_T::CTXDSA: CTXDSA Position        */
#define EMAC_CTXDSA_CTXDSA_Msk           (0xfffffffful << EMAC_CTXDSA_CTXDSA_Pos)          /*!< EMAC_T::CTXDSA: CTXDSA Mask            */

#define EMAC_CTXBSA_CTXBSA_Pos           (0)                                               /*!< EMAC_T::CTXBSA: CTXBSA Position        */
#define EMAC_CTXBSA_CTXBSA_Msk           (0xfffffffful << EMAC_CTXBSA_CTXBSA_Pos)          /*!< EMAC_T::CTXBSA: CTXBSA Mask            */

#define EMAC_CRXDSA_CRXDSA_Pos           (0)                                               /*!< EMAC_T::CRXDSA: CRXDSA Position        */
#define EMAC_CRXDSA_CRXDSA_Msk           (0xfffffffful << EMAC_CRXDSA_CRXDSA_Pos)          /*!< EMAC_T::CRXDSA: CRXDSA Mask            */

#define EMAC_CRXBSA_CRXBSA_Pos           (0)                                               /*!< EMAC_T::CRXBSA: CRXBSA Position        */
#define EMAC_CRXBSA_CRXBSA_Msk           (0xfffffffful << EMAC_CRXBSA_CRXBSA_Pos)          /*!< EMAC_T::CRXBSA: CRXBSA Mask            */


/**@}*/ /* EMAC_CONST */
/**@}*/ /* end of EMAC register group */
/**@}*/ /* end of REGISTER group */

#define EMAC                 ((EMAC_T *)  EMAC_BA)

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif /* __EMAC_REG_H__ */
