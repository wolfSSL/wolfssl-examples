/* startup.c
 *
 * Vector table and reset handler for the PSOC Edge E8 CM33 (Secure).
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata;
extern uint32_t _sbss, _ebss;
extern void __libc_init_array(void);
extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);
void HardFault_Handler(void);

/* Extended boot has already set VTOR to this table and left the clocks
 * running, so the reset handler only has to set up the C runtime. */
void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst;

    for (dst = &_sdata; dst < &_edata; dst++)
        *dst = *src++;
    for (dst = &_sbss; dst < &_ebss; dst++)
        *dst = 0;

    __libc_init_array();
    (void)main();
    while (1)
        ;
}

/* Called by __libc_init_array; nothing to do without crt0. */
void _init(void) {}
void _fini(void) {}

void Default_Handler(void)
{
    while (1)
        ;
}

void HardFault_Handler(void)
{
    while (1)
        ;
}

/* Exceptions only; the example uses no interrupts. */
__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[16])(void) = {
    (void (*)(void))&_estack,
    Reset_Handler,
    Default_Handler,        /* NMI */
    HardFault_Handler,
    Default_Handler,        /* MemManage */
    Default_Handler,        /* BusFault */
    Default_Handler,        /* UsageFault */
    Default_Handler,        /* SecureFault */
    0, 0, 0,
    Default_Handler,        /* SVCall */
    Default_Handler,        /* DebugMonitor */
    0,
    Default_Handler,        /* PendSV */
    Default_Handler,        /* SysTick */
};
