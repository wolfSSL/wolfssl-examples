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

#include <wolfclu/clu_header_main.h>
#include <wolfclu/clu_optargs.h>
#include <wolfclu/sign-verify/clu_sign.h>
#include <wolfclu/sign-verify/clu_verify.h>

int wolfCLU_dgst_setup(int argc, char** argv)
{
    WOLFSSL_BIO *sigBio = NULL;
    WOLFSSL_BIO *pubKeyBio = NULL;
    WOLFSSL_BIO *dataBio = NULL;
    WOLFSSL_EVP_PKEY *pkey;
    int     ret = 0;
    int     i   = 0;
    char*   in;
    char*   out;
    char*   priv;
    char* sig  = NULL;
    char* data = NULL;
    void* key  = NULL;
    int derSz  = 0;
    int dataSz = 0;
    int sigSz  = 0;
    int keySz  = 0;
    int option;
    int long_index = 2;

    unsigned char* der = NULL;
    ecc_key ecc;
    RsaKey  rsa;
    word32 idx = 0;

    char*   alg;                /* algorithm being used */
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

            case WOLFCLU_CERT_SHA:
                hashType = WC_HASH_TYPE_SHA;
                break;

            case WOLFCLU_CERT_SHA224:
                hashType = WC_HASH_TYPE_SHA224;
                break;

            case WOLFCLU_CERT_SHA256:
                hashType = WC_HASH_TYPE_SHA256;
                break;

            case WOLFCLU_CERT_SHA384:
                hashType = WC_HASH_TYPE_SHA384;
                break;

            case WOLFCLU_CERT_SHA512:
                hashType = WC_HASH_TYPE_SHA512;
                break;

            case WOLFCLU_VERIFY:
                pubKeyBio = wolfSSL_BIO_new_file(optarg, "rb");
                if (pubKeyBio == NULL) {
                    printf("unable to open public key file %s\n", optarg);
                    ret = -1;
                }
                break;

            case WOLFCLU_INFILE:
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
    if (ret == 0) {
        dataBio = wolfSSL_BIO_new_file(argv[argc-1], "rb");
        if (dataBio == NULL) {
            printf("unable to open data file %s\n", argv[argc-1]);
            ret = -1;
        }
    }

    if (ret == 0) {
        dataSz = wolfSSL_BIO_get_len(dataBio);
        sigSz  = wolfSSL_BIO_get_len(sigBio);
        if (dataSz <= 0 || sigSz <= 0) {
            printf("no signature or data\n");
            ret = -1;
        }
    }

    /* create buffers and fill them */
    if (ret == 0) {
        data = (char*)malloc(dataSz);
        if (data == NULL) {
            ret = MEMORY_E;
        }
        else {
            if (wolfSSL_BIO_read(dataBio, data, dataSz) <= 0) {
                printf("error reading data\n");
                ret = -1;
            }
        }
    }

    if (ret == 0) {
        sig = (char*)malloc(sigSz);
        if (sig == NULL) {
            ret = MEMORY_E;
        }
        else {
            if (wolfSSL_BIO_read(sigBio, sig, sigSz) <= 0) {
                printf("error reading sig\n");
                ret = -1;
            }
        }
    }

    /* get type of key and size of structure */
    if (ret == 0) {
        pkey = wolfSSL_PEM_read_bio_PUBKEY(pubKeyBio, NULL, NULL, NULL);
        if (pkey == NULL) {
            printf("unable to decode public key\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        switch (wolfSSL_EVP_PKEY_id(pkey)) {
            case EVP_PKEY_RSA:
                keySz   = sizeof(RsaKey);
                sigType = WC_SIGNATURE_TYPE_RSA;

                key = (void*)&rsa;
                if (wc_InitRsaKey(&rsa, NULL) != 0) {
                    printf("unable to initialize rsa key\n");
                    ret = -1;
                }

                if (ret == 0) {
                    derSz = wolfSSL_i2d_PUBKEY(pkey, &der);
                    if (derSz <= 0) {
                        printf("error converting pkey to der\n");
                        ret = -1;
                    }
                }

                if (ret == 0 &&
                    wc_RsaPublicKeyDecode(der, &idx, &rsa, derSz) != 0) {
                    printf("error decoding public rsa key\n");
                    ret = -1;
                }

                break;

            case EVP_PKEY_EC:
                keySz   = sizeof(ecc_key);
                sigType = WC_SIGNATURE_TYPE_ECC;

                key = (void*)&ecc;
                if (wc_ecc_init(&ecc) != 0) {
                    printf("error initializing ecc key\n");
                    ret = -1;
                }

                if (ret == 0) {
                    derSz = wolfSSL_i2d_PUBKEY(pkey, &der);
                    if (derSz <= 0) {
                        printf("error converting pkey to der\n");
                        ret = -1;
                    }
                }

                if (ret == 0 &&
                        wc_EccPublicKeyDecode(der, &idx, &ecc, derSz) != 0) {
                    printf("error decoding public ecc key\n");
                    ret = -1;
                }

                break;

            default:
                printf("key type not yet supported\n");
                ret = -1;
        }
    }

    if (ret == 0) {
        if (wc_SignatureVerify(hashType, sigType, (const byte*)data, dataSz,
                    (const byte*)sig, sigSz, key, keySz) == 0) {
            printf("Verify OK\n");
        }
        else {
            printf("Verification failure\n");
        }
    }

    if (ret == 0) {
        switch (sigType) {
            case WC_SIGNATURE_TYPE_RSA:
                wc_FreeRsaKey(&rsa);
                break;

            case WC_SIGNATURE_TYPE_ECC:
                wc_ecc_free(&ecc);
                break;

            default:
                printf("key type not yet supported\n");
                ret = -1;
        }
    }

    if (data != NULL)
        free(data);
    if (sig != NULL)
        free(sig);

    wolfSSL_BIO_free(sigBio);
    wolfSSL_BIO_free(pubKeyBio);
    wolfSSL_BIO_free(dataBio);

    return ret;
}

