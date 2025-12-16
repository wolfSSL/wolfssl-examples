/* aes-cts.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* AES-CTS Example
 * This example demonstrates:
 * - One-shot encryption using wc_AesCtsEncrypt()
 * - One-shot decryption using wc_AesCtsDecrypt()
 * Note: CTS (Ciphertext Stealing) allows encryption of data that is not
 *       a multiple of the block size without padding expansion
 *       Requires minimum 16 bytes input
 * Note: CTS streaming API (Update/Final) requires careful handling of
 *       internal buffering and is not demonstrated in this example.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#if !defined(NO_AES) && defined(WOLFSSL_AES_CTS)

#define AES_KEY_SIZE    AES_256_KEY_SIZE

static int read_file(const char* filename, byte** data, word32* dataSz)
{
    FILE* fp;
    long fileSz;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Error: Cannot open file %s\n", filename);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    fileSz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *data = (byte*)malloc(fileSz);
    if (*data == NULL) {
        fclose(fp);
        printf("Error: Memory allocation failed\n");
        return -1;
    }

    *dataSz = (word32)fread(*data, 1, fileSz, fp);
    fclose(fp);

    return 0;
}

static int write_file(const char* filename, const byte* data, word32 dataSz)
{
    FILE* fp;

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("Error: Cannot create file %s\n", filename);
        return -1;
    }

    fwrite(data, 1, dataSz, fp);
    fclose(fp);

    return 0;
}

/* One-shot encryption */
static int encrypt_file(const char* inFile, const char* outFile,
                        const byte* key, word32 keySz)
{
    WC_RNG rng;
    byte   iv[AES_BLOCK_SIZE];
    byte*  plaintext = NULL;
    byte*  output = NULL;
    word32 plaintextSz;
    word32 outputSz;
    int    ret;

    ret = read_file(inFile, &plaintext, &plaintextSz);
    if (ret != 0) return ret;

    /* CTS requires minimum 16 bytes */
    if (plaintextSz < AES_BLOCK_SIZE) {
        printf("Error: Input must be at least %d bytes for CTS\n",
               AES_BLOCK_SIZE);
        free(plaintext);
        return -1;
    }

    /* Output: IV + ciphertext (same size as plaintext with CTS) */
    outputSz = AES_BLOCK_SIZE + plaintextSz;
    output = (byte*)malloc(outputSz);
    if (output == NULL) {
        free(plaintext);
        return -1;
    }

    /* Generate random IV */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    ret = wc_RNG_GenerateBlock(&rng, iv, AES_BLOCK_SIZE);
    wc_FreeRng(&rng);
    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    /* One-shot encrypt */
    ret = wc_AesCtsEncrypt(key, keySz, output + AES_BLOCK_SIZE,
                           plaintext, plaintextSz, iv);

    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    /* Prepend IV to output */
    memcpy(output, iv, AES_BLOCK_SIZE);

    ret = write_file(outFile, output, outputSz);

    free(plaintext);
    free(output);

    printf("AES-CTS encryption complete (one-shot)\n");
    return ret;
}

/* One-shot decryption (CTS streaming API requires complex buffering) */
static int decrypt_file(const char* inFile, const char* outFile,
                        const byte* key, word32 keySz)
{
    byte   iv[AES_BLOCK_SIZE];
    byte*  input = NULL;
    byte*  plaintext = NULL;
    word32 inputSz;
    word32 ciphertextSz;
    int    ret;

    ret = read_file(inFile, &input, &inputSz);
    if (ret != 0) return ret;

    if (inputSz < AES_BLOCK_SIZE * 2) {
        free(input);
        printf("Error: File too small\n");
        return -1;
    }

    /* Extract IV from beginning */
    memcpy(iv, input, AES_BLOCK_SIZE);

    ciphertextSz = inputSz - AES_BLOCK_SIZE;
    plaintext = (byte*)malloc(ciphertextSz);
    if (plaintext == NULL) {
        free(input);
        return -1;
    }

    /* One-shot decrypt */
    ret = wc_AesCtsDecrypt(key, keySz, plaintext,
                           input + AES_BLOCK_SIZE, ciphertextSz, iv);

    if (ret != 0) {
        free(input);
        free(plaintext);
        return ret;
    }

    ret = write_file(outFile, plaintext, ciphertextSz);

    free(input);
    free(plaintext);

    printf("AES-CTS decryption complete (one-shot, no streaming API "
           "available)\n");
    return ret;
}

int main(int argc, char** argv)
{
    byte key[AES_KEY_SIZE];
    int  ret;

    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        printf("Encrypts input file (one-shot), then decrypts to output file "
               "(streaming)\n");
        printf("Note: Input must be at least 16 bytes\n");
        return 1;
    }

    wolfCrypt_Init();

    /* Use a fixed key for demonstration */
    memset(key, 0x10, AES_KEY_SIZE);

    /* Encrypt to temporary file */
    ret = encrypt_file(argv[1], "temp_encrypted.bin", key, AES_KEY_SIZE);
    if (ret != 0) {
        printf("Encryption failed: %d\n", ret);
        wolfCrypt_Cleanup();
        return 1;
    }

    /* Decrypt to output file */
    ret = decrypt_file("temp_encrypted.bin", argv[2], key, AES_KEY_SIZE);
    if (ret != 0) {
        printf("Decryption failed: %d\n", ret);
        wolfCrypt_Cleanup();
        return 1;
    }

    /* Clean up temp file */
    remove("temp_encrypted.bin");

    printf("Success! Decrypted file written to %s\n", argv[2]);

    wolfCrypt_Cleanup();
    return 0;
}

#else

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    printf("AES-CTS not compiled in. Enable with WOLFSSL_AES_CTS\n");
    return 0;
}

#endif
