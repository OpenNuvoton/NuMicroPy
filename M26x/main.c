#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#include "hal/pin_int.h"
#include "hal/StorIF.h"
#include "mods/pybsdcard.h"
#include "hal/M26x_USBD.h"
#include "hal/MSC_Trans.h"


#define CLK_PLLCTL_144MHz_HXT   (CLK_PLLCTL_PLLSRC_HXT  | CLK_PLLCTL_NR(2) | CLK_PLLCTL_NF( 12) | CLK_PLLCTL_NO_1)

extern mp_uint_t gc_helper_get_regs_and_sp(mp_uint_t *regs);


void SYS_Init(void)
{
    /* Set PF multi-function pins for XT1_OUT(PF.2) and XT1_IN(PF.3) */
    SYS->GPF_MFPL = (SYS->GPF_MFPL & (~SYS_GPF_MFPL_PF2MFP_Msk)) | SYS_GPF_MFPL_PF2MFP_XT1_OUT;
    SYS->GPF_MFPL = (SYS->GPF_MFPL & (~SYS_GPF_MFPL_PF3MFP_Msk)) | SYS_GPF_MFPL_PF3MFP_XT1_IN;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
#if 1
    /* Enable Internal RC 48MHz clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);

    /* Waiting for Internal RC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRC48STB_Msk);

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC48, CLK_CLKDIV0_HCLK(1));

#endif

    /* Enable external XTAL 12MHz clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for 12MHz clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Set PLL frequency */
    CLK->PLLCTL = CLK_PLLCTL_128MHz_HXT;

    /* Waiting for clock ready */
    CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);

    /* Switch HCLK clock source to PLL */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(2));

    /* Select IP clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Enable IP clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & (~(UART0_RXD_PB12_Msk | UART0_TXD_PB13_Msk))) | UART0_RXD_PB12 | UART0_TXD_PB13;

#if !MICROPY_PY_THREAD
	/*Enable SysTick*/
	CLK_EnableSysTick(CLK_CLKSEL0_STCLKSEL_HCLK, CLK_GetHCLKFreq() / 1000);
#endif

}

static fs_user_mount_t s_sflash_vfs_fat;

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

            {
                if (first_part) {
                    // use SD card as current directory
                    MP_STATE_PORT(vfs_cur) = vfs;
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

static void ExecuteUsbMSC(void){
	S_USBDEV_STATE *psUSBDev_msc_state = NULL;
	psUSBDev_msc_state = USBDEV_Init(USBD_VID, USBD_MSC_PID, eUSBDEV_MODE_MSC);
	if(psUSBDev_msc_state == NULL){
		mp_raise_ValueError("bad USB mode");
	}

	/* Start transaction */
	if (USBD_IS_ATTACHED())
	{
		USBDEV_Start(psUSBDev_msc_state);
		printf("Start USB device MSC class \n");
		while(1)
		{
			MSCTrans_ProcessCmd();
		}
	}
	else{
		USBDEV_Deinit(psUSBDev_msc_state);
	}
}

// MicroPython runs as a task under FreeRTOS
#define MP_TASK_PRIORITY        (configMAX_PRIORITIES - 1)
#define MP_TASK_STACK_SIZE      (4 * 1024)
#define MP_TASK_STACK_LEN       (MP_TASK_STACK_SIZE / sizeof(StackType_t))

#define MP_TASK_HEAP_SIZE	(16 * 1024)

#if MICROPY_PY_THREAD
STATIC StaticTask_t mp_task_tcb;
STATIC StackType_t mp_task_stack[MP_TASK_STACK_LEN] __attribute__((aligned (8)));
#endif

STATIC char mp_task_heap[MP_TASK_HEAP_SIZE]__attribute__((aligned (8)));

void mp_task(void *pvParameter) {
	mp_uint_t regs[10];
	volatile uint32_t sp = (uint32_t)gc_helper_get_regs_and_sp(regs);
	bool mounted_sdcard = false;

	#if MICROPY_PY_THREAD
	mp_thread_init(&mp_task_stack[0], MP_TASK_STACK_LEN);
	#endif

	//Detect USB cable plugin or not
	//execute USB mass storage mode and export internal flash
	ExecuteUsbMSC();

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
    // Create it if needed, mount in on /flash, and set it as current dir.
	bool mounted_flash = init_flash_fs();

#if MICROPY_HW_HAS_SDCARD
	// if an SD card is present then mount it on /sd/
	if (sdcard_is_present()) {
		// if there is a file in the flash called "SKIPSD", then we don't mount the SD card
		mounted_sdcard = init_sdcard_fs();
	}
#endif

	// set sys.path based on mounted filesystems (/sd is first so it can override /flash)
	if (mounted_sdcard) {
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_sd));
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_sd_slash_lib));
	}
	
	if (mounted_flash) {
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash));
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash_slash_lib));
    }

	// initialise peripherals
	//    machine_pins_init();

	// run boot-up scripts
//	pyexec_frozen_module("frozentest.py");
	pyexec_file("boot.py");
	if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
		pyexec_file("main.py");
	}

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

	/*
	This sample code sets I2C bus clock to 100kHz. Then, Master accesses Slave with Byte Write
	and Byte Read operations, and check if the read data is equal to the programmed data.
	*/

	printf("+-------------------------------------------------------+\n");
	printf("|          Micropython Code for Nuvoton M26x            |\n");
	printf("+-------------------------------------------------------+\n");

	flash_init();

#if MICROPY_HW_HAS_SDCARD
        sdcard_init();
#endif

#if MICROPY_HW_ENABLE_RNG
	PRNG_Open(CRPT, PRNG_KEY_SIZE_64, 1, 0xAA);     // start PRNG with seed 0xAA
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

