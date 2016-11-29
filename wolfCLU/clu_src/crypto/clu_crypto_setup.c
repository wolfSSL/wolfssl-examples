/* clu_crypto_setup.c
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

    if (action == 'e')
        eCheck = 1;
    if (action == 'd')
        dCheck = 1;

    for (i = 2; i < argc; i++) {
        if (XSTRNCMP(argv[i], "-help", 5) == 0 || XSTRNCMP(argv[i], "-h", 2)
                                                                         == 0) {
            if (eCheck == 1) {
                /*wolfCLU_encryptHelp*/
                wolfCLU_encryptHelp();
                return 0;
            } else {
                /*wolfCLU_decryptHelp*/
                wolfCLU_decryptHelp();
                return 0;
            }
        }
    }

    name = argv[2];
    /* gets blocksize, algorithm, mode, and key size from name argument */
    block = wolfCLU_getAlgo(name, &alg, &mode, &size);

    if (block != FATAL_ERROR) {
        pwdKey = (byte*) XMALLOC(size, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (pwdKey == NULL)
            return MEMORY_E;
        iv = (byte*) XMALLOC(block, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (iv == NULL) {
            wolfCLU_freeBins(pwdKey, NULL, NULL, NULL, NULL);
            return MEMORY_E;
        }
        key = (byte*) XMALLOC(size, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (key == NULL) {
            wolfCLU_freeBins(pwdKey, iv, NULL, NULL, NULL);
            return MEMORY_E;
        }

        /* Start at the third flag entered */
        i = 3;
        do {
            if (argv[i] == NULL){
                break;
            }
            else if (XSTRNCMP(argv[i], "-out", 4) == 0 && argv[i+1] != NULL) {
                /* output file */
                out = argv[i+1];
                outCheck = 1;
                i+=2;
                /* it is mandatory that this be set last */
                continue;
            }

            else if (XSTRNCMP(argv[i], "-in", 3) == 0 && argv[i+1] != NULL) {
                 /* input file/text */
                in = argv[i+1];
                inCheck = 1;
                /* continue while out check not equal 1 */
                i+=2;
                continue;
            }

            else if (XSTRNCMP(argv[i], "-pwd", 4) == 0 && argv[i+1] != NULL) {
                /* password pwdKey */
                XMEMCPY(pwdKey, argv[i+1], size);
                pwdKeyChk = 1;
                keyType = 1;
                i+=2;
                continue;
            }
            else if (XSTRNCMP(argv[i], "-verify", 7) == 0) {
                /* using hexidecimal format */
                inputHex = 1;
                i++;
                continue;
            }
            else if (XSTRNCMP(argv[i], "-iv", 3) == 0 && argv[i+1] != NULL) {
                /* iv for encryption */
                if (pwdKeyChk == 1) {
                    printf("Invalid option, attempting to use IV with password"
                           " based key.");
                    wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);
                    return FATAL_ERROR;
                }
                 ivSize = block*2;
                if (XSTRLEN(argv[i+1]) != ivSize) {
                    printf("Invalid IV. Must match algorithm block size.\n");
                    printf("Invalid IV size was: %d.\n",
                                                       (int) strlen(argv[i+1]));
                    printf("size of IV expected was: %d.\n", ivSize);
                    wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);
                    return FATAL_ERROR;
                }
                else {
                    char ivString[XSTRLEN(argv[i+1])];
                    XSTRNCPY(ivString, argv[i+1], XSTRLEN(argv[i+1]));
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
                    i+=2;
                    continue;
                }
            }
            else if (XSTRNCMP(argv[i], "-key", 4) == 0 && argv[i+1] != NULL) {
                /* 2 characters = 1 byte. 1 byte = 8 bits
                 * number of characters / 2 = bytes
                 * bytes * 8 = bits
                 */
                numBits = (int) (XSTRLEN(argv[i+1]) / 2 ) * 8;
                /* Key for encryption */
                if ((int)numBits != size) {
                    printf("Length of key provided was: %d.\n", numBits);
                    printf("Length of key expected was: %d.\n", size);
                    printf("Invalid Key. Must match algorithm key size.\n");
                    wolfCLU_freeBins(pwdKey, iv, key, NULL, NULL);
                    return FATAL_ERROR;
                }
                else {
                    char keyString[strlen(argv[i+1])];
                    XSTRNCPY(keyString, argv[i+1], XSTRLEN(argv[i+1]));
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
                    i+=2;
                    continue;
                }
            }
            else {
                i++; continue;
            }

        }while(i < 15);

        if (pwdKeyChk == 0 && keyCheck == 0) {
            if (dCheck == 1) {
                printf("\nDECRYPT ERROR:\n");
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
    }
    else
        ret = FATAL_ERROR;
    return ret;
}
