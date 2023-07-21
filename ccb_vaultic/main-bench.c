/* wolfCrypt benchmark test application for Wisekey VaultIC
 *
 * Copyright (C) 2023 wolfSSL Inc.
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

#include <stdio.h>

#include "wolfssl/options.h"
#include "wolfssl/ssl.h"
#include "wolfcrypt/benchmark/benchmark.h"

#include "ccb_vaultic.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    int rc=0;

    /* Initialize wolfSSL and wolfCrypt */
    rc=wolfSSL_Init();
    if(rc!=WOLFSSL_SUCCESS) {
        fprintf(stderr, "Failed to initialize wolfSSL: %d\n", rc);
        return(rc);
    }

    /* Register VaultIC as cryptocb */
    rc = wc_CryptoCb_RegisterDevice(CCBVAULTIC420_DEVID,
                          ccbVaultIc_CryptoDevCb, NULL);
    if(rc) {
        fprintf(stderr, "Failed to register cryptocb: %d\n", rc);
        wolfSSL_Cleanup();
        return(rc);
    }

    /* Run benchmarks */
    benchmark_test(NULL);

    wc_CryptoCb_UnRegisterDevice(CCBVAULTIC420_DEVID);
    wolfSSL_Cleanup();

    return 0;


}
