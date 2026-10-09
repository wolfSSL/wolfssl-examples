/* csm-key-redirection.c
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

/* AUTOSAR CSM key input redirection with the wolfSSL AUTOSAR port.
 *
 * The Crypto driver has two ways of deciding which keystore slot a job uses:
 *
 *   default (no REDIRECTION_CONFIG)
 *       The driver scans the keystore from slot 0 and takes the first slot
 *       whose key element ID and key length match what the job asks for.
 *       Whatever landed in the lowest slot wins -- the job has no say.
 *
 *   redirected (REDIRECTION_CONFIG defined)
 *       Each key element ID is pinned to one specific slot, chosen at
 *       compile time. Other slots holding the same element ID are ignored.
 *
 * This example loads a decoy key into a low slot and the real key into a
 * higher one, encrypts a known-answer vector, and reports which key the
 * driver actually used. Build it both ways to see the two behaviours.
 *
 * IMPORTANT: the redirection macros are compiled into the wolfSSL *library*
 * (csm.c and crypto.c), not into this application. Setting them here alone
 * changes nothing -- wolfSSL itself must be rebuilt with them. See README.md.
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
    printf("\ncsm-key-redirection: SKIP\n");
    return 0;
}

#else /* WOLFSSL_AUTOSAR */

#include <wolfssl/wolfcrypt/port/autosar/Csm.h>

#define BLOCK_SIZE 16
#define ENCRYPT_JOB 1U

/* Keystore slots used below.
 *
 * DECOY_KEY_SLOT is deliberately lower than REAL_KEY_SLOT: in the default
 * build the driver's first-match scan hits the decoy, in the redirected build
 * it is skipped. The IV is written to both candidate slots so that the key is
 * the only thing that differs between the two builds. */
#define DECOY_KEY_SLOT     0U
#define REAL_KEY_SLOT      1U  /* expected REDIRECTION_IN1_KEYID */
#define IV_SLOT_FIRSTMATCH 2U  /* first CRYPTO_KE_CIPHER_IV slot scanned */
#define IV_SLOT_REDIRECTED 4U  /* expected REDIRECTION_IN2_KEYID */

#ifdef REDIRECTION_CONFIG
    /* Catch an application built against a library configured with different
     * slot assignments -- the symptom would otherwise be a silent wrong-key
     * encryption. */
    #if !defined(REDIRECTION_IN1_KEYID) || \
            REDIRECTION_IN1_KEYID != REAL_KEY_SLOT
        #error "REDIRECTION_IN1_KEYID must be 1 to match this example"
    #endif
    /* 0x01 spelled out rather than CRYPTO_KE_CIPHER_KEY: the key element IDs
     * are enum constants, and the preprocessor evaluates an enum name in #if
     * as 0. The runtime check in main() covers the enum itself. */
    #if !defined(REDIRECTION_IN1_KEYELMID) || REDIRECTION_IN1_KEYELMID != 0x01
        #error "REDIRECTION_IN1_KEYELMID must be 0x01 (CRYPTO_KE_CIPHER_KEY)"
    #endif
    #if !defined(REDIRECTION_IN2_KEYID) || \
            REDIRECTION_IN2_KEYID != IV_SLOT_REDIRECTED
        #error "REDIRECTION_IN2_KEYID must be 4 to match this example"
    #endif
    #if !defined(REDIRECTION_IN2_KEYELMID) || REDIRECTION_IN2_KEYELMID != 0x05
        #error "REDIRECTION_IN2_KEYELMID must be 0x05 (CRYPTO_KE_CIPHER_IV)"
    #endif
#endif

/* Known-answer vector: AES-128-CBC, key "0123456789abcdef",
 * IV "1234567890abcdef", one block of plaintext. Same vector the port's own
 * test uses, so a match here means the real key was picked up. */
static const uint8 realKey[BLOCK_SIZE] = { /* "0123456789abcdef" */
    '0','1','2','3','4','5','6','7',
    '8','9','a','b','c','d','e','f'
};
static const uint8 iv[BLOCK_SIZE] = { /* "1234567890abcdef" */
    '1','2','3','4','5','6','7','8',
    '9','0','a','b','c','d','e','f'
};
static const uint8 plain[BLOCK_SIZE] = {
    0x6e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74, /* "now is the time " */
    0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20
};
static const uint8 expected[BLOCK_SIZE] = {
    0x95,0x94,0x92,0x57,0x5f,0x42,0x81,0x53,
    0x2c,0xcc,0x9d,0x46,0x77,0xa2,0x33,0xcb
};

/* A different 128-bit key, so the decoy produces a different ciphertext. */
static const uint8 decoyKey[BLOCK_SIZE] = { /* "DECOY-KEY-NOT-OK" */
    'D','E','C','O','Y','-','K','E',
    'Y','-','N','O','T','-','O','K'
};
/* AES-128-CBC(decoyKey, iv) over the same plaintext. Known, rather than
 * inferred from "not the real key's output": otherwise a corrupted or
 * truncated result, or encryption under some third key, would be classified
 * as the decoy and the no-redirection path would report that as success. */
static const uint8 decoyExpected[BLOCK_SIZE] = {
    0x9d,0x2f,0x0c,0x66,0x39,0xde,0x49,0xf7,
    0xa6,0x74,0x13,0xf6,0xa9,0xc1,0x8e,0x5f
};

static void print_hex(const char* label, const uint8* buf, uint32 len)
{
    uint32 i;

    printf("%-16s", label);
    for (i = 0; i < len; i++) {
        printf("%02x", buf[i]);
    }
    printf("\n");
}

/* Loads the keystore. withDecoy == 0 leaves the decoy out, which is how the
 * ambiguity re-check below repeats the same setup with nothing to be
 * ambiguous about -- it must provision exactly what the real path does and no
 * more, or a job failing for some unrelated reason would be papered over.
 * Returns 0 on success. */
static int provision_keys(int withDecoy)
{
    /* Decoy first, in the lowest slot. */
    if (withDecoy && Csm_KeyElementSet(DECOY_KEY_SLOT, CRYPTO_KE_CIPHER_KEY,
                decoyKey, BLOCK_SIZE) != E_OK) {
        printf("Csm_KeyElementSet (decoy key) failed\n");
        return -1;
    }

    /* The key this job is supposed to use. */
    if (Csm_KeyElementSet(REAL_KEY_SLOT, CRYPTO_KE_CIPHER_KEY,
                realKey, BLOCK_SIZE) != E_OK) {
        printf("Csm_KeyElementSet (real key) failed\n");
        return -1;
    }

    /* Same IV in both candidate slots. */
    if (Csm_KeyElementSet(IV_SLOT_FIRSTMATCH, CRYPTO_KE_CIPHER_IV,
                iv, BLOCK_SIZE) != E_OK ||
        Csm_KeyElementSet(IV_SLOT_REDIRECTED, CRYPTO_KE_CIPHER_IV,
                iv, BLOCK_SIZE) != E_OK) {
        printf("Csm_KeyElementSet (IV) failed\n");
        return -1;
    }

    return 0;
}

/* Loads the keystore and encrypts one block.
 * Returns 0 if the real key was used, 1 if the decoy was, 2 if the driver
 * refused to choose between them, -1 on error. */
static int which_key_was_used(uint8* cipher)
{
    uint32 len;

    if (provision_keys(1) != 0) {
        return -1;
    }

    len = BLOCK_SIZE;
    if (Csm_Encrypt(ENCRYPT_JOB, CRYPTO_OPERATIONMODE_SINGLECALL,
                plain, BLOCK_SIZE, cipher, &len) != E_OK) {
        /* Two slots hold key element 0x01, and a library built with the MAC
         * services refuses that rather than taking the first: 0x01 is also
         * CRYPTO_KE_MAC_KEY, and the driver cannot tell which service a slot
         * belongs to.
         *
         * Only that one configuration can produce the refusal -- the
         * ambiguity check lives in the non-redirection branch of GetKeyRef().
         * Anywhere else an encrypt failure is a real failure, and reporting it
         * as expected would hide the library/application mismatch this example
         * exists to catch. */
#if defined(WOLFSSL_AUTOSAR_CMAC) && !defined(REDIRECTION_CONFIG)
        return 2;
#else
        printf("Csm_Encrypt failed\n");
        return -1;
#endif
    }

    if (XMEMCMP(cipher, expected, BLOCK_SIZE) == 0) {
        return 0; /* the real key */
    }
    if (XMEMCMP(cipher, decoyExpected, BLOCK_SIZE) == 0) {
        return 1; /* the decoy */
    }

    printf("Ciphertext matches neither key's known vector.\n");
    print_hex("got:", cipher, BLOCK_SIZE);
    print_hex("real key:", expected, BLOCK_SIZE);
    print_hex("decoy key:", decoyExpected, BLOCK_SIZE);
    return -1;
}

int main(void)
{
    uint8 cipher[BLOCK_SIZE];
    int used;

    Csm_Init(NULL);

    XMEMSET(cipher, 0, sizeof(cipher));

#ifdef REDIRECTION_CONFIG
    printf("built with REDIRECTION_CONFIG = 0x%02x\n",
            (unsigned int)REDIRECTION_CONFIG);
    printf("  CRYPTO_KE_CIPHER_KEY pinned to keystore slot %u\n",
            (unsigned int)REDIRECTION_IN1_KEYID);
    printf("  CRYPTO_KE_CIPHER_IV  pinned to keystore slot %u\n",
            (unsigned int)REDIRECTION_IN2_KEYID);

    /* The #if guards above compare against literals, so confirm here that
     * those literals really are the enum values they stand for. */
    if (REDIRECTION_IN1_KEYELMID != CRYPTO_KE_CIPHER_KEY ||
        REDIRECTION_IN2_KEYELMID != CRYPTO_KE_CIPHER_IV) {
        printf("key element ID enums do not match the redirection macros\n");
        printf("\ncsm-key-redirection: FAIL\n");
        return 1;
    }
#else
    printf("built without REDIRECTION_CONFIG\n");
    printf("  key selection is first-matching-slot\n");
#endif

    printf("\nkeystore slot %u: decoy key\n", (unsigned int)DECOY_KEY_SLOT);
    printf("keystore slot %u: real key\n", (unsigned int)REAL_KEY_SLOT);

    used = which_key_was_used(cipher);
    if (used < 0) {
        printf("\ncsm-key-redirection: FAIL\n");
        return 1;
    }

    if (used == 2) {
        uint32 len = BLOCK_SIZE;

        printf("\nThe driver refused the job: two slots hold key element"
               " 0x01, which is\nboth CRYPTO_KE_CIPHER_KEY and"
               " CRYPTO_KE_MAC_KEY, so it will not guess which\nservice a"
               " slot belongs to. That is a libwolfssl built with"
               " --enable-autosar-cmac.\n");

        /* Confirm that is really what happened, rather than the job failing
         * for some other reason: start from one key in one slot and the same
         * job must now succeed, under the real key. */
        Csm_Init(NULL);
        if (provision_keys(0) != 0) {
            printf("\ncsm-key-redirection: FAIL\n");
            return 1;
        }
        XMEMSET(cipher, 0, sizeof(cipher));
        if (Csm_Encrypt(ENCRYPT_JOB, CRYPTO_OPERATIONMODE_SINGLECALL, plain,
                    BLOCK_SIZE, cipher, &len) != E_OK) {
            printf("The same job failed with one unambiguous key, so the"
                   " refusal above was\nnot the ambiguity check.\n");
            printf("\ncsm-key-redirection: FAIL\n");
            return 1;
        }
        if (XMEMCMP(cipher, expected, BLOCK_SIZE) != 0) {
            printf("One key in the keystore, and still not the expected"
                   " ciphertext.\n");
            printf("\ncsm-key-redirection: FAIL\n");
            return 1;
        }
        printf("With the decoy gone the same job succeeds under the real"
               " key.\n");

        printf("\nName the slot with wolfSSL_Csm_EncryptWithKey(), which is"
               " unambiguous\nwhatever the build, or define"
               " WOLFSSL_AUTOSAR_ALLOW_AMBIGUOUS_KEY_ELEMENT for\nthe ECU"
               " that only ever uses one of the two services.\n");
        printf("\ncsm-key-redirection: PASS\n");
        return 0;
    }

    printf("\n");
    print_hex("ciphertext:", cipher, sizeof(cipher));
    print_hex("real key gives:", expected, sizeof(expected));
    printf("\ndriver used the %s key\n", (used == 0) ? "REAL" : "DECOY");

#ifdef REDIRECTION_CONFIG
    if (used != 0) {
        printf("\nUnexpected: redirection is configured but slot %u was not"
               " used.\nIs libwolfssl built with the same redirection macros"
               " as this example?\n", (unsigned int)REAL_KEY_SLOT);
        printf("\ncsm-key-redirection: FAIL\n");
        return 1;
    }
    printf("Redirection pinned the job to slot %u and ignored the decoy in"
           " slot %u.\n",
            (unsigned int)REAL_KEY_SLOT, (unsigned int)DECOY_KEY_SLOT);
#else
    if (used == 0) {
        printf("\nUnexpected: no redirection configured, yet the decoy in the"
               " lower slot\nwas not picked up. Is libwolfssl built with"
               " REDIRECTION_CONFIG after all?\n");
        printf("\ncsm-key-redirection: FAIL\n");
        return 1;
    }
    printf("The decoy in slot %u won because it was the first matching slot.\n"
           "This is why a keystore holding more than one key per element ID\n"
           "needs redirection -- rebuild wolfSSL with REDIRECTION_CONFIG to\n"
           "pin this job to slot %u.\n",
            (unsigned int)DECOY_KEY_SLOT, (unsigned int)REAL_KEY_SLOT);
#endif

    printf("\ncsm-key-redirection: PASS\n");
    return 0;
}

#endif /* WOLFSSL_AUTOSAR */
