/* import-key.c
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

static unsigned int ImportAesKey(const byte* key, int keySz)
{
    int keyGroup = 1; /* group one was chosen arbitrarily */
    unsigned int keyIdOut;
    byte iv[] = {1,2,3,4,5,6,7,8,9,10,11,12};
    int ivSz  = 12;

    /* generate a CMAC key in the HSM */
    keyIdOut = wc_SECO_WrapKey(0, (byte*)key, keySz, iv, ivSz,
        CAAM_KEYTYPE_AES128, CAAM_KEY_TRANSIENT, keyGroup);
    return keyIdOut;
}


/* known answer test from wolfcrypt/test/test.c */
int TestAesCbc(Aes* enc, Aes* dec)
{
    int i;
    int ret = 0;
    const byte msg[] = { /* "Now is the time for all " w/o trailing 0 */
        0x6e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,
        0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
        0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20
    };

    WOLFSSL_SMALL_STACK_STATIC const byte verify[] =
    {
        0x95,0x94,0x92,0x57,0x5f,0x42,0x81,0x53,
        0x2c,0xcc,0x9d,0x46,0x77,0xa2,0x33,0xcb
    };

    const byte iv[]  = "1234567890abcdef   ";  /* align */
    byte cipher[AES_BLOCK_SIZE * 4];
    byte plain[AES_BLOCK_SIZE * 4];

    if (enc == NULL || dec == NULL) {
        printf("null argument\n");
        return -1;
    }

    XMEMSET(cipher, 0, AES_BLOCK_SIZE * 4);
    ret = wc_AesSetIV(enc, iv);
    if (ret != 0) {
        printf("Aes CBC set iv failed with ret = %d\n", ret);
        return ret;
    }

    ret = wc_AesCbcEncrypt(enc, cipher, msg, AES_BLOCK_SIZE);
    if (ret != 0) {
        printf("Aes CBC encrypt failed with ret = %d\n", ret);
        return ret;
    }

    printf("cipher :");
    for (i = 0; i < AES_BLOCK_SIZE; i++)
        printf("%02X", cipher[i]);
    printf("\n");

    XMEMSET(plain, 0, AES_BLOCK_SIZE * 4);
    ret = wc_AesSetIV(dec, iv);
    if (ret != 0) {
        printf("Aes CBC set iv failed with ret = %d\n", ret);
        return ret;
    }

    ret = wc_AesCbcDecrypt(dec, plain, cipher, AES_BLOCK_SIZE);
    if (ret != 0) {
        printf("Aes CBC decrypt failed with ret = %d\n", ret);
        return ret;
    }

    printf("decrypted :");
    for (i = 0; i < AES_BLOCK_SIZE; i++)
        printf("%02X", plain[i]);
    printf("\n");

    if (XMEMCMP(plain, msg, AES_BLOCK_SIZE)) {
        printf("bad decryption found plain != msg\n");
        ret = -1;
    }

    if (XMEMCMP(cipher, verify, AES_BLOCK_SIZE)) {
        printf("bad encrypt found cipher != verify\n");
        ret = -1;
    }

    return ret;
}


int main(int argc, char** argv)
{
    Aes enc, dec;
    word32 nonce = 0x00001111;
    unsigned int keyId;
    unsigned int keyStoreId;
    const byte key[] = "0123456789abcdef   ";  /* align */

    if (argc == 2) {
        keyStoreId = (unsigned int)XATOI(argv[1]);
    }
    else {
        printf("USAGE: %s <key store id>\n", argv[0]);
        return -1;
    }

    wolfSSL_Debugging_ON();
    if (wolfCrypt_Init() != 0) {
        printf("Could not initialize wolfSSL library!\n");
        return -1;
    }

    if (wc_SECO_OpenHSM(keyStoreId, nonce, MAX_UPDATES, CAAM_KEYSTORE_CREATE)
            != 0) {
        printf("unable to open HSM\n");
        wolfCrypt_Cleanup();
        return -1;
    }

    keyId = ImportAesKey(key, 16);
    printf("Key ID: %u\n", keyId);

    wc_AesInit(&enc, NULL, WOLFSSL_SECO_DEVID);
    wc_SECO_AesSetKeyID(&enc, keyId);

    wc_AesInit(&dec, NULL, WOLFSSL_SECO_DEVID);
    wc_SECO_AesSetKeyID(&dec, keyId);

    TestAesCbc(&enc, &dec);

    wc_SECO_CloseHSM();
    wolfCrypt_Cleanup();
    return 0;
}

