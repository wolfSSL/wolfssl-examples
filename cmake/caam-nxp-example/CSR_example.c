/* csr_example.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */


#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MIMXRT1176_cm7.h"
#include "fsl_debug_console.h"


/* wolfSSL includes */
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/port/caam/wolfcaam.h>

#define LARGE_TEMP_SZ 4096

#if !defined(WOLFSSL_KEY_GEN) || !defined(WOLFSSL_CERT_GEN) || \
    !defined(WOLFSSL_CERT_REQ)
    #error correct wolfSSL defines not set
#endif

/* generate a key to use for signing */
static int CreateKey(ecc_key* key, WC_RNG* rng)
{
    int ret;

    ret = wc_ecc_init_ex(key, NULL, WOLFSSL_CAAM_DEVID);
    if (ret != 0) {
        PRINTF("Key initialization failed: %d\r\n", ret);
        return ret;
    }

    ret = wc_ecc_make_key_ex(rng, 32, key, ECC_SECP256R1);
    if (ret != 0) {
        PRINTF("Key generation failed: %d\r\n", ret);
        return ret;
    }

    PRINTF("Successfully created a new key\r\n");
    return ret;
}


/* export the key that was generated */
static int ExportKey(ecc_key* key, byte* der, word32* derSz,
        byte* blob, word32* blobSz)
{
    int ret;
    word32 i;

    PRINTF("\n\rExporting Key\r\n");
    ret = wc_EccKeyToDer(key, der, *derSz);
    if (ret <= 0) {
        PRINTF("Key To DER failed: %d\r\n", ret);
        return ret;
    }
    *derSz = (word32)ret;

    PRINTF("DER of Key : ");
    for (i = 0; i < *derSz; i++)
        PRINTF("%02X", der[i]);
    PRINTF("\r\n");

    /* export private key in a black blob */
#ifndef WOLFSSL_CAAM_BLOB
    PRINTF("WOLFSSL_CAAM_BLOB not defined\r\n");
#else
    if (ret >= 0) {
        /* using an arbitrary keymod, this needs to be the same as whats
         * used for decapsulating the blob */
        byte mod[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        int modSz = WC_CAAM_BLACK_KEYMOD_SZ;
        int type = WC_CAAM_BLOB_BLACK;

        byte pri[MAX_ECC_BYTES];
        word32 priSz = MAX_ECC_BYTES;

        ret = wc_ecc_export_private_only(key, pri, &priSz);
        if (ret >= 0) {
            ret = wc_caamCreateBlob_ex(pri, priSz, blob, blobSz, type, mod,
                modSz);
        }
        PRINTF("BLACK BLOB [%d] :", *blobSz);
        for (i = 0; i < *blobSz; i++)
            PRINTF("%02X", blob[i]);
        PRINTF("\r\n");
    }
#endif

    return 0;
}


/* import the key into a wolfSSL structure */
static int ImportKey(ecc_key* key, byte* der, int derSz,
        byte* blob, int blobSz)
{
    int ret;
    word32 idx = 0;

    /* clear out ecc_key structure */
    wc_ecc_free(key);
    memset(key, 0, sizeof(ecc_key));

    ret = wc_ecc_init_ex(key, NULL, WOLFSSL_CAAM_DEVID);
    if (ret == 0) {
        ret = wc_EccPrivateKeyDecode(der, &idx, key, derSz);
    }

#ifdef WOLFSSL_CAAM_BLOB
    if (ret == 0) {
        /* using an arbitrary keymod, this needs to be the same as whats
         * used for decapsulating the blob */
        byte mod[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        int modSz = WC_CAAM_BLACK_KEYMOD_SZ;
        int type = WC_CAAM_BLOB_BLACK;

        byte pri[MAX_ECC_BYTES];
        word32 priSz = MAX_ECC_BYTES;

        ret = wc_caamOpenBlob_ex(blob, blobSz, pri, &priSz, type, mod, modSz);

        if (ret == 0) {
            byte pub[MAX_ECC_BYTES];
            word32 pubSz = MAX_ECC_BYTES;

            ret = wc_ecc_export_x963(key, pub, &pubSz);
            if (ret == 0) {
                ret = wc_ecc_import_private_key(pri, priSz, pub, pubSz, key);
            }
        }
    }
#endif


    return ret;
}


/* setup structure for CSR, sign, and export it */
static int CreateCSR(ecc_key* key, WC_RNG* rng, byte* csr, word32* csrSz)
{
    int ret;
    Cert req;
    int  type = ECC_TYPE; /* also ava. RSA_TYPE, ED25519_TYPE */

    ret = wc_InitCert(&req);
    if (ret != 0) {
        PRINTF("Init Cert failed: %d\r\n", ret);
    }

    if (ret == 0) {
        strncpy(req.subject.country, "US", CTC_NAME_SIZE);
        strncpy(req.subject.state, "MT", CTC_NAME_SIZE);
        strncpy(req.subject.locality, "Bozeman", CTC_NAME_SIZE);
        strncpy(req.subject.org, "wolfSSL", CTC_NAME_SIZE);
        strncpy(req.subject.unit, "Development", CTC_NAME_SIZE);
        strncpy(req.subject.commonName, "www.wolfssl.com", CTC_NAME_SIZE);
        strncpy(req.subject.email, "info@wolfssl.com", CTC_NAME_SIZE);
        req.version = 0;
        ret = wc_MakeCertReq_ex(&req, csr, *csrSz, type, key);
        if (ret <= 0) {
            PRINTF("Make Cert Req failed: %d\r\n", ret);
        }
    }

    if (ret > 0) {
        req.sigType = CTC_SHA256wECDSA;
        ret = wc_SignCert_ex(req.bodySz, req.sigType, csr, *csrSz, type,
                key, rng);
        if (ret <= 0) {
            PRINTF("Sign Cert failed: %d\r\n", ret);
        }
        else {
            int i;

            *csrSz = (word32)ret;

            PRINTF("\n\rCSR : ");
            for (i = 0; i < ret; i++)
                PRINTF("%02X", csr[i]);
            PRINTF("\r\n");
        }
    }

    return ret;
}


int main(void)
{
    ecc_key key;
    WC_RNG  rng;
    int ret;

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    if (wolfCrypt_Init() != 0) {
        PRINTF("Error initializing wolfCrypt!!\r\n");
    }
    else {
        byte der[LARGE_TEMP_SZ];
        byte blob[LARGE_TEMP_SZ];
        word32 blobSz = LARGE_TEMP_SZ;
        word32 derSz  = LARGE_TEMP_SZ;

        ret = wc_InitRng(&rng);
        if (ret != 0) {
            PRINTF("Failed to init RNG : %d\n", ret);
        }

        if (ret == 0)
            ret = CreateKey(&key, &rng);
        if (ret >= 0)
            ret = ExportKey(&key, der, &derSz, blob, &blobSz);
        if (ret >= 0)
            ret = ImportKey(&key, der, derSz, blob, blobSz);
        if (ret >= 0) {
            derSz = LARGE_TEMP_SZ;
            ret = CreateCSR(&key, &rng, der, &derSz);
        }

        wc_ecc_free(&key);
        wc_FreeRng(&rng);

        PRINTF("Done\r\n");

        /* last wolfSSL call in application before exit */
        wolfCrypt_Cleanup();
    }

    /* Force the counter to be placed into memory. */
    volatile static int i = 0 ;
    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
        i++ ;
        /* 'Dummy' NOP to allow source level single stepping of
            tight while() loop */
        __asm volatile ("nop");
    }
    return 0 ;
}
