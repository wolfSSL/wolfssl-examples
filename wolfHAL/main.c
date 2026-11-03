/* main.c
 *
 * wolfCrypt + wolfHAL example — runs the wolfCrypt test suite using
 * hardware-accelerated crypto via wolfHAL.
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

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfcrypt/test/test.h>
#include <wolfcrypt/benchmark/benchmark.h>

#include "wolfHAL_board.h"

double current_time(int reset)
{
    uint32_t timeMs = Board_GetTick();
    double timeNow;

    (void)reset;

    timeNow  = (double)(timeMs / 1000);           /* seconds */
    timeNow += (double)(timeMs % 1000) / 1000.0;  /* milliseconds */

    return timeNow;
}

void main(void)
{
    int ret;

    /* Initialize board hardware (clocks, peripherals) */
    if (Board_Init() != 0) {
        while (1);
    }

    /* Registers the wolfHAL crypto callback via wc_wolfHAL_RegisterDevice(),
     * so the peripherals Board_Init() brought up are already live. */
    ret = wolfCrypt_Init();
    if (ret != 0) {
        while (1);
    }

    /* Correctness first, then throughput */
    wolfcrypt_test(NULL);
    benchmark_test(NULL);

    wolfCrypt_Cleanup();
    Board_Deinit();

    while (1);
}
