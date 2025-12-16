/* aes-xts.c
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

/* AES-XTS Example
 * This example demonstrates:
 * - One-shot encryption using wc_AesXtsEncrypt()
 * - Streaming decryption using wc_AesXtsDecryptInit/Update/Final()
 * Note: XTS is designed for disk encryption (sector-based)
 *       Requires minimum 16 bytes input
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

#if !defined(NO_AES) && defined(WOLFSSL_AES_XTS)

#define AES_XTS_KEY_SIZE    (AES_256_KEY_SIZE * 2)  /* XTS uses two keys */
#define XTS_TWEAK_SIZE      16  /* Tweak/IV size */
#define CHUNK_SIZE          64  /* Chunk size for streaming demo */

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
    XtsAes xts;
    WC_RNG rng;
    byte   tweak[XTS_TWEAK_SIZE];
    byte*  plaintext = NULL;
    byte*  output = NULL;
    word32 plaintextSz;
    word32 outputSz;
    int    ret;

    ret = read_file(inFile, &plaintext, &plaintextSz);
    if (ret != 0) return ret;

    /* XTS requires minimum 16 bytes */
    if (plaintextSz < AES_BLOCK_SIZE) {
        printf("Error: Input must be at least %d bytes for XTS\n", AES_BLOCK_SIZE);
        free(plaintext);
        return -1;
    }

    /* Output: tweak + ciphertext */
    outputSz = XTS_TWEAK_SIZE + plaintextSz;
    output = (byte*)malloc(outputSz);
    if (output == NULL) {
        free(plaintext);
        return -1;
    }

    /* Generate random tweak */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    ret = wc_RNG_GenerateBlock(&rng, tweak, XTS_TWEAK_SIZE);
    wc_FreeRng(&rng);
    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    /* Initialize AES-XTS */
    ret = wc_AesXtsSetKey(&xts, key, keySz, AES_ENCRYPTION, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    /* One-shot encrypt */
    ret = wc_AesXtsEncrypt(&xts, output + XTS_TWEAK_SIZE, plaintext,
                           plaintextSz, tweak, XTS_TWEAK_SIZE);
    wc_AesXtsFree(&xts);

    if (ret != 0) {
        free(plaintext);
        free(output);
        return ret;
    }

    /* Prepend tweak to output */
    memcpy(output, tweak, XTS_TWEAK_SIZE);

    ret = write_file(outFile, output, outputSz);

    free(plaintext);
    free(output);

    printf("AES-XTS encryption complete (one-shot)\n");
    return ret;
}

#ifdef WOLFSSL_AESXTS_STREAM
/* Streaming decryption using Init/Update/Final API */
static int decrypt_file(const char* inFile, const char* outFile,
                        const byte* key, word32 keySz)
{
    XtsAes xts;
    struct XtsAesStreamData stream;
    byte   tweak[XTS_TWEAK_SIZE];
    byte*  input = NULL;
    byte*  plaintext = NULL;
    word32 inputSz;
    word32 ciphertextSz;
    word32 offset;
    word32 chunkSz;
    word32 remaining;
    int    ret;

    ret = read_file(inFile, &input, &inputSz);
    if (ret != 0) return ret;

    if (inputSz < XTS_TWEAK_SIZE + AES_BLOCK_SIZE) {
        free(input);
        printf("Error: File too small\n");
        return -1;
    }

    /* Extract tweak from beginning */
    memcpy(tweak, input, XTS_TWEAK_SIZE);

    ciphertextSz = inputSz - XTS_TWEAK_SIZE;
    plaintext = (byte*)malloc(ciphertextSz);
    if (plaintext == NULL) {
        free(input);
        return -1;
    }

    /* Initialize AES-XTS for streaming decryption */
    ret = wc_AesXtsSetKey(&xts, key, keySz, AES_DECRYPTION, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(input);
        free(plaintext);
        return ret;
    }

    ret = wc_AesXtsDecryptInit(&xts, tweak, XTS_TWEAK_SIZE, &stream);
    if (ret != 0) {
        wc_AesXtsFree(&xts);
        free(input);
        free(plaintext);
        return ret;
    }

    /* Streaming decrypt - process in chunks */
    printf("AES-XTS streaming decryption:\n");
    offset = 0;
    remaining = ciphertextSz;

    while (remaining > AES_BLOCK_SIZE) {
        chunkSz = remaining - AES_BLOCK_SIZE;  /* Leave at least one block for final */
        if (chunkSz > CHUNK_SIZE) {
            chunkSz = CHUNK_SIZE;
        }
        /* Ensure chunk is block-aligned for Update */
        chunkSz = (chunkSz / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
        if (chunkSz == 0) break;

        ret = wc_AesXtsDecryptUpdate(&xts, plaintext + offset,
                                     input + XTS_TWEAK_SIZE + offset,
                                     chunkSz, &stream);
        if (ret != 0) {
            wc_AesXtsFree(&xts);
            free(input);
            free(plaintext);
            return ret;
        }

        printf("  Decrypted chunk: offset=%u, size=%u\n", offset, chunkSz);
        offset += chunkSz;
        remaining -= chunkSz;
    }

    /* Final chunk */
    ret = wc_AesXtsDecryptFinal(&xts, plaintext + offset,
                                input + XTS_TWEAK_SIZE + offset,
                                remaining, &stream);
    wc_AesXtsFree(&xts);

    if (ret != 0) {
        free(input);
        free(plaintext);
        return ret;
    }

    printf("  Decrypted final chunk: offset=%u, size=%u\n", offset, remaining);

    ret = write_file(outFile, plaintext, ciphertextSz);

    free(input);
    free(plaintext);

    printf("AES-XTS decryption complete (streaming)\n");
    return ret;
}
#else
/* Fallback: One-shot decryption when streaming not available */
static int decrypt_file(const char* inFile, const char* outFile,
                        const byte* key, word32 keySz)
{
    XtsAes xts;
    byte   tweak[XTS_TWEAK_SIZE];
    byte*  input = NULL;
    byte*  plaintext = NULL;
    word32 inputSz;
    word32 ciphertextSz;
    int    ret;

    ret = read_file(inFile, &input, &inputSz);
    if (ret != 0) return ret;

    if (inputSz < XTS_TWEAK_SIZE + AES_BLOCK_SIZE) {
        free(input);
        printf("Error: File too small\n");
        return -1;
    }

    /* Extract tweak from beginning */
    memcpy(tweak, input, XTS_TWEAK_SIZE);

    ciphertextSz = inputSz - XTS_TWEAK_SIZE;
    plaintext = (byte*)malloc(ciphertextSz);
    if (plaintext == NULL) {
        free(input);
        return -1;
    }

    /* Initialize AES-XTS */
    ret = wc_AesXtsSetKey(&xts, key, keySz, AES_DECRYPTION, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(input);
        free(plaintext);
        return ret;
    }

    /* One-shot decrypt */
    ret = wc_AesXtsDecrypt(&xts, plaintext, input + XTS_TWEAK_SIZE,
                           ciphertextSz, tweak, XTS_TWEAK_SIZE);
    wc_AesXtsFree(&xts);

    if (ret != 0) {
        free(input);
        free(plaintext);
        return ret;
    }

    ret = write_file(outFile, plaintext, ciphertextSz);

    free(input);
    free(plaintext);

    printf("AES-XTS decryption complete (one-shot, streaming not available)\n");
    return ret;
}
#endif /* WOLFSSL_AESXTS_STREAM */

int main(int argc, char** argv)
{
    byte key[AES_XTS_KEY_SIZE];
    int  ret;

    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        printf("Encrypts input file (one-shot), then decrypts to output file\n");
        printf("Note: Input must be at least 16 bytes\n");
#ifdef WOLFSSL_AESXTS_STREAM
        printf("Streaming decryption enabled\n");
#else
        printf("Streaming decryption not available (enable WOLFSSL_AESXTS_STREAM)\n");
#endif
        return 1;
    }

    wolfCrypt_Init();

    /* Use a fixed key for demonstration (XTS uses two keys) */
    memset(key, 0x0D, AES_XTS_KEY_SIZE);

    /* Encrypt to temporary file */
    ret = encrypt_file(argv[1], "temp_encrypted.bin", key, AES_XTS_KEY_SIZE);
    if (ret != 0) {
        printf("Encryption failed: %d\n", ret);
        wolfCrypt_Cleanup();
        return 1;
    }

    /* Decrypt to output file */
    ret = decrypt_file("temp_encrypted.bin", argv[2], key, AES_XTS_KEY_SIZE);
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
    printf("AES-XTS not compiled in. Enable with WOLFSSL_AES_XTS\n");
    return 0;
}

#endif
