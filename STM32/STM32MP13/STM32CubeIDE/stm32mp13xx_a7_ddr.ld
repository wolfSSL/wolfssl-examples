/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Abstract    : Linker script for STM32MP13xx series
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed �as is,� without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
**
*****************************************************************************
*/


/* Entry Point */
ENTRY(Reset_Handler)

_Min_Heap_Size = 0x200;     /* required amount of heap  */
_Min_Stack_Size = 0x400;    /* required amount of stack */

TTB_L1_SIZE = 16384;        /* MMU Level 1 Translation table size */
TTB_L2_SIZE = 1024;         /* MMU Level 2 Translation table size */

MEMORY {
    SYSRAM_BASE (rwx)   : ORIGIN = 0x2FFE0000, LENGTH = 128K
    SRAM1_BASE (rwx)    : ORIGIN = 0x30000000, LENGTH = 16K
    SRAM2_BASE (rwx)    : ORIGIN = 0x30004000, LENGTH = 8K
    SRAM3_BASE (rwx)    : ORIGIN = 0x30006000, LENGTH = 8K
    DDR_BASE (rwx)      : ORIGIN = 0xC0000000, LENGTH = 512M
}

/* Select Memory in which code has to be placed and offset of code from start of this memory */
REGION_ALIAS("RAM", DDR_BASE);
START_OFFSET = 0x00;

/* Code Memory */
__MEM_START__ = ORIGIN(RAM) + START_OFFSET;
__MEM_SIZE__  = LENGTH(RAM) - START_OFFSET;
__FIQ_STACK_SIZE = 0x400;
__IRQ_STACK_SIZE = 0x400;
__ABT_STACK_SIZE = 0x400;
__SVC_STACK_SIZE = 0x400;
__UND_STACK_SIZE = 0x400;


/* put the stacks at the end of the memory */
FIQ_STACK = __MEM_START__ + __MEM_SIZE__;
IRQ_STACK = FIQ_STACK - __FIQ_STACK_SIZE;
ABT_STACK = IRQ_STACK - __IRQ_STACK_SIZE;
SVC_STACK = ABT_STACK - __ABT_STACK_SIZE;
UND_STACK = SVC_STACK - __SVC_STACK_SIZE;
SYS_STACK = UND_STACK - __UND_STACK_SIZE;

PHDRS
{
	text PT_LOAD FLAGS(5);  /* READ and EXECUTE */
	data PT_LOAD FLAGS(6);  /* READ and WRITE */
	bss PT_LOAD FLAGS(6);   /* READ and WRITE */
}

SECTIONS
{
    . = __MEM_START__;
    .RESET . : {
        __TEXT_START__ = .;
        *(.text.Reset_Handler)
        * (RESET)
        *(.text*)

        /* .init is used by libc_nano */
        KEEP (*(.init))
        KEEP (*(.fini))

        __TEXT_END_UNALIGNED__ = .;

        /*
         * Memory page(s) mapped to this section will be marked as
         * read-only, executable.  No non-executable data from the next section must
         * creep in.  Ensure the rest of the current memory page is unused.
         */
        __TEXT_END__ = .;

        . = NEXT(4096);

        __RO_START__ = .;
        *(.rodata*)

        __RO_END_UNALIGNED__ = .;
        /*
         * Memory page(s) mapped to this section will be marked as
         * read-only, non-executable.  No RW data from the next section must
         * creep in.  Ensure the rest of the current memory page is unused.
         */
        . = NEXT(4096);
        __RO_END__ = .;
    } >RAM :text


    .ARM.extab   : {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
    } >RAM

    .ARM : {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
    } >RAM

    /* .init_array is used by libc_nano */

    .preinit_array     :
    {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
    } >RAM

    .init_array :
    {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
    } >RAM

    .fini_array :
    {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
    } >RAM


    .data . : {
        RW_DATA = .;
        *(.data*)
        __DATA_END__ = .;
    } >RAM

    /*
     * The .bss section gets initialised to 0 at runtime.
     * Its base address must be 16-byte aligned.
     */
    .bss : ALIGN(32) {
        ZI_DATA = .;
        *(SORT_BY_ALIGNMENT(.bss*))
        *(COMMON)
        . = ALIGN(32);
        __BSS_END__ = .;
    } >RAM

    __BSS_SIZE__ = SIZEOF(.bss);

/* 1 TTB Level 1 table */
/* 4 TTB Level 2 tables */
    ._TTB : ALIGN(16384) {
       TTB = .;
       . = . + TTB_L1_SIZE;
       . = . + TTB_L2_SIZE * 4;
    } > RAM


    /* User_heap_stack section, used to check that there is enough RAM left */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    .tcp_sec (NOLOAD) : 
    {
        . = ALIGN(4);
        *(.RxDecripSection)
        . = ALIGN(4);
        *(.TxDecripSection)
        . = ALIGN(4);
        *(.Rx_PoolSection)
        . = ALIGN(4);
        *(.LwIP_Heap_RAM_Section)
    } >SYSRAM_BASE

    .stack     :
    {

    _stack_bottom = ABSOLUTE(.) ;

    /* Allocate room for stack. This must be big enough for the IRQ, FIQ, and
       SYS stack if nested interrupts are enabled.*/
    .   =  ALIGN(8) ;
    .   += 32768 ;
    _sp =  . - 16 ;
    _stack_top = ABSOLUTE(.) ;
    } >RAM

    _end = .;


  /* Remove information from the compiler libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
}
