/* clu_dgst_setup.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/signature.h>

#include "clu_include/clu_header_main.h"
#include "clu_include/clu_optargs.h"
#include "clu_include/sign-verify/clu_sign.h"
#include "clu_include/sign-verify/clu_verify.h"

int wolfCLU_dgst_setup(int argc, char** argv)
{
    WOLFSSL_BIO *sigBio = NULL;
    WOLFSSL_BIO *pubKeyBio = NULL;
    WOLFSSL_BIO *dataBio = NULL;
    WOLFSSL_EVP_PKEY *pkey;
    int     ret         = 0;    /* return variable, counter */
    int     i           = 0;    /* loop variable */
    char*   in;                 /* input variable */
    char*   out;                /* output variable */
    char*   priv;               /* private key variable */
    char*   sig;
    int derSz;
    char* data;
    int dataSz;
    int sigSz;
    int keySz;
    void*   key = NULL;
    int option;
    int long_index = 2;

    unsigned char* der = NULL;
    ecc_key ecc;
    RsaKey  rsa;
    word32 idx = 0;

    char*   alg;                /* algorithm being used */
    int     algCheck    = -1;   /* acceptable algorithm check */
    int     inCheck     = 0;    /* input check */
    int     privCheck   = 0;    /* private key check */
    int     signCheck   = 0;
    int     verifyCheck = 0;
    int     pubInCheck  = 0;
    int     sigCheck    = 0;
    enum wc_HashType      hashType = WC_HASH_TYPE_NONE;
    enum wc_SignatureType sigType  = WC_SIGNATURE_TYPE_NONE;

    opterr = 0; /* do not display unrecognized options */
    optind = 0; /* start at indent 0 */
    while ((option = getopt_long_only(argc, argv, "",
                   dgst_options, &long_index )) != -1) {

        switch (option) {

            case CERT_SHA:
                hashType = WC_HASH_TYPE_SHA;
                break;

            case CERT_SHA224:
                hashType = WC_HASH_TYPE_SHA224;
                break;

            case CERT_SHA256:
                hashType = WC_HASH_TYPE_SHA256;
                break;

            case CERT_SHA384:
                hashType = WC_HASH_TYPE_SHA384;
                break;

            case CERT_SHA512:
                hashType = WC_HASH_TYPE_SHA512;
                break;

            case WOLFCLU_VERIFY:
                pubKeyBio = wolfSSL_BIO_new_file(optarg, "rb");
                if (pubKeyBio == NULL) {
                    printf("unable to open public key file %s\n", optarg);
                    ret = -1;
                }
                break;

            case INFILE:
                sigBio = wolfSSL_BIO_new_file(optarg, "rb");
                if (sigBio == NULL) {
                    printf("unable to signature file %s\n", optarg);
                    ret = -1;
                }
                break;

        case ':':
        case '?':
            break;

        default:
            /* do nothing. */
            (void)ret;
        }
    }

    /* signed file should be the last arg */
    dataBio = wolfSSL_BIO_new_file(argv[argc-1], "rb");
    if (dataBio == NULL) {
        printf("unable to open data file %s\n", argv[argc-1]);
        ret = -1;
    }

    dataSz = wolfSSL_BIO_get_len(dataBio);
    sigSz  = wolfSSL_BIO_get_len(sigBio);

    /* create buffers and fill them */
    data = malloc(dataSz);
    wolfSSL_BIO_read(dataBio, data, dataSz);

    sig = malloc(sigSz);
    wolfSSL_BIO_read(sigBio, sig, sigSz);

    /* get type of key and size of structure */
    //pkey = wolfSSL_d2i_PUBKEY_bio(pubKeyBio, NULL); @TODO if DER input
    pkey = wolfSSL_PEM_read_bio_PUBKEY(pubKeyBio, NULL, NULL, NULL);
    if (pkey == NULL) {
        printf("unable to decode public key\n");
        return -1;
    }

    switch (wolfSSL_EVP_PKEY_id(pkey)) {
        case EVP_PKEY_RSA:
            keySz   = sizeof(RsaKey);
            sigType = WC_SIGNATURE_TYPE_RSA;

            key = (void*)&rsa;
            wc_InitRsaKey(&rsa, NULL);
            derSz = wolfSSL_i2d_PUBKEY(pkey, &der);
            wc_RsaPublicKeyDecode(der, &idx, &rsa, derSz);

            break;

        case EVP_PKEY_EC:
            keySz   = sizeof(ecc_key);
            sigType = WC_SIGNATURE_TYPE_ECC;

            key = (void*)&ecc;
            wc_ecc_init(&ecc);
            derSz = wolfSSL_i2d_PUBKEY(pkey, &der);
            wc_EccPublicKeyDecode(der, &idx, &ecc, derSz);

            break;

        default:
            printf("key type not yet supported\n");
            return -1;
    }

    if (wc_SignatureVerify(hashType, sigType, (const byte*)data, dataSz,
                (const byte*)sig, sigSz, key, keySz) == 0) {
        printf("Verify OK\n");
    }
    else {
        printf("Verification failure\n");
    }

    switch (sigType) {
        case WC_SIGNATURE_TYPE_RSA:
            wc_FreeRsaKey(&rsa);
            break;

        case WC_SIGNATURE_TYPE_ECC:
            wc_ecc_free(&ecc);
            break;

        default:
            printf("key type not yet supported\n");
            return -1;
    }

    free(data);
    free(sig);

    wolfSSL_BIO_free(sigBio);
    wolfSSL_BIO_free(pubKeyBio);
    wolfSSL_BIO_free(dataBio);

    return ret;
}

