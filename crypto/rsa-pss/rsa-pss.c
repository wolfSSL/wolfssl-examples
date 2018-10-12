/* rsa-pss.c
 *
 * Copyright (C) 2006-2018 wolfSSL Inc.
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
*/

#include <stdio.h>
#include <string.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/rsa.h>

static const char* kRsaPubKey = "./rsa-public.key";
#define MAX_DER_SIZE 2048
#define RSA_KEY_SIZE 2048

int main(int argc, char** argv)
{
    RsaKey* pRsaKey = NULL;
    WC_RNG rng;
    const char* szMessage = "This is the string to be signed";
    unsigned char pSignature[RSA_KEY_SIZE/8];
    unsigned char pDecrypted[RSA_KEY_SIZE/8];
    int status = 0;
    int sz;
    word32 idx = 0;
    unsigned char derBuf[MAX_DER_SIZE];

    pRsaKey = malloc(sizeof(RsaKey));
    wolfSSL_Debugging_ON();

    wolfSSL_Init();

    wc_InitRsaKey(pRsaKey, NULL);
    wc_InitRng(&rng);
    wc_RsaSetRNG(pRsaKey, &rng);

    if (memcmp(argv[1], "-o", 2) == 0) {
        printf("generating RSA key to make a PSS signature\n");
        /* Generate an RSA key pair */
        if (wc_MakeRsaKey(pRsaKey, RSA_KEY_SIZE, 0x010001, &rng) != 0) {
            printf("failed to create rsa key\n");
        }

        if (pRsaKey)
        {
            FILE* f;
            f = fopen(kRsaPubKey, "wb");
            printf("writing public key to %s\n", kRsaPubKey);
            if (f == NULL) {
                printf("unable to write out private key\n");
            }
            else {
                sz = wc_RsaKeyToDer(pRsaKey, derBuf, sizeof(derBuf));
                if (sz <= 0) {
                    printf("error with rsa load der %d\n", sz);
                }
                fwrite(derBuf, 1, sz, f);
                fclose(f);
            }
        }
    }
    else {
        printf("reading in RSA key to verify signature\n");
        sz = wolfSSL_PemPubKeyToDer(kRsaPubKey, derBuf, sizeof(derBuf));
        wc_RsaPublicKeyDecode(derBuf, &idx, pRsaKey, sz);
    }
    if (!pRsaKey)
    {
        printf("RSA_generate_key failed with error\n");
        goto prog_end;
    }

    if (memcmp(argv[1], "-o", 2) == 0) {
        FILE* f = fopen(argv[2], "wb");
        printf("Creating PSS signature and writing to %s\n", argv[2]);
        if (f == NULL) {
            printf("error opening output file %s\n", argv[2]);
            goto prog_end;
        }

        /* perform digital signature */
        status = wc_RsaPSS_Sign((byte*)szMessage, sizeof(szMessage),
                pSignature, sizeof(pSignature),
                WC_HASH_TYPE_SHA256, WC_MGF1SHA256, pRsaKey, &rng);
        if (status <= 0)
        {
            printf("RSA_private_encrypt failed with error %d\n", status);
            goto prog_end;
        }

        fwrite(pSignature, 1, status, f);
        fclose(f);
    }
    else {
        FILE* f;
        f = fopen(argv[2], "rb");
        if (f == NULL) {
            printf("unable to open %s\n", argv[2]);
            goto prog_end;
        }
        fread(pSignature, 1, sizeof(pSignature), f);
        fclose(f);

        /* now we will verify the signature
        Start by a RAW decrypt of the signature
        */
        byte* pt = pDecrypted;
        status = wc_RsaPSS_VerifyInline(pSignature, sizeof(pSignature), &pt,
                WC_HASH_TYPE_SHA256, WC_MGF1SHA256, pRsaKey);
        if (status <= 0)
        {
            printf("RSA_public_decrypt failed with error %d\n", status);
            goto prog_end;
        }
        else
        {
            printf("RSA PSS verify success\n");
        }
   }

prog_end:

   if (pRsaKey)
       free(pRsaKey);

   wolfSSL_Cleanup();

   return 0;
}

