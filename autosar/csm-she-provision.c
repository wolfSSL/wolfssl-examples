/* csm-she-provision.c
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

/* Where an AUTOSAR ECU's keys come from: the SHE key update protocol.
 *
 * Every other example in this directory calls Csm_KeyElementSet() with a key
 * compiled into the binary, which no production ECU does. In a vehicle the key
 * arrives as a SHE (Secure Hardware Extension) key update block, authorized by
 * a key the ECU already holds, and lands in a protected HSM slot. This example
 * is that step, and the handover to the CSM afterwards.
 *
 * The protocol, five messages:
 *
 *   backend -> ECU
 *     M1  UID | target key ID | authorizing key ID          (16 bytes)
 *     M2  AES-CBC(K1, counter | flags | pad | new key)      (32 bytes)
 *     M3  AES-CMAC(K2, M1|M2)                               (16 bytes)
 *   ECU -> backend
 *     M4  UID | IDs | AES-ECB(K3, counter | pad)            (32 bytes)
 *     M5  AES-CMAC(K4, M4)                                  (16 bytes)
 *
 * K1 and K2 are derived from the authorizing key, K3 and K4 from the new key,
 * so M3 proves the sender knew the authorizing key and M5 proves the ECU
 * actually installed the new one. The counter is what stops a replay of an old
 * update block.
 *
 * What this example is: the backend side. It builds a key update block, works
 * out the M4/M5 it expects back, checks the ECU's answer, and then uses the
 * provisioned key through the CSM. That is what a key-management backend or an
 * end-of-line programming tool does.
 *
 * What it is NOT: the ECU side. Recovering the new key from M2 needs the
 * authorizing key's derived K1 and happens inside the HSM; wolfSSL does not
 * expose that as a software call, deliberately. With real hardware and a crypto
 * callback, wc_SHE_LoadKey_Verify() sends M1/M2/M3 to the HSM and checks the
 * M4/M5 it returns in one call -- see the end of this file.
 *
 * Requires wolfSSL built with --enable-autosar --enable-she=standard
 * (add --enable-autosar-cmac for the MAC half of the CSM section).
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
    printf("\ncsm-she-provision: SKIP\n");
    return 0;
}

#elif !defined(WOLFSSL_SHE)

int main(void)
{
    printf("This example needs wolfSSL's SHE support.\n");
    printf("Rebuild wolfSSL with: "
           "./configure --enable-autosar --enable-she=standard\n");
    printf("\ncsm-she-provision: SKIP\n");
    return 0;
}

#else /* WOLFSSL_AUTOSAR && WOLFSSL_SHE */

#include <wolfssl/wolfcrypt/port/autosar/Csm.h>
#include <wolfssl/wolfcrypt/wc_she.h>

#define BLOCK_SIZE 16

/* SHE key slots. Slot 1 is MASTER_ECU_KEY, the key that authorizes updates to
 * the others; KEY_1..KEY_10 are slots 4..13 and hold application keys. */
#define AUTH_KEY_ID   WC_SHE_MASTER_ECU_KEY_ID  /* 1 */
#define TARGET_KEY_ID 4                         /* KEY_1 */

/* CSM keystore slot the provisioned key is loaded into for use. Unrelated to
 * the SHE slot number above: SHE says where the key lives in hardware, the
 * keystore is how this port's software jobs reach it. */
#ifdef REDIRECTION_CONFIG
    #define CSM_KEY_SLOT ((uint32)REDIRECTION_IN1_KEYID)
    #define CSM_IV_SLOT  ((uint32)REDIRECTION_IN2_KEYID)
#else
    #define CSM_KEY_SLOT 0U
    #define CSM_IV_SLOT  1U
#endif

/* The SHE specification's memory update example. Pinning these means a block
 * built here is one real SHE hardware will accept -- a self-consistency check
 * would prove nothing about interoperability. wolfSSL's own test suite pins
 * M1, M4 and M5 against these same values. */
static const byte sheUid[WC_SHE_UID_SZ] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};
static const byte authKey[WC_SHE_KEY_SZ] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
static const byte newKey[WC_SHE_KEY_SZ] = {
    0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
    0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
};
static const byte expectM1[WC_SHE_M1_SZ] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41
};
static const byte expectM4[WC_SHE_M4_SZ] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41,
    0xb4, 0x72, 0xe8, 0xd8, 0x72, 0x7d, 0x70, 0xd5,
    0x72, 0x95, 0xe7, 0x48, 0x49, 0xa2, 0x79, 0x17
};
static const byte expectM5[WC_SHE_M5_SZ] = {
    0x82, 0x0d, 0x8d, 0x95, 0xdc, 0x11, 0xb4, 0x66,
    0x88, 0x78, 0x16, 0x0c, 0xb2, 0xa4, 0xe2, 0x3e
};

static int failures = 0;

static void check(const char* what, int ok)
{
    printf("  %-44s %s\n", what, ok ? "ok" : "FAILED");
    if (!ok) {
        failures++;
    }
}

static void print_hex(const char* label, const byte* buf, word32 len)
{
    word32 i;

    printf("  %-5s", label);
    for (i = 0; i < len; i++) {
        printf("%02x", buf[i]);
        /* wrap the 32 byte messages so they stay readable */
        if (((i + 1) % 16) == 0 && (i + 1) < len) {
            printf("\n       ");
        }
    }
    printf("\n");
}

/* One key update block, plus the M4/M5 the backend expects in return. */
struct update_block {
    byte m1[WC_SHE_M1_SZ];
    byte m2[WC_SHE_M2_SZ];
    byte m3[WC_SHE_M3_SZ];
    byte m4[WC_SHE_M4_SZ];
    byte m5[WC_SHE_M5_SZ];
};

/* Builds the block the backend sends, and the response it expects.
 * Returns 0 on success. */
static int build_update(struct update_block* b, const byte* aKey,
        const byte* nKey, word32 counter, byte flags)
{
    wc_SHE she;
    int ret;

    /* INVALID_DEVID: compute in software. A real backend talking to an HSM
     * passes its devId here instead. */
    ret = wc_SHE_Init(&she, NULL, INVALID_DEVID);
    if (ret != 0) {
        printf("wc_SHE_Init failed: %d\n", ret);
        return -1;
    }

    ret = wc_SHE_GenerateM1M2M3(&she,
            sheUid, WC_SHE_UID_SZ,
            AUTH_KEY_ID, aKey, WC_SHE_KEY_SZ,
            TARGET_KEY_ID, nKey, WC_SHE_KEY_SZ,
            counter, flags,
            b->m1, WC_SHE_M1_SZ,
            b->m2, WC_SHE_M2_SZ,
            b->m3, WC_SHE_M3_SZ);
    if (ret != 0) {
        printf("wc_SHE_GenerateM1M2M3 failed: %d\n", ret);
        wc_SHE_Free(&she);
        return -1;
    }

    /* M4/M5 depend on the NEW key, not the authorizing one, which is why they
     * prove the ECU installed it. Computed on the same inputs, so the backend
     * knows the answer before the ECU replies. */
    ret = wc_SHE_GenerateM4M5(&she,
            sheUid, WC_SHE_UID_SZ,
            AUTH_KEY_ID, TARGET_KEY_ID,
            nKey, WC_SHE_KEY_SZ,
            counter,
            b->m4, WC_SHE_M4_SZ,
            b->m5, WC_SHE_M5_SZ);
    if (ret != 0) {
        printf("wc_SHE_GenerateM4M5 failed: %d\n", ret);
        wc_SHE_Free(&she);
        return -1;
    }

    wc_SHE_Free(&she);
    return 0;
}

/* Phase 1: the block we generate is the one the specification describes. */
static void known_answer(struct update_block* b)
{
    printf("\n== the key update block ==\n");
    printf("  SHE slot %d (KEY_1), authorized by slot %d "
           "(MASTER_ECU_KEY), counter 1\n\n",
            TARGET_KEY_ID, AUTH_KEY_ID);

    if (build_update(b, authKey, newKey, 1, 0) != 0) {
        failures++;
        return;
    }

    print_hex("M1", b->m1, WC_SHE_M1_SZ);
    print_hex("M2", b->m2, WC_SHE_M2_SZ);
    print_hex("M3", b->m3, WC_SHE_M3_SZ);
    printf("\n  expected back from the ECU:\n");
    print_hex("M4", b->m4, WC_SHE_M4_SZ);
    print_hex("M5", b->m5, WC_SHE_M5_SZ);
    printf("\n");

    check("M1 matches the SHE specification vector",
            XMEMCMP(b->m1, expectM1, WC_SHE_M1_SZ) == 0);
    check("M4 matches the SHE specification vector",
            XMEMCMP(b->m4, expectM4, WC_SHE_M4_SZ) == 0);
    check("M5 matches the SHE specification vector",
            XMEMCMP(b->m5, expectM5, WC_SHE_M5_SZ) == 0);

    /* M2 and M3 are just as deterministic, but wolfSSL's test suite does not
     * pin them, so this example does not claim a vector it cannot cite. */
    printf("  (M2 and M3 are deterministic too, but no vector is cited "
           "for them here)\n");
}

/* Computes the M4/M5 an ECU holding 'installed' as the target key would send.
 * That is the ECU half of the protocol: the same wc_SHE_GenerateM4M5() the
 * backend runs, from the key the ECU actually has.
 * Returns 0 on success. */
static int ecu_response(const byte* installed, word32 counter,
        byte* m4, byte* m5)
{
    wc_SHE she;
    int ret;

    ret = wc_SHE_Init(&she, NULL, INVALID_DEVID);
    if (ret != 0) {
        printf("wc_SHE_Init failed: %d\n", ret);
        return -1;
    }

    ret = wc_SHE_GenerateM4M5(&she,
            sheUid, WC_SHE_UID_SZ,
            AUTH_KEY_ID, TARGET_KEY_ID,
            installed, WC_SHE_KEY_SZ,
            counter,
            m4, WC_SHE_M4_SZ,
            m5, WC_SHE_M5_SZ);
    wc_SHE_Free(&she);

    if (ret != 0) {
        printf("wc_SHE_GenerateM4M5 failed: %d\n", ret);
        return -1;
    }
    return 0;
}

/* Phase 2: checking what the ECU sent back.
 *
 * The response has to be computed, not copied: comparing the expected M5 with
 * itself would pass whatever the protocol did. So this runs the ECU side --
 * once with the key the backend sent, once with a different key, standing in
 * for an ECU that installed the wrong one -- and checks the backend accepts
 * only the first. */
static void verify_response(const struct update_block* b)
{
    byte theirM4[WC_SHE_M4_SZ];
    byte theirM5[WC_SHE_M5_SZ];
    /* an ECU that ended up with something else in TARGET_KEY_ID */
    static const byte otherKey[WC_SHE_KEY_SZ] = {
        0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,
        0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5
    };

    printf("\n== verifying the ECU's response ==\n");

    /* The wrong-key case is only a test if the key really differs. */
    check("the stand-in key differs from the new key",
            XMEMCMP(otherKey, newKey, WC_SHE_KEY_SZ) != 0);

    /* The ECU that installed the key the backend sent. */
    if (ecu_response(newKey, 1, theirM4, theirM5) != 0) {
        check("ECU response computed", 0);
        return;
    }
    check("correct M5 accepted",
            wc_ConstantCompare(theirM5, b->m5, WC_SHE_M5_SZ) == 0);
    check("correct M4 accepted",
            wc_ConstantCompare(theirM4, b->m4, WC_SHE_M4_SZ) == 0);

    /* An ECU that failed to install it, or installed a different one, cannot
     * produce this M5 -- K4 is derived from the new key. */
    if (ecu_response(otherKey, 1, theirM4, theirM5) != 0) {
        check("wrong-key response computed", 0);
        return;
    }
    check("M5 from the wrong key rejected",
            wc_ConstantCompare(theirM5, b->m5, WC_SHE_M5_SZ) != 0);

    printf("  M5 is a CMAC under a key derived from the new key, so only an\n");
    printf("  ECU that really installed it can produce one that matches\n");
}

/* Phase 3: the counter, and why the backend has to track it. */
static void counter_and_auth(void)
{
    struct update_block first, again, attacker;
    byte wrongAuth[WC_SHE_KEY_SZ];

    printf("\n== replay and authorization ==\n");

    if (build_update(&first, authKey, newKey, 1, 0) != 0 ||
            build_update(&again, authKey, newKey, 2, 0) != 0) {
        failures++;
        return;
    }

    check("a new counter changes M2",
            XMEMCMP(first.m2, again.m2, WC_SHE_M2_SZ) != 0);
    check("a new counter changes M3",
            XMEMCMP(first.m3, again.m3, WC_SHE_M3_SZ) != 0);
    check("a new counter changes the expected M4/M5",
            XMEMCMP(first.m5, again.m5, WC_SHE_M5_SZ) != 0);

    printf("  the counter is inside M2 and covered by M3, so an old block\n");
    printf("  cannot be edited to look new. SHE hardware refuses a counter\n");
    printf("  that is not greater than the one in the slot, so the backend\n");
    printf("  must track it per slot -- reusing one wastes the update\n");

    /* Without the authorizing key an attacker cannot produce a usable M3.
     *
     * Counter 1, the same as 'first': the check above already showed that
     * changing the counter changes M3, so an attacker block on counter 3
     * would differ for that reason alone and prove nothing about
     * authorization. Same counter, same target key, only the authorizing key
     * differs. */
    XMEMSET(wrongAuth, 0xA5, sizeof(wrongAuth));
    if (build_update(&attacker, wrongAuth, newKey, 1, 0) != 0) {
        failures++;
        return;
    }
    check("a wrong authorizing key changes M3",
            XMEMCMP(attacker.m3, first.m3, WC_SHE_M3_SZ) != 0);
    printf("  M3 is a CMAC under a key derived from MASTER_ECU_KEY, so the\n");
    printf("  ECU rejects an update from anyone who does not hold it\n");
}

/* Phase 4: handing the provisioned key to the CSM. */
static void use_through_csm(void)
{
    uint8  iv[BLOCK_SIZE];
    uint8  plain[BLOCK_SIZE];
    uint8  cipher[BLOCK_SIZE];
    uint8  recovered[BLOCK_SIZE];
    uint32 len;
    int    i;

    printf("\n== using the provisioned key through the CSM ==\n");

    for (i = 0; i < BLOCK_SIZE; i++) {
        iv[i]    = (uint8)(0x10 + i);
        plain[i] = (uint8)('a' + i);
    }

    /* On real hardware the key value never leaves the HSM: the ECU runs its
     * CSM jobs against the SHE slot, which is what Csm_ConfigType's devId is
     * for -- the driver hands it to every wolfCrypt context it creates, so the
     * work goes to the crypto callback backing the HSM instead of to software.
     *
     * This example has no HSM to talk to, so it loads the key value into the
     * software keystore, which is the one thing a production ECU must not do.
     * With a device registered, Csm_Init() would be given that devId and the
     * keystore would hold a reference rather than the key. */
    printf("  NOTE: loading the key VALUE into the software keystore.\n");
    printf("  On an ECU the key stays in the HSM: register a crypto\n");
    printf("  callback and pass its devId in Csm_ConfigType, and these\n");
    printf("  same jobs run against the hardware slot instead.\n\n");

    if (Csm_KeyElementSet(CSM_KEY_SLOT, CRYPTO_KE_CIPHER_KEY,
                newKey, WC_SHE_KEY_SZ) != E_OK) {
        check("Csm_KeyElementSet with the provisioned key", 0);
        return;
    }
    if (Csm_KeyElementSet(CSM_IV_SLOT, CRYPTO_KE_CIPHER_IV,
                iv, BLOCK_SIZE) != E_OK) {
        check("Csm_KeyElementSet (IV)", 0);
        return;
    }
    check("provisioned key accepted by the keystore", 1);

    len = sizeof(cipher);
    if (Csm_Encrypt(1U, CRYPTO_OPERATIONMODE_SINGLECALL, plain, BLOCK_SIZE,
                cipher, &len) != E_OK) {
        check("AES-CBC with the provisioned key", 0);
        return;
    }
    len = sizeof(recovered);
    if (Csm_Decrypt(2U, CRYPTO_OPERATIONMODE_SINGLECALL, cipher, BLOCK_SIZE,
                recovered, &len) != E_OK) {
        check("AES-CBC with the provisioned key", 0);
        return;
    }
    check("AES-CBC round trip with the provisioned key",
            XMEMCMP(plain, recovered, BLOCK_SIZE) == 0);

#ifdef WOLFSSL_AUTOSAR_CMAC
    {
        uint8  mac[BLOCK_SIZE];
        uint32 macLen = (uint32)sizeof(mac);
        Crypto_VerifyResultType verify = CRYPTO_E_VER_NOT_OK;

        if (Csm_KeyElementSet(CSM_KEY_SLOT, CRYPTO_KE_MAC_KEY,
                    newKey, WC_SHE_KEY_SZ) != E_OK) {
            check("Csm_KeyElementSet (MAC key)", 0);
            return;
        }
        if (Csm_MacGenerate(3U, CRYPTO_OPERATIONMODE_SINGLECALL,
                    plain, BLOCK_SIZE, mac, &macLen) != E_OK ||
            Csm_MacVerify(4U, CRYPTO_OPERATIONMODE_SINGLECALL,
                    plain, BLOCK_SIZE, mac, macLen * 8, &verify) != E_OK) {
            check("AES-CMAC with the provisioned key", 0);
            return;
        }
        check("AES-CMAC generate and verify with the provisioned key",
                verify == CRYPTO_E_VER_OK);
        printf("  a SecOC key arrives exactly this way; see csm-secoc.c\n");
    }
#else
    printf("  %-44s %s\n", "AES-CMAC with the provisioned key",
            "not built (--enable-autosar-cmac)");
#endif
}

int main(void)
{
    struct update_block block;

    Csm_Init(NULL);

    printf("SHE key update, then the CSM\n");

    XMEMSET(&block, 0, sizeof(block));
    known_answer(&block);
    verify_response(&block);
    counter_and_auth();
    use_through_csm();

    printf("\n== on real hardware ==\n");
    printf("  With a crypto callback and a live devId, the ECU side is one\n");
    printf("  call: wc_SHE_LoadKey_Verify() sends M1/M2/M3 to the HSM and\n");
    printf("  compares the M4/M5 it returns against the expected values,\n");
    printf("  returning SIG_VERIFY_E on a mismatch. It needs WOLF_CRYPTO_CB\n");
    printf("  and a driver for the part, so it is not exercised here.\n");

    if (failures != 0) {
        printf("\n%d check(s) failed\n", failures);
        printf("\ncsm-she-provision: FAIL\n");
        return 1;
    }

    printf("\ncsm-she-provision: PASS\n");
    return 0;
}

#endif /* WOLFSSL_AUTOSAR && WOLFSSL_SHE */
