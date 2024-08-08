/* aesgcm-oneshot.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/aes.h>

#if !defined(NO_AES) && defined(HAVE_AESGCM)
int aesgcm_enc_dec()
{
    Aes           aesEnc;
    Aes           aesDec;
    unsigned char key[AES_256_KEY_SIZE];
    int           ret = 0;
    unsigned char data[33];
    unsigned char enc[33];
    unsigned char dec[33];
    unsigned char iv[GCM_NONCE_MID_SZ];
    unsigned char authTag[AES_BLOCK_SIZE];
    size_t        i;

    memset(key, 9, sizeof(key));
    memset(data, 9, sizeof(data));
    memset(iv, 9, sizeof(iv));

    fprintf(stderr, "Encrypt with AES128-GCM\n");
    /* Initialize AES encryption object. */
    ret = wc_AesInit(&aesEnc, NULL, INVALID_DEVID);
    if (ret == 0) {
        /* Set GCM key into AES encryption object. */
        ret = wc_AesGcmSetKey(&aesEnc, key, AES_128_KEY_SIZE);
        if (ret != 0)
            fprintf(stderr, "Set Key failed: %d\n", ret);
    }
    if (ret == 0) {
        printf(" Plaintext: ");
        for (i = 0; i < sizeof(data); i++)
            printf("%02x", data[i]);
        printf("\n");

        /* Encrypt data with AES encryption object and get ciphertext and
         * authentication tag. No additional authentication data. */
        ret = wc_AesGcmEncrypt(&aesEnc, enc, data, sizeof(data), iv, sizeof(iv),
                               authTag, sizeof(authTag), NULL, 0);
        if (ret != 0)
            fprintf(stderr, "Encrypt failed: %d\n", ret);
    }
    if (ret == 0) {
        printf("Ciphertext: ");
        for (i = 0; i < sizeof(data); i++)
            printf("%02x", enc[i]);
        printf("\n");
        printf("  Auth Tag: ");
        for (i = 0; i < sizeof(authTag); i++)
            printf("%02x", authTag[i]);
        printf("\n");
    }

    if (ret == 0) {
        fprintf(stderr, "Decrypt with AES128-GCM\n");
        /* Initialize AES decryption object. */
        ret = wc_AesInit(&aesDec, NULL, INVALID_DEVID);
    }
    if (ret == 0) {
        /* Set GCM key into AES decryption object. */
        ret = wc_AesGcmSetKey(&aesDec, key, AES_128_KEY_SIZE);
        if (ret != 0)
            fprintf(stderr, "Set Key failed: %d\n", ret);
    }
    if (ret == 0) {
        /* Check authentication tag with ciphertext and decrypt ciphertext with
         * AES decryption object and get decrypted data. No additional
         * authentication data. */
        ret = wc_AesGcmDecrypt(&aesDec, dec, enc, sizeof(enc), iv, sizeof(iv),
                               authTag, sizeof(authTag), NULL, 0);
        if (ret == AES_GCM_AUTH_E)
            fprintf(stderr, "Authentication failed: %d\n", ret);
        else if (ret != 0)
            fprintf(stderr, "Decrypt failed: %d\n", ret);
    }
    if (ret == 0) {
        printf(" Decrypted: ");
        for (i = 0; i < sizeof(data); i++)
            printf("%02x", dec[i]);
        printf("\n");
    }

    return ret;
}
#endif

int main(int argc, char* argv[])
{
    int ret = 0;

    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        return 1;
    }

#if !defined(NO_AES) && defined(HAVE_AESGCM)

#if defined(DEBUG_WOLFSSL)
    wolfSSL_Debugging_ON();
#endif
    wolfCrypt_Init();

    if (aesgcm_enc_dec() != 0)
        ret = 1;

    wolfCrypt_Cleanup();

#else

    printf("AES-GCM not built into wolfSSL\n");

#endif

    return ret;
}

