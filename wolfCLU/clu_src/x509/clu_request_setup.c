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
#include "clu_include/x509/clu_request.h"
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

    int     ret        =   0;   /* return variable, counter */
    int     i          =   0;   /* loop variable */
    char*   in;                 /* input variable */
    char*   out;                /* output variable */

    char*   alg;                /* algorithm being used */
    int     keyCheck =   0;     /* input check */
    int     algCheck =   0;     /* algorithm type */
    int     oid;
    int     outform = 0;


    /* help checking */
    ret = wolfCLU_checkForArg("-help", 5, argc, argv);
    if (ret > 0) {
        wolfCLU_certgenHelp();
        return 0;
    }

    // TODO remove hard coded
    if (wolfCLU_checkForArg("-rsa", 3, argc, argv) > 0) {
        algCheck = 1;
    } else if (wolfCLU_checkForArg("-ed25519", 7, argc, argv) > 0) {
        algCheck = 2;
    } else if (wolfCLU_checkForArg("-ecc", 3, argc, argv) > 0) {
        algCheck = 3;
    } else {
        /* hang on, lets see if a key was passed in */
        //wolfCLU_certgenHelp();
        //return FATAL_ERROR;
    }

    ret = wolfCLU_checkForArg("-config", 7, argc, argv);
    if (ret > 0) {
        x509 = wolfCLU_readConfig(argv[ret+1], "req");
    }
    else {
        /* if no configure is passed in then get input from command line */
        WOLFSSL_X509_NAME *name;

        x509 = wolfSSL_X509_new();
        name = wolfSSL_X509_NAME_new();
        wolfCLU_CreateX509Name(name);
        wolfSSL_X509_REQ_set_subject_name(x509, name);
    }
    if (x509 == NULL) {
        printf("issue creating structure to use\n");
    }

    ret = wolfCLU_checkForArg("-days", 5, argc, argv);
    if (ret > 0) {
        WOLFSSL_ASN1_TIME *notBefore, *notAfter;
        time_t t;

        t = time(NULL);
        notBefore = wolfSSL_ASN1_TIME_adj(NULL, t, 0, 0);
        notAfter = wolfSSL_ASN1_TIME_adj(NULL, t, atoi(argv[ret+1]), 0);
        wolfSSL_X509_set_notBefore(x509, notBefore);
        wolfSSL_X509_set_notAfter(x509, notAfter);

        wolfSSL_ASN1_TIME_free(notBefore);
        wolfSSL_ASN1_TIME_free(notAfter);
    }


    ret = wolfCLU_checkForArg("-in", 3, argc, argv);
    if (ret <= 0) {
        /* check for key argument instead of -in */
        ret = wolfCLU_checkForArg("-key", 4, argc, argv);
    }
    if (ret > 0) {
        if (argv[ret+1] != NULL) {
            bioIn = wolfSSL_BIO_new_file(argv[ret+1], "rb");
            if (bioIn == NULL) {
                printf("error opening file %s\n", argv[ret+1]);
                return USER_INPUT_ERROR;
            }

            pkey = wolfSSL_PEM_read_bio_PrivateKey(bioIn, NULL, NULL, NULL);
            if (pkey == NULL) {
                printf("error reading key from file %s\n", argv[ret+1]);
                return USER_INPUT_ERROR;
            }
            wolfSSL_X509_set_pubkey(x509, pkey);
            keyCheck = 1;
            wolfSSL_BIO_free(bioIn);
        }
    }
    else {
        printf("Please specify a -key <key> option when "
               "generating a certificate.\n");
        wolfCLU_certgenHelp();
        return ret;
    }

    ret = wolfCLU_checkForArg("-out", 4, argc, argv);
    if (ret > 0) {
        bioOut = wolfSSL_BIO_new_file(argv[ret+1], "wb");
    }
    else {
        /* output to stdout if no output is provided */
        bioOut = wolfSSL_BIO_new(wolfSSL_BIO_s_file());
        if (bioOut != NULL) {
            if (wolfSSL_BIO_set_fp(bioOut, stdout, BIO_NOCLOSE)
                    != WOLFSSL_SUCCESS) {
                ret = -1;
            }
        }
    }

    ret = wolfCLU_checkForArg("-outform", 8, argc, argv);
    if (ret > 0) {
        if (XSTRNCMP("DER", argv[ret+1], 3) == 0) {
            outform = 1;
        }
        if (XSTRNCMP("der", argv[ret+1], 3) == 0) {
            outform = 1;
        }
    }

    if (wolfCLU_checkForArg("-sha224", 7, argc, argv) != 0) {
        md  = wolfSSL_EVP_sha224();
        oid = SHA_HASH224;
    } else if (wolfCLU_checkForArg("-sha256", 7, argc, argv) != 0) {
        md  = wolfSSL_EVP_sha256();
        oid = SHA_HASH256;
    } else if (wolfCLU_checkForArg("-sha384", 7, argc, argv) != 0) {
        md  = wolfSSL_EVP_sha384();
        oid = SHA_HASH384;
    } else if (wolfCLU_checkForArg("-sha512", 7, argc, argv) != 0) {
        md  = wolfSSL_EVP_sha512();
        oid = SHA_HASH512;
    } else {
        /* default to using sha256 if hash is not set */
        md  = wolfSSL_EVP_sha256();
        oid = SHA_HASH256;
    }

    if (keyCheck == 0) {
        printf("Must have input as either a file or standard I/O\n");
        return FATAL_ERROR;
    }

    // TODO remove hard coded values
    if (algCheck == 1) {
        //ret = make_self_signed_rsa_certificate(in, out, oid);
    } else if (algCheck == 2) {
        ret = make_self_signed_ed25519_certificate(in, out);
    } else if (algCheck == 3) {
        //ret = make_self_signed_ecc_certificate(in, out, oid);
    }

    /* sign the req */
    ret = wolfSSL_X509_REQ_sign(x509, pkey, md);
    if (ret != WOLFSSL_SUCCESS) {
        printf("error %d signing REQ\n", ret);
    }

    if (ret == WOLFSSL_SUCCESS) {
        if (outform == 1) {
            ret = wolfSSL_i2d_X509_REQ_bio(bioOut, x509);
        }
        else {
            ret = wolfSSL_PEM_write_bio_X509_REQ(bioOut, x509);
        }
        if (ret != WOLFSSL_SUCCESS) {
            printf("error %d writing out cert req\n", ret);
        }
    }

    wolfSSL_BIO_free(bioOut);
    return ret;
#endif
}


