/* ascon-file-encrypt.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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


typedef struct wc_AsconCtx {
    wc_AsconAEAD128*   ascon;
    byte*              key;
    FILE*              inFile;
    FILE*              outFile;
    const char*        outFileName;
    WC_RNG             rng;
    int                rngInit;
    int                encrypt;
    byte*              input;
    byte*              output;
    long               inFileLength;
} wc_AsconCtx;

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

void MemFree(wc_AsconCtx* ctx, int error) {
    if (ctx != NULL) {
        if (ctx->input != NULL) {
            memset(ctx->input, 0, ctx->encrypt ? BLOCK_SIZE : ctx->inFileLength);
            free(ctx->input);
        }
        if (ctx->output != NULL) {
            memset(ctx->output, 0, ctx->inFileLength);
            free(ctx->output);
        }
        if (ctx->outFile != NULL) {
            fclose(ctx->outFile);
            if (error != 0) {
                remove(ctx->outFileName);
            }

        }
        if (ctx->inFile != NULL) {
            fclose(ctx->inFile);
        }
        if (ctx->key != NULL) {
            memset(ctx->key, 0, ASCON_AEAD128_KEY_SZ);
            free(ctx->key);
        }
        /* Those two functions check validity of pointer before freeing it*/
        if (ctx->rngInit && &ctx->rng != NULL) {
            wc_FreeRng(&ctx->rng);
        }

        if (ctx->ascon != NULL) {
            wc_AsconAEAD128_Free(ctx->ascon);
        }

        memset(ctx, 0, sizeof(wc_AsconCtx));
        free(ctx);
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
int AsconEncrypt(wc_AsconCtx* ctx)
{

    byte     nonce[ASCON_AEAD128_NONCE_SZ] = {0};
    byte     salt[SALT_SIZE] = {0};
    byte     tag[ASCON_AEAD128_TAG_SZ] = {0};
    int      ret = 1;
    int      chunk_read = BLOCK_SIZE;

    ctx->encrypt = 1;

    fseek(ctx->inFile, 0, SEEK_END);
    ctx->inFileLength = ftell(ctx->inFile);
    fseek(ctx->inFile, 0, SEEK_SET);

    ctx->input = malloc(BLOCK_SIZE);
    ctx->output = malloc(ctx->inFileLength);

    if (ctx->input == NULL || ctx->output == NULL) {
        return ret;
    }

    ret = wc_InitRng(&ctx->rng);
    if (ret != 0) {
        printf("Failed to initialize random number generator\n");
        return ret;
    }
    ctx->rngInit = 1;

    /* stretches key to fit size */
    ret = GenerateKey(&ctx->rng, ctx->key, ASCON_AEAD128_KEY_SZ, salt);
    if (ret != 0) {
        printf("Generate key failed.\n");
        return ret;
    }

    /* sets key */
    ret = wc_AsconAEAD128_SetKey(ctx->ascon, ctx->key);
    if (ret != 0) {
        printf("Set key failed.\n");
        return ret;
    }

    /* Generate random nonce for each encryption */
    ret = wc_RNG_GenerateBlock(&ctx->rng, nonce, ASCON_AEAD128_NONCE_SZ);
    if (ret != 0) {
        printf("Generate nonce failed.\n");
        return ret;
    }

    /* sets nonce */
    ret = wc_AsconAEAD128_SetNonce(ctx->ascon, nonce);
    if (ret != 0) {
        printf("Set nonce failed.\n");
        return ret;
    }


    /* sets additional data */
    ret = wc_AsconAEAD128_SetAD(ctx->ascon, ad, AD_SIZE);
    if (ret != 0) {
        printf("Set AD failed.\n");
        return ret;
    }

    /* Loop reading a block at a time */
    for (int j = 0; j < ctx->inFileLength; j += BLOCK_SIZE) {
        if (chunk_read > ctx->inFileLength - j) {
            chunk_read = ctx->inFileLength - j;
        }

        ret = fread(ctx->input, 1, chunk_read, ctx->inFile);
        if (ret != chunk_read) {
            printf("ERROR: Failed to read the appropriate amount\n");
            ret = -1;
            return ret;
        }

        /* encrypts the message to the output based on input length */
        ret = wc_AsconAEAD128_EncryptUpdate(ctx->ascon, ctx->output+j, ctx->input, chunk_read);
        if (ret != 0) {
            printf("Encrypt update failed.\n");
            return ret;
        }

    }

    /* Finalize encryption and generate tag */
    ret = wc_AsconAEAD128_EncryptFinal(ctx->ascon, tag);
    if (ret != 0) {
        printf("Encrypt final failed.\n");
        return ret;
    }

    /* writes to outFile */
    fwrite(salt, 1, SALT_SIZE, ctx->outFile);
    fwrite(nonce, 1, ASCON_AEAD128_NONCE_SZ, ctx->outFile);
    fwrite(tag, 1, ASCON_AEAD128_TAG_SZ, ctx->outFile);
    fwrite(ctx->output, 1, ctx->inFileLength, ctx->outFile);

    return ret;
}

/*
 * Decrypts a file using Ascon
 */
int AsconDecrypt(wc_AsconCtx* ctx)
{
    byte     nonce[ASCON_AEAD128_NONCE_SZ] = {0};
    byte     salt[SALT_SIZE] = {0};
    byte     tag[ASCON_AEAD128_TAG_SZ] = {0};
    int      ret = 1;
    long     length = 0;

    ctx->encrypt = 0;

    fseek(ctx->inFile, 0, SEEK_END);
    length = ftell(ctx->inFile);
    fseek(ctx->inFile, 0, SEEK_SET);
    ctx->inFileLength = length;

    ctx->input = malloc(ctx->inFileLength);
    ctx->output = malloc(ctx->inFileLength);

    if (ctx->input == NULL || ctx->output == NULL) {
        return ret;
    }


    ret = wc_InitRng(&ctx->rng);
    if (ret != 0) {
        printf("Failed to initialize random number generator\n");
        return ret;
    }
    ctx->rngInit = 1;

    /* reads from inFile and writes whatever is there to the input array */
    ret = fread(ctx->input, 1, length, ctx->inFile);
    if (ret != length) {
        printf("Error while reading input file.\n");
        ret = 1;
        return ret;
    }

    int i = 0;

    if (length > FILE_HEADER_SIZE) {

        /* finds salt from input message */
        for (; i < SALT_SIZE; i++) {
            salt[i] = ctx->input[i];
        }

        /* finds nonce from input message */
        for (; i < ASCON_AEAD128_NONCE_SZ + SALT_SIZE; i++) {
            nonce[i - SALT_SIZE] = ctx->input[i];
        }

        /* finds tag from input message */
        for (; i < FILE_HEADER_SIZE; i++) {
            tag[i - SALT_SIZE - ASCON_AEAD128_NONCE_SZ] = ctx->input[i];
        }

        /* replicates old key if passwords match */
        ret = wc_PBKDF2(ctx->key, ctx->key, strlen((const char*)ctx->key), salt, SALT_SIZE, 4096,ASCON_AEAD128_KEY_SZ, WC_SHA256);
        if (ret != 0) {
            printf("Replicating old key failed.\n");
            return ret;
        }

        /* sets key */
        ret = wc_AsconAEAD128_SetKey(ctx->ascon, ctx->key);
        if (ret != 0) {
            printf("Set key failed.\n");
            return ret;
        }

        /* sets nonce */
        ret = wc_AsconAEAD128_SetNonce(ctx->ascon, nonce);
        if (ret != 0) {
            printf("Set nonce failed.\n");
            return ret;
        }

        /* sets additional data */
        ret = wc_AsconAEAD128_SetAD(ctx->ascon, ad, AD_SIZE);
        if (ret != 0) {
            printf("Set AD failed.\n");
            return ret;
        }

        /* change length to remove salt/nonce/tag block from being decrypted */
        length -= FILE_HEADER_SIZE;
        for (int j = 0; j < length; j++) {
            /* shifts message: ignores salt/nonce/tag on message*/
            ctx->input[j] = ctx->input[j + FILE_HEADER_SIZE];
        }

        /* decrypts the message to output based on input length */
        ret = wc_AsconAEAD128_DecryptUpdate(ctx->ascon, ctx->output, ctx->input, length);
        if (ret != 0) {
            printf("Decrypt update failed.\n");
            return ret;
        }

        /* Finalize decryption and verify tag */
        ret = wc_AsconAEAD128_DecryptFinal(ctx->ascon, tag);
        if (ret != 0) {
            printf("Decrypt final failed.\n");
            return ret;
        }

        /* writes output to the outFile based on shortened length */
        fwrite(ctx->output, 1, length, ctx->outFile);
    } else {
        printf("Invalid length of input file\n");
    }

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

    key[strlen(key) - 1] = 0;
    // Error out on no valid password.
    if (key[0] == 0) {
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
    int    ret = 1;   /* return value */
    wc_AsconCtx* ctx = (wc_AsconCtx*) malloc(sizeof(wc_AsconCtx));
    if (ctx == NULL) {
        printf("Memory allocation for ctx failed.\n");
        return ret;
    }
    ctx->ascon = NULL;
    ctx->key = NULL;
    ctx->inFile = NULL;
    ctx->outFile = NULL;
    ctx->input = NULL;
    ctx->output = NULL;
    ctx->outFile = NULL;


    const char* in;

    int    option;    /* choice of how to run program */
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
                ctx->inFile = fopen(in, "rb");
                if (ctx->inFile == NULL) {
                    printf("Error: unable to open input file\n");
                    goto cleanup;
                }
                break;
            case 'o': /* output file */
                ctx->outFileName = optarg;
                outCheck = 1;
                ctx->outFile = fopen(ctx->outFileName, "wb");
                if (ctx->outFile == NULL) {
                    printf("Error: unable to open output file\n");
                    goto cleanup;
                }
                break;
            case '?':
                if (optopt) {
                    printf("Ending Session\n");
                    goto cleanup;
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
            ctx->key = malloc(ASCON_AEAD128_KEY_SZ);    /* sets size memory of key */
            if (ctx->key == NULL) {
                printf("Could not allocate memory for key\n");
                goto cleanup;
            }
            ret = NoEcho((char*)ctx->key);
            if (ret != 0) {
                printf("Entering user password failed\n");
                goto cleanup;
            }
            ctx->ascon = wc_AsconAEAD128_New();
            if (ctx->ascon == NULL) {
                printf("Error: initiating Ascon object failed\n");
                ret = 1;
                goto cleanup;
            }
            if (choice == 'e') {
                ret = AsconEncrypt(ctx);
                if (ret != 0) {
                    printf("Ascon encrypt failed\n");
                }
            } else  {
                ret = AsconDecrypt(ctx);
                if (ret != 0) {
                    printf("Ascon decrypt failed\n");
                }
            }
        }
        else {
            printf("Must select either -e or -d for encryption and decryption\n");
        }
    }

    cleanup:
        MemFree(ctx, ret);
    return ret;
}