/* ecc-sign.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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


#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>

#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/random.h>

#define MAX_FIRMWARE_LEN (1024 * 1024)
static const int gFwLen = MAX_FIRMWARE_LEN;
static const int gFwChunkLen = 128;
static byte gFwBuf[MAX_FIRMWARE_LEN];
static ecc_key gMyKey;
static int gMyKeyInit = 0;
static const int gSignTimes = 10;

//#define ENABLE_BUF_PRINT
#define ECC_KEY_SIZE  48
#define ECC_KEY_CURVE ECC_SECP384R1


#ifdef ENABLE_BUF_PRINT
static void PrintBuffer(const byte* buffer, word32 length)
{
    #define LINE_LEN 16
    word32 i;
    char line[80];

    if (!buffer) {
        printf("\tNULL");
        return;
    }

    sprintf(line, "\t");

    for (i = 0; i < LINE_LEN; i++) {
        if (i < length)
            sprintf(line + 1 + i * 3,"%02x ", buffer[i]);
        else
            sprintf(line + 1 + i * 3, "   ");
    }

    sprintf(line + 1 + LINE_LEN * 3, "| ");

    for (i = 0; i < LINE_LEN; i++)
        if (i < length)
            sprintf(line + 3 + LINE_LEN * 3 + i,
                 "%c", 31 < buffer[i] && buffer[i] < 127 ? buffer[i] : '.');

    puts(line);

    if (length > LINE_LEN)
        PrintBuffer(buffer + LINE_LEN, length - LINE_LEN);
}
#endif

static int HashFirmware(byte* hashBuf)
{
    int ret;
    wc_Sha256 sha;
    int idx = 0, len = gFwLen, sz;

    ret = wc_InitSha256(&sha);
    if (ret != 0)
        return ret;

    /* loop through each chunk of firmware */
    while (len > 0) {
        /* determine hash update size */
        sz = len;
        if (sz > gFwChunkLen)
            sz = gFwChunkLen;

        /* update hash */
        ret = wc_Sha256Update(&sha, &gFwBuf[idx], (word32)sz);
        if (ret != 0)
            break;

        len -= sz;
        idx += sz;
    }

    if (ret == 0) {
        ret = wc_Sha256Final(&sha, hashBuf);
    }

    wc_Sha256Free(&sha);

    return ret;
}

static int SignFirmware(byte* hashBuf, word32 hashLen, byte* sigBuf, word32* sigLen)
{
    int ret = 0;
    WC_RNG rng;

    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;

    /* generate key for testing if one hasn't been created */
    if (gMyKeyInit == 0) {
        ret = wc_ecc_init(&gMyKey);
        if (ret == 0) {
            ret = wc_ecc_make_key_ex(&rng, ECC_KEY_SIZE, &gMyKey, ECC_KEY_CURVE);
            if (ret == 0) {
                gMyKeyInit = 1;
                printf("KeyGen Done\n");
            }
        }
    }

    /* sign hash */
    if (ret == 0) {
        ret = wc_ecc_sign_hash(hashBuf, hashLen, sigBuf, sigLen, &rng, &gMyKey);
        printf("Sign ret %d, sigLen %d\n", ret, *sigLen);
    }
    if (ret == 0) {
        int is_valid_sig = 0;
        ret = wc_ecc_verify_hash(sigBuf, *sigLen, hashBuf, hashLen,
            &is_valid_sig, &gMyKey);
        printf("Verify ret %d, is_valid_sig %d\n", ret, is_valid_sig);
    }

    wc_FreeRng(&rng);

    return ret;
}

int main(void)
{
    int ret;
    byte hashBuf[WC_SHA256_DIGEST_SIZE];
    word32 hashLen = WC_SHA256_DIGEST_SIZE;
    byte sigBuf[ECC_MAX_SIG_SIZE];
    word32 sigLen = ECC_MAX_SIG_SIZE;
    int i;

    /* init bogus firmware */
    for (i=0; i<gFwLen; i++) {
        gFwBuf[i] = (byte)i;
    }

    /* try performing signature a few times */
    for (i=0; i < gSignTimes; i++) {
        memset(hashBuf, 0, hashLen);
        ret = HashFirmware(hashBuf);
        if (ret == 0) {
            sigLen = ECC_MAX_SIG_SIZE;
            memset(sigBuf, 0, sigLen);
            ret = SignFirmware(hashBuf, hashLen, sigBuf, &sigLen);
        }

        printf("Firmware Signature %d: Ret %d, HashLen %d, SigLen %d\n", i, ret, hashLen, sigLen);

    #ifdef ENABLE_BUF_PRINT
        PrintBuffer(hashBuf, hashLen);
        printf("\n");
        PrintBuffer(sigBuf, sigLen);
    #endif
    }

    return ret;
}
