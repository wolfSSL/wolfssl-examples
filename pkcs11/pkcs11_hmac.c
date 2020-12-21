/* pkcs11_hmac.c
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
#include <wolfssl/wolfcrypt/wc_pkcs11.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#if !defined(NO_HMAC) && !defined(NO_SHA256)
int hmac(int devId, Pkcs11Token* token)
{
    Hmac          hmac;
    unsigned char key[SHA256_DIGEST_SIZE];
    int           ret = 0;
    unsigned char data[57];
    unsigned char mac[SHA256_DIGEST_SIZE];

    memset(key, 9, sizeof(key));
    memset(data, 9, sizeof(data));

    fprintf(stderr, "HMAC with SHA-256\n");
    wc_Pkcs11Token_Open(token, 1);
    ret = wc_HmacInit(&hmac, NULL, devId);
    if (ret == 0) {
        ret = wc_HmacSetKey(&hmac, WC_SHA256, key, sizeof(key));
        if (ret != 0)
            fprintf(stderr, "Set Key failed: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_HmacUpdate(&hmac, data, sizeof(data));
        if (ret != 0)
            fprintf(stderr, "HMAC Update failed: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_HmacFinal(&hmac, mac);
        if (ret != 0)
            fprintf(stderr, "HMAC Final failed: %d\n", ret);
    }
    wc_Pkcs11Token_Close(token);

    return ret;
}
#endif

int main(int argc, char* argv[])
{
    int ret;
    const char* library;
    const char* slot;
    const char* tokenName;
    const char* userPin;
    Pkcs11Dev dev;
    Pkcs11Token token;
    int slotId;
    int devId = 1;

    if (argc != 4 && argc != 5) {
        fprintf(stderr,
               "Usage: pkcs11_aescbc <libname> <slot> <tokenname> [userpin]\n");
        return 1;
    }

    library = argv[1];
    slot = argv[2];
    tokenName = argv[3];
    userPin = (argc == 4) ? NULL : argv[4];
    slotId = atoi(slot);

#if defined(DEBUG_WOLFSSL)
    wolfSSL_Debugging_ON();
#endif
    wolfCrypt_Init();

    ret = wc_Pkcs11_Initialize(&dev, library, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize PKCS#11 library\n");
        ret = 2;
    }
    if (ret == 0) {
        ret = wc_Pkcs11Token_Init(&token, &dev, slotId, tokenName,
            (byte*)userPin, userPin == NULL ? 0 : strlen(userPin));
        if (ret != 0) {
            fprintf(stderr, "Failed to initialize PKCS#11 token\n");
            ret = 2;
        }
        if (ret == 0) {
            ret = wc_CryptoDev_RegisterDevice(devId, wc_Pkcs11_CryptoDevCb,
                                              &token);
            if (ret != 0) {
                fprintf(stderr, "Failed to register PKCS#11 token\n");
                ret = 2;
            }
            if (ret == 0) {
            #if !defined(NO_HMAC) && !defined(NO_SHA256)
                ret = hmac(devId, &token);
                if (ret != 0)
                    ret = 1;
            #endif
            }
            wc_Pkcs11Token_Final(&token);
        }
        wc_Pkcs11_Finalize(&dev);
    }

    wolfCrypt_Cleanup();

    return ret;
}

