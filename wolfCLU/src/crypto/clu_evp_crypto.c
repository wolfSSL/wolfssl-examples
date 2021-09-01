/* clu_evp_crypto.c
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
#include <wolfclu/genkey/clu_genkey.h>

#ifndef WOLFCLU_MAX_BUFFER
#define WOLFCLU_MAX_BUFFER 1024
#endif


int wolfCLU_evp_crypto(const WOLFSSL_EVP_CIPHER* cphr, char* mode, byte* pwdKey,
        byte* key, int keySz, char* fileIn, char* fileOut, char* hexIn,
        byte* iv, int hexOut, int enc, int pbkVersion,
        const WOLFSSL_EVP_MD* hashType, int printOut)
{
    WOLFSSL_BIO *out = NULL;
    WOLFSSL_BIO *in  = NULL;
    WOLFSSL_EVP_CIPHER_CTX* ctx    = NULL;

    WC_RNG     rng;                 /* random number generator declaration */

    byte*   input = NULL;           /* input buffer */
    byte*   output = NULL;          /* output buffer */
    byte    salt[SALT_SIZE] = {0};  /* salt variable */

    int     ret             = 0;    /* return variable */
    int     length          = 0;    /* total length */
    int     hexRet          = 0;    /* hex -> bin return*/
    int     ivSz            = 0;
    int     outputSz        = 0;
    int     iter            = 10000; /* default value for interop */

    word32  tempInputL      = 0;    /* temporary input Length */
    word32  tempMax         = WOLFCLU_MAX_BUFFER; /* controls encryption amount */

    char    inputString[WOLFCLU_MAX_BUFFER];       /* the input string */
    const char isSalted[] = "Salted__";

    if (cphr == NULL) {
        return BAD_FUNC_ARG;
    }

    /* open the inFile in read mode */
    if (fileIn != NULL) {
        in = wolfSSL_BIO_new_file(fileIn, "rb");
    }
    else {
        /* read hex from string instead */
        in = wolfSSL_BIO_new_mem_buf(hexIn, strlen(hexIn));
    }
    if (in == NULL) {
        printf("unable to open file %s\n", fileIn);
        return -1;
    }

    /* find length */
    length = wolfSSL_BIO_get_len(in);

    /* Start up the random number generator */
    if (ret >= 0) {
        ret = (int) wc_InitRng(&rng);
        if (ret != 0) {
            printf("Random Number Generator failed to start.\n");
        }
    }

    if (ret >= 0) {
        ctx = wolfSSL_EVP_CIPHER_CTX_new();
        if (ctx == NULL) {
            printf("Unable to create new ctx\n");
            ret = MEMORY_E;
        }
    }

    if (ret >= 0) {
        /* stretches pwdKey to fit size based on wolfCLU_getAlgo() */
        ivSz = wolfSSL_EVP_CIPHER_iv_length(cphr);
        if (enc) {
            /* randomly generates salt */
            ret = wc_RNG_GenerateBlock(&rng, salt, SALT_SIZE);
            if (ret != 0) {
                printf("Error creating salt\n");
                ret = -1;
            }
        }
        else {
            char tmp[sizeof(isSalted)];

            wolfSSL_BIO_read(in, tmp, XSTRLEN(isSalted));
            tmp[XSTRLEN(isSalted)] = '\0';
            if (XMEMCMP(tmp, isSalted, XSTRLEN(isSalted)) != 0) {
                printf("Was expecting salt\n");
                ret = -1;
            }

            if (ret >= 0) {
                ret = wolfSSL_BIO_read(in, salt, SALT_SIZE);
                if (ret != SALT_SIZE) {
                    printf("error reading salt\n");
                    ret = -1;
                }
            }
        }
    }

    /* stretches pwdKey */
    if (ret >= 0) {
        if (pbkVersion == WOLFCLU_PBKDF2) {
        #ifdef HAVE_FIPS
            if (strlen((const char*)pwdKey) < HMAC_FIPS_MIN_KEY) {
                printf("For use with FIPS mode key needs to be at least %d "
                        "characters long\n", HMAC_FIPS_MIN_KEY);
                ret = -1;
            }
        #endif
            if (ret >= 0) {
                ret = wolfSSL_PKCS5_PBKDF2_HMAC((const char*)pwdKey,
                    (int) strlen((const char*)pwdKey), salt, SALT_SIZE, iter,
                    hashType, keySz + ivSz, pwdKey);
                if (ret != 0) {
                    printf("failed to create key, ret = %d\n", ret);
                    ret = -1;
                }
            }

            if (ret >= 0) {
                /* move the generated pwdKey to "key" for encrypting */
                XMEMCPY(key, pwdKey, keySz);
                XMEMCPY(iv, pwdKey + keySz, ivSz);
            }
        }
        else {
            printf("WARNING: Using old version of PBKDF!!!!\n");
            iter = 1; /* default value for interop */
            ret = wolfSSL_EVP_BytesToKey(cphr, hashType, salt,
                    pwdKey, (int)strlen((const char*)pwdKey), iter, key, iv);
            if (ret == 0) {
                printf("failed to create key, ret = %d\n", ret);
                ret = -1;
            }
        }
    }

    /* open the outFile in write mode */
    if (ret >= 0) {
        if (fileOut != NULL) {
            out = wolfSSL_BIO_new_file(fileOut, "wb");
        }
        else {
            /* write to stdout if no file provided  */
            out = wolfSSL_BIO_new(wolfSSL_BIO_s_file());
            wolfSSL_BIO_set_fp(out, stdout, BIO_NOCLOSE);
        }
        if (out == NULL) {
            printf("unable to open output file %s\n", fileOut);
            ret = -1;
        }
    }

    /* when encrypting a file write out the salt value generated */
    if (ret >= 0 && enc) {
        wolfSSL_BIO_write(out, isSalted, XSTRLEN(isSalted));
        wolfSSL_BIO_write(out, salt, SALT_SIZE);
    }

    if (printOut) {
        int z;
        printf("salt [%d] :", SALT_SIZE);
        for (z = 0; z < SALT_SIZE; z++)
            printf("%02X", salt[z]);
        printf("\n");
        printf("key  [%d] :", keySz);
        for (z = 0; z < keySz; z++)
            printf("%02X", key[z]);
        printf("\n");
        printf("iv   [%d] :", ivSz);
        for (z = 0; z < ivSz; z++)
            printf("%02X", iv[z]);
        printf("\n");
        printf("itterations = %d\n", iter);
        printf("PBKDF version = %d\n", pbkVersion);
    }


    if (ret >= 0) {
        wolfSSL_EVP_CIPHER_CTX_init(ctx);
        if (wolfSSL_EVP_CipherInit(ctx, cphr, key, iv, enc) != WOLFSSL_SUCCESS){
            printf("failed to init evp ctx\n");
            ret = MEMORY_E;
        }
    }

    /* MALLOC 1kB buffers */
    if (ret >= 0) {
        input = (byte*)XMALLOC(WOLFCLU_MAX_BUFFER, HEAP_HINT,
                DYNAMIC_TYPE_TMP_BUFFER);
        if (input == NULL)
            ret = MEMORY_E;
    }

    if (ret >= 0) {
        output = (byte*)XMALLOC(WOLFCLU_MAX_BUFFER, HEAP_HINT,
                DYNAMIC_TYPE_TMP_BUFFER);
        if (output == NULL) {
            ret = MEMORY_E;
        }
    }

    /* loop, encrypt 1kB at a time till length <= 0 */
    while (length > 0) {
        /* Read in 1kB to input[] */
        ret = wolfSSL_BIO_read(in, input, WOLFCLU_MAX_BUFFER);
        if (ret < 0) {
            printf("error reading in data\n");
            ret = -1;
        }
        if (ret <= 0) {
            break; /* hit end of buffer or error */
        }

        if (ret >= 0 && hexIn) {
            hexRet = wolfCLU_hexToBin(inputString, &input, &tempInputL,
                                                NULL, NULL, NULL,
                                                NULL, NULL, NULL,
                                                NULL, NULL, NULL);
            if (hexRet != 0) {
                printf("failed during conversion of input, ret = %d\n", hexRet);
                ret = -1;
            }
        }
        
        if (ret >= 0) {
            tempMax = ret;
            outputSz = WOLFCLU_MAX_BUFFER;
            if (wolfSSL_EVP_CipherUpdate(ctx, output, &outputSz, input, tempMax)
                    != WOLFSSL_SUCCESS) {
                printf("Error with cipher update\n");
                ret = -1;
            }
        }

        if (ret >= 0) {
            ret = wolfSSL_BIO_write(out, output, outputSz);
            if (ret < 0) {
                printf("error writing out encrypted data\n");
                ret = -1;
            }
        }

        if (ret >= 0) {
            length -= ret;
            if (input != NULL)
                XMEMSET(input, 0, tempMax);
            if (output != NULL)
                XMEMSET(output, 0, tempMax);
        }
    }

    if (ret >= 0) {
        /* flush out last block (could have padding) */
        outputSz = tempMax;
        if (wolfSSL_EVP_CipherFinal(ctx, output, &outputSz)
                != WOLFSSL_SUCCESS) {
            printf("Error decrypting message\n");
            ret = -1;
        }
    }

    if (ret >= 0) {
        wolfSSL_BIO_write(out, output, outputSz);
    }

    /* closes the opened files and frees the memory */
    wolfSSL_BIO_free(out);
    wolfSSL_BIO_free(in);

    XMEMSET(key, 0, keySz);
    XMEMSET(iv, 0 , ivSz);

    wc_FreeRng(&rng);
    wolfCLU_freeBins(input, output, NULL, NULL, NULL);
    wolfSSL_EVP_CIPHER_CTX_free(ctx);
    return 0;
}

