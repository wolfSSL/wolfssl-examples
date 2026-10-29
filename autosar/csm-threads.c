/* csm-threads.c
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

/* Several AUTOSAR SW-Cs using the CSM at the same time.
 *
 * The Crypto driver keeps two pieces of shared state -- the keystore and the
 * job table -- and guards each with its own mutex. What that buys you, and
 * what it does not:
 *
 *   Safe:   any number of threads running jobs concurrently, as long as each
 *           job ID has one owner. Claiming and releasing job slots, and
 *           reading keys out of the keystore, are serialized internally.
 *
 *   Safe:   provisioning keys with Csm_KeyElementSet() while jobs are running.
 *           The driver copies the key out under the keystore lock, so a job
 *           never reads a key that is being rewritten underneath it.
 *
 *   Unsafe: two threads driving the SAME job ID. A streaming job is a
 *           conversation -- START, UPDATE, FINISH -- and AUTOSAR gives each
 *           job a single owner. Two threads sharing one job ID will interleave
 *           their UPDATEs into one cipher context and corrupt both streams.
 *
 * Note also that replacing key material with DIFFERENT bytes while a job is
 * mid-stream is not a data race but is still a logic error: the job's START
 * captured the old key, so the message it produces cannot be decrypted with
 * the new one. Rotate keys between messages, not during them.
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
    printf("\ncsm-threads: SKIP\n");
    return 0;
}

#elif defined(SINGLE_THREADED)

int main(void)
{
    printf("wolfSSL was built SINGLE_THREADED; nothing to demonstrate.\n");
    printf("\ncsm-threads: SKIP\n");
    return 0;
}

#else /* WOLFSSL_AUTOSAR && !SINGLE_THREADED */

#include <wolfssl/wolfcrypt/port/autosar/Csm.h>
#include <pthread.h>

#define BLOCK_SIZE 16

/* Must match the Crypto driver. Every concurrently *streaming* job holds one
 * slot from START to FINISH, so MAX_JOBS has to be at least the number of
 * streams an ECU can have open at once. Single-call jobs claim and release
 * within the call, so they only need a slot for the duration of the call. */
#ifndef MAX_JOBS
    #define MAX_JOBS 10
#endif

#define NUM_WORKERS 4
#define ROUNDS      400

#ifdef REDIRECTION_CONFIG
    #define KEY_SLOT ((uint32)REDIRECTION_IN1_KEYID)
    #define IV_SLOT  ((uint32)REDIRECTION_IN2_KEYID)
#else
    #define KEY_SLOT 0U
    #define IV_SLOT  1U
#endif

/* Each worker owns two job IDs, so no ID is ever driven by two threads. */
#define ENC_JOB(w) ((uint32)(10 + (w)))
#define DEC_JOB(w) ((uint32)(20 + (w)))

static const uint8 key[BLOCK_SIZE] = { /* "0123456789abcdef" */
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};
static const uint8 iv[BLOCK_SIZE] = { /* "1234567890abcdef" */
    '1','2','3','4','5','6','7','8','9','0','a','b','c','d','e','f'
};

/* Worker outcome. Written only by its own thread, read after join. */
struct worker {
    int      id;
    long     roundTrips;
    long     errors;     /* a Csm_* call reported E_NOT_OK */
    long     mismatches; /* round trip produced the wrong plaintext */
};

/* 'volatile' is not synchronization -- it orders nothing between threads and
 * ThreadSanitizer rightly flags it. The stop flag gets a mutex like any other
 * piece of shared state. */
static pthread_mutex_t stopLock = PTHREAD_MUTEX_INITIALIZER;
static int stopProvisioner = 0;

static int should_stop(void)
{
    int stop;

    pthread_mutex_lock(&stopLock);
    stop = stopProvisioner;
    pthread_mutex_unlock(&stopLock);
    return stop;
}

static void set_stop(int value)
{
    pthread_mutex_lock(&stopLock);
    stopProvisioner = value;
    pthread_mutex_unlock(&stopLock);
}

static int provision_key(void)
{
    if (Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY,
                key, (uint32)sizeof(key)) != E_OK) {
        return -1;
    }
    if (Csm_KeyElementSet(IV_SLOT, CRYPTO_KE_CIPHER_IV,
                iv, (uint32)sizeof(iv)) != E_OK) {
        return -1;
    }
    return 0;
}

/* Rewrites the same key material over and over while the workers run. This is
 * what used to race: the driver handed out a pointer into the keystore and
 * dropped the lock before the caller had used it. */
static void* provisioner(void* arg)
{
    long* err = (long*)arg;

    while (!should_stop()) {
        int i;

        for (i = 0; i < 64; i++) {
            if (provision_key() != 0) {
                (*err)++;
                return NULL;
            }
        }
    }
    return NULL;
}

/* Single-call AES-CBC round trips on this worker's own job IDs. */
static void* worker_singlecall(void* arg)
{
    struct worker* w = (struct worker*)arg;
    uint8  plain[BLOCK_SIZE];
    uint8  cipher[BLOCK_SIZE];
    uint8  recovered[BLOCK_SIZE];
    uint32 len;
    int    i;

    XMEMSET(plain, (int)('A' + w->id), sizeof(plain));

    for (i = 0; i < ROUNDS; i++) {
        len = (uint32)sizeof(cipher);
        if (Csm_Encrypt(ENC_JOB(w->id), CRYPTO_OPERATIONMODE_SINGLECALL,
                    plain, BLOCK_SIZE, cipher, &len) != E_OK) {
            w->errors++;
            continue;
        }

        len = (uint32)sizeof(recovered);
        if (Csm_Decrypt(DEC_JOB(w->id), CRYPTO_OPERATIONMODE_SINGLECALL,
                    cipher, BLOCK_SIZE, recovered, &len) != E_OK) {
            w->errors++;
            continue;
        }

        if (XMEMCMP(plain, recovered, BLOCK_SIZE) != 0) {
            w->mismatches++;
        }
        else {
            w->roundTrips++;
        }
    }
    return NULL;
}

/* Streamed round trips. Each worker holds a job slot open across three
 * UPDATEs, so NUM_WORKERS slots are in use at once. */
static void* worker_streaming(void* arg)
{
    struct worker* w = (struct worker*)arg;
    uint8  plain[BLOCK_SIZE * 3];
    uint8  cipher[sizeof(plain)];
    uint8  recovered[sizeof(plain)];
    uint32 len;
    int    i, b;

    XMEMSET(plain, (int)('a' + w->id), sizeof(plain));

    for (i = 0; i < ROUNDS / 4; i++) {
        int failed = 0;

        for (b = 0; b < 3; b++) {
            Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_UPDATE;

            if (b == 0) {
                mode |= CRYPTO_OPERATIONMODE_START;
            }
            if (b == 2) {
                mode |= CRYPTO_OPERATIONMODE_FINISH;
            }

            len = BLOCK_SIZE;
            if (Csm_Encrypt(ENC_JOB(w->id), mode,
                        plain + (b * BLOCK_SIZE), BLOCK_SIZE,
                        cipher + (b * BLOCK_SIZE), &len) != E_OK) {
                w->errors++;
                failed = 1;
                break;
            }
        }
        if (failed) {
            continue;
        }

        for (b = 0; b < 3; b++) {
            Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_UPDATE;

            if (b == 0) {
                mode |= CRYPTO_OPERATIONMODE_START;
            }
            if (b == 2) {
                mode |= CRYPTO_OPERATIONMODE_FINISH;
            }

            len = BLOCK_SIZE;
            if (Csm_Decrypt(DEC_JOB(w->id), mode,
                        cipher + (b * BLOCK_SIZE), BLOCK_SIZE,
                        recovered + (b * BLOCK_SIZE), &len) != E_OK) {
                w->errors++;
                failed = 1;
                break;
            }
        }
        if (failed) {
            continue;
        }

        if (XMEMCMP(plain, recovered, sizeof(plain)) != 0) {
            w->mismatches++;
        }
        else {
            w->roundTrips++;
        }
    }
    return NULL;
}

#ifdef WOLFSSL_AUTOSAR_CMAC
/* Concurrent MAC generate and verify, on their own job IDs again. */
static void* worker_mac(void* arg)
{
    struct worker* w = (struct worker*)arg;
    uint8  msg[BLOCK_SIZE * 2];
    uint8  mac[BLOCK_SIZE];
    uint32 len;
    int    i;

    XMEMSET(msg, (int)('0' + w->id), sizeof(msg));

    for (i = 0; i < ROUNDS; i++) {
        Crypto_VerifyResultType verify = CRYPTO_E_VER_NOT_OK;

        len = (uint32)sizeof(mac);
        if (Csm_MacGenerate(ENC_JOB(w->id), CRYPTO_OPERATIONMODE_SINGLECALL,
                    msg, (uint32)sizeof(msg), mac, &len) != E_OK) {
            w->errors++;
            continue;
        }

        /* len came back from Csm_MacGenerate() in BYTES; Csm_MacVerify()
         * takes BITS, as the specification defines each. */
        if (Csm_MacVerify(DEC_JOB(w->id), CRYPTO_OPERATIONMODE_SINGLECALL,
                    msg, (uint32)sizeof(msg), mac, len * 8, &verify)
                != E_OK) {
            w->errors++;
            continue;
        }

        if (verify != CRYPTO_E_VER_OK) {
            w->mismatches++;
        }
        else {
            w->roundTrips++;
        }
    }
    return NULL;
}
#endif /* WOLFSSL_AUTOSAR_CMAC */

/* Runs one phase: NUM_WORKERS threads plus a key provisioner.
 * Returns 0 if every worker came back clean. */
static int run_phase(const char* name, void* (*fn)(void*), long expected)
{
    pthread_t     tid[NUM_WORKERS];
    pthread_t     provTid;
    struct worker w[NUM_WORKERS];
    long provErrors = 0;
    long trips = 0, errors = 0, mismatches = 0;
    int i, ret = 0;

    printf("\n== %s ==\n", name);

    XMEMSET(w, 0, sizeof(w));
    set_stop(0);

    if (pthread_create(&provTid, NULL, provisioner, &provErrors) != 0) {
        printf("  could not start the provisioner thread\n");
        return -1;
    }

    for (i = 0; i < NUM_WORKERS; i++) {
        w[i].id = i;
        if (pthread_create(&tid[i], NULL, fn, &w[i]) != 0) {
            printf("  could not start worker %d\n", i);
            ret = -1;
            /* still join what did start */
            break;
        }
    }

    {
        int started = i;

        for (i = 0; i < started; i++) {
            pthread_join(tid[i], NULL);
        }
    }

    set_stop(1);
    pthread_join(provTid, NULL);

    for (i = 0; i < NUM_WORKERS; i++) {
        trips      += w[i].roundTrips;
        errors     += w[i].errors;
        mismatches += w[i].mismatches;
    }

    printf("  %d workers, %ld successful round trips\n", NUM_WORKERS, trips);
    if (errors != 0) {
        printf("  %ld Csm_* call(s) reported E_NOT_OK\n", errors);
        ret = -1;
    }
    if (mismatches != 0) {
        printf("  %ld result(s) came back wrong\n", mismatches);
        ret = -1;
    }
    if (provErrors != 0) {
        printf("  %ld key provisioning failure(s)\n", provErrors);
        ret = -1;
    }
    if (trips != expected) {
        printf("  expected %ld round trips, got %ld\n", expected, trips);
        ret = -1;
    }
    if (ret == 0) {
        printf("  clean, with key provisioning running throughout\n");
    }
    return ret;
}

int main(void)
{
    int ret;

    Csm_Init(NULL);

    printf("CSM under concurrent use\n");
    printf("  workers           %d\n", NUM_WORKERS);
    printf("  rounds per worker %d\n", ROUNDS);
    printf("  MAX_JOBS          %d (need >= %d for the streaming phase)\n",
            MAX_JOBS, NUM_WORKERS);

    if (NUM_WORKERS > MAX_JOBS) {
        printf("\nMAX_JOBS is too small for %d concurrent streams;\n"
               "rebuild wolfSSL with -DMAX_JOBS=%d or more.\n",
                NUM_WORKERS, NUM_WORKERS);
        return 1;
    }

    if (provision_key() != 0) {
        printf("Csm_KeyElementSet failed\n");
        return 1;
    }

    ret = run_phase("single-call jobs", worker_singlecall,
            (long)NUM_WORKERS * ROUNDS);

    if (ret == 0) {
        ret = run_phase("streaming jobs, one slot held per worker",
                worker_streaming, (long)NUM_WORKERS * (ROUNDS / 4));
    }

#ifdef WOLFSSL_AUTOSAR_CMAC
    if (ret == 0) {
        ret = run_phase("MAC generate and verify", worker_mac,
                (long)NUM_WORKERS * ROUNDS);
    }
#else
    if (ret == 0) {
        printf("\n== MAC generate and verify ==\n");
        printf("  skipped, build wolfSSL with --enable-autosar-cmac\n");
    }
#endif

    if (ret != 0) {
        printf("\ncsm-threads: FAIL\n");
        return 1;
    }

    printf("\ncsm-threads: PASS\n");
    return 0;
}

#endif /* WOLFSSL_AUTOSAR && !SINGLE_THREADED */
