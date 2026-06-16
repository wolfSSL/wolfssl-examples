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
#define PASSWORD_SIZE      256
#define ERROR              (-1)
#define SUCCESS            0



typedef struct wc_AsconCtx {
    wc_AsconAEAD128*   ascon;
    byte*              key;
    byte*              password;
    FILE*              inFile;
    FILE*              outFile;
    const char*        outFileName;
    WC_RNG             rng;
    int                rngInit;
    byte*              plainText;
    byte*              cipherText;
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
        if (ctx->plainText != NULL) {
            memset(ctx->plainText, 0, BLOCK_SIZE);
            free(ctx->plainText);
        }
        if (ctx->cipherText != NULL) {
            memset(ctx->cipherText, 0, BLOCK_SIZE);
            free(ctx->cipherText);
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

        if (ctx->password != NULL) {
            memset(ctx->password, 0, PASSWORD_SIZE);
            free(ctx->password);
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
 * Makes a cryptographically secure key by stretching a user entered password
 */
int GenerateKey(WC_RNG* rng, byte* key, byte* password, byte* salt)
{
    if (wc_RNG_GenerateBlock(rng, salt, SALT_SIZE) != SUCCESS)
        return ERROR;

    /* stretches key */
    if (wc_PBKDF2(key, password, strlen((const char*)password), salt, SALT_SIZE, 4096,
        ASCON_AEAD128_KEY_SZ, WC_SHA256) != SUCCESS)
        return ERROR;

    return SUCCESS;
}

/*
 * Encrypts a file using Ascon
 */
int AsconEncrypt(wc_AsconCtx* ctx)
{
    byte     nonce[ASCON_AEAD128_NONCE_SZ] = {0};
    byte     salt[SALT_SIZE] = {0};
    byte     tag[ASCON_AEAD128_TAG_SZ] = {0};
    int      chunk_read = BLOCK_SIZE;


    if (fseek(ctx->inFile, 0, SEEK_END) != SUCCESS) {
        printf("fseek failed\n");
        return ERROR;
    }

    ctx->inFileLength = ftell(ctx->inFile);
    if (ctx->inFileLength < 0) {
        printf("ftell failed\n");
        return ERROR;
    }

    if (fseek(ctx->inFile, 0, SEEK_SET) != SUCCESS) {
        printf("fseek failed\n");
        return ERROR;
    }

    if (fseek(ctx->outFile, FILE_HEADER_SIZE, SEEK_SET) != SUCCESS) {
        printf("fseek failed\n");
        return ERROR;
    }

    ctx->plainText = malloc(BLOCK_SIZE);
    ctx->cipherText = malloc(BLOCK_SIZE);

    if (ctx->plainText == NULL || ctx->cipherText == NULL) {
        return ERROR;
    }

    if (wc_InitRng(&ctx->rng) != SUCCESS) {
        printf("Failed to initialize random number generator\n");
        return ERROR;
    }
    ctx->rngInit = 1;

    /* Generate key from password and salt */
    if (GenerateKey(&ctx->rng, ctx->key, ctx->password, salt) != SUCCESS) {
        printf("Generate key failed.\n");
        return ERROR;
    }

    /* sets key */
    if (wc_AsconAEAD128_SetKey(ctx->ascon, ctx->key) != SUCCESS) {
        printf("Set key failed.\n");
        return ERROR;
    }

    /* Generate random nonce for each encryption */
    if (wc_RNG_GenerateBlock(&ctx->rng, nonce, ASCON_AEAD128_NONCE_SZ) != SUCCESS) {
        printf("Generate nonce failed.\n");
        return ERROR;
    }

    /* sets nonce */
    if (wc_AsconAEAD128_SetNonce(ctx->ascon, nonce) != SUCCESS) {
        printf("Set nonce failed.\n");
        return ERROR;
    }

    /* sets additional data */
    if (wc_AsconAEAD128_SetAD(ctx->ascon, ad, AD_SIZE) != SUCCESS) {
        printf("Set AD failed.\n");
        return ERROR;
    }

    /* Loop reading a block at a time */
    for (long j = 0; j <= ctx->inFileLength; j += BLOCK_SIZE) {
        if (chunk_read > ctx->inFileLength - j) {
            chunk_read = ctx->inFileLength - j;
        }

        if (fread(ctx->plainText, 1, chunk_read, ctx->inFile) != chunk_read) {
            printf("ERROR: Failed to read the appropriate amount\n");
            return ERROR;
        }

        /* encrypts the message to the output based on input length */
        if (wc_AsconAEAD128_EncryptUpdate(ctx->ascon, ctx->cipherText, ctx->plainText, chunk_read) != SUCCESS) {
            printf("Encrypt update failed.\n");
            return ERROR;
        }

        /* write ciphertext to output file */
        if (fwrite(ctx->cipherText, 1, chunk_read, ctx->outFile) != chunk_read) {
            printf("ERROR: Failed to write the appropriate amount\n");
            return ERROR;
        }

    }

    /* Finalize encryption and generate tag */
    if (wc_AsconAEAD128_EncryptFinal(ctx->ascon, tag) != SUCCESS) {
        printf("Encrypt final failed.\n");
        return ERROR;
    }

    /* writes to outFile */
    if (fseek(ctx->outFile, 0, SEEK_SET) != SUCCESS) {
        printf("fseek failed\n");
        return ERROR;
    }

    if (fwrite(salt, 1, SALT_SIZE, ctx->outFile) != SALT_SIZE) {
        printf("ERROR: Failed to write the appropriate amount\n");
        return ERROR;
    }

    if (fwrite(nonce, 1, ASCON_AEAD128_NONCE_SZ, ctx->outFile) != ASCON_AEAD128_NONCE_SZ) {
        printf("ERROR: Failed to write the appropriate amount\n");
        return ERROR;
    }

    if (fwrite(tag, 1, ASCON_AEAD128_TAG_SZ, ctx->outFile) != ASCON_AEAD128_TAG_SZ) {
        printf("ERROR: Failed to write the appropriate amount\n");
        return ERROR;
    }

    return SUCCESS;
}

/*
 * Decrypts a file using Ascon
 */
int AsconDecrypt(wc_AsconCtx* ctx)
{
    byte     nonce[ASCON_AEAD128_NONCE_SZ] = {0};
    byte     salt[SALT_SIZE] = {0};
    byte     tag[ASCON_AEAD128_TAG_SZ] = {0};


    if (fseek(ctx->inFile, 0, SEEK_END) != SUCCESS) {
        printf("fseek failed\n");
        return ERROR;
    }

    ctx->inFileLength = ftell(ctx->inFile);
    if (ctx->inFileLength < 0) {
        printf("ftell failed\n");
        return ERROR;
    }

    if (fseek(ctx->inFile, 0, SEEK_SET) != SUCCESS) {
        printf("fseek failed\n");
        return ERROR;
    }


    ctx->plainText = malloc(BLOCK_SIZE);
    ctx->cipherText = malloc(BLOCK_SIZE);

    if (ctx->plainText == NULL || ctx->cipherText == NULL) {
        return ERROR;
    }

    if (wc_InitRng(&ctx->rng) != SUCCESS) {
        printf("Failed to initialize random number generator\n");
        return ERROR;
    }
    ctx->rngInit = 1;


    if (ctx->inFileLength >= FILE_HEADER_SIZE) {
        int      chunk_read = BLOCK_SIZE;
        int      i = 0;

        /* read the file header and extract salt, nonce, and tag */
        if (fread(ctx->cipherText, 1, FILE_HEADER_SIZE, ctx->inFile) != FILE_HEADER_SIZE) {
            printf("Error while reading input file.\n");
            return ERROR;
        }

        /* finds salt from input message */
        for (; i < SALT_SIZE; i++) {
            salt[i] = ctx->cipherText[i];
        }

        /* finds nonce from input message */
        for (; i < ASCON_AEAD128_NONCE_SZ + SALT_SIZE; i++) {
            nonce[i - SALT_SIZE] = ctx->cipherText[i];
        }

        /* finds tag from input message */
        for (; i < FILE_HEADER_SIZE; i++) {
            tag[i - SALT_SIZE - ASCON_AEAD128_NONCE_SZ] = ctx->cipherText[i];
        }

        /* replicates old key if passwords match */
        if (wc_PBKDF2(ctx->key, ctx->password, strlen((const char*)ctx->password), salt, SALT_SIZE, 4096,ASCON_AEAD128_KEY_SZ, WC_SHA256) != SUCCESS) {
            printf("Replicating old key failed.\n");
            return ERROR;
        }

        /* sets key */
        if (wc_AsconAEAD128_SetKey(ctx->ascon, ctx->key) != SUCCESS) {
            printf("Set key failed.\n");
            return ERROR;
        }

        /* sets nonce */
        if (wc_AsconAEAD128_SetNonce(ctx->ascon, nonce) != SUCCESS) {
            printf("Set nonce failed.\n");
            return ERROR;
        }

        /* sets additional data */
        if (wc_AsconAEAD128_SetAD(ctx->ascon, ad, AD_SIZE) != SUCCESS) {
            printf("Set AD failed.\n");
            return ERROR;
        }

        /* Start decrypting ciphertext */
        ctx->inFileLength -= FILE_HEADER_SIZE;

        for (long j = 0; j <= ctx->inFileLength; j += BLOCK_SIZE) {
            if (chunk_read > ctx->inFileLength - j) {
                chunk_read = ctx->inFileLength - j;
            }

            if (fread(ctx->cipherText, 1, chunk_read, ctx->inFile) != chunk_read) {
                printf("ERROR: Failed to read the appropriate amount\n");
                return ERROR;
            }

            /* decrypts chunk read */
            if (wc_AsconAEAD128_DecryptUpdate(ctx->ascon, ctx->plainText, ctx->cipherText, chunk_read) != SUCCESS) {
                printf("Decrypt update failed.\n");
                return ERROR;
            }

            /* write plaintext to output file */
            if (fwrite(ctx->plainText, 1, chunk_read, ctx->outFile) != chunk_read) {
                printf("ERROR: Failed to write the appropriate amount\n");
                return ERROR;
            }

        }

        /* Finalize decryption and verify tag */
        if (wc_AsconAEAD128_DecryptFinal(ctx->ascon, tag) != SUCCESS) {
            printf("Decrypt final failed.\n");
            return ERROR;
        }

    } else {
        printf("Invalid length of input file\n");
        return ERROR;
    }

    return SUCCESS;
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
int NoEcho(char* password)
{
    struct termios oflags, nflags;
    int ret = SUCCESS;

    /* disabling echo */
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;


    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        printf("Error: tcsetattr failed to disable terminal echo\n");
        ret = ERROR;
        goto restore;
    }

    printf("Unique Password: ");
    if (fgets(password, PASSWORD_SIZE, stdin) == NULL) {
        printf("Error: fgets failed to retrieve password input\n");
        ret = ERROR;
        goto restore;
    }

    // Remove trailing newline, if present
    password[strcspn(password, "\n")] = '\0';

    // Check if password is empty
    if (password[0] == '\0') {
        printf("No password entered\n");
        ret = ERROR;
    }

    /* restore terminal regardless */
    restore:
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        printf("Error: tcsetattr failed to enable terminal echo\n");
        ret = ERROR;
    }
    return ret;
}



int main(int argc, char** argv)
{
    int    ret = ERROR;   /* return value */
    wc_AsconCtx* ctx = (wc_AsconCtx*) malloc(sizeof(wc_AsconCtx));
    if (ctx == NULL) {
        printf("Memory allocation for ctx failed.\n");
        return ret;
    }
    ctx->ascon = NULL;
    ctx->key = NULL;
    ctx->password = NULL;
    ctx->inFile = NULL;
    ctx->outFile = NULL;
    ctx->plainText = NULL;
    ctx->cipherText = NULL;
    ctx->outFile = NULL;


    const char* in;

    int    option;    /* choice of how to run program */
    int    inCheck = 0;
    int    outCheck = 0;
    char   choice = 'n';



    while ((option = getopt(argc, argv, "dei:o:")) != -1) {
        switch (option) {
            case 'd': /* if entered decrypt */
                choice = 'd';
                break;
            case 'e': /* if entered encrypt */
                choice = 'e';
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
            default:
                help();
                printf("Ending Session\n");
                ret = SUCCESS;
                goto cleanup;
        }
    }

    if (inCheck == 0 || outCheck == 0) {
            printf("Must have both input and output file");
            printf(": -i filename -o filename\n");
    } else {
        if (choice != 'n') {
            ctx->key = malloc(ASCON_AEAD128_KEY_SZ);
            if (ctx->key == NULL) {
                printf("Could not allocate memory for key\n");
                goto cleanup;
            }

            ctx->password = malloc(PASSWORD_SIZE);
            if (ctx->password == NULL) {
                printf("Could not allocate memory for password\n");
                goto cleanup;
            }

            if (NoEcho((char*)ctx->password) != SUCCESS) {
                printf("Entering user password failed\n");
                goto cleanup;
            }
            ctx->ascon = wc_AsconAEAD128_New();
            if (ctx->ascon == NULL) {
                printf("Error: initiating Ascon object failed\n");
                goto cleanup;
            }
            if (choice == 'e') {
                ret = AsconEncrypt(ctx);
                if (ret != SUCCESS) {
                    printf("Ascon encrypt failed\n");
                }
            } else  {
                ret = AsconDecrypt(ctx);
                if (ret != SUCCESS) {
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