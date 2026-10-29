/* main.c
 *
 * wolfCrypt test or benchmark on the PSOC Edge E8 CM33.
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

#include <stdio.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfcrypt/test/test.h>
#include <wolfcrypt/benchmark/benchmark.h>

#include "board.h"

int main(void)
{
    int ret;

    board_init();
    printf("\nwolfCrypt on PSOC Edge E8 (CM33), crypto engine %s\n",
#ifdef WOLFSSL_PSOC6_CRYPTO
        "enabled"
#else
        "disabled"
#endif
        );

    ret = wolfCrypt_Init();
    if (ret == 0) {
#ifdef BENCH_APP
        ret = benchmark_test(NULL);
#else
        ret = wolfcrypt_test(NULL);
#endif
        wolfCrypt_Cleanup();
    }
    printf("%s: %d\n", (ret == 0) ? "Done" : "Failed", ret);
    return ret;
}
