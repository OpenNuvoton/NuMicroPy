#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if MICROPY_PY_THREAD
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "py/stackctrl.h"
#include "py/nlr.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#if MICROPY_KBD_EXCEPTION
#include "shared/runtime/interrupt_char.h"
#endif

#include "NuMicro.h"

#include "mpconfigboard_common.h"
#include "mpconfigboard.h"
#include "mods/pybflash.h"
#include "mods/pybsdcard.h"
#include "mods/classPin.h"
#include "hal/pin_int.h"
#include "hal/M55M1_USBD.h"
#include "hal/MSC_VCPTrans.h"
#include "hal/StorIF.h"
#include "misc/mperror.h"

//#define _CTRL_D_CHIP_RESET_

extern mp_uint_t gc_helper_get_regs_and_sp(mp_uint_t *regs);

/* ---------------------------------------------------------------------------------------------------------------------*/
/* hardware resource init                                                                                               */
/* ---------------------------------------------------------------------------------------------------------------------*/

void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set X32_OUT(PF.4) and X32_IN(PF.5)*/	//LXT for RTC and WDT
    SET_X32_IN_PF5();
    SET_X32_OUT_PF4();

    /* Enable HIRC clock */
    CLK_EnableXtalRC(CLK_SRCCTL_HIRCEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Enable HIRC48M clock */
    CLK_EnableXtalRC(CLK_SRCCTL_HIRC48MEN_Msk);

    /* Waiting for HIRC48M clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRC48MSTB_Msk);

    /* Enable External LXT clock */
    CLK_EnableXtalRC(CLK_SRCCTL_LXTEN_Msk);

    /* Waiting for LXT clock ready */
    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

    /* Switch SCLK clock source to APLL0 and Enable APLL0 200MHz clock */
    CLK_SetBusClock(CLK_SCLKSEL_SCLKSEL_APLL0, CLK_APLLCTL_APLLSRC_HIRC, SYSTEM_CORE_CLOCK);

    /* Enable APLL1 200MHz clock */
    CLK_EnableAPLL(CLK_APLLCTL_APLLSRC_HIRC, SYSTEM_CORE_CLOCK, CLK_APLL1_SELECT);

    /* Set HCLK2 divide 2 */
    CLK_SET_HCLK2DIV(2);

    /* Set PCLKx divide 2 */
    CLK_SET_PCLK0DIV(2);
    CLK_SET_PCLK1DIV(2);
    CLK_SET_PCLK2DIV(2);
    CLK_SET_PCLK3DIV(2);
    CLK_SET_PCLK4DIV(2);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Enable all GPIO clock */
    CLK_EnableModuleClock(GPIOA_MODULE);
    CLK_EnableModuleClock(GPIOB_MODULE);
    CLK_EnableModuleClock(GPIOC_MODULE);
    CLK_EnableModuleClock(GPIOD_MODULE);
    CLK_EnableModuleClock(GPIOE_MODULE);
    CLK_EnableModuleClock(GPIOF_MODULE);
    CLK_EnableModuleClock(GPIOG_MODULE);
    CLK_EnableModuleClock(GPIOH_MODULE);
    CLK_EnableModuleClock(GPIOI_MODULE);
    CLK_EnableModuleClock(GPIOJ_MODULE);

    /* Enable all PDMA clock */
    CLK_EnableModuleClock(PDMA0_MODULE);
    CLK_EnableModuleClock(PDMA1_MODULE);
    CLK_EnableModuleClock(LPPDMA0_MODULE);


#if MICROPY_HW_ENABLE_RNG
    /* Enable CRYPTO module clock */
    CLK_EnableModuleClock(CRPT_MODULE);
#endif

    /* Enable FMC0 module clock to keep FMC clock when CPU idle but NPU running*/
    CLK_EnableModuleClock(FMC0_MODULE);

    /* Select debug UART port clock source */
    SetDebugUartCLK();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set multi-function pins for UART RXD and TXD */

#if MICROPY_HW_ENABLE_HW_DAC
    SET_UART0_RXD_PA0();	//UNO_D11 for NuMaker_M55M1
    SET_UART0_TXD_PA1();	//UNO_D12 for NuMaker_M55M1
#else
    SetDebugUartMFP();		//default PB12/PB13
#endif

#if !MICROPY_PY_THREAD
    /*Enable SysTick*/
    SysTick_Config(SystemCoreClock / 1000);
#endif
}


static const char fresh_main_py[] =
    "# main.py -- put your code here!\r\n"
    ;

static const char fresh_boot_py[] =
    "# boot.py -- run on boot-up\r\n"
    "# can run arbitrary Python, but best to keep it minimal\r\n"
    "\r\n"
    "import machine\r\n"
    "import pyb\r\n"
    "#pyb.main('main.py') # main script to run after this one\r\n"
    ;

#if MICROPY_HW_HAS_FLASH

static fs_user_mount_t s_sflash_vfs_fat;

static bool init_flash_fs(void)
{
    // init the vfs object
    fs_user_mount_t *vfs_fat = &s_sflash_vfs_fat;
    vfs_fat->blockdev.flags = 0;
    pyb_flash_init_vfs(vfs_fat);

    // try to mount the flash
    FRESULT res = f_mount(&vfs_fat->fatfs);
    if (res == FR_NO_FILESYSTEM) {
        // no filesystem, or asked to reset it, so create a fresh one
        uint8_t working_buf[FF_MAX_SS];
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

#if MICROPY_HW_HAS_SDCARD
static bool init_sdcard_fs(void)
{
    bool first_part = true;
    //    for (int part_num = 1; part_num <= 4; ++part_num) {		//support 4 part
    for (int part_num = 1; part_num <= 1; ++part_num) {		//only one part
        // create vfs object
        fs_user_mount_t *vfs_fat = m_new_obj_maybe(fs_user_mount_t);
        mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
        if (vfs == NULL || vfs_fat == NULL) {
            break;
        }
        vfs_fat->blockdev.flags = MP_BLOCKDEV_FLAG_FREE_OBJ;
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


/* ---------------------------------------------------------------------------------------------------------------------*/
/* main and USB task                                                                                                            */
/* ---------------------------------------------------------------------------------------------------------------------*/

#ifndef MP_TASK_HEAP_SIZE
#define MP_TASK_HEAP_SIZE	(100 * 1024)
#endif

#if MICROPY_PY_THREAD

// MicroPython runs as a task under FreeRTOS
#define MP_TASK_PRIORITY        (configMAX_PRIORITIES - 1)
#define MP_TASK_STACK_SIZE      (4 * 1024)
#define MP_TASK_STACK_LEN       (MP_TASK_STACK_SIZE / sizeof(StackType_t))

static StaticTask_t mp_task_tcb;
static StackType_t mp_task_stack[MP_TASK_STACK_LEN] __attribute__((aligned (32)));

static StaticTask_t mp_usbtask_tcb;
static StackType_t mp_usbtask_stack[MP_TASK_STACK_LEN] __attribute__((aligned (32)));

#endif

static char mp_task_heap[MP_TASK_HEAP_SIZE]__attribute__((aligned (32)));
static volatile bool mp_USBRun;


#if MICROPY_KBD_EXCEPTION

static int32_t VCPRecvSignalCB(
    uint8_t *pu8Buf,
    uint32_t u32Size
)
{
    if(mp_interrupt_char != -1) {
        if(pu8Buf[0] == mp_interrupt_char) {
            mp_sched_keyboard_interrupt();
        }
        return 0;
    }
    return 1;
}

#endif

static void ExecuteUsbMSC(bool WriteProtect)
{
    S_USBDEV_STATE *psUSBDev_msc_state = NULL;
    uint32_t u32CheckConnTimeOut = 0;

    if(WriteProtect)
        psUSBDev_msc_state = USBDEV_Init(USBD_VID, USBD_MSC_VCP_PID, eUSBDEV_MODE_MSC_WP_VCP);
    else
        psUSBDev_msc_state = USBDEV_Init(USBD_VID, USBD_MSC_VCP_PID, eUSBDEV_MODE_MSC_VCP);

    if(psUSBDev_msc_state == NULL) {
        mp_raise_ValueError("bad USB mode");
    }

    USBDEV_Start(psUSBDev_msc_state);
    printf("Start USB device MSC and VCP \n");

#if MICROPY_KBD_EXCEPTION
    USBDEV_VCPRegisterSingal(VCPRecvSignalCB);
#endif

    while(mp_USBRun == true) {
        if (USBD_IS_ATTACHED()) {
            u32CheckConnTimeOut = mp_hal_ticks_ms() + 1000;

            //Wait usb data bus ready (1 sec)
            while(mp_hal_ticks_ms() < u32CheckConnTimeOut) {
                if(USBDEV_DataBusConnect())
                    break;
            }

            if(USBDEV_DataBusConnect()) {
                MSCTrans_ProcessCmd();
            }
        }
        taskYIELD();
    }

    USBDEV_Deinit(psUSBDev_msc_state);

    printf("ExecuteUsbMSC exit \n");
    vTaskDelete(NULL);
}

void mp_usbtask(void *pvParameter)
{
    bool bMSCWriteProtect = (bool)pvParameter;

    ExecuteUsbMSC(bMSCWriteProtect);
}

void mp_task(void *pvParameter)
{

    printf("start mptask \n");

    mp_uint_t regs[10];
    volatile uint32_t sp = (uint32_t)gc_helper_get_regs_and_sp(regs);
    bool mounted_flash = false;
    bool mounted_sdcard = false;
    bool mounted_spiflash = false;
    bool OnlyMSCVCPMode = false;

#if MICROPY_PY_THREAD
    TaskHandle_t mpUSBTaskHandle = NULL;

    mp_thread_init(&mp_task_stack[0], MP_TASK_STACK_LEN);
#endif

    //Detect sw0 button press or not
    if(mp_hal_pin_read(MICROPY_HW_USRSW_SW0_PIN) == 0) {
        //execute USB mass storage mode and export internal flash
        OnlyMSCVCPMode = true;
        printf("Run MSC and VCP only mode \n");
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

    //initiate module/class resource
    readline_init0();
    pin_init0();
    extint_init0();

    // Initialise the local flash filesystem.
#if MICROPY_HW_HAS_FLASH

    // Create it if needed, mount in on /flash, and set it as current dir.
    mounted_flash = init_flash_fs();
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

    if(mpUSBTaskHandle == NULL) {
        mp_USBRun = true;
        mpUSBTaskHandle = xTaskCreateStatic(mp_usbtask, "USB_MSCVCP",
                                            MP_TASK_STACK_LEN,(void *)!OnlyMSCVCPMode, MP_TASK_PRIORITY, mp_usbtask_stack, &mp_usbtask_tcb);

        if (mpUSBTaskHandle == NULL) {
            __fatal_error("FreeRTOS create USB task!");
        }
    }

    S_USBDEV_STATE *psUSBDevState;
    while(1) {
        psUSBDevState = USBDEV_UpdateState();

        if(mp_USBRun == false)
            break;

        if(OnlyMSCVCPMode == false) {
            if (USBD_IS_ATTACHED()) {
                if(psUSBDevState->bConnected)
                    break;
            } else {
                break;
            }
        }
        vTaskDelay(1);
    }

    vTaskDelay(500);

#endif

    printf("Start execute boot.py and main.py \n");

    // set sys.path based on mounted filesystems (/sd is first so it can override /spiflash)
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

#if MICROPY_KBD_EXCEPTION
    mp_hal_set_interrupt_char(-1);
#endif

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
#if MICROPY_PY_THREAD
        taskYIELD();
#endif
    }

#if defined(_CTRL_D_CHIP_RESET_)
    printf("Do chip reset \n");
    mp_hal_stdout_tx_str("PYB: chip reset \r\n");
#else
    printf("Do soft reset \n");
    mp_hal_stdout_tx_str("PYB: soft reset \r\n");
#endif

#if MICROPY_PY_THREAD
    mp_thread_deinit();
#endif

    mp_deinit();
    fflush(stdout);

#if defined(_CTRL_D_CHIP_RESET_)
    SYS_ResetChip();
#else
    goto soft_reset;
#endif

}

int main(void)
{

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, IP clock and multi-function I/O. */
    SYS_Init();

    /* UART init - will enable valid use of printf (stdout
     * re-directed at this UART (UART6) */
    InitDebugUart();

    printf("+-------------------------------------------------------+\n");
    printf("|          Micropython Code for Nuvoton M55M1           |\n");
    printf("|                     Debug Console                     |\n");
    printf("+-------------------------------------------------------+\n");

#if MICROPY_HW_HAS_FLASH
    flash_init();
#endif

#if MICROPY_HW_HAS_SDCARD
    sdcard_init();
#endif

#if MICROPY_PY_THREAD
    TaskHandle_t mpTaskHandle;

    mpTaskHandle = xTaskCreateStatic(mp_task, "MicroPy",
                                     MP_TASK_STACK_LEN, NULL, MP_TASK_PRIORITY, mp_task_stack, &mp_task_tcb);

    if (mpTaskHandle == NULL) {
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
    return 0;
}

#if MICROPY_PY_THREAD

// We need these functions when configSUPPORT_STATIC_ALLOCATION is enabled
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{

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
