/* clu_bench_setup.c
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
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

int wolfCLU_benchSetup(int argc, char** argv)
{
    int     ret     =   0;          /* return variable */
    int     time    =   3;          /* timer variable */
    int     i, j    =   0;          /* second loop variable */
    const char*   algs[]  =   {     /* list of acceptable algorithms */
#ifndef NO_AES
        "aes-cbc"
#endif
#ifdef WOLFSSL_AES_COUNTER
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
#ifdef WOLFSSL_SHA384
            , "sha384"
#endif
#ifdef WOLFSSL_SHA512
            , "sha512"
#endif
#ifdef HAVE_BLAKE2
            , "blake2b"
#endif
    };

    int option[sizeof(algs)/sizeof(algs[0])] = {0};/* acceptable options */
    int optionCheck = 0;                           /* acceptable option check */

    for (i = 2; i < argc; i++) {
        if (XSTRNCMP(argv[i], "-help", 5) == 0 || XSTRNCMP(argv[i], "-h", 2)
                                                                         == 0) {
            /* help checking */
            wolfCLU_benchHelp();
            return 0;
        }
        for (j = 0; j < (int) sizeof(algs)/(int) sizeof(algs[0]); j++) {
            /* checks for individual tests in the arguments */
            if (XSTRNCMP(argv[i], algs[j], XSTRLEN(argv[i])) == 0) {
                option[j] = 1;
                optionCheck = 1;
            }
        }
        if (XSTRNCMP(argv[i], "-time", 5) == 0 && argv[i+1] != NULL) {
            /* time for each test in seconds */
            time = atoi(argv[i+1]);
            if (time < 1 || time > 10) {
                printf("Invalid time, must be between 1-10. Using default"
                                                " of three seconds.\n");
                time = 3;
            }
            i++;
        }
        if (XSTRNCMP(argv[i], "-all", 4) == 0) {
            /* perform all available tests */
            for (j = 0; j < (int) sizeof(algs)/(int) sizeof(algs[0]); j++) {
                option[j] = 1;
                optionCheck = 1;
            }
        }
    }
    if (optionCheck != 1) {
        /* help checking */
        wolfCLU_help();
    }
    else {
        /* benchmarking function */
        printf("\nTesting for %d second(s)\n", time);
        ret = wolfCLU_benchmark(time, option);
    }
    return ret;
}
