/* rsa.c
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
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/port/caam/wolfcaam.h>

#include <stdio.h>

#define KEY_SIZE 3072

static int RsaGen(RsaKey* key, int size, long e, int devId)
{
    int    ret;
    WC_RNG rng;

    ret = wc_InitRng(&rng);
    if (ret == 0) {
        ret = wc_InitRsaKey_ex(key, NULL, devId);
        if (ret == 0)
            ret = wc_MakeRsaKey(key, size, e, &rng);

        wc_FreeRng(&rng); /* always free rng if initialized */
    }

    return ret;
}


static int RsaSign(RsaKey* key, byte* in, int inSz, byte* out, int outSz)
{
    int    ret;
    WC_RNG rng;

    ret = wc_InitRng(&rng);
    if (ret == 0)
        ret = wc_RsaSSL_Sign(in, inSz, out, outSz, key, &rng);
    wc_FreeRng(&rng);
    return ret;
}


int main(int argc, char** argv)
{
    int ret, i;
    int outSz;
    RsaKey rsaHw, rsaSw;

    byte msg[] = "message to encrypt";
    int  msgSz, resSz;
    byte out[512];
    byte res[512];
    byte der[8192];
    int  derSz = 8192;

    outSz = sizeof(out);
    msgSz = sizeof(msg);

    wolfSSL_Debugging_ON();
    XMEMSET(out, 0, outSz);
    if (wolfCrypt_Init() != 0) {
        printf("Error initializing wolfSSL library\n");
        return -1;
    }

    printf("Buffer to encrypt : ");
    for (i = 0; i < msgSz; i++)
        printf("%02X", msg[i]);
    printf("\n");

    /* create hardware key with encrypted private 'd' value */
    printf("Creating key with hardware support\n");
    ret = RsaGen(&rsaHw, KEY_SIZE, WC_RSA_EXPONENT, WOLFSSL_CAAM_DEVID);
    if (ret != 0) {
        printf("Issue with key generation\n");
    }

    /* create a key using software implementation */
    printf("Creating key with software implementation\n");
    ret = RsaGen(&rsaSw, KEY_SIZE, WC_RSA_EXPONENT, INVALID_DEVID);
    if (ret != 0) {
        printf("Issue with key generation\n");
    }

    /* for sign/verify sanity check importing the hardware key components */
    wc_FreeRsaKey(&rsaSw);
    wc_InitRsaKey_ex(&rsaSw, NULL, INVALID_DEVID);
    ret = wc_RsaKeyToDer(&rsaHw, der, derSz);
    if (ret <= 0) {
        printf("failed to get DER of key : %d\n", ret);
        ret = -1;
    }
    else {
        word32 idx = 0;
        derSz = ret;
        ret   = wc_RsaPrivateKeyDecode(der, &idx, &rsaSw, derSz);
    }

    /* sign and verify using hardware key with encrypted private 'd' value */
    if (ret == 0) {
        outSz = RsaSign(&rsaHw, msg, msgSz, out, outSz);
        if (outSz > 0) {
            printf("Result of RSA encrypt [%d]: ", outSz);
            for (i = 0; i < outSz; i++)
                printf("%02X", out[i]);
            printf("\n");

            resSz = wc_RsaSSL_Verify(out, outSz, res, outSz, &rsaHw);
            if (resSz <= 0) {
            	printf("RSA decrypt failed\n");
            }
            else {
                printf("Result of RSA decrypt: ");
                for (i = 0; i < resSz; i++)
                    printf("%02X", res[i]);
                printf("\n");
            }

            /* verify with software key should work too (only private encrypted) */
            resSz = wc_RsaSSL_Verify(out, outSz, res, outSz, &rsaSw);
            if (resSz <= 0) {
            	printf("RSA decrypt failed with software key\n");
            }
            else {
                printf("Result of RSA decrypt with software key: ");
                for (i = 0; i < resSz; i++)
                    printf("%02X", res[i]);
                printf("\n");
            }

        }
        else {
            printf("RsaSign failed\n");
        }
    }

    /* sign with black key used without decryption should fail */
    if (ret == 0) {
        outSz = RsaSign(&rsaSw, msg, msgSz, out, outSz);
        if (outSz > 0) {
            printf("Result of RSA encrypt [%d]: ", outSz);
            for (i = 0; i < outSz; i++)
                printf("%02X", out[i]);
            printf("\n");

            resSz = wc_RsaSSL_Verify(out, outSz, res, outSz, &rsaSw);
            if (resSz > 0) {
            	printf("RSA decrypt should fail!!\n");
            }
            else {
                printf("Succesful fail test case\n");
            }
        }
        else {
            printf("RsaSign failed\n");
        }
    }
    wc_FreeRsaKey(&rsaHw);
    wc_FreeRsaKey(&rsaSw);
    wolfCrypt_Cleanup();
    return 0;
}
