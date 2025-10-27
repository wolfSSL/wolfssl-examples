/* ml_kem.c
 *
 * Copyright (C) 2025 wolfSSL Inc.
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
 */

/* Credit goes to Professor Bill Buchanan of Edinburgh Napier University for
 * creating this example and contributing it back to wolfSSL. */

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/mlkem.h>
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#include <wolfssl/wolfcrypt/random.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

char* to_hex_string(const unsigned char* array, size_t length)
{
    char* outstr = malloc(2 * length + 1);
    if (!outstr) return outstr;

    char* p = outstr;
    for (size_t i = 0; i < length; ++i) {
        p += sprintf(p, "%02hhx", array[i]);
    }

    return outstr;
}


int main(int argc, char** argv)
{
    MlKemKey AliceKey;
    MlKemKey BobKey;
    int ret = 0;
    WC_RNG rng;
    byte alice_pub[WC_ML_KEM_512_PUBLIC_KEY_SIZE];
    byte bob_ct[WC_ML_KEM_512_CIPHER_TEXT_SIZE];
    byte alice_ss[WC_ML_KEM_SS_SZ];
    byte bob_ss[WC_ML_KEM_SS_SZ];

    printf("Alice creates an ML-KEM-512 key pair\n\n");

    ret = wc_InitRng(&rng);

    if (ret == 0)
        ret = wc_MlKemKey_Init(&AliceKey, WC_ML_KEM_512, 0, INVALID_DEVID);

    if (ret == 0)
        ret = wc_MlKemKey_Init(&BobKey, WC_ML_KEM_512, 0, INVALID_DEVID);

    if (ret == 0)
        ret = wc_MlKemKey_MakeKey(&AliceKey, &rng);

    if (ret == 0)
        ret = wc_MlKemKey_EncodePublicKey(&AliceKey, alice_pub, WC_ML_KEM_512_PUBLIC_KEY_SIZE);

    if (ret == 0)
        printf("Bob receives public key (size=%d): %s\n\n", WC_ML_KEM_512_PUBLIC_KEY_SIZE, to_hex_string(alice_pub, WC_ML_KEM_512_PUBLIC_KEY_SIZE));

    if (ret == 0)
        ret = wc_MlKemKey_DecodePublicKey(&BobKey, alice_pub, WC_ML_KEM_512_PUBLIC_KEY_SIZE);

    if (ret == 0)
        ret = wc_MlKemKey_Encapsulate(&BobKey, bob_ct, bob_ss, &rng);

    if (ret == 0)
        printf("Bob Encapsulates secret (Size=%d): %s\n\n", WC_ML_KEM_512_CIPHER_TEXT_SIZE, to_hex_string(bob_ct, WC_ML_KEM_512_CIPHER_TEXT_SIZE));

    if (ret == 0)
        printf("Alice receives the ciphertext\n");

    if (ret == 0)
        ret = wc_MlKemKey_Decapsulate(&AliceKey, alice_ss, bob_ct, WC_ML_KEM_512_CIPHER_TEXT_SIZE);

    if (ret == 0) {
        printf("Alice's Shared Secret: %s\n\n", to_hex_string(alice_ss, WC_ML_KEM_SS_SZ));
        printf("  Bob's Shared Secret: %s\n\n", to_hex_string(bob_ss, WC_ML_KEM_SS_SZ));
    } else {
        printf("An error occurred\n");
    }

    wc_MlKemKey_Free(&AliceKey);
    wc_MlKemKey_Free(&BobKey);
    wc_FreeRng(&rng);
    return ret;
}

