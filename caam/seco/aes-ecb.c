/* aes-ecb.c
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

#ifdef HAVE_AES_ECB

#define MAX_UPDATES 100

static unsigned int createAesKey()
{
    int keyGroup = 1; /* group one was chosen arbitrarily */
    unsigned int keyIdOut;
    int keyInfo = CAAM_KEY_PERSISTENT;
    int keyType = CAAM_KEYTYPE_AES128;
    byte pubOut[AES_MAX_KEY_SIZE];

    /* generate a AES-ECB key in the HSM */
    if (wc_SECO_GenerateKey(CAAM_GENERATE_KEY, keyGroup, pubOut, 0, keyType,
        keyInfo, &keyIdOut) != 0) {
        printf("Error generating key in hsm\n");
        return -1;
    }
    return keyIdOut;
}


static int doEcb(unsigned int keyId, const byte* in, int inSz)
{
    Aes aes;
    byte out[AES_BLOCK_SIZE*2];
    byte cipherTxt[AES_BLOCK_SIZE*2];
    int i;

    XMEMSET(cipherTxt, 0, sizeof(cipherTxt));
    wc_AesInit(&aes, NULL, WOLFSSL_SECO_DEVID);
    wc_SECO_AesSetKeyID(&aes, keyId);

    printf("in = %p\n", in);
    printf("out = %p\n", out);
    printf("cipherTxt = %p\n", cipherTxt);
    printf("Encrypting : ");
    for (i = 0; i < inSz; i++)
        printf("%02X", in[i]);
    printf("\n");

    if (wc_AesEcbEncrypt(&aes, cipherTxt, in, inSz) != 0) {
        printf("Issue with ecb encrypt\n");
    }

    printf("Cipher text: ");
    for (i = 0; i < inSz; i++)
        printf("%02X", cipherTxt[i]);
    printf("\n");

    wc_AesEcbDecrypt(&aes, out, cipherTxt, inSz);

    printf("Decrypted : ");
    for (i = 0; i < inSz; i++)
        printf("%02X", out[i]);
    printf("\n");

    wc_AesFree(&aes);
    return 0;
}


int main(int argc, char** argv)
{
    word32 nonce = 0x1111;
    int create   = 0;
    unsigned int keyId;
    unsigned int keyStoreId;
    const byte in[] = "test message to encrypt";

    if (argc == 4) {
        if (XSTRNCMP(argv[1], "1", 1) == 0) {
            create = CAAM_KEYSTORE_CREATE;
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
        keyId = createAesKey();
    }
    printf("Key ID: %u\n", keyId);

    doEcb(keyId, in, AES_BLOCK_SIZE);

    wc_SECO_CloseHSM();
    wolfCrypt_Cleanup();
    return 0;
}
#else
#warning compile wolfSSL with HAVE_AES_ECB for aes-ecb
int main(int argc, char** argv)
{
    printf("Compile wolfSSL with HAVE_AES_ECB for aes-ecb\n");
    return 0;
}
#endif
