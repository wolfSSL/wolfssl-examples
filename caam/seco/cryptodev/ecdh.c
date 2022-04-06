/* ecdh.c
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

static void printEccKey(ecc_key* key)
{
    byte pubX[MAX_ECC_BYTES];
    byte pubY[MAX_ECC_BYTES];
    byte pri[MAX_ECC_BYTES];
    word32 qxSz = MAX_ECC_BYTES, qySz = MAX_ECC_BYTES, dSz = MAX_ECC_BYTES, i;

    memset(pubX, 0, qxSz);
    memset(pubY, 0, qySz);
    memset(pri,  0, dSz);
    if (wc_ecc_export_private_raw(key, pubX, &qxSz, pubY, &qySz, pri, &dSz)
        != MP_OKAY) {
        printf("Error exporting private key\n");
        return;
    }
    wc_ecc_export_private_only(key, pri, &dSz);

    printf("Created KEY:\n");
    printf("Public X :");
    for (i = 0; i < qxSz; i++)
        printf("%02X", pubX[i]);
    printf("\n");

    printf("Public Y :");
    for (i = 0; i < qySz; i++)
        printf("%02X", pubY[i]);
    printf("\n");

    printf("Private [%d]:", dSz);
    for (i = 0; i < dSz; i++)
        printf("%02X", pri[i]);
    printf("\n");

    printf("Is Private Encrypted? %s\n", (key->blackKey) ? "YES":"NO");
}


static int createEccKey(WC_RNG* rng, ecc_key* key, int devId)
{
    int ret;

    wc_ecc_init_ex(key, NULL, devId);
    ret  = wc_ecc_make_key(rng, 32, key);
    if (ret != 0) {
        printf("error making key\n");
    }
    else {
        printEccKey(key);
    }

    wc_ecc_set_rng(key, rng);
    return ret;
}


int main(int argc, char** argv)
{
    ecc_key softKey, hwKey;
    byte sharedA[48];
    byte sharedB[48];
    word32 sharedSz = 32;
    word32 i;
    int ret;
    WC_RNG rng;

    wolfSSL_Debugging_ON();
    if (wolfCrypt_Init() != 0) {
        printf("Could not initialize wolfSSL library!\n");
        return -1;
    }

    wc_InitRng(&rng);

    ret = createEccKey(&rng, &hwKey, WOLFSSL_CAAM_DEVID);
    if (ret != 0) {
        printf("failed to create hardware key\n");
    }

    if (ret == 0) {
        ret = createEccKey(&rng, &softKey, 0);
        if (ret != 0) {
            printf("failed to create software key\n");
        }
    }

    if (ret == 0) {
        ret = wc_ecc_shared_secret(&softKey, &hwKey, sharedA, &sharedSz);
        if (ret != 0) {
            printf("failed to create shared secret, ret = %d\n", ret);
        }
        else {
            printf("Shared secret : ");
            for (i = 0; i < sharedSz; i++)
                printf("%02X", sharedA[i]);
            printf("\n");
            fflush(stdout);
        }
    }

    if (ret == 0) {
        XMEMSET(sharedB, 0, 32); sharedSz = 32;
        ret = wc_ecc_shared_secret(&hwKey, &softKey, sharedB, &sharedSz);
        if (ret != 0) {
            printf("failed to create shared secret, ret = %d\n", ret);
        }
        else {
            printf("Shared secret : ");
            for (i = 0; i < sharedSz; i++)
                printf("%02X", sharedB[i]);
            printf("\n");
            fflush(stdout);
        }
    }

    if (ret == 0 && XMEMCMP(sharedA, sharedB, sharedSz) != 0) {
        printf("shared secret missmatch!!\n");
        ret = -1;
    }

    /* treat the encrypted private key like a normal software key and verify
     * that it fails to create the correct shared secret */
    if (ret == 0 && hwKey.blackKey != 0) {
        hwKey.devId = INVALID_DEVID;
        XMEMSET(sharedB, 0, 32); sharedSz = 32;
        ret = wc_ecc_shared_secret(&hwKey, &softKey, sharedB, &sharedSz);
        if (ret != 0) {
            printf("failed to create shared secret, ret = %d\n", ret);
        }
        hwKey.devId = WOLFSSL_CAAM_DEVID;
    }

    if (ret == 0 && XMEMCMP(sharedA, sharedB, sharedSz) == 0) {
        printf("shared secret matched when expected to fail!!\n");
        ret = -1;
    }

    wc_ecc_free(&hwKey);
    wc_ecc_free(&softKey);
    wc_FreeRng(&rng);
    wolfCrypt_Cleanup();
    return ret;
}

