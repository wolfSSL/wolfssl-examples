/* aes-direct.c
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

/* AES-DIRECT Example
 * This example demonstrates:
 * - One-shot encryption using wc_AesEncryptDirect() (single block)
 * - One-shot decryption using wc_AesDecryptDirect() (single block)
 * Note: DIRECT mode operates on single 16-byte blocks only
 *       No streaming API available - this is raw block cipher access
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#if !defined(NO_AES) && defined(WOLFSSL_AES_DIRECT)

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

/* One-shot encryption using direct block cipher */
static int encrypt_file(const char* inFile, const char* outFile,
                        const byte* key, word32 keySz)
{
    Aes    aes;
    byte*  plaintext = NULL;
    byte*  ciphertext = NULL;
    word32 plaintextSz;
    word32 paddedSz;
    word32 padLen;
    word32 i;
    int    ret;

    ret = read_file(inFile, &plaintext, &plaintextSz);
    if (ret != 0) return ret;

    /* Calculate padded size (PKCS#7 padding) */
    padLen = AES_BLOCK_SIZE - (plaintextSz % AES_BLOCK_SIZE);
    paddedSz = plaintextSz + padLen;

    /* Store original size at beginning (4 bytes) + ciphertext */
    ciphertext = (byte*)malloc(4 + paddedSz);
    if (ciphertext == NULL) {
        free(plaintext);
        return -1;
    }

    /* Apply PKCS#7 padding */
    plaintext = (byte*)realloc(plaintext, paddedSz);
    memset(plaintext + plaintextSz, (int)padLen, padLen);

    /* Initialize AES */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(plaintext);
        free(ciphertext);
        return ret;
    }

    ret = wc_AesSetKeyDirect(&aes, key, keySz, NULL, AES_ENCRYPTION);
    if (ret != 0) {
        wc_AesFree(&aes);
        free(plaintext);
        free(ciphertext);
        return ret;
    }

    /* Encrypt block by block using direct API */
    for (i = 0; i < paddedSz; i += AES_BLOCK_SIZE) {
        ret = wc_AesEncryptDirect(&aes, ciphertext + 4 + i, plaintext + i);
        if (ret != 0) {
            wc_AesFree(&aes);
            free(plaintext);
            free(ciphertext);
            return ret;
        }
    }

    wc_AesFree(&aes);

    /* Store original size at beginning */
    ciphertext[0] = (byte)(plaintextSz >> 24);
    ciphertext[1] = (byte)(plaintextSz >> 16);
    ciphertext[2] = (byte)(plaintextSz >> 8);
    ciphertext[3] = (byte)(plaintextSz);

    ret = write_file(outFile, ciphertext, 4 + paddedSz);

    free(plaintext);
    free(ciphertext);

    printf("AES-DIRECT encryption complete (one-shot, block by block)\n");
    return ret;
}

/* One-shot decryption using direct block cipher */
static int decrypt_file(const char* inFile, const char* outFile,
                        const byte* key, word32 keySz)
{
    Aes    aes;
    byte*  ciphertext = NULL;
    byte*  plaintext = NULL;
    word32 ciphertextSz;
    word32 plaintextSz;
    word32 originalSz;
    word32 i;
    int    ret;

    ret = read_file(inFile, &ciphertext, &ciphertextSz);
    if (ret != 0) return ret;

    if (ciphertextSz < 4 + AES_BLOCK_SIZE) {
        free(ciphertext);
        printf("Error: File too small\n");
        return -1;
    }

    /* Extract original size from beginning */
    originalSz = ((word32)ciphertext[0] << 24) |
                 ((word32)ciphertext[1] << 16) |
                 ((word32)ciphertext[2] << 8) |
                 ((word32)ciphertext[3]);

    plaintextSz = ciphertextSz - 4;
    plaintext = (byte*)malloc(plaintextSz);
    if (plaintext == NULL) {
        free(ciphertext);
        return -1;
    }

    /* Initialize AES */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(ciphertext);
        free(plaintext);
        return ret;
    }

    ret = wc_AesSetKeyDirect(&aes, key, keySz, NULL, AES_DECRYPTION);
    if (ret != 0) {
        wc_AesFree(&aes);
        free(ciphertext);
        free(plaintext);
        return ret;
    }

    /* Decrypt block by block using direct API */
    for (i = 0; i < plaintextSz; i += AES_BLOCK_SIZE) {
        ret = wc_AesDecryptDirect(&aes, plaintext + i, ciphertext + 4 + i);
        if (ret != 0) {
            wc_AesFree(&aes);
            free(ciphertext);
            free(plaintext);
            return ret;
        }
    }

    wc_AesFree(&aes);

    /* Use original size to remove padding */
    ret = write_file(outFile, plaintext, originalSz);

    free(ciphertext);
    free(plaintext);

    printf("AES-DIRECT decryption complete (one-shot, no streaming API available)\n");
    return ret;
}

int main(int argc, char** argv)
{
    byte key[AES_KEY_SIZE];
    int  ret;

    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        printf("Encrypts input file, then decrypts to output file\n");
        printf("Note: DIRECT mode provides raw block cipher access\n");
        return 1;
    }

    wolfCrypt_Init();

    /* Use a fixed key for demonstration */
    memset(key, 0x08, AES_KEY_SIZE);

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
    printf("AES-DIRECT not compiled in. Enable with WOLFSSL_AES_DIRECT\n");
    return 0;
}

#endif
