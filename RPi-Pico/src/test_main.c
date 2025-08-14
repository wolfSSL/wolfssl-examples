/* bench_main.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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

#include <stdint.h>

#include "wolfssl/wolfcrypt/settings.h"
#include <wolfssl/ssl.h>
#include <wolfcrypt/test/test.h>

#include <stdio.h>
#include "pico/stdlib.h"

int main(int argc, char **argv)
{
    int i;
    int ret;

    stdio_init_all();

    wolfSSL_Init();
    wolfSSL_Debugging_ON();

    ret = wolfcrypt_test(NULL);

    printf("End: %d\n", ret);
    return ret;
}

#include <time.h>
time_t myTime(time_t *t)
{
    *t = (((2023 - 1970) * 12 + 8) * 30 * 24 * 60 * 60);
    return *t;
}
