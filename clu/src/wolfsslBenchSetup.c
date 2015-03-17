/* wolfsslBenchSetup.c
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

int wolfsslBenchSetup(int argc, char** argv)
{
    int     ret     =   0;          /* return variable */
    int     time    =   3;          /* timer variable */
    int     i, j    =   0;          /* second loop variable */
    const char*   algs[]  =   {     /* list of acceptable algorithms */
#ifndef NO_AES
        "aes-cbc"
#endif
#ifdef CYASSL_AES_COUNTER
            , "aes-ctr"
#endif
#ifndef NO_DES3
            , "3des"
#endif
#ifdef HAVE_CAMELLIA
            , "camellia"
#endif
#ifndef NO_MD5
            , "md5"
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

    int option[sizeof(algs)/sizeof(algs[0])] = {0};/* acceptable options */
    int optionCheck = 0;                           /* acceptable option check */

    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-help") == 0) {
            /* help checking */
            wolfsslBenchHelp();
            return 0;
        }
        for (j = 0; j < (int) sizeof(algs)/(int) sizeof(algs[0]); j++) {
            /* checks for individual tests in the arguments */
            if (strcmp(argv[i], algs[j]) == 0) {
                option[j] = 1;    
                optionCheck = 1;
            }
        }
        if (strcmp(argv[i], "-t") == 0 && argv[i+1] != NULL) {
            /* time for each test in seconds */
            time = atoi(argv[i+1]);
            if (time < 1 || time > 10) {
                printf("Invalid time, must be between 1-10. Using default.\n");
                time = 3;
            }
            i++;
        }
        if (strcmp(argv[i], "-a") == 0) {
            /* perform all available tests */
            for (j = 0; j < (int) sizeof(algs)/(int) sizeof(algs[0]); j++) {
                option[j] = 1;
                optionCheck = 1;
            }
        }
    }
    if (optionCheck != 1) {
        /* help checking */
        wolfsslHelp();
    }
    else {
        /* benchmarking function */
        printf("\nTesting for %d second(s)\n", time);
        ret = wolfsslBenchmark(time, option);
    }
    return ret;
}
