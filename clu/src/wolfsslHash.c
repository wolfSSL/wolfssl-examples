/* wolfsslHash.c
 *
 * Copyright (C) 2006-2014 wolfSSL Inc.
 * This file is part of CyaSSL.
 *
 * CyaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CyaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,USA
 */

#include "include/wolfssl.h"

#define LENGTH_IN       (int)strlen(in)      /* type cast unsigned int to int */

/*
 * hashing function
 */
int wolfsslHash(char* in, char* out, char* alg, int size)
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

    output = malloc(size);
    memset(output, 0, size);

    /* opens input file */
    inFile = fopen(in, "rb");
    if (inFile == NULL) {
        /* if no input file was provided */
        length = LENGTH_IN;

        input = malloc(length);
        memset(input, 0, length);
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

        input = malloc(length+1);
        memset(input, 0, length+1);
        if (input == NULL) {
            printf("Failed to create input buffer\n");
            return FATAL_ERROR;
        }
        ret = (int) fread(input, 1, length, inFile);
        fclose(inFile);
    }
    /* hashes using accepted algorithm */
#ifndef NO_MD5
    if (strcmp(alg, "md5") == 0) {
        ret = Md5Hash(input, length, output);
    }
#endif
#ifndef NO_SHA
    else if (strcmp(alg, "sha") == 0) {
        ret = ShaHash(input, length, output);
    }
#endif
#ifndef NO_SHA256
    else if (strcmp(alg, "sha256") == 0) {
        ret = Sha256Hash(input, length, output);
    }
#endif
#ifdef CYASSL_SHA384
    else if (strcmp(alg, "sha384") == 0) {
        ret = Sha384Hash(input, length, output);
    }
#endif
#ifdef CYASSL_SHA512
    else if (strcmp(alg, "sha512") == 0) {
        ret = Sha512Hash(input, length, output);
    }
#endif
#ifdef HAVE_BLAKE2
    else if (strcmp(alg, "blake2b") == 0) {
        ret = InitBlake2b(&hash, size);
        ret = Blake2bUpdate(&hash, input, length);
        ret = Blake2bFinal(&hash, output, size);
    }
#endif
    if (ret == 0) {
        /* if no errors so far */
        if (out != NULL) {
            /* if output file provided */
            outFile = fopen(out, "wb");
            if (outFile != NULL) {
                /* if outFile exists */
                for (i = 0; i < size; i++) {
                    /* writes hashed output to outFile */
                    fprintf(outFile, "%02x", output[i]);
                }
                fclose(outFile);
            }
        }
        else {
            /*  if no output file */
            for (i = 0; i < size; i++) {
                /* write hashed output to terminal */
                printf("%02x", output[i]);
            }
            printf("\n");
        }
    }

    /* closes the opened files and frees the memory */
    memset(input, 0, length);
    memset(output, 0, size);
    free(input);
    free(output);
    return ret;
}
