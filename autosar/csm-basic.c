/* csm-basic.c
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

/* Minimal AUTOSAR Classic (4.4) CSM example using the wolfSSL AUTOSAR port.
 *
 * Shows the four calls an application SW-C actually needs:
 *   Csm_Init()            - bring up the CSM -> CryIf -> Crypto driver chain
 *   Csm_RandomGenerate()  - DRBG output, used here to produce the CBC IV
 *   Csm_KeyElementSet()   - load the key and IV into the Crypto keystore
 *   Csm_Encrypt/Decrypt() - AES-CBC, single-call and streamed
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
    printf("\ncsm-basic: SKIP\n");
    return 0;
}

#else /* WOLFSSL_AUTOSAR */

#include <wolfssl/wolfcrypt/port/autosar/Csm.h>

/* AES-CBC block size; the port has no padding, so every buffer handed to
 * Csm_Encrypt()/Csm_Decrypt() must be a whole number of blocks. */
#define BLOCK_SIZE 16

/* Keystore slots. Csm_KeyElementSet() addresses a slot directly by keyId, but
 * without REDIRECTION_CONFIG the Crypto driver picks the *first* slot whose
 * element ID and key length match what the job asks for. See
 * csm-key-redirection.c for pinning a job to a specific slot. */
#ifdef REDIRECTION_CONFIG
    /* Redirection pins each key element to one slot, so write to the slots
     * the library was configured with or the job will not find the key. */
    #define KEY_SLOT ((uint32)REDIRECTION_IN1_KEYID)
    #define IV_SLOT  ((uint32)REDIRECTION_IN2_KEYID)
#else
    #define KEY_SLOT 0U
    #define IV_SLOT  1U
#endif

/* Job IDs. A job is bound to one service, so encrypt and decrypt get their
 * own IDs. Any uint32 will do: the driver matches jobId against its job table
 * rather than indexing by it, so MAX_JOBS bounds how many jobs can be open at
 * once, not what they may be numbered. */
#define ENCRYPT_JOB 1U
#define DECRYPT_JOB 2U

static void print_hex(const char* label, const uint8* buf, uint32 len)
{
    uint32 i;

    printf("%-12s", label);
    for (i = 0; i < len; i++) {
        printf("%02x", buf[i]);
    }
    printf(" (%u bytes)\n", (unsigned int)len);
}

/* AES-128-CBC round trip in one call per direction.
 * Returns 0 on success. */
static int singlecall_example(void)
{
    const char* msg = "AUTOSAR CSM demo payload";
    const uint8 key[BLOCK_SIZE] = { /* "0123456789abcdef" */
        '0','1','2','3','4','5','6','7',
        '8','9','a','b','c','d','e','f'
    };

    /* Block-aligned, because the port does no padding for us. */
    uint8  plain[BLOCK_SIZE * 2];
    uint8  iv[BLOCK_SIZE];
    uint8  cipher[sizeof(plain)];
    uint8  recovered[sizeof(plain)];
    uint32 len;

    printf("\n== single-call AES-128-CBC ==\n");

    /* Zero-pad the message out to a whole number of blocks. A real application
     * uses a scheme it can strip again -- CRYPTO_ALGOFAM_PADDING_PKCS7 exists
     * in Csm.h as an enum value but the driver does not implement it. */
    XMEMSET(plain, 0, sizeof(plain));
    XMEMCPY(plain, msg, XSTRLEN(msg));

    /* A fresh random IV per message, straight from the CSM DRBG.
     * *resultLengthPtr is an input here: it is how many bytes you want. */
    len = sizeof(iv);
    if (Csm_RandomGenerate(0U, iv, &len) != E_OK) {
        printf("Csm_RandomGenerate failed\n");
        return -1;
    }

    /* Provision the key material. In a real ECU this happens once, at
     * startup or during key provisioning, not per message. */
    if (Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY,
                key, sizeof(key)) != E_OK) {
        printf("Csm_KeyElementSet (key) failed\n");
        return -1;
    }
    if (Csm_KeyElementSet(IV_SLOT, CRYPTO_KE_CIPHER_IV,
                iv, sizeof(iv)) != E_OK) {
        printf("Csm_KeyElementSet (IV) failed\n");
        return -1;
    }

    print_hex("plaintext:", plain, sizeof(plain));
    print_hex("iv:", iv, sizeof(iv));

    /* CRYPTO_OPERATIONMODE_SINGLECALL is START|UPDATE|FINISH, so the key and
     * IV are fetched from the keystore and the job is torn down in one call.
     *
     * Note: this port does not write back through resultLengthPtr for
     * encrypt/decrypt. For CBC the output is the same length as the input, so
     * track it yourself rather than reading cipherLen afterwards. */
    len = sizeof(cipher);
    if (Csm_Encrypt(ENCRYPT_JOB, CRYPTO_OPERATIONMODE_SINGLECALL,
                plain, sizeof(plain), cipher, &len) != E_OK) {
        printf("Csm_Encrypt failed\n");
        return -1;
    }
    print_hex("ciphertext:", cipher, sizeof(cipher));

    /* The keystore still holds the same key and IV, and SINGLECALL re-reads
     * both, so decrypt needs no re-provisioning. */
    len = sizeof(recovered);
    if (Csm_Decrypt(DECRYPT_JOB, CRYPTO_OPERATIONMODE_SINGLECALL,
                cipher, sizeof(cipher), recovered, &len) != E_OK) {
        printf("Csm_Decrypt failed\n");
        return -1;
    }
    print_hex("recovered:", recovered, sizeof(recovered));

    if (XMEMCMP(plain, recovered, sizeof(plain)) != 0) {
        printf("FAIL: round trip mismatch\n");
        return -1;
    }

    printf("round trip OK\n");
    return 0;
}

/* Same payload, but fed through the job one block at a time using
 * START / UPDATE / FINISH. Returns 0 on success. */
static int streamed_example(void)
{
    const char* msg = "three block AUTOSAR CSM streaming payload";
    const uint8 key[BLOCK_SIZE] = { /* "0123456789abcdef" */
        '0','1','2','3','4','5','6','7',
        '8','9','a','b','c','d','e','f'
    };
    const uint8 iv[BLOCK_SIZE] = { /* "fedcba9876543210" */
        'f','e','d','c','b','a','9','8',
        '7','6','5','4','3','2','1','0'
    };

    uint8  plain[BLOCK_SIZE * 3];
    uint8  cipher[sizeof(plain)];
    uint8  recovered[sizeof(plain)];
    uint32 len;
    int    i;

    printf("\n== streamed AES-128-CBC (START/UPDATE/FINISH) ==\n");

    XMEMSET(plain, 0, sizeof(plain));
    XMEMCPY(plain, msg, XSTRLEN(msg));

    if (Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY,
                key, sizeof(key)) != E_OK ||
        Csm_KeyElementSet(IV_SLOT, CRYPTO_KE_CIPHER_IV,
                iv, sizeof(iv)) != E_OK) {
        printf("Csm_KeyElementSet failed\n");
        return -1;
    }

    /* START allocates the job's AES context and pulls key + IV from the
     * keystore. UPDATE chains CBC blocks. FINISH releases the context, which
     * frees the MAX_JOBS slot for reuse -- skip it and the slot leaks. */
    for (i = 0; i < 3; i++) {
        Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_UPDATE;

        if (i == 0) {
            mode |= CRYPTO_OPERATIONMODE_START;
        }
        if (i == 2) {
            mode |= CRYPTO_OPERATIONMODE_FINISH;
        }

        len = BLOCK_SIZE;
        if (Csm_Encrypt(ENCRYPT_JOB, mode,
                    plain  + (i * BLOCK_SIZE), BLOCK_SIZE,
                    cipher + (i * BLOCK_SIZE), &len) != E_OK) {
            printf("Csm_Encrypt block %d failed\n", i);
            return -1;
        }
    }
    print_hex("ciphertext:", cipher, sizeof(cipher));

    /* Re-set the IV: CBC decryption has to START from the original IV, and
     * the encrypt pass above left the keystore slot untouched, so this is
     * only here to make the dependency explicit. */
    if (Csm_KeyElementSet(IV_SLOT, CRYPTO_KE_CIPHER_IV,
                iv, sizeof(iv)) != E_OK) {
        printf("Csm_KeyElementSet (IV) failed\n");
        return -1;
    }

    for (i = 0; i < 3; i++) {
        Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_UPDATE;

        if (i == 0) {
            mode |= CRYPTO_OPERATIONMODE_START;
        }
        if (i == 2) {
            mode |= CRYPTO_OPERATIONMODE_FINISH;
        }

        len = BLOCK_SIZE;
        if (Csm_Decrypt(DECRYPT_JOB, mode,
                    cipher    + (i * BLOCK_SIZE), BLOCK_SIZE,
                    recovered + (i * BLOCK_SIZE), &len) != E_OK) {
            printf("Csm_Decrypt block %d failed\n", i);
            return -1;
        }
    }

    if (XMEMCMP(plain, recovered, sizeof(plain)) != 0) {
        printf("FAIL: round trip mismatch\n");
        return -1;
    }

    printf("round trip OK\n");
    return 0;
}

int main(void)
{
    Std_VersionInfoType version;
    int ret;

    /* Uncomment (and build wolfSSL with --enable-debug) to see the port's
     * WOLFSSL_MSG / ReportToDET output on failures.
     * wolfSSL_Debugging_ON(); */

    /* Brings up CryIf and the Crypto driver, including the keystore, the
     * job table and the driver mutexes. The config carries the heap hint and
     * the crypto callback device ID; NULL selects the default allocator and
     * software, which is what this example wants (see csm-cryptocb.c for the
     * other case). */
    Csm_Init(NULL);

    XMEMSET(&version, 0, sizeof(version));
    Csm_GetVersionInfo(&version);
    printf("wolfSSL AUTOSAR CSM %d.%d.%d\n",
            version.sw_major_version,
            version.sw_minor_version,
            version.sw_patch_version);

    ret = singlecall_example();
    if (ret == 0) {
        ret = streamed_example();
    }

    if (ret != 0) {
        printf("\ncsm-basic: FAIL\n");
        return 1;
    }

    printf("\ncsm-basic: PASS\n");
    return 0;
}

#endif /* WOLFSSL_AUTOSAR */
