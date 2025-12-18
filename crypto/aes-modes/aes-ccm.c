/* aes-ccm.c
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

/* AES-CCM Example
 * This example demonstrates:
 * - One-shot encryption using wc_AesCcmEncrypt()
 * - One-shot decryption using wc_AesCcmDecrypt()
 * Note: CCM provides authenticated encryption (AEAD), no streaming API
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

#if !defined(NO_AES) && defined(HAVE_AESCCM)

#define AES_KEY_SIZE    AES_256_KEY_SIZE
#define CCM_NONCE_SIZE  12  /* Nonce size (7-13 bytes allowed) */
#define CCM_TAG_SIZE    16  /* Authentication tag size */

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
    Aes    aes;
    WC_RNG rng;
    byte   nonce[CCM_NONCE_SIZE];
    byte   authTag[CCM_TAG_SIZE];
    byte*  plaintext = NULL;
    byte*  output = NULL;
    word32 plaintextSz;
    word32 outputSz;
    int    ret;

    ret = read_file(inFile, &plaintext, &plaintextSz);
    if (ret != 0) return ret;

    /* Output: nonce + authTag + ciphertext */
    outputSz = CCM_NONCE_SIZE + CCM_TAG_SIZE + plaintextSz;
    output = (byte*)malloc(outputSz);
    if (output == NULL) {
        free(plaintext);
        return -1;
    }

    /* Generate random nonce */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    ret = wc_RNG_GenerateBlock(&rng, nonce, CCM_NONCE_SIZE);
    wc_FreeRng(&rng);
    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    /* Initialize AES-CCM */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    ret = wc_AesCcmSetKey(&aes, key, keySz);
    if (ret != 0) {
        wc_AesFree(&aes);
        free(plaintext);
        free(output);
        return ret;
    }

    /* One-shot encrypt with authentication */
    ret = wc_AesCcmEncrypt(&aes, output + CCM_NONCE_SIZE + CCM_TAG_SIZE,
                           plaintext, plaintextSz,
                           nonce, CCM_NONCE_SIZE,
                           authTag, CCM_TAG_SIZE,
                           NULL, 0);  /* No additional authenticated data */
    wc_AesFree(&aes);

    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    /* Prepend nonce and authTag to output */
    memcpy(output, nonce, CCM_NONCE_SIZE);
    memcpy(output + CCM_NONCE_SIZE, authTag, CCM_TAG_SIZE);

    ret = write_file(outFile, output, outputSz);

    free(plaintext);
    free(output);

    printf("AES-CCM encryption complete (one-shot)\n");
    return ret;
}

/* One-shot decryption (no streaming API available for CCM) */
static int decrypt_file(const char* inFile, const char* outFile,
                        const byte* key, word32 keySz)
{
    Aes    aes;
    byte   nonce[CCM_NONCE_SIZE];
    byte   authTag[CCM_TAG_SIZE];
    byte*  input = NULL;
    byte*  plaintext = NULL;
    word32 inputSz;
    word32 ciphertextSz;
    int    ret;

    ret = read_file(inFile, &input, &inputSz);
    if (ret != 0) return ret;

    if (inputSz < CCM_NONCE_SIZE + CCM_TAG_SIZE) {
        free(input);
        printf("Error: File too small\n");
        return -1;
    }

    /* Extract nonce and authTag from beginning */
    memcpy(nonce, input, CCM_NONCE_SIZE);
    memcpy(authTag, input + CCM_NONCE_SIZE, CCM_TAG_SIZE);

    ciphertextSz = inputSz - CCM_NONCE_SIZE - CCM_TAG_SIZE;
    plaintext = (byte*)malloc(ciphertextSz);
    if (plaintext == NULL) {
        free(input);
        return -1;
    }

    /* Initialize AES-CCM */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(input);
        free(plaintext);
        return ret;
    }

    ret = wc_AesCcmSetKey(&aes, key, keySz);
    if (ret != 0) {
        wc_AesFree(&aes);
        free(input);
        free(plaintext);
        return ret;
    }

    /* One-shot decrypt with authentication verification */
    ret = wc_AesCcmDecrypt(&aes, plaintext,
                           input + CCM_NONCE_SIZE + CCM_TAG_SIZE, ciphertextSz,
                           nonce, CCM_NONCE_SIZE,
                           authTag, CCM_TAG_SIZE,
                           NULL, 0);
    wc_AesFree(&aes);

    if (ret != 0) {
        free(input);
        free(plaintext);
        if (ret == AES_CCM_AUTH_E) {
            printf("Error: Authentication failed!\n");
        }
        return ret;
    }

    ret = write_file(outFile, plaintext, ciphertextSz);

    free(input);
    free(plaintext);

    printf("AES-CCM decryption complete (one-shot, no streaming API "
           "available)\n");
    return ret;
}

int main(int argc, char** argv)
{
    byte key[AES_KEY_SIZE];
    int  ret;

    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        printf("Encrypts input file, then decrypts to output file\n");
        return 1;
    }

    wolfCrypt_Init();

    /* Use a fixed key for demonstration */
    memset(key, 0x0B, AES_KEY_SIZE);

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
    printf("AES-CCM not compiled in. Enable with HAVE_AESCCM\n");
    return 0;
}

#endif
