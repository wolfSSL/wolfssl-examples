/* rsa-kg.c
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
* An implementation of RSA key generation using wolfSSL
* Usage:
./rsa-kg -priv <private key filename> -pub <public key filename>
*/

#include <stdio.h>
#include <string.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define MAX_DER_SIZE        2500
#define MIN_RSA_KEY_SIZE    1024
#define MAX_RSA_KEY_SIZE    4096
#define DEF_RSA_KEY_SIZE    2048

static const char* kRsaPubKey = "./rsa-public.der";
static const char* kRsaPrivKey = "./rsa-private.der";

/* Shows usage information */
void usage()
{
    fprintf(stderr, "rsa_kg <options>:\n");
    fprintf(stderr, "  -bits <num>       Size in bits of RSA keys generated\n");
    fprintf(stderr, "                    Range: 1024-4096\n");
    fprintf(stderr, "  -priv <filename>  Private key filename\n");
    fprintf(stderr, "  -pub <filename>   Public key filename\n");
    fprintf(stderr, "\n");
}

int main(int argc, char** argv)
{
/* These examples require RSA and Key Gen */
#if !defined(NO_RSA) && defined(WOLFSSL_KEY_GEN)
    RsaKey* pRsaKey = NULL;
    WC_RNG rng;
    int ret = 0;
    int bits = DEF_RSA_KEY_SIZE;
    int sz;
    unsigned char derBuf[MAX_DER_SIZE];
    FILE* f;
    const char* pubKey = kRsaPubKey;
    const char* privKey = kRsaPrivKey;

    argc--;
    argv++;
    while (argc > 0) {
        /* Number of bits in RSA key to generate */
        if (XSTRNCMP(*argv, "-bits", 6) == 0) {
            ++argv;
            if (--argc == 0) {
                fprintf(stderr, "Missing bits value\n");
                usage();
                return 1;
            }
            bits = atoi(*argv);
        }
        else if (XSTRNCMP(*argv, "-priv", 6) == 0) {
            ++argv;
            if (--argc == 0) {
                fprintf(stderr, "Missing private key filename\n");
                usage();
                return 1;
            }
            privKey = *argv;
        }
        else if (XSTRNCMP(*argv, "-pub", 6) == 0) {
            ++argv;
            if (--argc == 0) {
                fprintf(stderr, "Missing public key filename\n");
                usage();
                return 1;
            }
            pubKey = *argv;
        }
        else if (XSTRNCMP(*argv, "-help", 6) == 0) {
            usage();
            return 0;
        }
        else {
            fprintf(stderr, "Unrecognized option: %s\n", *argv);
            usage();
            return 1;
        }

        argc--;
        argv++;
    }

    /* Check bit count if generating keys */
    if ((bits < MIN_RSA_KEY_SIZE || bits > MAX_RSA_KEY_SIZE)) {
        fprintf(stderr, "Bits out of range (%d-%d): %d\n", MIN_RSA_KEY_SIZE,
                MAX_RSA_KEY_SIZE, bits);
        usage();
        return 1;
    }
#ifdef WOLFSSL_SP_MATH
    if (0) {
    }
#ifndef WOLFSSL_SP_NO_2048
    else if (bits == 2048) {
    }
#endif
#ifndef WOLFSSL_SP_NO_3072
    else if (bits == 3072) {
    }
#endif
#ifdef WOLFSSL_SP_4096
    else if (bits == 4096) {
    }
#endif
    else {
        fprintf(stderr, "Bit size not supported with SP_MATH: %d\n", bits);
        fprintf(stderr, " wolfSSL compiled to support, in bits:");
#ifndef WOLFSSL_SP_NO_2048
        fprintf(stderr, " 2048");
#endif
#ifndef WOLFSSL_SP_NO_3072
        fprintf(stderr, " 3072");
#endif
#ifdef WOLFSSL_SP_4096
        fprintf(stderr, " 4096");
#endif
        fprintf(stderr, "\n");
        return 1;
    }
#endif

    wolfSSL_Debugging_ON();

    wolfSSL_Init();

    /* Allocate space for RSA key object. */
    pRsaKey = malloc(sizeof(RsaKey));
    if (!pRsaKey) {
        printf("RSA_generate_key failed with error\n");
        return MEMORY_E;
    }

    /* Create a random number generator for key generation. */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Init RNG failed %d\n", ret);
        free(pRsaKey);
        return ret;
    }

    /* Initialize RSA key object. */
    ret = wc_InitRsaKey(pRsaKey, NULL);
    if (ret != 0) {
        printf("Init RSA key failed %d\n", ret);
        wc_FreeRng(&rng);
        free(pRsaKey);
        return ret;
    }

    printf("Generating RSA key\n");

    /* Generate an RSA key pair. */
    if (wc_MakeRsaKey(pRsaKey, bits, WC_RSA_EXPONENT, &rng) != 0) {
        printf("failed to create rsa key\n");
    }
    else {
        /* Open public key file. */
        f = fopen(pubKey, "wb");
        printf("writing public key to %s\n", pubKey);
        if (f == NULL) {
            printf("unable to write out public key\n");
        }
        else {
            /* Encode public key to DER. */
            sz = wc_RsaKeyToPublicDer(pRsaKey, derBuf, sizeof(derBuf));
            if (sz <= 0) {
                printf("error with rsa to public der %d\n", sz);
            }
            else {
                /* Write DER encoded public key to file. */
                fwrite(derBuf, 1, sz, f);
            }
            fclose(f);
        }

        /* Open private key file. */
        f = fopen(privKey, "wb");
        printf("writing public key to %s\n", privKey);
        if (f == NULL) {
            printf("unable to write out public key\n");
        }
        else {
            /* Encode private key to DER. */
            sz = wc_RsaKeyToDer(pRsaKey, derBuf, sizeof(derBuf));
            if (sz <= 0) {
                printf("error with rsa to public der %d\n", sz);
            }
            else {
                /* Write DER encoded private key to file. */
                fwrite(derBuf, 1, sz, f);
            }
            fclose(f);
        }
    }

    /* Dispose of allocated memory. */
    wc_FreeRsaKey(pRsaKey);
    wc_FreeRng(&rng);
    free(pRsaKey);
    wolfSSL_Cleanup();

    return 0;
#else
    (void)kRsaPubKey;
    (void)kRsaPrivKey;

    printf("wolfSSL missing build features.\n");
    printf("Please build using `./configure --enable-keygen`\n");
    return 1;
#endif
}
