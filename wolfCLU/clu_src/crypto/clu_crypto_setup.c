/* clu_crypto_setup.c
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

#include "clu_include/clu_header_main.h"
#include "clu_include/clu_optargs.h"

int wolfCLU_setup(int argc, char** argv, char action)
{
    char     outNameE[256];     /* default outFile for encrypt */
    char     outNameD[256];     /* default outfile for decrypt */
    char     inName[256];       /* name of the in File if not provided */

    char*    name = NULL;       /* string of algorithm, mode, keysize */
    char*    alg = NULL;        /* algorithm from name */
    char*    mode = NULL;       /* mode from name */
    char*    out = outNameE;    /* default output file name */
    char*    in = inName;       /* default in data */
    byte*    pwdKey = NULL;     /* password for generating pwdKey */
    byte*    key = NULL;        /* user set key NOT PWDBASED */
    byte*    iv = NULL;         /* iv for initial encryption */


    int      size       =   0;  /* keysize from name */
    int      ret        =   0;  /* return variable */
    int      block      =   0;  /* block size based on algorithm */
    int      pwdKeyChk  =   0;  /* if a pwdKey has been provided */
    int      ivCheck    =   0;  /* if the user sets the IV explicitly */
    int      keyCheck   =   0;  /* if ivCheck is 1 this should be set also */
    int      inCheck    =   0;  /* if input has been provided */
    int      outCheck   =   0;  /* if output has been provided */
    int      i          =   0;  /* loop counter */
    int      eCheck     =   0;  /* if user is encrypting data */
    int      dCheck     =   0;  /* if user is decrypting data */
    int      inputHex   =   0;  /* if user is encrypting hexidecimal stuff */
    int      keyType    =   0;  /* tells Decrypt which key it will be using
                                 * 1 = password based key, 2 = user set key
                                 */
    word32   ivSize     =   0;  /* IV if provided should be 2*block */
    word32   numBits    =   0;  /* number of bits in argument from the user */
    int      option;
    int      long_index = 0;

    if (action == 'e')
        eCheck = 1;
    if (action == 'd')
        dCheck = 1;

    ret = wolfCLU_checkForArg("-h", 2, argc, argv);
    if (ret > 0) {
        if (eCheck == 1) {
            wolfCLU_encryptHelp();
            return 0;
        } else {
            wolfCLU_decryptHelp();
            return 0;
        }
    }

    /* gets blocksize, algorithm, mode, and key size from name argument */
    block = wolfCLU_getAlgo(argc, argv, &alg, &mode, &size);
    if (block < 0) {
        printf("unable to find algorithm to use\n");
        return -1;
    }

    /* initialize memory buffers */
    pwdKey = (byte*) XMALLOC(size, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (pwdKey == NULL)
        return MEMORY_E;
    XMEMSET(pwdKey, 0, size);

    iv = (byte*) XMALLOC(block, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (iv == NULL) {
        wolfCLU_freeBins(pwdKey, NULL, NULL, NULL, NULL);
        return MEMORY_E;
    }
    XMEMSET(iv, 0, block);

    key = (byte*) XMALLOC(size, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (key == NULL) {
        wolfCLU_freeBins(pwdKey, iv, NULL, NULL, NULL);
        return MEMORY_E;
    }
    XMEMSET(key, 0, size);

    opterr = 0; /* do not display unrecognized options */
    optind = 0; /* start at indent 0 */
    while ((option = getopt_long_only(argc, argv, "",
                   crypt_options, &long_index )) != -1) {

        switch (option) {
        case PASSWORD: /* Password                                  */
            XSTRNCPY((char*)pwdKey, optarg, size);
            pwdKeyChk = 1;
            keyType   = 1;
            break;

        case PBKDF2:   /* pbkdf2 is used by default                 */
            break;

        case KEY:      /* Key if used must be in hex                */
            break;

        case IV:       /* IV if used must be in hex                 */
            {
                char ivString[XSTRLEN(optarg)];
                XSTRNCPY(ivString, optarg, XSTRLEN(optarg));
                ret = wolfCLU_hexToBin(ivString, &iv, &ivSize,
                                       NULL, NULL, NULL,
                                       NULL, NULL, NULL,
                                       NULL, NULL, NULL);
                if (ret != 0) {
                    printf("failed during conversion of IV, ret = %d\n",
                           ret);
                    wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);
                    return -1;
                }
                ivCheck = 1;
            }
            break;

        case SIGN:
            break;

        case WOLFCLU_VERIFY:   /* Verify results, used with -iv and -key    */
            /* using hexidecimal format */
            inputHex = 1;
            break;

        case INFORM:   /* Certificate Stuff                         */
        case OUTFORM:
        case OUTPUT:
        case NOOUT:
        case TEXT_OUT:
        case SILENT:
        case PUBIN:
        case PUBOUT:
        case PUBKEY:


            /* The cases above have their arguments converted to lower case */
            if (optarg) convert_to_lower(optarg, (int)XSTRLEN(optarg));
            /* The cases below won't have their argument's molested */

        case INFILE:   /* File passed in by user                    */
            in = optarg;
            inCheck = 1;
            break;

        case OUTFILE:  /* Output file                               */
            out = optarg;
            outCheck = 1;
            break;

        case INKEY:
            if (optarg == NULL) {
                printf("no key passed in..\n");
                wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);
                return FATAL_ERROR;
            }

            /* 2 characters = 1 byte. 1 byte = 8 bits
             * number of characters / 2 = bytes
             * bytes * 8 = bits
             */
            numBits = (int) (XSTRLEN(optarg) / 2 ) * 8;
            /* Key for encryption */
            if ((int)numBits != size) {
                printf("Length of key provided was: %d.\n", numBits);
                printf("Length of key expected was: %d.\n", size);
                printf("Invalid Key. Must match algorithm key size.\n");
                wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);
                return FATAL_ERROR;
            }
            else {
                char keyString[strlen(optarg)];
                XSTRNCPY(keyString, optarg, XSTRLEN(optarg));
                ret = wolfCLU_hexToBin(keyString, &key, &numBits,
                                       NULL, NULL, NULL,
                                       NULL, NULL, NULL,
                                       NULL, NULL, NULL);
                if (ret != 0) {
                    printf("failed during conversion of Key, ret = %d\n",
                           ret);
                    wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);
                    return -1;
                }
                keyCheck = 1;
                keyType = 2;
            }
            break;

        case SIGFILE:
            break;

        case WOLFCLU_DECRYPT:
            eCheck = 0;
            dCheck = 1;
            break;

        case ':':
        case '?':
            break;

        default:
            /* do nothing. */
            (void)ret;
        }
    }

    if (pwdKeyChk == 0 && keyCheck == 0) {
        if (dCheck == 1) {
            printf("\nDECRYPT ERROR:\n");
            printf("no key set or passphrase\n");
            printf("Please type \"wolfssl -decrypt -help\" for decryption"
                                                            " usage \n\n");
            return 0;
        }
        /* if no pwdKey is provided */
        else {
            printf("No -pwd flag set, please enter a password to use for"
            " encrypting.\n");
            printf("Write your password down so you don't forget it.\n");
            ret = wolfCLU_noEcho((char*)pwdKey, size);
            pwdKeyChk = 1;
        }
    }

    if (inCheck == 0 && eCheck == 1) {
        ret = 0;
        while (ret == 0) {
            printf("-in flag was not set, please enter a string or\n"
                   "file name to be encrypted: ");
            ret = (int) scanf("%s", inName);
        }
        in = inName;
        /* if no input is provided */
        printf("Ok,  We will encrypt:\"%s\" for you.\n", inName);
        inCheck = 1;
    }

    if (eCheck == 1 && dCheck == 1) {
        printf("You want to encrypt and decrypt simultaneously? That is"
                "not possible...\n");
        wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);
        return FATAL_ERROR;
    }

    if (inCheck == 0 && dCheck == 1) {
        printf("We are so sorry but you must specify what it is you are "
                "trying to decrypt.\n");
        wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);
        return FATAL_ERROR;
    }

    if (ivCheck == 1) {
        if (keyCheck == 0) {
            printf("-iv was explicitly set, but no -key was set. User\n"
                " needs to provide a non-password based key when setting"
                    " the -iv flag.\n");
            wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);
            return FATAL_ERROR;
        }
    }

    if (pwdKeyChk == 1 && keyCheck == 1) {
        XMEMSET(pwdKey, 0, size);
    }

    /* encryption function call */
    if (eCheck == 1) {

        printf("\n");
        if (outCheck == 0) {
            ret = 0;
            while (ret == 0) {
                printf("Please enter a name for the output file: ");
                ret = (int) scanf("%s", outNameE);
                out = (ret > 0) ? outNameE : '\0';
            }
        }
        ret = wolfCLU_encrypt(alg, mode, pwdKey, key, size, in, out,
                iv, block, ivCheck, inputHex);
    }
    /* decryption function call */
    else if (dCheck == 1) {
        if (outCheck == 0) {
            ret = 0;
            while (ret == 0) {
                printf("Please enter a name for the output file: ");
                ret = (int) scanf("%s", outNameD);
                out = (ret > 0) ? outNameD : '\0';
            }
        }
        ret = wolfCLU_decrypt(alg, mode, pwdKey, key, size, in, out,
                iv, block, keyType);
    }
    else {
        wolfCLU_help();
    }
    /* clear and free data */
    XMEMSET(key, 0, size);
    XMEMSET(pwdKey, 0, size);
    XMEMSET(iv, 0, block);
    wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);

    if (alg != NULL)
        XFREE(alg, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (mode != NULL)
        XFREE(mode, NULL, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}
