/* csm-cryptocb.c
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

/* Backing the AUTOSAR Crypto driver with hardware.
 *
 * The CSM/CryIf/Crypto split exists so that the bottom layer can be a real
 * driver for an HSM, SHE block or accelerator. In this port that happens
 * through a wolfCrypt crypto callback: put its device ID in Csm_ConfigType and
 * every context the driver creates carries it, so the work is offered to your
 * callback instead of being done in software.
 *
 * This example is the skeleton of that callback. What it gives you is the part
 * that is genuinely hard to discover: which operations arrive for each Csm_*
 * call, in what order, with what parameters, and how to accept some and decline
 * others. Where your HSM SDK call belongs is marked; there is no pretend
 * hardware here, because a fake would teach the wrong shape.
 *
 * The required order is worth noting, because getting it wrong fails in a
 * confusing way:
 *
 *     wolfCrypt_Init();                                  <-- not optional
 *     wc_CryptoCb_RegisterDevice(devId, callback, ctx);
 *     config.devId = devId;
 *     Csm_Init(&config);
 *
 * Free slots in the crypto callback device table are marked with INVALID_DEVID,
 * and nothing sets that up until wolfCrypt_Init() runs -- skip it and
 * registration fails with BUFFER_E, "out of devices".
 *
 * Requires wolfSSL built with --enable-autosar --enable-cryptocb
 * (add --enable-autosar-cmac to see the MAC operations).
 */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>

#include <stdio.h>
#include <string.h>

#if !defined(WOLFSSL_AUTOSAR)

int main(void)
{
    printf("wolfSSL was not built with AUTOSAR support.\n");
    printf("Rebuild wolfSSL with: ./configure --enable-autosar\n");
    /* SKIP and exit 0, like the missing-option paths, so `make
     * check` reports a configuration this example cannot run on
     * rather than failing on it. 77 is automake's skip code, but
     * this directory's check target reads the marker, not the
     * status. */
    printf("\ncsm-cryptocb: SKIP\n");
    return 0;
}

#elif !defined(WOLF_CRYPTO_CB)

int main(void)
{
    printf("This example needs wolfSSL's crypto callbacks.\n");
    printf("Rebuild wolfSSL with: "
           "./configure --enable-autosar --enable-cryptocb\n");
    printf("\ncsm-cryptocb: SKIP\n");
    return 0;
}

#else /* WOLFSSL_AUTOSAR && WOLF_CRYPTO_CB */

#include <wolfssl/wolfcrypt/port/autosar/Csm.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define BLOCK_SIZE 16

/* Whatever your driver registers. Any value but INVALID_DEVID. */
#define HSM_DEVID 42

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

static void check(const char* what, int ok)
{
    printf("  %-46s %s\n", what, ok ? "ok" : "FAILED");
    if (!ok) {
        failures++;
    }
}

/* ---- the driver's own state ----------------------------------------- */

/* What the callback did, so the example can show it. A real driver keeps its
 * session handles and SDK context here instead. */
struct driver_state {
    int  cipherOps;     /* AES-CBC encrypt/decrypt offered */
    int  setKeyOps;     /* key installs offered (see below) */
    int  macOps;        /* CMAC offered */
    int  rngOps;        /* random / seed offered */
    int  hashOps;       /* hash / HMAC, from inside the DRBG */
    int  otherOps;
    int  badDevId;

    /* which operations this driver claims. A real one reports what the part
     * can actually do. */
    int  doCipher;
    int  doMac;
    int  doRng;

    /* set to make the next claimed cipher operation fail, to show the error
     * path from hardware back up to the Csm_* return value */
    int  failNextCipher;

    /* trace of the last run, with repeats collapsed */
    char trace[16][48];
    int  traceCount[16];
    int  traceLen;
};

/* Repeated identical operations collapse into one entry with a count. The
 * DRBG alone offers dozens of hash operations, which would otherwise fill the
 * trace and hide everything else. */
static void trace_add(struct driver_state* st, const char* what)
{
    int i;

    for (i = 0; i < st->traceLen; i++) {
        if (strcmp(st->trace[i], what) == 0) {
            st->traceCount[i]++;
            return;
        }
    }
    if (st->traceLen < (int)(sizeof(st->trace) / sizeof(st->trace[0]))) {
        strncpy(st->trace[st->traceLen], what, sizeof(st->trace[0]) - 1);
        st->trace[st->traceLen][sizeof(st->trace[0]) - 1] = '\0';
        st->traceCount[st->traceLen] = 1;
        st->traceLen++;
    }
}

static void trace_print(struct driver_state* st, const char* title)
{
    int i;

    printf("  %s\n", title);
    if (st->traceLen == 0) {
        printf("      (nothing reached the driver)\n");
    }
    for (i = 0; i < st->traceLen; i++) {
        if (st->traceCount[i] > 1) {
            printf("      %-36s x%d\n", st->trace[i], st->traceCount[i]);
        }
        else {
            printf("      %s\n", st->trace[i]);
        }
    }
    st->traceLen = 0;
}

/* The Crypto driver callback.
 *
 * Return 0 when the driver handled the operation, CRYPTOCB_UNAVAILABLE to let
 * wolfCrypt do it in software, or a wolfCrypt error to fail the job. That third
 * case is what surfaces as E_NOT_OK from Csm_Encrypt() and friends. */
static int hsmCryptoCb(int devId, wc_CryptoInfo* info, void* ctx)
{
    struct driver_state* st = (struct driver_state*)ctx;

    if (st == NULL || info == NULL) {
        return CRYPTOCB_UNAVAILABLE;
    }
    if (devId != HSM_DEVID) {
        /* one callback can serve several device IDs; a real driver switches on
         * this to pick the right part */
        st->badDevId++;
    }

    /* Register and unregister notifications arrive as WC_ALGO_TYPE_NONE, but
     * only in a library built with WOLF_CRYPTO_CB_CMD -- and wolfCrypt accepts
     * CRYPTOCB_UNAVAILABLE for them, so a callback that ignores them still
     * registers. Claimed here because a driver that needs to open and close a
     * session has somewhere obvious to do it. */
    if (info->algo_type == WC_ALGO_TYPE_NONE) {
        return 0;
    }

    switch (info->algo_type) {

    case WC_ALGO_TYPE_CIPHER:
        /* Two different operations arrive here. WC_CIPHER_AES is the key
         * install, and only when wolfSSL was built with
         * WOLF_CRYPTO_CB_AES_SETKEY -- see the note at the end of this file.
         * WC_CIPHER_AES_CBC is the actual encrypt or decrypt. */
        if (info->cipher.type == WC_CIPHER_AES) {
            st->setKeyOps++;
            trace_add(st, "AES set key      (WC_CIPHER_AES)");
            /* >>> your SDK: install the key into a hardware slot, keep the
             * handle against info->cipher.aessetkey.aes <<< */
            return CRYPTOCB_UNAVAILABLE;
        }
        if (info->cipher.type == WC_CIPHER_AES_CBC) {
            st->cipherOps++;
            trace_add(st, info->cipher.enc ?
                    "AES-CBC encrypt  (WC_CIPHER_AES_CBC)" :
                    "AES-CBC decrypt  (WC_CIPHER_AES_CBC)");

            if (!st->doCipher) {
                return CRYPTOCB_UNAVAILABLE; /* software does it */
            }
            if (st->failNextCipher) {
                st->failNextCipher = 0;
                trace_add(st, "  -> driver reports a failure");
                return WC_HW_E; /* becomes E_NOT_OK at the Csm_* API */
            }
            /* >>> your SDK: run AES-CBC on the part, using
             * info->cipher.aescbc.{aes,out,in,sz} and info->cipher.enc.
             * Returning 0 here claims the operation. <<<
             *
             * This example has no hardware, so it declines and lets wolfCrypt
             * do the arithmetic. That keeps the results correct and the
             * plumbing honest. */
            return CRYPTOCB_UNAVAILABLE;
        }
        st->otherOps++;
        trace_add(st, "other cipher operation");
        return CRYPTOCB_UNAVAILABLE;

#ifdef WOLFSSL_AUTOSAR_CMAC
    case WC_ALGO_TYPE_CMAC:
        st->macOps++;
        trace_add(st, "AES-CMAC         (WC_ALGO_TYPE_CMAC)");
        if (!st->doMac) {
            return CRYPTOCB_UNAVAILABLE;
        }
        /* >>> your SDK: CMAC on the part <<< */
        return CRYPTOCB_UNAVAILABLE;
#endif

    case WC_ALGO_TYPE_RNG:
    case WC_ALGO_TYPE_SEED:
        st->rngOps++;
        trace_add(st, (info->algo_type == WC_ALGO_TYPE_SEED) ?
                "entropy seed     (WC_ALGO_TYPE_SEED)" :
                "random block     (WC_ALGO_TYPE_RNG)");
        if (!st->doRng) {
            return CRYPTOCB_UNAVAILABLE;
        }
        /* >>> your SDK: read the hardware TRNG <<< */
        return CRYPTOCB_UNAVAILABLE;

    case WC_ALGO_TYPE_HASH:
        /* Not something any Csm_* call asks for directly. These come from
         * inside the DRBG, which is built on SHA-256 -- a single
         * Csm_RandomGenerate() offers dozens of them. Worth knowing before
         * you decide to offload hashing to a part that is slower per call
         * than software. */
        st->hashOps++;
        trace_add(st, "hash             (DRBG internals)");
        return CRYPTOCB_UNAVAILABLE;

    case WC_ALGO_TYPE_HMAC:
        st->hashOps++;
        trace_add(st, "HMAC             (WC_ALGO_TYPE_HMAC)");
        return CRYPTOCB_UNAVAILABLE;

    default:
        st->otherOps++;
        trace_add(st, "operation this driver does not know");
        return CRYPTOCB_UNAVAILABLE;
    }
}

/* ---- exercising it -------------------------------------------------- */

static int provision(void)
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

/* one AES-CBC round trip. Returns 0 if it succeeded and round tripped. */
static int cbc_roundtrip(void)
{
    uint8  plain[BLOCK_SIZE];
    uint8  cipher[BLOCK_SIZE];
    uint8  recovered[BLOCK_SIZE];
    uint32 len;
    int    i;

    for (i = 0; i < BLOCK_SIZE; i++) {
        plain[i] = (uint8)('a' + i);
    }
    if (provision() != 0) {
        return -1;
    }

    len = sizeof(cipher);
    if (Csm_Encrypt(1U, CRYPTO_OPERATIONMODE_SINGLECALL, plain, BLOCK_SIZE,
                cipher, &len) != E_OK) {
        return -1;
    }
    len = sizeof(recovered);
    if (Csm_Decrypt(2U, CRYPTO_OPERATIONMODE_SINGLECALL, cipher, BLOCK_SIZE,
                recovered, &len) != E_OK) {
        return -1;
    }
    return (XMEMCMP(plain, recovered, BLOCK_SIZE) == 0) ? 0 : -1;
}

static int random_draw(void)
{
    uint8  buf[BLOCK_SIZE];
    uint32 len = (uint32)sizeof(buf);

    return (Csm_RandomGenerate(0U, buf, &len) == E_OK) ? 0 : -1;
}

#ifdef WOLFSSL_AUTOSAR_CMAC
static int mac_op(void)
{
    uint8  msg[BLOCK_SIZE];
    uint8  mac[BLOCK_SIZE];
    uint32 len = (uint32)sizeof(mac);

    XMEMSET(msg, 'm', sizeof(msg));
    if (Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_MAC_KEY,
                key, (uint32)sizeof(key)) != E_OK) {
        return -1;
    }
    return (Csm_MacGenerate(3U, CRYPTO_OPERATIONMODE_SINGLECALL,
                msg, BLOCK_SIZE, mac, &len) == E_OK) ? 0 : -1;
}
#endif

int main(void)
{
    struct driver_state st;
    Csm_ConfigType config;
    int rc;

    XMEMSET(&st, 0, sizeof(st));
    st.doCipher = 1;
    st.doMac    = 1;
    st.doRng    = 1;

    printf("An AUTOSAR Crypto driver backed by a crypto callback\n");

    /* The device table's free slots are marked INVALID_DEVID, which only
     * wolfCrypt_Init() sets up. Without it registration fails with BUFFER_E. */
    if (wolfCrypt_Init() != 0) {
        printf("wolfCrypt_Init failed\n");
        return 1;
    }

    rc = wc_CryptoCb_RegisterDevice(HSM_DEVID, hsmCryptoCb, &st);
    if (rc != 0) {
        printf("wc_CryptoCb_RegisterDevice failed: %d\n", rc);
        return 1;
    }
    printf("  registered device %d\n", HSM_DEVID);

    /* ---- no devId: the driver is not consulted ---- */
    printf("\n== Csm_Init(NULL) ==\n");
    Csm_Init(NULL);
    st.cipherOps = st.macOps = st.rngOps = st.setKeyOps = 0;
    st.traceLen = 0;

    check("AES-CBC round trip", cbc_roundtrip() == 0);
    check("Csm_RandomGenerate", random_draw() == 0);
    trace_print(&st, "operations offered to the driver:");
    check("nothing was offered to the driver",
            st.cipherOps == 0 && st.rngOps == 0 && st.macOps == 0);
    printf("  without a devId the port is entirely software, which is what\n");
    printf("  it did before Csm_ConfigType carried one\n");

    /* ---- with a devId: every context carries it ---- */
    printf("\n== Csm_Init(devId=%d) ==\n", HSM_DEVID);
    XMEMSET(&config, 0, sizeof(config));
    config.heap  = NULL;
    config.devId = HSM_DEVID;
    Csm_Init(&config);

    st.cipherOps = st.macOps = st.rngOps = st.setKeyOps = 0;
    st.traceLen = 0;
    check("AES-CBC round trip", cbc_roundtrip() == 0);
    trace_print(&st, "Csm_Encrypt + Csm_Decrypt offered:");
    check("the cipher operations reached the driver", st.cipherOps > 0);

    st.traceLen = 0;
    check("Csm_RandomGenerate", random_draw() == 0);
    trace_print(&st, "Csm_RandomGenerate offered:");
    check("the random operations reached the driver", st.rngOps > 0);

#ifdef WOLFSSL_AUTOSAR_CMAC
    st.traceLen = 0;
    check("Csm_MacGenerate", mac_op() == 0);
    trace_print(&st, "Csm_MacGenerate offered:");
    check("the MAC operation reached the driver", st.macOps > 0);
#else
    printf("  %-46s %s\n", "MAC operations", "not built");
#endif

    check("the driver only ever saw its own devId", st.badDevId == 0);

    /* ---- declining an operation ---- */
    printf("\n== a driver that does not do everything ==\n");
    st.doCipher = 0;
    st.traceLen = 0;
    check("AES-CBC still round trips when declined",
            cbc_roundtrip() == 0);
    trace_print(&st, "offered, then declined:");
    printf("  CRYPTOCB_UNAVAILABLE falls back to software\n");
    printf("  transparently, so a part that does AES but not CMAC\n");
    printf("  just declines the rest\n");
    st.doCipher = 1;

    /* ---- a driver that fails ---- */
    printf("\n== a driver that reports a failure ==\n");
    st.failNextCipher = 1;
    st.traceLen = 0;
    st.cipherOps = 0;
    check("Csm_Encrypt returns E_NOT_OK when the driver fails",
            cbc_roundtrip() != 0);
    /* The job has to have failed because the callback failed it: without
     * these, a job that never reached the callback at all -- a missing key, a
     * full job table -- would satisfy the check above just as well. */
    check("the callback was the one that failed it", st.cipherOps > 0);
    check("and it consumed the one-shot failure", st.failNextCipher == 0);
    trace_print(&st, "offered:");
    printf("  a wolfCrypt error from the callback becomes E_NOT_OK at the\n");
    printf("  Csm_* API, so hardware faults are not silently ignored\n");

    /* ---- what this does and does not protect ---- */
    printf("\n== what the devId does and does not give you ==\n");
    printf("  Offloads the WORK: the cipher, MAC and random\n");
    printf("  operations above were all offered to the driver.\n\n");
    printf("  Does NOT move the KEY. GetKey() still copies key bytes out of\n");
    printf("  the software keystore and wc_AesSetKey() takes them, so\n");
    printf("  the key is in RAM either way. Two things change that:\n");
#ifdef WOLF_CRYPTO_CB_AES_SETKEY
    printf("    - WOLF_CRYPTO_CB_AES_SETKEY is defined here, so the\n");
    printf("      key install was offered to the driver (%d time(s))\n",
            st.setKeyOps);
    printf("      and a real one would keep it in a hardware slot.\n");
#else
    printf("    - WOLF_CRYPTO_CB_AES_SETKEY, which offers the key\n");
    printf("      install to the driver so it can own the key. Not set\n");
    printf("      here: it is a raw -D, not a configure option.\n");
#endif
#ifdef WOLF_PRIVATE_KEY_ID
    printf("    - naming the key instead of storing it. This build has\n");
    printf("      WOLF_PRIVATE_KEY_ID, so wolfSSL_Csm_KeyElementSetId()\n");
    printf("      and ...SetLabel() point a keystore slot at a key in the\n");
    printf("      device and the driver uses wc_AesInit_Id(). The key then\n");
    printf("      never enters RAM at all -- cipher services only; the MAC\n");
    printf("      services still need material.\n");
#else
    printf("    - naming the key instead of storing it, with\n");
    printf("      wolfSSL_Csm_KeyElementSetId()/...SetLabel(). Needs\n");
    printf("      WOLF_PRIVATE_KEY_ID, which is not set in this build.\n");
#endif

    (void)wc_CryptoCb_UnRegisterDevice(HSM_DEVID);
    Csm_Init(NULL);

    if (failures != 0) {
        printf("\n%d check(s) failed\n", failures);
        printf("\ncsm-cryptocb: FAIL\n");
        return 1;
    }

    printf("\ncsm-cryptocb: PASS\n");
    return 0;
}

#endif /* WOLFSSL_AUTOSAR && WOLF_CRYPTO_CB */
