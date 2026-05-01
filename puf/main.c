/* main.c
 *
 * Bare-metal SRAM PUF example for Cortex-M.
 *
 * Platform-portable PUF demonstration: enrollment, reconstruction,
 * key derivation, and device identity. HAL-specific code (UART, RNG)
 * is in a separate file (e.g., stm32.c).
 *
 * Supports both test mode (synthetic SRAM data) and real hardware SRAM PUF.
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/puf.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/wc_port.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Platform HAL init (implemented in stm32.c or other target file) */
extern void hal_init(void);

/* -------------------------------------------------------------------------- */
/* PUF SRAM region (real hardware path)                                       */
/* -------------------------------------------------------------------------- */

#ifndef WOLFSSL_PUF_TEST
/* This buffer is placed in the .puf_sram linker section (NOLOAD).
 * The startup code must NOT zero this region - the raw power-on SRAM
 * state is the PUF entropy source. */
__attribute__((section(".puf_sram")))
static volatile uint8_t puf_sram_region[WC_PUF_RAW_BYTES];
#endif

/* -------------------------------------------------------------------------- */
/* Helper: print hex buffer                                                   */
/* -------------------------------------------------------------------------- */

static void print_hex(const char* label, const uint8_t* data, uint32_t len)
{
    uint32_t i;
    printf("%s: ", label);
    for (i = 0; i < len; i++)
        printf("%02x", data[i]);
    printf("\n");
}

/* -------------------------------------------------------------------------- */
/* Main PUF demonstration                                                     */
/* -------------------------------------------------------------------------- */

int main(void)
{
    wc_PufCtx ctx;
    int ret;
    uint8_t identity[WC_PUF_ID_SZ];
    uint8_t key[WC_PUF_KEY_SZ];
    uint8_t helperData[WC_PUF_HELPER_BYTES];
    const uint8_t info[] = "puf-example-key";

    /* Initialize platform HAL (UART, clocks, etc.) */
    hal_init();

    printf("\n--- wolfCrypt SRAM PUF Example ---\n\n");

    /* Initialize wolfCrypt */
    ret = wolfCrypt_Init();
    if (ret != 0) {
        printf("ERROR: wolfCrypt_Init failed: %d\n", ret);
        return ret;
    }

    /* Step 1: Initialize PUF context */
    ret = wc_PufInit(&ctx);
    if (ret != 0) {
        printf("ERROR: wc_PufInit failed: %d\n", ret);
        goto cleanup;
    }
    printf("PUF initialized.\n");

#ifdef WOLFSSL_PUF_TEST
    /* ================================================================== */
    /* TEST MODE: Use synthetic SRAM data                                 */
    /* ================================================================== */
    {
        /* Synthetic SRAM pattern for testing (deterministic) */
        uint8_t testSram[WC_PUF_RAW_BYTES];
        uint8_t noisySram[WC_PUF_RAW_BYTES];
        uint8_t identity2[WC_PUF_ID_SZ];
        uint8_t key2[WC_PUF_KEY_SZ];
        uint32_t i;

        printf("Mode: TEST (synthetic SRAM data)\n\n");

        /* Generate deterministic test pattern */
        for (i = 0; i < WC_PUF_RAW_BYTES; i++)
            testSram[i] = (uint8_t)((i * 37 + 13) ^ (i >> 2));

        /* Step 2: Load test SRAM data */
        ret = wc_PufSetTestData(&ctx, testSram, sizeof(testSram));
        if (ret != 0) {
            printf("ERROR: wc_PufSetTestData failed: %d\n", ret);
            goto cleanup;
        }

        /* Step 3: Enroll - generates helper data from SRAM pattern */
        ret = wc_PufEnroll(&ctx);
        if (ret != 0) {
            printf("ERROR: wc_PufEnroll failed: %d\n", ret);
            goto cleanup;
        }
        printf("Enrollment complete.\n");

        /* Save helper data (in production, store to flash/NVM) */
        memcpy(helperData, ctx.helperData, WC_PUF_HELPER_BYTES);

        /* Get device identity */
        ret = wc_PufGetIdentity(&ctx, identity, sizeof(identity));
        if (ret != 0) {
            printf("ERROR: wc_PufGetIdentity failed: %d\n", ret);
            goto cleanup;
        }
        print_hex("Identity (enrollment)", identity, WC_PUF_ID_SZ);

        /* Derive a key */
        ret = wc_PufDeriveKey(&ctx, info, sizeof(info), key, sizeof(key));
        if (ret != 0) {
            printf("ERROR: wc_PufDeriveKey failed: %d\n", ret);
            goto cleanup;
        }
        print_hex("Derived key (enrollment)", key, WC_PUF_KEY_SZ);

        /* ---- Simulate power cycle with noisy SRAM ---- */
        printf("\n--- Simulating power cycle (noisy SRAM) ---\n\n");

        /* Create noisy copy: flip a few bits per 128-bit block */
        memcpy(noisySram, testSram, WC_PUF_RAW_BYTES);
        for (i = 0; i < 16; i++) {
            /* Flip 2 bits in each 16-byte (128-bit) block */
            noisySram[i * 16 + 3] ^= 0x04;
            noisySram[i * 16 + 11] ^= 0x20;
        }

        /* Re-initialize and load noisy SRAM */
        ret = wc_PufInit(&ctx);
        if (ret != 0) {
            printf("ERROR: wc_PufInit failed: %d\n", ret);
            goto cleanup;
        }

        ret = wc_PufSetTestData(&ctx, noisySram, sizeof(noisySram));
        if (ret != 0) {
            printf("ERROR: wc_PufSetTestData failed: %d\n", ret);
            goto cleanup;
        }

        /* Step 4: Reconstruct stable bits using helper data */
        ret = wc_PufReconstruct(&ctx, helperData, WC_PUF_HELPER_BYTES);
        if (ret != 0) {
            printf("ERROR: wc_PufReconstruct failed: %d\n", ret);
            goto cleanup;
        }
        printf("Reconstruction complete (BCH corrected noisy bits).\n");

        /* Verify identity matches */
        ret = wc_PufGetIdentity(&ctx, identity2, sizeof(identity2));
        if (ret != 0) {
            printf("ERROR: wc_PufGetIdentity failed: %d\n", ret);
            goto cleanup;
        }
        print_hex("Identity (reconstructed)", identity2, WC_PUF_ID_SZ);

        if (memcmp(identity, identity2, WC_PUF_ID_SZ) == 0) {
            printf("PASS: Identity matches after reconstruction.\n");
        }
        else {
            printf("FAIL: Identity mismatch!\n");
            ret = -1;
            goto cleanup;
        }

        /* Verify derived key matches */
        ret = wc_PufDeriveKey(&ctx, info, sizeof(info), key2, sizeof(key2));
        if (ret != 0) {
            printf("ERROR: wc_PufDeriveKey failed: %d\n", ret);
            goto cleanup;
        }
        print_hex("Derived key (reconstructed)", key2, WC_PUF_KEY_SZ);

        if (memcmp(key, key2, WC_PUF_KEY_SZ) == 0) {
            printf("PASS: Derived key matches after reconstruction.\n");
        }
        else {
            printf("FAIL: Derived key mismatch!\n");
            ret = -1;
            goto cleanup;
        }
    }
#else
    /* ================================================================== */
    /* REAL HARDWARE: Read actual SRAM PUF                                */
    /* ================================================================== */
    {
        uint8_t identity2[WC_PUF_ID_SZ];
        uint8_t key2[WC_PUF_KEY_SZ];

        printf("Mode: HARDWARE (real SRAM PUF)\n\n");

        /* ---- Phase 1: Enrollment ---- */

        /* Read raw SRAM (must happen before any other access to the
         * .puf_sram region - the power-on entropy is consumed once). */
        ret = wc_PufReadSram(&ctx, (const uint8_t*)puf_sram_region,
                             sizeof(puf_sram_region));
        if (ret != 0) {
            printf("ERROR: wc_PufReadSram failed: %d\n", ret);
            goto cleanup;
        }
        printf("SRAM read complete (%d bytes).\n",
               (int)sizeof(puf_sram_region));

        ret = wc_PufEnroll(&ctx);
        if (ret != 0) {
            printf("ERROR: wc_PufEnroll failed: %d\n", ret);
            goto cleanup;
        }
        printf("Enrollment complete.\n");

        /* Save helper data. In production this is written to flash/NVM
         * for use across reboots; here it stays in RAM so the same run
         * can also exercise the reconstruction path below. */
        memcpy(helperData, ctx.helperData, WC_PUF_HELPER_BYTES);
        print_hex("Helper data (store to NVM)", helperData,
                  WC_PUF_HELPER_BYTES);

        ret = wc_PufGetIdentity(&ctx, identity, sizeof(identity));
        if (ret != 0) {
            printf("ERROR: wc_PufGetIdentity failed: %d\n", ret);
            goto cleanup;
        }
        print_hex("Identity (enrollment)", identity, WC_PUF_ID_SZ);

        ret = wc_PufDeriveKey(&ctx, info, sizeof(info), key, sizeof(key));
        if (ret != 0) {
            printf("ERROR: wc_PufDeriveKey failed: %d\n", ret);
            goto cleanup;
        }
        print_hex("Derived key (enrollment)", key, WC_PUF_KEY_SZ);

        /* ---- Phase 2: Reconstruction ---- */

        /* On real hardware the same .puf_sram contents are still in
         * RAM, so re-reading them yields the same bytes and BCH runs
         * with zero errors. Under m33mu, run the example a second time
         * with a different --puf-cold-boot or with --puf-noise to feed
         * a noisy SRAM image through wc_PufReconstruct using the
         * helper data captured above. */
        printf("\n--- Reconstructing from saved helper data ---\n\n");

        ret = wc_PufInit(&ctx);
        if (ret != 0) {
            printf("ERROR: wc_PufInit (reconstruct) failed: %d\n", ret);
            goto cleanup;
        }

        ret = wc_PufReadSram(&ctx, (const uint8_t*)puf_sram_region,
                             sizeof(puf_sram_region));
        if (ret != 0) {
            printf("ERROR: wc_PufReadSram (reconstruct) failed: %d\n", ret);
            goto cleanup;
        }

        ret = wc_PufReconstruct(&ctx, helperData, WC_PUF_HELPER_BYTES);
        if (ret != 0) {
            printf("ERROR: wc_PufReconstruct failed: %d\n", ret);
            goto cleanup;
        }
        printf("Reconstruction complete (BCH error correction ran).\n");

        ret = wc_PufGetIdentity(&ctx, identity2, sizeof(identity2));
        if (ret != 0) {
            printf("ERROR: wc_PufGetIdentity (reconstruct) failed: %d\n", ret);
            goto cleanup;
        }
        print_hex("Identity (reconstructed)", identity2, WC_PUF_ID_SZ);

        if (memcmp(identity, identity2, WC_PUF_ID_SZ) == 0) {
            printf("PASS: Identity matches after reconstruction.\n");
        }
        else {
            printf("FAIL: Identity mismatch after reconstruction!\n");
            ret = -1;
            goto cleanup;
        }

        ret = wc_PufDeriveKey(&ctx, info, sizeof(info), key2, sizeof(key2));
        if (ret != 0) {
            printf("ERROR: wc_PufDeriveKey (reconstruct) failed: %d\n", ret);
            goto cleanup;
        }
        print_hex("Derived key (reconstructed)", key2, WC_PUF_KEY_SZ);

        if (memcmp(key, key2, WC_PUF_KEY_SZ) == 0) {
            printf("PASS: Derived key matches after reconstruction.\n");
        }
        else {
            printf("FAIL: Derived key mismatch after reconstruction!\n");
            ret = -1;
            goto cleanup;
        }
    }
#endif /* WOLFSSL_PUF_TEST */

    printf("\n--- PUF example complete ---\n");

cleanup:
    /* Securely zeroize all PUF secrets */
    wc_PufZeroize(&ctx);
    wolfCrypt_Cleanup();

    return ret;
}
