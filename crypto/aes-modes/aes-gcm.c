/* aes-gcm.c
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

/* AES-GCM Example
 * This example demonstrates:
 * - One-shot encryption using wc_AesGcmEncrypt()
 * - Streaming decryption using wc_AesGcmDecryptInit/Update/Final()
 * Note: GCM provides authenticated encryption (AEAD)
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

#if !defined(NO_AES) && defined(HAVE_AESGCM)

#define AES_KEY_SIZE    AES_256_KEY_SIZE
#define GCM_IV_SIZE     12  /* Recommended IV size for GCM */
#define GCM_TAG_SIZE    16  /* Authentication tag size */
#define CHUNK_SIZE      64  /* Small chunk size to demonstrate streaming */

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
    byte   iv[GCM_IV_SIZE];
    byte   authTag[GCM_TAG_SIZE];
    byte*  plaintext = NULL;
    byte*  output = NULL;
    word32 plaintextSz;
    word32 outputSz;
    int    ret;

    ret = read_file(inFile, &plaintext, &plaintextSz);
    if (ret != 0) return ret;

    /* Output: IV + authTag + ciphertext */
    outputSz = GCM_IV_SIZE + GCM_TAG_SIZE + plaintextSz;
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

    ret = wc_RNG_GenerateBlock(&rng, iv, GCM_IV_SIZE);
    wc_FreeRng(&rng);
    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    /* Initialize AES-GCM */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    ret = wc_AesGcmSetKey(&aes, key, keySz);
    if (ret != 0) {
        wc_AesFree(&aes);
        free(plaintext);
        free(output);
        return ret;
    }

    /* One-shot encrypt with authentication */
    ret = wc_AesGcmEncrypt(&aes, output + GCM_IV_SIZE + GCM_TAG_SIZE,
                           plaintext, plaintextSz,
                           iv, GCM_IV_SIZE,
                           authTag, GCM_TAG_SIZE,
                           NULL, 0);  /* No additional authenticated data */
    wc_AesFree(&aes);

    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    /* Prepend IV and authTag to output */
    memcpy(output, iv, GCM_IV_SIZE);
    memcpy(output + GCM_IV_SIZE, authTag, GCM_TAG_SIZE);

    ret = write_file(outFile, output, outputSz);

    free(plaintext);
    free(output);

    printf("AES-GCM encryption complete (one-shot)\n");
    return ret;
}

#ifdef WOLFSSL_AESGCM_STREAM
/* Streaming decryption using Init/Update/Final API */
static int decrypt_file(const char* inFile, const char* outFile,
                        const byte* key, word32 keySz)
{
    Aes    aes;
    byte   iv[GCM_IV_SIZE];
    byte   authTag[GCM_TAG_SIZE];
    byte*  input = NULL;
    byte*  plaintext = NULL;
    word32 inputSz;
    word32 ciphertextSz;
    word32 offset;
    word32 chunkSz;
    int    ret;

    ret = read_file(inFile, &input, &inputSz);
    if (ret != 0) return ret;

    if (inputSz < GCM_IV_SIZE + GCM_TAG_SIZE) {
        free(input);
        printf("Error: File too small\n");
        return -1;
    }

    /* Extract IV and authTag from beginning */
    memcpy(iv, input, GCM_IV_SIZE);
    memcpy(authTag, input + GCM_IV_SIZE, GCM_TAG_SIZE);

    ciphertextSz = inputSz - GCM_IV_SIZE - GCM_TAG_SIZE;
    plaintext = (byte*)malloc(ciphertextSz);
    if (plaintext == NULL) {
        free(input);
        return -1;
    }

    /* Initialize AES-GCM for streaming decryption */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(input);
        free(plaintext);
        return ret;
    }

    ret = wc_AesGcmDecryptInit(&aes, key, keySz, iv, GCM_IV_SIZE);
    if (ret != 0) {
        wc_AesFree(&aes);
        free(input);
        free(plaintext);
        return ret;
    }

    /* Streaming decrypt - process in chunks */
    printf("AES-GCM streaming decryption:\n");
    offset = 0;
    while (offset < ciphertextSz) {
        chunkSz = ciphertextSz - offset;
        if (chunkSz > CHUNK_SIZE) {
            chunkSz = CHUNK_SIZE;
        }

        ret = wc_AesGcmDecryptUpdate(&aes, plaintext + offset,
                                     input + GCM_IV_SIZE + GCM_TAG_SIZE + offset,
                                     chunkSz, NULL, 0);
        if (ret != 0) {
            wc_AesFree(&aes);
            free(input);
            free(plaintext);
            return ret;
        }

        printf("  Decrypted chunk: offset=%u, size=%u\n", offset, chunkSz);
        offset += chunkSz;
    }

    /* Finalize and verify authentication tag */
    ret = wc_AesGcmDecryptFinal(&aes, authTag, GCM_TAG_SIZE);
    wc_AesFree(&aes);

    if (ret != 0) {
        free(input);
        free(plaintext);
        if (ret == AES_GCM_AUTH_E) {
            printf("Error: Authentication failed!\n");
        }
        return ret;
    }

    ret = write_file(outFile, plaintext, ciphertextSz);

    free(input);
    free(plaintext);

    printf("AES-GCM decryption complete (streaming) - authentication verified\n");
    return ret;
}
#else
/* Fallback: One-shot decryption when streaming not available */
static int decrypt_file(const char* inFile, const char* outFile,
                        const byte* key, word32 keySz)
{
    Aes    aes;
    byte   iv[GCM_IV_SIZE];
    byte   authTag[GCM_TAG_SIZE];
    byte*  input = NULL;
    byte*  plaintext = NULL;
    word32 inputSz;
    word32 ciphertextSz;
    int    ret;

    ret = read_file(inFile, &input, &inputSz);
    if (ret != 0) return ret;

    if (inputSz < GCM_IV_SIZE + GCM_TAG_SIZE) {
        free(input);
        printf("Error: File too small\n");
        return -1;
    }

    /* Extract IV and authTag from beginning */
    memcpy(iv, input, GCM_IV_SIZE);
    memcpy(authTag, input + GCM_IV_SIZE, GCM_TAG_SIZE);

    ciphertextSz = inputSz - GCM_IV_SIZE - GCM_TAG_SIZE;
    plaintext = (byte*)malloc(ciphertextSz);
    if (plaintext == NULL) {
        free(input);
        return -1;
    }

    /* Initialize AES-GCM */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(input);
        free(plaintext);
        return ret;
    }

    ret = wc_AesGcmSetKey(&aes, key, keySz);
    if (ret != 0) {
        wc_AesFree(&aes);
        free(input);
        free(plaintext);
        return ret;
    }

    /* One-shot decrypt with authentication verification */
    ret = wc_AesGcmDecrypt(&aes, plaintext,
                           input + GCM_IV_SIZE + GCM_TAG_SIZE, ciphertextSz,
                           iv, GCM_IV_SIZE,
                           authTag, GCM_TAG_SIZE,
                           NULL, 0);
    wc_AesFree(&aes);

    if (ret != 0) {
        free(input);
        free(plaintext);
        if (ret == AES_GCM_AUTH_E) {
            printf("Error: Authentication failed!\n");
        }
        return ret;
    }

    ret = write_file(outFile, plaintext, ciphertextSz);

    free(input);
    free(plaintext);

    printf("AES-GCM decryption complete (one-shot, streaming not available)\n");
    return ret;
}
#endif /* WOLFSSL_AESGCM_STREAM */

int main(int argc, char** argv)
{
    byte key[AES_KEY_SIZE];
    int  ret;

    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        printf("Encrypts input file (one-shot), then decrypts to output file\n");
#ifdef WOLFSSL_AESGCM_STREAM
        printf("Streaming decryption enabled\n");
#else
        printf("Streaming decryption not available (enable WOLFSSL_AESGCM_STREAM)\n");
#endif
        return 1;
    }

    wolfCrypt_Init();

    /* Use a fixed key for demonstration */
    memset(key, 0x09, AES_KEY_SIZE);

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
    printf("AES-GCM not compiled in. Enable with HAVE_AESGCM\n");
    return 0;
}

#endif
