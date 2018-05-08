/* clu_genkey_setup.c
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
#include "clu_include/genkey/clu_genkey.h"
#include "clu_include/x509/clu_cert.h"  /* argument checking */

const struct { char *name; int len; int id; } ecc_curves[] = {
    { "default",         7,  ECC_CURVE_DEF       },
    { "secp192r1",       9,  ECC_SECP192R1       },
    { "prime192v2",      10, ECC_PRIME192V2      },
    { "prime192v3",      10, ECC_PRIME192V3      },
    { "prime239v1",      10, ECC_PRIME239V1      },
    { "prime239v2",      10, ECC_PRIME239V2      },
    { "prime239v3",      10, ECC_PRIME239V3      },
    { "secp256r1",       9,  ECC_SECP256R1       },
    { "prime256v1",      10, ECC_SECP256R1       },
    { "secp112r1",       9,  ECC_SECP112R1       },
    { "secp112r2",       9,  ECC_SECP112R2       },
    { "secp128r1",       9,  ECC_SECP128R1       },
    { "secp128r2",       9,  ECC_SECP128R2       },
    { "secp160r1",       9,  ECC_SECP160R1       },
    { "secp160r2",       9,  ECC_SECP160R2       },
    { "secp224r1",       9,  ECC_SECP224R1       },
    { "secp384r1",       9,  ECC_SECP384R1       },
    { "secp521r1",       9,  ECC_SECP521R1       },
    { "secp160k1",       9,  ECC_SECP160K1       },
    { "secp192k1",       9,  ECC_SECP192K1       },
    { "secp224k1",       9,  ECC_SECP224K1       },
    { "secp256k1",       9,  ECC_SECP256K1       },
    { "brainpoolp160r1", 15, ECC_BRAINPOOLP160R1 },
    { "brainpoolp192r1", 15, ECC_BRAINPOOLP192R1 },
    { "brainpoolp224r1", 15, ECC_BRAINPOOLP224R1 },
    { "brainpoolp256r1", 15, ECC_BRAINPOOLP256R1 },
    { "brainpoolp320r1", 15, ECC_BRAINPOOLP320R1 },
    { "brainpoolp384r1", 15, ECC_BRAINPOOLP384R1 },
    { "brainpoolp512r1", 15, ECC_BRAINPOOLP512R1 },
#ifdef HAVE_CURVE25519
    { "x25519",          6,  ECC_X25519          },
#endif
#ifdef HAVE_X448
    { "x448",            4,  ECC_X448            },
#endif
#ifdef WOLFSSL_CUSTOM_CURVES
    { "custom",          6,  ECC_CURVE_CUSTOM    },
#endif
};
int num_ecc_curves = sizeof(ecc_curves) / sizeof(ecc_curves[0]);

int wolfCLU_genKeySetup(int argc, char** argv)
{
    char     keyOutFName[MAX_FILENAME_SZ];  /* default outFile for genKey */
    char     defaultFormat[4] = "der\0";
    WC_RNG   rng;

    char*    keyType = NULL;       /* keyType */
    char*    format  = defaultFormat;

    int      formatArg  =   DER_FORM;
    int      ret        =   0;  /* return variable */
    int      i          =   0;  /* loop counter */

    ret = wolfCLU_checkForArg("-help", 5, argc, argv);
    if (ret > 0) {
        wolfCLU_genKeyHelp();
        return 0;
    }

    ret = wolfCLU_checkForArg("-list_curves", 12, argc, argv);
    if (ret > 0) {
        for (i = 0; i < num_ecc_curves; ++i) {
            printf("  %s\n", ecc_curves[i].name);
        }
        return 0;
    }

    XMEMSET(keyOutFName, 0, MAX_FILENAME_SZ);

    keyType = argv[2];

    ret = wc_InitRng(&rng);

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
        int directiveArg;
        int sizeArg;
        int curveId;

        printf("generate ECC key\n");

        /* get the directive argument */
        ret = wolfCLU_checkForArg("-output", 7, argc, argv);
        if (ret > 0) {
            if (argv[ret+1] != NULL) {
                if (XSTRNCMP(argv[ret+1], "pub", 3) == 0)
                    directiveArg = PUB_ONLY;
                else if (XSTRNCMP(argv[ret+1], "priv", 4) == 0)
                    directiveArg = PRIV_ONLY;
                else if (XSTRNCMP(argv[ret+1], "keypair", 7) == 0)
                    directiveArg = PRIV_AND_PUB;
            }
        } else {
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
                    printf("Empty -size argument, using 32\n");
                    sizeArg = 32;
                }
                else {
                    for (cur = argv[ret+1]; *cur && isdigit(*cur); ++cur);
                    if (*cur == '\0') {
                        sizeArg = atoi(argv[ret+1]);
                    }
                    else {
                        printf("Invalid -size (%s), using 32\n",
                               argv[ret+1]);
                        sizeArg = 32;
                    }
                }
            }
        } else {
            printf("No -size <SIZE>\n");
            printf("DEFAULT: use a 32 ECC key\n");
            sizeArg = 32;
        }

        /* get the directive argument */
        ret = wolfCLU_checkForArg("-curve", 6, argc, argv);
        if (ret > 0) {
            for (i = 0; i < num_ecc_curves; ++i) {
                if (XSTRNCMP(argv[ret+1], ecc_curves[i].name,
                             ecc_curves[i].len) == 0) {
                    printf("DEBUG: got '%s' (id %d)\n", ecc_curves[i].name,
                           ecc_curves[i].id);
                    curveId = ecc_curves[i].id;
                    break;
                }
            }
        } else {
            printf("No -curve <ID>\n");
            printf("DEFAULT: using \"default\" for default curve.\n");
            curveId = ECC_CURVE_DEF;
        }

        ret = wolfCLU_genKey_ECC(&rng, keyOutFName, directiveArg,
                                 formatArg, sizeArg, curveId);
    #else
        printf("Invalid option, ECC not enabled.\n");
        printf("Please re-configure wolfSSL with --enable-ecc and "
               "try again\n");
        return NOT_COMPILED_IN;
    #endif /* HAVE_ECC */
    } else if (XSTRNCMP(keyType, "rsa", 3) == 0) {
    #if defined(NO_RSA) && defined(WOLFSSL_KEY_GEN)
        /* RSA flags */
        int directiveArg;
        int sizeArg;
        int expArg;

        printf("generate RSA key\n");

        /* get the directive argument */
        ret = wolfCLU_checkForArg("-output", 7, argc, argv);
        if (ret > 0) {
            if (argv[ret+1] != NULL) {
                if (XSTRNCMP(argv[ret+1], "pub", 3) == 0)
                    directiveArg = PUB_ONLY;
                else if (XSTRNCMP(argv[ret+1], "priv", 4) == 0)
                    directiveArg = PRIV_ONLY;
                else if (XSTRNCMP(argv[ret+1], "keypair", 7) == 0)
                    directiveArg = PRIV_AND_PUB;
            }
        } else {
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
                        sizeArg = atoi(argv[ret+1]);
                    }
                    else {
                        printf("Invalid -size (%s), using 2048\n",
                               argv[ret+1]);
                        sizeArg = 2048;
                    }
                }
            }
        } else {
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
                        sizeArg = atoi(argv[ret+1]);
                    }
                    else {
                        printf("Invalid -exponent (%s), using 65537\n",
                               argv[ret+1]);
                        expArg = 65537;
                    }
                }
            }
        } else {
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


