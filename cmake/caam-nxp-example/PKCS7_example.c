/* PKCS7_example.c
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
#include "fsl_snvs_hp.h"

/* wolfSSL includes */
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/port/caam/wolfcaam.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define LARGE_TMP_SIZE 4096

static int SetDate()
{
    snvs_hp_rtc_datetime_t rtcDate;
    snvs_hp_rtc_config_t snvsRtcConfig;

    SNVS_HP_RTC_GetDefaultConfig(&snvsRtcConfig);
    SNVS_HP_RTC_Init(SNVS, &snvsRtcConfig);

    /* Set a start date time and start RT */
    rtcDate.year   = 2022U;
    rtcDate.month  = 12U;
    rtcDate.day    = 25U;
    rtcDate.hour   = 19U;
    rtcDate.minute = 0;
    rtcDate.second = 0;

    /* Set RTC time to default time and date and start the RTC */
    SNVS_HP_RTC_SetDatetime(SNVS, &rtcDate);

    return 0;
}


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
    #if defined(configTOTAL_HEAP_SIZE)
        if (configTOTAL_HEAP_SIZE < 10241 && ret == MEMORY_E)
            PRINTF("Heap size set with configTOTAL_HEAP_SIZE is likely too "
                "small\r\n");
    #endif
        return ret;
    }

    PRINTF("Successfully created a new key\r\n");
    return ret;
}


static int CreateCert(WC_RNG* rng, byte* out, int* outSz,
        byte* keyOut, int* keySz)
{
    ecc_key key;
    int ret;
    Cert cert;

    ret = CreateKey(&key, rng);

    /* make cert with key */
    if (ret == 0) {
        wc_InitCert(&cert);

        (void)XSTRNCPY(cert.subject.country, "US", CTC_NAME_SIZE);
        (void)XSTRNCPY(cert.subject.state, "MT", CTC_NAME_SIZE);
        (void)XSTRNCPY(cert.subject.locality, "Bozeman", CTC_NAME_SIZE);
        (void)XSTRNCPY(cert.subject.org, "yourOrgNameHere", CTC_NAME_SIZE);
        (void)XSTRNCPY(cert.subject.unit, "yourUnitNameHere", CTC_NAME_SIZE);
        (void)XSTRNCPY(cert.subject.commonName, "www.yourDomain.com",
            CTC_NAME_SIZE);
        (void)XSTRNCPY(cert.subject.email, "yourEmail@yourDomain.com",
            CTC_NAME_SIZE);

        cert.selfSigned = 1;
        cert.isCA       = 1;
        cert.sigType    = CTC_SHA256wECDSA;

        ret = wc_SetSubjectKeyIdFromPublicKey(&cert, NULL, &key);
        if (ret != 0) {
            PRINTF("Set subject key id from public key failed: %d\r\n", ret);
        }
    }

    if (ret == 0) {
        ret = wc_MakeCert(&cert, out, *outSz, NULL, &key, rng);
        if (ret < 0) {
            PRINTF("MakeCert failed : %d [%s]\r\n", ret,
                wc_GetErrorString(ret));
        }
    }

    if (ret >= 0) {
        ret = wc_SignCert(cert.bodySz, cert.sigType, out, *outSz, NULL,
                           &key, rng);
        if (ret > 0) {
            *outSz = ret;
            ret = 0;
        }
        else {
            PRINTF("Sign cert. failed: %d\r\n", ret);
        }
    }

    /* return DER version of key */
    if (ret == 0) {
        PRINTF("\n\rExporting Key\r\n");
        ret = wc_EccKeyToDer(&key, keyOut, *keySz);
        if (ret <= 0) {
            PRINTF("Key To DER failed: %d\r\n", ret);
        }
        else {
            *keySz = (word32)ret;
        }
    }

    return ret;
}


static int EnvelopData(WC_RNG* rng, byte* in, int inSz,
        byte* cert, int certSz,
        byte* key, int keySz,
        byte* out, int* outSz)
{
    int ret = MEMORY_E;
    PKCS7* pkcs7;

    /* create envelope */
    pkcs7 = wc_PKCS7_New(NULL, WOLFSSL_CAAM_DEVID);
    if (pkcs7 != NULL) {
        pkcs7->content   = in;
        pkcs7->contentSz = inSz;
        pkcs7->contentOID = DATA;
        pkcs7->encryptOID = AES256CBCb;
        pkcs7->rng = rng;

        pkcs7->keyWrapOID  = AES256_WRAP;
        pkcs7->keyAgreeOID = dhSinglePass_stdDH_sha256kdf_scheme;

        ret = wc_PKCS7_AddRecipient_KARI(pkcs7, cert, certSz, AES256_WRAP,
            dhSinglePass_stdDH_sha256kdf_scheme,
            NULL, 0, 0);
        if (ret < 0) {
            PRINTF("PKCS7 add recipient failed: %d\r\n", ret);
        }
    }

    if (ret >= 0) {
        ret = wc_PKCS7_EncodeEnvelopedData(pkcs7, out, *outSz);
        if (ret > 0) {
            *outSz = ret;
        }
        else {
            PRINTF("Encode enveloped data failed: %d\r\n", ret);
        }
    }
    wc_PKCS7_Free(pkcs7);

    return ret;
}

static int SignBundle(WC_RNG* rng, byte* content, int contentSz,
        byte* cert, int certSz,
        byte* key, int keySz,
        byte* out, int* outSz)
{
    PKCS7* pkcs7;
    int ret = MEMORY_E;

    pkcs7 = wc_PKCS7_New(NULL, WOLFSSL_CAAM_DEVID);
    if (pkcs7 != NULL) {
        wc_PKCS7_InitWithCert(pkcs7, cert, certSz);

        pkcs7->content = content;
        pkcs7->contentSz = contentSz;
        pkcs7->privateKey = key;
        pkcs7->privateKeySz = keySz;
        pkcs7->encryptOID = ECDSAk;
        pkcs7->hashOID = SHA256h;
        pkcs7->rng = rng;

        ret = wc_PKCS7_EncodeSignedData(pkcs7, out, *outSz);
        if (ret > 0) {
            *outSz = ret;
        }
        else {
            PRINTF("Encode signed data failed: %d\r\n", ret);
        }
    }

    wc_PKCS7_Free(pkcs7);
    return ret;
}


static int DecodeEnvelope(WC_RNG* rng, byte* content, int contentSz,
        byte* cert, int certSz, byte* key, int keySz)
{
    int i;
    int ret = 0;
    PKCS7* pkcs7;
    byte decoded[LARGE_TMP_SIZE];
    int decodedSz = LARGE_TMP_SIZE;

    pkcs7 = wc_PKCS7_New(NULL, WOLFSSL_CAAM_DEVID);
    if (pkcs7 == NULL) {
        ret = MEMORY_E;
    }

    if (ret == 0) {
        wc_PKCS7_InitWithCert(pkcs7, cert, certSz);
        ret = wc_PKCS7_SetKey(pkcs7, key, keySz);
        pkcs7->rng = rng;
    }

    if (ret == 0) {
        decodedSz = wc_PKCS7_DecodeEnvelopedData(pkcs7, content,
            contentSz, decoded, decodedSz);
    }

    if (ret == 0 && decodedSz > 0) {
        PRINTF("\r\nDecoded Data : ");
        for (i = 0; i < decodedSz; i++)
            PRINTF("%c", decoded[i]);
        PRINTF("\r\n");
    }

    wc_PKCS7_Free(pkcs7);
    return ret;
}

static int VerifyBundle(WC_RNG* rng, byte* cert, int certSz,
        byte* key, int keySz, byte* in, int inSz)
{
    PKCS7* pkcs7;
    int ret = 0;

    pkcs7 = wc_PKCS7_New(NULL, WOLFSSL_CAAM_DEVID);
    if (pkcs7 == NULL) {
        ret = MEMORY_E;
    }

    if (ret == 0) {
        wc_PKCS7_InitWithCert(pkcs7, cert, certSz);
        ret = wc_PKCS7_VerifySignedData(pkcs7, in, inSz);
    }

    if (ret < 0) {
        PRINTF("Verify failed with ret = %d\r\n", ret);
    }
    else {
        PRINTF("Verified PKCS7 bundle\r\n");
        DecodeEnvelope(rng, pkcs7->content, pkcs7->contentSz, cert, certSz, key,
            keySz);
    }

    wc_PKCS7_Free(pkcs7);
    return 0;
}


int main(void)
{
    int ret;

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    SetDate();
    if (wolfCrypt_Init() != 0) {
        PRINTF("Issue with wolfCrypt init!!!\r\n");
    }
    else {
        WC_RNG rng;
        byte cert[LARGE_TMP_SIZE];
        int certSz = LARGE_TMP_SIZE;
        byte key[1024];
        int keySz = 1024;
        byte msg[] = "Test message to envelop";
        byte env[LARGE_TMP_SIZE];
        int envSz = LARGE_TMP_SIZE;
        byte bundle[LARGE_TMP_SIZE];
        int bundleSz = LARGE_TMP_SIZE;

        wc_InitRng(&rng);

        ret = CreateCert(&rng, cert, &certSz, key, &keySz);
        if (ret >= 0) {
            ret = EnvelopData(&rng, msg, sizeof(msg), cert, certSz,
                key, keySz, env, &envSz);
        }

        if (ret >= 0) {
            ret = SignBundle(&rng, env, envSz, cert, certSz,
                key, keySz, bundle, &bundleSz);
        }

//        uncomment to print hex of PKCS7 bundle
//        if (ret >= 0) {
//            PRINTF("\r\nPKCS7 BUNDLE : ");
//            for (i = 0; i < bundleSz; i++)
//                PRINTF("%02X", bundle[i]);
//            PRINTF("\r\n");
//        }

        /* verify and decode */
        if (ret >= 0) {
            ret = VerifyBundle(&rng, cert, certSz, key, keySz, bundle, bundleSz);
        }

        wc_InitRng(&rng);

        if (ret >= 0) {
            PRINTF("Done\r\n");
        }
        else {
            PRINTF("Failed!\r\n");
        }
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
