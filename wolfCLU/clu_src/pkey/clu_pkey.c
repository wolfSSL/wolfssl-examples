/* clu_pkey_setup.c
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
#include "clu_include/pkey/clu_pkey.h"
#include "clu_include/x509/clu_cert.h"
#include "clu_include/x509/clu_parse.h"

#include <wolfssl/ssl.h>
#include <wolfssl/openssl/pem.h>

static void wolfCLU_pKeyHelp(void)
{
    printf("./wolfssl pkey\n");
    printf("\t-in file input for key to read\n");
    printf("\t-pubout output the public key\n");
}


/* print out PEM public key
 * returns 0 on success
 */
static int wolfCLU_pKeyPEMtoPubKey(WOLFSSL_BIO* bio, WOLFSSL_EVP_PKEY* pkey)
{
    int type;
    int ret = WOLFSSL_FAILURE;

    type = wolfSSL_EVP_PKEY_id(pkey);
    switch (type) {
        case EVP_PKEY_RSA:
            ret = wolfSSL_PEM_write_bio_RSA_PUBKEY(bio,
                    wolfSSL_EVP_PKEY_get0_RSA(pkey));
            break;
        case EVP_PKEY_DSA:
            break;
        case EVP_PKEY_EC:
            ret = wolfSSL_PEM_write_bio_EC_PUBKEY(bio,
                    wolfSSL_EVP_PKEY_get0_EC_KEY(pkey));
            break;
        default:
            printf("unknown / unsupported key type\n");
    }

    if (ret == WOLFSSL_SUCCESS) {
        return 0;
    }
    else {
        return -1;
    }
}


/* creates an out buffer containing only the public key from the pkey
 * returns size of buffer on success
 */
static int wolfCLU_pKeytoPubKey(WOLFSSL_EVP_PKEY* pkey, unsigned char** out,
        int* outSz)
{
    int type;
    int ret = 0;

    *outSz = 0;
    type   = wolfSSL_EVP_PKEY_id(pkey);
    switch (type) {
        case EVP_PKEY_RSA:
            *outSz = wolfSSL_i2d_RSAPublicKey(
                   wolfSSL_EVP_PKEY_get0_RSA(pkey), (const unsigned char**)out);
            break;

        case EVP_PKEY_DSA:
            printf("DSA key not yet supported\n");
            ret = USER_INPUT_ERROR;
            break;

        case EVP_PKEY_EC:
            {
                int derSz = 0, ret = 0;
                unsigned char *der = NULL;
                WOLFSSL_EC_KEY *ec = NULL;

                ec = wolfSSL_EVP_PKEY_get0_EC_KEY(pkey);
                if (ec == NULL) {
                    printf("no ecc key found in pkey\n");
                    ret = BAD_FUNC_ARG;
                }

                if (ret == 0) {
                    derSz = wc_EccPublicKeyDerSize((ecc_key*)ec->internal, 1);
                    if (derSz < 0) {
                        printf("unable to get ecc der size\n");
                        ret = BAD_FUNC_ARG;
                    }
                }

                if (ret == 0) {
                     der = malloc(derSz);
                     if (der == NULL) {
                         printf("unable to malloc der buffer\n");
                         ret = MEMORY_E;
                     }
                }

                if (ret == 0) {
                     ret = wc_EccPublicKeyToDer((ecc_key*)ec->internal, der,
                             derSz, 1);
                     if (ret > 0) {
                         ret    = 0;
                         *out   = der;
                         *outSz = derSz;
                     }
                     else {
                        ret = BAD_FUNC_ARG;
                        printf("decoding der from internal structure failed\n");
                     }
                }

                if (der != NULL)
                    free(der);
            }
            break;

        default:
            printf("unknown / unsupported key type\n");
            ret = USER_INPUT_ERROR;
    }

    if (ret == 0)
        ret = *outSz;
    return ret;
}


int wolfCLU_pKeySetup(int argc, char** argv)
{
    int ret    = 0;
    int inForm = PEM_FORM;
    int pubOut = 0;
    int option;
    int long_index = 1;
    WOLFSSL_EVP_PKEY *pkey = NULL;
    WOLFSSL_BIO *bioIn  = NULL;
    WOLFSSL_BIO *bioOut = NULL;

    opterr = 0; /* do not display unrecognized options */
    optind = 0; /* start at indent 0 */
    while ((option = getopt_long_only(argc, argv, "",
                   pkey_options, &long_index )) != -1) {
        switch (option) {
            case PUBOUT:
                pubOut = 1;
                break;

            case INFILE:
                bioIn = wolfSSL_BIO_new_file(optarg, "rb");
                if (bioIn == NULL) {
                    printf("unable to open public key file %s\n", optarg);
                    ret = -1;
                }
                break;

            case INFORM:
                inForm = wolfCLU_checkInform(optarg);
                break;

            case WOLFCLU_HELP:
                wolfCLU_pKeyHelp();
                return 0;

            case ':':
            case '?':
                break;

            default:
                /* do nothing. */
                (void)ret;
        }
    }


    if (ret == 0 && bioIn != NULL) {
        if (inForm == PEM_FORM) {
            pkey = wolfSSL_PEM_read_bio_PrivateKey(bioIn, NULL, NULL, NULL);
        }
        else {
            pkey = wolfSSL_d2i_PrivateKey_bio(bioIn, NULL);
        }
        if (pkey == NULL) {
            printf("error reading key from file\n");
            ret = USER_INPUT_ERROR;
        }
    }

    if (ret == 0) {
        bioOut = wolfSSL_BIO_new(wolfSSL_BIO_s_file());
        if (bioOut == NULL) {
            ret = -1;
        }
        else {
            if (wolfSSL_BIO_set_fp(bioOut, stdout, BIO_NOCLOSE)
                    != WOLFSSL_SUCCESS) {
                ret = -1;
            }
        }
    }

    if (ret == 0 && pubOut == 1) {
        if (pkey != NULL) {
            if (inForm == PEM_FORM) {
                ret = wolfCLU_pKeyPEMtoPubKey(bioOut, pkey);
            }
            else {
                unsigned char *der = NULL;
                int derSz = 0;

                if (wolfCLU_pKeytoPubKey(pkey, &der, &derSz) <= 0) {
                    printf("error converting der found to public key\n");
                    ret = -1;
                }
                else {
                    if (wolfCLU_printDerPubKey(bioOut, der, derSz) != 0) {
                        printf("error printing out pubkey\n");
                        ret = -1;
                    }
                    free(der);
                }
            }
        }
    }

    wolfSSL_BIO_free(bioIn);
    wolfSSL_BIO_free(bioOut);
    wolfSSL_EVP_PKEY_free(pkey);

    return ret;
}



