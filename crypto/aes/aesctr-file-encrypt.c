/* aesctr-file-encrypt.c
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

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/pwdbased.h>

#if defined(HAVE_PBKDF2) && !defined(NO_PWDBASED) && \
    defined(WOLFSSL_AES_COUNTER)
#define SALT_SIZE 8

/*
 * Makes a cryptographically secure key by stretching a user entered key
 */
int GenerateKey(WC_RNG* rng, byte* key, int size, byte* salt)
{
    int ret;

    ret = wc_RNG_GenerateBlock(rng, salt, SALT_SIZE);
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
 * Encrypts a file using AES-CTR
 */
int AesCtrEncrypt(Aes* aes, byte* key, int size, FILE* inFile, FILE* outFile)
{
    WC_RNG     rng;
    byte    iv[AES_BLOCK_SIZE];
    byte*   input;
    byte*   output;
    byte    salt[SALT_SIZE] = {0};

    int     ret = 0;
    int     length;

    fseek(inFile, 0, SEEK_END);
    length = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    input = malloc(length);
    output = malloc(length);

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Failed to initialize random number generator\n");
        return -1030;
    }

    /* reads from inFile and writes whatever is there to the input array */
    ret = fread(input, 1, length, inFile);
    if (ret == 0) {
        printf("Input file does not exist.\n");
        return -1010;
    }

    ret = wc_RNG_GenerateBlock(&rng, iv, AES_BLOCK_SIZE);
    if (ret != 0)
        return -1020;

    /* stretches key to fit size */
    ret = GenerateKey(&rng, key, size, salt);
    if (ret != 0)
        return -1040;

    /* sets key */
    ret = wc_AesSetKey(aes, key, size, iv, AES_ENCRYPTION);
    if (ret != 0)
        return -1001;

    /* encrypts the message to the output based on input length + padding */
    ret = wc_AesCtrEncrypt(aes, output, input, length);
    if (ret != 0)
        return -1005;

    /* writes to outFile */
    fwrite(salt, 1, SALT_SIZE, outFile);
    fwrite(iv, 1, AES_BLOCK_SIZE, outFile);
    fwrite(output, 1, length, outFile);

    /* closes the opened files and frees the memory*/
    memset(input, 0, length);
    memset(output, 0, length);
    memset(key, 0, size);
    free(input);
    free(output);
    free(key);
    fclose(inFile);
    fclose(outFile);
    wc_FreeRng(&rng);

    return ret;
}

/*
 * Decrypts a file using AES-CTR
 */
int AesCtrDecrypt(Aes* aes, byte* key, int size, FILE* inFile, FILE* outFile)
{
    byte*   input;
    byte*   output;
    byte*   salt;
    byte*   iv;
    byte*   c;

    int     ret = 0;
    int     bufSz;
    int     cSz;

    fseek(inFile, 0, SEEK_END);
    bufSz = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    input = malloc(bufSz);
    output = malloc(bufSz);

    /* reads from inFile and writes whatever is there to the input array */
    ret = fread(input, 1, bufSz, inFile);
    if (ret == 0) {
        printf("Input file does not exist.\n");
        return -1010;
    }

    salt = input;
    iv = input + SALT_SIZE;
    c = input + SALT_SIZE + AES_BLOCK_SIZE;
    cSz = bufSz - SALT_SIZE - AES_BLOCK_SIZE;

    /* replicates old key if keys match */
    ret = wc_PBKDF2(key, key, strlen((const char*)key), salt, SALT_SIZE, 4096,
        size, WC_SHA256);
    if (ret != 0)
        return -1050;

    /* sets key */
    /* decrypt uses AES_ENCRYPTION */
    ret = wc_AesSetKey(aes, key, size, iv, AES_ENCRYPTION);
    if (ret != 0)
        return -1002;

    ret = wc_AesCtrEncrypt(aes, output, c, cSz);
    if (ret != 0)
        return -1006;

    /* writes output to the outFile based on shortened length */
    fwrite(output, 1, cSz, outFile);

    /* closes the opened files and frees the memory*/
    memset(input, 0, bufSz);
    memset(output, 0, bufSz);
    memset(key, 0, size);
    free(input);
    free(output);
    free(key);
    fclose(inFile);
    fclose(outFile);

    return 0;
}

/*
 * help message
 */
void help()
{
    printf("\n~~~~~~~~~~~~~~~~~~~~|Help|~~~~~~~~~~~~~~~~~~~~~\n\n");
    printf("Usage: ./aesctr-file-encrypt <-option> <KeySize> <-i file.in> "
        "<-o file.out>\n\n");
    printf("Options\n");
    printf("-d    Decryption\n-e    Encryption\n-h    Help\n");
}

/*
 * temporarily disables echoing in terminal for secure key input
 */
int NoEcho(char* key, int size)
{
    struct termios oflags, nflags;

    /* disabling echo */
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;

    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        printf("Error: tcsetattr failed to disable terminal echo\n");
        return -1060;
    }

    printf("Unique Password: ");
    if (fgets(key, size, stdin) == NULL) {
        printf("Error: fgets failed to retrieve secure key input\n");
        return -1070;
    }

    key[strlen(key) - 1] = 0;

    /* restore terminal */
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        printf("Error: tcsetattr failed to enable terminal echo\n");
        return -1080;
    }
    return 0;
}

int SizeCheck(int *size)
{
    int ret = 0;

    /* Use key size values (size/8) */
    if (*size == 128) {
        *size = AES_128_KEY_SIZE;
    }
    else if (*size == 192) {
        *size = AES_192_KEY_SIZE;
    }
    else if (*size == 256) {
        *size = AES_256_KEY_SIZE;
    }
    else {
        /* if the entered size does not match acceptable size */
        printf("Invalid AES key size\n");
        ret = -1080;
    }

    return ret;
}

int main(int argc, char** argv)
{
    Aes    aes;
    byte*  key;       /* user entered key */
    FILE*  inFile = NULL;
    FILE*  outFile = NULL;

    const char* in;
    const char* out;

    int    option;    /* choice of how to run program */
    int    ret = 0;   /* return value */
    int    size = 0;
    int    inCheck = 0;
    int    outCheck = 0;
    char   choice = 'n';

    while ((option = getopt(argc, argv, "d:e:i:o:h")) != -1) {
        switch (option) {
            case 'd': /* if entered decrypt */
                size = atoi(optarg);
                ret = SizeCheck(&size);
                choice = 'd';
                break;
            case 'e': /* if entered encrypt */
                size = atoi(optarg);
                ret = SizeCheck(&size);
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
                    printf("Ending Session\n");
                    return -111;
                }
            default:
                abort();
        }
    }
    if (inCheck == 0 || outCheck == 0) {
            printf("Must have both input and output file");
            printf(": -i filename -o filename\n");
    }
    else if (ret == 0 && choice != 'n' && inFile != NULL) {
        key = malloc(size);    /* sets size memory of key */
        ret = NoEcho((char*)key, size);
        if (choice == 'e')
            AesCtrEncrypt(&aes, key, size, inFile, outFile);
        else if (choice == 'd')
            AesCtrDecrypt(&aes, key, size, inFile, outFile);
    }
    else if (choice == 'n') {
        printf("Must select either -e[128, 192, 256] or -d[128, 192, 256] \
                for encryption and decryption\n");
        ret = -110;
    }

    return ret;
}

#else
int main()
{
    printf("Missing pwdbased, pbkdf2, or aes-ctr from wolfSSL\n");
    return 0;
}
#endif
