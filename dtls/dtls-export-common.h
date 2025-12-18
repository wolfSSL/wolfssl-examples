/* dtls-export-common.h
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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
 *
 *=============================================================================
 *
 * Common header for DTLS session export/import examples.
 * Provides AES encryption/decryption for session data protection.
 */

#ifndef DTLS_EXPORT_COMMON_H
#define DTLS_EXPORT_COMMON_H

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/random.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Default file for session export/import */
#define DEFAULT_SESSION_FILE "dtls_session.bin"

/* AES-256-CBC key (32 bytes) - In production, use a secure key management system */
static const unsigned char AES_KEY[32] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
};

/* AES block size */
#define AES_BLOCK_SZ 16

/* File format:
 * [4 bytes: original data length]
 * [16 bytes: IV]
 * [N bytes: encrypted data (padded to AES_BLOCK_SZ)]
 */

/*
 * Encrypt session data and save to file
 * Returns 0 on success, negative on error
 */
static int SaveEncryptedSession(const char* filename,
                                const unsigned char* sessionData,
                                unsigned int sessionSz)
{
    FILE* fp = NULL;
    Aes aes;
    WC_RNG rng;
    unsigned char iv[AES_BLOCK_SZ];
    unsigned char* paddedData = NULL;
    unsigned char* encryptedData = NULL;
    unsigned int paddedSz;
    unsigned int padding;
    int ret = 0;
    int aesInit = 0;
    int rngInit = 0;

    /* Calculate padded size (must be multiple of AES_BLOCK_SZ) */
    padding = AES_BLOCK_SZ - (sessionSz % AES_BLOCK_SZ);
    paddedSz = sessionSz + padding;

    /* Initialize RNG for IV generation */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Error: wc_InitRng failed: %d\n", ret);
        return -1;
    }
    rngInit = 1;

    /* Generate random IV */
    ret = wc_RNG_GenerateBlock(&rng, iv, AES_BLOCK_SZ);
    if (ret != 0) {
        printf("Error: Failed to generate IV: %d\n", ret);
        goto cleanup;
    }

    /* Allocate buffers */
    paddedData = (unsigned char*)malloc(paddedSz);
    encryptedData = (unsigned char*)malloc(paddedSz);
    if (paddedData == NULL || encryptedData == NULL) {
        printf("Error: Memory allocation failed\n");
        ret = -1;
        goto cleanup;
    }

    /* Copy data and add PKCS#7 padding */
    memcpy(paddedData, sessionData, sessionSz);
    memset(paddedData + sessionSz, (int)padding, padding);

    /* Initialize AES */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        printf("Error: wc_AesInit failed: %d\n", ret);
        goto cleanup;
    }
    aesInit = 1;

    /* Set AES key for encryption */
    ret = wc_AesSetKey(&aes, AES_KEY, sizeof(AES_KEY), iv, AES_ENCRYPTION);
    if (ret != 0) {
        printf("Error: wc_AesSetKey failed: %d\n", ret);
        goto cleanup;
    }

    /* Encrypt the data */
    ret = wc_AesCbcEncrypt(&aes, encryptedData, paddedData, paddedSz);
    if (ret != 0) {
        printf("Error: wc_AesCbcEncrypt failed: %d\n", ret);
        goto cleanup;
    }

    /* Write to file */
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("Error: Cannot open file %s for writing\n", filename);
        ret = -1;
        goto cleanup;
    }

    /* Write original size (4 bytes, big-endian) */
    {
        unsigned char sizeBuf[4];
        sizeBuf[0] = (sessionSz >> 24) & 0xFF;
        sizeBuf[1] = (sessionSz >> 16) & 0xFF;
        sizeBuf[2] = (sessionSz >> 8) & 0xFF;
        sizeBuf[3] = sessionSz & 0xFF;
        if (fwrite(sizeBuf, 1, 4, fp) != 4) {
            printf("Error: Failed to write size to file\n");
            ret = -1;
            goto cleanup;
        }
    }

    /* Write IV */
    if (fwrite(iv, 1, AES_BLOCK_SZ, fp) != AES_BLOCK_SZ) {
        printf("Error: Failed to write IV to file\n");
        ret = -1;
        goto cleanup;
    }

    /* Write encrypted data */
    if (fwrite(encryptedData, 1, paddedSz, fp) != paddedSz) {
        printf("Error: Failed to write encrypted data to file\n");
        ret = -1;
        goto cleanup;
    }

    printf("Session saved to %s (%u bytes encrypted)\n", filename, paddedSz);
    ret = 0;

cleanup:
    if (fp != NULL) fclose(fp);
    if (paddedData != NULL) free(paddedData);
    if (encryptedData != NULL) free(encryptedData);
    if (aesInit) wc_AesFree(&aes);
    if (rngInit) wc_FreeRng(&rng);

    return ret;
}

/*
 * Load and decrypt session data from file
 * Returns allocated buffer with session data (caller must free), or NULL on error
 * sessionSz is set to the size of the returned data
 */
static unsigned char* LoadEncryptedSession(const char* filename,
                                           unsigned int* sessionSz)
{
    FILE* fp = NULL;
    Aes aes;
    unsigned char iv[AES_BLOCK_SZ];
    unsigned char sizeBuf[4];
    unsigned char* encryptedData = NULL;
    unsigned char* decryptedData = NULL;
    unsigned int originalSz;
    unsigned int encryptedSz;
    long fileSize;
    int ret = 0;
    int aesInit = 0;

    /* Open file */
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Error: Cannot open file %s for reading\n", filename);
        return NULL;
    }

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fileSize < (4 + AES_BLOCK_SZ + AES_BLOCK_SZ)) {
        printf("Error: File too small to contain valid session data\n");
        fclose(fp);
        return NULL;
    }

    /* Read original size */
    if (fread(sizeBuf, 1, 4, fp) != 4) {
        printf("Error: Failed to read size from file\n");
        fclose(fp);
        return NULL;
    }
    originalSz = ((unsigned int)sizeBuf[0] << 24) |
                 ((unsigned int)sizeBuf[1] << 16) |
                 ((unsigned int)sizeBuf[2] << 8) |
                 (unsigned int)sizeBuf[3];

    /* Read IV */
    if (fread(iv, 1, AES_BLOCK_SZ, fp) != AES_BLOCK_SZ) {
        printf("Error: Failed to read IV from file\n");
        fclose(fp);
        return NULL;
    }

    /* Calculate encrypted data size */
    encryptedSz = (unsigned int)(fileSize - 4 - AES_BLOCK_SZ);

    /* Allocate buffers */
    encryptedData = (unsigned char*)malloc(encryptedSz);
    decryptedData = (unsigned char*)malloc(encryptedSz);
    if (encryptedData == NULL || decryptedData == NULL) {
        printf("Error: Memory allocation failed\n");
        goto cleanup;
    }

    /* Read encrypted data */
    if (fread(encryptedData, 1, encryptedSz, fp) != encryptedSz) {
        printf("Error: Failed to read encrypted data from file\n");
        goto cleanup;
    }
    fclose(fp);
    fp = NULL;

    /* Initialize AES */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        printf("Error: wc_AesInit failed: %d\n", ret);
        goto cleanup;
    }
    aesInit = 1;

    /* Set AES key for decryption */
    ret = wc_AesSetKey(&aes, AES_KEY, sizeof(AES_KEY), iv, AES_DECRYPTION);
    if (ret != 0) {
        printf("Error: wc_AesSetKey failed: %d\n", ret);
        goto cleanup;
    }

    /* Decrypt the data */
    ret = wc_AesCbcDecrypt(&aes, decryptedData, encryptedData, encryptedSz);
    if (ret != 0) {
        printf("Error: wc_AesCbcDecrypt failed: %d\n", ret);
        goto cleanup;
    }

    /* Verify padding and original size */
    if (originalSz > encryptedSz) {
        printf("Error: Invalid original size in file\n");
        goto cleanup;
    }

    *sessionSz = originalSz;
    printf("Session loaded from %s (%u bytes)\n", filename, originalSz);

    /* Clean up and return decrypted data */
    if (encryptedData != NULL) free(encryptedData);
    if (aesInit) wc_AesFree(&aes);

    return decryptedData;

cleanup:
    if (fp != NULL) fclose(fp);
    if (encryptedData != NULL) free(encryptedData);
    if (decryptedData != NULL) free(decryptedData);
    if (aesInit) wc_AesFree(&aes);

    return NULL;
}

#endif /* DTLS_EXPORT_COMMON_H */
