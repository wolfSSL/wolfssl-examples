/* stm32.c
 *
 * STM32H5 HAL support for PUF example (tested on NUCLEO-H563ZI).
 * Provides USART3 init/output, printf retarget, RNG stub, and time stub.
 *
 * To port to a different MCU, replace this file with your platform's
 * UART and RNG implementation. The interface is:
 *   void hal_init(void) - called once at startup before printf
 *   int my_rng_seed_gen(uint8_t* output, uint32_t sz) - RNG seed
 *   unsigned long my_time(unsigned long* timer) - monotonic time
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

/* -------------------------------------------------------------------------- */
/* STM32H5 USART3 (ST-LINK VCP) bare-metal driver                            */
/* -------------------------------------------------------------------------- */

/* STM32H563 register bases.
 * TZEN=0: use non-secure aliases (0x4xxx).
 * TZEN=1: use secure aliases (0x5xxx). */
#ifdef STM32H5_TZEN
#define RCC_BASE        0x54020C00u
#define GPIOD_BASE      0x52020C00u
#define USART3_BASE     0x50004800u
#else
#define RCC_BASE        0x44020C00u
#define GPIOD_BASE      0x42020C00u
#define USART3_BASE     0x40004800u
#endif

#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x8Cu))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x9Cu))

/* GPIO registers */
#define GPIO_MODER(b)   (*(volatile uint32_t *)((b) + 0x00u))
#define GPIO_OSPEEDR(b) (*(volatile uint32_t *)((b) + 0x08u))
#define GPIO_AFRH(b)    (*(volatile uint32_t *)((b) + 0x24u))

/* USART3 registers */
#define USART3_CR1      (*(volatile uint32_t *)(USART3_BASE + 0x00u))
#define USART3_CR2      (*(volatile uint32_t *)(USART3_BASE + 0x04u))
#define USART3_CR3      (*(volatile uint32_t *)(USART3_BASE + 0x08u))
#define USART3_BRR      (*(volatile uint32_t *)(USART3_BASE + 0x0Cu))
#define USART3_ISR      (*(volatile uint32_t *)(USART3_BASE + 0x1Cu))
#define USART3_TDR      (*(volatile uint32_t *)(USART3_BASE + 0x28u))
#define USART3_PRESC    (*(volatile uint32_t *)(USART3_BASE + 0x2Cu))

static void delay(volatile uint32_t n)
{
    while (n--) { }
}

static void uart_init(void)
{
    uint32_t moder, afr;

    /* Enable GPIOD clock */
    RCC_AHB2ENR |= (1u << 3);
    /* Enable USART3 clock (APB1LENR bit 18) */
    RCC_APB1ENR |= (1u << 18);
    delay(100);

    /* Configure PD8 (TX) as AF7, push-pull, high speed */
    moder = GPIO_MODER(GPIOD_BASE);
    moder &= ~(3u << 16);
    moder |= (2u << 16);   /* Alternate function */
    GPIO_MODER(GPIOD_BASE) = moder;
    GPIO_OSPEEDR(GPIOD_BASE) |= (3u << 16);  /* High speed for PD8 */
    afr = GPIO_AFRH(GPIOD_BASE);
    afr &= ~(0xFu << 0);
    afr |= (7u << 0);      /* AF7 = USART3 */
    GPIO_AFRH(GPIOD_BASE) = afr;

    /* Configure USART3: 115200 baud at default 32 MHz PCLK1 */
    USART3_CR1 = 0;
    USART3_CR2 = 0;
    USART3_CR3 = 0;
    USART3_PRESC = 0;
    USART3_BRR = 32000000u / 115200u;  /* ~278 */
    USART3_CR1 = (1u << 3);  /* TE */
    delay(10);
    USART3_CR1 |= (1u << 0);  /* UE */
    delay(100);
}

static void uart_putc(char c)
{
    while ((USART3_ISR & (1u << 7)) == 0) { }
    USART3_TDR = (uint32_t)c;
}

/* Retarget _write for printf via USART3 */
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

/* -------------------------------------------------------------------------- */
/* STM32H5 Hardware RNG (TRNG) driver                                        */
/* -------------------------------------------------------------------------- */

/* RCC clock control */
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00u))
#define RCC_CR_HSI48ON      (1u << 12)
#define RCC_CR_HSI48RDY     (1u << 13)
#define RCC_CCIPR5      (*(volatile uint32_t *)(RCC_BASE + 0xE8u))
#define RCC_CCIPR5_RNGSEL_Msk (3u << 4)

/* RNG peripheral */
#ifdef STM32H5_TZEN
#define RNG_BASE        0x520C0800u
#else
#define RNG_BASE        0x420C0800u
#endif

#define RNG_CR          (*(volatile uint32_t *)(RNG_BASE + 0x00u))
#define RNG_SR          (*(volatile uint32_t *)(RNG_BASE + 0x04u))
#define RNG_DR          (*(volatile uint32_t *)(RNG_BASE + 0x08u))
#define RNG_CR_RNGEN          (1u << 2)
#define RNG_CR_CONDRST        (1u << 30)
#define RNG_CR_CONFIG3_SHIFT  8u
#define RNG_CR_CONFIG2_SHIFT  13u
#define RNG_CR_CLKDIV_SHIFT   16u
#define RNG_CR_CONFIG1_SHIFT  20u
#define RNG_SR_DRDY           (1u << 0)
#define RNG_SR_CECS           (1u << 1)
#define RNG_SR_SECS           (1u << 2)
#define RNG_SR_CEIS           (1u << 5)
#define RNG_SR_SEIS           (1u << 6)

static void rng_init(void)
{
    uint32_t rng_cr;

    /* Enable HSI48 as RNG kernel clock source */
    RCC_CR |= RCC_CR_HSI48ON;
    while ((RCC_CR & RCC_CR_HSI48RDY) == 0u) { }

    /* Select HSI48 for RNG clock */
    RCC_CCIPR5 &= ~RCC_CCIPR5_RNGSEL_Msk;
    RCC_AHB2ENR |= (1u << 18);  /* RNG clock enable */
    delay(100);

    /* Configure and enable RNG with conditioning reset */
    rng_cr = RNG_CR;
    rng_cr &= ~(0x1Fu << RNG_CR_CONFIG1_SHIFT);
    rng_cr &= ~(0x7u << RNG_CR_CLKDIV_SHIFT);
    rng_cr &= ~(0x3u << RNG_CR_CONFIG2_SHIFT);
    rng_cr &= ~(0x7u << RNG_CR_CONFIG3_SHIFT);
    rng_cr |= 0x0Fu << RNG_CR_CONFIG1_SHIFT;
    rng_cr |= 0x0Du << RNG_CR_CONFIG3_SHIFT;

    RNG_CR = RNG_CR_CONDRST | rng_cr;
    while ((RNG_CR & RNG_CR_CONDRST) == 0u) { }
    RNG_CR = rng_cr | RNG_CR_RNGEN;
    while ((RNG_SR & RNG_SR_DRDY) == 0u) { }
}

static int rng_get_word(uint32_t *out)
{
    uint32_t timeout = 100000u;
    while ((RNG_SR & RNG_SR_DRDY) == 0u) {
        if ((RNG_SR & (RNG_SR_CECS | RNG_SR_SECS | RNG_SR_CEIS | RNG_SR_SEIS))
                != 0u) {
            rng_init();
            timeout = 100000u;
            continue;
        }
        if (--timeout == 0u)
            return -1;
    }
    *out = RNG_DR;
    return 0;
}

/* wolfCrypt custom RNG block generator using STM32H5 TRNG */
int custom_rand_gen_block(unsigned char *output, unsigned int sz)
{
    uint32_t word;
    while (sz >= 4u) {
        if (rng_get_word(&word) != 0)
            return -1;
        output[0] = (unsigned char)word;
        output[1] = (unsigned char)(word >> 8);
        output[2] = (unsigned char)(word >> 16);
        output[3] = (unsigned char)(word >> 24);
        output += 4;
        sz -= 4;
    }
    if (sz > 0u) {
        if (rng_get_word(&word) != 0)
            return -1;
        while (sz-- > 0u) {
            *output++ = (unsigned char)word;
            word >>= 8;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
/* hal_init - platform initialization entry point                             */
/* -------------------------------------------------------------------------- */

void hal_init(void)
{
    uart_init();
    rng_init();
}

/* Custom time function */
unsigned long my_time(unsigned long* timer)
{
    static unsigned long t = 1000;
    if (timer)
        *timer = t;
    return t++;
}
