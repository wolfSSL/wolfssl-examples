/* stsafe_test.c
 *
 * STSAFE-A120 test program for wolfSSL integration
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

/* Include STSELib */
#include "stselib.h"

/* STSAFE-A120 default I2C bus */
#define STSAFE_A120_I2C_BUS     1

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_PASS(msg) do { printf("[PASS] %s\n", msg); tests_passed++; } while(0)
#define TEST_FAIL(msg) do { printf("[FAIL] %s\n", msg); tests_failed++; } while(0)

/* STSAFE device handler */
static stse_Handler_t stsafe_handler;

/*--------------------- Test: Echo Command --------------------------- */
static int test_echo(void)
{
    stse_ReturnCode_t ret;
    uint8_t echo_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t response[sizeof(echo_data)];

    printf("\nTest: Echo Command\n");
    printf("  Sending %zu bytes...\n", sizeof(echo_data));

    memset(response, 0, sizeof(response));
    ret = stse_device_echo(&stsafe_handler, echo_data, response, sizeof(echo_data));

    if (ret != STSE_OK) {
        printf("  Error: stse_device_echo returned 0x%04X\n", ret);
        TEST_FAIL("Echo command failed");
        return -1;
    }

    if (memcmp(echo_data, response, sizeof(echo_data)) != 0) {
        printf("  Error: Response data mismatch\n");
        printf("  Expected: ");
        for (size_t i = 0; i < sizeof(echo_data); i++) printf("%02X ", echo_data[i]);
        printf("\n  Got:      ");
        for (size_t i = 0; i < sizeof(response); i++) printf("%02X ", response[i]);
        printf("\n");
        TEST_FAIL("Echo data mismatch");
        return -1;
    }

    printf("  Echo response matches!\n");
    TEST_PASS("Echo command");
    return 0;
}

/*--------------------- Test: Random Number Generation --------------------------- */
static int test_random(void)
{
    stse_ReturnCode_t ret;
    uint8_t random_data[32];
    int all_zeros = 1;

    printf("\nTest: Random Number Generation\n");
    printf("  Requesting 32 random bytes...\n");

    memset(random_data, 0, sizeof(random_data));

    ret = stse_generate_random(&stsafe_handler, random_data, sizeof(random_data));

    if (ret != STSE_OK) {
        printf("  Error: stse_generate_random returned 0x%04X\n", ret);
        TEST_FAIL("Random generation failed");
        return -1;
    }

    /* Check that data is not all zeros */
    for (size_t i = 0; i < sizeof(random_data); i++) {
        if (random_data[i] != 0) {
            all_zeros = 0;
            break;
        }
    }

    if (all_zeros) {
        printf("  Error: Random data is all zeros\n");
        TEST_FAIL("Random data is all zeros");
        return -1;
    }

    printf("  Random data: ");
    for (size_t i = 0; i < 16; i++) {
        printf("%02X ", random_data[i]);
    }
    printf("...\n");

    TEST_PASS("Random number generation");
    return 0;
}

/*--------------------- Test: ECC Key Generation (P-256) --------------------------- */
static int test_ecc_keygen_p256(void)
{
    stse_ReturnCode_t ret;
    uint8_t public_key[64]; /* P-256 public key: 32 bytes X + 32 bytes Y */
    uint8_t slot = 1; /* Key slot 1 */
    stse_ecc_key_type_t key_type = STSE_ECC_KT_NIST_P_256;

    printf("\nTest: ECC Key Generation (P-256)\n");

    memset(public_key, 0, sizeof(public_key));

    /* usage_limit = 255 for limited use */
    ret = stse_generate_ecc_key_pair(&stsafe_handler,
                                      slot,
                                      key_type,
                                      255, /* usage_limit */
                                      public_key);

    if (ret != STSE_OK) {
        printf("  Error: stse_generate_ecc_key_pair returned 0x%04X\n", ret);
        TEST_FAIL("ECC P-256 key generation failed");
        return -1;
    }

    printf("  Public Key X: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X", public_key[i]);
    }
    printf("...\n");

    printf("  Public Key Y: ");
    for (int i = 32; i < 40; i++) {
        printf("%02X", public_key[i]);
    }
    printf("...\n");

    TEST_PASS("ECC P-256 key generation");
    return 0;
}

/*--------------------- Test: ECDSA Sign (P-256) --------------------------- */
static int test_ecdsa_sign_p256(void)
{
    stse_ReturnCode_t ret;
    uint8_t digest[32]; /* SHA-256 digest */
    uint8_t signature[64]; /* P-256 signature: 32 bytes R + 32 bytes S */
    uint8_t public_key[64];
    uint8_t slot = 2; /* Key slot 2 for signing test */
    stse_ecc_key_type_t key_type = STSE_ECC_KT_NIST_P_256;

    printf("\nTest: ECDSA Sign (P-256)\n");

    /* Create a test digest */
    memset(digest, 0xAB, sizeof(digest));
    memset(signature, 0, sizeof(signature));
    memset(public_key, 0, sizeof(public_key));

    /* First generate a key pair */
    ret = stse_generate_ecc_key_pair(&stsafe_handler,
                                      slot,
                                      key_type,
                                      255, /* usage_limit */
                                      public_key);
    if (ret != STSE_OK) {
        printf("  Error: Key generation failed with 0x%04X\n", ret);
        TEST_FAIL("ECDSA sign - key generation failed");
        return -1;
    }
    printf("  Key pair generated.\n");

    /* Sign the digest */
    ret = stse_ecc_generate_signature(&stsafe_handler,
                                       slot,
                                       key_type,
                                       digest, sizeof(digest),
                                       signature);

    if (ret != STSE_OK) {
        printf("  Error: stse_ecc_generate_signature returned 0x%04X\n", ret);
        TEST_FAIL("ECDSA P-256 sign failed");
        return -1;
    }

    printf("  Signature R: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X", signature[i]);
    }
    printf("...\n");

    printf("  Signature S: ");
    for (int i = 32; i < 40; i++) {
        printf("%02X", signature[i]);
    }
    printf("...\n");

    TEST_PASS("ECDSA P-256 sign");
    return 0;
}

/*--------------------- Test: ECC Key Generation (P-384) --------------------------- */
static int test_ecc_keygen_p384(void)
{
    stse_ReturnCode_t ret;
    uint8_t public_key[96]; /* P-384 public key: 48 bytes X + 48 bytes Y */
    uint8_t slot = 3; /* Key slot 3 for P-384 */
    stse_ecc_key_type_t key_type = STSE_ECC_KT_NIST_P_384;

    printf("\nTest: ECC Key Generation (P-384)\n");

    memset(public_key, 0, sizeof(public_key));

    ret = stse_generate_ecc_key_pair(&stsafe_handler,
                                      slot,
                                      key_type,
                                      255, /* usage_limit */
                                      public_key);

    if (ret != STSE_OK) {
        printf("  Error: stse_generate_ecc_key_pair returned 0x%04X\n", ret);
        TEST_FAIL("ECC P-384 key generation failed");
        return -1;
    }

    printf("  Public Key X: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X", public_key[i]);
    }
    printf("...\n");

    printf("  Public Key Y: ");
    for (int i = 48; i < 56; i++) {
        printf("%02X", public_key[i]);
    }
    printf("...\n");

    TEST_PASS("ECC P-384 key generation");
    return 0;
}

/*--------------------- Main --------------------------- */
int main(int argc, char *argv[])
{
    stse_ReturnCode_t ret;

    (void)argc;
    (void)argv;

    printf("================================================\n");
    printf("STSAFE-A120 Test Suite for wolfSSL Integration\n");
    printf("================================================\n");

    /* Initialize STSAFE handler with defaults */
    printf("\nInitializing STSAFE handler...\n");
    ret = stse_set_default_handler_value(&stsafe_handler);
    if (ret != STSE_OK) {
        printf("Error: stse_set_default_handler_value failed: 0x%04X\n", ret);
        return 1;
    }

    /* Configure for STSAFE-A120 on I2C bus 1 */
    stsafe_handler.device_type = STSAFE_A120;
    stsafe_handler.io.busID = STSAFE_A120_I2C_BUS;
    stsafe_handler.io.BusSpeed = 100; /* 100 kHz for reliability */

    printf("Initializing STSAFE-A120 device on I2C bus %d...\n", STSAFE_A120_I2C_BUS);
    ret = stse_init(&stsafe_handler);
    if (ret != STSE_OK) {
        printf("Error: stse_init failed: 0x%04X\n", ret);
        return 1;
    }
    printf("STSAFE-A120 initialized successfully.\n");

    /* Run tests */
    test_echo();
    test_random();
    test_ecc_keygen_p256();
    test_ecdsa_sign_p256();
    test_ecc_keygen_p384();

    /* Print summary */
    printf("\n================================================\n");
    printf("Test Summary: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("================================================\n");

    return (tests_failed > 0) ? 1 : 0;
}
