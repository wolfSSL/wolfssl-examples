/* clu_request_setup.c
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

#include "clu_include/clu_header_main.h"
#include "clu_include/clu_optargs.h"
#include "clu_include/x509/clu_request.h"
#include "clu_include/x509/clu_cert.h"
#include "clu_include/certgen/clu_certgen.h"

int wolfCLU_requestSetup(int argc, char** argv)
{
#ifndef WOLFSSL_CERT_REQ
    printf("wolfSSL not compiled with --enable-certreq\n");
    return -1;
#else
    WOLFSSL_BIO *bioOut = NULL;
    WOLFSSL_BIO *bioIn  = NULL;
    WOLFSSL_X509 *x509  = NULL;
    const WOLFSSL_EVP_MD *md  = NULL;
    WOLFSSL_EVP_PKEY *pkey = NULL;

    int     ret = 0;
    int     i   = 0;
    char*   in  = NULL;
    char*   out = NULL;
    char*   config = NULL;

    char*   alg;                /* algorithm being used */
    int     algCheck =   0;     /* algorithm type */
    int     oid;
    int     outForm = PEM_FORM; /* default to PEM format */
    int     inForm  = PEM_FORM; /* default to PEM format */
    int     option;
    int     long_index = 1;
    int     days = 0;


    opterr = 0; /* do not display unrecognized options */
    optind = 0; /* start at indent 0 */
    while ((option = getopt_long_only(argc, argv, "", req_options,
                    &long_index )) != -1) {

        switch (option) {
            case INFILE:
            case KEY:
                in = optarg;
                bioIn = wolfSSL_BIO_new_file(optarg, "rb");
                if (bioIn == NULL) {
                    printf("unable to open public key file %s\n", optarg);
                    ret = -1;
                }
                break;

            case OUTFILE:
                out = optarg;
                bioOut = wolfSSL_BIO_new_file(optarg, "wb");
                if (bioOut == NULL) {
                    printf("unable to open output file %s\n", optarg);
                    ret = -1;
                }
                break;

            case INFORM:
                inForm = wolfCLU_checkInform(optarg);
                (void)inForm; /* for future use */
                break;

            case OUTFORM:
                outForm = wolfCLU_checkOutform(optarg);
                break;

            case WOLFCLU_HELP:
                wolfCLU_certgenHelp();
                return 0;

            case WOLFCLU_RSA:
                algCheck = 1;
                break;

            case WOLFCLU_ECC:
                algCheck = 3;
                break;

            case WOLFCLU_ED25519:
                algCheck = 2;
                break;

            case WOLFCLU_CONFIG:
                config = optarg;
                break;

            case WOLFCLU_DAYS:
                days = atoi(optarg);
                break;

            case CERT_SHA:
                md  = wolfSSL_EVP_sha1();
                oid = SHA_HASH;
                break;

            case CERT_SHA224:
                md  = wolfSSL_EVP_sha224();
                oid = SHA_HASH224;
                break;

            case CERT_SHA256:
                md  = wolfSSL_EVP_sha256();
                oid = SHA_HASH256;
                break;

            case CERT_SHA384:
                md  = wolfSSL_EVP_sha384();
                oid = SHA_HASH384;
                break;

            case CERT_SHA512:
                md  = wolfSSL_EVP_sha512();
                oid = SHA_HASH512;
                break;

            case ':':
            case '?':
                break;

            default:
                /* do nothing. */
                (void)ret;
        }
    }

    /* default to sha256 if not set */
    if (md == NULL) {
        md  = wolfSSL_EVP_sha256();
        oid = SHA_HASH256;
    }

    x509 = wolfSSL_X509_new();
    if (x509 == NULL) {
        printf("issue creating structure to use\n");
        ret = MEMORY_E;
    }

    if (ret == 0 && days > 0) {
        WOLFSSL_ASN1_TIME *notBefore, *notAfter;
        time_t t;

        t = time(NULL);
        notBefore = wolfSSL_ASN1_TIME_adj(NULL, t, 0, 0);
        notAfter = wolfSSL_ASN1_TIME_adj(NULL, t, atoi(argv[ret+1]), 0);
        if (notBefore == NULL || notAfter == NULL) {
            printf("error creating not before/after dates\n");
            ret = -1;
        }
        else {
            wolfSSL_X509_set_notBefore(x509, notBefore);
            wolfSSL_X509_set_notAfter(x509, notAfter);
        }

        wolfSSL_ASN1_TIME_free(notBefore);
        wolfSSL_ASN1_TIME_free(notAfter);
    }


    if (ret == 0 && bioIn != NULL) {
        pkey = wolfSSL_PEM_read_bio_PrivateKey(bioIn, NULL, NULL, NULL);
        if (pkey == NULL) {
            printf("error reading key from file\n");
            ret = USER_INPUT_ERROR;
        }

        if (ret == 0 &&
                wolfSSL_X509_set_pubkey(x509, pkey) != WOLFSSL_SUCCESS) {
            ret = -1;
        }
    }
    else {
        printf("Please specify a -key <key> option when "
               "generating a certificate.\n");
        wolfCLU_certgenHelp();
        ret = USER_INPUT_ERROR;
    }

    if (ret == 0) {
        if (config != NULL) {
            ret = wolfCLU_readConfig(x509, config, "req");
        }
        else {
            /* if no configure is passed in then get input from command line */
            WOLFSSL_X509_NAME *name;

            name = wolfSSL_X509_NAME_new();
            if (name == NULL) {
                ret = MEMORY_E;
            }
            else {
                wolfCLU_CreateX509Name(name);
                wolfSSL_X509_REQ_set_subject_name(x509, name);
            }
        }
    }

    if (ret == 0 && bioOut == NULL) {
        /* output to stdout if no output is provided */
        bioOut = wolfSSL_BIO_new(wolfSSL_BIO_s_file());
        if (bioOut != NULL) {
            if (wolfSSL_BIO_set_fp(bioOut, stdout, BIO_NOCLOSE)
                    != WOLFSSL_SUCCESS) {
                ret = -1;
            }
        }
    }

    if (ret == 0) {
        if (algCheck == 3) {
            ret = make_self_signed_ecc_certificate(in, out, oid);
        }
        else {
            /* sign the req */
            ret = wolfSSL_X509_REQ_sign(x509, pkey, md);
            if (ret != WOLFSSL_SUCCESS) {
                printf("error %d signing REQ\n", ret);
            }

            if (ret == WOLFSSL_SUCCESS) {
                if (outForm == DER_FORM) {
                    ret = wolfSSL_i2d_X509_REQ_bio(bioOut, x509);
                }
                else {
                    ret = wolfSSL_PEM_write_bio_X509_REQ(bioOut, x509);
                }

                if (ret != WOLFSSL_SUCCESS) {
                    printf("error %d writing out cert req\n", ret);
                    ret = -1;
                }
                else {
                    /* set WOLFSSL_SUCCESS case to success value */
                    ret = 0;
                }
            }
        }
    }

    wolfSSL_BIO_free(bioIn);
    wolfSSL_BIO_free(bioOut);
    wolfSSL_X509_free(x509);
    wolfSSL_EVP_PKEY_free(pkey);
    return ret;
#endif
}


