/* aes-gmac.c
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

/* AES-GMAC Example
 * This example demonstrates:
 * - One-shot authentication using wc_GmacUpdate() to generate auth tag
 * - One-shot verification using wc_GmacVerify()
 * Note: GMAC is authentication-only (no encryption), no streaming API
 *       It authenticates data without encrypting it
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
#define GMAC_IV_SIZE    12  /* Recommended IV size */
#define GMAC_TAG_SIZE   16  /* Authentication tag size */

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

/* One-shot authentication - generate auth tag for file */
static int authenticate_file(const char* inFile, const char* outFile,
                             const byte* key, word32 keySz)
{
    Gmac   gmac;
    WC_RNG rng;
    byte   iv[GMAC_IV_SIZE];
    byte   authTag[GMAC_TAG_SIZE];
    byte*  data = NULL;
    byte*  output = NULL;
    word32 dataSz;
    word32 outputSz;
    int    ret;

    ret = read_file(inFile, &data, &dataSz);
    if (ret != 0) return ret;

    /* Output: IV + authTag + original data (unencrypted) */
    outputSz = GMAC_IV_SIZE + GMAC_TAG_SIZE + dataSz;
    output = (byte*)malloc(outputSz);
    if (output == NULL) {
        free(data);
        return -1;
    }

    /* Generate random IV */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        free(data);
        free(output);
        return ret;
    }

    ret = wc_RNG_GenerateBlock(&rng, iv, GMAC_IV_SIZE);
    wc_FreeRng(&rng);
    if (ret != 0) {
        free(data);
        free(output);
        return ret;
    }

    /* Initialize GMAC */
    ret = wc_AesInit(&gmac.aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(data);
        free(output);
        return ret;
    }

    ret = wc_GmacSetKey(&gmac, key, keySz);
    if (ret != 0) {
        wc_AesFree(&gmac.aes);
        free(data);
        free(output);
        return ret;
    }

    /* One-shot generate authentication tag */
    ret = wc_GmacUpdate(&gmac, iv, GMAC_IV_SIZE, data, dataSz,
                        authTag, GMAC_TAG_SIZE);
    wc_AesFree(&gmac.aes);

    if (ret != 0) {
        free(data);
        free(output);
        return ret;
    }

    /* Store IV + authTag + data */
    memcpy(output, iv, GMAC_IV_SIZE);
    memcpy(output + GMAC_IV_SIZE, authTag, GMAC_TAG_SIZE);
    memcpy(output + GMAC_IV_SIZE + GMAC_TAG_SIZE, data, dataSz);

    ret = write_file(outFile, output, outputSz);

    free(data);
    free(output);

    printf("AES-GMAC authentication complete (one-shot)\n");
    printf("  Generated %d-byte authentication tag\n", GMAC_TAG_SIZE);
    return ret;
}

/* One-shot verification - verify auth tag and extract data */
static int verify_file(const char* inFile, const char* outFile,
                       const byte* key, word32 keySz)
{
    byte   iv[GMAC_IV_SIZE];
    byte   authTag[GMAC_TAG_SIZE];
    byte*  input = NULL;
    byte*  data = NULL;
    word32 inputSz;
    word32 dataSz;
    int    ret;

    ret = read_file(inFile, &input, &inputSz);
    if (ret != 0) return ret;

    if (inputSz < GMAC_IV_SIZE + GMAC_TAG_SIZE) {
        free(input);
        printf("Error: File too small\n");
        return -1;
    }

    /* Extract IV and authTag from beginning */
    memcpy(iv, input, GMAC_IV_SIZE);
    memcpy(authTag, input + GMAC_IV_SIZE, GMAC_TAG_SIZE);

    dataSz = inputSz - GMAC_IV_SIZE - GMAC_TAG_SIZE;
    data = input + GMAC_IV_SIZE + GMAC_TAG_SIZE;

    /* One-shot verify authentication tag */
    ret = wc_GmacVerify(key, keySz, iv, GMAC_IV_SIZE,
                        data, dataSz, authTag, GMAC_TAG_SIZE);

    if (ret != 0) {
        free(input);
        if (ret == AES_GCM_AUTH_E) {
            printf("Error: Authentication failed! Data may have been "
                   "tampered with.\n");
        }
        return ret;
    }

    /* Write verified data to output */
    ret = write_file(outFile, data, dataSz);

    free(input);

    printf("AES-GMAC verification complete (one-shot)\n");
    printf("  Authentication tag verified successfully\n");
    return ret;
}

int main(int argc, char** argv)
{
    byte key[AES_KEY_SIZE];
    int  ret;

    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        printf("Authenticates input file (one-shot), then verifies to output "
               "file\n");
        printf("Note: GMAC provides authentication only, no encryption\n");
        return 1;
    }

    wolfCrypt_Init();

    /* Use a fixed key for demonstration */
    memset(key, 0x0A, AES_KEY_SIZE);

    /* Authenticate to temporary file */
    ret = authenticate_file(argv[1], "temp_authenticated.bin", key, AES_KEY_SIZE);
    if (ret != 0) {
        printf("Authentication failed: %d\n", ret);
        wolfCrypt_Cleanup();
        return 1;
    }

    /* Verify and extract to output file */
    ret = verify_file("temp_authenticated.bin", argv[2], key, AES_KEY_SIZE);
    if (ret != 0) {
        printf("Verification failed: %d\n", ret);
        wolfCrypt_Cleanup();
        return 1;
    }

    /* Clean up temp file */
    remove("temp_authenticated.bin");

    printf("Success! Verified file written to %s\n", argv[2]);

    wolfCrypt_Cleanup();
    return 0;
}

#else

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    printf("AES-GMAC not compiled in. Enable with HAVE_AESGCM\n");
    return 0;
}

#endif
