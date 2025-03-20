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
*   ./rsa-pss -s sign.bin
*   ./rsa-pss -v sign.bin
*/

#include <stdio.h>
#include <string.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>


/* Maximum size of buffer to hold DER encodings. */
#define MAX_DER_SIZE 2048
/* Key size in bits. */
#define RSA_KEY_SIZE 2048


static const char* kRsaSignOpt = "-s";
static const char* kRsaVerifyOpt = "-v";
static const char* kRsaPubKey = "./rsa-public.der";


/* These examples require RSA and Key Gen */
#if !defined(NO_RSA) && defined(WOLFSSL_KEY_GEN)
void print_wolfssl_error(const char* msg, int err)
{
#ifndef NO_ERROR_STRINGS
    printf("%s: %s (%d)\n", msg, wc_GetErrorString(err), err);
#else
    printf("%s: %d\n", msg, err);
#endif
}

static int write_file(const char* filename, unsigned char* data, int sz)
{
    FILE* f;
    int err = 1;

    printf("  Writing to file: %s\n", filename);

    /* Open file to put public RSA key into. */
    f = fopen(filename, "wb");
    if (f == NULL) {
        printf("      unable to write to file\n");
        goto write_file_end;
    }

    /* Write out DER encoding of RSA public key. */
    fwrite(data, 1, sz, f);
    fclose(f);

    err = 0;
write_file_end:
    return err;
}

static int read_file(const char* filename, unsigned char* data, int* sz)
{
    FILE* f = NULL;
    int err = 1;
    int fileSz;

    printf("  Reading from file: %s\n", filename);

    /* Open file with RSA public key. */
    f = fopen(filename, "rb");
    if (f == NULL) {
        printf("    unable to open public key\n");
        goto load_end;
    }

    /* Get length of file. */
    fseek(f, 0, SEEK_END);
    fileSz = ftell(f);
    /* Check buffer is big enough. */
    if (fileSz > *sz) {
        printf("    File %s exceeds max size: %d > %d\n", filename, fileSz,
            *sz);
        goto load_end;
    }
    /* Go back to start of file. */
    fseek(f, 0, SEEK_SET);

    /* Read in all of file. */
    fileSz = fread(data, 1, fileSz, f);

    *sz = fileSz;
    err = 0;
load_end:
    fclose(f);
    return err;
}

static int generate_rsa_key(RsaKey* pRsaKey, WC_RNG* rng)
{
    int ret;
    int err = 1;

    printf("Generating RSA key to make a PSS signature\n");
    /* Generate an RSA key pair */
    ret = wc_MakeRsaKey(pRsaKey, RSA_KEY_SIZE, WC_RSA_EXPONENT, rng);
    if (ret != 0) {
        print_wolfssl_error("  failed to create rsa key", ret);
        goto genkey_end;
    }

    err = 0;
genkey_end:
   return err;
}

static int store_rsa_public_key(RsaKey* pRsaKey)
{
    unsigned char derBuf[MAX_DER_SIZE];
    int sz;
    int ret;
    int err = 1;

    printf("Encode RSA public key\n");
    /* Create DER encoding of RSA public key. */
    ret = wc_RsaKeyToPublicDer(pRsaKey, derBuf, sizeof(derBuf));
    if (ret <= 0) {
        print_wolfssl_error("  error encoding rsa public key", ret);
        goto store_end;
    }
    sz = ret;

    printf("Writing public key to %s\n", kRsaPubKey);
    if (write_file(kRsaPubKey, derBuf, sz) != 0) {
        goto store_end;
    }

    err = 0;
store_end:
   return err;
}

static int load_rsa_public_key(RsaKey* pRsaKey)
{
    unsigned char derBuf[MAX_DER_SIZE];
    int sz = (int)sizeof(derBuf);
    int ret;
    int err = 1;
    word32 idx = 0;

    printf("Reading in RSA public key to verify signature\n");
    if (read_file(kRsaPubKey, derBuf, &sz) != 0) {
        goto load_end;
    }

    printf("Decoding RSA public key\n");
    /* Decode DER encoding of RSA public key. */
    ret = wc_RsaPublicKeyDecode(derBuf, &idx, pRsaKey, sz);
    if (ret < 0) {
        print_wolfssl_error("  error decoding rsa public key", ret);
        goto load_end;
    }

   err = 0;
load_end:
    return err;
}

static int hash_string(const char* msg, unsigned char* hash)
{
    wc_Sha256 sha256;
    int ret;
    int err = 1;

    printf("Hashing message: '%s'\n", msg);

    /* Initialize hash for use. */
    ret = wc_InitSha256(&sha256);
    if (ret < 0) {
        print_wolfssl_error("  error initializing SHA-256 hash", ret);
        goto hash_end;
    }

    /* Hash the message string. */
    ret = wc_Sha256Update(&sha256, (unsigned char*)msg, XSTRLEN(msg));
    if (ret < 0) {
        print_wolfssl_error("  error SHA-256 hashing msg", ret);
        goto hash_end;
    }

    /* Generate hash value. */
    ret = wc_Sha256Final(&sha256, hash);
    if (ret < 0) {
        print_wolfssl_error("  error creating SHA-256 hashing", ret);
        goto hash_end;
    }

    err = 0;
hash_end:
    return err;
}

static int sign_with_rsa_key(RsaKey* pRsaKey, WC_RNG* rng, const char* msg,
    const char* sigFilename)
{
    unsigned char hash[WC_SHA256_DIGEST_SIZE];
    unsigned char pSignature[RSA_KEY_SIZE/8];
    int sz;
    int ret;
    int err = 1;

    /* Hash message to be signed. */
    if (hash_string(msg, hash) != 0) {
        goto sign_end;
    }

    printf("Signing hash of message\n");
    /* RSA-PSS sign. */
    ret = wc_RsaPSS_Sign(hash, sizeof(hash), pSignature, sizeof(pSignature),
        WC_HASH_TYPE_SHA256, WC_MGF1SHA256, pRsaKey, rng);
    if (ret <= 0) {
        print_wolfssl_error("  RSA_private_encrypt failed with error", ret);
        goto sign_end;
    }
    sz = ret;

    printf("Writing signature\n");
    if (write_file(sigFilename, pSignature, sz) != 0) {
        goto sign_end;
    }

    err = 0;
sign_end:
    return err;
}

static int verify_with_rsa_public_key(RsaKey* pRsaKey, const char* msg,
    const char* sigFilename)
{
    unsigned char pSignature[RSA_KEY_SIZE/8];
    int sz = sizeof(pSignature);
    unsigned char pDecrypted[RSA_KEY_SIZE/8];
    unsigned char* pt;
    unsigned char hash[WC_SHA256_DIGEST_SIZE];
    int ret;
    int err = 1;

    /* Hash message to be signed. */
    if (hash_string(msg, hash) != 0) {
        goto verify_end;
    }

    printf("Read signature\n");
    /* Read in signature file. */
    if (read_file(sigFilename, pSignature, &sz) != 0) {
        goto verify_end;
    }

    printf("Verify hash of message\n");

    /* Verify hash against signature with RSA public key. */
    pt = pDecrypted;
    /* Verify the signature decrypts. */
    ret = wc_RsaPSS_VerifyInline(pSignature, sz, &pt,
        WC_HASH_TYPE_SHA256, WC_MGF1SHA256, pRsaKey);
    if (ret < 0) {
        print_wolfssl_error("  error verify signature", ret);
        goto verify_end;
    }
    sz = ret;

    /* Check PSS padding on decrypted signature. */
    ret = wc_RsaPSS_CheckPadding(hash, sizeof(hash), pt, sz,
        WC_HASH_TYPE_SHA256);
    if (ret < 0) {
        print_wolfssl_error("  error checking padding", ret);
        goto verify_end;
    }

    printf("RSA PSS verify success\n");

    err = 0;
verify_end:
    return err;
}

static void print_usage()
{
    printf("Usage:\n");
    printf("\trsa-pss -s sign.bin\n");
    printf("\trsa-pss -v sign.bin\n");
}

int main(int argc, char** argv)
{
    RsaKey* pRsaKey = NULL;
    WC_RNG rng;
    const char* msg = "This is the string to be signed";
    int ret = 0;
    int rc = 1;

    if (argc <= 2) {
        print_usage();
        goto prog_end;
    }

    /* Turn on debugging logging. */
    wolfSSL_Debugging_ON();

    /* Initialize wolfCrypt for operations. */
    wolfCrypt_Init();

    /* Initialize an random number generator for key generation and signing. */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        print_wolfssl_error("Init RNG failed", ret);
        goto prog_end;
    }

    /* Allocate memory for RSA key. */
    pRsaKey = malloc(sizeof(RsaKey));
    if (!pRsaKey) {
        printf("Failed to allocate %d bnytes\n", (int)sizeof(RsaKey));
        goto prog_end;
    }

    /* Initialize RSA key. */
    ret = wc_InitRsaKey(pRsaKey, NULL);
    if (ret != 0) {
        print_wolfssl_error("Init RSA key failed", ret);
        goto prog_end;
    }

    /* Set the random number generator against RSA key for signing. */
    ret = wc_RsaSetRNG(pRsaKey, &rng);
    if (ret != 0) {
        print_wolfssl_error("Set RSA RNG failed", ret);
        goto prog_end;
    }

    /* Check whether we have been asked to sign the message. */
    if (memcmp(argv[1], kRsaSignOpt, XSTRLEN(kRsaSignOpt)) == 0) {
        /* Generate RSA public/private key pair. */
        if (generate_rsa_key(pRsaKey, &rng) != 0) {
            goto prog_end;
        }

        /* Store the RSA public key for verification. */
        if (store_rsa_public_key(pRsaKey) != 0) {
            goto prog_end;
        }

        /* Sign the message with the RSA private key. */
        if (sign_with_rsa_key(pRsaKey, &rng, msg, argv[2]) != 0) {
            goto prog_end;
        }
    }
    /* Check whether we have been asked to verify the message. */
    else if (memcmp(argv[1], kRsaVerifyOpt, XSTRLEN(kRsaVerifyOpt)) == 0) {
        /* Load the RSA public key. */
        if (load_rsa_public_key(pRsaKey) != 0) {
            goto prog_end;
        }

        /* Verify the message with the RSA public key. */
        if (verify_with_rsa_public_key(pRsaKey, msg, argv[2]) != 0) {
            goto prog_end;
        }
    }
    /* Unrecognized command line argument. */
    else {
        print_usage();
        goto prog_end;
    }

    rc = 0;
prog_end:
    /* Zeroize and free any internally allocated data in RSA key. */
    wc_FreeRsaKey(pRsaKey);
    /* Dispose of RSA key memory. */
    if (pRsaKey)
        free(pRsaKey);
    /* Free any internally allocated data in random number generator. */
    wc_FreeRng(&rng);

    /* Cleanup wolfCrypt after operations. */
    wolfCrypt_Cleanup();

    return rc;
}
#else
int main(int argc, char** argv)
{
    (void)kRsaSignOpt;
    (void)kRsaPubKey;

    printf("wolfSSL missing build features.\n");
    printf("Please build using `./configure --enable-rsapss "
           "--enable-keygen`\n");
    return -1;
}
#endif

