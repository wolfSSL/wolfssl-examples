/* aes-keywrap.c
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

/* AES-KEYWRAP Example
 * This example demonstrates:
 * - One-shot key wrapping using wc_AesKeyWrap()
 * - One-shot key unwrapping using wc_AesKeyUnWrap()
 * Note: Key wrap is designed for wrapping cryptographic keys
 *       Input must be multiple of 8 bytes (64 bits)
 *       No streaming API available
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

#if !defined(NO_AES) && defined(HAVE_AES_KEYWRAP)

#define AES_KEY_SIZE    AES_256_KEY_SIZE

/* Key wrap block size */
#define KEYWRAP_BLOCK   8

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

/* One-shot key wrapping */
static int wrap_file(const char* inFile, const char* outFile,
                     const byte* key, word32 keySz)
{
    byte*  plaintext = NULL;
    byte*  wrapped = NULL;
    word32 plaintextSz;
    word32 paddedSz;
    word32 wrappedSz;
    word32 padLen;
    int    ret;

    ret = read_file(inFile, &plaintext, &plaintextSz);
    if (ret != 0) return ret;

    /* Key wrap requires input to be multiple of 8 bytes */
    padLen = (KEYWRAP_BLOCK - (plaintextSz % KEYWRAP_BLOCK)) % KEYWRAP_BLOCK;
    paddedSz = plaintextSz + padLen;

    /* Wrapped output is input + 8 bytes (integrity check value) */
    /* Also store original size (4 bytes) at beginning */
    wrappedSz = 4 + paddedSz + KEYWRAP_BLOCK;
    wrapped = (byte*)malloc(wrappedSz);
    if (wrapped == NULL) {
        free(plaintext);
        return -1;
    }

    /* Apply padding if needed */
    if (padLen > 0) {
        plaintext = (byte*)realloc(plaintext, paddedSz);
        memset(plaintext + plaintextSz, 0, padLen);
    }

    /* One-shot key wrap */
    ret = wc_AesKeyWrap(key, keySz, plaintext, paddedSz,
                        wrapped + 4, wrappedSz - 4, NULL);

    if (ret < 0) {
        free(plaintext);
        free(wrapped);
        return ret;
    }

    /* Store original size at beginning */
    wrapped[0] = (byte)(plaintextSz >> 24);
    wrapped[1] = (byte)(plaintextSz >> 16);
    wrapped[2] = (byte)(plaintextSz >> 8);
    wrapped[3] = (byte)(plaintextSz);

    ret = write_file(outFile, wrapped, wrappedSz);

    free(plaintext);
    free(wrapped);

    printf("AES-KEYWRAP wrapping complete (one-shot)\n");
    return ret;
}

/* One-shot key unwrapping (no streaming API available) */
static int unwrap_file(const char* inFile, const char* outFile,
                       const byte* key, word32 keySz)
{
    byte*  wrapped = NULL;
    byte*  plaintext = NULL;
    word32 wrappedSz;
    word32 plaintextSz;
    word32 originalSz;
    int    ret;

    ret = read_file(inFile, &wrapped, &wrappedSz);
    if (ret != 0) return ret;

    if (wrappedSz < 4 + KEYWRAP_BLOCK * 2) {
        free(wrapped);
        printf("Error: File too small\n");
        return -1;
    }

    /* Extract original size from beginning */
    originalSz = ((word32)wrapped[0] << 24) |
                 ((word32)wrapped[1] << 16) |
                 ((word32)wrapped[2] << 8) |
                 ((word32)wrapped[3]);

    /* Unwrapped size is wrapped - 4 (size header) - 8 (integrity check) */
    plaintextSz = wrappedSz - 4 - KEYWRAP_BLOCK;
    plaintext = (byte*)malloc(plaintextSz);
    if (plaintext == NULL) {
        free(wrapped);
        return -1;
    }

    /* One-shot key unwrap */
    ret = wc_AesKeyUnWrap(key, keySz, wrapped + 4, wrappedSz - 4,
                          plaintext, plaintextSz, NULL);

    if (ret < 0) {
        free(wrapped);
        free(plaintext);
        printf("Error: Key unwrap failed (integrity check may have failed)\n");
        return ret;
    }

    /* Use original size to remove padding */
    ret = write_file(outFile, plaintext, originalSz);

    free(wrapped);
    free(plaintext);

    printf("AES-KEYWRAP unwrapping complete (one-shot, no streaming API "
           "available)\n");
    return ret;
}

int main(int argc, char** argv)
{
    byte key[AES_KEY_SIZE];
    int  ret;

    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        printf("Wraps input file, then unwraps to output file\n");
        printf("Note: Key wrap is designed for wrapping cryptographic keys\n");
        return 1;
    }

    wolfCrypt_Init();

    /* Use a fixed key for demonstration */
    memset(key, 0x0C, AES_KEY_SIZE);

    /* Wrap to temporary file */
    ret = wrap_file(argv[1], "temp_wrapped.bin", key, AES_KEY_SIZE);
    if (ret != 0) {
        printf("Wrapping failed: %d\n", ret);
        wolfCrypt_Cleanup();
        return 1;
    }

    /* Unwrap to output file */
    ret = unwrap_file("temp_wrapped.bin", argv[2], key, AES_KEY_SIZE);
    if (ret != 0) {
        printf("Unwrapping failed: %d\n", ret);
        wolfCrypt_Cleanup();
        return 1;
    }

    /* Clean up temp file */
    remove("temp_wrapped.bin");

    printf("Success! Unwrapped file written to %s\n", argv[2]);

    wolfCrypt_Cleanup();
    return 0;
}

#else

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    printf("AES-KEYWRAP not compiled in. Enable with HAVE_AES_KEYWRAP\n");
    return 0;
}

#endif
