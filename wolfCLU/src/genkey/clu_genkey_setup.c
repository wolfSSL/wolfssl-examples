/* clu_genkey_setup.c
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
#include <wolfclu/genkey/clu_genkey.h>
#include <wolfclu/x509/clu_cert.h>  /* argument checking */

int wolfCLU_genKeySetup(int argc, char** argv)
{
    char     keyOutFName[MAX_FILENAME_SZ];  /* default outFile for genKey */
    char     defaultFormat[4] = "der\0";
    WC_RNG   rng;

    char*    keyType = NULL;       /* keyType */
    char*    format  = defaultFormat;
    char*    name    = NULL;

    int      formatArg;
    int      ret        =   0;  /* return variable */

    ret = wolfCLU_checkForArg("-h", 2, argc, argv);
    if (ret > 0) {
        wolfCLU_genKeyHelp();
        return 0;
    }

    XMEMSET(keyOutFName, 0, MAX_FILENAME_SZ);

    keyType = argv[2];

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("rng init failed\n");
        return ret;
    }

    ret = wolfCLU_checkForArg("-out", 4, argc, argv);
    if (ret > 0) {
        if (argv[ret+1] != NULL) {
            XSTRNCPY(keyOutFName, argv[ret+1], XSTRLEN(argv[ret+1]));
        } else {
            printf("ERROR: No output file name specified\n");
            wolfCLU_genKeyHelp();
            return USER_INPUT_ERROR;
        }
    } else {
        printf("ERROR: Please specify an output file name\n");
        wolfCLU_genKeyHelp();
        return USER_INPUT_ERROR;
    }

    ret = wolfCLU_checkForArg("-outform", 8, argc, argv);
    if (ret > 0) {
        format = argv[ret+1];
    }
    ret = wolfCLU_checkOutform(format);
    if (ret == PEM_FORM || ret == DER_FORM) {
        printf("OUTPUT A %s FILE\n", (ret == PEM_FORM)? "PEM": "DER");
        formatArg = ret;
    } else {
        printf("ERROR: \"%s\" is not a valid file format\n", format);
        return ret;
    }

    /* type of key to generate */
    if (0) {
        /* force fail w/ check on condition "false" */
    } else if (XSTRNCMP(keyType, "ed25519", 7) == 0) {

    #ifdef HAVE_ED25519

        ret = wolfCLU_checkForArg("-output", 7, argc, argv);
        if (ret > 0) {
            if (argv[ret+1] != NULL) {
                if (XSTRNCMP(argv[ret+1], "pub", 3) == 0)
                    ret = wolfCLU_genKey_ED25519(&rng, keyOutFName, PUB_ONLY,
                                                                     formatArg);
                else if (XSTRNCMP(argv[ret+1], "priv", 4) == 0)
                    ret = wolfCLU_genKey_ED25519(&rng, keyOutFName, PRIV_ONLY,
                                                                     formatArg);
                else if (XSTRNCMP(argv[ret+1], "keypair", 7) == 0)
                    ret = wolfCLU_genKey_ED25519(&rng, keyOutFName,
                                                       PRIV_AND_PUB, formatArg);
            }
        } else {
            printf("No -output <PUB/PRIV/KEYPAIR>\n");
            printf("DEFAULT: output public and private key pair\n");
            ret = wolfCLU_genKey_ED25519(&rng, keyOutFName, PRIV_AND_PUB,
                                                                     formatArg);
        }
    #else
        printf("Invalid option, ED25519 not enabled.\n");
        printf("Please re-configure wolfSSL with --enable-ed25519 and "
               "try again\n");
        return NOT_COMPILED_IN;
    #endif /* HAVE_ED25519 */

    } else if (XSTRNCMP(keyType, "ecc", 3) == 0) {
    #if defined(HAVE_ECC) && defined(WOLFSSL_KEY_GEN)
        /* ECC flags */
        int directiveArg = PRIV_AND_PUB;

        printf("generate ECC key\n");

        /* get the directive argument */
        ret = wolfCLU_checkForArg("-output", 7, argc, argv);
        if (ret > 0) {
            if (argv[ret+1] != NULL) {
                if (XSTRNCMP(argv[ret+1], "pub", 3) == 0)
                    directiveArg = PUB_ONLY_FILE;
                else if (XSTRNCMP(argv[ret+1], "priv", 4) == 0)
                    directiveArg = PRIV_ONLY_FILE;
                else if (XSTRNCMP(argv[ret+1], "keypair", 7) == 0)
                    directiveArg = PRIV_AND_PUB;
            }
        }
        else {
            printf("No -output <PUB/PRIV/KEYPAIR>\n");
            printf("DEFAULT: output public and private key pair\n");
            directiveArg = PRIV_AND_PUB;
        }

        /* get the curve name */
        ret = wolfCLU_checkForArg("-name", 5, argc, argv);
        if (ret > 0) {
            if (argv[ret+1] != NULL) {
                int i;

                name = argv[ret+1];

                /* convert name to upper case */
                for (i = 0; i < XSTRLEN(name); i++)
                    (void)toupper(name[i]);
            }
        }

        if (name == NULL) { /* if we have the name of curve we know */
            printf("DEFAULT: use a 32 ECC key\n");
        }

        ret = wolfCLU_genKey_ECC(&rng, keyOutFName, directiveArg,
                                 formatArg, NULL);
    #else
        printf("Invalid option, ECC not enabled.\n");
        printf("Please re-configure wolfSSL with --enable-ecc and "
               "try again\n");
        return NOT_COMPILED_IN;
    #endif /* HAVE_ECC */
    } else if (XSTRNCMP(keyType, "rsa", 3) == 0) {
    #if !defined(NO_RSA) && defined(WOLFSSL_KEY_GEN)
        /* RSA flags */
        int directiveArg = PRIV_AND_PUB;
        int sizeArg = 0;
        int expArg  = 0;

        printf("generate RSA key\n");

        /* get the directive argument */
        ret = wolfCLU_checkForArg("-output", 7, argc, argv);
        if (ret > 0) {
            if (argv[ret+1] != NULL) {
                if (XSTRNCMP(argv[ret+1], "pub", 3) == 0)
                    directiveArg = PUB_ONLY_FILE;
                else if (XSTRNCMP(argv[ret+1], "priv", 4) == 0)
                    directiveArg = PRIV_ONLY_FILE;
                else if (XSTRNCMP(argv[ret+1], "keypair", 7) == 0)
                    directiveArg = PRIV_AND_PUB;
            }
        }
        else {
            printf("No -output <PUB/PRIV/KEYPAIR>\n");
            printf("DEFAULT: output public and private key pair\n");
            directiveArg = PRIV_AND_PUB;
        }

        /* get the size argument */
        ret = wolfCLU_checkForArg("-size", 5, argc, argv);
        if (ret > 0) {
            if (argv[ret+1] != NULL) {
                char* cur;
                /* make sure it's an integer */
                if (*argv[ret+1] == '\0') {
                    printf("Empty -size argument, using 2048\n");
                    sizeArg = 2048;
                }
                else {
                    for (cur = argv[ret+1]; *cur && isdigit(*cur); ++cur);
                    if (*cur == '\0') {
                        sizeArg = XATOI(argv[ret+1]);
                    }
                    else {
                        printf("Invalid -size (%s), using 2048\n",
                               argv[ret+1]);
                        sizeArg = 2048;
                    }
                }
            }
        }
        else {
            printf("No -size <SIZE>\n");
            printf("DEFAULT: use a 2048 RSA key\n");
            sizeArg = 2048;
        }

        /* get the exponent argument */
        ret = wolfCLU_checkForArg("-exponent", 9, argc, argv);
        if (ret > 0) {
            if (argv[ret+1] != NULL) {
                char* cur;
                /* make sure it's an integer */
                if (*argv[ret+1] == '\0') {
                    printf("Empty -exponent argument, using 65537\n");
                    expArg = 65537;
                }
                else {
                    for (cur = argv[ret+1]; *cur && isdigit(*cur); ++cur);
                    if (*cur == '\0') {
                        sizeArg = XATOI(argv[ret+1]);
                    }
                    else {
                        printf("Invalid -exponent (%s), using 65537\n",
                               argv[ret+1]);
                        expArg = 65537;
                    }
                }
            }
        }
        else {
            printf("No -exponent <SIZE>\n");
            printf("DEFAULT: use an exponent of 65537\n");
            expArg = 65537;
        }

        ret = wolfCLU_genKey_RSA(&rng, keyOutFName, directiveArg,
                                 formatArg, sizeArg, expArg);
    #else
        printf("Invalid option, RSA not enabled.\n");
        printf("Please re-configure wolfSSL with --enable-rsa and "
               "try again\n");
        return NOT_COMPILED_IN;
    #endif /* NO_RSA */
    } else {
        printf("\"%s\" is an invalid key type, or not compiled in\n", keyType);
        return USER_INPUT_ERROR;
    }

    return ret;
}


