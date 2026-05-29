/* ascon-file-encrypt.c
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

#ifdef NO_INLINE
    #include <wolfssl/wolfcrypt/misc.h>
#endif
#include <wolfssl/options.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <wolfssl/wolfcrypt/ascon.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/pwdbased.h>

#define ASCON_AEAD128_RATE 16
#define SALT_SIZE           8
#define AD_SIZE             32
#define FILE_HEADER_SIZE   40
#define BLOCK_SIZE         4096

static const byte ad[AD_SIZE] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x01A, 0x1B,
    0x1C, 0x1D, 0x1E, 0x1F,
};

void MemFree(wc_AsconAEAD128 *ascon, byte *key, int size, FILE *inFile, FILE *outFile, WC_RNG* rng, byte *input, byte *output, int length, int rngInit, int encrypt) {
    if (input != NULL) {
        memset(input, 0, encrypt ? BLOCK_SIZE : length);
        free(input);
    }
    if (output != NULL) {
        memset(output, 0, length);
        free(output);
    }
    if (outFile != NULL) {
        fclose(outFile);
    }
    if (inFile != NULL) {
        fclose(inFile);
    }
    if (key != NULL) {
        memset(key, 0, size);
        free(key);
    }
    /* Those two functions check validity of pointer before freeing it*/
    if (rngInit && rng != NULL) {
        wc_FreeRng(rng);
    }

    if (ascon != NULL) {
        wc_AsconAEAD128_Free(ascon);
    }
}

/*
 * Makes a cryptographically secure key by stretching a user entered key
 */
int GenerateKey(WC_RNG* rng, byte* key, int size, byte* salt)
{

    int ret = wc_RNG_GenerateBlock(rng, salt, SALT_SIZE);
    if (ret != 0)
        return -1020;

    /* stretches key */
    ret = wc_PBKDF2(key, key, strlen((const char*)key), salt, SALT_SIZE, 4096,
        size, WC_SHA256);
    if (ret != 0)
        return -1030;

    return 0;
}

/*
 * Encrypts a file using Ascon
 */
int AsconEncrypt(wc_AsconAEAD128* ascon, byte* key, int size, FILE* inFile, FILE* outFile)
{
    WC_RNG   rng;
    int      rngInit = 0;
    int      encrypt = 1;
    byte     nonce[ASCON_AEAD128_NONCE_SZ] = {0};
    byte     salt[SALT_SIZE] = {0};
    byte     tag[ASCON_AEAD128_TAG_SZ] = {0};
    int     ret = 1;
    int     chunk_read = BLOCK_SIZE;


    fseek(inFile, 0, SEEK_END);
    const int inFileLength = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    byte* input = malloc(BLOCK_SIZE);
    byte* output = malloc(inFileLength);

    if (input == NULL || output == NULL) {
        goto cleanup;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Failed to initialize random number generator\n");
        goto cleanup;
    }
    rngInit = 1;

    /* stretches key to fit size */
    ret = GenerateKey(&rng, key, size, salt);
    if (ret != 0) {
        printf("Generate key failed.\n");
        goto cleanup;
    }

    /* sets key */
    ret = wc_AsconAEAD128_SetKey(ascon, key);
    if (ret != 0) {
        printf("Set key failed.\n");
        goto cleanup;
    }

    /* Generate random nonce for each encryption */
    ret = wc_RNG_GenerateBlock(&rng, nonce, ASCON_AEAD128_NONCE_SZ);
    if (ret != 0) {
        printf("Generate nonce failed.\n");
        goto cleanup;
    }

    /* sets nonce */
    ret = wc_AsconAEAD128_SetNonce(ascon, nonce);
    if (ret != 0) {
        printf("Set nonce failed.\n");
        goto cleanup;
    }


    /* sets additional data */
    ret = wc_AsconAEAD128_SetAD(ascon, ad, AD_SIZE);
    if (ret != 0) {
        printf("Set AD failed.\n");
        goto cleanup;
    }

    /* Loop reading a block at a time */
    for (int j = 0; j < inFileLength; j += BLOCK_SIZE) {
        if (chunk_read > inFileLength - j) {
            chunk_read = inFileLength - j;
        }

        ret = fread(input, 1, chunk_read, inFile);
        if (ret != chunk_read) {
            printf("ERROR: Failed to read the appropriate amount\n");
            ret = -1;
            goto cleanup;
        }

        /* encrypts the message to the output based on input length */
        ret = wc_AsconAEAD128_EncryptUpdate(ascon, output+j, input, chunk_read);
        if (ret != 0) {
            printf("Encrypt update failed.\n");
            goto cleanup;
        }

    }

    /* Finalize encryption and generate tag */
    ret = wc_AsconAEAD128_EncryptFinal(ascon, tag);
    if (ret != 0) {
        printf("Encrypt final failed.\n");
        goto cleanup;
    }

    /* writes to outFile */
    fwrite(salt, 1, SALT_SIZE, outFile);
    fwrite(nonce, 1, ASCON_AEAD128_NONCE_SZ, outFile);
    fwrite(tag, 1, ASCON_AEAD128_TAG_SZ, outFile);
    fwrite(output, 1, inFileLength, outFile);


    /* closes the opened files and frees the memory*/
    cleanup:
        MemFree(ascon, key, size, inFile, outFile, &rng, input, output, inFileLength, rngInit, encrypt);
    return ret;
}

/*
 * Decrypts a file using Ascon
 */
int AsconDecrypt(wc_AsconAEAD128* ascon, byte* key, int size, FILE* inFile, FILE* outFile)
{
    WC_RNG   rng;
    int      rngInit = 0;
    int      encrypt = 0;
    byte     nonce[ASCON_AEAD128_NONCE_SZ] = {0};
    byte     salt[SALT_SIZE] = {0};
    byte     tag[ASCON_AEAD128_TAG_SZ] = {0};
    int     ret = 1;
    int length = 0;

    fseek(inFile, 0, SEEK_END);
    length = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);
    const int aSize = length;

    byte* input = malloc(aSize);
    byte* output = malloc(aSize);

    if (input == NULL || output == NULL) {
        goto cleanup;
    }


    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Failed to initialize random number generator\n");
        goto cleanup;
    }
    rngInit = 1;

    /* reads from inFile and writes whatever is there to the input array */
    ret = fread(input, 1, length, inFile);
    if (ret != length) {
        printf("Error while reading input file.\n");
        goto cleanup;
    }

    int i = 0;

    if (length > FILE_HEADER_SIZE) {

        /* finds salt from input message */
        for (; i < SALT_SIZE; i++) {
            salt[i] = input[i];
        }

        /* finds nonce from input message */
        for (; i < ASCON_AEAD128_NONCE_SZ + SALT_SIZE; i++) {
            nonce[i - SALT_SIZE] = input[i];
        }

        /* finds tag from input message */
        for (; i < FILE_HEADER_SIZE; i++) {
            tag[i - SALT_SIZE - ASCON_AEAD128_NONCE_SZ] = input[i];
        }

        /* replicates old key if passwords match */
        ret = wc_PBKDF2(key, key, strlen((const char*)key), salt, SALT_SIZE, 4096,size, WC_SHA256);
        if (ret != 0) {
            printf("Replicating old key failed.\n");
            goto cleanup;
        }

        /* sets key */
        ret = wc_AsconAEAD128_SetKey(ascon, key);
        if (ret != 0) {
            printf("Set key failed.\n");
            goto cleanup;
        }

        /* sets nonce */
        ret = wc_AsconAEAD128_SetNonce(ascon, nonce);
        if (ret != 0) {
            printf("Set nonce failed.\n");
            goto cleanup;
        }

        /* sets additional data */
        ret = wc_AsconAEAD128_SetAD(ascon, ad, AD_SIZE);
        if (ret != 0) {
            printf("Set AD failed.\n");
            goto cleanup;
        }

        /* change length to remove salt/nonce/tag block from being decrypted */
        length -= FILE_HEADER_SIZE;
        for (int j = 0; j < length; j++) {
            /* shifts message: ignores salt/nonce/tag on message*/
            input[j] = input[j + FILE_HEADER_SIZE];
        }

        /* decrypts the message to output based on input length */
        ret = wc_AsconAEAD128_DecryptUpdate(ascon, output, input, length);
        if (ret != 0) {
            printf("Decrypt update failed.\n");
            goto cleanup;
        }

        /* Finalize decryption and verify tag */
        ret = wc_AsconAEAD128_DecryptFinal(ascon, tag);
        if (ret != 0) {
            printf("Decrypt final failed.\n");
            goto cleanup;
        }

        /* writes output to the outFile based on shortened length */
        fwrite(output, 1, length, outFile);
    } else {
        printf("Invalid length of input file\n");
    }

    cleanup:
        MemFree(ascon, key, size, inFile, outFile, &rng, input, output, aSize, rngInit, encrypt);
    return ret;
}

/*
 * help message
 */
void help()
{
    printf("\n~~~~~~~~~~~~~~~~~~~~|Help|~~~~~~~~~~~~~~~~~~~~~\n\n");
    printf("Usage: ./ascon-file-encrypt -d|-e -i <file.in> "
    "-o <file.out>\n\n");
    printf("Options\n");
    printf("-d    Decryption\n-e    Encryption\n-i    Input file\n");
    printf("-o    Output file\n-h    Help\n");
}

/*
 * temporarily disables echoing in terminal for secure key input
 */
int NoEcho(char* key)
{
    struct termios oflags, nflags;
    int ret = 0;

    /* disabling echo */
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;


    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        printf("Error: tcsetattr failed to disable terminal echo\n");
        ret = -1;
        goto restore;
    }

    printf("Unique Password: ");
    if (fgets(key, ASCON_AEAD128_KEY_SZ, stdin) == NULL) {
        printf("Error: fgets failed to retrieve secure key input\n");
        ret = -1;
        goto restore;
    }

    // Error out on no valid password.
    if (strlen(key) == 1) {
        printf("No password entered\n");
        ret = -1;
    }

    /* restore terminal regardless */
    restore:
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        printf("Error: tcsetattr failed to enable terminal echo\n");
        ret = -1;
    }
    return ret;
}



int main(int argc, char** argv)
{
    wc_AsconAEAD128*   ascon = NULL;
    byte*   key = NULL;       /* user entered key */
    FILE*  inFile = NULL;
    FILE*  outFile = NULL;

    const char* in;
    const char* out;

    int    option;    /* choice of how to run program */
    int    ret = 1;   /* return value */
    int    inCheck = 0;
    int    outCheck = 0;
    char   choice = 'n';



    while ((option = getopt(argc, argv, "dei:o:h")) != -1) {
        switch (option) {
            case 'd': /* if entered decrypt */
                choice = 'd';
                break;
            case 'e': /* if entered encrypt */
                choice = 'e';
                break;
            case 'h': /* if entered 'help' */
                help();
                break;
            case 'i': /* input file */
                in = optarg;
                inCheck = 1;
                inFile = fopen(in, "rb");
                if (inFile == NULL) {
                    printf("Error: unable to open input file\n");
                    return ret;
                }
                break;
            case 'o': /* output file */
                out = optarg;
                outCheck = 1;
                outFile = fopen(out, "wb");
                if (outFile == NULL) {
                    printf("Error: unable to open output file\n");
                    return ret;
                }
                break;
            case '?':
                if (optopt) {
                    printf("Ending Session\n");
                    return ret;
                }
            default:
                abort();
        }
    }

    if (inCheck == 0 || outCheck == 0) {
            printf("Must have both input and output file");
            printf(": -i filename -o filename\n");
    } else {
        if (choice != 'n') {
            key = malloc(ASCON_AEAD128_KEY_SZ);    /* sets size memory of key */
            if (key == NULL) {
                printf("Could not allocate memory for key\n");
                return ret;
            }
            ret = NoEcho((char*)key);
            if (ret != 0) {
                printf("Entering user password failed\n");
                free(key);
                return ret;
            }
            ascon = wc_AsconAEAD128_New();
            if (ascon == NULL) {
                free(key);
                printf("Error: initiating Ascon object failed\n");
                return 1;
            }
            if (choice == 'e') {
                ret = AsconEncrypt(ascon, key, ASCON_AEAD128_KEY_SZ, inFile, outFile);
                if (ret != 0) {
                    printf("Ascon encrypt failed\n");
                }
            } else  {
                ret = AsconDecrypt(ascon, key, ASCON_AEAD128_KEY_SZ, inFile, outFile);
                if (ret != 0) {
                    printf("Ascon decrypt failed\n");
                }
            }
        }
        else {
            printf("Must select either -e or -d for encryption and decryption\n");
        }
    }

    return ret;
}