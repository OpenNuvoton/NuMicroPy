/**************************************************************************//**
 * @file     N9H26_ETH.c
 * @version  V0.01
 * @brief    N9H26 EMAC driver file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#if MICROPY_NVT_LWIP

#include "FreeRTOS.h"
#include "task.h"

#include "wblib.h"
#include "N9H26_reg.h"

#include "emac_reg.h"
#include "N9H26_ETH.h"

#define ETH_TRIGGER_RX()    do{EMAC->RSDR = 0;}while(0)
#define ETH_TRIGGER_TX()    do{EMAC->TSDR = 0;}while(0)
#define ETH_ENABLE_TX()     do{EMAC->MCMDR |= EMAC_MCMDR_TXON;}while(0)
#define ETH_ENABLE_RX()     do{EMAC->MCMDR |= EMAC_MCMDR_RXON;}while(0)
#define ETH_DISABLE_TX()    do{EMAC->MCMDR &= ~EMAC_MCMDR_TXON;}while(0)
#define ETH_DISABLE_RX()    do{EMAC->MCMDR &= ~EMAC_MCMDR_RXON;}while(0)

struct eth_descriptor rx_desc[RX_DESCRIPTOR_NUM] __attribute__ ((aligned(4)));
struct eth_descriptor tx_desc[TX_DESCRIPTOR_NUM] __attribute__ ((aligned(4)));
struct eth_descriptor volatile *cur_tx_desc_ptr, *cur_rx_desc_ptr, *fin_tx_desc_ptr;


uint8_t rx_buf[RX_DESCRIPTOR_NUM][PACKET_BUFFER_SIZE];
uint8_t tx_buf[TX_DESCRIPTOR_NUM][PACKET_BUFFER_SIZE];

extern void ethernetif_input(uint16_t len, uint8_t *buf, uint32_t s, uint32_t ns);
extern void ethernetif_loopback_input(struct pbuf *p);

extern portBASE_TYPE xInsideISR;


void EMAC_RX_IRQHandler(void)
{
    unsigned int status;

    xInsideISR = pdTRUE;
    status = EMAC->MISTA & 0xFFFF;
    EMAC->MISTA = status;
    if (status & EMAC_MISTA_RXBERR_Msk)
    {
        // Shouldn't goes here, unless descriptor corrupted
    }

    do
    {

        //cur_entry = EMAC->CRXDSA;

        //if ((cur_entry == (u32_t)cur_rx_desc_ptr) && (!(status & EMAC_INTSTS_RDUIF_Msk)))  // cur_entry may equal to cur_rx_desc_ptr if RDU occures
        //    break;
        status = cur_rx_desc_ptr->status1;

        if(status & OWNERSHIP_EMAC)
            break;

        if (status & RXFD_RXGD)
        {
            ethernetif_input(status & 0xFFFF, cur_rx_desc_ptr->buf, cur_rx_desc_ptr->status2, (uint32_t)cur_rx_desc_ptr->next);
        }

        cur_rx_desc_ptr->status1 = OWNERSHIP_EMAC;
        cur_rx_desc_ptr = cur_rx_desc_ptr->next;

    }
    while (1);

    ETH_TRIGGER_RX();
    xInsideISR = pdFALSE;

}

void EMAC_TX_IRQHandler(void)
{
#if 0
    unsigned int cur_entry; 
#endif
    unsigned int status;


    xInsideISR = pdTRUE;
    status = EMAC->MISTA & 0xFFFF0000;
    EMAC->MISTA = status;
    if(status & EMAC_MISTA_TXBERR_Msk)
    {
        // Shouldn't goes here, unless descriptor corrupted
        return;
    }

#if 0
    cur_entry = EMAC->CTXDSA;

    while (cur_entry != (uint32_t)fin_tx_desc_ptr)
    {
        fin_tx_desc_ptr = fin_tx_desc_ptr->next;
    }
#endif
    xInsideISR = pdFALSE;
}

int32_t ETH_Init0(void){
    /* Enable IP clock */
    outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | EMAC_CKE);

	/*Config interrupt handler*/
	sysInstallISR(IRQ_LEVEL_1, IRQ_EMCTX, (PVOID) EMAC_TX_IRQHandler);
	sysInstallISR(IRQ_LEVEL_1, IRQ_EMCRX, (PVOID) EMAC_RX_IRQHandler);
	
	sysEnableInterrupt(IRQ_EMCTX);
	sysEnableInterrupt(IRQ_EMCRX);
	
	return 0;
}

static void mdio_write(uint8_t addr, uint8_t reg, uint16_t val)
{

    EMAC->MIID = val;
    EMAC->MIIDA = (addr << EMAC_MIIDA_PHYAD_Pos) | reg | EMAC_MIIDA_BUSY_Msk | EMAC_MIIDA_WRITE_Msk | EMAC_MIIDA_MDCON_Msk;

    while (EMAC->MIIDA & EMAC_MIIDA_BUSY_Msk);
}

static uint16_t mdio_read(uint8_t addr, uint8_t reg)
{
    EMAC->MIIDA = (addr << EMAC_MIIDA_PHYAD_Pos) | reg | EMAC_MIIDA_BUSY_Msk | EMAC_MIIDA_MDCON_Msk;
    while (EMAC->MIIDA & EMAC_MIIDA_BUSY_Msk);

    return(EMAC->MIID);
}

static int reset_phy(void)
{
    uint16_t reg;
    uint32_t delay;


    mdio_write(CONFIG_PHY_ADDR, MII_BMCR, BMCR_RESET);

    delay = 2000;
    while(delay-- > 0)
    {
        if((mdio_read(CONFIG_PHY_ADDR, MII_BMCR) & BMCR_RESET) == 0)
            break;

    }

    if(delay == 0)
    {
        printf("Reset phy failed\n");
        return(-1);
    }

    mdio_write(CONFIG_PHY_ADDR, MII_ADVERTISE, ADVERTISE_CSMA |
               ADVERTISE_10HALF |
               ADVERTISE_10FULL |
               ADVERTISE_100HALF |
               ADVERTISE_100FULL);

    reg = mdio_read(CONFIG_PHY_ADDR, MII_BMCR);
    mdio_write(CONFIG_PHY_ADDR, MII_BMCR, reg | BMCR_ANRESTART);

    delay = 200000;
    while(delay-- > 0)
    {
        if((mdio_read(CONFIG_PHY_ADDR, MII_BMSR) & (BMSR_ANEGCOMPLETE | BMSR_LSTATUS))
                == (BMSR_ANEGCOMPLETE | BMSR_LSTATUS))
            break;
    }

    if(delay == 0)
    {
        printf("AN failed. Set to 100 FULL\n");
        EMAC->MCMDR |= (EMAC_MCMDR_OPMOD_Msk | EMAC_MCMDR_FDUP_Msk);
        return(-1);
    }
    else
    {
        reg = mdio_read(CONFIG_PHY_ADDR, MII_LPA);

        if(reg & ADVERTISE_100FULL)
        {
            printf("100 full\n");
            EMAC->MCMDR |= (EMAC_MCMDR_OPMOD_Msk | EMAC_MCMDR_FDUP_Msk);
        }
        else if(reg & ADVERTISE_100HALF)
        {
            printf("100 half\n");
            EMAC->MCMDR = (EMAC->MCMDR & ~EMAC_MCMDR_FDUP_Msk) | EMAC_MCMDR_OPMOD_Msk;
        }
        else if(reg & ADVERTISE_10FULL)
        {
            printf("10 full\n");
            EMAC->MCMDR = (EMAC->MCMDR & ~EMAC_MCMDR_OPMOD_Msk) | EMAC_MCMDR_FDUP_Msk;
        }
        else
        {
            printf("10 half\n");
            EMAC->MCMDR &= ~(EMAC_MCMDR_OPMOD_Msk | EMAC_MCMDR_FDUP_Msk);
        }
    }

    return(0);
}

static void init_tx_desc(void)
{
    uint32_t i;


    cur_tx_desc_ptr = fin_tx_desc_ptr = &tx_desc[0];

    for(i = 0; i < TX_DESCRIPTOR_NUM; i++)
    {
        tx_desc[i].status1 = TXFD_PADEN | TXFD_CRCAPP | TXFD_INTEN;
        tx_desc[i].buf = (uint8_t *)((UINT)(&tx_buf[i][0]) | 0x80000000);
        tx_desc[i].status2 = 0;
        tx_desc[i].next = (struct eth_descriptor *)((UINT)(&tx_desc[(i + 1) % TX_DESCRIPTOR_NUM]) | 0x80000000);
    }
    EMAC->TXDLSA = (unsigned int)(&tx_desc[0]) | 0x80000000;
    return;
}

static void init_rx_desc(void)
{
    uint32_t i;


    cur_rx_desc_ptr = &rx_desc[0];

    for(i = 0; i < RX_DESCRIPTOR_NUM; i++)
    {
        rx_desc[i].status1 = OWNERSHIP_EMAC;
        rx_desc[i].buf = (uint8_t *)((UINT)(&rx_buf[i][0]) | 0x80000000);
        rx_desc[i].status2 = 0;
        rx_desc[i].next = (struct eth_descriptor *)((UINT)(&rx_desc[(i + 1) % RX_DESCRIPTOR_NUM]) | 0x80000000);
    }
    EMAC->RXDLSA = (unsigned int)(&rx_desc[0]) | 0x80000000;
    return;
}

static void set_mac_addr(uint8_t *addr)
{
    uint32_t u32Lsw, u32Msw;
    uint32_t reg;
    uint32_t u32Entry = 0;
    
    u32Lsw = (uint32_t)(((uint32_t)addr[4] << 24) |
                        ((uint32_t)addr[5] << 16));
    u32Msw = (uint32_t)(((uint32_t)addr[0] << 24)|
                        ((uint32_t)addr[1] << 16)|
                        ((uint32_t)addr[2] << 8)|
                        (uint32_t)addr[3]);

    reg = (uint32_t)&EMAC->CAM0M + u32Entry * 2UL * 4UL;
    *(uint32_t volatile *)reg = u32Msw;
    reg = (uint32_t)&EMAC->CAM0L + u32Entry * 2UL * 4UL;
    *(uint32_t volatile *)reg = u32Lsw;

    EMAC->CAMEN |= (1UL << u32Entry);
}


void ETH_init(uint8_t *mac_addr)
{

    // Reset MAC
    EMAC->MCMDR = EMAC_MCMDR_SWR_Msk;
    while(EMAC->MCMDR & EMAC_MCMDR_SWR_Msk) {}

    init_tx_desc();
    init_rx_desc();

    set_mac_addr(mac_addr);  // need to reconfigure hardware address 'cos we just RESET emc...
    reset_phy();

    EMAC->MCMDR |= EMAC_MCMDR_SPCRC_Msk | EMAC_MCMDR_RXON_Msk | EMAC_MCMDR_TXON_Msk | EMAC_MCMDR_ENRMII_Msk;
    EMAC->MIEN |= EMAC_MIEN_ENRXINTR_Msk |
                   EMAC_MIEN_ENRXGD_Msk |
                   EMAC_MIEN_ENRDU_Msk |
                   EMAC_MIEN_ENRXBERR_Msk |
                   EMAC_MIEN_ENTXINTR_Msk |
                   EMAC_MIEN_ENTXBERR_Msk |
                   EMAC_MIEN_ENTXCP_Msk |
                   EMAC_MIEN_ENTXABT_Msk;

    /* Limit the max receive frame length to 1514 + 4 */
    EMAC->DMARFC = PACKET_BUFFER_SIZE;
    EMAC->RSDR = 0;  // trigger Rx
}

void  ETH_halt(void)
{

    EMAC->MCMDR &= ~(EMAC_MCMDR_RXON_Msk | EMAC_MCMDR_TXON_Msk);
}

uint8_t *ETH_get_tx_buf(void)
{
    if(cur_tx_desc_ptr->status1 & OWNERSHIP_EMAC)
        return(NULL);
    else
        return(cur_tx_desc_ptr->buf);
}

void ETH_trigger_tx(uint16_t length, struct pbuf *p)
{
    struct eth_descriptor volatile *desc;
    cur_tx_desc_ptr->status2 = (unsigned int)length;
    desc = cur_tx_desc_ptr->next;    // in case TX is transmitting and overwrite next pointer before we can update cur_tx_desc_ptr
    cur_tx_desc_ptr->status1 |= OWNERSHIP_EMAC;
    cur_tx_desc_ptr = desc;
    ETH_TRIGGER_TX();
}

#endif
