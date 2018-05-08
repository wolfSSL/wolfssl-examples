/* clu_request_setup.c
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
#include "clu_include/x509/clu_request.h"
#include "clu_include/certgen/clu_certgen.h"

int wolfCLU_requestSetup(int argc, char** argv)
{


    int     ret        =   0;   /* return variable, counter */
    int     i          =   0;   /* loop variable */
    char*   in;                 /* input variable */
    char*   out;                /* output variable */

    char*   alg;                /* algorithm being used */
    int     keyCheck =   0;     /* input check */
    int     algCheck =   0;     /* algorithm type */
    
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
        wolfCLU_certgenHelp();
        return FATAL_ERROR;
    }

    ret = wolfCLU_checkForArg("-in", 3, argc, argv);
    if (ret > 0) {
        in = XMALLOC(strlen(argv[ret+1]), HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (in == NULL) {
            return MEMORY_E;
        } else if (access(argv[ret+1], F_OK) == -1) {
            printf("Access: %s\n",argv[ret+1]);
            printf("In: %s\n", in);
            printf("Key file did not exist. Please check your options.\n");
            return MEMORY_E;
        }

        XSTRNCPY(in, &argv[ret+1][0], XSTRLEN(argv[ret+1]));
        in[XSTRLEN(argv[ret+1])] = '\0';
        keyCheck = 1;        
    }
    else {
        printf("Please specify a -key <key> option when "
               "generating a certificate.\n");
        wolfCLU_certgenHelp();
        return ret;
    }
    
    ret = wolfCLU_checkForArg("-out", 4, argc, argv);
    if (ret > 0) {
        /* output file */
        out = argv[ret+1];
    } else {
        printf("Please specify an output file path when generating a "
                                                      "certificate.\n");
            wolfCLU_certgenHelp();
            return ret;
    }

    if (keyCheck == 0) {
        printf("Must have input as either a file or standard I/O\n");
        return FATAL_ERROR;
    }
    
    // TODO remove hard coded values
    if (algCheck == 1) {
        ret = make_self_signed_rsa_certificate(in, out);
    } else if (algCheck == 2) {
        ret = make_self_signed_ed25519_certificate(in, out);
    } else if (algCheck == 3) {
        ret = make_self_signed_ecc_certificate(in, out);
    }
    
    XFREE(in, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}


