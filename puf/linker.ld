/* linker.ld
 *
 * Linker script for bare-metal Cortex-M33 with PUF SRAM region.
 * Default: NUCLEO-H563ZI (STM32H563ZI)
 *
 * Uses STM32-standard symbol names for compatibility with
 * startup_stm32h563zitx.s from STM32CubeH5.
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 */

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size  = 0x400;
_Min_Stack_Size = 0x2000;

MEMORY
{
    /* STM32H563ZI: 2MB flash, 640KB SRAM */
    FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    RAM     (rwx) : ORIGIN = 0x20000000, LENGTH = 636K
    /* Reserve 4K at end of SRAM for PUF (not touched by .bss clear) */
    PUF_RAM (rw)  : ORIGIN = 0x2009F000, LENGTH = 4K
}

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /* PUF SRAM section: NOLOAD prevents startup code from zeroing.
     * The raw power-on state of these bytes is the PUF source.
     * Required size is driven by WC_PUF_RAW_BYTES in
     * wolfssl/wolfcrypt/puf.h (currently 256 bytes). The 4 KB
     * reservation in MEMORY{} above is generous headroom; the ASSERT
     * below catches future overflow if WC_PUF_RAW_BYTES grows. */
    .puf_sram (NOLOAD) :
    {
        . = ALIGN(4);
        _puf_sram_start = .;
        KEEP(*(.puf_sram))
        KEEP(*(.puf_sram.*))
        _puf_sram_end = .;
    } > PUF_RAM

    ASSERT(SIZEOF(.puf_sram) <= LENGTH(PUF_RAM),
           "Error: .puf_sram exceeds reserved PUF_RAM region size")
}
