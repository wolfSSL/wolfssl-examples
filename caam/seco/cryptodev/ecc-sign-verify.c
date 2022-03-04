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
#include <wolfssl/wolfcrypt/signature.h>
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

    /* if the key was encrypted it would be given an ID or blackKey flag set */
    printf("Is Private Encrypted? %s\n", (key->blackKey) ? "YES":"NO");
}


/* create an ECC key using the given devId
 * return 0 on success */
static int createEccKey(WC_RNG* rng, ecc_key* key, int sz, int devId)
{
    int ret;

    wc_ecc_init_ex(key, NULL, devId);
    ret  = wc_ecc_make_key(rng, sz, key);
    if (ret != 0) {
        printf("error making key\n");
    }
    else {
        printEccKey(key);
    }

    wc_ecc_set_rng(key, rng);
    return ret;
}


/* create signature
 * return 0 on success */
static int createSignature(ecc_key* key, byte* sigOut, word32* sigOutSz,
    byte* msg, word32 msgSz)
{
    int ret;
    WC_RNG rng;

    wc_InitRng(&rng);
    ret = wc_ecc_sign_hash(msg, msgSz, sigOut, sigOutSz, &rng, key);
    if (ret != 0) {
        printf("sign hash failed with error %d\n", ret);
    }
    wc_FreeRng(&rng);
    return ret;
}


/* export the public key from 'from' and import it into key 'to' setting 'to'
 * with the devId provided
 * return 0 on success */
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


/* attempt to verify the signature, prints success or failure */
static int verifySignature(ecc_key* key, byte* sig, word32 sigSz,
    byte* msg, word32 msgSz)
{
    int ret;
    int stat = 0;

    ret = wc_ecc_verify_hash(sig, sigSz, msg, msgSz, &stat, key);
    printf("ret = %d stat = %d\n", ret, stat);
    if (ret == 0 && stat == 1) {
        printf("verification success\n");
    }
    else {
        printf("verification fail [%d]\n", ret);
    }

    return ret;
}


int main(int argc, char** argv)
{
    int ret;
    ecc_key softKey, hardKey;
    byte sig[1024];
    byte msg[] = "Test signing and verifying";
    word32 msgSz;
    word32 sigSz = 1024, i;
    WC_RNG rng;

    msgSz = (word32)XSTRLEN((char*)msg);
    wolfSSL_Debugging_ON();
    if (wolfCrypt_Init() != 0) {
        printf("Could not initialize wolfSSL library!\n");
        return -1;
    }
    wc_InitRng(&rng);

    XMEMSET(sig, 0, sigSz);
    ret = createEccKey(&rng, &hardKey, 32, WOLFSSL_CAAM_DEVID);
    if (ret == 0)
        ret = createSignature(&hardKey, sig, &sigSz, msg, msgSz);

    if (ret == 0) {
        printf("signature created from hardware key:\n\t");
        for (i = 0; i < sigSz; i++)
            printf("%02X", sig[i]);
        printf("\n");

        /* use INVALID_DEVID to signal is a software ecc_key structure */
        ret = exportPubKey(&hardKey, &softKey, INVALID_DEVID);
    }

    if (ret == 0) {
        verifySignature(&softKey, sig, sigSz, msg, msgSz);

        printf("\nverify signature using HSM key\n");
        verifySignature(&hardKey, sig, sigSz, msg, msgSz);

        printf("\nalter the signature and confirm verification fails\n");
        sig[4] = !sig[4];
        verifySignature(&softKey, sig, sigSz, msg, msgSz);
    }

    wc_ecc_free(&softKey);
    wc_ecc_free(&hardKey);

    wc_FreeRng(&rng);
    wolfCrypt_Cleanup();
    return 0;
}

