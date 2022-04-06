/* ecc-sign-verify.c
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
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/port/caam/wolfcaam.h>

#include <stdio.h>

#define MAX_UPDATES 100
#define ECC_P256_KEYSIZE 32

static int createHardwareEccKey(ecc_key* key)
{
    WC_RNG rng;
    int ret;

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Error with RNG init\n");
    }

    if (ret == 0) {
        ret = wc_ecc_init_ex(key, NULL, WOLFSSL_SECO_DEVID);
    }

    if (ret == 0) {
        ret  = wc_ecc_make_key(&rng, ECC_P256_KEYSIZE, key);
        if (ret != 0) {
            printf("error %d making hardware key\n", ret);
        }
    }
    wc_FreeRng(&rng);
    return ret;
}


/* create signature, return 0 on success */
static int createSignature(ecc_key* key, byte* sigOut, word32* sigOutSz,
    byte* msg, word32 msgSz)
{
    int ret;
    byte digest[WC_SHA256_DIGEST_SIZE];
    byte digestSz = WC_SHA256_DIGEST_SIZE;
    WC_RNG rng;

    ret = wc_InitRng(&rng);
    if (ret == 0)
        ret = wc_Hash(WC_HASH_TYPE_SHA256, msg, msgSz, digest, digestSz);

    if (ret == 0)
        ret = wc_ecc_sign_hash(digest, digestSz, sigOut, sigOutSz, &rng, key);

    wc_FreeRng(&rng);
    return ret;
}


static int exportPubKey(ecc_key* from, ecc_key* to, int devId)
{
    int ret;
    byte pub[1024];
    word32 pubSz = 1024;

    wc_ecc_init_ex(to, NULL, devId);

    ret = wc_ecc_export_x963(from, pub, &pubSz);
    if (ret == 0) {
        ret = wc_ecc_import_x963(pub, pubSz, to);
        if (ret != 0) {
            printf("issue %d importing public key\n", ret);
        }
    }
    else {
        printf("issue %d exporting public key\n", ret);
    }
    return ret;
}


static int verifySignature(ecc_key* key, byte* sig, word32 sigSz,
    byte* msg, word32 msgSz)
{
    int ret;
    byte digest[WC_SHA256_DIGEST_SIZE];
    byte digestSz = WC_SHA256_DIGEST_SIZE;
    int res = 0;

    ret = wc_Hash(WC_HASH_TYPE_SHA256, msg, msgSz, digest, digestSz);
    if (ret == 0)
        ret = wc_ecc_verify_hash(sig, sigSz, digest, digestSz, &res, key);

    if (ret == 0 && res == 1) {
        printf("verify successful\n");
    }
    else {
        printf("verify failed\n");
    }
    return ret;
}


int main(int argc, char** argv)
{
    ecc_key softKey, hardKey;
    byte sig[1024];
    byte msg[] = "Test signing and verifying";
    word32 msgSz;
    word32 sigSz = 1024, i;
    word32 nonce = 0x1111;
    int create   = 0;
    int ret;
    unsigned int keyId;
    unsigned int keyStoreId;

    msgSz = (word32)XSTRLEN((char*)msg);
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

    XMEMSET(sig, 0, sigSz);
    ret = createHardwareEccKey(&hardKey);
    if (ret == 0)
        ret = createSignature(&hardKey, sig, &sigSz, msg, msgSz);

    if (ret == 0) {
        printf("signature created from hardware key:\n\t");
        for (i = 0; i < sigSz; i++)
            printf("%02X", sig[i]);
        printf("\n");

        exportPubKey(&hardKey, &softKey, INVALID_DEVID);
    }

    if (ret == 0)
        ret = verifySignature(&softKey, sig, sigSz, msg, msgSz);

    if (ret == 0) {
        printf("\nverify signature using HSM key\n");
        ret = verifySignature(&hardKey, sig, sigSz, msg, msgSz);
    }

    if (ret == 0) {
        printf("\nalter the signature and confirm verification fails\n");
        sig[4] = !sig[4];
        verifySignature(&softKey, sig, sigSz, msg, msgSz);
    }

    wc_ecc_free(&softKey);
    wc_ecc_free(&hardKey);

    if (wc_SECO_CloseHSM() != 0) {
        printf("Error closing down the key store\n");
    }
    wolfCrypt_Cleanup();
    (void)keyId;
    return 0;
}

