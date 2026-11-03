/* csm-jobs.c
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

/* AUTOSAR CSM job lifecycle with the wolfSSL AUTOSAR port.
 *
 * A streaming job holds state between calls, and that state is keyed on the
 * jobId. Two things follow, and neither is obvious from the Csm_* prototypes:
 *
 *   - Two different jobIds stream independently. Their UPDATE calls can be
 *     interleaved in any order without corrupting either stream, which is what
 *     lets two SW-Cs share the CSM.
 *
 *   - A job's context is allocated on START and released on FINISH, out of a
 *     fixed table of MAX_JOBS entries. Walk away from a stream without calling
 *     FINISH and that entry is gone for the life of the ECU.
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
    printf("\ncsm-jobs: SKIP\n");
    return 0;
}

#else /* WOLFSSL_AUTOSAR */

#include <wolfssl/wolfcrypt/port/autosar/Csm.h>

#define BLOCK_SIZE 16
#define BLOCKS     3

/* Must match the Crypto driver. Override both here and in the wolfSSL build
 * if you raise it. */
#ifndef MAX_JOBS
    #define MAX_JOBS 10
#endif

#ifdef REDIRECTION_CONFIG
    #define KEY_SLOT ((uint32)REDIRECTION_IN1_KEYID)
    #define IV_SLOT  ((uint32)REDIRECTION_IN2_KEYID)
#else
    #define KEY_SLOT 0U
    #define IV_SLOT  1U
#endif

#define JOB_A 1U
#define JOB_B 2U

static const uint8 key[BLOCK_SIZE] = { /* "0123456789abcdef" */
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};
static const uint8 iv[BLOCK_SIZE] = { /* "1234567890abcdef" */
    '1','2','3','4','5','6','7','8','9','0','a','b','c','d','e','f'
};

/* Reloads the key and IV. Every START re-reads both from the keystore, so
 * this is what pins a stream to a known IV. */
static int provision(void)
{
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
    return 0;
}

/* Encrypts the whole message in one call, for comparison. */
static int reference(const uint8* msg, uint8* out)
{
    uint32 len = BLOCK_SIZE * BLOCKS;

    if (provision() != 0) {
        return -1;
    }
    if (Csm_Encrypt(JOB_A, CRYPTO_OPERATIONMODE_SINGLECALL,
                msg, BLOCK_SIZE * BLOCKS, out, &len) != E_OK) {
        printf("reference Csm_Encrypt failed\n");
        return -1;
    }
    return 0;
}

/* Runs two streams block by block, alternating between them, and checks each
 * against its single-call reference. Returns 0 on success. */
static int interleaved_test(void)
{
    uint8 msgA[BLOCK_SIZE * BLOCKS];
    uint8 msgB[BLOCK_SIZE * BLOCKS];
    uint8 refA[BLOCK_SIZE * BLOCKS];
    uint8 refB[BLOCK_SIZE * BLOCKS];
    uint8 outA[BLOCK_SIZE * BLOCKS];
    uint8 outB[BLOCK_SIZE * BLOCKS];
    uint32 len;
    int i;

    printf("\n== two jobs streaming interleaved ==\n");

    /* two distinguishable payloads */
    XMEMSET(msgA, 'A', sizeof(msgA));
    XMEMSET(msgB, 'B', sizeof(msgB));

    if (reference(msgA, refA) != 0 || reference(msgB, refB) != 0) {
        return -1;
    }

    if (provision() != 0) {
        return -1;
    }

    /* Both jobs START against the same key and IV. They still produce
     * independent CBC chains, because the chain state lives in the job. */
    for (i = 0; i < BLOCKS; i++) {
        Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_UPDATE;

        if (i == 0) {
            mode |= CRYPTO_OPERATIONMODE_START;
        }
        if (i == BLOCKS - 1) {
            mode |= CRYPTO_OPERATIONMODE_FINISH;
        }

        /* job A block i */
        len = BLOCK_SIZE;
        if (Csm_Encrypt(JOB_A, mode, msgA + (i * BLOCK_SIZE), BLOCK_SIZE,
                    outA + (i * BLOCK_SIZE), &len) != E_OK) {
            printf("job A block %d failed\n", i);
            return -1;
        }

        /* then job B block i, in between A's calls */
        len = BLOCK_SIZE;
        if (Csm_Encrypt(JOB_B, mode, msgB + (i * BLOCK_SIZE), BLOCK_SIZE,
                    outB + (i * BLOCK_SIZE), &len) != E_OK) {
            printf("job B block %d failed\n", i);
            return -1;
        }

        printf("  block %d: job %u, then job %u\n", i,
                (unsigned int)JOB_A, (unsigned int)JOB_B);
    }

    if (XMEMCMP(outA, refA, sizeof(refA)) != 0) {
        printf("job A stream does not match its single-call reference\n");
        return -1;
    }
    if (XMEMCMP(outB, refB, sizeof(refB)) != 0) {
        printf("job B stream does not match its single-call reference\n");
        return -1;
    }

    printf("both streams match their single-call references\n");
    return 0;
}

/* Opens streams until the job table is full, shows the next START failing,
 * then reclaims one slot with FINISH and shows it working again.
 * Returns 0 on success. */
static int exhaustion_test(void)
{
    const uint32 firstJob = 100U;
    uint8  msg[BLOCK_SIZE];
    uint8  out[BLOCK_SIZE];
    uint32 len;
    uint32 i;
    int    ret = -1;
    uint32 opened = 0;

    printf("\n== job table exhaustion (MAX_JOBS = %d) ==\n", MAX_JOBS);

    XMEMSET(msg, 'x', sizeof(msg));

    if (provision() != 0) {
        return -1;
    }

    /* Start MAX_JOBS streams and deliberately leave them open. */
    for (i = 0; i < (uint32)MAX_JOBS; i++) {
        len = sizeof(out);
        if (Csm_Encrypt(firstJob + i,
                    CRYPTO_OPERATIONMODE_START | CRYPTO_OPERATIONMODE_UPDATE,
                    msg, BLOCK_SIZE, out, &len) != E_OK) {
            printf("unexpected failure opening job %u of %d\n",
                    (unsigned int)(i + 1), MAX_JOBS);
            goto cleanup;
        }
        opened++;
    }
    printf("  opened %u streams without calling FINISH\n",
            (unsigned int)opened);

    /* One more must fail: there is no slot left to allocate. */
    len = sizeof(out);
    if (Csm_Encrypt(firstJob + (uint32)MAX_JOBS,
                CRYPTO_OPERATIONMODE_START | CRYPTO_OPERATIONMODE_UPDATE,
                msg, BLOCK_SIZE, out, &len) == E_OK) {
        printf("job %d started even though the table should be full\n",
                MAX_JOBS + 1);
        goto cleanup;
    }
    printf("  stream %d refused, as expected\n", MAX_JOBS + 1);

    /* FINISH one stream, which releases its slot... */
    len = sizeof(out);
    if (Csm_Encrypt(firstJob, CRYPTO_OPERATIONMODE_FINISH,
                msg, 0, out, &len) != E_OK) {
        printf("failed to FINISH the first stream\n");
        goto cleanup;
    }
    opened--;

    /* ...and the same START now succeeds. */
    len = sizeof(out);
    if (Csm_Encrypt(firstJob + (uint32)MAX_JOBS,
                CRYPTO_OPERATIONMODE_START | CRYPTO_OPERATIONMODE_UPDATE,
                msg, BLOCK_SIZE, out, &len) != E_OK) {
        printf("slot was not reclaimed by FINISH\n");
        goto cleanup;
    }
    printf("  after one FINISH, a new stream starts again\n");

    ret = 0;

cleanup:
    /* Close everything still open so the table is clean for the next test. */
    for (i = 0; i <= (uint32)MAX_JOBS; i++) {
        len = sizeof(out);
        (void)Csm_Encrypt(firstJob + i, CRYPTO_OPERATIONMODE_FINISH,
                msg, 0, out, &len);
    }
    return ret;
}

int main(void)
{
    int ret;

    Csm_Init(NULL);

    ret = interleaved_test();
    if (ret == 0) {
        ret = exhaustion_test();
    }

    if (ret != 0) {
        printf("\ncsm-jobs: FAIL\n");
        return 1;
    }

    printf("\ncsm-jobs: PASS\n");
    return 0;
}

#endif /* WOLFSSL_AUTOSAR */
