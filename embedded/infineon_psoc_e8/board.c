/* board.c
 *
 * KIT_PSE84_AI board support: console UART, benchmark time base, TRNG seed
 * and heap for the wolfCrypt test and benchmark.
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
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>

#include "cy_crypto_core.h"
#include "board.h"

/* Extended boot leaves the clocks running and this example does not change
 * them: CLK_HF0 (CM33) at 200 MHz and CLK_HF10 (peripheral group 1) at
 * 100 MHz. */
#ifndef BOARD_CPU_HZ
#define BOARD_CPU_HZ        200000000UL
#endif
#ifndef BOARD_PCLK_HZ
#define BOARD_PCLK_HZ       100000000UL
#endif
#define BOARD_UART_BAUD     115200UL
#define BOARD_UART_OVS      8UL

/* Console: SCB2 on P6.5 (RX) / P6.7 (TX), the KitProg3 COM port. */
#define SCB2_BASE           0x529A0000UL
#define UART_PORT           6
#define UART_RX_PIN         5
#define UART_TX_PIN         7
#define UART_HSIOM_SEL      20UL
#define UART_PCLK_GR        1UL     /* PCLK_SCB2_CLOCK_SCB_EN = 0x101 */
#define UART_PCLK_IDX       1UL
#define UART_PCLK_DIV       0UL

#define REG(a)              (*(volatile uint32_t *)(a))

#define PERI0_GR_SL_CTL(g)  REG(0x52004010UL + ((g) * 0x40UL))
#define PERI0_GR_SL_CTL2(g) REG(0x52004014UL + ((g) * 0x40UL))
#define PCLK_GR(g)          (0x52040000UL + ((g) * 0x2000UL))
#define PCLK_DIV_CMD(g)     REG(PCLK_GR(g))
#define PCLK_CLOCK_CTL(g,i) REG(PCLK_GR(g) + 0xC00UL + ((i) * 4UL))
#define PCLK_DIV_16_5(g,d)  REG(PCLK_GR(g) + 0x1800UL + ((d) * 4UL))
#define HSIOM_PORT_SEL(p,n) REG(0x52800000UL + ((p) * 0x10UL) + \
                                (((n) < 4) ? 0x0UL : 0x4UL))
#define HSIOM_NSMASK(p)     REG(0x52801000UL + ((p) * 0x10UL))
#define GPIO_CFG(p)         REG(0x52810000UL + ((p) * 0x80UL) + 0x44UL)

#define UART_CTRL(b)         REG((b) + 0x000UL)
#define UART_UART_CTRL(b)    REG((b) + 0x040UL)
#define UART_UART_TX_CTRL(b) REG((b) + 0x044UL)
#define UART_UART_RX_CTRL(b) REG((b) + 0x048UL)
#define UART_TX_CTRL(b)      REG((b) + 0x200UL)
#define UART_TX_FIFO_CTRL(b) REG((b) + 0x204UL)
#define UART_TX_FIFO_STAT(b) REG((b) + 0x208UL)
#define UART_TX_FIFO_WR(b)   REG((b) + 0x240UL)
#define UART_RX_CTRL(b)      REG((b) + 0x300UL)
#define UART_RX_FIFO_CTRL(b) REG((b) + 0x304UL)

#define DEMCR               REG(0xE000EDFCUL)
#define DWT_CTRL            REG(0xE0001000UL)
#define DWT_CYCCNT          REG(0xE0001004UL)

static int uart_ready;

static void pin_setup(uint32_t port, uint32_t pin, uint32_t drive)
{
    uint32_t sh = (pin & 3U) * 8U;

    /* Secure writes to a pin marked non-secure are ignored. */
    HSIOM_NSMASK(port) &= ~(1UL << pin);
    HSIOM_PORT_SEL(port, pin) = (HSIOM_PORT_SEL(port, pin) &
        ~(0xFFUL << sh)) | (UART_HSIOM_SEL << sh);
    GPIO_CFG(port) = (GPIO_CFG(port) & ~(0xFUL << (pin * 4U))) |
        (drive << (pin * 4U));
}

static void uart_init(void)
{
    uint32_t scaled = (uint32_t)(((uint64_t)BOARD_PCLK_HZ * 32U) /
                                 (BOARD_UART_BAUD * BOARD_UART_OVS));
    uint32_t timeout = 1000000;

    /* PERI0 group 1 holds the SCBs and is partly gated out of extended boot. */
    PERI0_GR_SL_CTL2(1) = 0;
    PERI0_GR_SL_CTL(1) = 0xFFFFFFFFUL;

    pin_setup(UART_PORT, UART_RX_PIN, 0x8UL);   /* high-Z input */
    pin_setup(UART_PORT, UART_TX_PIN, 0x6UL);   /* strong drive */

    /* 16.5 fractional divider: pclk / (int + 1 + frac / 32). */
    PCLK_DIV_16_5(UART_PCLK_GR, UART_PCLK_DIV) =
        (((scaled / 32U) - 1U) << 8) | ((scaled % 32U) << 3);
    PCLK_CLOCK_CTL(UART_PCLK_GR, UART_PCLK_IDX) = (2UL << 8) | UART_PCLK_DIV;
    PCLK_DIV_CMD(UART_PCLK_GR) = (1UL << 31) | (3UL << 24) | (0xFFUL << 16) |
        (2UL << 8) | UART_PCLK_DIV;
    while (((PCLK_DIV_16_5(UART_PCLK_GR, UART_PCLK_DIV) & 1U) == 0) &&
           (timeout > 0))
        timeout--;
    if (timeout == 0)
        return;     /* an unclocked SCB bus-faults, so stay silent */

    UART_CTRL(SCB2_BASE) = 0;
    UART_UART_CTRL(SCB2_BASE) = 0;
    UART_UART_TX_CTRL(SCB2_BASE) = 1;
    UART_UART_RX_CTRL(SCB2_BASE) = 1;
    UART_TX_CTRL(SCB2_BASE) = 7;                 /* 8 data bits */
    UART_RX_CTRL(SCB2_BASE) = 7;
    UART_TX_FIFO_CTRL(SCB2_BASE) = 0x3F;
    UART_RX_FIFO_CTRL(SCB2_BASE) = 0x3F;
    UART_CTRL(SCB2_BASE) = (1UL << 31) | (2UL << 24) | (BOARD_UART_OVS - 1U);
    uart_ready = 1;
}

static void uart_putc(char c)
{
    uint32_t timeout = 1000000;

    if (!uart_ready)
        return;
    while (((UART_TX_FIFO_STAT(SCB2_BASE) & 0x1FFU) >= 8U) && (timeout > 0))
        timeout--;
    UART_TX_FIFO_WR(SCB2_BASE) = (uint32_t)(uint8_t)c;
}

void board_init(void)
{
    uart_init();
    DEMCR |= (1UL << 24);                       /* TRCENA */
    DWT_CYCCNT = 0;
    DWT_CTRL |= 1UL;                            /* CYCCNTENA */
    (void)Cy_Crypto_Core_Enable(CRYPTO);
}

/* DWT cycle counter extended to 64 bits. It wraps every ~21 s, and each read
 * recovers one wrap, which the benchmark's timing windows stay well inside. */
static uint64_t cycles(void)
{
    static uint64_t hi;
    static uint32_t last;
    uint32_t now = DWT_CYCCNT;

    if (now < last)
        hi += 0x100000000ULL;
    last = now;
    return hi + now;
}

double current_time(int reset)
{
    (void)reset;
    return (double)cycles() / (double)BOARD_CPU_HZ;
}

unsigned long board_time(unsigned long *t)
{
    unsigned long s = (unsigned long)(cycles() / BOARD_CPU_HZ);

    if (t != NULL)
        *t = s;
    return s;
}

/* Seed the Hash-DRBG from the crypto engine's TRNG, one 32-bit word at a
 * time. */
int board_rng_seed(unsigned char *out, unsigned int sz)
{
    uint32_t rnd;
    unsigned int i;

    if (!Cy_Crypto_Core_IsEnabled(CRYPTO) &&
        (Cy_Crypto_Core_Enable(CRYPTO) != CY_CRYPTO_SUCCESS))
        return -1;
    for (i = 0; i < sz; i++) {
        if ((i & 3U) == 0U) {
            if (Cy_Crypto_Core_Trng_Ext(CRYPTO, 32U, &rnd) !=
                    CY_CRYPTO_SUCCESS)
                return -1;
        }
        out[i] = (unsigned char)(rnd >> ((i & 3U) * 8U));
    }
    return 0;
}

/* newlib hooks: console output, and a heap bounded below the stack. */
int _write(int fd, const char *buf, int len)
{
    int i;

    (void)fd;
    for (i = 0; i < len; i++) {
        if (buf[i] == '\n')
            uart_putc('\r');
        uart_putc(buf[i]);
    }
    return len;
}

void *_sbrk(ptrdiff_t incr)
{
    extern char end;
    extern char _heap_limit;
    static char *brk = &end;
    char *prev = brk;

    if ((brk + incr) > &_heap_limit) {
        errno = ENOMEM;
        return (void *)-1;
    }
    brk += incr;
    return prev;
}

int _close(int fd) { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd; st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd) { (void)fd; return 1; }
int _lseek(int fd, int off, int dir) { (void)fd; (void)off; (void)dir; return 0; }
int _read(int fd, char *buf, int len) { (void)fd; (void)buf; (void)len; return 0; }
