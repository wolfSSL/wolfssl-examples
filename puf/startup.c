/* startup.c
 *
 * Minimal Cortex-M startup for PUF example.
 * Provides vector table, Reset_Handler, and SystemInit.
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
#include <string.h>

/* Linker-provided symbols (STM32-standard names) */
extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata;
extern uint32_t _sbss, _ebss;
extern void __libc_init_array(void);
extern int main(void);

/* Forward declarations */
void Reset_Handler(void);
void Default_Handler(void);
void SystemInit(void);

/* -------------------------------------------------------------------------- */
/* SystemInit - minimal clock setup                                           */
/* -------------------------------------------------------------------------- */

void SystemInit(void)
{
    /* Set VTOR to flash base */
    *(volatile uint32_t *)0xE000ED08 = 0x08000000;
    /* Default HSI clock (64 MHz) is sufficient for this example */
}

/* -------------------------------------------------------------------------- */
/* Reset Handler                                                              */
/* -------------------------------------------------------------------------- */

void __attribute__((naked)) Reset_Handler(void)
{
    __asm volatile (
        "ldr r0, =_estack  \n"
        "mov sp, r0        \n"
        "bl SystemInit     \n"

        /* Copy .data from flash to SRAM */
        "ldr r0, =_sdata   \n"
        "ldr r1, =_edata   \n"
        "ldr r2, =_sidata  \n"
        "movs r3, #0       \n"
        "b .Ldata_check    \n"
    ".Ldata_copy:           \n"
        "ldr r4, [r2, r3]  \n"
        "str r4, [r0, r3]  \n"
        "adds r3, r3, #4   \n"
    ".Ldata_check:          \n"
        "adds r4, r0, r3   \n"
        "cmp r4, r1        \n"
        "bcc .Ldata_copy   \n"

        /* Zero .bss */
        "ldr r2, =_sbss    \n"
        "ldr r4, =_ebss    \n"
        "movs r3, #0       \n"
        "b .Lbss_check     \n"
    ".Lbss_zero:            \n"
        "str r3, [r2]      \n"
        "adds r2, r2, #4   \n"
    ".Lbss_check:           \n"
        "cmp r2, r4        \n"
        "bcc .Lbss_zero    \n"

        /* Call static constructors, then main */
        "bl __libc_init_array \n"
        "bl main           \n"
    ".Lhang:                \n"
        "b .Lhang          \n"
    );
}

void Default_Handler(void)
{
    while (1) { }
}

/* -------------------------------------------------------------------------- */
/* Vector table                                                               */
/* -------------------------------------------------------------------------- */

/* Weak aliases so the application can override individual handlers */
void __attribute__((weak, alias("Default_Handler"))) NMI_Handler(void);
void __attribute__((weak, alias("Default_Handler"))) HardFault_Handler(void);
void __attribute__((weak, alias("Default_Handler"))) MemManage_Handler(void);
void __attribute__((weak, alias("Default_Handler"))) BusFault_Handler(void);
void __attribute__((weak, alias("Default_Handler"))) UsageFault_Handler(void);
void __attribute__((weak, alias("Default_Handler"))) SecureFault_Handler(void);
void __attribute__((weak, alias("Default_Handler"))) SVC_Handler(void);
void __attribute__((weak, alias("Default_Handler"))) DebugMon_Handler(void);
void __attribute__((weak, alias("Default_Handler"))) PendSV_Handler(void);
void __attribute__((weak, alias("Default_Handler"))) SysTick_Handler(void);

typedef void (*vector_fn)(void);

const vector_fn __isr_vector[] __attribute__((section(".isr_vector"), used)) = {
    (vector_fn)(uintptr_t)&_estack,  /* Initial SP */
    Reset_Handler,                    /* Reset */
    NMI_Handler,                      /* NMI */
    HardFault_Handler,                /* Hard Fault */
    MemManage_Handler,                /* MPU Fault */
    BusFault_Handler,                 /* Bus Fault */
    UsageFault_Handler,               /* Usage Fault */
    SecureFault_Handler,              /* Secure Fault (M33) */
    0, 0, 0,                          /* Reserved */
    SVC_Handler,                      /* SVCall */
    DebugMon_Handler,                 /* Debug Monitor */
    0,                                /* Reserved */
    PendSV_Handler,                   /* PendSV */
    SysTick_Handler,                  /* SysTick */
    /* Peripheral IRQs default to Default_Handler via unused slots */
};
