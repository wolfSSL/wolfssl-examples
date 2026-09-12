/* keystore_test.c
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
 * Foundation, Inc., 02110-1301, USA
 */

/* Key Store round trip for the Nuvoton M2354 port.
 *
 * wolfcrypt_test() does not cover wrapped keys, so this exercises a key that
 * never leaves the store: written to a slot, used by handle for AES, read
 * back where allowed, and erased. ECC keys from the store are not supported
 * by the port, so there is nothing to test for them here.
 *
 * SRAM slots only by default. A Flash slot has no per-key erase, only an
 * irreversible revoke, so a run would spend one for good; build with
 * -DM2354_KS_TEST_FLASH to include that leg. */

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/misc.h>
#include <wolfssl/wolfcrypt/port/nuvoton/nuvoton_cryptocb.h>
#include <wolfssl/wolfcrypt/port/nuvoton/nuvoton_key.h>

#include <stdio.h>
#include <string.h>

#define KS_CHECK(cond, msg)                                 \
    do {                                                    \
        if (!(cond)) {                                       \
            printf("  FAIL %s (line %d)\n", (msg), __LINE__); \
            fails++;                                        \
        }                                                   \
    } while (0)

/* AES-256-CBC with the key in a slot, against the same key in software. */
static int ks_aes_test(int mem, const char* memName)
{
    static const byte key[32] = {
        0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
        0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
        0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
        0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
    };
    static const byte iv[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    static const byte plain[32] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
    };
    byte            ksOut[32], swOut[32], readBack[32];
    wc_NuvotonKsKey ksKey;
    Aes             aesKs, aesSw;
    int             fails = 0;
    int             ret;

    printf(" AES-256-CBC from a %s slot\n", memName);

    XMEMSET(&ksKey, 0, sizeof(ksKey));
    ret = wc_NuvotonKs_Write(&ksKey, mem, WC_NUVOTON_KS_OWNER_AES, 256,
        key, sizeof(key), 1);
    KS_CHECK(ret == 0, "wc_NuvotonKs_Write");
    if (ret != 0) {
        return fails;
    }
    printf("  slot %d\n", ksKey.slot);

    /* Readable was asked for, so the material comes back. */
    XMEMSET(readBack, 0, sizeof(readBack));
    ret = wc_NuvotonKs_Read(&ksKey, readBack, sizeof(readBack));
    KS_CHECK(ret == 0, "wc_NuvotonKs_Read");
    KS_CHECK(XMEMCMP(readBack, key, sizeof(key)) == 0, "read back matches");

    ret = wc_AesInit(&aesKs, NULL, WOLFSSL_NUVOTON_DEVID);
    KS_CHECK(ret == 0, "wc_AesInit hw");
    ret = wc_NuvotonKs_SetAesKey(&aesKs, &ksKey);
    KS_CHECK(ret == 0, "wc_NuvotonKs_SetAesKey");
    ret = wc_AesSetIV(&aesKs, iv);
    KS_CHECK(ret == 0, "wc_AesSetIV hw");
    ret = wc_AesCbcEncrypt(&aesKs, ksOut, plain, sizeof(plain));
    KS_CHECK(ret == 0, "wc_AesCbcEncrypt from slot");

    ret = wc_AesInit(&aesSw, NULL, INVALID_DEVID);
    KS_CHECK(ret == 0, "wc_AesInit sw");
    ret = wc_AesSetKey(&aesSw, key, sizeof(key), iv, AES_ENCRYPTION);
    KS_CHECK(ret == 0, "wc_AesSetKey sw");
    ret = wc_AesCbcEncrypt(&aesSw, swOut, plain, sizeof(plain));
    KS_CHECK(ret == 0, "wc_AesCbcEncrypt sw");

    KS_CHECK(XMEMCMP(ksOut, swOut, sizeof(ksOut)) == 0,
        "stored key matches software");

    wc_AesFree(&aesKs);
    wc_AesFree(&aesSw);

    if (mem == WC_NUVOTON_KS_MEM_SRAM) {
        wc_NuvotonKsKey sealed;

        ret = wc_NuvotonKs_Erase(&ksKey);
        KS_CHECK(ret == 0, "wc_NuvotonKs_Erase");

        /* An erased slot must not still answer. */
        ret = wc_NuvotonKs_Read(&ksKey, readBack, sizeof(readBack));
        KS_CHECK(ret != 0, "read of an erased slot is refused");

        /* A slot written without the readable flag is the whole point of the
         * store, so check the engine keeps it in rather than assuming. */
        XMEMSET(&sealed, 0, sizeof(sealed));
        ret = wc_NuvotonKs_Write(&sealed, mem, WC_NUVOTON_KS_OWNER_AES, 256,
            key, sizeof(key), 0);
        KS_CHECK(ret == 0, "write of a non readable key");
        if (ret == 0) {
            ret = wc_NuvotonKs_Read(&sealed, readBack, sizeof(readBack));
            KS_CHECK(ret != 0, "read of a non readable slot is refused");
            (void)wc_NuvotonKs_Erase(&sealed);
        }
    }

    wc_ForceZero(readBack, sizeof(readBack));

    return fails;
}

int keystore_test(void);
int keystore_test(void)
{
    int fails = 0;

    printf("\n--- Key Store round trip ---\n");

    fails += ks_aes_test(WC_NUVOTON_KS_MEM_SRAM, "SRAM");

#ifdef M2354_KS_TEST_FLASH
    /* Consumes a Flash slot for good: there is no per-key erase, only revoke.
     * Off by default so a routine run does not spend the board's slots. */
    fails += ks_aes_test(WC_NUVOTON_KS_MEM_FLASH, "Flash");
#endif

    printf("Key Store: %d failure(s)\n", fails);

    return fails;
}
