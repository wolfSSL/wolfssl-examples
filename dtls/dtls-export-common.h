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
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/memory.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/pwdbased.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef XPRINTF
    #define XPRINTF printf
#endif
#ifndef XFPRINTF
    #define XFPRINTF fprintf
#endif

/* Default file for session export/import */
#define DEFAULT_CLIENT_SESSION_FILE "dtls_client_session.bin"
#define DEFAULT_SERVER_SESSION_FILE "dtls_server_session.bin"

/* Passphrase source for session-file encryption; falls back to a demo
 * value if unset. Set this for anything beyond local demos. */
#define SESSION_PASSPHRASE_ENV "DTLS_SESSION_PASSPHRASE"
#define DEMO_SESSION_PASSPHRASE "dtls-example-demo-passphrase-change-me"

/* AES block size / PBKDF2 salt size / AES-256 key size */
#define GCM_IV_SZ 12
#define GCM_TAG_SZ 16
#define SESSION_SALT_SZ 16
#define SESSION_KEY_SZ 32
#define SESSION_PBKDF2_ITERATIONS 100000

/* File format:
 * [4 bytes: original data length]
 * [16 bytes: PBKDF2 salt]
 * [12 bytes: IV]
 * [16 bytes: auth tag]
 * [N bytes: encrypted data]
 */

static const char* GetSessionPassphrase(void)
{
    const char* passphrase = getenv(SESSION_PASSPHRASE_ENV);
    if (passphrase == NULL || passphrase[0] == '\0') {
        XFPRINTF(stderr,
                "Warning: %s not set, using demo passphrase. "
                "Set %s to a real secret for anything beyond local demos.\n",
                SESSION_PASSPHRASE_ENV, SESSION_PASSPHRASE_ENV);
        passphrase = DEMO_SESSION_PASSPHRASE;
    }
    return passphrase;
}

/*
 * Derive an AES-256 key from a passphrase and salt using PBKDF2-HMAC-SHA256.
 * Returns 0 on success, negative on error.
 */
static int DeriveSessionKey(const char* passphrase,
                            const unsigned char* salt, unsigned int saltSz,
                            unsigned char* key, unsigned int keySz)
{
    return wc_PBKDF2(key, (const byte*)passphrase, (int)strlen(passphrase),
                     salt, (int)saltSz, SESSION_PBKDF2_ITERATIONS,
                     (int)keySz, WC_SHA256);
}

/*
 * Encrypt session data and save to file
 * Returns 0 on success, negative on error
 */
int SaveEncryptedSession(const char* filename,
                         const unsigned char* sessionData,
                         unsigned int sessionSz)
{
    FILE* fp = NULL;
    Aes aes;
    WC_RNG rng;
    unsigned char iv[GCM_IV_SZ];
    unsigned char tag[GCM_TAG_SZ];
    unsigned char salt[SESSION_SALT_SZ];
    unsigned char key[SESSION_KEY_SZ];
    unsigned char* encryptedData = NULL;
    unsigned char sizeBuf[4];
    int ret = 0;
    int aesInit = 0;
    int rngInit = 0;

    /* Initialize RNG for IV/salt generation */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        XPRINTF("Error: wc_InitRng failed: %d\n", ret);
        return -1;
    }
    rngInit = 1;

    /* Generate random IV */
    ret = wc_RNG_GenerateBlock(&rng, iv, GCM_IV_SZ);
    if (ret != 0) {
        XPRINTF("Error: Failed to generate IV: %d\n", ret);
        goto cleanup;
    }

    /* Derive AES key from passphrase */
    ret = wc_RNG_GenerateBlock(&rng, salt, SESSION_SALT_SZ);
    if (ret != 0) {
        XPRINTF("Error: Failed to generate salt: %d\n", ret);
        goto cleanup;
    }

    ret = DeriveSessionKey(GetSessionPassphrase(), salt, SESSION_SALT_SZ,
                           key, SESSION_KEY_SZ);
    if (ret != 0) {
        XPRINTF("Error: Failed to derive session key: %d\n", ret);
        goto cleanup;
    }

    /* Allocate buffer for encrypted data */
    encryptedData = (unsigned char*)XMALLOC(sessionSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (encryptedData == NULL && sessionSz > 0) {
        XPRINTF("Error: Memory allocation failed\n");
        ret = -1;
        goto cleanup;
    }

    /* Initialize AES */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        XPRINTF("Error: wc_AesInit failed: %d\n", ret);
        goto cleanup;
    }
    aesInit = 1;

    /* Set AES key for GCM */
    ret = wc_AesGcmSetKey(&aes, key, sizeof(key));
    if (ret != 0) {
        XPRINTF("Error: wc_AesGcmSetKey failed: %d\n", ret);
        goto cleanup;
    }

    /* Prepare AAD: original data length (4 bytes) */
    sizeBuf[0] = (sessionSz >> 24) & 0xFF;
    sizeBuf[1] = (sessionSz >> 16) & 0xFF;
    sizeBuf[2] = (sessionSz >> 8) & 0xFF;
    sizeBuf[3] = sessionSz & 0xFF;

    /* Encrypt the data with GCM */
    ret = wc_AesGcmEncrypt(&aes, encryptedData, sessionData, sessionSz,
                           iv, GCM_IV_SZ, tag, GCM_TAG_SZ, sizeBuf, 4);
    if (ret != 0) {
        XPRINTF("Error: wc_AesGcmEncrypt failed: %d\n", ret);
        goto cleanup;
    }

    /* Write to file */
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        XPRINTF("Error: Cannot open file %s for writing\n", filename);
        ret = -1;
        goto cleanup;
    }

    /* Write size */
    if (fwrite(sizeBuf, 1, 4, fp) != 4) {
        XPRINTF("Error: Failed to write size to file\n");
        ret = -1;
        goto cleanup;
    }

    /* Write salt */
    if (fwrite(salt, 1, SESSION_SALT_SZ, fp) != SESSION_SALT_SZ) {
        XPRINTF("Error: Failed to write salt to file\n");
        ret = -1;
        goto cleanup;
    }

    /* Write IV */
    if (fwrite(iv, 1, GCM_IV_SZ, fp) != GCM_IV_SZ) {
        XPRINTF("Error: Failed to write IV to file\n");
        ret = -1;
        goto cleanup;
    }
    
    /* Write tag */
    if (fwrite(tag, 1, GCM_TAG_SZ, fp) != GCM_TAG_SZ) {
        XPRINTF("Error: Failed to write tag to file\n");
        ret = -1;
        goto cleanup;
    }

    /* Write encrypted data */
    if (sessionSz > 0 && fwrite(encryptedData, 1, sessionSz, fp) != sessionSz) {
        XPRINTF("Error: Failed to write encrypted data to file\n");
        ret = -1;
        goto cleanup;
    }

    XPRINTF("Session saved to %s (%u bytes encrypted)\n", filename, sessionSz);
    ret = 0;

cleanup:
    if (fp != NULL) fclose(fp);
    if (encryptedData != NULL) {
        wc_ForceZero(encryptedData, sessionSz);
        XFREE(encryptedData, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
    if (aesInit) wc_AesFree(&aes);
    if (rngInit) wc_FreeRng(&rng);
    wc_ForceZero(key, sizeof(key));

    return ret;
}

/*
 * Load and decrypt session data from file
 * Returns allocated session data (caller must free), or NULL on error
 * sessionSz is set to the size of the returned data
 * bufSz, if non-NULL, is set to the full allocated size of the returned
 * buffer (>= *sessionSz), for callers that need to wc_ForceZero() the
 * entire allocation before freeing it
 */
unsigned char* LoadEncryptedSession(const char* filename,
                                    unsigned int* sessionSz,
                                    unsigned int* bufSz)
{
    FILE* fp = NULL;
    Aes aes;
    unsigned char iv[GCM_IV_SZ];
    unsigned char tag[GCM_TAG_SZ];
    unsigned char salt[SESSION_SALT_SZ];
    unsigned char key[SESSION_KEY_SZ];
    unsigned char sizeBuf[4];
    unsigned char* encryptedData = NULL;
    unsigned char* decryptedData = NULL;
    unsigned int originalSz;
    long fileSize;
    int ret = 0;
    int aesInit = 0;

    /* Open file */
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        XPRINTF("Error: Cannot open file %s for reading\n", filename);
        return NULL;
    }

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fileSize < (4 + SESSION_SALT_SZ + GCM_IV_SZ + GCM_TAG_SZ)) {
        XPRINTF("Error: File too small to contain valid session data\n");
        fclose(fp);
        return NULL;
    }

    /* Read size */
    if (fread(sizeBuf, 1, 4, fp) != 4) {
        XPRINTF("Error: Failed to read size from file\n");
        fclose(fp);
        return NULL;
    }
    originalSz = ((unsigned int)sizeBuf[0] << 24) |
                 ((unsigned int)sizeBuf[1] << 16) |
                 ((unsigned int)sizeBuf[2] << 8) |
                 (unsigned int)sizeBuf[3];

    /* Read salt */
    if (fread(salt, 1, SESSION_SALT_SZ, fp) != SESSION_SALT_SZ) {
        XPRINTF("Error: Failed to read salt from file\n");
        fclose(fp);
        return NULL;
    }

    /* Read IV */
    if (fread(iv, 1, GCM_IV_SZ, fp) != GCM_IV_SZ) {
        XPRINTF("Error: Failed to read IV from file\n");
        fclose(fp);
        return NULL;
    }
    
    /* Read tag */
    if (fread(tag, 1, GCM_TAG_SZ, fp) != GCM_TAG_SZ) {
        XPRINTF("Error: Failed to read tag from file\n");
        fclose(fp);
        return NULL;
    }

    /* Derive AES key from passphrase */
    ret = DeriveSessionKey(GetSessionPassphrase(), salt, SESSION_SALT_SZ,
                           key, SESSION_KEY_SZ);
    if (ret != 0) {
        XPRINTF("Error: Failed to derive session key: %d\n", ret);
        goto cleanup;
    }

    if ((fileSize - 4 - SESSION_SALT_SZ - GCM_IV_SZ - GCM_TAG_SZ) !=
            (long)originalSz) {
        XPRINTF("Error: File size does not match expected encrypted data size\n");
        goto cleanup;
    }

    /* Allocate buffers. Always allocate at least 1 byte so a legitimate
     * zero-length session is returned as a non-NULL pointer, distinct
     * from a NULL/failure return. */
    encryptedData = (unsigned char*)XMALLOC(originalSz > 0 ? originalSz : 1,
                                            NULL, DYNAMIC_TYPE_TMP_BUFFER);
    decryptedData = (unsigned char*)XMALLOC(originalSz > 0 ? originalSz : 1,
                                            NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (encryptedData == NULL || decryptedData == NULL) {
        XPRINTF("Error: Memory allocation failed\n");
        goto cleanup;
    }

    if (originalSz > 0) {
        /* Read encrypted data */
        if (fread(encryptedData, 1, originalSz, fp) != originalSz) {
            XPRINTF("Error: Failed to read encrypted data from file\n");
            goto cleanup;
        }
    }
    fclose(fp);
    fp = NULL;

    /* Initialize AES */
    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        XPRINTF("Error: wc_AesInit failed: %d\n", ret);
        goto cleanup;
    }
    aesInit = 1;

    /* Set AES key */
    ret = wc_AesGcmSetKey(&aes, key, sizeof(key));
    if (ret != 0) {
        XPRINTF("Error: wc_AesGcmSetKey failed: %d\n", ret);
        goto cleanup;
    }

    /* Decrypt and authenticate the data */
    ret = wc_AesGcmDecrypt(&aes, decryptedData, encryptedData, originalSz,
                           iv, GCM_IV_SZ, tag, GCM_TAG_SZ, sizeBuf, 4);
    if (ret != 0) {
        XPRINTF("Error: wc_AesGcmDecrypt failed (authentication error): %d\n", ret);
        goto cleanup;
    }

    *sessionSz = originalSz;
    if (bufSz != NULL) {
        *bufSz = originalSz > 0 ? originalSz : 1;
    }
    XPRINTF("Session loaded from %s (%u bytes)\n", filename, originalSz);

    /* Clean up and return decrypted data */
    if (encryptedData != NULL) {
        wc_ForceZero(encryptedData, originalSz);
        XFREE(encryptedData, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
    if (aesInit) wc_AesFree(&aes);
    wc_ForceZero(key, sizeof(key));

    return decryptedData;

cleanup:
    if (fp != NULL) fclose(fp);
    if (encryptedData != NULL) {
        wc_ForceZero(encryptedData, originalSz);
        XFREE(encryptedData, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
    if (decryptedData != NULL) {
        wc_ForceZero(decryptedData, originalSz);
        XFREE(decryptedData, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
    if (aesInit) wc_AesFree(&aes);
    wc_ForceZero(key, sizeof(key));

    return NULL;
}

#endif /* DTLS_EXPORT_COMMON_H */
