/* csm-errors.c
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

/* Every way the wolfSSL AUTOSAR port says no.
 *
 * The Csm_* API reports a single Std_ReturnType, so E_NOT_OK is all the caller
 * sees. The reason goes to the Development Error Tracer, which in this port
 * means the wolfSSL log: build wolfSSL with --enable-debug and the
 * wolfSSL_Debugging_ON() below turns those messages on. Without it the calls
 * still fail correctly, just silently.
 *
 * Note what is NOT an E_NOT_OK: a MAC that does not match is a successful job
 * with a CRYPTO_E_VER_NOT_OK result. Conflating the two is how a verify failure
 * ends up being treated as a transient error and retried.
 *
 * Requires wolfSSL built with --enable-autosar (or -DWOLFSSL_AUTOSAR).
 */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>

#include <stdio.h>
#include <string.h>

#ifndef WOLFSSL_AUTOSAR

int main(void)
{
    printf("wolfSSL was not built with AUTOSAR support.\n");
    printf("Rebuild wolfSSL with: ./configure --enable-autosar\n");
    /* SKIP and exit 0, like the missing-option paths, so `make
     * check` reports a configuration this example cannot run on
     * rather than failing on it. 77 is automake's skip code, but
     * this directory's check target reads the marker, not the
     * status. */
    printf("\ncsm-errors: SKIP\n");
    return 0;
}

#else /* WOLFSSL_AUTOSAR */

#include <wolfssl/wolfcrypt/port/autosar/Csm.h>
#include <wolfssl/wolfcrypt/logging.h> /* wolfSSL_Debugging_ON */

#define BLOCK_SIZE 16

/* Must match the Crypto driver. */
#ifndef MAX_KEYSTORE
    #define MAX_KEYSTORE 15
#endif

#ifdef REDIRECTION_CONFIG
    #define KEY_SLOT ((uint32)REDIRECTION_IN1_KEYID)
    #define IV_SLOT  ((uint32)REDIRECTION_IN2_KEYID)
#else
    #define KEY_SLOT 0U
    #define IV_SLOT  1U
#endif

static const uint8 key[BLOCK_SIZE] = { /* "0123456789abcdef" */
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};
static const uint8 iv[BLOCK_SIZE] = { /* "1234567890abcdef" */
    '1','2','3','4','5','6','7','8','9','0','a','b','c','d','e','f'
};

static int failures = 0;

/* Records whether a call that was supposed to be rejected actually was. */
static void expect_not_ok(const char* what, Std_ReturnType ret)
{
    if (ret == E_NOT_OK) {
        printf("  rejected  %s\n", what);
    }
    else {
        printf("  ACCEPTED  %s  <-- expected E_NOT_OK\n", what);
        failures++;
    }
}

static void expect_ok(const char* what, Std_ReturnType ret)
{
    if (ret == E_OK) {
        printf("  accepted  %s\n", what);
    }
    else {
        printf("  REJECTED  %s  <-- expected E_OK\n", what);
        failures++;
    }
}

/* Runs first, while the keystore is still empty. */
static void no_key_available(void)
{
    uint8  msg[BLOCK_SIZE];
    uint8  out[BLOCK_SIZE];
    uint32 len = sizeof(out);

    printf("\n== before any key is provisioned ==\n");
    XMEMSET(msg, 'x', sizeof(msg));

    expect_not_ok("Csm_Encrypt with an empty keystore",
        Csm_Encrypt(1U, CRYPTO_OPERATIONMODE_SINGLECALL, msg, BLOCK_SIZE,
            out, &len));
}

static void key_element_set_arguments(void)
{
    uint8 oversized[64];

    printf("\n== Csm_KeyElementSet argument checks ==\n");
    XMEMSET(oversized, 'k', sizeof(oversized));

    expect_not_ok("NULL key pointer",
        Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY, NULL, BLOCK_SIZE));

    expect_not_ok("zero key length",
        Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY, key, 0));

    /* slots are 0 .. MAX_KEYSTORE-1 */
    expect_not_ok("keyId past the end of the keystore",
        Csm_KeyElementSet((uint32)MAX_KEYSTORE, CRYPTO_KE_CIPHER_KEY,
            key, BLOCK_SIZE));

    /* a slot holds at most one AES-256 key */
    expect_not_ok("key longer than a keystore slot",
        Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY,
            oversized, (uint32)sizeof(oversized)));
}

static void streaming_misuse(void)
{
    uint8  msg[BLOCK_SIZE];
    uint8  out[BLOCK_SIZE];
    uint32 len;

    printf("\n== streaming out of order ==\n");
    XMEMSET(msg, 'x', sizeof(msg));

    /* provision properly first, so the failures below are about the mode
     * sequence and not about missing key material */
    expect_ok("provisioning the key",
        Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY, key, BLOCK_SIZE));
    expect_ok("provisioning the IV",
        Csm_KeyElementSet(IV_SLOT, CRYPTO_KE_CIPHER_IV, iv, BLOCK_SIZE));

    /* UPDATE needs the context that START allocates */
    len = sizeof(out);
    expect_not_ok("UPDATE without START",
        Csm_Encrypt(20U, CRYPTO_OPERATIONMODE_UPDATE, msg, BLOCK_SIZE,
            out, &len));

    /* likewise FINISH has nothing to release */
    len = sizeof(out);
    expect_not_ok("FINISH without START",
        Csm_Encrypt(21U, CRYPTO_OPERATIONMODE_FINISH, msg, 0, out, &len));
}

static void unusable_key_length(void)
{
    uint8  odd[20];
    uint8  msg[BLOCK_SIZE];
    uint8  out[BLOCK_SIZE];
    uint32 len = sizeof(out);

    printf("\n== a stored key that AES cannot use ==\n");
    XMEMSET(odd, 'k', sizeof(odd));
    XMEMSET(msg, 'x', sizeof(msg));

    /* 20 bytes fits a keystore slot, so this call succeeds... */
    expect_ok("storing a 20 byte key",
        Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY,
            odd, (uint32)sizeof(odd)));

    /* ...and the job that picks it up is where it fails. The keystore does
     * not police key lengths; the driver does, when it builds the AES key. */
    expect_not_ok("encrypting with a 20 byte key",
        Csm_Encrypt(22U, CRYPTO_OPERATIONMODE_SINGLECALL, msg, BLOCK_SIZE,
            out, &len));

    /* put a usable key back */
    expect_ok("restoring a 16 byte key",
        Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY, key, BLOCK_SIZE));
}

static void random_arguments(void)
{
    uint32 len = BLOCK_SIZE;

    printf("\n== Csm_RandomGenerate argument checks ==\n");

    expect_not_ok("NULL output buffer",
        Csm_RandomGenerate(0U, NULL, &len));
}

#ifdef WOLFSSL_AUTOSAR_CMAC
static void mac_cases(void)
{
    uint8  msg[BLOCK_SIZE];
    uint8  mac[BLOCK_SIZE];
    uint8  small[4];
    uint32 len;
    Crypto_VerifyResultType verify;

    printf("\n== MAC services ==\n");
    XMEMSET(msg, 'x', sizeof(msg));

    expect_ok("provisioning the MAC key",
        Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_MAC_KEY, key, BLOCK_SIZE));

    len = sizeof(mac);
    expect_not_ok("Csm_MacGenerate with a NULL MAC buffer",
        Csm_MacGenerate(30U, CRYPTO_OPERATIONMODE_SINGLECALL, msg, BLOCK_SIZE,
            NULL, &len));

    /* the buffer has to hold the whole tag */
    len = (uint32)sizeof(small);
    expect_not_ok("Csm_MacGenerate into a 4 byte buffer",
        Csm_MacGenerate(31U, CRYPTO_OPERATIONMODE_SINGLECALL, msg, BLOCK_SIZE,
            small, &len));

    len = sizeof(mac);
    expect_not_ok("Csm_MacVerify with a NULL verify result pointer",
        Csm_MacVerify(32U, CRYPTO_OPERATIONMODE_SINGLECALL, msg, BLOCK_SIZE,
            mac, BLOCK_SIZE * 8, NULL));

    /* Now the case that is NOT an error. Generate a real tag, corrupt it, and
     * verify: the job succeeds and reports the mismatch out of band. */
    len = sizeof(mac);
    expect_ok("Csm_MacGenerate producing a tag",
        Csm_MacGenerate(33U, CRYPTO_OPERATIONMODE_SINGLECALL, msg, BLOCK_SIZE,
            mac, &len));

    mac[0] ^= 0xFF;
    verify = CRYPTO_E_VER_OK;
    expect_ok("Csm_MacVerify on a corrupted tag",
        Csm_MacVerify(34U, CRYPTO_OPERATIONMODE_SINGLECALL, msg, BLOCK_SIZE,
            mac, BLOCK_SIZE * 8, &verify));

    if (verify == CRYPTO_E_VER_NOT_OK) {
        printf("  and reported CRYPTO_E_VER_NOT_OK, not E_NOT_OK\n");
    }
    else {
        printf("  but did NOT report CRYPTO_E_VER_NOT_OK  <-- wrong\n");
        failures++;
    }
}
#endif /* WOLFSSL_AUTOSAR_CMAC */

int main(void)
{
    /* No-op unless wolfSSL was built with --enable-debug. With it, each
     * rejection below is accompanied by the driver's reason. */
    wolfSSL_Debugging_ON();

    Csm_Init(NULL);

    no_key_available();
    key_element_set_arguments();
    streaming_misuse();
    unusable_key_length();
    random_arguments();
#ifdef WOLFSSL_AUTOSAR_CMAC
    mac_cases();
#else
    printf("\n== MAC services ==\n");
    printf("  skipped, build wolfSSL with --enable-autosar-cmac\n");
#endif

    if (failures != 0) {
        printf("\n%d case(s) behaved unexpectedly\n", failures);
        printf("\ncsm-errors: FAIL\n");
        return 1;
    }

    printf("\ncsm-errors: PASS\n");
    return 0;
}

#endif /* WOLFSSL_AUTOSAR */
