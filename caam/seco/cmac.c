/* cmac.c
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
#include <wolfssl/wolfcrypt/port/caam/wolfcaam_cmac.h>

#include <stdio.h>

#define MAX_UPDATES 100

static int doCmac(unsigned int keyId, int create, const byte* in, int inSz,
    byte* out, word32* outSz)
{
    Cmac cmac;
    int keyGroup = 1; /* group one was chosen arbitrarily */
    unsigned int keyIdOut;
    int keyInfo = CAAM_KEY_TRANSIENT;
    int keyType = CAAM_KEYTYPE_AES128;

    byte pubKey[32];

    if (create) {
        /* generate a CMAC key in the HSM */
        if (wc_SECO_GenerateKey(CAAM_GENERATE_KEY, keyGroup, pubKey, 0, keyType,
            keyInfo, &keyIdOut) != 0) {
            printf("Error generating key in hsm\n");
            return -1;
        }
    }
    else {
        keyIdOut = keyId;
    }
    printf("Using key ID %u\n", keyIdOut);
    wc_InitCmac_ex(&cmac, NULL, 0, WC_CMAC_AES, NULL, NULL, WOLFSSL_SECO_DEVID);
    wc_SECO_CMACSetKeyID(&cmac, keyIdOut);

    if (wc_CmacUpdate(&cmac, in, inSz) != 0) {
        printf("CMAC update failed\n");
    }
    else {
        if (wc_CmacFinal(&cmac, out, outSz) != 0) {
            printf("CMAC final failed\n");
        }
    }
    return 0;
}

int main(int argc, char** argv)
{
    word32 nonce = 0x101;
    int create   = 0;
    unsigned int keyId;
    unsigned int keyStoreId;
    const byte in[] = "test message to mac";
    int inSz;
    byte out[AES_BLOCK_SIZE];
    word32 outSz = AES_BLOCK_SIZE, i;

    inSz  = (int)XSTRLEN((char*)in);
    outSz = (word32)sizeof(out);
    if (argc == 4) {
        if (XSTRNCMP(argv[1], "1", 1) == 0) {
            create = CAAM_KEYSTORE_CREATE;
        }
        else {
            printf("This example uses a transient key which gets deleted once"
                   "HSM is closed down\n");
            return -1;
        }

        keyId      = (unsigned int)XATOI(argv[2]);
        keyStoreId = (unsigned int)XATOI(argv[3]);
    }
    else {
        printf("USAGE: %s <1/0 create> <keyid (0 if create)> <key store id>\n",
            argv[0]);
        return -1;
    }

    wolfSSL_Debugging_ON();
    if (wolfCrypt_Init() != 0) {
        printf("Could not initialize wolfSSL library!\n");
        return -1;
    }

    if (create) {
        printf("Creating key store\n");
    }

    if (wc_SECO_OpenHSM(keyStoreId, nonce, MAX_UPDATES, create) != 0) {
        printf("unable to open HSM, key store ID = %u create = %d\n",
            keyStoreId, create);
        wolfCrypt_Cleanup();
        return -1;
    }

    doCmac(keyId, create, in, inSz, out, &outSz);
    printf("mac : ");
    for (i = 0; i < outSz; i++)
        printf("%02X", out[i]);
    printf("\n");

    wc_SECO_CloseHSM();
    wolfCrypt_Cleanup();
    return 0;
}

