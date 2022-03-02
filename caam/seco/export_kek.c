/* export_kek.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
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

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/port/caam/wolfcaam.h>

#include <stdio.h>

int main(int argc, char** argv)
{
    byte kek[32];
    byte kekSz = 32, i;

    wolfSSL_Debugging_ON();
    if (wolfCrypt_Init() != 0) {
        printf("Could not initialize wolfSSL library!\n");
        return -1;
    }

    XMEMSET(kek, 0, kekSz);
    wc_SECO_ExportKEK(kek, kekSz, 0);
    printf("exported : ");
    for (i = 0; i < kekSz; i++)
        printf("%02X", kek[i]);
    printf("\n");

    wolfCrypt_Cleanup();
    return 0;
}

