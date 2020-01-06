/* sign.c
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

/* This file shows how to sign a message with an RSA private key.
 * The signature is PKCS#1.5 formatted.
 * Key and data are held in buffers.
 * The output of this program can be used with "verify.c".
 */

#include <stdio.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/asn_public.h>

#include "rsa_priv_2048.h"

/* Signature size is the length of the modulus of the RSA key */
#define SIG_SZ              (2048 / 8)
/* Maximum bound on digest algorithm encoding around digest */
#define MAX_ENC_ALG_SZ      32

/* Print out the buffer in C code.
 *
 * name  [in]  Name of the variable.
 * data  [in]  Data to print out.
 * len   [in]  Length of the data.
 */
void print_buffer(char* name, unsigned char* data, word32 len)
{
    word32 i;

    printf("unsigned char %s[] = {\n", name);
    for (i = 0; i < len; i++) {
        if ((i % 8) == 0)
            printf("   ");
        printf(" 0x%02x,", data[i]);
        if ((i % 8) == 7)
            printf("\n");
    }
    if ((i % 8) != 0)
        printf("\n");
    printf("};\n");

}

/* Main entry point.
 * Signs the message passed in as the first command line argument.
 *
 * argc  [in]  Count of command line arguments.
 * argv  [in]  Command line argument vector.
 * Returns 0 on success and 1 otherwise.
 */
int main(int argc, char* argv[])
{
    int            ret = 0;
    Sha256         sha256;
    Sha256*        pSha256 = NULL;
    RsaKey         rsaKey;
    RsaKey*        pRsaKey = NULL;
#ifdef WC_RSA_BLINDING
    WC_RNG         rng;
    WC_RNG*        pRng = NULL;
#endif
    word32         idx;
    unsigned char* msg;
    word32         msgLen;
    unsigned char  signature[SIG_SZ];
    word32         sigLen;
    unsigned char  digest[WC_SHA256_DIGEST_SIZE];
    unsigned char  encSig[WC_SHA256_DIGEST_SIZE + MAX_ENC_ALG_SZ];
    word32         encSigLen;

    /* Get the message to sign from the command line */
    if (argc != 2) {
        fprintf(stderr, "Message to sign required\n");
        ret = -1;
    }
    else {
        msg = (unsigned char*)argv[1];
        msgLen = strlen(argv[1]);
    }

    /* Calculate SHA-256 digest of message */
    if (ret == 0)
        ret = wc_InitSha256(&sha256);
    if (ret == 0) {
        pSha256 = &sha256;
        ret = wc_Sha256Update(&sha256, msg, msgLen);
    }
    if (ret == 0)
        ret = wc_Sha256Final(&sha256, digest);

    /* Encode digest with algorithm information as per PKCS#1.5 */
    if (ret == 0) {
        encSigLen = wc_EncodeSignature(encSig, digest, sizeof(digest), SHA256h);
        if ((int)encSigLen < 0)
            ret = (int)encSigLen;
    }

    /* Initialize RSA key and random (if required) */
    if (ret == 0) {
        ret = wc_InitRsaKey(&rsaKey, NULL);
        if (ret == 0)
            pRsaKey = &rsaKey;
    }
#ifdef WC_RSA_BLINDING
    if (ret == 0)
        ret = wc_InitRng(&rng);
#endif
    /* Load DER encoded RSA private key from buffer */
    if (ret == 0) {
#ifdef WC_RSA_BLINDING
        pRng = &rng;
#endif
        idx = 0;
        ret = wc_RsaPrivateKeyDecode(private_key_2048, &idx, &rsaKey,
                                     sizeof(private_key_2048));
    }

    /* Sign encoded digest */
    if (ret == 0) {
#ifdef WC_RSA_BLINDING
        ret = wc_RsaSSL_Sign(encSig, encSigLen, signature, sizeof(signature),
                             &rsaKey, pRng);
#else
        ret = wc_RsaSSL_Sign(encSig, encSigLen, signature, sizeof(signature),
                             &rsaKey, NULL);
#endif
        if (ret >= 0) {
            sigLen = ret;
            ret = 0;
        }
    }

    if (ret == 0) {
        /* Display message as a buffer */
        print_buffer("msg", msg, msgLen);
        printf("\n");
        /* Display binary signature as a buffer */
        print_buffer("rsa_sig_2048", signature, sigLen);
    }

    /* Free data structures */
#ifdef WC_RSA_BLINDING
    if (pRng != NULL)
        wc_FreeRng(pRng);
#endif
    if (pRsaKey != NULL)
        wc_FreeRsaKey(pRsaKey);
    if (pSha256 != NULL)
        wc_Sha256Free(pSha256);

    return ret == 0 ? 0 : 1;
}

