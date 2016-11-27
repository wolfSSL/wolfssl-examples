/* clu_hash.c
 *
 * Copyright (C) 2006-2016 wolfSSL Inc.
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

#define LENGTH_IN       (int)strlen(in)      /* type cast unsigned int to int */

/*
 * hashing function
 */
int wolfCLU_hash(char* in, char* out, char* alg, int size)
{
#ifdef HAVE_BLAKE2
    Blake2b hash;               /* blake2b declaration */
#endif
    FILE*   inFile;             /* input file */
    FILE*   outFile;            /* output file */

    byte*   input;              /* input buffer */
    byte*   output;             /* output buffer */

    int     i  =   0;           /* loop variable */
    int     ret = -1;           /* return variable */
    int     length;             /* length of hash */
    int     outputAsHexString = 1;

    /* opens input file */
    inFile = fopen(in, "rb");
    if (inFile == NULL) {
        /* if no input file was provided */
        length = LENGTH_IN;

        input = XMALLOC(length, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (input == NULL)
            return MEMORY_E;

        XMEMSET(input, 0, length);
        for (i = 0; i < length; i++) {
            /* copies text from in to input */
            if (i <= LENGTH_IN ) {
                input[i] = in[i];
            }
        }
    }
    else {
        /* if input file provided finds end of file for length */
        fseek(inFile, 0, SEEK_END);
        int leng = (int) ftell(inFile);
        fseek(inFile, 0, SEEK_SET);

        length = leng;

        input = XMALLOC(length+1, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (input == NULL)
            return MEMORY_E;

        XMEMSET(input, 0, length+1);
        if (input == NULL) {
            printf("Failed to create input buffer\n");
            return FATAL_ERROR;
        }
        ret = (int) fread(input, 1, length, inFile);
        fclose(inFile);
    }

    /* if size not provided then use input length */
    if (size == 0) {
        size = length * 4;
    }

    output = XMALLOC(size, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (output == NULL) {
        wolfCLU_freeBins(input, NULL, NULL, NULL, NULL);
        return MEMORY_E;
    }
    XMEMSET(output, 0, size);

    /* hashes using accepted algorithm */
#ifndef NO_MD5
    if (strcmp(alg, "md5") == 0) {
        ret = wc_Md5Hash(input, length, output);
    }
#endif
#ifndef NO_SHA
    else if (strcmp(alg, "sha") == 0) {
        ret = wc_ShaHash(input, length, output);
    }
#endif
#ifndef NO_SHA256
    else if (strcmp(alg, "sha256") == 0) {
        ret = wc_Sha256Hash(input, length, output);
    }
#endif
#ifdef WOLFSSL_SHA384
    else if (strcmp(alg, "sha384") == 0) {
        ret = wc_Sha384Hash(input, length, output);
    }
#endif
#ifdef WOLFSSL_SHA512
    else if (strcmp(alg, "sha512") == 0) {
        ret = wc_Sha512Hash(input, length, output);
    }
#endif
#ifdef HAVE_BLAKE2
    else if (strcmp(alg, "blake2b") == 0) {
        ret = wc_InitBlake2b(&hash, size);
        if (ret != 0) return ret;
        ret = wc_Blake2bUpdate(&hash, input, length);
        if (ret != 0) return ret;
        ret = wc_Blake2bFinal(&hash, output, size);
        if (ret != 0) return ret;
    }
#endif

#ifndef NO_CODING
#ifdef WOLFSSL_BASE64_ENCODE
    else if (strcmp(alg, "base64enc") == 0) {
        ret = Base64_Encode(input, length, output, (word32*)&size);
        outputAsHexString = 0;
    }
#endif /* WOLFSSL_BASE64_ENCODE */
    else if (strcmp(alg, "base64dec") == 0) {
        ret = Base64_Decode(input, length, output, (word32*)&size);
        outputAsHexString = 0;
    }
#endif /* !NO_CODING */

    if (ret == 0) {
        /* if no errors so far */
        if (out != NULL) {
            /* if output file provided */
            outFile = fopen(out, "wb");
            if (outFile != NULL) {
                /* if outFile exists */
                for (i = 0; i < size; i++) {
                    /* writes hashed output to outFile */
                    if (outputAsHexString)
                        fprintf(outFile, "%02x", output[i]);
                    else
                        fprintf(outFile, "%c", output[i]);
                }
                fclose(outFile);
            }
        }
        else {
            /*  if no output file */
            for (i = 0; i < size; i++) {
                /* write hashed output to terminal */
                if (outputAsHexString)
                    printf("%02x", output[i]);
                else
                    printf("%c", output[i]);
            }
            printf("\n");
        }
    }

    /* closes the opened files and frees the memory */
    XMEMSET(input, 0, length);
    XMEMSET(output, 0, size);
    wolfCLU_freeBins(input, output, NULL, NULL, NULL);
    return ret;
}
