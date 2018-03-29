/* clu_sign_verify_setup.c
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
#include "clu_include/sign-verify/clu_sign.h"
#include "clu_include/sign-verify/clu_verify.h"

int wolfCLU_sign_verify_setup(int argc, char** argv)
{
    int     ret        =   0;   /* return variable, counter */
    int     i          =   0;   /* loop variable */
    char*   in;                 /* input variable */
    char*   out;                /* output variable */
    char*   priv;               /* private key variable */
    char*   sig;

    char*   alg;                /* algorithm being used */
    int     algCheck=   -1;      /* acceptable algorithm check */
    int     inCheck =   0;      /* input check */
    int     privCheck = 0;      /* private key check */
    int     signCheck = 0;
    int     verifyCheck = 0;
    int     pubInCheck = 0;
    int     sigCheck = 0;
    
    /* help checking
    ret = wolfCLU_checkForArg("-help", 5, argc, argv);
    if (ret > 0) {
        wolfCLU_hashHelp();
        return 0;
    }
    */
    
    if (wolfCLU_checkForArg("-rsa", 4, argc, argv) > 0) {
        algCheck = RSA_SIGN;
    } else if (wolfCLU_checkForArg("-ed25519", 8, argc, argv) > 0) {
        algCheck = ED25519_SIGN;
    } else if (wolfCLU_checkForArg("-ecc", 4, argc, argv) > 0) {
        algCheck = ECC_SIGN;
    } else {
        return FATAL_ERROR;
    }
    
    ret = wolfCLU_checkForArg("-sign", 5, argc, argv);
    if (ret > 0) {
        /* output file */
        signCheck = 1;
    }
    
    ret = wolfCLU_checkForArg("-verify", 7, argc, argv);
    if (ret > 0) {
        /* output file */
        verifyCheck = 1;
    }

    ret = wolfCLU_checkForArg("-inkey", 6, argc, argv);
    if (ret > 0) {
        priv = XMALLOC(strlen(argv[ret+1]), HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (priv == NULL) {
            return MEMORY_E;
        } else if (access(argv[ret+1], F_OK) == -1) {
            printf("Inkey file did not exist. Please check your options.\n");
            return MEMORY_E;
        }

        XSTRNCPY(priv, &argv[ret+1][0], XSTRLEN(argv[ret+1]));
        priv[XSTRLEN(argv[ret+1])] = '\0';
        privCheck = 1;        
    }
    else {
        printf("Please specify an -inkey <key> option when "
               "signing or verifying.\n");
        return ret;
    }
    
    ret = wolfCLU_checkForArg("-pubin", 6, argc, argv);
    if (ret > 0) {
        /* output file */
        pubInCheck = 1;
    }
    
    ret = wolfCLU_checkForArg("-in", 3, argc, argv);
    if (ret > 0) {
        /* input file/text */
        in = XMALLOC(strlen(argv[ret+1]), HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (in == NULL) {
            return MEMORY_E;
        } else if (access(argv[ret+1], F_OK) == -1) {
            printf("In file did not exist. Please check your options.\n");
            return MEMORY_E;
        }

        XSTRNCPY(in, &argv[ret+1][0], XSTRLEN(argv[ret+1]));
        in[XSTRLEN(argv[ret+1])] = '\0';
        inCheck = 1;
    }
    
    ret = wolfCLU_checkForArg("-signature", 10, argc, argv);
    if (ret > 0) {
        sig = XMALLOC(strlen(argv[ret+1]), HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (sig == NULL) {
            return MEMORY_E;
        } else if (access(argv[ret+1], F_OK) == -1) {
            printf("Signature file did not exist. Please check your options.\n");
            return MEMORY_E;
        }

        XSTRNCPY(sig, &argv[ret+1][0], XSTRLEN(argv[ret+1]));
        sig[XSTRLEN(argv[ret+1])] = '\0';
        sigCheck = 1;
    }
    else if (verifyCheck == 1 && algCheck != RSA_SIGN) {
        printf("Please specify -signature <sig> when verifying"
               " with ECC or ED25519.\n");
        return ret;
    }
    
    ret = wolfCLU_checkForArg("-out", 4, argc, argv);
    if (ret > 0) {
        /* output file */
        out = argv[ret+1];
    } else {
        if(algCheck == RSA_SIGN) {
            printf("Please specify an output file when "
                   "signing or verifing with RSA.\n");
            return ret;
        }
        else if (algCheck == ECC_SIGN && verifyCheck == 0) {
            printf("Please specify an output file when "
                   "signing with ECC.\n");
            return ret;
        }
        else {
            /* No out needed for ECC verifying */
            /* ED25519 exceptions will need to be added at a later date */ 
        }
    }

    if (inCheck == 0) {
        printf("Must have input as either a file or standard I/O\n");
        return FATAL_ERROR;
    }
    
    if (signCheck == 1) {
        ret = wolfCLU_sign_data(in, out, priv, algCheck);
    }
    else if (verifyCheck == 1) {
        ret = wolfCLU_verify_signature(sig, in, out, priv, algCheck, pubInCheck);
        if(ret >= 0) {
            return 0;
        }
    }
    
    XFREE(in, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}
