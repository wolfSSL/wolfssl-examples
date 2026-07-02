/* 3des-file-encrypt.c
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

#include <wolfssl/options.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/memory.h>
#include <wolfssl/wolfcrypt/des3.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/pwdbased.h>

#ifndef XPRINTF
    #define XPRINTF printf
#endif

#define DES3_BLOCK_SIZE 24               /* size of encryption blocks */
#define SALT_SIZE 8

/*
 * Makes a cryptographically secure key by stretching a user entered key
 */
int GenerateKey(WC_RNG* rng, byte* key, int size, byte* salt, int pad)
{
    int ret;

    ret = wc_RNG_GenerateBlock(rng, salt, SALT_SIZE);
    if (ret != 0)
        return -1020;

    if (pad == 0)        /* sets first value of salt to check if the */
        salt[0] = 0;            /* message is padded */

    /* stretches key */
    ret = wc_PBKDF2(key, key, strlen((const char*)key), salt, SALT_SIZE, 4096,
        size, WC_SHA256);
    if (ret != 0)
        return -1030;

    return 0;
}

/*
 * Encrypts a file using 3DES
 */
int Des3Encrypt(Des3* des3, byte* key, int size, FILE* inFile, FILE* outFile)
{
    WC_RNG  rng;
    byte    iv[DES3_BLOCK_SIZE];
    byte*   input = NULL;
    byte*   output = NULL;
    byte    salt[SALT_SIZE] = {0};

    int     i = 0;
    int     ret = 0;
    int     inputLength;
    int     length;
    int     padCounter = 0;
    int     rngInit = 0;
    int     des3Init = 0;

    fseek(inFile, 0, SEEK_END);
    inputLength = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    length = inputLength;
    /* pads the length until it evenly matches a block / increases pad number*/
    while (length % DES3_BLOCK_SIZE != 0) {
        length++;
        padCounter++;
    }

    input = (byte*)XMALLOC(length, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    output = (byte*)XMALLOC(length, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (input == NULL || output == NULL) {
        XPRINTF("Failed to allocate memory\n");
        ret = -1050;
    }

    if (ret == 0) {
        ret = wc_InitRng(&rng);
        if (ret != 0)
            XPRINTF("Failed to initialize random number generator\n");
        else
            rngInit = 1;
    }

    /* reads from inFile and writes whatever is there to the input array */
    if (ret == 0) {
        ret = fread(input, 1, inputLength, inFile);
        if (ret == 0) {
            XPRINTF("Input file does not exist.\n");
            ret = -1010;
        }
        else {
            ret = 0;
            for (i = inputLength; i < length; i++) {
                /* pads the added characters with the number of pads */
                input[i] = padCounter;
            }
        }
    }

    if (ret == 0) {
        ret = wc_RNG_GenerateBlock(&rng, iv, DES3_BLOCK_SIZE);
        if (ret != 0)
            ret = -1020;
    }

    /* stretches key to fit size */
    if (ret == 0) {
        ret = GenerateKey(&rng, key, size, salt, padCounter);
        if (ret != 0)
            ret = -1040;
    }

    /* inits des3 structure */
    if (ret == 0) {
        ret = wc_Des3Init(des3, NULL, INVALID_DEVID);
        if (ret != 0) {
            XPRINTF("Des3Init returned: %d\n", ret);
            ret = -1000;
        }
        else
            des3Init = 1;
    }

    /* sets key */
    if (ret == 0) {
        ret = wc_Des3_SetKey(des3, key, iv, DES_ENCRYPTION);
        if (ret != 0)
            ret = -1001;
    }

    /* encrypts the message to the output based on input length + padding */
    if (ret == 0) {
        ret = wc_Des3_CbcEncrypt(des3, output, input, length);
        if (ret != 0)
            ret = -1005;
    }

    if (ret == 0) {
        /* writes to outFile */
        fwrite(salt, 1, SALT_SIZE, outFile);
        fwrite(iv, 1, DES3_BLOCK_SIZE, outFile);
        fwrite(output, 1, length, outFile);
    }

    /* closes the opened files and frees the memory*/
    if (input != NULL) {
        wc_ForceZero(input, length);
        XFREE(input, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
    if (output != NULL) {
        wc_ForceZero(output, length);
        XFREE(output, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
    wc_ForceZero(key, size);
    XFREE(key, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    fclose(inFile);
    fclose(outFile);
    if (des3Init)
        wc_Des3Free(des3);
    if (rngInit)
        wc_FreeRng(&rng);

    return ret;
}

/*
 * Decrypts a file using 3DES
 */
int Des3Decrypt(Des3* des3, byte* key, int size, FILE* inFile, FILE* outFile)
{
    WC_RNG  rng;
    byte    iv[DES3_BLOCK_SIZE];
    byte*   input = NULL;
    byte*   output = NULL;
    byte    salt[SALT_SIZE] = {0};

    int     i = 0;
    int     ret = 0;
    int     length;
    int     aSize;
    int     rngInit = 0;
    int     des3Init = 0;

    fseek(inFile, 0, SEEK_END);
    length = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);
    aSize = length;

    /* verifies the file is at least large enough to hold the salt and iv
     * before any subtraction/allocation is performed on its length, to
     * avoid an integer underflow / negative-size allocation on a
     * truncated file */
    if (length < SALT_SIZE + DES3_BLOCK_SIZE) {
        XPRINTF("Input file is too small.\n");
        ret = -1011;
    }

    if (ret == 0) {
        input = (byte*)XMALLOC(aSize, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        output = (byte*)XMALLOC(aSize, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (input == NULL || output == NULL) {
            XPRINTF("Failed to allocate memory\n");
            ret = -1051;
        }
    }

    if (ret == 0) {
        ret = wc_InitRng(&rng);
        if (ret != 0)
            XPRINTF("Failed to initialize random number generator\n");
        else
            rngInit = 1;
    }

    /* reads from inFile and writes whatever is there to the input array */
    if (ret == 0) {
        ret = fread(input, 1, length, inFile);
        if (ret == 0) {
            XPRINTF("Input file does not exist.\n");
            ret = -1010;
        }
        else {
            ret = 0;
            for (i = 0; i < SALT_SIZE; i++) {
                /* finds salt from input message */
                salt[i] = input[i];
            }
            for (i = SALT_SIZE; i < DES3_BLOCK_SIZE + SALT_SIZE; i++) {
                /* finds iv from input message */
                iv[i - SALT_SIZE] = input[i];
            }
        }
    }

    /* replicates old key if keys match */
    if (ret == 0) {
        ret = wc_PBKDF2(key, key, strlen((const char*)key), salt, SALT_SIZE,
            4096, size, WC_SHA256);
        if (ret != 0)
            ret = -1050;
    }

    /* inits des3 structure */
    if (ret == 0) {
        ret = wc_Des3Init(des3, NULL, INVALID_DEVID);
        if (ret != 0) {
            XPRINTF("Des3Init returned: %d\n", ret);
            ret = -1000;
        }
        else
            des3Init = 1;
    }

    /* sets key */
    if (ret == 0) {
        ret = wc_Des3_SetKey(des3, key, iv, DES_DECRYPTION);
        if (ret != 0)
            ret = -1002;
    }

    if (ret == 0) {
        /* change length to remove salt/iv block from being decrypted */
        length -= (DES3_BLOCK_SIZE + SALT_SIZE);
        for (i = 0; i < length; i++) {
            /* shifts message: ignores salt/iv on message*/
            input[i] = input[i + (DES3_BLOCK_SIZE + SALT_SIZE)];
        }
        /* decrypts the message to output based on input length + padding */
        ret = wc_Des3_CbcDecrypt(des3, output, input, length);
        if (ret != 0)
            ret = -1006;
    }

    if (ret == 0) {
        if (salt[0] != 0) {
            /* validates the padding byte before using it to reduce length,
             * to avoid an out-of-bounds/garbage-length fwrite on
             * corrupted or malicious ciphertext */
            if (length < 1 || output[length-1] < 1 ||
                    output[length-1] > length ||
                    output[length-1] > DES3_BLOCK_SIZE) {
                XPRINTF("Error: invalid padding value\n");
                ret = -1013;
            }
            else {
                /* reduces length based on number of padded elements  */
                length -= output[length-1];
            }
        }
        if (ret == 0) {
            /* writes output to the outFile based on shortened length */
            fwrite(output, 1, length, outFile);
        }
    }

    /* closes the opened files and frees the memory*/
    if (input != NULL) {
        wc_ForceZero(input, aSize);
        XFREE(input, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
    if (output != NULL) {
        wc_ForceZero(output, aSize);
        XFREE(output, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
    wc_ForceZero(key, size);
    XFREE(key, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    fclose(inFile);
    fclose(outFile);
    if (des3Init)
        wc_Des3Free(des3);
    if (rngInit)
        wc_FreeRng(&rng);

    return ret;
}

/*
 * help message
 */
void help()
{
    XPRINTF("\n~~~~~~~~~~~~~~~~~~~~|Help|~~~~~~~~~~~~~~~~~~~~~\n\n");
    XPRINTF("Usage: ./3des-encrypt <-option> <KeySize> <file.in> "
        "<file.out>\n\n");
    XPRINTF("Options\n");
    XPRINTF("-d    Decryption\n-e    Encryption\n-h    Help\n");
}

/*
 * temporarily disables echoing in terminal for secure key input
 */
int NoEcho(char* key, int size)
{
    struct termios oflags, nflags;
    int isTTY;
    size_t keyLen;

    isTTY = isatty(fileno(stdin));

    if (isTTY) {
        /* disabling echo */
        tcgetattr(fileno(stdin), &oflags);
        nflags = oflags;
        nflags.c_lflag &= ~ECHO;
        nflags.c_lflag |= ECHONL;

        if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
            XPRINTF("Error: tcsetattr failed to disable terminal echo\n");
            return -1060;
        }
    }

    XPRINTF("Unique Password: ");
    if (fgets(key, size, stdin) == NULL) {
        XPRINTF("Error: fgets failed to retrieve secure key input\n");
        return -1070;
    }
    keyLen = strlen(key);
    if (keyLen > 0 && key[keyLen - 1] == '\n') {
        key[keyLen - 1] = 0;
    }

    if (isTTY) {
        /* restore terminal */
        if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
            XPRINTF("Error: tcsetattr failed to enable terminal echo\n");
            return -1080;
        }
    }
    return 0;
}

int SizeCheck(int size)
{
    int ret = 0;

    if (size != 56 && size != 112 && size != 168) {
        /* if the entered size does not match acceptable size */
        XPRINTF("Invalid 3DES key size\n");
        ret = -1080;
    }

    return ret;
}

int main(int argc, char** argv)
{
    Des3   des3;
    byte*  key;       /* user entered key */
    FILE*  inFile = NULL;
    FILE*  outFile = NULL;

    const char* in;
    const char* out;

    int    option;    /* choice of how to run program */
    int    ret = 0;   /* return value */
    int    inCheck = 0;
    int    outCheck = 0;
    int    size = 0;
    char   choice = 'n';

    while ((option = getopt(argc, argv, "d:e:i:o:h")) != -1) {
        switch (option) {
            case 'd': /* if entered decrypt */
                size = atoi(optarg);
                ret = SizeCheck(size);
                choice = 'd';
                break;
            case 'e': /* if entered encrypt */
                size = atoi(optarg);
                ret = SizeCheck(size);
                choice = 'e';
                break;
            case 'h': /* if entered 'help' */
                help();
                break;
            case 'i': /* input file */
                in = optarg;
                inCheck = 1;
                inFile = fopen(in, "r");
                break;
            case 'o': /* output file */
                out = optarg;
                outCheck = 1;
                outFile = fopen(out, "w");
                break;
            case '?':
                if (optopt) {
                    XPRINTF("Ending Session\n");
                    return -111;
                }
            default:
                abort();
        }
    }

    if (inCheck == 0 || outCheck == 0) {
            XPRINTF("Must have both input and output file");
            XPRINTF(": -i filename -o filename\n");
    }

    else if (ret == 0 && choice != 'n' && inFile != NULL) {
        key = (byte*)XMALLOC(size, NULL, DYNAMIC_TYPE_TMP_BUFFER);    /* sets size memory of key */
        if (key == NULL) {
            XPRINTF("Failed to allocate memory for key\n");
            ret = -1050;
        }
        else {
            ret = NoEcho((char*)key, size);
            if (ret == 0) {
                if (choice == 'e')
                    ret = Des3Encrypt(&des3, key, size, inFile, outFile);
                else if (choice == 'd')
                    ret = Des3Decrypt(&des3, key, size, inFile, outFile);
            }
            else {
                wc_ForceZero(key, size);
                XFREE(key, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            }
        }
    }
    else if (choice == 'n') {
        XPRINTF("Must select either -e[56,112,168] or -d[56,112,168] for \
                encryption and decryption\n");
    }

    return ret;
}
