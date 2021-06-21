/* pkcs11_genecc.c
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

static WC_RNG rng;

int gen_ec_keys(Pkcs11Token* token, ecc_key* key, unsigned char* id, int idLen,
                int devId)
{
    int ret;

    ret = wc_ecc_init_id(key, id, idLen, NULL, devId);
    if (ret != 0)
        fprintf(stderr, "Failed to initialize EC key: %d\n", ret);
    if (ret == 0) {
        ret = wc_ecc_make_key_ex2(&rng, 32, key, ECC_CURVE_DEF,
                                  WC_ECC_FLAG_DEC_SIGN);
        if (ret != 0)
            fprintf(stderr, "Failed to generate EC key: %d\n", ret);
    }
    return ret;
}

int ecdsa_sign_verify(int devId, Pkcs11Token* token)
{
    int    ret = 0;
    byte   hash[32], out[128];
    word32 hashSz, outSz;
    int    verify;
    ecc_key eccKey;

    memset(hash, 9, sizeof(hash));
    hashSz = sizeof(hash);
    outSz = sizeof(out);

    ret = wc_Pkcs11Token_Open(token, 1);
    if (ret == 0) {
        fprintf(stderr, "Generate EC Keys\n");
        ret = gen_ec_keys(token, &eccKey, (unsigned char*)"123ecc", 6, devId);

        if (ret == 0) {
            ret = wc_ecc_sign_hash(hash, hashSz, out, &outSz, &rng, &eccKey);
            if (ret < 0)
                fprintf(stderr, "Failed to sign: %d\n", ret);
        }
        if (ret == 0) {
            /* Don't use device for public key operation. */
            eccKey.devId = INVALID_DEVID;

            ret = wc_ecc_verify_hash(out, outSz, hash, (int)hashSz, &verify,
                                                                       &eccKey);
            if (ret < 0 || !verify)
                fprintf(stderr, "Failed to verify: %d (%d)\n", ret, verify);
            if (!verify)
                ret = -1;
        }
        wc_Pkcs11Token_Close(token);
        wc_ecc_free(&eccKey);
    }

    return ret;
}


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
               "Usage: pkcs11_genecc <libname> <slot> <tokenname> [userpin]\n");
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
                wc_InitRng_ex(&rng, NULL, devId);

                ret = ecdsa_sign_verify(devId, &token);
                if (ret != 0)
                    ret = 1;

                wc_FreeRng(&rng);
            }
            wc_Pkcs11Token_Final(&token);
        }
        wc_Pkcs11_Finalize(&dev);
    }

    wolfCrypt_Cleanup();

    return ret;
}

