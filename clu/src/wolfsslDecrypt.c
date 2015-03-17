/* wolfsslDecrypt.c
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

#define SALT_SIZE       8
#define MAX             1024

int wolfsslDecrypt(char* alg, char* mode, byte* pwdKey, byte* key, int size, 
        char* in, char* out, byte* iv, int block, int keyType)
{
#ifndef NO_AES
    Aes aes;                            /* aes declaration */
#endif

#ifndef NO_DES3
    Des3 des3;                          /* 3des declaration */
#endif

#ifdef HAVE_CAMELLIA
    Camellia camellia;                  /* camellia declaration */
#endif

    FILE*  inFile;                      /* input file */
    FILE*  outFile;                     /* output file */

    RNG     rng;                        /* random number generator */
    byte*   input;                      /* input buffer */
    byte*   output;                     /* output buffer */
    byte    salt[SALT_SIZE] = {0};      /* salt variable */

    int     currLoopFlag = 1;           /* flag to track the loop */
    int     lastLoopFlag = 0;           /* flag for last loop */
    int     ret          = 0;           /* return variable */
    int     length;                     /* length of message */
    int     tempMax = MAX;              /* equal to MAX until feof */
    int     keyVerify   = 0;            /* verify the key is set */
    int     i           = 0;            /* loop variable */
    int     sbSize = SALT_SIZE + block; /* size of salt and iv together */

    /* opens input file */
    inFile = fopen(in, "rb");
    if (inFile == NULL) {
        printf("Input file does not exist.\n");
        return DECRYPT_ERROR;
    }
    /* opens output file */

    if ((outFile = fopen(out, "wb")) == NULL) {
        printf("Error creating output file.\n");
        return DECRYPT_ERROR; 
    }

    /* find end of file for length */
    fseek(inFile, 0, SEEK_END);
    length = (int) ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    /* if there is a remainder, 
     * round up else no round 
     */
    if (length % MAX > 0) {
        lastLoopFlag = (length/MAX) + 1;
    }
    else {
        lastLoopFlag =  length/MAX;
    }

    input = (byte*) malloc(MAX);
    output = (byte*) malloc(MAX);

    InitRng(&rng);

    /* reads from inFile and writes whatever
     * is there to the input buffer 
     */
    while ( length > 0 ) {

        /* On first loop only read in salt and iv */
        if (currLoopFlag == 1) {
            if ( (int) fread (salt, 1, SALT_SIZE, inFile) != SALT_SIZE) {
                printf("Error reading salt.\n");
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return FREAD_ERROR;
            }

            if ( (int) fread (iv, 1, block, inFile) != block) {
                printf("Error reading salt.\n");
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return FREAD_ERROR;
            } 
            /* replicates old pwdKey if pwdKeys match */
            if (keyType == 1) {
                if (PBKDF2(key, pwdKey, (int) strlen((const char*)pwdKey), salt, 
                            SALT_SIZE, 4096, size, SHA256) != 0) {
                    printf("pwdKey set error.\n");
                    wolfsslFreeBins(input, output, NULL, NULL, NULL);
                    return ENCRYPT_ERROR;
                }
            }
            else if (keyType == 2) {
                for (i = 0; i < size; i++) {

                    /* ensure key is set */
                    if (key[i] == 0 || key[i] == '\0') {
                        continue;
                    }
                    else {
                        keyVerify++;
                    }
                }
                if (keyVerify == 0) {
                    printf("the key is all zero's or not set.\n");
                    wolfsslFreeBins(input, output, NULL, NULL, NULL);
                    return ENCRYPT_ERROR;
                } 
            }
        }

        /* Read in 1kB */
        if ((ret = (int) fread(input, 1, MAX, inFile)) != MAX) {
            if (feof(inFile)) {
                tempMax = ret;
            }
            else {
                printf("Input file does not exist.\n");
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return FREAD_ERROR;
            }
        }

        /* sets pwdKey decrypts the message to ouput from input length */
#ifndef NO_AES
        if (strcmp(alg, "aes") == 0) {
            if (strcmp(mode, "cbc") == 0) {
                ret = AesSetKey(&aes, key, AES_BLOCK_SIZE, iv, AES_DECRYPTION);
                if (ret != 0) {
                    fclose(inFile);
                    fclose(outFile);
                    wolfsslFreeBins(input, output, NULL, NULL, NULL);
                    return ret;
                }
                ret = AesCbcDecrypt(&aes, output, input, tempMax);
                if (ret != 0) {
                    fclose(inFile);
                    fclose(outFile);
                    wolfsslFreeBins(input, output, NULL, NULL, NULL);
                    return DECRYPT_ERROR;
                }
            }  
#ifdef CYASSL_AES_COUNTER
            else if (strcmp(mode, "ctr") == 0) {
                /* if mode is ctr */
                AesSetKeyDirect(&aes, key, AES_BLOCK_SIZE, iv, AES_ENCRYPTION);
                AesCtrEncrypt(&aes, output, input, tempMax);
            }
#endif
        }
#endif
#ifndef NO_DES3
        if (strcmp(alg, "3des") == 0) {
            ret = Des3_SetKey(&des3, key, iv, DES_DECRYPTION);
            if (ret != 0) {
                fclose(inFile);
                fclose(outFile);
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return ret;
            }
            ret = Des3_CbcDecrypt(&des3, output, input, tempMax);
            if (ret != 0){
                fclose(inFile);
                fclose(outFile);
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return DECRYPT_ERROR;
            }
        }
#endif
#ifdef HAVE_CAMELLIA
        if (strcmp(alg, "camellia") == 0) {
            ret = CamelliaSetKey(&camellia, key, block, iv);
            if (ret != 0) {
                fclose(inFile);
                fclose(outFile);
                wolfsslFreeBins(input, output, NULL, NULL, NULL);
                return ret;
            }
            CamelliaCbcDecrypt(&camellia, output, input, tempMax);
        }
#endif
        if (currLoopFlag == lastLoopFlag) {
            if (salt[0] != 0) {
                /* reduces length based on number of padded elements  */
                int pad = output[tempMax-1];
                /* adjust length for padded bytes and salt size */
                length -= pad + sbSize;
                /* reset tempMax for smaller decryption */
                fwrite(output, 1, length, outFile);
                break;
            }
            else {
                fwrite(output, 1, tempMax, outFile);

                memset(input, 0, tempMax);
                memset(output, 0, tempMax);
                break;
            }
        } 
        /* writes output to the outFile */
        fwrite(output, 1, tempMax, outFile);

        memset(input, 0, tempMax);
        memset(output, 0, tempMax);

        currLoopFlag++;
        length -= tempMax;
    }
    /* closes the opened files and frees memory */
    memset(input, 0, MAX);
    memset (output, 0, MAX);
    wolfsslFreeBins(input, output, NULL, NULL, NULL);
    memset(key, 0, size);
    /* Use the cyassl FreeRng to free rng */
    FreeRng(&rng);
    fclose(inFile);
    fclose(outFile);

    return 0;
}
