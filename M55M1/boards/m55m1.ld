/*
 * Copyright (c) 2009-2021 Arm Limited. All rights reserved.
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * +-----------------------+-------------+-------------+----+----------------------------------+
 * |  Memory region name   | Base addr   |    Size     |IDAU| Description                      |
 * +-----------------------+-------------+-------------+----+----------------------------------+
 * | ITCM                  | 0x0000_0000 | 0x0001_0000 | S  | 64 kiB;                          |
 * | ITCM                  | 0x1000_0000 | 0x0001_0000 | NS | Non-Secure alias for ITCM        |
 * | DTCM                  | 0x2000_0000 | 0x0002_0000 | S  | 128 kiB;                         |
 * | DTCM                  | 0x3000_0000 | 0x0002_0000 | NS | Non-Secure alias for DTCM        |
 * | SRAM01                | 0x2010_0000 | 0x0010_0000 | S  | 2 banks of 512 kiB;              |
 * | SRAM01                | 0x3010_0000 | 0x0010_0000 | NS | Non-Secure alias for SRAM01      |
 * | SRAM2                 | 0x2020_0000 | 0x0005_0000 | S  | 320 kiB                          |
 * | SRAM2                 | 0x3020_0000 | 0x0005_0000 | NS | Non-Secure alias for SRAM2       |
 * | SRAM3                 | 0x2030_0000 | 0x0000_2000 | S  | 8 kiB                            |
 * | SRAM3                 | 0x3030_0000 | 0x0000_2000 | NS | Non-Secure alias for SRAM3       |
 * | LPSRAM                | 0x2031_0000 | 0x0000_2000 | S  | 8 kiB                            |
 * | LPSRAM                | 0x3031_0000 | 0x0000_2000 | NS | Non-Secure alias for LPSRAM      |
 * | FLASH                 | 0x0010_0000 | 0x0020_0000 | S  | 2 MiB                            |
 * | FLASH                 | 0x1010_0000 | 0x0020_0000 | NS | Non-Secure alias for Flash       |
 * | EBI                   | 0x6000_0000 | 0x0030_0000 | S  | 3 MiB                            |
 * | EBI                   | 0x7000_0000 | 0x0030_0000 | NS | Non-Secure alias for EBI         |
 * | Aliased SRAM01        | 0x81F0_0000 | 0x0010_0000 | S  | 2 banks of 512 kiB;              |
 * | Aliased SRAM01        | 0x91F0_0000 | 0x0010_0000 | NS | Non-Secure alias for SRAM01      |
 * | SPIM0                 | 0x8200_0000 | 0x0200_0000 | S  | 32 MiB                           |
 * | SPIM0                 | 0x9200_0000 | 0x0200_0000 | NS | Non-Secure alias for SPIM0       |
 * +-----------------------+-------------+-------------+----+----------------------------------+
 *
 *  Note: Ethos-U55 can access FLASH, internal SRAM 0,1 => activation buffers and
 *        the model should only be placed in those regions.
 *
 *  Note: Alias regions means that secure and non-secure addresses are mapped to the same physical
 *        memory banks.
 */

__STACK_SIZE = 0x00008000;
__HEAP_SIZE  = 0x00020000;

/* System memory brief */
MEMORY
{
  ITCM   (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00010000
  FLASH  (rx)  : ORIGIN = 0x00100000, LENGTH = 0x00200000
  DTCM   (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00020000
  SRAM   (rwx) : ORIGIN = 0x20100000, LENGTH = 0x00130000
  /* Start address and size of NonCacheable region must be 32 byte alignment */
  SRAM_NONCACHEABLE (rw) : ORIGIN = (ORIGIN(SRAM) + LENGTH(SRAM)), LENGTH = (0x00150000 - LENGTH(SRAM))
  LPSRAM (rwx) : ORIGIN = 0x20310000, LENGTH = 0x00002000
}


/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ITCM and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */

ENTRY(Reset_Handler)

SECTIONS
{
  .text :
  {
    KEEP(*(.vectors))
    *startup_*.o(.text*)

    *(.text*)

    KEEP(*(.init))
    KEEP(*(.fini))

    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
    *crtbegin.o(.dtors)
    *crtbegin?.o(.dtors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
    *(SORT(.dtors.*))
    *(.dtors)


    KEEP(*(.eh_frame*))
  } > FLASH

  .rodata : {
    . = ALIGN(8);
    *(.rodata*)
    . = ALIGN(8);
  } >FLASH

  .rel.dyn : {
    . = ALIGN(8);
    __rel_dyn_start = .;
    *(.rel*)
    __rel_dyn_end = .;
    . = ALIGN(8);
   } > FLASH

   .dynsym : {
     . = ALIGN(8);
     __dynsym_start = .;
     *(.dynsym)
     __dynsym_end = .;
     . = ALIGN(8);
    } > FLASH

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } > FLASH

  .zero.table :
  {
    . = ALIGN(4);
    __zero_table_start__ = .;
    LONG (__bss_start__)
    LONG ((__bss_end__ - __bss_start__) / 4)
    LONG (__dtcm_zi_start__)
    LONG ((__dtcm_zi_end__ - __dtcm_zi_start__) / 4)
    LONG (__sram_noncacheable_zi_start__)
    LONG ((__sram_noncacheable_zi_end__ - __sram_noncacheable_zi_start__) / 4)
    LONG (__lpsram_zi_start__)
    LONG ((__lpsram_zi_end__ - __lpsram_zi_start__) / 4)
    __zero_table_end__ = .;
  } > FLASH

  .copy.table :
  {
    . = ALIGN(4);
    __copy_table_start__ = .;

    /* Section to be copied - part 1: RW data for SRAM */
    LONG (__eitcm)
    LONG (__data_start__)
    LONG ((__data_end__ - __data_start__) / 4)

    /* Section to be copied - part 2: RO code for ITCM */
    LONG (__etext)
    LONG (__itcm_text_start__)
    LONG ((__itcm_text_end__ - __itcm_text_start__ ) / 4)

    /* Section to be copied - part 3: RW data for DTCM */
    LONG (__esram_data)
    LONG (__dtcm_init_start__)
    LONG ((__dtcm_init_end__ - __dtcm_init_start__ ) / 4)

    /* Section to be copied - part 4: RW data for SRAM NonCacheable */
    LONG (__edtcm_init_data)
    LONG (__sram_noncacheable_init_start__)
    LONG ((__sram_noncacheable_init_end__ - __sram_noncacheable_init_start__) / 4)

    __copy_table_end__ = .;
  } > FLASH

  /**
   * Location counter can end up 2 byte aligned with narrow Thumb code but
   * __etext is assumed by startup code to be the LMA of a section
   * which must be 4byte aligned
   */
  __etext = ALIGN (4);

  .itcm : AT (__etext)
  {
    . = ALIGN(4);
    __itcm_text_start__ = .;
    *(ITCM)
    . = ALIGN(4);
    __itcm_text_end__ = .;
  } > ITCM

  __eitcm = __etext + (__itcm_text_end__ - __itcm_text_start__);

  .data : AT(__eitcm)
  {
    __data_start__ = .;
    *(vtable)
    *(.data)
    *(.data.*)

    . = ALIGN(4);
    /* preinit data */
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);

    . = ALIGN(4);
    /* finit data */
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);

    KEEP(*(.jcr*))
    . = ALIGN(4);
    /* All data end */
    __data_end__ = .;
  } > SRAM

  .sram :
  {
    . = ALIGN(16);
    *(.bss.tensor_arena)
  } > SRAM

  .bss :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > SRAM AT > SRAM

  __esram_data = __eitcm + (__data_end__ - __data_start__);

  .dtcm.init : AT (__esram_data)
  {
    . = ALIGN(4);
    __dtcm_init_start__ = .;
    *(DTCM.VTOR)
    *(DTCM.Init)
    . = ALIGN(4);
    __dtcm_init_end__ = .;
  } > DTCM

  .dtcm.zi (NOLOAD) :
  {
    . = ALIGN(4);
    __dtcm_zi_start__ = .;
    *(.DTCM.ZeroInit)
    . = ALIGN(4);
    __dtcm_zi_end__ = .;
  } > DTCM AT > DTCM

  __edtcm_init_data = __esram_data + (__dtcm_init_end__ - __dtcm_init_start__);

  .NonCacheable.Init : AT (__edtcm_init_data)
  {
    . = ALIGN(4);
    __sram_noncacheable_start__ = .;
    __sram_noncacheable_init_start__ = .;
    *(NonCacheable.Init)
    . = ALIGN(4);
    __sram_noncacheable_init_end__ = .;
  } > SRAM_NONCACHEABLE

  .NonCacheable.zi (NOLOAD) :
  {
    . = ALIGN(4);
    __sram_noncacheable_zi_start__ = .;
    *(.NonCacheable.ZeroInit)
    . = ALIGN(4);
    __sram_noncacheable_zi_end__ = .;
    __sram_noncacheable_end__ = (ORIGIN(SRAM_NONCACHEABLE) + LENGTH(SRAM_NONCACHEABLE));
  } > SRAM_NONCACHEABLE AT > SRAM_NONCACHEABLE

  .lpsram (NOLOAD) :
  {
    . = ALIGN(4);
    __lpsram_zi_start__ = .;
    *(.lpSram)
    . = ALIGN(4);
    __lpsram_zi_end__ = .;
  } > LPSRAM AT > LPSRAM

  .heap (NOLOAD) :
  {
    . = ALIGN(8);
    __end__ = .;
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > SRAM

  .stack (ORIGIN(DTCM) + LENGTH(DTCM) - __STACK_SIZE) (NOLOAD) :
  {
    . = ALIGN(8);
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > DTCM
  PROVIDE(__stack = __StackTop);

  /* Because stack placed in DTCM, cannot check if data + heap + stack exceeds SRAM limit */
  /*ASSERT(__StackLimit >= __HeapLimit, "region SRAM overflowed with stack")*/
}
