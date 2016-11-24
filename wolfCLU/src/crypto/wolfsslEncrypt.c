/* wolfsslEncrypt.c
 *
 * Copyright (C) 2006-2015 wolfSSL Inc.
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

#include "include/wolfssl.h"

#define SALT_SIZE       8
#define MAX             1024

int wolfsslEncrypt(char* alg, char* mode, byte* pwdKey, byte* key, int size,
        char* in, char* out, byte* iv, int block, int ivCheck, int inputHex)
{
#ifndef NO_AES
    Aes aes;                        /* aes declaration */
#endif

#ifndef NO_DES3
    Des3 des3;                      /* 3des declaration */
#endif

#ifdef HAVE_CAMELLIA
    Camellia camellia;              /* camellia declaration */
#endif

    FILE*  tempInFile = NULL;       /* if user not provide a file */
    FILE*  inFile = NULL;           /* input file */
    FILE*  outFile = NULL;          /* output file */

    RNG     rng;                    /* random number generator declaration */

    byte*   input = NULL;           /* input buffer */
    byte*   output = NULL;          /* output buffer */
    byte    salt[SALT_SIZE] = {0};  /* salt variable */

    int     ret             = 0;    /* return variable */
    int     inputLength     = 0;    /* length of input */
    int     length          = 0;    /* total length */
    int     padCounter      = 0;    /* number of padded bytes */
    int     i               = 0;    /* loop variable */
    int     hexRet          = 0;    /* hex -> bin return*/

    word32  tempInputL      = 0;    /* temporary input Length */
    word32  tempMax         = MAX;  /* controls encryption amount */

    char    inputString[MAX];       /* the input string */
    char*   userInputBuffer = NULL; /* buffer when input is not a file */


    if (access (in, F_OK) == -1) {
        printf("file did not exist, encrypting string following \"-i\""
                "instead.\n");

        /* use user entered data to encrypt */
        inputLength = (int) strlen(in);
        userInputBuffer = (char*) XMALLOC(inputLength, HEAP_HINT,
                                                       DYNAMIC_TYPE_TMP_BUFFER);
        if (userInputBuffer == NULL)
            return MEMORY_E;

        /* writes the entered text to the input buffer */
        XMEMCPY(userInputBuffer, in, inputLength);

        /* open the file to write */
        tempInFile = fopen(in, "wb");
        fwrite(userInputBuffer, 1, inputLength, tempInFile);
        fclose(tempInFile);

        /* free buffer */
        XFREE(userInputBuffer, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    }

    /* open the inFile in read mode */
    inFile = fopen(in, "rb");

    /* find length */
    fseek(inFile, 0, SEEK_END);
    inputLength = (int) ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    length = inputLength;

    /* Start up the random number generator */
    ret = (int) wc_InitRng(&rng);
    if (ret != 0) {
        printf("Random Number Generator failed to start.\n");
        return ret;
    }

    /* pads the length until it matches a block,
     * and increases pad number
     */
    while (length % block != 0) {
        length++;
        padCounter++;
    }

    /* if the iv was not explicitly set,
     * generate an iv and use the pwdKey
     */
    if (ivCheck == 0) {
        /* IV not set, generate it */
        ret = wc_RNG_GenerateBlock(&rng, iv, block);

        if (ret != 0) {
            return ret;
        }

        /* stretches pwdKey to fit size based on wolfsslGetAlgo() */
        ret = wolfsslGenKey(&rng, pwdKey, size, salt, padCounter);

        if (ret != 0) {
            printf("failed to set pwdKey.\n");
            return ret;
        }
        /* move the generated pwdKey to "key" for encrypting */
        for (i = 0; i < size; i++) {
            key[i] = pwdKey[i];
        }
    }

    /* open the outFile in write mode */
    outFile = fopen(out, "wb");
    fwrite(salt, 1, SALT_SIZE, outFile);
    fwrite(iv, 1, block, outFile);
    fclose(outFile);

    /* MALLOC 1kB buffers */
    input = (byte*) XMALLOC(MAX, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (input == NULL)
        return MEMORY_E;
    output = (byte*) XMALLOC(MAX, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (output == NULL) {
        wolfsslFreeBins(input, NULL, NULL, NULL, NULL);
        return MEMORY_E;
    }

    /* loop, encrypt 1kB at a time till length <= 0 */
    while (length > 0) {
        /* Read in 1kB to input[] */
        if (inputHex == 1)
            ret = (int) fread(inputString, 1, MAX, inFile);
        else
            ret = (int) fread(input, 1, MAX, inFile);

        if (ret != MAX) {
            /* check for end of file */
            if (feof(inFile)) {

                /* hex or ascii */
                if (inputHex == 1) {
                    hexRet = wolfsslHexToBin(inputString, &input,
                                                &tempInputL,
                                                NULL, NULL, NULL,
                                                NULL, NULL, NULL,
                                                NULL, NULL, NULL);
                     if (hexRet != 0) {
                        printf("failed during conversion of input,"
                            " ret = %d\n", hexRet);
                        return hexRet;
                    }
                }/* end hex or ascii */

                /* pad to end of block */
                for (i = ret ; i < (ret + padCounter); i++) {
                    input[i] = padCounter;
                }
                /* adjust tempMax for less than 1kB encryption */
                tempMax = ret + padCounter;
            }
            else { /* otherwise we got a file read error */
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return FREAD_ERROR;
            }/* End feof check */
        }/* End fread check */

        /* sets key encrypts the message to ouput from input */
#ifndef NO_AES
        if (XSTRNCMP(alg, "aes", 3) == 0) {
            if (XSTRNCMP(mode, "cbc", 3) == 0) {
                ret = wc_AesSetKey(&aes, key, AES_BLOCK_SIZE, iv, AES_ENCRYPTION);
                if (ret != 0) {
                    printf("wc_AesSetKey failed.\n");
                    wolfsslFreeBins(input, output, NULL, NULL, NULL);
                    return ret;
                }
                ret = wc_AesCbcEncrypt(&aes, output, input, tempMax);
                if (ret != 0) {
                    printf("wc_AesCbcEncrypt failed.\n");
                    wolfsslFreeBins(input, output, NULL, NULL, NULL);
                    return ENCRYPT_ERROR;
                }
            }
#ifdef WOLFSSL_AES_COUNTER
            else if (XSTRNCMP(mode, "ctr", 3) == 0) {
                /* if mode is ctr */
                wc_AesSetKeyDirect(&aes, key, AES_BLOCK_SIZE, iv, AES_ENCRYPTION);
                wc_AesCtrEncrypt(&aes, output, input, tempMax);
            }
#endif
        }
#endif
#ifndef NO_DES3
        if (XSTRNCMP(alg, "3des", 4) == 0) {
            ret = wc_Des3_SetKey(&des3, key, iv, DES_ENCRYPTION);
            if (ret != 0) {
                printf("wc_Des3_SetKey failed.\n");
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return ret;
            }
            ret = wc_Des3_CbcEncrypt(&des3, output, input, tempMax);
            if (ret != 0) {
                printf("wc_Des3_cbcEncrypt failed.\n");
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return ENCRYPT_ERROR;
            }
        }
#endif
#ifdef HAVE_CAMELLIA
        if (XSTRNCMP(alg, "camellia", 8) == 0) {
            ret = wc_CamelliaSetKey(&camellia, key, block, iv);
            if (ret != 0) {
                printf("CamelliaSetKey failed.\n");
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return ret;
            }
            if (XSTRNCMP(mode, "cbc", 3) == 0) {
                wc_CamelliaCbcEncrypt(&camellia, output, input, tempMax);
            }
            else {
                printf("Incompatible mode while using Camellia.\n");
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return FATAL_ERROR;
            }
        }
#endif /* HAVE_CAMELLIA */

        /* this method added for visual confirmation of nist test vectors,
         * automated tests to come soon
         */

        /* something in the output buffer and using hex */
        if (output != NULL && inputHex == 1) {
            int tempi;

            printf("\nUser specified hex input this is a representation of "
                "what\nis being written to file in hex form.\n\n[ ");
            for (tempi = 0; tempi < block; tempi++ ) {
                printf("%02x", output[tempi]);
            }
            printf(" ]\n\n");
        } /* end visual confirmation */

        /* Open the outFile in append mode */
        outFile = fopen(out, "ab");
        ret = (int) fwrite(output, 1, tempMax, outFile);

        if (ferror(outFile)) {
            printf("failed to write to file.\n");
            if (input != NULL)
                XMEMSET(input, 0, tempMax);
            if (output != NULL)
                XMEMSET(output, 0, tempMax);
            wolfsslFreeBins(input, output, NULL, NULL, NULL);
            return FWRITE_ERROR;
        }
        if (ret > MAX) {
            printf("Wrote too much to file.\n");
            if (input != NULL)
                XMEMSET(input, 0, tempMax);
            if (output != NULL)
                XMEMSET(output, 0, tempMax);
            wolfsslFreeBins(input, output, NULL, NULL, NULL);
            return FWRITE_ERROR;
        }
        /* close the outFile */
        fclose(outFile);

        length -= tempMax;
        if (length < 0)
            printf("length went past zero.\n");
        if (input != NULL)
            XMEMSET(input, 0, tempMax);
        if (output != NULL)
            XMEMSET(output, 0, tempMax);
    }

    /* closes the opened files and frees the memory */
    fclose(inFile);
    XMEMSET(key, 0, size);
    XMEMSET(iv, 0 , block);
    XMEMSET(alg, 0, size);
    XMEMSET(mode, 0 , block);
    /* Use the wolfssl free for rng */
    wc_FreeRng(&rng);
    wolfsslFreeBins(input, output, NULL, NULL, NULL);
    return 0;
}
