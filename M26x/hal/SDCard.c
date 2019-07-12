/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "py/obj.h"
#include "mods/pybirq.h"

#include "SDCard.h"

#define Sector_Size 512   //512 bytes
static uint32_t Tmp_Buffer[Sector_Size];

static SDH_T *s_pSDH = SDH0;
static SDH_INFO_T *s_psSDInfo = &SD0;

extern uint8_t volatile g_u8SDDataReadyFlag;
extern uint8_t g_u8R3Flag;

void SDH0_IRQHandler(void)
{
    unsigned int volatile isr;
    unsigned int volatile ier;

	IRQ_ENTER(SDH0_IRQn);
    // FMI data abort interrupt
    if (s_pSDH->GINTSTS & SDH_GINTSTS_DTAIF_Msk)
    {
        /* ResetAllEngine() */
        s_pSDH->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }

    //----- SD interrupt status
    isr = s_pSDH->INTSTS;
    if (isr & SDH_INTSTS_BLKDIF_Msk)
    {
        // block down
        g_u8SDDataReadyFlag = TRUE;
        s_pSDH->INTSTS = SDH_INTSTS_BLKDIF_Msk;
    }

    if (isr & SDH_INTSTS_CDIF_Msk)   // card detect
    {
        //----- SD interrupt status
        // it is work to delay 50 times for SD_CLK = 200KHz
        {
            int volatile i;         // delay 30 fail, 50 OK
            for (i=0; i<0x500; i++);  // delay to make sure got updated value from REG_SDISR.
            isr = s_pSDH->INTSTS;
        }

        if (isr & SDH_INTSTS_CDSTS_Msk)
        {
            printf("\n***** card remove !\n");
            s_psSDInfo->IsCardInsert = FALSE;   // SDISR_CD_Card = 1 means card remove for GPIO mode
            memset(s_psSDInfo, 0, sizeof(SDH_INFO_T));
        }
        else
        {
            printf("***** card insert !\n");
            SDH_Open(s_pSDH, CardDetect_From_GPIO);
            SDH_Probe(s_pSDH);
        }

        s_pSDH->INTSTS = SDH_INTSTS_CDIF_Msk;
    }

    // CRC error interrupt
    if (isr & SDH_INTSTS_CRCIF_Msk)
    {
        if (!(isr & SDH_INTSTS_CRC16_Msk))
        {
            //printf("***** ISR sdioIntHandler(): CRC_16 error !\n");
            // handle CRC error
        }
        else if (!(isr & SDH_INTSTS_CRC7_Msk))
        {
            if (!g_u8R3Flag)
            {
                //printf("***** ISR sdioIntHandler(): CRC_7 error !\n");
                // handle CRC error
            }
        }
        s_pSDH->INTSTS = SDH_INTSTS_CRCIF_Msk;      // clear interrupt flag
    }

    if (isr & SDH_INTSTS_DITOIF_Msk)
    {
        printf("***** ISR: data in timeout !\n");
        s_pSDH->INTSTS |= SDH_INTSTS_DITOIF_Msk;
    }

    // Response in timeout interrupt
    if (isr & SDH_INTSTS_RTOIF_Msk)
    {
        printf("***** ISR: response in timeout !\n");
        s_pSDH->INTSTS |= SDH_INTSTS_RTOIF_Msk;
    }

	IRQ_EXIT(SDH0_IRQn);
}



int32_t SDCard_Init(void){
    //for SD0

    /* select multi-function pin */
    /* CD: PB12(9), PD13(3) */
    //SYS->GPB_MFPH = (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB12MFP_Msk)) | SD0_nCD_PB12;
    SYS->GPD_MFPH = (SYS->GPD_MFPH & (~SYS_GPD_MFPH_PD13MFP_Msk)) | SD0_nCD_PD13;

    /* CLK: PB1(3), PE6(3) */
    //SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB1MFP_Msk)) | SD0_CLK_PB1;
    SYS->GPE_MFPL = (SYS->GPE_MFPL & (~SYS_GPE_MFPL_PE6MFP_Msk)) | SD0_CLK_PE6;

    /* CMD: PB0(3), PE7(3) */
    //SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB0MFP_Msk)) | SD0_CMD_PB0;
    SYS->GPE_MFPL = (SYS->GPE_MFPL & (~SYS_GPE_MFPL_PE7MFP_Msk)) | SD0_CMD_PE7;

    /* D0: PB2(3), PE2(3) */
    //SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB2MFP_Msk)) | SD0_DAT0_PB2;
    SYS->GPE_MFPL = (SYS->GPE_MFPL & (~SYS_GPE_MFPL_PE2MFP_Msk)) | SD0_DAT0_PE2;

    /* D1: PB3(3), PE3(3) */
    //SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB3MFP_Msk)) | SD0_DAT1_PB3;
    SYS->GPE_MFPL = (SYS->GPE_MFPL & (~SYS_GPE_MFPL_PE3MFP_Msk)) | SD0_DAT1_PE3;

    /* D2: PB4(3), PE4(3) */
    //SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB4MFP_Msk)) | SD0_DAT2_PB4;
    SYS->GPE_MFPL = (SYS->GPE_MFPL & (~SYS_GPE_MFPL_PE4MFP_Msk)) | SD0_DAT2_PE4;

    /* D3: PB5(3)-, PE5(3) */
    //SYS->GPB_MFPL = (SYS->GPB_MFPL & (~SYS_GPB_MFPL_PB5MFP_Msk)) | SD0_DAT3_PB5;
    SYS->GPE_MFPL = (SYS->GPE_MFPL & (~SYS_GPE_MFPL_PE5MFP_Msk)) | SD0_DAT3_PE5;

    /* Select IP clock source */
    CLK_SetModuleClock(SDH0_MODULE, CLK_CLKSEL0_SDH0SEL_HCLK, CLK_CLKDIV0_SDH0(1));

    /* Enable IP clock */
    CLK_EnableModuleClock(SDH0_MODULE);

    SystemCoreClockUpdate();

    SDH_Open(s_pSDH, CardDetect_From_GPIO);
    SDH_Probe(s_pSDH);


    return 0;
}


int32_t SDCard_CardDetection(void){
    return SDH_CardDetection(s_pSDH);
}

int32_t SDCard_Read (
    uint8_t *buff,     /* Data buffer to store read data */
    uint32_t sector,   /* Sector address (LBA) */
    uint32_t count      /* Number of sectors to read (1..128) */
)
{
    int32_t ret;
    uint32_t shift_buf_flag = 0;
    uint32_t tmp_StartBufAddr;

//    printf("disk_read - sec:%d, cnt:%d, buff:0x%x\n",sector, count, (uint32_t)buff);

    if ((uint32_t)buff%4)
    {
        shift_buf_flag = 1;
    }

    if(shift_buf_flag == 1)
    {
        if(count == 1)
        {
            ret = SDH_Read(s_pSDH, (uint8_t*)(&Tmp_Buffer), sector, count);
            memcpy(buff, (&Tmp_Buffer), count*s_psSDInfo->sectorSize);
        }
        else
        {
            tmp_StartBufAddr = (((uint32_t)buff/4 + 1) * 4);
            ret = SDH_Read(s_pSDH, ((uint8_t*)tmp_StartBufAddr), sector, (count -1));
            memcpy(buff, (void*)tmp_StartBufAddr, (s_psSDInfo->sectorSize*(count-1)) );
            ret = SDH_Read(s_pSDH, (uint8_t*)(&Tmp_Buffer), (sector+count-1), 1);
            memcpy( (buff+(s_psSDInfo->sectorSize*(count-1))), (void*)Tmp_Buffer, s_psSDInfo->sectorSize);
        }
    }
    else
    {
        ret = SDH_Read(s_pSDH, buff, sector, count);
    }

    return ret;
}


int32_t SDCard_Write (
    const uint8_t *buff,   /* Data to be written */
    uint32_t sector,       /* Sector address (LBA) */
    uint32_t count          /* Number of sectors to write (1..128) */
)
{
    int32_t  ret;
    uint32_t shift_buf_flag = 0;
    uint32_t tmp_StartBufAddr;
    uint32_t volatile i;

    //printf("disk_write - drv:%d, sec:%d, cnt:%d, buff:0x%x\n", pdrv, sector, count, (uint32_t)buff);
    if ((uint32_t)buff%4)
    {
        shift_buf_flag = 1;
    }

    if(shift_buf_flag == 1)
    {
        if(count == 1)
        {
            memcpy((&Tmp_Buffer), buff, count*s_psSDInfo->sectorSize);
            ret = SDH_Write(s_pSDH, (uint8_t*)(&Tmp_Buffer), sector, count);
        }
        else
        {
            tmp_StartBufAddr = (((uint32_t)buff/4 + 1) * 4);
            memcpy((void*)Tmp_Buffer, (buff+(s_psSDInfo->sectorSize*(count-1))), s_psSDInfo->sectorSize);

            for(i = (s_psSDInfo->sectorSize*(count-1)); i > 0; i--)
            {
                memcpy((void *)(tmp_StartBufAddr + i - 1), (buff + i -1), 1);
            }

            ret = SDH_Write(s_pSDH, ((uint8_t*)tmp_StartBufAddr), sector, (count -1));
            ret = SDH_Write(s_pSDH, (uint8_t*)(&Tmp_Buffer), (sector+count-1), 1);
        }
    }
    else
    {
        ret = SDH_Write(s_pSDH, (uint8_t *)buff, sector, count);
    }

    return ret;
}

int32_t SDCard_GetCardInfo(
    S_SDCARD_INFO *psCardInfo
)
{
    psCardInfo->CardType =  s_psSDInfo->CardType;
    psCardInfo->RCA = s_psSDInfo->RCA;
    psCardInfo->IsCardInsert = s_psSDInfo->IsCardInsert;
    psCardInfo->totalSectorN = s_psSDInfo->totalSectorN; 
    psCardInfo->diskSize = s_psSDInfo->diskSize;
    psCardInfo->sectorSize = s_psSDInfo->sectorSize;

    return 0;
}




