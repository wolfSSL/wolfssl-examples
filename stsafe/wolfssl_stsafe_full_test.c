/* wolfssl_stsafe_full_test.c
 *
 * Comprehensive wolfSSL integration tests with STSAFE-A120
 * Tests: RNG, ECC KeyGen, ECDSA Sign/Verify, ECDH, Benchmarks, Brainpool curves
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

/* wolfSSL headers */
#include "user_settings.h"
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/port/st/stsafe.h>

/* Test result macros */
#define TEST_PASS(msg) do { printf("[PASS] %s\n", msg); g_passed++; } while(0)
#define TEST_FAIL(msg) do { printf("[FAIL] %s\n", msg); g_failed++; } while(0)

static int g_passed = 0;
static int g_failed = 0;

/* STSAFE crypto callback context */
static wolfSTSAFE_CryptoCb_Ctx g_stsafe_ctx;
static int g_devId = INVALID_DEVID;

/* Get time in microseconds */
static double get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
}

/*-----------------------------------------------------------------------------
 * Test: RNG Performance
 *---------------------------------------------------------------------------*/
static int test_rng_benchmark(void)
{
    WC_RNG rng;
    byte rand_buf[256];
    int ret;
    int iterations = 100;
    double start, end, elapsed;
    int i;

    printf("\nTest: RNG Benchmark (STSAFE-A120)\n");

    ret = wc_InitRng_ex(&rng, NULL, g_devId);
    if (ret != 0) {
        printf("  Error: wc_InitRng_ex failed: %d\n", ret);
        TEST_FAIL("RNG benchmark init");
        return -1;
    }

    /* Warm up */
    wc_RNG_GenerateBlock(&rng, rand_buf, sizeof(rand_buf));

    /* Benchmark */
    start = get_time_us();
    for (i = 0; i < iterations; i++) {
        ret = wc_RNG_GenerateBlock(&rng, rand_buf, sizeof(rand_buf));
        if (ret != 0) break;
    }
    end = get_time_us();
    elapsed = (end - start) / 1000.0; /* ms */

    if (ret != 0) {
        printf("  Error: wc_RNG_GenerateBlock failed: %d\n", ret);
        wc_FreeRng(&rng);
        TEST_FAIL("RNG benchmark");
        return -1;
    }

    printf("  Generated %d x 256 bytes in %.2f ms\n", iterations, elapsed);
    printf("  Throughput: %.2f KB/s\n",
           (iterations * 256.0) / (elapsed / 1000.0) / 1024.0);

    wc_FreeRng(&rng);
    TEST_PASS("RNG benchmark");
    return 0;
}

/*-----------------------------------------------------------------------------
 * Test: ECDSA P-256 Benchmark
 *---------------------------------------------------------------------------*/
static int test_ecdsa_p256_benchmark(void)
{
    ecc_key key;
    WC_RNG rng;
    int ret;
    byte digest[32];
    byte sig[128];
    word32 sigLen;
    int verified = 0;
    int iterations = 10;
    double start, end, elapsed;
    int i;

    printf("\nTest: ECDSA P-256 Benchmark (STSAFE-A120)\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("  Error: wc_InitRng failed: %d\n", ret);
        TEST_FAIL("ECDSA P-256 benchmark RNG init");
        return -1;
    }

    memset(digest, 0xAB, sizeof(digest));

    ret = wc_ecc_init_ex(&key, NULL, g_devId);
    if (ret != 0) {
        printf("  Error: wc_ecc_init_ex failed: %d\n", ret);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-256 benchmark key init");
        return -1;
    }

    /* Benchmark key generation */
    start = get_time_us();
    ret = wc_ecc_make_key_ex(&rng, 32, &key, ECC_SECP256R1);
    end = get_time_us();
    elapsed = (end - start) / 1000.0;

    if (ret != 0) {
        printf("  Error: wc_ecc_make_key_ex failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-256 keygen");
        return -1;
    }
    printf("  Key generation: %.2f ms\n", elapsed);

    /* Benchmark signing */
    start = get_time_us();
    for (i = 0; i < iterations; i++) {
        sigLen = sizeof(sig);
        ret = wc_ecc_sign_hash(digest, sizeof(digest), sig, &sigLen, &rng, &key);
        if (ret != 0) break;
    }
    end = get_time_us();
    elapsed = (end - start) / 1000.0;

    if (ret != 0) {
        printf("  Error: wc_ecc_sign_hash failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-256 sign");
        return -1;
    }
    printf("  Signing: %d ops in %.2f ms (%.2f ops/sec)\n",
           iterations, elapsed, iterations / (elapsed / 1000.0));

    /* Benchmark verification */
    start = get_time_us();
    for (i = 0; i < iterations; i++) {
        ret = wc_ecc_verify_hash(sig, sigLen, digest, sizeof(digest), &verified, &key);
        if (ret != 0 || verified != 1) break;
    }
    end = get_time_us();
    elapsed = (end - start) / 1000.0;

    if (ret != 0 || verified != 1) {
        printf("  Error: wc_ecc_verify_hash failed: %d, verified=%d\n", ret, verified);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-256 verify");
        return -1;
    }
    printf("  Verification: %d ops in %.2f ms (%.2f ops/sec)\n",
           iterations, elapsed, iterations / (elapsed / 1000.0));

    wc_ecc_free(&key);
    wc_FreeRng(&rng);
    TEST_PASS("ECDSA P-256 benchmark");
    return 0;
}

/*-----------------------------------------------------------------------------
 * Test: ECDH Key Exchange
 * Note: STSAFE has a single key slot, so we generate one key in STSAFE
 * and one in software to test ECDH properly.
 *---------------------------------------------------------------------------*/
static int test_ecdh_p256(void)
{
    ecc_key keyHW;      /* Hardware key in STSAFE */
    ecc_key keySW;      /* Software key */
    WC_RNG rng;
    int ret;
    byte sharedHW[32], sharedSW[32];
    word32 sharedHWLen = sizeof(sharedHW), sharedSWLen = sizeof(sharedSW);
    double start, end;
    int i;

    printf("\nTest: ECDH P-256 Key Exchange (HW + SW)\n");
    printf("  Note: STSAFE key used for one side, software for other\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("  Error: wc_InitRng failed: %d\n", ret);
        TEST_FAIL("ECDH P-256 RNG init");
        return -1;
    }

    /* Initialize hardware key (with STSAFE devId) */
    ret = wc_ecc_init_ex(&keyHW, NULL, g_devId);
    if (ret != 0) {
        printf("  Error: wc_ecc_init_ex (HW) failed: %d\n", ret);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDH P-256 key init HW");
        return -1;
    }

    /* Initialize software key (INVALID_DEVID for software) */
    ret = wc_ecc_init_ex(&keySW, NULL, INVALID_DEVID);
    if (ret != 0) {
        printf("  Error: wc_ecc_init_ex (SW) failed: %d\n", ret);
        wc_ecc_free(&keyHW);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDH P-256 key init SW");
        return -1;
    }

    /* Generate hardware key pair (in STSAFE) */
    printf("  Generating hardware key pair (STSAFE)...\n");
    start = get_time_us();
    ret = wc_ecc_make_key_ex(&rng, 32, &keyHW, ECC_SECP256R1);
    end = get_time_us();
    if (ret != 0) {
        printf("  Error: wc_ecc_make_key_ex (HW) failed: %d\n", ret);
        wc_ecc_free(&keyHW);
        wc_ecc_free(&keySW);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDH P-256 keygen HW");
        return -1;
    }
    printf("    HW Key generated in %.2f ms\n", (end - start) / 1000.0);

    /* Generate software key pair */
    printf("  Generating software key pair...\n");
    start = get_time_us();
    ret = wc_ecc_make_key_ex(&rng, 32, &keySW, ECC_SECP256R1);
    end = get_time_us();
    if (ret != 0) {
        printf("  Error: wc_ecc_make_key_ex (SW) failed: %d\n", ret);
        wc_ecc_free(&keyHW);
        wc_ecc_free(&keySW);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDH P-256 keygen SW");
        return -1;
    }
    printf("    SW Key generated in %.2f ms\n", (end - start) / 1000.0);

    /* Set RNG on keys for shared secret computation */
    ret = wc_ecc_set_rng(&keyHW, &rng);
    if (ret != 0) {
        printf("  Error: wc_ecc_set_rng (HW) failed: %d\n", ret);
        wc_ecc_free(&keyHW);
        wc_ecc_free(&keySW);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDH P-256 set RNG HW");
        return -1;
    }
    ret = wc_ecc_set_rng(&keySW, &rng);
    if (ret != 0) {
        printf("  Error: wc_ecc_set_rng (SW) failed: %d\n", ret);
        wc_ecc_free(&keyHW);
        wc_ecc_free(&keySW);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDH P-256 set RNG SW");
        return -1;
    }

    /* Compute shared secret using STSAFE (HW priv * SW pub) */
    printf("  Computing shared secret (STSAFE: HW priv * SW pub)...\n");
    start = get_time_us();
    ret = wc_ecc_shared_secret(&keyHW, &keySW, sharedHW, &sharedHWLen);
    end = get_time_us();
    if (ret != 0) {
        printf("  Error: wc_ecc_shared_secret (HW) failed: %d\n", ret);
        wc_ecc_free(&keyHW);
        wc_ecc_free(&keySW);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDH P-256 shared secret HW");
        return -1;
    }
    printf("    STSAFE ECDH computed in %.2f ms\n", (end - start) / 1000.0);

    /* Compute shared secret using software (SW priv * HW pub) */
    printf("  Computing shared secret (Software: SW priv * HW pub)...\n");
    start = get_time_us();
    ret = wc_ecc_shared_secret(&keySW, &keyHW, sharedSW, &sharedSWLen);
    end = get_time_us();
    if (ret != 0) {
        printf("  Error: wc_ecc_shared_secret (SW) failed: %d\n", ret);
        wc_ecc_free(&keyHW);
        wc_ecc_free(&keySW);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDH P-256 shared secret SW");
        return -1;
    }
    printf("    Software ECDH computed in %.2f ms\n", (end - start) / 1000.0);

    /* Compare shared secrets - they must match! */
    if (sharedHWLen != sharedSWLen || memcmp(sharedHW, sharedSW, sharedHWLen) != 0) {
        printf("  Error: Shared secrets don't match!\n");
        printf("    HW: ");
        for (i = 0; i < 8; i++) printf("%02X", sharedHW[i]);
        printf("...\n");
        printf("    SW: ");
        for (i = 0; i < 8; i++) printf("%02X", sharedSW[i]);
        printf("...\n");
        wc_ecc_free(&keyHW);
        wc_ecc_free(&keySW);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDH P-256 shared secret mismatch");
        return -1;
    }

    printf("  Shared secrets match! (");
    for (i = 0; i < 8; i++) printf("%02X", sharedHW[i]);
    printf("...)\n");

    wc_ecc_free(&keyHW);
    wc_ecc_free(&keySW);
    wc_FreeRng(&rng);
    TEST_PASS("ECDH P-256 key exchange (HW+SW)");
    return 0;
}

/*-----------------------------------------------------------------------------
 * Test: ECDSA Brainpool P-256 Benchmark
 *---------------------------------------------------------------------------*/
#if defined(HAVE_ECC_BRAINPOOL) && defined(STSE_CONF_ECC_BRAINPOOL_P_256)
static int test_ecdsa_brainpool_p256_benchmark(void)
{
    ecc_key key;
    WC_RNG rng;
    int ret;
    byte digest[32];
    byte sig[128];
    word32 sigLen;
    int verified = 0;
    int iterations = 10;
    double start, end, elapsed;
    int i;

    printf("\nTest: ECDSA Brainpool P-256 Benchmark (STSAFE-A120)\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("  Error: wc_InitRng failed: %d\n", ret);
        TEST_FAIL("ECDSA Brainpool P-256 benchmark RNG init");
        return -1;
    }

    memset(digest, 0xAB, sizeof(digest));

    ret = wc_ecc_init_ex(&key, NULL, g_devId);
    if (ret != 0) {
        printf("  Error: wc_ecc_init_ex failed: %d\n", ret);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA Brainpool P-256 benchmark key init");
        return -1;
    }

    /* Benchmark key generation */
    start = get_time_us();
    ret = wc_ecc_make_key_ex(&rng, 32, &key, ECC_BRAINPOOLP256R1);
    end = get_time_us();
    elapsed = (end - start) / 1000.0;

    if (ret != 0) {
        printf("  Error: wc_ecc_make_key_ex failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA Brainpool P-256 keygen");
        return -1;
    }
    printf("  Key generation: %.2f ms\n", elapsed);

    /* Benchmark signing */
    start = get_time_us();
    for (i = 0; i < iterations; i++) {
        sigLen = sizeof(sig);
        ret = wc_ecc_sign_hash(digest, sizeof(digest), sig, &sigLen, &rng, &key);
        if (ret != 0) break;
    }
    end = get_time_us();
    elapsed = (end - start) / 1000.0;

    if (ret != 0) {
        printf("  Error: wc_ecc_sign_hash failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA Brainpool P-256 sign");
        return -1;
    }
    printf("  Signing: %d ops in %.2f ms (%.2f ops/sec)\n",
           iterations, elapsed, iterations / (elapsed / 1000.0));

    /* Benchmark verification */
    start = get_time_us();
    for (i = 0; i < iterations; i++) {
        ret = wc_ecc_verify_hash(sig, sigLen, digest, sizeof(digest), &verified, &key);
        if (ret != 0 || verified != 1) break;
    }
    end = get_time_us();
    elapsed = (end - start) / 1000.0;

    if (ret != 0 || verified != 1) {
        printf("  Error: wc_ecc_verify_hash failed: %d, verified=%d\n", ret, verified);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA Brainpool P-256 verify");
        return -1;
    }
    printf("  Verification: %d ops in %.2f ms (%.2f ops/sec)\n",
           iterations, elapsed, iterations / (elapsed / 1000.0));

    wc_ecc_free(&key);
    wc_FreeRng(&rng);
    TEST_PASS("ECDSA Brainpool P-256 benchmark");
    return 0;
}
#endif /* HAVE_ECC_BRAINPOOL && STSE_CONF_ECC_BRAINPOOL_P_256 */

/*-----------------------------------------------------------------------------
 * Test: ECDSA Brainpool P-384 Benchmark
 *---------------------------------------------------------------------------*/
#if defined(HAVE_ECC_BRAINPOOL) && defined(STSE_CONF_ECC_BRAINPOOL_P_384)
static int test_ecdsa_brainpool_p384_benchmark(void)
{
    ecc_key key;
    WC_RNG rng;
    int ret;
    byte digest[48];
    byte sig[128];
    word32 sigLen;
    int verified = 0;
    int iterations = 10;
    double start, end, elapsed;
    int i;

    printf("\nTest: ECDSA Brainpool P-384 Benchmark (STSAFE-A120)\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("  Error: wc_InitRng failed: %d\n", ret);
        TEST_FAIL("ECDSA Brainpool P-384 benchmark RNG init");
        return -1;
    }

    memset(digest, 0xCD, sizeof(digest));

    ret = wc_ecc_init_ex(&key, NULL, g_devId);
    if (ret != 0) {
        printf("  Error: wc_ecc_init_ex failed: %d\n", ret);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA Brainpool P-384 benchmark key init");
        return -1;
    }

    /* Benchmark key generation */
    start = get_time_us();
    ret = wc_ecc_make_key_ex(&rng, 48, &key, ECC_BRAINPOOLP384R1);
    end = get_time_us();
    elapsed = (end - start) / 1000.0;

    if (ret != 0) {
        printf("  Error: wc_ecc_make_key_ex failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA Brainpool P-384 keygen");
        return -1;
    }
    printf("  Key generation: %.2f ms\n", elapsed);

    /* Benchmark signing */
    start = get_time_us();
    for (i = 0; i < iterations; i++) {
        sigLen = sizeof(sig);
        ret = wc_ecc_sign_hash(digest, sizeof(digest), sig, &sigLen, &rng, &key);
        if (ret != 0) break;
    }
    end = get_time_us();
    elapsed = (end - start) / 1000.0;

    if (ret != 0) {
        printf("  Error: wc_ecc_sign_hash failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA Brainpool P-384 sign");
        return -1;
    }
    printf("  Signing: %d ops in %.2f ms (%.2f ops/sec)\n",
           iterations, elapsed, iterations / (elapsed / 1000.0));

    /* Benchmark verification */
    start = get_time_us();
    for (i = 0; i < iterations; i++) {
        ret = wc_ecc_verify_hash(sig, sigLen, digest, sizeof(digest), &verified, &key);
        if (ret != 0 || verified != 1) break;
    }
    end = get_time_us();
    elapsed = (end - start) / 1000.0;

    if (ret != 0 || verified != 1) {
        printf("  Error: wc_ecc_verify_hash failed: %d, verified=%d\n", ret, verified);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA Brainpool P-384 verify");
        return -1;
    }
    printf("  Verification: %d ops in %.2f ms (%.2f ops/sec)\n",
           iterations, elapsed, iterations / (elapsed / 1000.0));

    wc_ecc_free(&key);
    wc_FreeRng(&rng);
    TEST_PASS("ECDSA Brainpool P-384 benchmark");
    return 0;
}
#endif /* HAVE_ECC_BRAINPOOL && STSE_CONF_ECC_BRAINPOOL_P_384 */

/*-----------------------------------------------------------------------------
 * Test: Multiple Key Slots
 *---------------------------------------------------------------------------*/
static int test_multiple_operations(void)
{
    ecc_key keys[3];
    WC_RNG rng;
    int ret;
    byte digest[32];
    byte sigs[3][128];
    word32 sigLens[3];
    int verified;
    int i;

    printf("\nTest: Multiple Sequential Operations\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("  Error: wc_InitRng failed: %d\n", ret);
        TEST_FAIL("Multiple ops RNG init");
        return -1;
    }

    memset(digest, 0xCD, sizeof(digest));

    /* Generate 3 key pairs and sign with each */
    for (i = 0; i < 3; i++) {
        ret = wc_ecc_init_ex(&keys[i], NULL, g_devId);
        if (ret != 0) {
            printf("  Error: wc_ecc_init_ex[%d] failed: %d\n", i, ret);
            while (--i >= 0) wc_ecc_free(&keys[i]);
            wc_FreeRng(&rng);
            TEST_FAIL("Multiple ops key init");
            return -1;
        }

        printf("  Generating key pair %d...\n", i + 1);
        ret = wc_ecc_make_key_ex(&rng, 32, &keys[i], ECC_SECP256R1);
        if (ret != 0) {
            printf("  Error: wc_ecc_make_key_ex[%d] failed: %d\n", i, ret);
            for (int j = 0; j <= i; j++) wc_ecc_free(&keys[j]);
            wc_FreeRng(&rng);
            TEST_FAIL("Multiple ops keygen");
            return -1;
        }

        /* Sign with this key */
        sigLens[i] = sizeof(sigs[i]);
        ret = wc_ecc_sign_hash(digest, sizeof(digest), sigs[i], &sigLens[i], &rng, &keys[i]);
        if (ret != 0) {
            printf("  Error: wc_ecc_sign_hash[%d] failed: %d\n", i, ret);
            for (int j = 0; j <= i; j++) wc_ecc_free(&keys[j]);
            wc_FreeRng(&rng);
            TEST_FAIL("Multiple ops sign");
            return -1;
        }
        printf("    Key %d: Generated and signed successfully\n", i + 1);
    }

    /* Verify all signatures */
    printf("  Verifying all signatures...\n");
    for (i = 0; i < 3; i++) {
        ret = wc_ecc_verify_hash(sigs[i], sigLens[i], digest, sizeof(digest),
                                  &verified, &keys[i]);
        if (ret != 0 || verified != 1) {
            printf("  Error: Verification[%d] failed: ret=%d, verified=%d\n",
                   i, ret, verified);
            for (int j = 0; j < 3; j++) wc_ecc_free(&keys[j]);
            wc_FreeRng(&rng);
            TEST_FAIL("Multiple ops verify");
            return -1;
        }
    }
    printf("    All 3 signatures verified!\n");

    for (i = 0; i < 3; i++) wc_ecc_free(&keys[i]);
    wc_FreeRng(&rng);
    TEST_PASS("Multiple sequential operations");
    return 0;
}

/*-----------------------------------------------------------------------------
 * Main
 *---------------------------------------------------------------------------*/
int main(void)
{
    int ret;

    printf("================================================================\n");
    printf("wolfSSL STSAFE-A120 Full Integration Test Suite\n");
    printf("================================================================\n");

    /* Initialize wolfCrypt */
    ret = wolfCrypt_Init();
    if (ret != 0) {
        printf("Error: wolfCrypt_Init failed: %d\n", ret);
        return 1;
    }

    /* Initialize STSAFE interface */
    printf("\nInitializing STSAFE-A120...\n");
    ret = stsafe_interface_init();
    if (ret != 0) {
        printf("Error: stsafe_interface_init failed: %d\n", ret);
        wolfCrypt_Cleanup();
        return 1;
    }
    printf("STSAFE-A120 initialized successfully.\n");

    /* Register STSAFE crypto callback */
    g_devId = 0x53545341; /* STSA */
    memset(&g_stsafe_ctx, 0, sizeof(g_stsafe_ctx));
    g_stsafe_ctx.devId = g_devId;

    ret = wc_CryptoCb_RegisterDevice(g_devId, wolfSSL_STSAFE_CryptoDevCb,
                                     &g_stsafe_ctx);
    if (ret != 0) {
        printf("Error: wc_CryptoCb_RegisterDevice failed: %d\n", ret);
        wolfCrypt_Cleanup();
        return 1;
    }
    printf("STSAFE crypto callback registered (devId: 0x%08X).\n", g_devId);

    /* Run tests */
    test_rng_benchmark();
    test_ecdsa_p256_benchmark();
#if defined(HAVE_ECC_BRAINPOOL) && defined(STSE_CONF_ECC_BRAINPOOL_P_256)
    test_ecdsa_brainpool_p256_benchmark();
#endif
#if defined(HAVE_ECC_BRAINPOOL) && defined(STSE_CONF_ECC_BRAINPOOL_P_384)
    test_ecdsa_brainpool_p384_benchmark();
#endif

    /* ECDH test - now uses ECDHE ephemeral keys */
    test_ecdh_p256();

    test_multiple_operations();

    /* Cleanup */
    wc_CryptoCb_UnRegisterDevice(g_devId);
    wolfCrypt_Cleanup();

    printf("\n================================================================\n");
    printf("Test Summary: %d passed, %d failed\n", g_passed, g_failed);
    printf("================================================================\n");

    return (g_failed > 0) ? 1 : 0;
}

