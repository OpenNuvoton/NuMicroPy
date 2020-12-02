;/*****************************************************************************
; * @file     GCC.ld
; * @brief    N9H26 link file
; *
; * SPDX-License-Identifier: Apache-2.0
; * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
; *****************************************************************************/


ENTRY(__vector)
/*
	We allocate stack size 0x9000 in Startup_GCC.S
*/
/* define stack size and heap size here */
stack_size = 0x9000;

/* define beginning and ending of stack */
_stack_end = ORIGIN(RAM)+LENGTH(RAM);
_stack_start = _stack_end - stack_size;
MEMORY
{
    RAM  (rwx)   : ORIGIN = 0x000000, LENGTH = 0x2000000
}

SECTIONS
{
    .text :
    {
        PROVIDE(__image_start = .);
        PROVIDE(__text_start = .);

        PROVIDE(__vectors_start = .);
        *(.vectors);
        . = ALIGN(4);
        PROVIDE(__vectors_end = .);
        *(.init);
        . = ALIGN(4);
        *(.text);
        . = ALIGN(4);
        *(.rodata);
        . = ALIGN(4);
        *(.rodata*);
        . = ALIGN(4);

        etext = .;

        PROVIDE(__text_end = .);
    } > RAM

    . = ALIGN(4);
    _etext = . ;
    PROVIDE (etext = .);

    .data : AT (_etext)
    {
        PROVIDE(__data_start__ = .);
        _data = . ;
        *(.data)
        . = ALIGN(4);
        PROVIDE(__data_end__ = .);
    } > RAM

    . = ALIGN(4);
    _edata = . ;
    PROVIDE (edata = .);

    .bss :
    {
        PROVIDE (__bss_start__ = .);
        *(.bss)
        *(.bss.**)
        *(COMMON)
        . = ALIGN(4);
        PROVIDE (__bss_end__ = .);
    }>RAM
    bss_size = __bss_end__ - __bss_start__;
    heap_size = _stack_start - __bss_end__;

    . = ALIGN(8);	
    .heap (COPY):
    {
        PROVIDE (__heap_start__ = .);
        . = . + heap_size;
        end = .;
        PROVIDE (__heap_end__ = .);
    } >RAM
    
    __end__ =.;
}

