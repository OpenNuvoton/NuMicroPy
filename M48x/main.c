
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "py/stackctrl.h"
#include "py/nlr.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "lib/mp-readline/readline.h"
#include "lib/utils/pyexec.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "NuMicro.h"

#include "mpconfigboard_common.h"
#include "mods/pybflash.h"
#include "mods/pybspiflash.h"
#include "mods/pybsdcard.h"
#include "mods/pybpin.h"
#include "mods/modnetwork.h"
#include "hal/pin_int.h"
#include "hal/M48x_USBD.h"
#include "hal/MSC_VCPTrans.h"
#include "hal/StorIF.h"

#include "misc/mperror.h"

extern mp_uint_t gc_helper_get_sp(void);
extern mp_uint_t gc_helper_get_regs_and_sp(mp_uint_t *regs);


void SYS_Init(void)
{
    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable external XTAL 12MHz clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for external XTAL clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

   /* Switch HCLK clock source to HXT */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT,CLK_CLKDIV0_HCLK(1));

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(192000000);

    /* Set both PCLK0 and PCLK1 as HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_PCLK0DIV2 | CLK_PCLKDIV_PCLK1DIV2); // PCLK divider set 2

    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);


    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));

#if ENABLE_SPIM
    /* Enable SPIM module clock */
    CLK_EnableModuleClock(SPIM_MODULE);
#endif

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and cyclesPerUs automatically. */
    SystemCoreClockUpdate();

#if 1
    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);
#else
    /* Set GPH multi-function pins for UART0 RXD and TXD for DAC testing*/
    SYS->GPH_MFPH &= ~(SYS_GPH_MFPH_PH10MFP_Msk | SYS_GPH_MFPH_PH11MFP_Msk);
    SYS->GPH_MFPH |= (SYS_GPH_MFPH_PH10MFP_UART0_TXD | SYS_GPH_MFPH_PH11MFP_UART0_RXD);
#endif

#if ENABLE_SPIM
    /* Init SPIM multi-function pins, MOSI(PC.0), MISO(PC.1), CLK(PC.2), SS(PC.3), D3(PC.4), and D2(PC.5) */
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC0MFP_Msk | SYS_GPC_MFPL_PC1MFP_Msk | SYS_GPC_MFPL_PC2MFP_Msk |
                       SYS_GPC_MFPL_PC3MFP_Msk | SYS_GPC_MFPL_PC4MFP_Msk | SYS_GPC_MFPL_PC5MFP_Msk);
    SYS->GPC_MFPL |= SYS_GPC_MFPL_PC0MFP_SPIM_MOSI | SYS_GPC_MFPL_PC1MFP_SPIM_MISO |
                     SYS_GPC_MFPL_PC2MFP_SPIM_CLK | SYS_GPC_MFPL_PC3MFP_SPIM_SS |
                     SYS_GPC_MFPL_PC4MFP_SPIM_D3 | SYS_GPC_MFPL_PC5MFP_SPIM_D2;
    PC->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;

    /* Set SPIM I/O pins as high slew rate up to 80 MHz. */
    PC->SLEWCTL = (PC->SLEWCTL & 0xFFFFF000) |
                  (0x1<<GPIO_SLEWCTL_HSREN0_Pos) | (0x1<<GPIO_SLEWCTL_HSREN1_Pos) |
                  (0x1<<GPIO_SLEWCTL_HSREN2_Pos) | (0x1<<GPIO_SLEWCTL_HSREN3_Pos) |
                  (0x1<<GPIO_SLEWCTL_HSREN4_Pos) | (0x1<<GPIO_SLEWCTL_HSREN5_Pos);

#endif

#if !MICROPY_PY_THREAD
	/*Enable SysTick*/
	CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HCLK, CLK_GetHCLKFreq() / 1000);
#endif

}

static const char fresh_main_py[] =
"# main.py -- put your code here!\r\n"
;

static const char fresh_boot_py[] =
"# boot.py -- run on boot-up\r\n"
"# can run arbitrary Python, but best to keep it minimal\r\n"
"\r\n"
"import umachine\r\n"
"import pyb\r\n"
"#pyb.main('main.py') # main script to run after this one\r\n"
;

#if MICROPY_HW_HAS_FLASH

static fs_user_mount_t s_sflash_vfs_fat;

STATIC bool init_flash_fs(void){
    // init the vfs object
    fs_user_mount_t *vfs_fat = &s_sflash_vfs_fat;
    vfs_fat->flags = 0;
    pyb_flash_init_vfs(vfs_fat);

    // try to mount the flash
    FRESULT res = f_mount(&vfs_fat->fatfs);
    if (res == FR_NO_FILESYSTEM) {
        // no filesystem, or asked to reset it, so create a fresh one
        uint8_t working_buf[_MAX_SS];
		vfs_fat->fatfs.part = 0;
        res = f_mkfs(&vfs_fat->fatfs, FM_FAT, 0, working_buf, sizeof(working_buf));
        if (res == FR_OK) {
            // success creating fresh LFS
        } else {
            printf("PYB: can't create flash filesystem %d \n", res);
            return false;
        }
        
        // set label
        f_setlabel(&vfs_fat->fatfs, MICROPY_HW_FLASH_FS_LABEL);

        // create empty main.py
        FIL fp;
        f_open(&vfs_fat->fatfs, &fp, "/main.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_main_py, sizeof(fresh_main_py) - 1 /* don't count null terminator */, &n);
        f_close(&fp);

    } else if (res == FR_OK) {
        // mount sucessful
    } else {
    fail:
        printf("PYB: can't mount flash\n");
        return false;
    }

    // mount the flash device (there should be no other devices mounted at this point)
    // we allocate this structure on the heap because vfs->next is a root pointer
    mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
    if (vfs == NULL) {
        goto fail;
    }
    vfs->str = "/flash";
    vfs->len = 6;
    vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
    vfs->next = NULL;
    MP_STATE_VM(vfs_mount_table) = vfs;

    // The current directory is used as the boot up directory.
    // It is set to the internal flash filesystem by default.
    MP_STATE_PORT(vfs_cur) = vfs;

    // Make sure we have a /flash/boot.py.  Create it if needed.
    FILINFO fno;
    res = f_stat(&vfs_fat->fatfs, "/boot.py", &fno);
    if (res != FR_OK) {
        // doesn't exist, create fresh file
        FIL fp;
        f_open(&vfs_fat->fatfs, &fp, "/boot.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_boot_py, sizeof(fresh_boot_py) - 1 /* don't count null terminator */, &n);
        f_close(&fp);
    }

    return true;
}
#endif


#if MICROPY_HW_HAS_SPIFLASH

static fs_user_mount_t s_sspiflash_vfs_fat;

STATIC bool init_spiflash_fs(void){
    // init the vfs object
    fs_user_mount_t *vfs_fat = &s_sspiflash_vfs_fat;
    vfs_fat->flags = 0;
    pyb_spiflash_init_vfs(vfs_fat);

    // try to mount the flash
    FRESULT res = f_mount(&vfs_fat->fatfs);
    if (res == FR_NO_FILESYSTEM) {
        // no filesystem, or asked to reset it, so create a fresh one
        uint8_t working_buf[_MAX_SS];
		vfs_fat->fatfs.part = 0;
        res = f_mkfs(&vfs_fat->fatfs, FM_FAT, 0, working_buf, sizeof(working_buf));
        if (res == FR_OK) {
            // success creating fresh LFS
        } else {
            printf("PYB: can't create flash filesystem %d \n", res);
            return false;
        }
        
        // set label
        f_setlabel(&vfs_fat->fatfs, MICROPY_HW_FLASH_FS_LABEL);

        // create empty main.py
        FIL fp;
        f_open(&vfs_fat->fatfs, &fp, "/main.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_main_py, sizeof(fresh_main_py) - 1 /* don't count null terminator */, &n);
        f_close(&fp);

    } else if (res == FR_OK) {
        // mount sucessful
    } else {
    fail:
        printf("PYB: can't mount flash\n");
        return false;
    }

    // mount the flash device (there should be no other devices mounted at this point)
    // we allocate this structure on the heap because vfs->next is a root pointer
    mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
    if (vfs == NULL) {
        goto fail;
    }
    vfs->str = "/spiflash";
    vfs->len = 9;
    vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
    vfs->next = NULL;
    MP_STATE_VM(vfs_mount_table) = vfs;

    // The current directory is used as the boot up directory.
    // It is set to the internal flash filesystem by default.
    MP_STATE_PORT(vfs_cur) = vfs;

    // Make sure we have a /flash/boot.py.  Create it if needed.
    FILINFO fno;
    res = f_stat(&vfs_fat->fatfs, "/boot.py", &fno);
    if (res != FR_OK) {
        // doesn't exist, create fresh file
        FIL fp;
        res = f_open(&vfs_fat->fatfs, &fp, "/boot.py", FA_WRITE | FA_CREATE_ALWAYS);
		if(res != FR_OK)
			printf("Unable create boot.py file \n");
        UINT n;
        res = f_write(&fp, fresh_boot_py, sizeof(fresh_boot_py) - 1 /* don't count null terminator */, &n);
		if(res != FR_OK)
			printf("Unable write boot.py file \n");
        f_close(&fp);
    }

    return true;
}
#endif


#if MICROPY_HW_HAS_SDCARD
STATIC bool init_sdcard_fs(void) {
    bool first_part = true;
//    for (int part_num = 1; part_num <= 4; ++part_num) {		//support 4 part 
    for (int part_num = 1; part_num <= 1; ++part_num) {		//only one part 
        // create vfs object
        fs_user_mount_t *vfs_fat = m_new_obj_maybe(fs_user_mount_t);
        mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
        if (vfs == NULL || vfs_fat == NULL) {
            break;
        }
        vfs_fat->flags = FSUSER_FREE_OBJ;
        sdcard_init_vfs(vfs_fat, part_num);

        // try to mount the partition
        FRESULT res = f_mount(&vfs_fat->fatfs);

        if (res != FR_OK) {
            // couldn't mount
            m_del_obj(fs_user_mount_t, vfs_fat);
            m_del_obj(mp_vfs_mount_t, vfs);
        } else {
            // mounted via FatFs, now mount the SD partition in the VFS
            if (first_part) {
                // the first available partition is traditionally called "sd" for simplicity
                vfs->str = "/sd";
                vfs->len = 3;
            } else {
                // subsequent partitions are numbered by their index in the partition table
                if (part_num == 2) {
                    vfs->str = "/sd2";
                } else if (part_num == 2) {
                    vfs->str = "/sd3";
                } else {
                    vfs->str = "/sd4";
                }
                vfs->len = 4;
            }
            vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
            vfs->next = NULL;
            for (mp_vfs_mount_t **m = &MP_STATE_VM(vfs_mount_table);; m = &(*m)->next) {
                if (*m == NULL) {
                    *m = vfs;
                    break;
                }
            }

            first_part = false;
        }
    }

    if (first_part) {
        return false;
    } else {
        return true;
    }
}
#endif


// MicroPython runs as a task under FreeRTOS
#define MP_TASK_PRIORITY        (configMAX_PRIORITIES - 1)
#define MP_TASK_STACK_SIZE      (4 * 1024)
#define MP_TASK_STACK_LEN       (MP_TASK_STACK_SIZE / sizeof(StackType_t))

#ifndef MP_TASK_HEAP_SIZE
#define MP_TASK_HEAP_SIZE	(28 * 1024)
#endif

#if MICROPY_PY_THREAD
STATIC StaticTask_t mp_task_tcb;
STATIC StackType_t mp_task_stack[MP_TASK_STACK_LEN] __attribute__((aligned (8)));

STATIC StaticTask_t mp_usbtask_tcb;
STATIC StackType_t mp_usbtask_stack[MP_TASK_STACK_LEN] __attribute__((aligned (8)));
#endif

STATIC char mp_task_heap[MP_TASK_HEAP_SIZE]__attribute__((aligned (8)));
STATIC volatile bool mp_USBRun;


static void ExecuteUsbMSC(void){
	S_USBDEV_STATE *psUSBDev_msc_state = NULL;
	psUSBDev_msc_state = USBDEV_Init(USBD_VID, USBD_MSC_VCP_PID, eUSBDEV_MODE_MSC_VCP);
	if(psUSBDev_msc_state == NULL){
		mp_raise_ValueError("bad USB mode");
	}

	/* Start transaction */
	if (USBD_IS_ATTACHED())
	{
		USBDEV_Start(psUSBDev_msc_state);
		printf("Start USB device MSC and VCP \n");

		while(USBD_IS_ATTACHED())
		{
			MSCTrans_ProcessCmd();
		}
	}
	else{
		USBDEV_Deinit(psUSBDev_msc_state);
	}

	mp_USBRun = false;
	printf("ExecuteUsbMSC exit \n");
	vTaskDelete(NULL);
}

void mp_usbtask(void *pvParameter) {
		ExecuteUsbMSC();
}

void mp_task(void *pvParameter) {
	mp_uint_t regs[10];
	volatile uint32_t sp = (uint32_t)gc_helper_get_regs_and_sp(regs);
	bool mounted_sdcard = false;
	bool mounted_flash = false;
	bool mounted_spiflash = false;
	bool OnlyMSCVCPMode = false;

	#if MICROPY_PY_THREAD
	mp_thread_init(&mp_task_stack[0], MP_TASK_STACK_LEN);
	#endif
	
	//Detect sw2 button press or not
	if(mp_hal_pin_read(MICROPY_HW_USRSW_SW2_PIN) == 0){
		//execute USB mass storage mode and export internal flash
		OnlyMSCVCPMode = true;
		printf("Run only MSC and VCP mode \n");
	}

soft_reset:
	// initialise the stack pointer for the main thread
	mp_stack_set_top((void *)sp);
	mp_stack_set_limit(MP_TASK_STACK_SIZE - 1024);
	gc_init(mp_task_heap, mp_task_heap + MP_TASK_HEAP_SIZE);
	mp_init();
	mp_obj_list_init(mp_sys_path, 0);
	mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)

	mp_obj_list_init(mp_sys_argv, 0);
	readline_init0();
	pin_init0();
	extint_init0();


   // Initialise the local flash filesystem.
#if MICROPY_HW_HAS_FLASH

    // Create it if needed, mount in on /flash, and set it as current dir.
	mounted_flash = init_flash_fs();
#endif

#if MICROPY_HW_HAS_SPIFLASH

    // Create it if needed, mount in on /flash, and set it as current dir.
	mounted_spiflash = init_spiflash_fs();
#endif

#if MICROPY_HW_HAS_SDCARD
	// if an SD card is present then mount it on /sd/
	if (sdcard_is_present()) {
		// if there is a file in the flash called "SKIPSD", then we don't mount the SD card
		mounted_sdcard = init_sdcard_fs();
	}
#endif


#if MICROPY_PY_THREAD

	//Open MSC and VCP

	TaskHandle_t mpUSBTaskHandle;

	mp_USBRun = true;
    mpUSBTaskHandle = xTaskCreateStatic(mp_usbtask, "USB_MSCVCP",
        MP_TASK_STACK_LEN, NULL, MP_TASK_PRIORITY, mp_usbtask_stack, &mp_usbtask_tcb);

    if (mpUSBTaskHandle == NULL){
		__fatal_error("FreeRTOS create USB task!");
	}

	S_USBDEV_STATE *psUSBDevState;
	while(1){
		psUSBDevState = USBDEV_UpdateState();

		if(mp_USBRun == false)
			break;

		if(OnlyMSCVCPMode == false){
			if(psUSBDevState->bConnected)
				break;
		}
		vTaskDelay(1);
	}

	vTaskDelay(500);
	
	printf("Start execute boot.py and main.py \n");
#endif

	// set sys.path based on mounted filesystems (/sd is first so it can override /flash)	
	if (mounted_spiflash) {
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_spiflash));
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_spiflash_slash_lib));
    }

	if (mounted_flash) {
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash));
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash_slash_lib));
    }

	if (mounted_sdcard) {
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_sd));
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_sd_slash_lib));
	}

#if MICROPY_PY_NETWORK
    mod_network_init();
#endif

	// initialise peripherals
	//    machine_pins_init();

	// run boot-up scripts
//	pyexec_frozen_module("frozentest.py");
	pyexec_file("boot.py");
	if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
		pyexec_file("main.py");
	}

	printf("Run REPL \n");
	for (;;) {
		if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
			if (pyexec_raw_repl() != 0) {
				break;
			}
		} else {
			if (pyexec_friendly_repl() != 0) {
				break;
			}
		}
	}

	#if MICROPY_PY_THREAD
	mp_thread_deinit();
	#endif

	mp_hal_stdout_tx_str("PYB: soft reboot\r\n");

	// deinitialise peripherals
	//    machine_pins_deinit();

	mp_deinit();
	fflush(stdout);
	goto soft_reset;
}


int main (void)
{
	/* Unlock protected registers */
	SYS_UnlockReg();

	/* Init System, IP clock and multi-function I/O */
	SYS_Init();

	/* Init UART to 115200-8n1 for print message */
	UART_Open(UART0, 115200);

	printf("+-------------------------------------------------------+\n");
	printf("|          Micropython Code for Nuvoton M48x            |\n");
	printf("|                     Debug Console                     |\n");	
	printf("+-------------------------------------------------------+\n");

#if MICROPY_HW_HAS_FLASH
	flash_init();
#endif

#if MICROPY_HW_HAS_SPIFLASH
	spiflash_init();
#endif

#if MICROPY_HW_HAS_SDCARD
	sdcard_init();
#endif

#if MICROPY_HW_ENABLE_RNG
	PRNG_Open(CRPT, PRNG_KEY_SIZE_64, 1, 0xAA);     // start PRNG with seed 0x55
#endif

#if ENABLE_SPIM

#define SPIM_CIPHER_ON              0
#define USE_4_BYTES_MODE            0            /* W25Q20 does not support 4-bytes address mode. */

    uint8_t     idBuf[3];

    SPIM_SET_CLOCK_DIVIDER(1);        /* Set SPIM clock as HCLK divided by 2 */

    SPIM_SET_RXCLKDLY_RDDLYSEL(0);    /* Insert 0 delay cycle. Adjust the sampling clock of received data to latch the correct data. */
    SPIM_SET_RXCLKDLY_RDEDGE();       /* Use SPI input clock rising edge to sample received data. */

    SPIM_SET_DCNUM(8);                /* 8 is the default value. */

    if (SPIM_InitFlash(1) != 0)        /* Initialized SPI flash */
    {
        __fatal_error("SPIM flash initialize failed!\n");
    }

    SPIM_ReadJedecId(idBuf, sizeof (idBuf), 1);
    printf("SPIM get JEDEC ID=0x%02X, 0x%02X, 0x%02X\n", idBuf[0], idBuf[1], idBuf[2]);

    SPIM_DISABLE_CCM();

    //SPIM_DISABLE_CACHE();
    SPIM_ENABLE_CACHE();

    if (SPIM_CIPHER_ON)
        SPIM_ENABLE_CIPHER();
    else
        SPIM_DISABLE_CIPHER();

    SPIM_Enable_4Bytes_Mode(USE_4_BYTES_MODE, 1);

    SPIM->CTL1 |= SPIM_CTL1_CDINVAL_Msk;        // invalid cache

    SPIM_EnterDirectMapMode(USE_4_BYTES_MODE, CMD_DMA_FAST_READ, 0);

#endif

#if MICROPY_PY_THREAD
	TaskHandle_t mpTaskHandle;

    mpTaskHandle = xTaskCreateStatic(mp_task, "MicroPy",
        MP_TASK_STACK_LEN, NULL, MP_TASK_PRIORITY, mp_task_stack, &mp_task_tcb);

    if (mpTaskHandle == NULL){
		__fatal_error("FreeRTOS create mp task!");
	}

    /* Start the scheduler. */
	vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for( ;; );

#else
	mp_task(NULL);
	return 0;

#endif
}

#if MICROPY_PY_THREAD

// We need this when configSUPPORT_STATIC_ALLOCATION is enabled
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize ) {

// This is the static memory (TCB and stack) for the idle task
static StaticTask_t xIdleTaskTCB; // __attribute__ ((section (".rtos_heap")));
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE]; // __attribute__ ((section (".rtos_heap"))) __attribute__((aligned (8)));


    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint16_t *pusTimerTaskStackSize )
{
/* The buffers used by the Timer/Daemon task must be static so they are
persistent, and so exist after this function returns. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	/* configUSE_STATIC_ALLOCATION is set to 1, so the application has the
	opportunity to supply the buffers that will be used by the Timer/RTOS daemon
	task as its	stack and to hold its TCB.  If these are set to NULL then the
	buffers will be allocated dynamically, just as if xTaskCreate() had been
	called. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;
	*pusTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH; /* In words.  NOT in bytes! */
}

#endif
