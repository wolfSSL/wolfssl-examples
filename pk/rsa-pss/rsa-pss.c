/* rsa-pss.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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

/*
* An implementation of RSA PSS digital signature using wolfSSL
* Usage:
./rsa-pss -s sign.txt
./rsa-pss -v sign.txt
*/

#include <stdio.h>
#include <string.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

static const char* kRsaSignOpt = "-s";
static const char* kRsaPubKey = "./rsa-public.der";
#define MAX_DER_SIZE 2048
#define RSA_KEY_SIZE 2048

int main(int argc, char** argv)
{
/* These examples require RSA and Key Gen */
#if !defined(NO_RSA) && defined(WOLFSSL_KEY_GEN)
    RsaKey* pRsaKey = NULL;
    WC_RNG rng;
    /* PSS requires message to be same as hash digest (SHA256=32) */
    const char* szMessage = "This is the string to be signed";
    unsigned char pSignature[RSA_KEY_SIZE/8];
    unsigned char pDecrypted[RSA_KEY_SIZE/8];
    int ret = 0;
    int sz;
    word32 idx = 0;
    unsigned char derBuf[MAX_DER_SIZE];
    FILE* f;

    if (argc <= 2) {
        printf("Usage:\n");
        printf("\trsa-pss -s sign.txt\n");
        printf("\trsa-pss -v sign.txt\n");
        return 0;
    }

    wolfSSL_Debugging_ON();

    wolfSSL_Init();

    pRsaKey = malloc(sizeof(RsaKey));
    if (!pRsaKey) {
        printf("RSA_generate_key failed with error\n");
        return MEMORY_E;
    }

    ret = wc_InitRsaKey(pRsaKey, NULL);
    if (ret != 0) {
        printf("Init RSA key failed %d\n", ret);
        return ret;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Init RNG failed %d\n", ret);
        wc_FreeRsaKey(pRsaKey);
        return ret;
    }

    ret = wc_RsaSetRNG(pRsaKey, &rng);
    if (ret != 0) {
        printf("Set RSA RNG failed %d\n", ret);
        goto prog_end;
    }

    if (memcmp(argv[1], kRsaSignOpt, 2) == 0) {
        printf("generating RSA key to make a PSS signature\n");

        /* Generate an RSA key pair */
        if (wc_MakeRsaKey(pRsaKey, RSA_KEY_SIZE, WC_RSA_EXPONENT, &rng) != 0) {
            printf("failed to create rsa key\n");
        }

        if (pRsaKey) {
            f = fopen(kRsaPubKey, "wb");
            printf("writing public key to %s\n", kRsaPubKey);
            if (f == NULL) {
                printf("unable to write out public key\n");
            }
            else {
                sz = wc_RsaKeyToPublicDer(pRsaKey, derBuf, sizeof(derBuf));
                if (sz <= 0) {
                    printf("error with rsa to public der %d\n", sz);
                    goto prog_end;
                }
                else {
                    fwrite(derBuf, 1, sz, f);
                }
                fclose(f);
            }
        }
    }
    else {
        f = fopen(kRsaPubKey, "rb");
        printf("reading in RSA key to verify signature\n");
        if (f == NULL) {
            printf("unable to open public key\n");
        }
        else {
            fseek(f, 0, SEEK_END);
            sz = ftell(f);
            if (sz > sizeof(derBuf)) {
                printf("File %s exceeds max size\n", kRsaPubKey);
                fclose(f);
                return BUFFER_E;
            }
            fseek(f, 0, SEEK_SET);
            sz = fread(derBuf, 1, sz, f);
            fclose(f);

            ret = wc_RsaPublicKeyDecode(derBuf, &idx, pRsaKey, sz);
            if (ret < 0) {
                printf("Failed to load public rsa key der buffer %d\n", ret);
                goto prog_end;
            }
        }
    }

    if (memcmp(argv[1], kRsaSignOpt, 2) == 0) {
        f = fopen(argv[2], "wb");
        printf("Creating PSS signature and writing to %s\n", argv[2]);
        if (f == NULL) {
            printf("error opening output file %s\n", argv[2]);
            goto prog_end;
        }

        /* perform digital signature */
        ret = wc_RsaPSS_Sign((byte*)szMessage, XSTRLEN(szMessage)+1,
                pSignature, sizeof(pSignature),
                WC_HASH_TYPE_SHA256, WC_MGF1SHA256, pRsaKey, &rng);
        if (ret <= 0) {
            printf("RSA_private_encrypt failed with error %d\n", ret);
            goto prog_end;
        }
        sz = ret;

        fwrite(pSignature, 1, sz, f);
        fclose(f);
    }
    else {
        byte* pt;

        f = fopen(argv[2], "rb");
        if (f == NULL) {
            printf("unable to open %s\n", argv[2]);
            goto prog_end;
        }

        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        if (sz > sizeof(pSignature)) {
            printf("file is too big (%d bytes)\n", sz);
            fclose(f);
            goto prog_end;
        }
        fseek(f, 0, SEEK_SET);
        sz = fread(pSignature, 1, sz, f);
        fclose(f);

        /* now we will verify the signature
        Start by a RAW decrypt of the signature
        */
        pt = pDecrypted;
        ret = wc_RsaPSS_VerifyInline(pSignature, sz, &pt,
                WC_HASH_TYPE_SHA256, WC_MGF1SHA256, pRsaKey);
        if (ret <= 0) {
            printf("RSA_public_decrypt failed with error %d\n", ret);
            goto prog_end;
        }
        else {
            sz = ret;
            ret = wc_RsaPSS_CheckPadding((byte*)szMessage, XSTRLEN(szMessage)+1,
                pt, sz, WC_HASH_TYPE_SHA256);
            if (ret == 0) {
                printf("RSA PSS verify success\n");
            }
            else {
                printf("RSA PSS Padding check failed! %d\n", ret);
                goto prog_end;
            }
        }
   }

prog_end:

    wc_FreeRsaKey(pRsaKey);

    if (pRsaKey)
        free(pRsaKey);

    wc_FreeRng(&rng);
    wolfSSL_Cleanup();

    return 0;
#else
    (void)kRsaSignOpt;
    (void)kRsaPubKey;

    printf("wolfSSL missing build features.\n");
    printf("Please build using `./configure --enable-rsapss --enable-keygen`\n");
    return -1;
#endif
}
