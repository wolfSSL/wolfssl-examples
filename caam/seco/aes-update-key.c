/* aes-update-key.c
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

#define MAX_UPDATES 100

static unsigned int generateAesKey(int flag, unsigned int* keyIdOut,
    int keyGroup)
{
    int keyInfo = CAAM_KEY_TRANSIENT;
    int keyType = CAAM_KEYTYPE_AES128;
    byte pubOut[32];

    /* generate or update a key in the HSM */
    if (wc_SECO_GenerateKey(flag, keyGroup, pubOut, 0, keyType,
        keyInfo, keyIdOut) != 0) {
        printf("Error generating key in hsm\n");
        return -1;
    }
    return *keyIdOut;
}


static int doCcb(unsigned int keyId, const byte* in, int inSz, const byte* iv,
        int ivSz)
{
    Aes aes;
    byte out[AES_BLOCK_SIZE*2];
    byte cipherTxt[AES_BLOCK_SIZE*2];
    int i;

    XMEMSET(cipherTxt, 0, sizeof(cipherTxt));
    wc_AesInit(&aes, NULL, WOLFSSL_SECO_DEVID);
    wc_AesSetIV(&aes, iv);
    wc_SECO_AesSetKeyID(&aes, keyId);

    printf("in = %p\n", in);
    printf("out = %p\n", out);
    printf("cipherTxt = %p\n", cipherTxt);
    printf("Encrypting : ");
    for (i = 0; i < inSz; i++)
        printf("%02X", in[i]);
    printf("\n");

    if (wc_AesCbcEncrypt(&aes, cipherTxt, in, inSz) != 0) {
        printf("Issue with ccb encrypt\n");
    }

    printf("Cipher text: ");
    for (i = 0; i < inSz; i++)
        printf("%02X", cipherTxt[i]);
    printf("\n");

    wc_AesCbcDecrypt(&aes, out, cipherTxt, inSz);

    printf("Decrypted : ");
    for (i = 0; i < inSz; i++)
        printf("%02X", out[i]);
    printf("\n");

    wc_AesFree(&aes);
    return 0;
}


int main(int argc, char** argv)
{
    int group = 1; /* group one was chosen arbitrarily */
    word32 nonce = 0x1111;
    int create   = 0;
    unsigned int keyId;
    unsigned int keyStoreId;
    const byte in[] = "test message to encrypt";
    const byte iv[] = {0,1,2,3,4,5,6,7,8,9,10,11,12};

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

    if (wc_SECO_OpenHSM(keyStoreId, nonce, MAX_UPDATES, create) != 0) {
        printf("unable to open HSM\n");
        wolfCrypt_Cleanup();
        return -1;
    }

    if (create) {
        keyId = generateAesKey(CAAM_GENERATE_KEY, &keyId, group);
    }
    printf("Key ID: %u\n", keyId);

    doCcb(keyId, in, AES_BLOCK_SIZE, iv, sizeof(iv));

    printf("\nNow update the key and do the operation again.\n");
    printf("The encrypted results should be different with new key.\n");
    keyId = generateAesKey(CAAM_UPDATE_KEY, &keyId, 0);
    printf("Key ID after update : %u (should be the same ID still)\n", keyId);
    doCcb(keyId, in, AES_BLOCK_SIZE, iv, sizeof(iv));

    wc_SECO_CloseHSM();
    wolfCrypt_Cleanup();
    return 0;
}

