/* clu_genkey.c
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

#include "clu_include/clu_header_main.h"
#include "clu_include/genkey/clu_genkey.h"
#include "clu_include/x509/clu_parse.h"

#if defined(WOLFSSL_KEY_GEN) && !defined(NO_ASN)

#include "clu_include/x509/clu_cert.h"    /* PER_FORM/DER_FORM */
#include <wolfssl/wolfcrypt/asn_public.h> /* wc_DerToPem */

#ifdef HAVE_ED25519
int wolfCLU_genKey_ED25519(WC_RNG* rng, char* fOutNm, int directive, int format)
{
    int ret;                             /* return value */
    int fOutNmSz = XSTRLEN(fOutNm);      /* file name without append */
    int fOutNmAppendSz = 6;              /* # of bytes to append to file name */
    int flag_outputPub = 0;              /* set if outputting both priv/pub */
    char privAppend[6] = ".priv\0";      /* last part of the priv file name */
    char pubAppend[6] = ".pub\0\0";      /* last part of the pub file name*/
    byte privKeyBuf[ED25519_KEY_SIZE*2]; /* will hold public & private parts */
    byte pubKeyBuf[ED25519_KEY_SIZE];    /* holds just the public key part */
    word32 privKeySz;                    /* size of private key */
    word32 pubKeySz;                     /* size of public key */
    ed25519_key edKeyOut;                /* the ed25519 key structure */
    char* finalOutFNm;                   /* file name + append */
    FILE* file;                          /* file stream */


    printf("fOutNm = %s\n", fOutNm);

    /*--------------- INIT ---------------------*/
    ret = wc_ed25519_init(&edKeyOut);
    if (ret != 0)
        return ret;
    /*--------------- MAKE KEY ---------------------*/
    ret = wc_ed25519_make_key(rng, ED25519_KEY_SIZE, &edKeyOut);
    if (ret != 0)
        return ret;
    /*--------------- GET KEY SIZES ---------------------*/
    privKeySz = wc_ed25519_priv_size(&edKeyOut);
    if (privKeySz <= 0)
        return WC_KEY_SIZE_E;

    pubKeySz = wc_ed25519_pub_size(&edKeyOut);
    if (pubKeySz <= 0)
        return WC_KEY_SIZE_E;
    /*--------------- EXPORT KEYS TO BUFFERS ---------------------*/
    ret = wc_ed25519_export_key(&edKeyOut, privKeyBuf, &privKeySz, pubKeyBuf,
                                                                     &pubKeySz);
    /*--------------- CONVERT TO PEM IF APPLICABLE  ---------------------*/
    if (format == PEM_FORM) {
        printf("Der to Pem for ed25519 key not yet implemented\n");
        printf("FEATURE COMING SOON!\n");
        return FEATURE_COMING_SOON;
    }
    /*--------------- OUTPUT KEYS TO FILE(S) ---------------------*/
    finalOutFNm = (char*) XMALLOC( (fOutNmSz + fOutNmAppendSz), HEAP_HINT,
                                               DYNAMIC_TYPE_TMP_BUFFER);
    if (finalOutFNm == NULL)
        return MEMORY_E;

    /* get the first part of the file name setup */
    XMEMSET(finalOutFNm, 0, fOutNmSz + fOutNmAppendSz);
    XMEMCPY(finalOutFNm, fOutNm, fOutNmSz);

    switch(directive) {
        case PRIV_AND_PUB:
            flag_outputPub = 1;
            /* Fall through to PRIV_ONLY */
        case PRIV_ONLY:
            /* add on the final part of the file name ".priv" */
            XMEMCPY(finalOutFNm+fOutNmSz, privAppend, fOutNmAppendSz);
            printf("finalOutFNm = %s\n", finalOutFNm);

            file = fopen(finalOutFNm, "wb");
            if (!file) {
                XFREE(finalOutFNm, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return OUTPUT_FILE_ERROR;
            }

            ret = (int) fwrite(privKeyBuf, 1, privKeySz, file);
            if (ret <= 0) {
                fclose(file);
                XFREE(finalOutFNm, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return OUTPUT_FILE_ERROR;
            }
            fclose(file);

            if (flag_outputPub == 0) {
                break;
            } /* else fall through to PUB_ONLY if flag_outputPub == 1*/
        case PUB_ONLY:
            /* add on the final part of the file name ".pub" */
            XMEMCPY(finalOutFNm+fOutNmSz, pubAppend, fOutNmAppendSz);
            printf("finalOutFNm = %s\n", finalOutFNm);

            file = fopen(finalOutFNm, "wb");
            if (!file) {
                XFREE(finalOutFNm, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return OUTPUT_FILE_ERROR;
            }

            ret = (int) fwrite(pubKeyBuf, 1, pubKeySz, file);
            if (ret <= 0) {
                fclose(file);
                XFREE(finalOutFNm, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return OUTPUT_FILE_ERROR;
            }
            fclose(file);
            break;
        default:
            printf("Invalid directive\n");
            XFREE(finalOutFNm, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            return BAD_FUNC_ARG;
    }

    XFREE(finalOutFNm, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    if (ret > 0) {
        /* ret > 0 indicates a successful file write, set to zero for return */
        ret = 0;
    }

    return ret;
}
#endif /* HAVE_ED25519 */

int wolfCLU_genKey_ECC_ex(WC_RNG* rng, char* fName, int directive, int fmt,
                       char* name)
{
#ifdef HAVE_ECC
    WOLFSSL_BIO *bioOut = NULL;
    WOLFSSL_EC_KEY   *key;
    int ret = 0;

    byte*  der   = NULL;
    int    derSz = -1;
    int    curve_id    = ECC_SECP256R1; /* default to common 256 bit curve */

    if (rng == NULL || fName == NULL)
        return BAD_FUNC_ARG;

    key = wolfSSL_EC_KEY_new();
    if (key == NULL)
        return ret;

    if (name != NULL) {
        WOLFSSL_EC_GROUP *group;

        group = wolfSSL_EC_GROUP_new_by_curve_name(wolfSSL_OBJ_txt2nid(name));
        if (group == NULL) {
            printf("unable to set curve\n");
            ret = -1;
        }


        if (ret == 0) {
            if (wolfSSL_EC_KEY_set_group(key, group) != WOLFSSL_SUCCESS) {
                printf("unable to set ec group\n");
                ret = -1;
            }
        }
    }

    if (ret == 0) {
        ret = wolfSSL_EC_KEY_generate_key(key);
        if (ret != WOLFSSL_SUCCESS) {
            printf("error generating EC key\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        bioOut = wolfSSL_BIO_new_file(fName, "wb");
        if (bioOut == NULL) {
            printf("unable to read outfile %s\n", fName);
            ret = MEMORY_E;
        }
    }

    if (ret == 0) {
        switch(directive) {
            case ECPARAM:
                {
                    if (fmt == PEM_FORM) {
                        if (wolfSSL_PEM_write_bio_ECPrivateKey(bioOut, key,
                                NULL, NULL, 0, NULL, NULL) != WOLFSSL_SUCCESS) {
                            ret = -1;
                        }
                    }
                    else {
                        WOLFSSL_EVP_PKEY *pkey;

                        pkey = wolfSSL_EVP_PKEY_new();
                        if (pkey == NULL) {
                            ret = MEMORY_E;
                        }

                        if (ret == 0) {
                            wolfSSL_EVP_PKEY_set1_EC_KEY(pkey, key);
                        }

                        if (ret == 0) {
                            derSz = wolfSSL_i2d_PrivateKey(pkey, &der);
                            if (derSz <= 0) {
                                printf("error converting to der\n");
                                ret = -1;
                            }
                        }

                        if (ret == 0 && derSz > 0) {
                            ret = wolfSSL_BIO_write(bioOut, der, derSz);
                            if (ret != derSz) {
                                printf("issue writing out data\n");
                                ret = -1;
                            }
                        }

                        if (der != NULL)
                            free(der);
                        wolfSSL_EVP_PKEY_free(pkey);
                    }
                }
                break;
            case PUB_ONLY:
                if (fmt == PEM_FORM) {
                    if (wolfSSL_PEM_write_bio_EC_PUBKEY(bioOut, key)
                            != WOLFSSL_SUCCESS) {
                        ret = -1;
                    }
                }
                else {
                    derSz = wc_EccPublicKeyDerSize(key->internal, 1);
                    if (derSz <= 0) {
                        printf("error getting der size\n");
                        ret = derSz;
                    }

                    if (ret == 0) {
                        der = (byte*)XMALLOC(derSz, HEAP_HINT,
                                DYNAMIC_TYPE_TMP_BUFFER);
                        if (der == NULL) {
                            ret = MEMORY_E;
                        }
                        else {
                            derSz = wc_EccPublicKeyToDer(key->internal, der,
                                    derSz, 1);
                            if (derSz < 0) {
                                XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                                ret = derSz;
                            }
                        }
                    }

                    if (ret > 0) {
                        ret = wolfSSL_BIO_write(bioOut, der, derSz);
                        if (ret != derSz) {
                            ret = -1;
                        }
                    }

                    if (der != NULL)
                        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                }
                break;
            default:
                printf("Invalid directive\n");
                ret = BAD_FUNC_ARG;
        }
    }

    wolfSSL_EC_KEY_free(key);
    wolfSSL_BIO_free(bioOut);

    if (ret > 0) {
        /* ret > 0 indicates a successful file write, set to zero for return */
        ret = 0;
    }

    return ret;
#else
    (void)rng;
    (void)fName;
    (void)directive;
    (void)fmt;

    return NOT_COMPILED_IN;
#endif /* HAVE_ECC */
}

int wolfCLU_genKey_ECC(WC_RNG* rng, char* fName, int directive, int fmt,
                       int keySz)
{
#ifdef HAVE_ECC
    ecc_key key;
    FILE*   file;
    int     ret;

    int   fNameSz     = XSTRLEN(fName);
    int   fExtSz      = 6;
    char  fExtPriv[6] = ".priv\0";
    char  fExtPub[6]  = ".pub\0\0";
    char* fOutNameBuf = NULL;
    
    #ifdef NO_AES
    /* use 16 bytes for AES block size */
    size_t maxDerBufSz = 4 * keySz * 16;
    #else
    size_t maxDerBufSz = 4 * keySz * AES_BLOCK_SIZE;
    #endif
    byte*  derBuf      = NULL;
    int    derBufSz    = -1;

    if (rng == NULL || fName == NULL)
        return BAD_FUNC_ARG;

    if (fmt == PEM_FORM) {
        printf("Der to Pem for rsa key not yet implemented\n");
        printf("FEATURE COMING SOON!\n");
        return FEATURE_COMING_SOON;
    }

    ret = wc_ecc_init_ex(&key, HEAP_HINT, INVALID_DEVID);
    if (ret != 0)
        return ret;
    ret = wc_ecc_make_key(rng, keySz, &key);
#if defined(WOLFSSL_ASYNC_CRYPT)
    /* @Audit: is this all correct? */
    ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
    if (ret != 0)
        return ret;

    /*
     * Output key(s) to file(s)
     */

    /* set up the file name output buffer */
    fOutNameBuf = (char*)XMALLOC(fNameSz + fExtSz, HEAP_HINT,
                                 DYNAMIC_TYPE_TMP_BUFFER);
    if (fOutNameBuf == NULL)
        return MEMORY_E;
    XMEMSET(fOutNameBuf, 0, fNameSz + fExtSz);
    XMEMCPY(fOutNameBuf, fName, fNameSz);

    derBuf = (byte*) XMALLOC(maxDerBufSz, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (derBuf == NULL) {
        XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return MEMORY_E;
    }

    switch(directive) {
        case PRIV_AND_PUB:
            /* Fall through to PRIV_ONLY */
        case PRIV_ONLY:
            /* add on the final part of the file name ".priv" */
            XMEMCPY(fOutNameBuf + fNameSz, fExtPriv, fExtSz);
            printf("fOutNameBuf = %s\n", fOutNameBuf);

            derBufSz = wc_EccPrivateKeyToDer(&key, derBuf, maxDerBufSz);
            if (derBufSz < 0) {
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return derBufSz;
            }

            file = fopen(fOutNameBuf, "wb");
            if (file == XBADFILE) {
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return OUTPUT_FILE_ERROR;
            }

            ret = (int)fwrite(derBuf, 1, derBufSz, file);
            if (ret <= 0) {
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                fclose(file);
                return OUTPUT_FILE_ERROR;
            }
            fclose(file);

            if (directive != PRIV_AND_PUB) {
                break;
            }
        case PUB_ONLY:
            /* add on the final part of the file name ".pub" */
            XMEMCPY(fOutNameBuf + fNameSz, fExtPub, fExtSz);
            printf("fOutNameBuf = %s\n", fOutNameBuf);

            derBufSz = wc_EccPublicKeyToDer(&key, derBuf, maxDerBufSz, 1);
            if (derBufSz < 0) {
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return derBufSz;
            }

            file = fopen(fOutNameBuf, "wb");
            if (file == XBADFILE) {
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return OUTPUT_FILE_ERROR;
            }

            ret = (int) fwrite(derBuf, 1, derBufSz, file);
            if (ret <= 0) {
                fclose(file);
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return OUTPUT_FILE_ERROR;
            }
            fclose(file);
            break;
        default:
            printf("Invalid directive\n");
            XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            return BAD_FUNC_ARG;
    }

    XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    wc_ecc_free(&key);

    if (ret > 0) {
        /* ret > 0 indicates a successful file write, set to zero for return */
        ret = 0;
    }

    return ret;
#else
    (void)rng;
    (void)fName;
    (void)directive;
    (void)fmt;
    (void)keySz;

    return NOT_COMPILED_IN;
#endif /* HAVE_ECC */
}

int wolfCLU_genKey_RSA(WC_RNG* rng, char* fName, int directive, int fmt, int
                       keySz, long exp)
{
#ifndef NO_RSA
    RsaKey key;
    FILE*  file;
    int    ret;

    int   fNameSz     = XSTRLEN(fName);
    int   fExtSz      = 6;
    char  fExtPriv[6] = ".priv\0";
    char  fExtPub[6]  = ".pub\0\0";
    char* fOutNameBuf = NULL;

    #ifdef NO_AES
    /* use 16 bytes for AES block size */
    size_t maxDerBufSz = 4 * keySz * 16;
    #else
    size_t maxDerBufSz = 4 * keySz * AES_BLOCK_SIZE;
    #endif
    byte*  derBuf      = NULL;
    int    derBufSz    = -1;

    if (rng == NULL || fName == NULL)
        return BAD_FUNC_ARG;

    if (fmt == PEM_FORM) {
        printf("Der to Pem for rsa key not yet implemented\n");
        printf("FEATURE COMING SOON!\n");
        return FEATURE_COMING_SOON;
    }

    ret = wc_InitRsaKey(&key, HEAP_HINT);
    if (ret != 0)
        return ret;
    ret = wc_MakeRsaKey(&key, keySz, exp, rng);
    if (ret != 0)
        return ret;

    /*
     * Output key(s) to file(s)
     */

    /* set up the file name output buffer */
    fOutNameBuf = (char*)XMALLOC(fNameSz + fExtSz, HEAP_HINT,
                                 DYNAMIC_TYPE_TMP_BUFFER);
    if (fOutNameBuf == NULL)
        return MEMORY_E;
    XMEMSET(fOutNameBuf, 0, fNameSz + fExtSz);
    XMEMCPY(fOutNameBuf, fName, fNameSz);

    derBuf = (byte*) XMALLOC(maxDerBufSz, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (derBuf == NULL) {
        XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return MEMORY_E;
    }

    switch(directive) {
        case PRIV_AND_PUB:
            /* Fall through to PRIV_ONLY */
        case PRIV_ONLY:
            /* add on the final part of the file name ".priv" */
            XMEMCPY(fOutNameBuf + fNameSz, fExtPriv, fExtSz);
            printf("fOutNameBuf = %s\n", fOutNameBuf);

            derBufSz = wc_RsaKeyToDer(&key, derBuf, maxDerBufSz);
            if (derBufSz < 0) {
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return derBufSz;
            }

            file = fopen(fOutNameBuf, "wb");
            if (file == XBADFILE) {
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return OUTPUT_FILE_ERROR;
            }

            ret = (int)fwrite(derBuf, 1, derBufSz, file);
            if (ret <= 0) {
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                fclose(file);
                return OUTPUT_FILE_ERROR;
            }
            fclose(file);

            if (directive != PRIV_AND_PUB) {
                break;
            }
        case PUB_ONLY:
            /* add on the final part of the file name ".pub" */
            XMEMCPY(fOutNameBuf + fNameSz, fExtPub, fExtSz);
            printf("fOutNameBuf = %s\n", fOutNameBuf);

            derBufSz = wc_RsaKeyToPublicDer(&key, derBuf, maxDerBufSz);
            if (derBufSz < 0) {
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return derBufSz;
            }

            file = fopen(fOutNameBuf, "wb");
            if (file == XBADFILE) {
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return OUTPUT_FILE_ERROR;
            }

            ret = (int) fwrite(derBuf, 1, derBufSz, file);
            if (ret <= 0) {
                fclose(file);
                XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                return OUTPUT_FILE_ERROR;
            }
            fclose(file);
            break;
        default:
            printf("Invalid directive\n");
            XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            return BAD_FUNC_ARG;
    }

    XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(fOutNameBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    wc_FreeRsaKey(&key);

    if (ret > 0) {
        /* ret > 0 indicates a successful file write, set to zero for return */
        ret = 0;
    }

    return ret;
#else
    (void)rng;
    (void)fName;
    (void)directive;
    (void)fmt;
    (void)keySz;
    (void)exp;

    return NOT_COMPILED_IN;
#endif
}

#endif /* WOLFSSL_KEY_GEN && !NO_ASN*/


/*
 * makes a cyptographically secure key by stretching a user entered pwdKey
 */
int wolfCLU_genKey_PWDBASED(WC_RNG* rng, byte* pwdKey, int size, byte* salt,
                            int pad)
{
    int ret;        /* return variable */

    /* randomly generates salt */

    ret = wc_RNG_GenerateBlock(rng, salt, SALT_SIZE-1);

    if (ret != 0)
        return ret;

    /* set first value of salt to let us know
     * if message has padding or not
     */
    if (pad == 0)
        salt[0] = 0;

    /* stretches pwdKey */
    ret = (int) wc_PBKDF2(pwdKey, pwdKey, (int) strlen((const char*)pwdKey),
                          salt, SALT_SIZE, CLU_4K_TYPE, size, CLU_SHA256);
    if (ret != 0)
        return ret;

    return 0;
}

