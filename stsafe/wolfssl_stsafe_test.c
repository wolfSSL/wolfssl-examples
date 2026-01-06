/* wolfssl_stsafe_test.c
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

/* wolfSSL headers */
/* Must include options.h to match libwolfssl build settings */
#include <wolfssl/options.h>
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

/*-----------------------------------------------------------------------------
 * Test: RNG with STSAFE-A120
 *---------------------------------------------------------------------------*/
static int test_rng(void)
{
    WC_RNG rng;
    byte rand_buf[32];
    int ret;
    int i;

    printf("\nTest: RNG with STSAFE-A120\n");

    ret = wc_InitRng_ex(&rng, NULL, g_devId);
    if (ret != 0) {
        printf("  Error: wc_InitRng_ex failed: %d\n", ret);
        TEST_FAIL("RNG initialization");
        return -1;
    }

    /* Generate random bytes */
    ret = wc_RNG_GenerateBlock(&rng, rand_buf, sizeof(rand_buf));
    if (ret != 0) {
        printf("  Error: wc_RNG_GenerateBlock failed: %d\n", ret);
        wc_FreeRng(&rng);
        TEST_FAIL("RNG generation");
        return -1;
    }

    /* Print random data */
    printf("  Random data: ");
    for (i = 0; i < 16; i++) {
        printf("%02X ", rand_buf[i]);
    }
    printf("...\n");

    wc_FreeRng(&rng);
    TEST_PASS("RNG with STSAFE-A120");
    return 0;
}

/*-----------------------------------------------------------------------------
 * Test: ECC P-256 Key Generation with STSAFE-A120
 *---------------------------------------------------------------------------*/
static int test_ecc_keygen_p256(void)
{
    ecc_key key;
    WC_RNG rng;
    int ret;
    byte pubX[32], pubY[32];
    word32 pubX_len = sizeof(pubX), pubY_len = sizeof(pubY);
    int i;

    printf("\nTest: ECC P-256 Key Generation with STSAFE-A120\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("  Error: wc_InitRng failed: %d\n", ret);
        TEST_FAIL("ECC P-256 RNG init");
        return -1;
    }

    ret = wc_ecc_init_ex(&key, NULL, g_devId);
    if (ret != 0) {
        printf("  Error: wc_ecc_init_ex failed: %d\n", ret);
        wc_FreeRng(&rng);
        TEST_FAIL("ECC P-256 init");
        return -1;
    }

    /* Generate P-256 key pair - should use STSAFE via crypto callback */
    ret = wc_ecc_make_key_ex(&rng, 32, &key, ECC_SECP256R1);
    if (ret != 0) {
        printf("  Error: wc_ecc_make_key_ex failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECC P-256 key generation");
        return -1;
    }

    /* Export public key */
    ret = wc_ecc_export_public_raw(&key, pubX, &pubX_len, pubY, &pubY_len);
    if (ret != 0) {
        printf("  Error: wc_ecc_export_public_raw failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECC P-256 export public key");
        return -1;
    }

    printf("  Public Key X: ");
    for (i = 0; i < 8; i++) printf("%02X", pubX[i]);
    printf("...\n");
    printf("  Public Key Y: ");
    for (i = 0; i < 8; i++) printf("%02X", pubY[i]);
    printf("...\n");

    wc_ecc_free(&key);
    wc_FreeRng(&rng);
    TEST_PASS("ECC P-256 key generation");
    return 0;
}

/*-----------------------------------------------------------------------------
 * Test: ECC P-384 Key Generation with STSAFE-A120
 *---------------------------------------------------------------------------*/
static int test_ecc_keygen_p384(void)
{
    ecc_key key;
    WC_RNG rng;
    int ret;
    byte pubX[48], pubY[48];
    word32 pubX_len = sizeof(pubX), pubY_len = sizeof(pubY);
    int i;

    printf("\nTest: ECC P-384 Key Generation with STSAFE-A120\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("  Error: wc_InitRng failed: %d\n", ret);
        TEST_FAIL("ECC P-384 RNG init");
        return -1;
    }

    ret = wc_ecc_init_ex(&key, NULL, g_devId);
    if (ret != 0) {
        printf("  Error: wc_ecc_init_ex failed: %d\n", ret);
        wc_FreeRng(&rng);
        TEST_FAIL("ECC P-384 init");
        return -1;
    }

    /* Generate P-384 key pair - should use STSAFE via crypto callback */
    ret = wc_ecc_make_key_ex(&rng, 48, &key, ECC_SECP384R1);
    if (ret != 0) {
        printf("  Error: wc_ecc_make_key_ex failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECC P-384 key generation");
        return -1;
    }

    /* Export public key */
    ret = wc_ecc_export_public_raw(&key, pubX, &pubX_len, pubY, &pubY_len);
    if (ret != 0) {
        printf("  Error: wc_ecc_export_public_raw failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECC P-384 export public key");
        return -1;
    }

    printf("  Public Key X: ");
    for (i = 0; i < 8; i++) printf("%02X", pubX[i]);
    printf("...\n");
    printf("  Public Key Y: ");
    for (i = 0; i < 8; i++) printf("%02X", pubY[i]);
    printf("...\n");

    wc_ecc_free(&key);
    wc_FreeRng(&rng);
    TEST_PASS("ECC P-384 key generation");
    return 0;
}

/*-----------------------------------------------------------------------------
 * Test: ECDSA Sign/Verify P-256 with STSAFE-A120
 *---------------------------------------------------------------------------*/
static int test_ecdsa_p256(void)
{
    ecc_key key;
    WC_RNG rng;
    int ret;
    byte digest[32];
    byte sig[128];
    word32 sigLen = sizeof(sig);
    int verified = 0;
    int i;

    printf("\nTest: ECDSA P-256 Sign/Verify with STSAFE-A120\n");

    /* Initialize RNG */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("  Error: wc_InitRng failed: %d\n", ret);
        TEST_FAIL("ECDSA P-256 RNG init");
        return -1;
    }

    /* Create test digest */
    memset(digest, 0xAB, sizeof(digest));

    ret = wc_ecc_init_ex(&key, NULL, g_devId);
    if (ret != 0) {
        printf("  Error: wc_ecc_init_ex failed: %d\n", ret);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-256 key init");
        return -1;
    }

    /* Generate key pair using STSAFE */
    ret = wc_ecc_make_key_ex(&rng, 32, &key, ECC_SECP256R1);
    if (ret != 0) {
        printf("  Error: wc_ecc_make_key_ex failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-256 key generation");
        return -1;
    }

    printf("  Key pair generated.\n");

    /* Sign the digest using STSAFE */
    ret = wc_ecc_sign_hash(digest, sizeof(digest), sig, &sigLen, &rng, &key);
    if (ret != 0) {
        printf("  Error: wc_ecc_sign_hash failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-256 sign");
        return -1;
    }

    printf("  Signature (%d bytes): ", sigLen);
    for (i = 0; i < 8 && i < (int)sigLen; i++) printf("%02X", sig[i]);
    printf("...\n");

    /* Verify the signature */
    ret = wc_ecc_verify_hash(sig, sigLen, digest, sizeof(digest), &verified, &key);
    if (ret != 0) {
        printf("  Error: wc_ecc_verify_hash failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-256 verify");
        return -1;
    }

    if (verified != 1) {
        printf("  Error: Signature verification failed\n");
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-256 verification result");
        return -1;
    }

    printf("  Signature verified!\n");

    wc_ecc_free(&key);
    wc_FreeRng(&rng);
    TEST_PASS("ECDSA P-256 sign/verify");
    return 0;
}

/*-----------------------------------------------------------------------------
 * Test: ECDSA Sign/Verify P-384 with STSAFE-A120
 *---------------------------------------------------------------------------*/
static int test_ecdsa_p384(void)
{
    ecc_key key;
    WC_RNG rng;
    int ret;
    byte digest[48];
    byte sig[128];
    word32 sigLen = sizeof(sig);
    int verified = 0;
    int i;

    printf("\nTest: ECDSA P-384 Sign/Verify with STSAFE-A120\n");

    /* Initialize RNG */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("  Error: wc_InitRng failed: %d\n", ret);
        TEST_FAIL("ECDSA P-384 RNG init");
        return -1;
    }

    /* Create test digest */
    memset(digest, 0xCD, sizeof(digest));

    ret = wc_ecc_init_ex(&key, NULL, g_devId);
    if (ret != 0) {
        printf("  Error: wc_ecc_init_ex failed: %d\n", ret);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-384 key init");
        return -1;
    }

    /* Generate key pair using STSAFE */
    ret = wc_ecc_make_key_ex(&rng, 48, &key, ECC_SECP384R1);
    if (ret != 0) {
        printf("  Error: wc_ecc_make_key_ex failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-384 key generation");
        return -1;
    }

    printf("  Key pair generated.\n");

    /* Sign the digest using STSAFE */
    ret = wc_ecc_sign_hash(digest, sizeof(digest), sig, &sigLen, &rng, &key);
    if (ret != 0) {
        printf("  Error: wc_ecc_sign_hash failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-384 sign");
        return -1;
    }

    printf("  Signature (%d bytes): ", sigLen);
    for (i = 0; i < 8 && i < (int)sigLen; i++) printf("%02X", sig[i]);
    printf("...\n");

    /* Verify the signature */
    ret = wc_ecc_verify_hash(sig, sigLen, digest, sizeof(digest), &verified, &key);
    if (ret != 0) {
        printf("  Error: wc_ecc_verify_hash failed: %d\n", ret);
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-384 verify");
        return -1;
    }

    if (verified != 1) {
        printf("  Error: Signature verification failed\n");
        wc_ecc_free(&key);
        wc_FreeRng(&rng);
        TEST_FAIL("ECDSA P-384 verification result");
        return -1;
    }

    printf("  Signature verified!\n");

    wc_ecc_free(&key);
    wc_FreeRng(&rng);
    TEST_PASS("ECDSA P-384 sign/verify");
    return 0;
}

/*-----------------------------------------------------------------------------
 * Main
 *---------------------------------------------------------------------------*/
int main(void)
{
    int ret;

    printf("================================================\n");
    printf("wolfSSL STSAFE-A120 Crypto Callback Test Suite\n");
    printf("================================================\n");

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
    test_rng();
    test_ecc_keygen_p256();
    test_ecc_keygen_p384();
    test_ecdsa_p256();
    test_ecdsa_p384();

    /* Cleanup */
    wc_CryptoCb_UnRegisterDevice(g_devId);
    wolfCrypt_Cleanup();

    printf("\n================================================\n");
    printf("Test Summary: %d passed, %d failed\n", g_passed, g_failed);
    printf("================================================\n");

    return (g_failed > 0) ? 1 : 0;
}

