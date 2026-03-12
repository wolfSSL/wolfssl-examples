/* aesgcm-minimal.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/random.h>

#include <stdio.h>
#include <string.h>

#define KEY_SZ   AES_256_KEY_SIZE
#define NONCE_SZ GCM_NONCE_MID_SZ
#define TAG_SZ   AES_BLOCK_SIZE

/* Optional setup path: seed wolfSSL RNG via RDSEED when available. */
#if defined(__x86_64__)
    #define RDSEED_ENABLED 1
#endif

#if defined(WC_RNG_SEED_CB) && defined(RDSEED_ENABLED)
#include <immintrin.h>

/* wc_RngSeed_Cb: feed wolfSSL DRBG seed using RDSEED. */
__attribute__((target("rdseed")))
static int RdseedSeedCb(OS_Seed* os, byte* seed, word32 sz)
{
    word32 i = 0;
    (void)os;

    while (i < sz) {
        unsigned long long v = 0;
        int ok = 0;
        int tries;
        word32 n;

        for (tries = 0; tries < 16; tries++) {
            if (_rdseed64_step(&v)) {
                ok = 1;
                break;
            }
        }
        if (!ok) {
            return RNG_FAILURE_E;
        }

        n = (sz - i < (word32)sizeof(v)) ? (sz - i) : (word32)sizeof(v);
        memcpy(seed + i, &v, n);
        i += n;
    }
    return 0;
}
#endif

static int GenerateKeyAndIv(byte* key, byte* iv)
{
    WC_RNG rng;
    int ret;

    /* Setup: initialize RNG and (optionally) override seed source. */
#if defined(WC_RNG_SEED_CB) && defined(RDSEED_ENABLED)
    wc_SetSeed_Cb(RdseedSeedCb);
    ret = wc_InitRng(&rng);
    wc_SetSeed_Cb(NULL);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0) {
        return ret;
    }

    ret = wc_RNG_GenerateBlock(&rng, key, KEY_SZ);
    if (ret == 0) {
        ret = wc_RNG_GenerateBlock(&rng, iv, NONCE_SZ);
    }
    wc_FreeRng(&rng);
    return ret;
}

static int Encrypt(const byte* key, const byte* iv, const byte* plaintext,
    word32 plaintextSz, byte* ciphertext, byte* tag, const byte* aad,
    word32 aadSz)
{
    int ret;
    Aes aes;

    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_AesGcmSetKey(&aes, key, KEY_SZ);
    }
    if (ret == 0) {
        ret = wc_AesGcmEncrypt(&aes, ciphertext, plaintext, plaintextSz, iv,
            NONCE_SZ, tag, TAG_SZ, aad, aadSz);
    }
    wc_AesFree(&aes);
    return ret;
}

static int Decrypt(const byte* key, const byte* iv, const byte* ciphertext,
    word32 ciphertextSz, byte* plaintext, const byte* tag, const byte* aad,
    word32 aadSz)
{
    int ret;
    Aes aes;

    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_AesGcmSetKey(&aes, key, KEY_SZ);
    }
    if (ret == 0) {
        ret = wc_AesGcmDecrypt(&aes, plaintext, ciphertext, ciphertextSz, iv,
            NONCE_SZ, tag, TAG_SZ, aad, aadSz);
    }
    wc_AesFree(&aes);
    return ret;
}

void print_hex(const char* label, const byte* data, word32 sz)
{
    printf("%s: ", label);
    for (word32 i = 0; i < sz; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

int main(void)
{
    /* Setup/example data and buffers. */
    byte key[KEY_SZ];
    const byte aad[] = "example-aad";
    const byte plaintext[] = "single block msg";
    byte decrypted[sizeof(plaintext)];
    byte iv[NONCE_SZ];
    byte ciphertext[sizeof(plaintext)];
    byte tag[TAG_SZ];
    int ret;

    ret = GenerateKeyAndIv(key, iv);
    if (ret != 0) {
        printf("Key/IV generation failed: %d\n", ret);
        return 1;
    }
    print_hex("Plaintext", plaintext, sizeof(plaintext));
    print_hex("  Key", key, sizeof(key));
    print_hex("  IV", iv, sizeof(iv));

    /* Encrypt with key + IV (+ optional AAD) to get tag. */
    ret = Encrypt(key, iv, plaintext, sizeof(plaintext), ciphertext, tag, aad,
        (word32)(sizeof(aad) - 1));
    if (ret != 0) {
        printf("Encryption failed: %d\n", ret);
        return 1;
    }
    print_hex("Ciphertext", ciphertext, sizeof(ciphertext));
    print_hex("  Auth Tag", tag, sizeof(tag));

    /* Decrypt with the same key/IV/AAD and received tag. */
    ret = Decrypt(key, iv, ciphertext, sizeof(ciphertext), decrypted, tag, aad,
        (word32)(sizeof(aad) - 1));
    if (ret != 0) {
        printf("Decryption failed: %d\n", ret);
        return 1;
    }

    if (memcmp(plaintext, decrypted, sizeof(plaintext)) != 0) {
        printf("Round-trip mismatch\n");
        return 1;
    }
    print_hex("Decrypted", decrypted, sizeof(decrypted));

    return 0;
}
