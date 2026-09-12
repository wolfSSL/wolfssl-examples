/* app.c
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

/* wolfCrypt test and benchmark on the NuMaker-M2354.
 *
 * Brings the clocks and the console up, registers the crypto callback device,
 * then runs the algorithm test followed by the benchmark. Output goes to
 * UART0, which the on board Nu-Link2-Me presents as a virtual COM port at
 * 115200 baud.
 */

/* NuMicro.h first: M2354.h defines TRUE and FALSE unconditionally and would
 * redefine the ones types.h puts up behind an #ifndef. */
#include "NuMicro.h"

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/port/nuvoton/nuvoton_cryptocb.h>
#include <wolfcrypt/test/test.h>
#include <wolfcrypt/benchmark/benchmark.h>

#include <stdio.h>
#include <errno.h>
#include <stddef.h>

/* Provided by the BSP linker script; used by the fault report and by _sbrk. */
/* Top of the 256 KB the part implements, for the stacked frame sanity check in
 * the fault handler. SRAM_BASE comes from M2354.h. */
#define APP_SRAM_TOP (SRAM_BASE + 0x40000u)

extern uint32_t __StackLimit;
extern uint32_t __HeapBase;
extern uint32_t __HeapLimit;

/* Bounded heap.
 *
 * The BSP's _syscalls.c does not define _sbrk, so the link would otherwise
 * pull in the libnosys one, which moves the break with no bounds check at all
 * and happily returns addresses past the end of physical RAM. The failure is
 * not a clean out-of-memory: malloc succeeds, the caller writes, and the fault
 * lands somewhere unrelated - here it surfaced as memcpy inside _realloc_r
 * storing to 0x20040000, one word past the top of SRAM.
 *
 * Defining _sbrk here means the linker resolves it from this object and never
 * looks in the library. Running out of heap now returns ENOMEM, which
 * wolfCrypt turns into MEMORY_E and reports against the test that asked. */
void* _sbrk(ptrdiff_t incr)
{
    static char* heapEnd = NULL;
    char*        prev;

    if (heapEnd == NULL) {
        heapEnd = (char*)&__HeapBase;
    }

    if (incr < 0 || (heapEnd + incr) > (char*)&__HeapLimit) {
        errno = ENOMEM;
        return (void*)-1;
    }

    prev = heapEnd;
    heapEnd += incr;

    return prev;
}

/* High-water mark of the heap, for reporting after the test run. */
static size_t heap_used(void)
{
    char* base = (char*)&__HeapBase;
    void* cur  = _sbrk(0);

    if (cur == (void*)-1) {
        return 0;
    }

    return (size_t)((char*)cur - base);
}

/* Free running millisecond counter, driven by SysTick. */
static volatile word32 msTicks = 0;

void SysTick_Handler(void);
void CRPT_IRQHandler(void);

void SysTick_Handler(void)
{
    msTicks++;
}

/* The BSP public key drivers block on a flag that only this interrupt sets
 * (see ECC_DriverISR in Library/StdDriver/src/crypto.c). Without this handler
 * every ECC and RSA call would spin until its timeout and fail. */
void CRPT_IRQHandler(void)
{
    ECC_DriverISR(CRPT);
}

/* WOLFSSL_USER_CURRTIME: seconds, used by the benchmark to work out rates. */
double current_time(int reset)
{
    if (reset) {
        msTicks = 0;
    }

    return (double)msTicks / 1000.0;
}

/* USER_TICKS: coarse seconds counter. */
word32 LowResTimer(void)
{
    return msTicks / 1000;
}

/* Route printf to UART0.
 *
 * UART_WRITE() is a bare store to DAT with no flow control, and the UART0
 * transmit FIFO is 16 bytes deep. At 96 MHz the CPU fills that far faster than
 * 115200 baud drains it, so writing without checking loses everything after
 * the first FIFO-full - which looks like a baud rate or wiring fault rather
 * than a software one, because what does arrive is correctly spelled. Wait for
 * space before every byte. */
static void uart_put(char c)
{
    while (UART_IS_TX_FULL(UART0)) {
        /* Wait for the transmit FIFO to drain. */
    }
    UART_WRITE(UART0, (uint8_t)c);
}

int _write(int fd, const char* buf, int len)
{
    int i;

    (void)fd;

    for (i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            uart_put('\r');
        }
        uart_put(buf[i]);
    }

    return len;
}

/* The BSP startup calls these two by name. ProcessHardFault gets the stacked
 * register frame and returns the address to resume at; parking in a loop keeps
 * a fault visible under the debugger instead of silently restarting.
 * SH_Return belongs to the BSP's semihosting path, which is not used here. */
uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp);
int32_t SH_Return(int32_t n32In_R0, int32_t n32In_R1, int32_t* pn32Out_R0);

uint32_t ProcessHardFault(uint32_t lr, uint32_t msp, uint32_t psp)
{
    const uint32_t* frame;

    /* EXC_RETURN bit 2 says which stack the exception frame went on. The frame
     * is r0, r1, r2, r3, r12, lr, pc, xpsr - so the faulting instruction is at
     * index 6. ARMv8-M baseline has no SecureFault and no CFSR to consult, so
     * the stacked PC is most of what there is to go on: feed it to
     * arm-none-eabi-addr2line against the .elf. */
    frame = (const uint32_t*)((lr & 0x4u) ? psp : msp);

    printf("\nhard fault\n");
    printf("  exc_return 0x%08lx  msp 0x%08lx  psp 0x%08lx\n",
        (unsigned long)lr, (unsigned long)msp, (unsigned long)psp);
    printf("  sp   0x%08lx  stack limit 0x%08lx\n",
        (unsigned long)frame, (unsigned long)&__StackLimit);

    /* A stack that collided with the bottom of RAM leaves the frame pointer
     * outside SRAM, and reading it here would fault again - which on ARMv8-M
     * baseline escalates to Lockup and prints nothing at all. That is the very
     * case the stack limit line above is here to diagnose, so the register
     * dump is skipped rather than risked. */
    if ((uint32_t)frame < SRAM_BASE ||
        (uint32_t)frame > (APP_SRAM_TOP - 8u * sizeof(uint32_t))) {
        printf("  stacked frame outside SRAM, not dumping registers\n");
        while (1) {
            /* Stop here so the fault state can be read out. */
        }
    }

    printf("  pc   0x%08lx\n", (unsigned long)frame[6]);
    printf("  lr   0x%08lx\n", (unsigned long)frame[5]);
    printf("  xpsr 0x%08lx\n", (unsigned long)frame[7]);
    printf("  r0   0x%08lx  r1 0x%08lx\n",
        (unsigned long)frame[0], (unsigned long)frame[1]);
    printf("  r2   0x%08lx  r3 0x%08lx\n",
        (unsigned long)frame[2], (unsigned long)frame[3]);
    while (1) {
        /* Stop here so the fault state can be read out. */
    }
}

int32_t SH_Return(int32_t n32In_R0, int32_t n32In_R1, int32_t* pn32Out_R0)
{
    (void)n32In_R0;
    (void)n32In_R1;
    (void)pn32Out_R0;

    return 0;
}

static void SYS_Init(void)
{
    SYS_UnlockReg();

    /* Only SRAM bank 0 is clocked out of reset: AHBCLK comes up as 0x00108000,
     * so SRAM0CKEN (bit 20) is set while SRAM1CKEN (21) and SRAM2CKEN (22) are
     * clear. Everything at or above 0x20008000 is dead, and writes to it are
     * silently discarded rather than faulting - so the failure is wrong
     * results or a garbage return address, not an exception pointing at the
     * access. Enable them before anything can allocate. Found on silicon by
     * the wolfBoot M2354 session, whose stack was at the top of SRAM and which
     * sat in Lockup with a perfectly valid image in flash. */
    CLK->AHBCLK |= CLK_AHBCLK_SRAM1CKEN_Msk | CLK_AHBCLK_SRAM2CKEN_Msk;

    /* 12 MHz crystal into the PLL, out at the part's 96 MHz ceiling.
     *
     * Go through CLK_SetCoreClock() rather than writing CLKSEL0 directly. It
     * calls CLK_SetHCLK(), which does three things in a strict order that a
     * hand-rolled sequence has to reproduce exactly:
     *
     *   1. Parks HCLK on HIRC before touching anything else.
     *   2. Raises the power level to PL0 and waits for PLSTS.PLCBUSY to
     *      clear. Only PL0 supports 96 MHz; PL1 tops out at 84 MHz.
     *   3. Sets FMC->CYCCTL CYCLE to 4 flash access cycles, which is what
     *      96 MHz needs, and only then switches HCLK to the PLL.
     *
     * Skipping either the power level or the wait states mostly works from a
     * cold reset, because reset leaves the part at PL0 - the failure shows up
     * on a warm reset that arrives at a lower level or with fewer wait states,
     * as an intermittent hard fault or garbage instruction fetch that points
     * nowhere near the clock code. Found in the wolfBoot M2354 HAL, which
     * reimplements this against raw registers; noted here so a later
     * simplification of this file does not reintroduce it. */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
    CLK_SetCoreClock(FREQ_96MHZ);

    /* HIRC for UART0, so the console baud rate does not move when the PLL
     * engages. Enable HIRC explicitly rather than assuming it is on:
     * CLK_SetHCLK() samples the HIRC stable flag on entry and, if it found
     * HIRC off, switches it back off before returning (clk.c, "Disable HIRC if
     * HIRC is disabled before switching HCLK source"). Pointing UART0 at a
     * source that was just switched off gives a console that works from a cold
     * reset - where HIRC is on by default - and is silent after any warm reset
     * that arrives with it off. That is a whole afternoon lost to suspecting
     * the PB12/PB13 pin choice instead. */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL2_UART0SEL_HIRC,
        CLK_CLKDIV0_UART0(1));

    /* The NuMaker-M2354 wires the Nu-Link2-Me virtual COM port to UART0 on
     * PA6 (RXD) and PA7 (TXD). Not PB12/PB13: those are also a valid UART0
     * assignment on this part, which is why the BSP offers both and why
     * picking from the macro names alone is not enough, but they go nowhere on
     * this board. Confirmed on hardware - a PB12/PB13 build transmits happily
     * (FIFOSTS reports TXEMPTY) with nothing arriving at the host. */
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA6MFP_Msk | SYS_GPA_MFPL_PA7MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA6MFP_UART0_RXD |
                      SYS_GPA_MFPL_PA7MFP_UART0_TXD);

    SystemCoreClockUpdate();

    SYS_LockReg();
}

int keystore_test(void);

int main(void)
{
    int ret;

    SYS_Init();
    UART_Open(UART0, 115200);

    /* One tick per millisecond for current_time() and LowResTimer(). */
    SysTick_Config(SystemCoreClock / 1000);

    /* The public key engines need this interrupt; see CRPT_IRQHandler. */
    NVIC_EnableIRQ(CRPT_IRQn);

    printf("\nwolfCrypt on NuMaker-M2354 (%u MHz)\n",
        (unsigned int)(SystemCoreClock / 1000000));
#ifdef WOLFSSL_NUVOTON_NSC
    printf("TrustZone: non-secure, hardware through NSC veneers\n");
#else
    printf("TrustZone: secure world, direct BSP calls\n");
#endif

    ret = wolfCrypt_Init();
    if (ret != 0) {
        printf("wolfCrypt_Init failed: %d\n", ret);
        return ret;
    }

    ret = wc_NuvotonCryptoCb_RegisterDevice(WOLFSSL_NUVOTON_DEVID);
    if (ret != 0) {
        printf("Nuvoton device registration failed: %d\n", ret);
        return ret;
    }
    printf("Nuvoton crypto callback device registered as devId %d\n",
        WOLFSSL_NUVOTON_DEVID);

    /* The Key Store is not covered by wolfcrypt_test, so it gets its own
     * round trip first. */
    ret = keystore_test();
    printf("keystore_test returned %d\n", ret);

    printf("\n--- wolfcrypt_test ---\n");
    ret = wolfcrypt_test(NULL);
    printf("wolfcrypt_test returned %d\n", ret);
    printf("heap used %u of %u bytes\n", (unsigned int)heap_used(),
        (unsigned int)((char*)&__HeapLimit - (char*)&__HeapBase));

    printf("\n--- benchmark ---\n");
    ret = benchmark_test(NULL);
    printf("benchmark_test returned %d\n", ret);

    wc_NuvotonCryptoCb_UnRegisterDevice(WOLFSSL_NUVOTON_DEVID);
    wolfCrypt_Cleanup();

    printf("\ndone\n");

    while (1) {
        /* Nothing left to do; leave the console output on screen. */
    }
}
