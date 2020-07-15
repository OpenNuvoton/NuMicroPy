/**************************************************************************//**
 * @file     dma.c
 * @version  V0.10
 * @brief   M480 dma functions
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "string.h"
#include "nu_modutil.h"
#include "nu_bitutil.h"
#include "dma.h"

#define NU_PDMA_CH_MAX      PDMA_CH_MAX     /* Specify maximum channels of PDMA */
#define NU_PDMA_CH_Pos      0               /* Specify first channel number of PDMA */
#define NU_PDMA_CH_Msk      (((1 << NU_PDMA_CH_MAX) - 1) << NU_PDMA_CH_Pos)

struct nu_dma_chn_s {
    void        (*handler)(uint32_t, uint32_t);
    uint32_t    id;
    uint32_t    event;
	uint32_t		timeout_us;    
};

static int dma_inited = 0;
static uint32_t dma_chn_mask = 0;
static struct nu_dma_chn_s dma_chn_arr[NU_PDMA_CH_MAX];

static const struct nu_modinit_s dma_modinit = {(uint32_t)PDMA, PDMA_MODULE, 0, 0, PDMA_RST, PDMA_IRQn, (void *) Handle_PDMA_Irq};


typedef struct {
	uint32_t m_u32Peripheral;
	uint32_t m_u32SrcCtl;
	uint32_t m_u32DstCtl;
} S_PDMA_Peripheral_Ctl;	

const S_PDMA_Peripheral_Ctl sPDMAPeripheralCtl[ ] = {
		{ PDMA_MEM, PDMA_SAR_INC, PDMA_DAR_INC },
		// M2P
		{ PDMA_USB_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_UART0_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_UART1_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_UART2_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_UART3_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_UART4_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_UART5_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_USCI0_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_USCI1_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_QSPI0_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_SPI0_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_SPI1_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_SPI2_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_SPI3_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_I2C0_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_I2C1_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_I2C2_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_I2S0_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_DAC0_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		{ PDMA_DAC1_TX, PDMA_SAR_INC, PDMA_DAR_FIX },
		
		// P2M
		{ PDMA_USB_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_UART0_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_UART1_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_UART2_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_UART3_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_UART4_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_UART5_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_USCI0_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_USCI1_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_QSPI0_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_SPI0_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_SPI1_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_SPI2_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_SPI3_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_EPWM0_P1_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_EPWM0_P2_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_EPWM0_P3_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_EPWM1_P1_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_EPWM1_P2_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_EPWM1_P3_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_I2C0_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_I2C1_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_I2C2_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_I2S0_RX, PDMA_SAR_FIX, PDMA_DAR_INC },
		{ PDMA_ADC_RX, PDMA_SAR_FIX, PDMA_DAR_INC },		
		{ 0 }
};


static void dma_enable_channel (PDMA_T * pdma, int channelid )
{
    pdma->CHCTL |=  (1 << channelid); 
}

static void dma_disable_channel (PDMA_T * pdma, int channelid )
{
    pdma->CHCTL &= ~(1 << channelid); 
}	

static int dma_peripheral_control ( uint32_t u32Peripheral, uint32_t* u32SrcCtl, uint32_t* u32DstCtl  )
{
	int idx=0;

	while ( idx < (sizeof(sPDMAPeripheralCtl) / sizeof(sPDMAPeripheralCtl[0])) )
	{
		if ( sPDMAPeripheralCtl[idx].m_u32Peripheral == u32Peripheral )
		{                                   
			*u32SrcCtl = sPDMAPeripheralCtl[idx].m_u32SrcCtl; 
			*u32DstCtl = sPDMAPeripheralCtl[idx].m_u32DstCtl; 
			return 1;
		}
		idx++;
	}
	return 0;
}

static int fill_pdma_timeout(PDMA_T * pdma, int channelid, int timeout_us )
{
	if ( !(dma_chn_mask & (1 << channelid))  )
			goto exit_hal_fill_pdma_timeout;

	dma_chn_arr[channelid - NU_PDMA_CH_Pos].timeout_us = timeout_us;

	if ( timeout_us && channelid <= 1 ) // M480 limit
	{
		uint32_t u32ToClk_Max = 1000000 / (CLK_GetHCLKFreq() /  (1<<8));
		uint32_t u32Divider 	= ( timeout_us / u32ToClk_Max ) / (1<<16);
		uint32_t u32TOutCnt 	= ( timeout_us / u32ToClk_Max ) % (1<<16);

		PDMA_DisableTimeout ( pdma,	1<<channelid );
		PDMA_EnableInt (  pdma, channelid, PDMA_INT_TIMEOUT); // Interrupt type		

		if ( u32Divider > 7 ) {
        u32Divider = 7;
				u32TOutCnt = (1<<16);
		}


		pdma->TOUTPSC |= ( u32Divider <<(PDMA_TOUTPSC_TOUTPSC1_Pos*channelid) );		
		PDMA_SetTimeOut ( pdma,	channelid, 1, u32TOutCnt );
		
		//printf("CLK_GetHCLKFreq=%dHz, CLK_GetHCLKFreq/2^8=%dhZ\n", CLK_GetHCLKFreq(), CLK_GetHCLKFreq()/(1<<8) );

		//printf("u32ToClk_Max=%dHz, u32Divider=%d , u32TOutCnt=%d\n", u32ToClk_Max, u32Divider, u32TOutCnt );
		//printf("pdma->TOUTPSC=%08x\n", pdma->TOUTPSC );
		return 0;
	}
	else
	{
		PDMA_DisableInt (  pdma, channelid, PDMA_INT_TIMEOUT); // Interrupt type		
		PDMA_DisableTimeout ( pdma,	1<<channelid );
	}
	
exit_hal_fill_pdma_timeout:
	return -1;
}



void dma_init(void)
{
    if (dma_inited) {
        return;
    }

    dma_inited = 1;
    dma_chn_mask = ~NU_PDMA_CH_Msk;
    memset(dma_chn_arr, 0x00, sizeof (dma_chn_arr));

    // Reset this module
    SYS_ResetModule(dma_modinit.rsetidx);

    // Enable IP clock
    CLK_EnableModuleClock(dma_modinit.clkidx);

    PDMA_Open((PDMA_T *)dma_modinit.modname, 0xF);

//    NVIC_SetVector(dma_modinit.irq_n, (uint32_t) dma_modinit.var);	//Remove:Use fixed PDMA vector
    NVIC_EnableIRQ(dma_modinit.irq_n);
}

int dma_channel_allocate(uint32_t capabilities)
{
    if (! dma_inited) {
        dma_init();
    }

    int i = nu_cto(dma_chn_mask);
    if (i != 32) {
        dma_chn_mask |= 1 << i;
        memset(dma_chn_arr + i - NU_PDMA_CH_Pos, 0x00, sizeof (struct nu_dma_chn_s));
        return i;
    }

    // No channel available
    return DMA_ERROR_OUT_OF_CHANNELS;
}

int dma_channel_free(int channelid)
{
    if (channelid != DMA_ERROR_OUT_OF_CHANNELS) {
        dma_chn_mask &= ~(1 << channelid);
    }

    return 0;
}

void dma_set_handler(int channelid, uint32_t handler, uint32_t id, uint32_t event)
{
	if((dma_chn_mask & (1 << channelid)) == 0)
		return;

    dma_chn_arr[channelid - NU_PDMA_CH_Pos].handler = (void (*)(uint32_t, uint32_t)) handler;
    dma_chn_arr[channelid - NU_PDMA_CH_Pos].id = id;
    dma_chn_arr[channelid - NU_PDMA_CH_Pos].event = event;

    // Set interrupt vector if someone has removed it.
//    NVIC_SetVector(dma_modinit.irq_n, (uint32_t) dma_modinit.var); //Remove:Use fixed PDMA vector
//    NVIC_EnableIRQ(dma_modinit.irq_n);
}

void dma_unset_handler(int channelid)
{
	if((dma_chn_mask & (1 << channelid)) == 0)
		return;

    dma_chn_arr[channelid - NU_PDMA_CH_Pos].handler = NULL;
    dma_chn_arr[channelid - NU_PDMA_CH_Pos].id = 0;
    dma_chn_arr[channelid - NU_PDMA_CH_Pos].event = 0;
}


PDMA_T *dma_modbase(void)
{
    return (PDMA_T *) dma_modinit.modname;
}

int dma_untransfer_bytecount(int channelid)
{
		PDMA_T * pdma;
		int i32BitWidth=0;
	
		if ( !(dma_chn_mask & (1 << channelid)) )
			return 0;
		
		pdma = dma_modbase();

		i32BitWidth = pdma->DSCT[channelid].CTL & PDMA_DSCT_CTL_TXWIDTH_Msk;
		i32BitWidth = (i32BitWidth==PDMA_WIDTH_8)?1:(i32BitWidth==PDMA_WIDTH_16)?2:(i32BitWidth==PDMA_WIDTH_32)?4:0;
		
		return i32BitWidth * (((pdma->DSCT[channelid].CTL & PDMA_DSCT_CTL_TXCNT_Msk)>>PDMA_DSCT_CTL_TXCNT_Pos) + 1);
}

void Handle_PDMA_Irq(void)
{
    uint32_t intsts = PDMA_GET_INT_STATUS(PDMA);

    // Abort
    if (intsts & PDMA_INTSTS_ABTIF_Msk) {
        uint32_t abtsts = PDMA_GET_ABORT_STS(PDMA);
        // Clear all Abort flags
        PDMA_CLR_ABORT_FLAG(PDMA, abtsts);

        while (abtsts) {
            int chn_id = nu_ctz(abtsts) - PDMA_ABTSTS_ABTIF0_Pos + NU_PDMA_CH_Pos;
            if (dma_chn_mask & (1 << chn_id)) {
                struct nu_dma_chn_s *dma_chn = dma_chn_arr + chn_id - NU_PDMA_CH_Pos;
                if (dma_chn->handler && (dma_chn->event & DMA_EVENT_ABORT)) {
                    dma_chn->handler(dma_chn->id, DMA_EVENT_ABORT);
                }
            }
            abtsts &= ~(1 << (chn_id - NU_PDMA_CH_Pos + PDMA_ABTSTS_ABTIF0_Pos));
        }
    }

    // Transfer done
    if (intsts & PDMA_INTSTS_TDIF_Msk) {
        uint32_t tdsts = PDMA_GET_TD_STS(PDMA);
        // Clear all transfer done flags
        PDMA_CLR_TD_FLAG(PDMA, tdsts);

        while (tdsts) {
            int chn_id = nu_ctz(tdsts) - PDMA_TDSTS_TDIF0_Pos + NU_PDMA_CH_Pos;
            if (dma_chn_mask & (1 << chn_id)) {
                struct nu_dma_chn_s *dma_chn = dma_chn_arr + chn_id - NU_PDMA_CH_Pos;
                if (dma_chn->handler && (dma_chn->event & DMA_EVENT_TRANSFER_DONE)) {
                    dma_chn->handler(dma_chn->id, DMA_EVENT_TRANSFER_DONE);
                }
            }
            tdsts &= ~(1 << (chn_id - NU_PDMA_CH_Pos + PDMA_TDSTS_TDIF0_Pos));
        }
    }

    // Timeout
    uint32_t reqto = intsts & (PDMA_INTSTS_REQTOF0_Msk | PDMA_INTSTS_REQTOF1_Msk);
    if (reqto) {
        // Clear all Timeout flags
        PDMA->INTSTS = reqto;

        while (reqto) {
            int chn_id = nu_ctz(reqto) - PDMA_INTSTS_REQTOF0_Pos + NU_PDMA_CH_Pos;
            if (dma_chn_mask & (1 << chn_id)) {
PDMA_DisableTimeout ( PDMA,	dma_chn_mask );

                struct nu_dma_chn_s *dma_chn = dma_chn_arr + chn_id - NU_PDMA_CH_Pos;
                if (dma_chn->handler && (dma_chn->event & DMA_EVENT_TIMEOUT)) {
                    dma_chn->handler(dma_chn->id, DMA_EVENT_TIMEOUT);
                }
fill_pdma_timeout(PDMA, chn_id, dma_chn_arr[chn_id - NU_PDMA_CH_Pos].timeout_us );
            }
            reqto &= ~(1 << (chn_id - NU_PDMA_CH_Pos + PDMA_INTSTS_REQTOF0_Pos));
        }
    }
}

int dma_fill_description(
	int channelid, 
	uint32_t u32Peripheral, 
	uint32_t data_width, 
	uint32_t addr_src, 
	uint32_t addr_dst, 
	int32_t length, 
	uint32_t timeout, 
	uint32_t u32ScatterEn 
)
{
		PDMA_T * pdma;
	
		uint32_t u32SrcCtrl, u32DstCtrl;
		//uint32_t u32PdmaIntMask=0;
		if ( !(dma_chn_mask & (1 << channelid))  )
			goto exit_hal_dma_fill_description;

		if ( !dma_peripheral_control ( u32Peripheral, &u32SrcCtrl, &u32DstCtrl ) )
			goto exit_hal_dma_fill_description;

		pdma = dma_modbase();
	
		dma_disable_channel (pdma, channelid);

		//printf("[%s] pdma=%08x, ch=%d, peripheral=%d, data_width=%d, addr_src=%08x, addr_dst=%08x, len=%d, to=%d \r\n", __func__, pdma, channelid, u32Peripheral, data_width, addr_src, addr_dst, length, timeout  );
		PDMA_DisableTimeout ( pdma,	1<<channelid );


		if(u32ScatterEn){
			// void PDMA_SetTransferMode(PDMA_T * pdma,uint32_t u32Ch, uint32_t u32Peripheral, uint32_t u32ScatterEn, uint32_t u32DescAddr)
			printf("dma_fill_description %d, %lx \n", channelid, addr_src);
			
			if(addr_src < pdma->SCATBA){
				printf("ERROR: scatter table address < SCATBA \n");
				goto exit_hal_dma_fill_description;
			}
			
			if((addr_src - pdma->SCATBA) >= 0x10000){
				printf("ERROR: scatter table address offset over 64KB \n");
				goto exit_hal_dma_fill_description;
			}
			
			
			PDMA_SetTransferMode ( 	pdma,	
															channelid,
															u32Peripheral, 
															1, 
															addr_src);
		}
		else{
			
			// void PDMA_SetTransferMode(PDMA_T * pdma,uint32_t u32Ch, uint32_t u32Peripheral, uint32_t u32ScatterEn, uint32_t u32DescAddr)
			PDMA_SetTransferMode ( 	pdma,	
															channelid,
															u32Peripheral, 
															0, 
															0);
			//	void PDMA_SetTransferCnt(PDMA_T * pdma,uint32_t u32Ch, uint32_t u32Width, uint32_t u32TransCount)
			PDMA_SetTransferCnt ( pdma, 
														channelid, 
														(data_width == 8) ? PDMA_WIDTH_8 : (data_width == 16) ? PDMA_WIDTH_16 : PDMA_WIDTH_32,
														length );
					
			// void PDMA_SetTransferAddr(PDMA_T * pdma,uint32_t u32Ch, uint32_t u32SrcAddr, uint32_t u32SrcCtrl, uint32_t u32DstAddr, uint32_t u32DstCtrl)
			PDMA_SetTransferAddr ( pdma,
															 channelid,
								   addr_src,  // NOTE:
								   // NUC472: End of source address
								   // M451: Start of source address
								   // M480: Start of source address
								   u32SrcCtrl,//hal_dma_check_fixed_addr(u32Peripheral)?PDMA_SAR_FIX:PDMA_SAR_INC,   	// check source address is incremental or fixed
								   addr_dst,  			// Destination address
								   u32DstCtrl);//hal_dma_check_fixed_addr(u32Peripheral)?PDMA_DAR_FIX:PDMA_DAR_INC);  	// check destination address is incremental or fixed

				//	void PDMA_SetBurstType(PDMA_T * pdma,uint32_t u32Ch, uint32_t u32BurstType, uint32_t u32BurstSize)
			PDMA_SetBurstType ( pdma,
														channelid,
								PDMA_REQ_SINGLE,    // Single mode
								0); // Burst size

		}
		
		PDMA_EnableInt (  pdma,	channelid, PDMA_INT_TRANS_DONE); // Interrupt type		

		if(!u32ScatterEn){
			fill_pdma_timeout( pdma, channelid, timeout );
		}
			
		dma_enable_channel (pdma, channelid);

		return 0;

exit_hal_dma_fill_description:
	
		return -1;
}

static inline void nu_pdma_channel_reset(int i32ChannID)
{
	PDMA_T * pdma;
	
	pdma = dma_modbase();
    pdma->CHRST = (1 << i32ChannID);
}

void dma_channel_terminate(int i32ChannID)
{
    int i;
    uint32_t u32EnabledChans;
    int ch_mask = 0;
	PDMA_T * pdma;
	
	pdma = dma_modbase();
    
    if (!(dma_chn_mask & (1 << i32ChannID)))
        goto exit_dma_channel_terminate;

   // Suspend all channels.
    u32EnabledChans = dma_chn_mask & NU_PDMA_CH_Msk;
    while ((i = nu_ctz(u32EnabledChans)) != 32)
    {
        ch_mask = (1 << i);
        if (i == i32ChannID)
        {
            u32EnabledChans &= ~ch_mask;
            continue;
        }

        // Pause the channel
        PDMA_PAUSE(pdma, i);

        // Wait for channel to finish current transfer
        while (pdma->TACTSTS & ch_mask) { }

        u32EnabledChans &= ~ch_mask;
    } //while

   // Reset specified channel ID
    nu_pdma_channel_reset(i32ChannID);

    // Clean descriptor table control register.
    pdma->DSCT[i32ChannID].CTL = 0UL;

    // Resume all channels.
    u32EnabledChans = dma_chn_mask & NU_PDMA_CH_Msk;

    while ((i = nu_ctz(u32EnabledChans)) != 32)
    {
        ch_mask = (1 << i);

        pdma->CHCTL |= ch_mask;
        PDMA_Trigger(pdma, i);
        u32EnabledChans &= ~ch_mask;
    }
    
exit_dma_channel_terminate:

    return;
}


