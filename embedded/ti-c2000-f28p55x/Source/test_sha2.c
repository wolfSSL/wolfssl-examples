/* test_sha2.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

/* Standalone SHA-2 example for the TI LAUNCHXL-F28P55X (C2000 C28x).
 * Build with `make SHA2=1`.
 *
 * Flow:
 *   1. c2000_board_init() - clocks, SCIA console, timer.
 *   2. One known-answer test (KAT) per SHA-2 variant on "abc" (FIPS 180-4).
 *      Each shows the full API sequence: Init -> Update -> Final -> Free.
 *   3. A streaming KAT: one message fed through several Update calls.
 *   4. A throughput loop per variant using the generic wc_Hash* API.
 *
 * On the C28x a byte is 16 bits (CHAR_BIT == 16).  Every wolfCrypt byte
 * buffer holds one octet per cell, so the message and digest arrays below are
 * written exactly as on an 8-bit-byte target.
 */

#include "driverlib.h"
#include "device.h"

#include <stdio.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/sha512.h>
#include <wolfssl/wolfcrypt/hash.h>

#include "c2000_board.h"

#define BENCH_BLOCK_SZ  1024    /* 1 KiB per Update */
#define BENCH_BLOCKS    64

static const byte msgAbc[] = { 'a', 'b', 'c' };

static int g_failures = 0;

/* Print the digest, compare it to the expected value and report PASS/FAIL. */
static void check_digest(const char* name, int ret, const byte* digest,
    const byte* expected, word32 len)
{
    word32 i;

    if (ret != 0) {
        printf("%-12s KAT: FAIL (ret %d)\r\n", name, ret);
        g_failures++;
        return;
    }
    printf("%-12s ", name);
    for (i = 0; i < len; i++) {
        printf("%02x", (unsigned)(digest[i] & 0xFF));
    }
    printf("\r\n");
    if (XMEMCMP(digest, expected, len) == 0) {
        printf("%-12s KAT: PASS\r\n", name);
    }
    else {
        printf("%-12s KAT: FAIL\r\n", name);
        g_failures++;
    }
}

static void sha224_example(void)
{
    static const byte expected[WC_SHA224_DIGEST_SIZE] = {
        0x23, 0x09, 0x7d, 0x22, 0x34, 0x05, 0xd8, 0x22,
        0x86, 0x42, 0xa4, 0x77, 0xbd, 0xa2, 0x55, 0xb3,
        0x2a, 0xad, 0xbc, 0xe4, 0xbd, 0xa0, 0xb3, 0xf7,
        0xe3, 0x6c, 0x9d, 0xa7
    };
    wc_Sha224 sha;
    byte digest[WC_SHA224_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha224(&sha);
    if (ret == 0) {
        ret = wc_Sha224Update(&sha, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Sha224Final(&sha, digest);
        }
        wc_Sha224Free(&sha);
    }
    check_digest("SHA-224", ret, digest, expected, sizeof(digest));
}

static void sha256_example(void)
{
    static const byte expected[WC_SHA256_DIGEST_SIZE] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    };
    wc_Sha256 sha;
    byte digest[WC_SHA256_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha256(&sha);
    if (ret == 0) {
        ret = wc_Sha256Update(&sha, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Sha256Final(&sha, digest);
        }
        wc_Sha256Free(&sha);
    }
    check_digest("SHA-256", ret, digest, expected, sizeof(digest));
}

/* The same hash state can absorb a message in any number of Update calls.
 * The 56-octet FIPS 180-4 message is split 20 + 20 + 16. */
static void sha256_stream_example(void)
{
    static const byte msg[] = {
        'a','b','c','d','b','c','d','e','c','d','e','f','d','e','f','g',
        'e','f','g','h','f','g','h','i','g','h','i','j','h','i','j','k',
        'i','j','k','l','j','k','l','m','k','l','m','n','l','m','n','o',
        'm','n','o','p','n','o','p','q'
    };
    static const byte expected[WC_SHA256_DIGEST_SIZE] = {
        0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
        0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
        0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
        0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1
    };
    wc_Sha256 sha;
    byte digest[WC_SHA256_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha256(&sha);
    if (ret == 0) {
        ret = wc_Sha256Update(&sha, msg, 20);
        if (ret == 0) {
            ret = wc_Sha256Update(&sha, msg + 20, 20);
        }
        if (ret == 0) {
            ret = wc_Sha256Update(&sha, msg + 40, (word32)sizeof(msg) - 40);
        }
        if (ret == 0) {
            ret = wc_Sha256Final(&sha, digest);
        }
        wc_Sha256Free(&sha);
    }
    check_digest("SHA-256 3x", ret, digest, expected, sizeof(digest));
}

static void sha384_example(void)
{
    static const byte expected[WC_SHA384_DIGEST_SIZE] = {
        0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
        0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
        0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
        0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
        0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
        0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7
    };
    wc_Sha384 sha;
    byte digest[WC_SHA384_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha384(&sha);
    if (ret == 0) {
        ret = wc_Sha384Update(&sha, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Sha384Final(&sha, digest);
        }
        wc_Sha384Free(&sha);
    }
    check_digest("SHA-384", ret, digest, expected, sizeof(digest));
}

static void sha512_example(void)
{
    static const byte expected[WC_SHA512_DIGEST_SIZE] = {
        0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
        0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
        0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
        0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
        0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
        0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
        0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
        0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f
    };
    wc_Sha512 sha;
    byte digest[WC_SHA512_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha512(&sha);
    if (ret == 0) {
        ret = wc_Sha512Update(&sha, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Sha512Final(&sha, digest);
        }
        wc_Sha512Free(&sha);
    }
    check_digest("SHA-512", ret, digest, expected, sizeof(digest));
}

static void sha512_224_example(void)
{
    static const byte expected[WC_SHA512_224_DIGEST_SIZE] = {
        0x46, 0x34, 0x27, 0x0f, 0x70, 0x7b, 0x6a, 0x54,
        0xda, 0xae, 0x75, 0x30, 0x46, 0x08, 0x42, 0xe2,
        0x0e, 0x37, 0xed, 0x26, 0x5c, 0xee, 0xe9, 0xa4,
        0x3e, 0x89, 0x24, 0xaa
    };
    wc_Sha512 sha;
    byte digest[WC_SHA512_224_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha512_224(&sha);
    if (ret == 0) {
        ret = wc_Sha512_224Update(&sha, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Sha512_224Final(&sha, digest);
        }
        wc_Sha512_224Free(&sha);
    }
    check_digest("SHA-512/224", ret, digest, expected, sizeof(digest));
}

static void sha512_256_example(void)
{
    static const byte expected[WC_SHA512_256_DIGEST_SIZE] = {
        0x53, 0x04, 0x8e, 0x26, 0x81, 0x94, 0x1e, 0xf9,
        0x9b, 0x2e, 0x29, 0xb7, 0x6b, 0x4c, 0x7d, 0xab,
        0xe4, 0xc2, 0xd0, 0xc6, 0x34, 0xfc, 0x6d, 0x46,
        0xe0, 0xe2, 0xf1, 0x31, 0x07, 0xe7, 0xaf, 0x23
    };
    wc_Sha512 sha;
    byte digest[WC_SHA512_256_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha512_256(&sha);
    if (ret == 0) {
        ret = wc_Sha512_256Update(&sha, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Sha512_256Final(&sha, digest);
        }
        wc_Sha512_256Free(&sha);
    }
    check_digest("SHA-512/256", ret, digest, expected, sizeof(digest));
}

/* Hash BENCH_BLOCKS KiB and print the throughput. */
static void hash_bench(const char* name, enum wc_HashType type)
{
    static byte data[BENCH_BLOCK_SZ];
    static wc_HashAlg hash;
    byte digest[WC_MAX_DIGEST_SIZE];
    double secs;
    int ret;
    int i;

    for (i = 0; i < BENCH_BLOCK_SZ; i++) {
        data[i] = (byte)(i & 0xFF);
    }

    (void)current_time(1);
    ret = wc_HashInit(&hash, type);
    for (i = 0; i < BENCH_BLOCKS && ret == 0; i++) {
        ret = wc_HashUpdate(&hash, type, data, BENCH_BLOCK_SZ);
    }
    if (ret == 0) {
        ret = wc_HashFinal(&hash, type, digest);
    }
    secs = current_time(0);
    (void)wc_HashFree(&hash, type);

    if (ret != 0 || secs <= 0.0) {
        printf("%-12s bench: FAIL (ret %d)\r\n", name, ret);
        g_failures++;
        return;
    }
    printf("%-12s %d KiB in %.3f s = %.1f KiB/s\r\n", name,
           BENCH_BLOCKS, secs, (double)BENCH_BLOCKS / secs);
}

int main(void)
{
    c2000_board_init();

    printf("\r\n=== wolfCrypt SHA-2 example on TI C2000 "
           "LAUNCHXL-F28P55X ===\r\n");

    printf("\r\n--- Known-answer tests ---\r\n");
    sha224_example();
    sha256_example();
    sha256_stream_example();
    sha384_example();
    sha512_example();
    sha512_224_example();
    sha512_256_example();

    printf("\r\n--- Throughput (%d MHz) ---\r\n",
           (int)(DEVICE_SYSCLK_FREQ / 1000000UL));
    hash_bench("SHA-224", WC_HASH_TYPE_SHA224);
    hash_bench("SHA-256", WC_HASH_TYPE_SHA256);
    hash_bench("SHA-384", WC_HASH_TYPE_SHA384);
    hash_bench("SHA-512", WC_HASH_TYPE_SHA512);
    hash_bench("SHA-512/224", WC_HASH_TYPE_SHA512_224);
    hash_bench("SHA-512/256", WC_HASH_TYPE_SHA512_256);

    printf("\r\nSHA-2 example: %s (%d failure(s))\r\n",
           (g_failures == 0) ? "PASS" : "FAIL", g_failures);
    printf("=== done ===\r\n");

    for (;;) {
        /* spin */
    }
}
