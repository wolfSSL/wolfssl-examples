/* test_sha3.c
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

/* Standalone SHA-3 example for the TI LAUNCHXL-F28P55X (C2000 C28x).
 * Build with `make SHA3=1`.
 *
 * Flow:
 *   1. c2000_board_init() - clocks, SCIA console, timer.
 *   2. One known-answer test (KAT) per SHA-3 variant on "abc" (FIPS 202).
 *      Each shows the full API sequence: Init -> Update -> Final -> Free.
 *   3. SHAKE128 / SHAKE256 (extendable output) KATs, then a streaming KAT
 *      that feeds one message through several Update calls.
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
#include <wolfssl/wolfcrypt/sha3.h>
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

static void sha3_224_example(void)
{
    static const byte expected[WC_SHA3_224_DIGEST_SIZE] = {
        0xe6, 0x42, 0x82, 0x4c, 0x3f, 0x8c, 0xf2, 0x4a,
        0xd0, 0x92, 0x34, 0xee, 0x7d, 0x3c, 0x76, 0x6f,
        0xc9, 0xa3, 0xa5, 0x16, 0x8d, 0x0c, 0x94, 0xad,
        0x73, 0xb4, 0x6f, 0xdf
    };
    wc_Sha3 sha;
    byte digest[WC_SHA3_224_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha3_224(&sha, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_Sha3_224_Update(&sha, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Sha3_224_Final(&sha, digest);
        }
        wc_Sha3_224_Free(&sha);
    }
    check_digest("SHA3-224", ret, digest, expected, sizeof(digest));
}

static void sha3_256_example(void)
{
    static const byte expected[WC_SHA3_256_DIGEST_SIZE] = {
        0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2,
        0x04, 0x5c, 0x17, 0x2d, 0x6b, 0xd3, 0x90, 0xbd,
        0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d, 0x52, 0x5b,
        0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32
    };
    wc_Sha3 sha;
    byte digest[WC_SHA3_256_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha3_256(&sha, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_Sha3_256_Update(&sha, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Sha3_256_Final(&sha, digest);
        }
        wc_Sha3_256_Free(&sha);
    }
    check_digest("SHA3-256", ret, digest, expected, sizeof(digest));
}

static void sha3_384_example(void)
{
    static const byte expected[WC_SHA3_384_DIGEST_SIZE] = {
        0xec, 0x01, 0x49, 0x82, 0x88, 0x51, 0x6f, 0xc9,
        0x26, 0x45, 0x9f, 0x58, 0xe2, 0xc6, 0xad, 0x8d,
        0xf9, 0xb4, 0x73, 0xcb, 0x0f, 0xc0, 0x8c, 0x25,
        0x96, 0xda, 0x7c, 0xf0, 0xe4, 0x9b, 0xe4, 0xb2,
        0x98, 0xd8, 0x8c, 0xea, 0x92, 0x7a, 0xc7, 0xf5,
        0x39, 0xf1, 0xed, 0xf2, 0x28, 0x37, 0x6d, 0x25
    };
    wc_Sha3 sha;
    byte digest[WC_SHA3_384_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha3_384(&sha, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_Sha3_384_Update(&sha, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Sha3_384_Final(&sha, digest);
        }
        wc_Sha3_384_Free(&sha);
    }
    check_digest("SHA3-384", ret, digest, expected, sizeof(digest));
}

static void sha3_512_example(void)
{
    static const byte expected[WC_SHA3_512_DIGEST_SIZE] = {
        0xb7, 0x51, 0x85, 0x0b, 0x1a, 0x57, 0x16, 0x8a,
        0x56, 0x93, 0xcd, 0x92, 0x4b, 0x6b, 0x09, 0x6e,
        0x08, 0xf6, 0x21, 0x82, 0x74, 0x44, 0xf7, 0x0d,
        0x88, 0x4f, 0x5d, 0x02, 0x40, 0xd2, 0x71, 0x2e,
        0x10, 0xe1, 0x16, 0xe9, 0x19, 0x2a, 0xf3, 0xc9,
        0x1a, 0x7e, 0xc5, 0x76, 0x47, 0xe3, 0x93, 0x40,
        0x57, 0x34, 0x0b, 0x4c, 0xf4, 0x08, 0xd5, 0xa5,
        0x65, 0x92, 0xf8, 0x27, 0x4e, 0xec, 0x53, 0xf0
    };
    wc_Sha3 sha;
    byte digest[WC_SHA3_512_DIGEST_SIZE];
    int ret;

    ret = wc_InitSha3_512(&sha, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_Sha3_512_Update(&sha, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Sha3_512_Final(&sha, digest);
        }
        wc_Sha3_512_Free(&sha);
    }
    check_digest("SHA3-512", ret, digest, expected, sizeof(digest));
}

/* SHAKE128 is an XOF: the caller picks the output length at Final. */
static void shake128_example(void)
{
    static const byte expected[32] = {
        0x58, 0x81, 0x09, 0x2d, 0xd8, 0x18, 0xbf, 0x5c,
        0xf8, 0xa3, 0xdd, 0xb7, 0x93, 0xfb, 0xcb, 0xa7,
        0x40, 0x97, 0xd5, 0xc5, 0x26, 0xa6, 0xd3, 0x5f,
        0x97, 0xb8, 0x33, 0x51, 0x94, 0x0f, 0x2c, 0xc8
    };
    wc_Shake shake;
    byte out[32];
    int ret;

    ret = wc_InitShake128(&shake, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_Shake128_Update(&shake, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Shake128_Final(&shake, out, (word32)sizeof(out));
        }
        wc_Shake128_Free(&shake);
    }
    check_digest("SHAKE128", ret, out, expected, sizeof(out));
}

/* SHAKE256 is an XOF: the caller picks the output length at Final. */
static void shake256_example(void)
{
    static const byte expected[64] = {
        0x48, 0x33, 0x66, 0x60, 0x13, 0x60, 0xa8, 0x77,
        0x1c, 0x68, 0x63, 0x08, 0x0c, 0xc4, 0x11, 0x4d,
        0x8d, 0xb4, 0x45, 0x30, 0xf8, 0xf1, 0xe1, 0xee,
        0x4f, 0x94, 0xea, 0x37, 0xe7, 0x8b, 0x57, 0x39,
        0xd5, 0xa1, 0x5b, 0xef, 0x18, 0x6a, 0x53, 0x86,
        0xc7, 0x57, 0x44, 0xc0, 0x52, 0x7e, 0x1f, 0xaa,
        0x9f, 0x87, 0x26, 0xe4, 0x62, 0xa1, 0x2a, 0x4f,
        0xeb, 0x06, 0xbd, 0x88, 0x01, 0xe7, 0x51, 0xe4
    };
    wc_Shake shake;
    byte out[64];
    int ret;

    ret = wc_InitShake256(&shake, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_Shake256_Update(&shake, msgAbc, (word32)sizeof(msgAbc));
        if (ret == 0) {
            ret = wc_Shake256_Final(&shake, out, (word32)sizeof(out));
        }
        wc_Shake256_Free(&shake);
    }
    check_digest("SHAKE256", ret, out, expected, sizeof(out));
}

/* The same state can absorb a message in any number of Update calls.
 * The 56-octet FIPS message is split 20 + 20 + 16. */
static void shake256_stream_example(void)
{
    static const byte msg[] = {
        'a','b','c','d','b','c','d','e','c','d','e','f','d','e','f','g',
        'e','f','g','h','f','g','h','i','g','h','i','j','h','i','j','k',
        'i','j','k','l','j','k','l','m','k','l','m','n','l','m','n','o',
        'm','n','o','p','n','o','p','q'
    };
    static const byte expected[64] = {
        0x4d, 0x8c, 0x2d, 0xd2, 0x43, 0x5a, 0x01, 0x28,
        0xee, 0xfb, 0xb8, 0xc3, 0x6f, 0x6f, 0x87, 0x13,
        0x3a, 0x79, 0x11, 0xe1, 0x8d, 0x97, 0x9e, 0xe1,
        0xae, 0x6b, 0xe5, 0xd4, 0xfd, 0x2e, 0x33, 0x29,
        0x40, 0xd8, 0x68, 0x8a, 0x4e, 0x6a, 0x59, 0xaa,
        0x80, 0x60, 0xf1, 0xf9, 0xbc, 0x99, 0x6c, 0x05,
        0xac, 0xa3, 0xc6, 0x96, 0xa8, 0xb6, 0x62, 0x79,
        0xdc, 0x67, 0x2c, 0x74, 0x0b, 0xb2, 0x24, 0xec
    };
    wc_Shake shake;
    byte out[64];
    int ret;

    ret = wc_InitShake256(&shake, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_Shake256_Update(&shake, msg, 20);
        if (ret == 0) {
            ret = wc_Shake256_Update(&shake, msg + 20, 20);
        }
        if (ret == 0) {
            ret = wc_Shake256_Update(&shake, msg + 40,
                                     (word32)sizeof(msg) - 40);
        }
        if (ret == 0) {
            ret = wc_Shake256_Final(&shake, out, (word32)sizeof(out));
        }
        wc_Shake256_Free(&shake);
    }
    check_digest("SHAKE256 3x", ret, out, expected, sizeof(out));
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

    printf("\r\n=== wolfCrypt SHA-3 example on TI C2000 "
           "LAUNCHXL-F28P55X ===\r\n");

    printf("\r\n--- Known-answer tests ---\r\n");
    sha3_224_example();
    sha3_256_example();
    sha3_384_example();
    sha3_512_example();
    shake128_example();
    shake256_example();
    shake256_stream_example();

    printf("\r\n--- Throughput (%d MHz) ---\r\n",
           (int)(DEVICE_SYSCLK_FREQ / 1000000UL));
    hash_bench("SHA3-224", WC_HASH_TYPE_SHA3_224);
    hash_bench("SHA3-256", WC_HASH_TYPE_SHA3_256);
    hash_bench("SHA3-384", WC_HASH_TYPE_SHA3_384);
    hash_bench("SHA3-512", WC_HASH_TYPE_SHA3_512);
    hash_bench("SHAKE128", WC_HASH_TYPE_SHAKE128);
    hash_bench("SHAKE256", WC_HASH_TYPE_SHAKE256);

    printf("\r\nSHA-3 example: %s (%d failure(s))\r\n",
           (g_failures == 0) ? "PASS" : "FAIL", g_failures);
    printf("=== done ===\r\n");

    for (;;) {
        /* spin */
    }
}
