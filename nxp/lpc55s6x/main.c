/* main.c
 *
 * wolfCrypt test and benchmark for the NXP LPC55S6x.
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#include "user_settings.h"

#include <stdarg.h>

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_str.h"
#include "fsl_rng.h"
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfcrypt/test/test.h>
#include <wolfcrypt/benchmark/benchmark.h>

static volatile uint32_t g_tickMs;

/* wolfCrypt's test and benchmark end their lines with a bare \n, which
 * staircases on a serial terminal.  StrFormatPrintf() formats straight into a
 * character callback, so the CR goes in here: no line buffer to size, and no
 * second printf implementation linked in beside the console's own. */
static void lpc_put_cb(char* buf, int32_t* count, char val, int len)
{
    int i;

    (void)buf;

    for (i = 0; i < len; i++) {
        if (val == '\n')
            (void)DbgConsole_Putchar('\r');
        (void)DbgConsole_Putchar(val);
        (*count)++;
    }
}

/* XPRINTF in user_settings.h points wolfCrypt's printf here. */
int wolf_printf(const char* fmt, ...)
{
    va_list ap;
    char unused;
    int ret;

    va_start(ap, fmt);
    ret = StrFormatPrintf(fmt, ap, &unused, lpc_put_cb);
    va_end(ap);

    return ret;
}

void SysTick_Handler(void)
{
    g_tickMs++;
}

/* WOLFSSL_USER_CURRTIME routes the benchmark's timebase here.  SysTick's
 * reload counter is read alongside the millisecond count so that operations
 * finishing inside a single tick -- which HashCrypt regularly does at 150 MHz
 * -- still land on a meaningful number. */
double current_time(int reset)
{
    uint32_t ms, val, load;

    (void)reset;

    do {
        ms  = g_tickMs;
        val = SysTick->VAL;
    } while (ms != g_tickMs);

    load = SysTick->LOAD + 1U;

    return (double)ms / 1000.0 +
           (double)(load - val) / ((double)load * 1000.0);
}

int main(void)
{
    int ret;

    BOARD_InitBootPins();
    BOARD_BootClockPLL150M();
    BOARD_InitDebugConsole();

    SysTick_Config(SystemCoreClock / 1000U);

    /* wc_GenerateSeed() calls RNG_GetRandomData() directly and does not
     * initialize the peripheral, so the ring oscillator has to be powered and
     * the entropy accumulator warmed up here. */
    RNG_Init(RNG);

    wolf_printf("\nwolfCrypt on LPC55S6x @ %u MHz\n",
           (unsigned)(SystemCoreClock / 1000000U));
#ifdef WOLFSSL_NXP_LPC55S6X
    wolf_printf("Hardware acceleration: TRNG, HashCrypt (AES/SHA), CASPER (RSA)\n");
#else
    wolf_printf("Hardware acceleration: disabled (software only)\n");
#endif

    ret = wolfCrypt_Init();
    if (ret != 0) {
        wolf_printf("wolfCrypt_Init failed: %d\n", ret);
        while (1) { }
    }

    wolf_printf("\n--- wolfCrypt test ---\n");
    ret = wolfcrypt_test(NULL);
    wolf_printf("wolfcrypt_test returned %d\n", ret);

    wolf_printf("\n--- wolfCrypt benchmark ---\n");
    ret = benchmark_test(NULL);
    wolf_printf("benchmark_test returned %d\n", ret);

    wolfCrypt_Cleanup();

    wolf_printf("\nDone.\n");
    while (1) { }
}
