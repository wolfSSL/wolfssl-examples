/* ocsp-request-response.c
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

/* Example: Generate and verify raw DER OCSP requests and responses using the
 * wolfSSL OCSP Responder API. No networking - pure encode/decode in memory. */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/ocsp.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/asn_public.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(HAVE_OCSP) && defined(HAVE_OCSP_RESPONDER) && \
    !defined(NO_FILESYSTEM)

#include "ocsp-load-certs.h"

/* Default certificate paths - these are the wolfSSL test certs */
#ifndef CA_CERT
#define CA_CERT    "../../certs/ca-cert.pem"
#endif
#ifndef CA_KEY
#define CA_KEY     "../../certs/ca-key.pem"
#endif
#ifndef SERVER_CERT
#define SERVER_CERT "../../certs/server-cert.pem"
#endif

#define MAX_DER_SZ    4096
#define MAX_RESP_SZ   4096

static void HexDump(const char* label, const unsigned char* data, int sz)
{
    int i;
    printf("%s (%d bytes):\n  ", label, sz);
    for (i = 0; i < sz && i < 64; i++) {
        printf("%02x", data[i]);
        if ((i + 1) % 32 == 0)
            printf("\n  ");
    }
    if (sz > 64)
        printf("...");
    printf("\n");
}

int main(int argc, char** argv)
{
    int ret;

    /* Certificate buffers */
    byte* caCertDer = NULL;
    int caCertDerSz = 0;
    byte* caKeyDer = NULL;
    int caKeyDerSz = 0;
    byte* serverCertDer = NULL;
    int serverCertDerSz = 0;

    /* DecodedCert for the server cert (to build the OCSP request) */
    DecodedCert serverCert;
    int serverCertInit = 0;

    /* DecodedCert for the CA cert (to get its subject for SetCertStatus) */
    DecodedCert caCert;
    int caCertInit = 0;
    char caSubject[256] = {0};
    word32 caSubjectSz = sizeof(caSubject);

    /* OCSP request */
    OcspRequest* req = NULL;
    unsigned char reqDer[MAX_DER_SZ];
    int reqDerSz;

    /* OCSP responder */
    OcspResponder* responder = NULL;

    /* OCSP response */
    unsigned char respDer[MAX_RESP_SZ];
    word32 respDerSz;

    /* For verification */
    WOLFSSL_CERT_MANAGER* cm = NULL;
    WOLFSSL_OCSP* ocsp = NULL;

    const char* caCertFile = CA_CERT;
    const char* caKeyFile = CA_KEY;
    const char* serverCertFile = SERVER_CERT;

    (void)argc;
    (void)argv;

    printf("=== OCSP Request/Response Example ===\n\n");

    ret = wolfSSL_Init();
    if (ret != WOLFSSL_SUCCESS) {
        printf("wolfSSL_Init failed\n");
        return 1;
    }

    /* --- Step 1: Load certificates --- */
    printf("Step 1: Loading certificates...\n");

    caCertDer = LoadCertDer(caCertFile, &caCertDerSz);
    if (!caCertDer) {
        printf("Error loading CA cert: %s\n", caCertFile);
        ret = -1;
        goto cleanup;
    }
    printf("  CA cert: %d bytes DER\n", caCertDerSz);

    caKeyDer = LoadKeyDer(caKeyFile, &caKeyDerSz);
    if (!caKeyDer) {
        printf("Error loading CA key: %s\n", caKeyFile);
        ret = -1;
        goto cleanup;
    }
    printf("  CA key:  %d bytes DER\n", caKeyDerSz);

    serverCertDer = LoadCertDer(serverCertFile, &serverCertDerSz);
    if (!serverCertDer) {
        printf("Error loading server cert: %s\n", serverCertFile);
        ret = -1;
        goto cleanup;
    }
    printf("  Server cert: %d bytes DER\n\n", serverCertDerSz);

    /* --- Step 2: Parse the server cert and build an OCSP request --- */
    printf("Step 2: Building OCSP request from server certificate...\n");

    /* Parse with the CA loaded so that issuerKeyHash gets populated.
     * The issuerKeyHash is derived from the CA's public key and is needed
     * for the OCSP request to match the responder's signer. */
    {
        WOLFSSL_CERT_MANAGER* tmpCm = wolfSSL_CertManagerNew();
        if (tmpCm == NULL) {
            printf("Error creating temp CertManager\n");
            ret = -1;
            goto cleanup;
        }
        ret = wolfSSL_CertManagerLoadCABuffer(tmpCm, caCertDer,
                  caCertDerSz, SSL_FILETYPE_ASN1);
        if (ret != WOLFSSL_SUCCESS) {
            printf("Error loading CA into temp CertManager: %d\n", ret);
            wolfSSL_CertManagerFree(tmpCm);
            goto cleanup;
        }
        ret = 0; /* Reset from WOLFSSL_SUCCESS (1) to 0 for error paths */
        wc_InitDecodedCert(&serverCert, serverCertDer,
                           (word32)serverCertDerSz, NULL);
        serverCertInit = 1;
        ret = wc_ParseCert(&serverCert, CERT_TYPE, 1, tmpCm);
        wolfSSL_CertManagerFree(tmpCm);
    }
    if (ret != 0) {
        printf("Error parsing/verifying server cert: %d\n", ret);
        goto cleanup;
    }

    /* Allocate and initialize the OCSP request from the parsed cert.
     * wc_InitOcspRequest fills in issuerHash, issuerKeyHash, and serial
     * from the DecodedCert automatically. */
    req = wc_OcspRequest_new(NULL);
    if (req == NULL) {
        printf("Error allocating OCSP request\n");
        ret = -1;
        goto cleanup;
    }

    ret = wc_InitOcspRequest(req, &serverCert, 0, NULL);
    if (ret != 0) {
        printf("Error initializing OCSP request: %d\n", ret);
        goto cleanup;
    }

    /* Encode the OCSP request into DER format */
    reqDerSz = wc_EncodeOcspRequest(req, reqDer, sizeof(reqDer));
    if (reqDerSz <= 0) {
        printf("Error encoding OCSP request: %d\n", reqDerSz);
        ret = reqDerSz;
        goto cleanup;
    }

    HexDump("  OCSP Request DER", reqDer, reqDerSz);
    printf("\n");

    /* --- Step 3: Set up the OCSP responder --- */
    printf("Step 3: Setting up OCSP responder...\n");

    /* Parse the CA cert to get its subject name */
    wc_InitDecodedCert(&caCert, caCertDer, (word32)caCertDerSz, NULL);
    caCertInit = 1;
    ret = wc_ParseCert(&caCert, CERT_TYPE, 0, NULL);
    if (ret != 0) {
        printf("Error parsing CA cert: %d\n", ret);
        goto cleanup;
    }

    ret = wc_GetDecodedCertSubject(&caCert, caSubject, &caSubjectSz);
    if (ret != 0) {
        printf("Error getting CA subject: %d\n", ret);
        goto cleanup;
    }
    printf("  CA Subject: %s\n", caSubject);

    /* Create the responder. The second argument controls whether the CA cert
     * is included in responses (1=yes). */
    responder = wc_OcspResponder_new(NULL, 1);
    if (responder == NULL) {
        printf("Error creating OCSP responder\n");
        ret = -1;
        goto cleanup;
    }

    /* Register the CA cert and key as a signer. When the CA signs directly
     * (not using a delegated responder cert), pass NULL/0 for the last two
     * parameters. */
    ret = wc_OcspResponder_AddSigner(responder,
                                     caCertDer, (word32)caCertDerSz,
                                     caKeyDer, (word32)caKeyDerSz,
                                     NULL, 0);
    if (ret != 0) {
        printf("Error adding signer: %d\n", ret);
        goto cleanup;
    }
    printf("  Signer added (CA as direct signer)\n");

    /* Set the status for the server certificate's serial number.
     * In production, you would populate this from a database or CRL.
     * Here we mark it as CERT_GOOD with 24-hour validity. */
    {
        byte serial[32];
        word32 serialSz = sizeof(serial);

        ret = wc_GetDecodedCertSerial(&serverCert, serial, &serialSz);
        if (ret != 0) {
            printf("Error getting server cert serial: %d\n", ret);
            goto cleanup;
        }

        HexDump("  Server cert serial", serial, (int)serialSz);

        ret = wc_OcspResponder_SetCertStatus(responder,
                                             caSubject, caSubjectSz,
                                             serial, serialSz,
                                             CERT_GOOD,
                                             0, /* revocationTime (unused) */
                                             WC_CRL_REASON_UNSPECIFIED,
                                             86400 /* validityPeriod secs */);
        if (ret != 0) {
            printf("Error setting cert status: %d\n", ret);
            goto cleanup;
        }
        printf("  Certificate status set to GOOD (24h validity)\n\n");
    }

    /* --- Step 4: Generate OCSP response --- */
    printf("Step 4: Generating OCSP response...\n");

    respDerSz = sizeof(respDer);
    ret = wc_OcspResponder_WriteResponse(responder,
                                         reqDer, (word32)reqDerSz,
                                         respDer, &respDerSz);
    if (ret != 0) {
        printf("Error generating OCSP response: %d\n", ret);
        goto cleanup;
    }

    HexDump("  OCSP Response DER", respDer, (int)respDerSz);
    printf("\n");

    /* --- Step 5: Verify the OCSP response --- */
    printf("Step 5: Verifying OCSP response...\n");

    cm = wolfSSL_CertManagerNew();
    if (cm == NULL) {
        printf("Error creating CertManager\n");
        ret = -1;
        goto cleanup;
    }

    ret = wolfSSL_CertManagerLoadCABuffer(cm, caCertDer, caCertDerSz,
                                          SSL_FILETYPE_ASN1);
    if (ret != WOLFSSL_SUCCESS) {
        printf("Error loading CA into CertManager: %d\n", ret);
        goto cleanup;
    }
    ret = 0; /* Reset from WOLFSSL_SUCCESS (1) to 0 for error paths */

    ocsp = wc_NewOCSP(cm);
    if (ocsp == NULL) {
        printf("Error creating OCSP verifier\n");
        ret = -1;
        goto cleanup;
    }

    ret = wc_CheckCertOcspResponse(ocsp, &serverCert,
                                   respDer, (int)respDerSz, NULL);
    if (ret == 0) {
        printf("  OCSP response verification: SUCCESS\n");
    }
    else {
        printf("  OCSP response verification FAILED: %d\n", ret);
    }

    /* --- Step 6: Demonstrate a REVOKED status --- */
    printf("\nStep 6: Demonstrating REVOKED status...\n");
    {
        byte serial[32];
        word32 serialSz = sizeof(serial);

        ret = wc_GetDecodedCertSerial(&serverCert, serial, &serialSz);
        if (ret != 0) {
            printf("Error getting server cert serial: %d\n", ret);
            goto cleanup;
        }

        /* Update the status to REVOKED */
        ret = wc_OcspResponder_SetCertStatus(responder,
                                             caSubject, caSubjectSz,
                                             serial, serialSz,
                                             CERT_REVOKED,
                                             time(NULL), /* revocationTime */
                                             WC_CRL_REASON_KEY_COMPROMISE,
                                             0);
        if (ret != 0) {
            printf("Error setting revoked status: %d\n", ret);
            goto cleanup;
        }

        respDerSz = sizeof(respDer);
        ret = wc_OcspResponder_WriteResponse(responder,
                                             reqDer, (word32)reqDerSz,
                                             respDer, &respDerSz);
        if (ret != 0) {
            printf("Error generating revoked response: %d\n", ret);
            goto cleanup;
        }

        HexDump("  REVOKED OCSP Response DER", respDer, (int)respDerSz);

        /* Verify - should indicate revoked */
        ret = wc_CheckCertOcspResponse(ocsp, &serverCert,
                                       respDer, (int)respDerSz, NULL);
        printf("  Verification result: %d (non-zero = revoked/failed)\n", ret);
    }

    /* --- Step 7: Demonstrate error response --- */
    printf("\nStep 7: Generating OCSP error response (unauthorized)...\n");
    {
        respDerSz = sizeof(respDer);
        ret = wc_OcspResponder_WriteErrorResponse(OCSP_UNAUTHORIZED,
                                                  respDer, &respDerSz);
        if (ret != 0) {
            printf("Error encoding error response: %d\n", ret);
            goto cleanup;
        }
        HexDump("  Error Response DER", respDer, (int)respDerSz);
    }

    printf("\n=== Example complete ===\n");
    ret = 0;

cleanup:
    if (ocsp)
        wc_FreeOCSP(ocsp);
    if (cm)
        wolfSSL_CertManagerFree(cm);
    if (responder)
        wc_OcspResponder_free(responder);
    if (req)
        wc_OcspRequest_free(req);
    if (serverCertInit)
        wc_FreeDecodedCert(&serverCert);
    if (caCertInit)
        wc_FreeDecodedCert(&caCert);
    free(caCertDer);
    if (caKeyDer) {
        /* Zero the CA private key material before releasing the buffer. */
        wc_ForceZero(caKeyDer, (word32)caKeyDerSz);
        free(caKeyDer);
    }
    free(serverCertDer);

    wolfSSL_Cleanup();
    return ret;
}

#else

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    printf("This example requires --enable-ocsp --enable-ocsp-responder\n");
    return 0;
}

#endif /* HAVE_OCSP && HAVE_OCSP_RESPONDER && !NO_FILESYSTEM */
