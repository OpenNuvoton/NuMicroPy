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

#if MICROPY_PY_THREAD
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "wblib.h"
#include "N9H26_USBD.h"

#include "gchelper.h"
#include "misc/mperror.h"
#include "mods/pybspiflash.h"
#include "mods/pybsdcard.h"
#include "hal/pin_int.h"
#include "hal/N9H26_USBDev.h"

#include "mods/modnetwork.h"

// MicroPython runs as a task under FreeRTOS
#define MP_TASK_PRIORITY        (configMAX_PRIORITIES - 1)
#define MP_TASK_STACK_SIZE      (4 * 1024)
#define MP_TASK_STACK_LEN       (MP_TASK_STACK_SIZE / sizeof(StackType_t))

#ifndef MP_TASK_HEAP_SIZE
#define MP_TASK_HEAP_SIZE	(512 * 1024)
#endif

//#define _CTRL_D_SOFT_RESET_

#if MICROPY_PY_THREAD
STATIC StaticTask_t mp_task_tcb;
STATIC StackType_t mp_task_stack[MP_TASK_STACK_LEN] __attribute__((aligned (8)));

STATIC StaticTask_t mp_usbtask_tcb;
STATIC StackType_t mp_usbtask_stack[MP_TASK_STACK_LEN] __attribute__((aligned (8)));
#endif

STATIC char mp_task_heap[MP_TASK_HEAP_SIZE]__attribute__((aligned (8)));
STATIC volatile bool mp_USBRun;

#if MICROPY_KBD_EXCEPTION

static int32_t VCPRecvSignalCB(
	uint8_t *pu8Buf, 
	uint32_t u32Size
)
{
	if(mp_interrupt_char != -1){
		if(pu8Buf[0] == mp_interrupt_char){
			mp_keyboard_interrupt();
		}
		return 0;
	}
	return 1;
}

#endif

static void ExecuteUsbMSC(void){
	S_USBDEV_STATE *psUSBDev_msc_state = NULL;
	uint32_t u32CheckConnTimeOut = 0;
	psUSBDev_msc_state = USBDEV_Init(USBD_VID, USBD_MSC_VCP_PID, eUSBDEV_MODE_MSC_VCP);
	if(psUSBDev_msc_state == NULL){
		mp_raise_ValueError("bad USB mode");
	}

#if 1 //For USB download code useage
	vTaskDelay(1000);
#endif

	/* Start transaction */
	if (udcIsAttached())
	{
		USBDEV_Start(psUSBDev_msc_state);
		printf("Start USB device MSC and VCP \n");

		u32CheckConnTimeOut = mp_hal_ticks_ms() + 1000;
		
		//Wait usb data bus ready (1 sec) 
		while(mp_hal_ticks_ms() < u32CheckConnTimeOut){
			if(USBDEV_DataBusConnect())
				break;
		}

		if(USBDEV_DataBusConnect()){

#if MICROPY_KBD_EXCEPTION
			USBDEV_VCPRegisterSingal(VCPRecvSignalCB);
#endif
			while(udcIsAttached() && (mp_USBRun == true))
			{
				MSCTrans_Process();
			}
		}
	}
	
	USBDEV_Deinit(psUSBDev_msc_state);

	mp_USBRun = false;
	printf("ExecuteUsbMSC exit \n");
	vTaskDelete(NULL);
}

void mp_usbtask(void *pvParameter) {
		ExecuteUsbMSC();
}


static void SetupHardware(void)
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	WB_UART_T sUart;

	// initial MMU but disable system cache feature
	//sysEnableCache(CACHE_DISABLE);
	// Cache on
	sysEnableCache(CACHE_WRITE_BACK);

	sUart.uart_no = WB_UART_1;
	sUart.uiFreq = EXTERNAL_CRYSTAL_CLOCK;	//use APB clock
	sUart.uiBaudrate = 115200;
	sUart.uiDataBits = WB_DATA_BITS_8;
	sUart.uiStopBits = WB_STOP_BITS_1;
	sUart.uiParity = WB_PARITY_NONE;
	sUart.uiRxTriggerLevel = LEVEL_1_BYTE;
	sysInitializeUART(&sUart);
	sysSetLocalInterrupt(ENABLE_IRQ);

#if !MICROPY_PY_THREAD
	UINT32 u32ExtFreq;
	
	u32ExtFreq = sysGetExternalClock();
	sysSetTimerReferenceClock(TIMER1, u32ExtFreq);
	sysStartTimer(TIMER1, 1000, PERIODIC_MODE);
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


void mp_task(void *pvParameter) {

	bool mounted_spiflash = false;
	bool mounted_sdcard = false;
	bool OnlyMSCVCPMode = false;

	#if MICROPY_PY_THREAD
	TaskHandle_t mpUSBTaskHandle = NULL;

	mp_thread_init(&mp_task_stack[0], MP_TASK_STACK_LEN);
	#endif

#if defined(_CTRL_D_SOFT_RESET_)
soft_reset:
#endif

   // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    // Note: stack control relies on main thread being initialised above
#if 0
    mp_stack_set_top(&_stack_end);
    mp_stack_set_limit((char*)&_stack_end - (char*)&_heap_end - 1024);
#else
	mp_stack_set_top(&mp_task_stack[0]);
	mp_stack_set_limit(MP_TASK_STACK_SIZE - 1024);
#endif

#if MICROPY_ENABLE_GC
	gc_init(mp_task_heap, mp_task_heap + MP_TASK_HEAP_SIZE);
#endif
	mp_init();
	mp_obj_list_init(mp_sys_path, 0);
	mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)

	mp_obj_list_init(mp_sys_argv, 0);
	readline_init0();
	pin_init0();
	extint_init0();

#if MICROPY_HW_HAS_SPIFLASH

    // Create it if needed, mount in on /flash, and set it as current dir.
	mounted_spiflash = init_spiflash_fs();
#endif

#if MICROPY_HW_HAS_SDCARD
	// if an SD card is present then mount it on /sd/
	if (sdcard_is_present()) {
		mounted_sdcard = init_sdcard_fs();
	}
#endif

#if MICROPY_PY_THREAD

	//Open MSC and VCP

	if(mpUSBTaskHandle == NULL){
		mp_USBRun = true;
		mpUSBTaskHandle = xTaskCreateStatic(mp_usbtask, "USB_MSCVCP",
			MP_TASK_STACK_LEN, NULL, MP_TASK_PRIORITY, mp_usbtask_stack, &mp_usbtask_tcb);

		if (mpUSBTaskHandle == NULL){
			__fatal_error("FreeRTOS create USB task!");
		}
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
	
#endif

	printf("Start execute boot.py and main.py \n");

	// set sys.path based on mounted filesystems (/sd is first so it can override /flash)	
	if (mounted_spiflash) {
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_spiflash));
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_spiflash_slash_lib));
    }

	if (mounted_sdcard) {
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_sd));
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_sd_slash_lib));
	}

#if MICROPY_PY_NETWORK
    mod_network_init();
#endif

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

#if defined(_CTRL_D_SOFT_RESET_)
	mp_hal_stdout_tx_str("PYB: soft reset \r\n");
#else
	mp_hal_stdout_tx_str("PYB: chip reset \r\n");
#endif

	// deinitialise peripherals
	//    machine_pins_deinit();

	mp_deinit();
	fflush(stdout);

#if defined(_CTRL_D_SOFT_RESET_)
	goto soft_reset;
#else
	//CPU reset
	outp32(REG_MISCR, inp32(REG_MISCR) | CPURST);
#endif

}

int main (void)
{
	SetupHardware();

	sysprintf("+-------------------------------------------------------+\n");
	sysprintf("|          Micropython Code for Nuvoton N9H26           |\n");
	sysprintf("|                     Debug Console                     |\n");	
	sysprintf("+-------------------------------------------------------+\n");

#if MICROPY_HW_HAS_SPIFLASH
	spiflash_init();
#endif

#if MICROPY_HW_HAS_SDCARD
	sdcard_init();
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

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pusTimerTaskStackSize )
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
