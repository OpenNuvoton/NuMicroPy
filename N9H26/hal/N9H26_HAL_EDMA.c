/**************************************************************************//**
 * @file     N9H26_HAL_EDMA.c
 * @version  V0.01
 * @brief    N9H26 series EDMA HAL source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
 
#include "N9H26_HAL_EDMA.h"

#if MICROPY_PY_THREAD
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif

/* Private define ---------------------------------------------------------------*/
// RT_DEV_NAME_PREFIX pdma
#define NU_EDMA_MEMFUN_ACTOR_MAX (2)

#define NU_EDMA_CH_MAX (6)				/* Specify maximum channels of PDMA */
//#define NU_EDMA_CH_Pos (0)				/* Specify first channel number of PDMA */
//#define NU_EDMA_CH_Msk (((1 << NU_EDMA_CH_MAX) - 1) << NU_EDMA_CH_Pos)
#define NU_EDMA_CH_Msk ((1 << NU_EDMA_CH_MAX) - 1)

/* Private typedef --------------------------------------------------------------*/


struct nu_edma_periph_ctl
{
    E_EDMA_PERIPH_TYPE		m_ePeripheralType;
    nu_edma_memctrl_t		m_eMemCtl;
	E_DRVEDMA_APB_DEVICE	m_eAPBDevice;
	E_DRVEDMA_APB_RW		m_eAPBRW;
};
typedef struct nu_edma_periph_ctl nu_edma_periph_ctl_t;

struct nu_edma_chn
{
    nu_edma_cb_handler_t   m_pfnCBHandler;
    void                  *m_pvUserData;
    uint32_t               m_u32EventFilter;
    uint32_t               m_u32IdleTimeout_us;
    nu_edma_periph_ctl_t   m_spPeripCtl;
};
typedef struct nu_edma_chn nu_edma_chn_t;

struct nu_edma_memfun_actor
{
    int         m_i32ChannID;
    uint32_t    m_u32Result;
    uint32_t    m_u32TrigTransferCnt;
#if MICROPY_PY_THREAD
	SemaphoreHandle_t m_psSemMemFun;
#else
	bool		m_bWaitMemFun;
#endif
} ;
typedef struct nu_edma_memfun_actor *nu_edma_memfun_actor_t;

/* Private functions ------------------------------------------------------------*/

/* Public functions -------------------------------------------------------------*/

/* Private variables ------------------------------------------------------------*/

static nu_edma_chn_t nu_edma_chn_arr[NU_EDMA_CH_MAX];
static volatile int nu_edma_inited = 0;
static volatile uint32_t nu_edma_chn_mask = 0;

static volatile uint32_t nu_edma_memfun_actor_mask = 0;
static volatile uint32_t nu_edma_memfun_actor_maxnum = 0;

#if MICROPY_PY_THREAD
static SemaphoreHandle_t nu_edma_memfun_actor_pool_sem;
static xSemaphoreHandle nu_edma_memfun_actor_pool_lock;

#define EDMA_MEMFUN_MUTEX_LOCK()		xSemaphoreTake(nu_edma_memfun_actor_pool_lock, portMAX_DELAY)
#define EDMA_MEMFUN_MUTEX_UNLOCK()		xSemaphoreGive(nu_edma_memfun_actor_pool_lock)
#else

#define EDMA_MEMFUN_MUTEX_LOCK()
#define EDMA_MEMFUN_MUTEX_UNLOCK()
#endif


#if MICROPY_PY_THREAD
static xSemaphoreHandle s_mutex_res = NULL;

#define EDMA_RES_MUTEX_LOCK()		xSemaphoreTake(s_mutex_res, portMAX_DELAY)
#define EDMA_RES_MUTEX_UNLOCK()		xSemaphoreGive(s_mutex_res)
#else
#define EDMA_RES_MUTEX_LOCK()
#define EDMA_RES_MUTEX_UNLOCK()
#endif

static const nu_edma_periph_ctl_t g_nu_edma_peripheral_ctl_pool[ ] =
{
    // M2M
    { EDMA_MEM, eMemCtl_SrcInc_DstInc, 0, 0 },

    // M2P
    { EDMA_SPIMS0_TX, eMemCtl_SrcInc_DstFix, eDRVEDMA_SPIMS0, eDRVEDMA_WRITE_APB },
    { EDMA_SPIMS1_TX, eMemCtl_SrcInc_DstFix, eDRVEDMA_SPIMS1, eDRVEDMA_WRITE_APB },
    { EDMA_UART0_TX, eMemCtl_SrcInc_DstFix, eDRVEDMA_UART0, eDRVEDMA_WRITE_APB },
    { EDMA_UART1_TX, eMemCtl_SrcInc_DstFix, eDRVEDMA_UART1, eDRVEDMA_WRITE_APB },
    { EDMA_RF_CODEC_TX, eMemCtl_SrcInc_DstFix, eDRVEDMA_RF_CODEC, eDRVEDMA_WRITE_APB },
    { EDMA_RS_CODEC_TX, eMemCtl_SrcInc_DstFix, eDRVEDMA_RS_CODEC, eDRVEDMA_WRITE_APB },

    // P2M
    { EDMA_SPIMS0_RX, eMemCtl_SrcFix_DstInc, eDRVEDMA_SPIMS0, eDRVEDMA_READ_APB },
    { EDMA_SPIMS1_RX, eMemCtl_SrcFix_DstInc, eDRVEDMA_SPIMS1, eDRVEDMA_READ_APB },
    { EDMA_UART0_RX, eMemCtl_SrcFix_DstInc, eDRVEDMA_UART0, eDRVEDMA_READ_APB },
    { EDMA_UART1_RX, eMemCtl_SrcFix_DstInc, eDRVEDMA_UART1, eDRVEDMA_READ_APB },
    { EDMA_ADC_RX, eMemCtl_SrcFix_DstWrap, eDRVEDMA_ADC, eDRVEDMA_READ_APB },
    { EDMA_RF_CODEC_RX, eMemCtl_SrcFix_DstInc, eDRVEDMA_RF_CODEC, eDRVEDMA_READ_APB },
    { EDMA_RS_CODEC_RX, eMemCtl_SrcFix_DstInc, eDRVEDMA_RS_CODEC, eDRVEDMA_READ_APB },
};

#define NU_PERIPHERAL_SIZE ( sizeof(g_nu_edma_peripheral_ctl_pool) / sizeof(g_nu_edma_peripheral_ctl_pool[0]) )

static struct nu_edma_memfun_actor nu_edma_memfun_actor_arr[NU_EDMA_MEMFUN_ACTOR_MAX];


// EDMA ISR
void EDMA_HAL_ISR(void)
{
    UINT32 u32IntStatus;
    UINT32 u32WraparoundStatus;
	INT32 i32ChannID;
    nu_edma_chn_t *psEdmaChann;
	UINT32 u32ChannEvent;
    
    if (inp32(REG_VDMA_ISR) & INTR)
    {
    	if (inp32(REG_VDMA_ISR) & INTR0)
    	{
			i32ChannID = 0;
			u32ChannEvent = 0;
			psEdmaChann = &nu_edma_chn_arr[i32ChannID];

	    	u32IntStatus = inp32(REG_VDMA_ISR) & inp32(REG_VDMA_IER);
    		if (u32IntStatus & EDMATABORT_IF)
    		{
    			outp32(REG_VDMA_ISR,EDMATABORT_IF);
				u32ChannEvent |= NU_PDMA_EVENT_ABORT;
 			}    			
    		else 
    		{
    			if (u32IntStatus & EDMABLKD_IF)
    			{
    				outp32(REG_VDMA_ISR,EDMABLKD_IF);
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
				}
				else if (u32IntStatus & EDMASG_IF)
				{
					outp32(REG_VDMA_ISR,EDMASG_IF);	
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
				}					    			
    		}
    		
    		if(psEdmaChann->m_pfnCBHandler)
    		{
				if(psEdmaChann->m_u32EventFilter &  u32ChannEvent)
				{
					psEdmaChann->m_pfnCBHandler(psEdmaChann->m_pvUserData, u32ChannEvent);
				}
			}
    	}

    	if (inp32(REG_VDMA_ISR) & INTR1)
    	{
			i32ChannID = 1;
			u32ChannEvent = 0;
			psEdmaChann = &nu_edma_chn_arr[i32ChannID];


    		if (inp32(REG_PDMA_IER1) & WAR_IE)
	    		u32IntStatus = inp32(REG_PDMA_ISR1) & (inp32(REG_PDMA_IER1) | 0x0F00);
	    	else
	    		u32IntStatus = inp32(REG_PDMA_ISR1) & inp32(REG_PDMA_IER1);	    	
	    	
    		if (u32IntStatus & EDMATABORT_IF)
    		{
    			outp32(REG_PDMA_ISR1,EDMATABORT_IF);
				u32ChannEvent |= NU_PDMA_EVENT_ABORT;
			}    			
    		else
    		{
    			if (u32IntStatus & EDMABLKD_IF)
    			{
    				outp32(REG_PDMA_ISR1,EDMABLKD_IF);
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
				}	   
	    		else if (u32IntStatus & EDMASG_IF)
	    		{
	    			outp32(REG_PDMA_ISR1,EDMASG_IF);
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
	    		}				 			
    			else
    			{
	    			u32WraparoundStatus = inp32(REG_PDMA_ISR1) & 0x0F00;
	    			if (u32WraparoundStatus)
	    			{
		    			if (u32WraparoundStatus & 0x0200)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_THREE_FOURTHS;
		    			else if (u32WraparoundStatus & 0x0400)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_HALF;
		    			else  if (u32WraparoundStatus & 0x0800)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_QUARTER;
		    			else		   
							u32ChannEvent |= NU_PDMA_EVENT_WRA_EMPTY;

						outp32(REG_PDMA_ISR1,u32WraparoundStatus);				
	    			}
				}	    			
    		}

    		if(psEdmaChann->m_pfnCBHandler)
    		{
				if(psEdmaChann->m_u32EventFilter &  u32ChannEvent)
				{
					psEdmaChann->m_pfnCBHandler(psEdmaChann->m_pvUserData, u32ChannEvent);
				}
			}
    	}

    	if (inp32(REG_VDMA_ISR) & INTR2)
    	{
			i32ChannID = 2;
			u32ChannEvent = 0;
			psEdmaChann = &nu_edma_chn_arr[i32ChannID];

    		if (inp32(REG_PDMA_IER2) & WAR_IE)
	    		u32IntStatus = inp32(REG_PDMA_ISR2) & (inp32(REG_PDMA_IER2) | 0x0F00);
	    	else
	    		u32IntStatus = inp32(REG_PDMA_ISR2) & inp32(REG_PDMA_IER2);	    	
	    	
    		if (u32IntStatus & EDMATABORT_IF)
    		{
    			outp32(REG_PDMA_ISR2,EDMATABORT_IF);
  				u32ChannEvent |= NU_PDMA_EVENT_ABORT;
 			}    			
    		else 
    		{
    			if (u32IntStatus & EDMABLKD_IF)
    			{
    				outp32(REG_PDMA_ISR2,EDMABLKD_IF);	
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
				}	
	    		else if (u32IntStatus & EDMASG_IF)
	    		{
	    			outp32(REG_PDMA_ISR2,EDMASG_IF);
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
	    		}					    			
    			else
    			{
	    			u32WraparoundStatus = inp32(REG_PDMA_ISR2) & 0x0F00;
	    			if (u32WraparoundStatus)
	    			{
		    			if (u32WraparoundStatus & 0x0200)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_THREE_FOURTHS;
		    			else if (u32WraparoundStatus & 0x0400)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_HALF;
		    			else  if (u32WraparoundStatus & 0x0800)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_QUARTER;
		    			else		   
							u32ChannEvent |= NU_PDMA_EVENT_WRA_EMPTY;

						outp32(REG_PDMA_ISR2,u32WraparoundStatus);		
	    			}
				}	    			
    		}     	

    		if(psEdmaChann->m_pfnCBHandler)
    		{
				if(psEdmaChann->m_u32EventFilter &  u32ChannEvent)
				{
					psEdmaChann->m_pfnCBHandler(psEdmaChann->m_pvUserData, u32ChannEvent);
				}
			}
    	}

    	if (inp32(REG_VDMA_ISR) & INTR3)
    	{
			i32ChannID = 3;
			u32ChannEvent = 0;
			psEdmaChann = &nu_edma_chn_arr[i32ChannID];

    		if (inp32(REG_PDMA_IER3) & WAR_IE)
	    		u32IntStatus = inp32(REG_PDMA_ISR3) & (inp32(REG_PDMA_IER3) | 0x0F00);
	    	else
	    		u32IntStatus = inp32(REG_PDMA_ISR3) & inp32(REG_PDMA_IER3);	    	
	    	
    		if (u32IntStatus & EDMATABORT_IF)
    		{
    			outp32(REG_PDMA_ISR3,EDMATABORT_IF);
				u32ChannEvent |= NU_PDMA_EVENT_ABORT;
			}    			
    		else 
    		{
    			if (u32IntStatus & EDMABLKD_IF)
    			{
    				outp32(REG_PDMA_ISR3,EDMABLKD_IF);	
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
				}	    
	    		else if (u32IntStatus & EDMASG_IF)
	    		{
	    			outp32(REG_PDMA_ISR3,EDMASG_IF);
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
	    		}						
    			else
    			{
	    			u32WraparoundStatus = inp32(REG_PDMA_ISR3) & 0x0F00;
	    			if (u32WraparoundStatus)
	    			{
		    			if (u32WraparoundStatus & 0x0200)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_THREE_FOURTHS;
		    			else if (u32WraparoundStatus & 0x0400)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_HALF;
		    			else  if (u32WraparoundStatus & 0x0800)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_QUARTER;
		    			else		   
							u32ChannEvent |= NU_PDMA_EVENT_WRA_EMPTY;

						outp32(REG_PDMA_ISR3,u32WraparoundStatus);				
	    			}
				}	    			
    		}     	

    		if(psEdmaChann->m_pfnCBHandler)
    		{
				if(psEdmaChann->m_u32EventFilter &  u32ChannEvent)
				{
					psEdmaChann->m_pfnCBHandler(psEdmaChann->m_pvUserData, u32ChannEvent);
				}
			}
    	}
    	
    	if (inp32(REG_VDMA_ISR) & INTR4)
    	{
			i32ChannID = 4;
			u32ChannEvent = 0;
			psEdmaChann = &nu_edma_chn_arr[i32ChannID];

    		if (inp32(REG_PDMA_IER4) & WAR_IE)
	    		u32IntStatus = inp32(REG_PDMA_ISR4) & (inp32(REG_PDMA_IER4) | 0x0F00);
	    	else
	    		u32IntStatus = inp32(REG_PDMA_ISR4) & inp32(REG_PDMA_IER4);	    	
	    	
    		if (u32IntStatus & EDMATABORT_IF)
    		{
    			outp32(REG_PDMA_ISR4,EDMATABORT_IF);
				u32ChannEvent |= NU_PDMA_EVENT_ABORT;
			}    			
    		else 
    		{
    			if (u32IntStatus & EDMABLKD_IF)
    			{
    				outp32(REG_PDMA_ISR4,EDMABLKD_IF);
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
				}	
	    		else if (u32IntStatus & EDMASG_IF)
	    		{
	    			outp32(REG_PDMA_ISR4,EDMASG_IF);
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
	    		}						
    			else
    			{
	    			u32WraparoundStatus = inp32(REG_PDMA_ISR4) & 0x0F00;
	    			if (u32WraparoundStatus)
	    			{
		    			if (u32WraparoundStatus & 0x0200)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_THREE_FOURTHS;
		    			else if (u32WraparoundStatus & 0x0400)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_HALF;
		    			else  if (u32WraparoundStatus & 0x0800)
							u32ChannEvent |= NU_PDMA_EVENT_WRA_QUARTER;
		    			else		   
							u32ChannEvent |= NU_PDMA_EVENT_WRA_EMPTY;

						outp32(REG_PDMA_ISR4,u32WraparoundStatus);					
	    			}
				}	    			
    		}     	

    		if(psEdmaChann->m_pfnCBHandler)
    		{
				if(psEdmaChann->m_u32EventFilter &  u32ChannEvent)
				{
					psEdmaChann->m_pfnCBHandler(psEdmaChann->m_pvUserData, u32ChannEvent);
				}
			}
    	}

    	if (inp32(REG_VDMA_ISR) & INTR5)
    	{
			i32ChannID = 5;
			u32ChannEvent = 0;
			psEdmaChann = &nu_edma_chn_arr[i32ChannID];

	    	u32IntStatus = inp32(REG_VDMA_ISR5) & inp32(REG_VDMA_IER5);
    		if (u32IntStatus & EDMATABORT_IF)
    		{
    			outp32(REG_VDMA_ISR5,EDMATABORT_IF);
				u32ChannEvent |= NU_PDMA_EVENT_ABORT;
			}    			
    		else 
    		{
    			if (u32IntStatus & EDMABLKD_IF)
    			{
    				outp32(REG_VDMA_ISR5,EDMABLKD_IF);	
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
				}
				else if (u32IntStatus & EDMASG_IF)
				{
					outp32(REG_VDMA_ISR5,EDMASG_IF);
					u32ChannEvent |= NU_PDMA_EVENT_TRANSFER_DONE;
				}
    		}
    		
    		if(psEdmaChann->m_pfnCBHandler)
    		{
				if(psEdmaChann->m_u32EventFilter &  u32ChannEvent)
				{
					psEdmaChann->m_pfnCBHandler(psEdmaChann->m_pvUserData, u32ChannEvent);
				}
			}
		}
	}
}


static int nu_edma_peripheral_set(E_EDMA_PERIPH_TYPE ePeriphType)
{
    int idx = 0;

    while (idx < NU_PERIPHERAL_SIZE)
    {
        if (g_nu_edma_peripheral_ctl_pool[idx].m_ePeripheralType == ePeriphType)
            return idx;
        idx++;
    }

    // Not such peripheral
    return -1;
}

static void nu_edma_periph_ctrl_fill(int i32ChannID, int i32CtlPoolIdx)
{
    nu_edma_chn_t *psEdmaChann = &nu_edma_chn_arr[i32ChannID];
    psEdmaChann->m_spPeripCtl.m_ePeripheralType = g_nu_edma_peripheral_ctl_pool[i32CtlPoolIdx].m_ePeripheralType;
    psEdmaChann->m_spPeripCtl.m_eMemCtl = g_nu_edma_peripheral_ctl_pool[i32CtlPoolIdx].m_eMemCtl;
    psEdmaChann->m_spPeripCtl.m_eAPBDevice = g_nu_edma_peripheral_ctl_pool[i32CtlPoolIdx].m_eAPBDevice;
    psEdmaChann->m_spPeripCtl.m_eAPBRW = g_nu_edma_peripheral_ctl_pool[i32CtlPoolIdx].m_eAPBRW;
}

static void nu_edma_init(void)
{
	int i;

    if (nu_edma_inited)
        return;

#if MICROPY_PY_THREAD
	s_mutex_res = xSemaphoreCreateMutex();
	if(s_mutex_res == NULL)
	{
		printf("Unable create edma resource mutex\n");
		return;
	}
#endif

    nu_edma_chn_mask = ~NU_EDMA_CH_Msk;
    memset(nu_edma_chn_arr, 0x00, sizeof(nu_edma_chn_t));

	// EDMA open
	DrvEDMA_Open();

	for (i = 0; i <= NU_EDMA_CH_MAX; i++) {
		DrvEDMA_ClearCHForAPBDevice((E_DRVEDMA_CHANNEL_INDEX)i);			
	}

	sysInstallISR(IRQ_LEVEL_7, IRQ_EDMA0, (PVOID)EDMA_HAL_ISR);
	sysSetInterruptType(IRQ_EDMA0, HIGH_LEVEL_SENSITIVE);
	sysEnableInterrupt(IRQ_EDMA0);	

	nu_edma_inited = 1;
}

static void nu_edma_channel_enable(int i32ChannID)
{
    DrvEDMA_EnableCH((E_DRVEDMA_CHANNEL_INDEX) i32ChannID, eDRVEDMA_ENABLE);
}

static void nu_edma_channel_disable(int i32ChannID)
{
	DrvEDMA_EnableCH((E_DRVEDMA_CHANNEL_INDEX) i32ChannID, eDRVEDMA_DISABLE);
}

static inline void nu_edma_channel_reset(int i32ChannID)
{
	DrvEDMA_CHSoftwareReset((E_DRVEDMA_CHANNEL_INDEX) i32ChannID);
}

void nu_edma_channel_terminate(int i32ChannID)
{
    if (!(nu_edma_chn_mask & (1 << i32ChannID)))
        goto exit_edma_channel_terminate;

	EDMA_RES_MUTEX_LOCK();
	
	//FixME:Pause all channels?
	
	//Disable edma channel
	nu_edma_channel_disable((E_DRVEDMA_CHANNEL_INDEX) i32ChannID);
	
	//Disable SG
	DrvEDMA_DisableScatterGather((E_DRVEDMA_CHANNEL_INDEX) i32ChannID);

    //Reset specified channel ID
    nu_edma_channel_reset((E_DRVEDMA_CHANNEL_INDEX) i32ChannID);	

	//FixME:Resume all channels

	EDMA_RES_MUTEX_UNLOCK();

exit_edma_channel_terminate:
	return;
}


int nu_edma_channel_allocate(E_EDMA_PERIPH_TYPE ePeripType)
{
    int i, i32PeripCtlIdx;

    nu_edma_init();

	i32PeripCtlIdx = nu_edma_peripheral_set(ePeripType);

	if(i32PeripCtlIdx < 0){
		goto exit_nu_edma_channel_allocate;
	}

	if(ePeripType == EDMA_MEM)
	{
		//Using VDMA channel for memory control
		if((nu_edma_chn_mask & (1 << 0)) == 0)
			i = 0;
		else if	((nu_edma_chn_mask & (1 << 5)) == 0)
			i = 5;
		else 
			goto exit_nu_edma_channel_allocate;
	}
	else {
		for(i = 1; i < 5; i ++){
			if((nu_edma_chn_mask & (1 << i)) == 0)
				break;
		}

		if( i == 5)
			goto exit_nu_edma_channel_allocate;			
	}

	nu_edma_chn_mask |= (1 << i);
	memset(nu_edma_chn_arr + i, 0x00, sizeof(nu_edma_chn_t));

	/* Set idx number of g_nu_pdma_peripheral_ctl_pool */
	nu_edma_periph_ctrl_fill(i, i32PeripCtlIdx);

	/* Reset channel */
	nu_edma_channel_reset(i);

	nu_edma_channel_enable(i);

	return i;

exit_nu_edma_channel_allocate:
	return -1;
}

int nu_edma_channel_free(int i32ChannID)
{
    int ret = -1;

    if (! nu_edma_inited)
        goto exit_nu_edma_channel_free;

    if (i32ChannID < NU_EDMA_CH_MAX)
    {
        nu_edma_chn_mask &= ~(1 << i32ChannID);
        nu_edma_channel_disable(i32ChannID);
        ret =  0;
    }
exit_nu_edma_channel_free:

    return ret;
}

int nu_edma_callback_register(int i32ChannID, nu_edma_cb_handler_t pfnHandler, void *pvUserData, uint32_t u32EventFilter)
{
    int ret = -1;

    if (!(nu_edma_chn_mask & (1 << i32ChannID)))
        goto exit_nu_edma_callback_register;

    nu_edma_chn_arr[i32ChannID].m_pfnCBHandler = pfnHandler;
    nu_edma_chn_arr[i32ChannID].m_pvUserData = pvUserData;
    nu_edma_chn_arr[i32ChannID].m_u32EventFilter = u32EventFilter;

    ret = 0;

exit_nu_edma_callback_register:

    return ret;
}

nu_edma_cb_handler_t nu_edma_callback_hijack(
	int i32ChannID,
	nu_edma_cb_handler_t *ppfnHandler_Hijack,
	void **ppvUserData_Hijack,
	uint32_t *pu32Events_Hijack
)
{
    nu_edma_cb_handler_t pfnHandler_Org = NULL;
    void    *pvUserData_Org;
    uint32_t u32Events_Org;

	if(ppfnHandler_Hijack == NULL)
	{
		printf("ERROR: ppfnHandler_Hijack is NULL \n");
		return NULL;
	}

	if(ppvUserData_Hijack == NULL)
	{
		printf("ERROR: ppvUserData_Hijack is NULL \n");
		return NULL;
	}

    if(pu32Events_Hijack == NULL)
    {
		printf("ERROR: pu32Events_Hijack is NULL \n");
		return NULL;
	}

    if (!(nu_edma_chn_mask & (1 << i32ChannID)))
        goto exit_nu_edma_callback_hijack;

    pfnHandler_Org = nu_edma_chn_arr[i32ChannID].m_pfnCBHandler;
    pvUserData_Org = nu_edma_chn_arr[i32ChannID].m_pvUserData;
    u32Events_Org  = nu_edma_chn_arr[i32ChannID].m_u32EventFilter;

    nu_edma_chn_arr[i32ChannID].m_pfnCBHandler = *ppfnHandler_Hijack;
    nu_edma_chn_arr[i32ChannID].m_pvUserData = *ppvUserData_Hijack;
    nu_edma_chn_arr[i32ChannID].m_u32EventFilter = *pu32Events_Hijack;

    *ppfnHandler_Hijack = pfnHandler_Org;
    *ppvUserData_Hijack = pvUserData_Org;
    *pu32Events_Hijack  = u32Events_Org;

exit_nu_edma_callback_hijack:

    return pfnHandler_Org;
}

static int nu_edma_non_transfer_count_get(int32_t i32ChannID)
{
	nu_edma_periph_ctl_t *psPeriphCtl = NULL;	
	uint32_t u32RemainTransByte;
	E_DRVEDMA_TRANSFER_WIDTH eTransWidth;
	uint32_t u32DataWidth = 8;
	
	u32RemainTransByte = DrvEDMA_GetCurrentTransferCount(i32ChannID);
	psPeriphCtl = &nu_edma_chn_arr[i32ChannID].m_spPeripCtl;                
	
	if(psPeriphCtl->m_ePeripheralType != EDMA_MEM)
	{
		DrvEDMA_GetAPBTransferWidth((E_DRVEDMA_CHANNEL_INDEX)i32ChannID, &eTransWidth);

		if(eTransWidth == eDRVEDMA_WIDTH_16BITS)
			u32DataWidth = 2;
		else if(eTransWidth == eDRVEDMA_WIDTH_32BITS)
			u32DataWidth = 4;
	}

    return u32RemainTransByte / u32DataWidth;
}

int nu_edma_transferred_byte_get(int32_t i32ChannID, int32_t i32TriggerByteLen)
{
    int cur_txcnt = 0;

    if (!(nu_edma_chn_mask & (1 << i32ChannID)))
        goto exit_nu_edma_transferred_byte_get;

    cur_txcnt = DrvEDMA_GetCurrentTransferCount(i32ChannID);

    return (i32TriggerByteLen - cur_txcnt);

exit_nu_edma_transferred_byte_get:

    return -1;
}

nu_edma_memctrl_t nu_edma_channel_memctrl_get(int i32ChannID)
{
    nu_edma_memctrl_t eMemCtrl = eMemCtl_Undefined;

    if (!(nu_edma_chn_mask & (1 << i32ChannID)))
        goto exit_nu_edma_channel_memctrl_get;

    eMemCtrl = nu_edma_chn_arr[i32ChannID].m_spPeripCtl.m_eMemCtl;

exit_nu_edma_channel_memctrl_get:

    return eMemCtrl;
}

int nu_edma_channel_memctrl_set(int i32ChannID, nu_edma_memctrl_t eMemCtrl)
{
    int ret = -1;
    nu_edma_chn_t *psEdmaChann = &nu_edma_chn_arr[i32ChannID];


    if (!(nu_edma_chn_mask & (1 << i32ChannID)))
        goto exit_nu_edma_channel_memctrl_set;
    else if ((eMemCtrl < eMemCtl_SrcFix_DstFix) || (eMemCtrl > eMemCtl_SrcInc_DstInc))
        goto exit_nu_edma_channel_memctrl_set;

    /* PDMA_MEM/SAR_FIX/BURST mode is not supported. */
    if ((psEdmaChann->m_spPeripCtl.m_ePeripheralType == EDMA_MEM) &&
            ((eMemCtrl == eMemCtl_SrcFix_DstInc) || (eMemCtrl == eMemCtl_SrcFix_DstFix)))
        goto exit_nu_edma_channel_memctrl_set;

    nu_edma_chn_arr[i32ChannID].m_spPeripCtl.m_eMemCtl = eMemCtrl;

    ret = 0;

exit_nu_edma_channel_memctrl_set:

    return ret;
}

static void nu_edma_channel_memctrl_fill(nu_edma_memctrl_t eMemCtl, uint32_t *pu32SrcCtl, uint32_t *pu32DstCtl)
{
    switch ((int)eMemCtl)
    {
    case eMemCtl_SrcFix_DstFix:
        *pu32SrcCtl = eDRVEDMA_DIRECTION_FIXED;
        *pu32DstCtl = eDRVEDMA_DIRECTION_FIXED;
        break;
    case eMemCtl_SrcFix_DstInc:
        *pu32SrcCtl = eDRVEDMA_DIRECTION_FIXED;
        *pu32DstCtl = eDRVEDMA_DIRECTION_INCREMENTED;
        break;
    case eMemCtl_SrcInc_DstFix:
        *pu32SrcCtl = eDRVEDMA_DIRECTION_INCREMENTED;
        *pu32DstCtl = eDRVEDMA_DIRECTION_FIXED;
        break;
    case eMemCtl_SrcInc_DstInc:
        *pu32SrcCtl = eDRVEDMA_DIRECTION_INCREMENTED;
        *pu32DstCtl = eDRVEDMA_DIRECTION_INCREMENTED;
        break;
    case eMemCtl_SrcFix_DstWrap:
        *pu32SrcCtl = eDRVEDMA_DIRECTION_FIXED;
        *pu32DstCtl = eDRVEDMA_DIRECTION_WRAPAROUND;
        break;
    case eMemCtl_SrcInc_DstWrap:
        *pu32SrcCtl = eDRVEDMA_DIRECTION_INCREMENTED;
        *pu32DstCtl = eDRVEDMA_DIRECTION_WRAPAROUND;
        break;
    default:
        break;
    }
}

/* This is for Scatter-gather DMA. */
int nu_edma_trans_setup(
	int i32ChannID,
	uint32_t u32DataWidth,
	uint32_t u32AddrSrc,
	uint32_t u32AddrDst,
	int32_t i32TransferCnt
)
{
	
	nu_edma_periph_ctl_t *psPeriphCtl = NULL;	
	S_DRVEDMA_CH_ADDR_SETTING sSrcAddr; 
	S_DRVEDMA_CH_ADDR_SETTING sDestAddr; 
	E_DRVEDMA_TRANSFER_WIDTH eTransWidth;
	uint32_t u32SrcCtl = 0;
    uint32_t u32DstCtl = 0;
	uint32_t u32TransByte;

	if(!(nu_edma_chn_mask & (1 << i32ChannID)))
		return -1;

    if(!(u32DataWidth == 8 || u32DataWidth == 16 || u32DataWidth == 32))
		return -2;
		
    if((u32AddrSrc % (u32DataWidth / 8)) || (u32AddrDst % (u32DataWidth / 8)))
		return -3;
        
	psPeriphCtl = &nu_edma_chn_arr[i32ChannID].m_spPeripCtl;                
	nu_edma_channel_memctrl_fill(psPeriphCtl->m_eMemCtl, &u32SrcCtl, &u32DstCtl);
	sSrcAddr.u32Addr = u32AddrSrc;
	sSrcAddr.eAddrDirection = u32SrcCtl;
	sDestAddr.u32Addr = u32AddrDst;
	sDestAddr.eAddrDirection = u32DstCtl;

	if(u32DataWidth == 32)
	{
		u32TransByte = i32TransferCnt * 4;
		eTransWidth = eDRVEDMA_WIDTH_32BITS;
	}
	else if(u32DataWidth == 16)
	{
		u32TransByte = i32TransferCnt * 2;
		eTransWidth = eDRVEDMA_WIDTH_16BITS;
	}
	else
	{
		u32TransByte = i32TransferCnt * 1;
		eTransWidth = eDRVEDMA_WIDTH_8BITS;
	}

	if(psPeriphCtl->m_ePeripheralType == EDMA_MEM)
	{
		
	}
	else
	{		
		DrvEDMA_SetAPBTransferWidth((E_DRVEDMA_CHANNEL_INDEX)i32ChannID, eTransWidth);
		sysSetLocalInterrupt(DISABLE_IRQ);
		DrvEDMA_SetCHForAPBDevice((E_DRVEDMA_CHANNEL_INDEX)i32ChannID, psPeriphCtl->m_eAPBDevice, psPeriphCtl->m_eAPBRW);
		sysSetLocalInterrupt(ENABLE_IRQ);
	}

	DrvEDMA_SetTransferSetting((E_DRVEDMA_CHANNEL_INDEX)i32ChannID, &sSrcAddr, &sDestAddr, u32TransByte);

	return	0;
}

static void _nu_edma_transfer(
	int i32ChannID,
	E_DRVEDMA_WRAPAROUND_SELECT eWrapSelect
)
{
	if(eWrapSelect != eDRVEDMA_WRAPAROUND_NO_INT)
	{
		DrvEDMA_SetWrapIntType((E_DRVEDMA_CHANNEL_INDEX)i32ChannID, eWrapSelect);
		DrvEDMA_DisableInt((E_DRVEDMA_CHANNEL_INDEX)i32ChannID,(E_DRVEDMA_INT_ENABLE)(eDRVEDMA_SG | eDRVEDMA_BLKD | eDRVEDMA_TABORT));
		DrvEDMA_EnableInt((E_DRVEDMA_CHANNEL_INDEX)i32ChannID, eDRVEDMA_WAR);
	}
	else
	{
		DrvEDMA_DisableInt((E_DRVEDMA_CHANNEL_INDEX)i32ChannID,eDRVEDMA_WAR);
		DrvEDMA_EnableInt((E_DRVEDMA_CHANNEL_INDEX)i32ChannID, (E_DRVEDMA_INT_ENABLE)(eDRVEDMA_SG | eDRVEDMA_BLKD | eDRVEDMA_TABORT));
	}
	
	DrvEDMA_CHEnablelTransfer((E_DRVEDMA_CHANNEL_INDEX)i32ChannID);
}

int nu_edma_transfer(
	int i32ChannID,
	uint32_t u32DataWidth,
	uint32_t u32AddrSrc,
	uint32_t u32AddrDst,
	int32_t i32TransferCnt,
	E_DRVEDMA_WRAPAROUND_SELECT eWrapSelect
)
{
	int ret;
//    nu_edma_periph_ctl_t *psPeriphCtl = NULL;

    if (!(nu_edma_chn_mask & (1 << i32ChannID)))
        return -1;

//    psPeriphCtl = &nu_edma_chn_arr[i32ChannID].m_spPeripCtl;

	ret = nu_edma_trans_setup(i32ChannID, u32DataWidth, u32AddrSrc, u32AddrDst, i32TransferCnt);
	
	if(ret != 0)
		return -2;

	_nu_edma_transfer(i32ChannID, eWrapSelect);
	
	return 0;
}

void nu_edma_memfun_actor_init(void)
{
    int i = 0 ;
    nu_edma_init();

    for (i = 0; i < NU_EDMA_MEMFUN_ACTOR_MAX; i++)
    {
        memset(&nu_edma_memfun_actor_arr[i], 0, sizeof(struct nu_edma_memfun_actor));
        if (-1 != (nu_edma_memfun_actor_arr[i].m_i32ChannID = nu_edma_channel_allocate(EDMA_MEM)))
        {
#if MICROPY_PY_THREAD
            nu_edma_memfun_actor_arr[i].m_psSemMemFun = xSemaphoreCreateBinary();
#else
            nu_edma_memfun_actor_arr[i].m_bWaitMemFun = false;
#endif
        }
        else
            break;
    }
    
    if (i)
    {
        nu_edma_memfun_actor_maxnum = i;
        nu_edma_memfun_actor_mask = ~(((1 << i) - 1));

#if MICROPY_PY_THREAD
        nu_edma_memfun_actor_pool_sem = xSemaphoreCreateCounting(nu_edma_memfun_actor_maxnum, nu_edma_memfun_actor_maxnum);
        nu_edma_memfun_actor_pool_lock = xSemaphoreCreateMutex();
#else
        nu_edma_memfun_actor_pool_wait = false;
#endif
    }
}

#if MICROPY_PY_THREAD
static portBASE_TYPE xHigherPrioTaskWoken = pdFALSE;
#endif

static void nu_edma_memfun_cb(void *pvUserData, uint32_t u32Events)
{
    nu_edma_memfun_actor_t psMemFunActor = (nu_edma_memfun_actor_t)pvUserData;
    psMemFunActor->m_u32Result = u32Events;
#if MICROPY_PY_THREAD
	xSemaphoreGiveFromISR(psMemFunActor->m_psSemMemFun, &xHigherPrioTaskWoken);
#else
	psMemFunActor->m_bWaitMemFun = false;
#endif
}

static int nu_edma_memfun_employ(void)
{
    int idx = -1 ;
	int i;
	
   /* Headhunter */
#if MICROPY_PY_THREAD
    if (nu_edma_memfun_actor_pool_sem && (xSemaphoreTake(nu_edma_memfun_actor_pool_sem, portMAX_DELAY)))
	{
#endif
		EDMA_MEMFUN_MUTEX_LOCK();

		for(i = 0; i < NU_EDMA_MEMFUN_ACTOR_MAX; i ++)
		{
			if((nu_edma_memfun_actor_mask & (1 << i)) == 0)
			{
				idx = i;
				break;
			}
		}

		EDMA_MEMFUN_MUTEX_UNLOCK();	
#if MICROPY_PY_THREAD
	}
#endif

	return idx;
}

static int nu_edma_memfun(
	void *dest,
	void *src,
	uint32_t u32DataWidth,
	unsigned int count,
	nu_edma_memctrl_t eMemCtl
)
{
    nu_edma_memfun_actor_t psMemFunActor = NULL;
    int idx;
    int ret = 0;

    while (1)
    {
		/* Employ actor */
		if ((idx = nu_edma_memfun_employ()) < 0)
			continue;

		psMemFunActor = &nu_edma_memfun_actor_arr[idx];

        psMemFunActor->m_u32TrigTransferCnt = count;

		/* Set EDMA memory control to eMemCtl. */
		nu_edma_channel_memctrl_set(psMemFunActor->m_i32ChannID, eMemCtl);

		/* Register ISR callback function */
		nu_edma_callback_register(psMemFunActor->m_i32ChannID, nu_edma_memfun_cb, (void *)psMemFunActor, NU_PDMA_EVENT_ABORT | NU_PDMA_EVENT_TRANSFER_DONE);

		psMemFunActor->m_u32Result = 0;

#if !MICROPY_PY_THREAD
		psMemFunActor->m_bWaitMemFun = true;
#endif
		/* Trigger it */
		nu_edma_transfer(psMemFunActor->m_i32ChannID, u32DataWidth, (uint32_t)src, (uint32_t)dest, count, eDRVEDMA_WRAPAROUND_NO_INT);

		/* Wait it done. */
#if MICROPY_PY_THREAD
		xSemaphoreTake(psMemFunActor->m_psSemMemFun, portMAX_DELAY);
#else
		while(psMemFunActor->m_bWaitMemFun == true)
		{
		}
#endif

		/* Give result if get NU_PDMA_EVENT_TRANSFER_DONE.*/
		if (psMemFunActor->m_u32Result & NU_PDMA_EVENT_TRANSFER_DONE)
		{
			ret = psMemFunActor->m_u32TrigTransferCnt;
		}
		else
		{
			ret = psMemFunActor->m_u32TrigTransferCnt - nu_edma_non_transfer_count_get(psMemFunActor->m_i32ChannID);
		}

        /* Terminate it if get ABORT event */
        if (psMemFunActor->m_u32Result & NU_PDMA_EVENT_ABORT)
        {
            nu_edma_channel_terminate(psMemFunActor->m_i32ChannID);
        }

		EDMA_MEMFUN_MUTEX_LOCK();
        nu_edma_memfun_actor_mask &= ~(1 << idx);
		EDMA_MEMFUN_MUTEX_UNLOCK();

        /* Fire actor */
#if MICROPY_PY_THREAD
		xSemaphoreGive(nu_edma_memfun_actor_pool_sem);
#endif

		break;
	}

	return ret;
}

int nu_edma_mempush(void *dest, void *src, unsigned int transfer_byte)
{
	return nu_edma_memfun(dest, src, 8, transfer_byte, eMemCtl_SrcInc_DstFix);
}

void *nu_edma_memcpy(void *dest, void *src, unsigned int count)
{
    if (count == nu_edma_memfun(dest, src, 8, count, eMemCtl_SrcInc_DstInc))
        return dest;
    else
        return NULL;
}
