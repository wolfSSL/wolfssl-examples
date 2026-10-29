/* linker.ld
 *
 * PSOC Edge E8 CM33 Secure image, run in place from QSPI.
 *
 * Extended boot launches the image at QSPI 0x60100000 after its 0x400-byte
 * MCUboot header, through the Secure code-bus alias 0x18100400. SRAM below
 * 0x34002000 is reserved for extended boot and the Secure Enclave.
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x18100400, LENGTH = 0x001FFC00
    RAM (rwx)  : ORIGIN = 0x34002000, LENGTH = 0x00056000
}

_Min_Stack_Size = 0x8000;

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
        *(.text*)
        *(.rodata*)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    } > FLASH

    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Heap from here up to the stack. */
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);

    _estack = ORIGIN(RAM) + LENGTH(RAM);
    _heap_limit = _estack - _Min_Stack_Size;
    ASSERT(end < _heap_limit, "no room left for the heap")
}
