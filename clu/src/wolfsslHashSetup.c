/* wolfsslHashSetup.c
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

/*
 * hash argument function
 */
int wolfsslHashSetup(int argc, char** argv)
{
    int     ret        =   0;   /* return variable, counter */
    int     i          =   0;   /* loop variable */
    char*   in;                 /* input variable */
    char*   out     =   NULL;   /* output variable */
    const char* algs[]  =   {   /* list of acceptable algorithms */
#ifndef NO_MD5
        "md5"
#endif
#ifndef NO_SHA
            , "sha"
#endif
#ifndef NO_SHA256
            , "sha256"
#endif
#ifdef CYASSL_SHA384
            , "sha384"
#endif
#ifdef CYASSL_SHA512
            , "sha512"
#endif
#ifdef HAVE_BLAKE2
            , "blake2b"
#endif
    };

    char*   alg;                /* algorithm being used */
    int     algCheck=   0;      /* acceptable algorithm check */
    int     inCheck =   0;      /* input check */
    int     size    =   0;      /* message digest size */

#ifdef HAVE_BLAKE2
    size = BLAKE_DIGEST_SIZE;
#endif

    /* help checking */
    if (argc == 2) {
        wolfsslHashHelp();
        return 0;
    }
    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-help") == 0) {
            wolfsslHashHelp();
            return 0;
        }
    }

    for (i = 0; i < (int) sizeof(algs)/(int) sizeof(algs[0]); i++) {
        /* checks for acceptable algorithms */
        if (strcmp(argv[2], algs[i]) == 0) {
            alg = argv[2];
            algCheck = 1;
        }
    }
    if (algCheck == 0) {
        printf("Invalid algorithm\n");
        return FATAL_ERROR;
    }

    for (i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && argv[i+1] != NULL) {
            /* input file/text */
            in = malloc(strlen(argv[i+1])+1);
            XSTRNCPY(in, &argv[i+1][0], XSTRLEN(&argv[i+1][0]));
            in[strlen(argv[i+1])] = '\0';
            inCheck = 1;
            i++;
        }
        else if (strcmp(argv[i], "-o") == 0 && argv[i+1] != NULL) {
            /* output file */
            out = argv[i+1];
            i++;
        }
        else if (strcmp(argv[i], "-s") == 0 && argv[i+1] != NULL) {
            /* size of output */
#ifndef HAVE_BLAKE2
            printf("Sorry, only to be used with Blake2b enabled\n");
#else
            size = atoi(argv[i+1]);
            if (size <= 0 || size > 64) {
                printf("Invalid size, Must be between 1-64. Using default.\n");
                size = BLAKE_DIGEST_SIZE;
            }
#endif
            i++;
        }
        else {
            printf("Unknown argument %s. Ignoring\n", argv[i]);
        }
    }
    if (inCheck == 0) {
        printf("Must have input as either a file or standard I/O\n");
        return FATAL_ERROR;
    }
    /* sets default size of algorithm */
#ifndef NO_MD5
    if (strcmp(alg, "md5") == 0)
        size = MD5_DIGEST_SIZE;
#endif

#ifndef NO_SHA
    if (strcmp(alg, "sha") == 0)
        size = SHA_DIGEST_SIZE;
#endif

#ifndef NO_SHA256
    if (strcmp(alg, "sha256") == 0)
        size = SHA256_DIGEST_SIZE;
#endif

#ifdef CYASSL_SHA384
    if (strcmp(alg, "sha384") == 0)
        size = SHA384_DIGEST_SIZE;
#endif

#ifdef CYASSL_SHA512
    if (strcmp(alg, "sha512") == 0)
        size = SHA512_DIGEST_SIZE;
#endif

    /* hashing function */
    wolfsslHash(in, out, alg, size);

    free(in);

    return ret;
}
