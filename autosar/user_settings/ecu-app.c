/* ecu-app.c
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* A minimal application SW-C for the user_settings.h build.
 *
 * Note what is NOT included: there is no <wolfssl/options.h> here, because
 * this build has no generated options.h. The configuration comes from
 * user_settings.h, which <wolfssl/wolfcrypt/settings.h> pulls in because the
 * build defines WOLFSSL_USER_SETTINGS.
 *
 * Exercises everything the trimmed configuration claims to support, so a
 * mis-trimmed user_settings.h fails here rather than in the field.
 */

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/port/autosar/Csm.h>

#include <stdio.h>

#define BLOCK_SIZE 16

#ifdef REDIRECTION_CONFIG
    #define KEY_SLOT ((uint32)REDIRECTION_IN1_KEYID)
    #define IV_SLOT  ((uint32)REDIRECTION_IN2_KEYID)
#else
    #define KEY_SLOT 0U
    #define IV_SLOT  1U
#endif

static int failures = 0;

static void check(const char* what, int ok)
{
    printf("  %-34s %s\n", what, ok ? "ok" : "FAILED");
    if (!ok) {
        failures++;
    }
}

/* AES-CBC at every key length this build enabled. The driver takes the length
 * from the keystore, so this is also a check that user_settings.h enabled the
 * WOLFSSL_AES_* sizes it thinks it did. */
static int cbc_roundtrip(uint32 keySz)
{
    uint8  key[32];
    uint8  iv[BLOCK_SIZE];
    uint8  plain[BLOCK_SIZE];
    uint8  cipher[BLOCK_SIZE];
    uint8  recovered[BLOCK_SIZE];
    uint32 len;
    uint32 i;

    for (i = 0; i < sizeof(key); i++) {
        key[i] = (uint8)i;
    }
    for (i = 0; i < BLOCK_SIZE; i++) {
        iv[i]    = (uint8)(0x10 + i);
        plain[i] = (uint8)('a' + i);
    }

    if (Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY, key, keySz) != E_OK) {
        return 0;
    }
    if (Csm_KeyElementSet(IV_SLOT, CRYPTO_KE_CIPHER_IV, iv, BLOCK_SIZE)
            != E_OK) {
        return 0;
    }

    len = sizeof(cipher);
    if (Csm_Encrypt(1U, CRYPTO_OPERATIONMODE_SINGLECALL, plain, BLOCK_SIZE,
                cipher, &len) != E_OK) {
        return 0;
    }

    len = sizeof(recovered);
    if (Csm_Decrypt(2U, CRYPTO_OPERATIONMODE_SINGLECALL, cipher, BLOCK_SIZE,
                recovered, &len) != E_OK) {
        return 0;
    }

    return XMEMCMP(plain, recovered, BLOCK_SIZE) == 0;
}

static int random_works(void)
{
    uint8  buf[32];
    uint32 len = (uint32)sizeof(buf);
    uint32 i;
    uint8  acc = 0;

    XMEMSET(buf, 0, sizeof(buf));
    if (Csm_RandomGenerate(0U, buf, &len) != E_OK) {
        return 0;
    }
    for (i = 0; i < sizeof(buf); i++) {
        acc |= buf[i];
    }
    return acc != 0;
}

#ifdef WOLFSSL_AUTOSAR_CMAC
static int mac_works(void)
{
    /* RFC 4493 example 2 */
    static const uint8 k[BLOCK_SIZE] = {
        0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
        0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c
    };
    static const uint8 m[BLOCK_SIZE] = {
        0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a
    };
    static const uint8 expect[BLOCK_SIZE] = {
        0x07,0x0a,0x16,0xb4,0x6b,0x4d,0x41,0x44,
        0xf7,0x9b,0xdd,0x9d,0xd0,0x4a,0x28,0x7c
    };
    uint8  mac[BLOCK_SIZE];
    uint32 len = (uint32)sizeof(mac);
    Crypto_VerifyResultType verify = CRYPTO_E_VER_NOT_OK;

    if (Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_MAC_KEY, k, BLOCK_SIZE) != E_OK) {
        return 0;
    }
    if (Csm_MacGenerate(3U, CRYPTO_OPERATIONMODE_SINGLECALL, m, BLOCK_SIZE,
                mac, &len) != E_OK) {
        return 0;
    }
    if (len != BLOCK_SIZE || XMEMCMP(mac, expect, BLOCK_SIZE) != 0) {
        return 0;
    }
    /* BLOCK_SIZE * 8: Csm_MacVerify() takes the length in BITS, as the
     * specification defines it, so the full 128 bit tag is checked. Passing
     * 16 here asked for 16 bits and was refused as below the minimum. */
    if (Csm_MacVerify(4U, CRYPTO_OPERATIONMODE_SINGLECALL, m, BLOCK_SIZE,
                mac, BLOCK_SIZE * 8, &verify) != E_OK) {
        return 0;
    }
    return verify == CRYPTO_E_VER_OK;
}
#endif

int main(void)
{
    Std_VersionInfoType version;

    Csm_Init(NULL);

    XMEMSET(&version, 0, sizeof(version));
    Csm_GetVersionInfo(&version);
    printf("AUTOSAR CSM on a user_settings.h build\n");
    printf("  wolfSSL %d.%d.%d, MAX_KEYSTORE=%d, MAX_JOBS=%d\n\n",
            version.sw_major_version, version.sw_minor_version,
            version.sw_patch_version, MAX_KEYSTORE, MAX_JOBS);

#ifdef WOLFSSL_AES_128
    check("AES-128-CBC round trip", cbc_roundtrip(16));
#endif
#ifdef WOLFSSL_AES_192
    check("AES-192-CBC round trip", cbc_roundtrip(24));
#endif
#ifdef WOLFSSL_AES_256
    check("AES-256-CBC round trip", cbc_roundtrip(32));
#endif

    check("Csm_RandomGenerate", random_works());

#ifdef WOLFSSL_AUTOSAR_CMAC
    check("AES-CMAC known answer", mac_works());
#else
    printf("  %-34s %s\n", "AES-CMAC", "not built");
#endif

    if (failures != 0) {
        printf("\necu-app: FAIL\n");
        return 1;
    }

    printf("\necu-app: PASS\n");
    return 0;
}
