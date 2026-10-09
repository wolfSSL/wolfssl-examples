/* certvfy.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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
#include <time.h>

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>

#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define MAX_DER_SZ          4096

int load_file(const char* name, byte* buf, int bufSz)
{
    FILE* file;

    file = fopen(name, "rb");
    if (file == NULL) {
        return 0;
    }
    bufSz = fread(buf, 1, bufSz, file);
    fclose(file);

    return bufSz;
}

/* Convert an encoded certificate date (tag, length and value) to time_t. */
static int cert_date_to_time(const byte* certDate, int certDateSz, time_t* t)
{
    const byte* date;
    byte format;
    int length;
    struct tm tm;
    int ret;

    ret = wc_GetDateInfo(certDate, certDateSz, &date, &format, &length);
    if (ret == 0) {
        XMEMSET(&tm, 0, sizeof(tm));
        ret = wc_GetDateAsCalendarTime(date, length, format, &tm);
    }
    if (ret == 0) {
        /* Certificate dates are UTC. */
        *t = timegm(&tm);
    }
    return ret;
}

/* Check the current time is within the certificate's validity period. */
static int check_cert_dates(DecodedCert* cert)
{
    time_t now = time(NULL);
    time_t notBefore;
    time_t notAfter;
    int ret;

    ret = cert_date_to_time(cert->beforeDate, cert->beforeDateLen, &notBefore);
    if (ret == 0) {
        ret = cert_date_to_time(cert->afterDate, cert->afterDateLen, &notAfter);
    }
    if (ret == 0 && now < notBefore) {
        ret = ASN_BEFORE_DATE_E;
    }
    if (ret == 0 && now > notAfter) {
        ret = ASN_AFTER_DATE_E;
    }
    return ret;
}

int main(void)
{
    int res = 0;
    int ret;

    const char* caCert     = "../certs/ca-cert.der";
    const char* verifyCert = "../certs/server-cert.der";

    byte caDer[MAX_DER_SZ];
    int caDerSz;
    byte certDer[MAX_DER_SZ];
    int certDerSz;

    DecodedCert ca;
    DecodedCert cert;

    XMEMSET(&ca, 0, sizeof(ca));
    XMEMSET(&cert, 0, sizeof(cert));

    wolfCrypt_Init();

    /* Load the DER encoded CA certificate. */
    caDerSz = load_file(caCert, caDer, (int)sizeof(caDer));
    if (caDerSz == 0) {
        printf("Failed to load CA file\n");
        res = 1;
        goto exit;
    }

    /* Put the CA certificate data into the object. */
    wc_InitDecodedCert(&ca, caDer, caDerSz, NULL);
    /* Parse fields of the certificate. */
    ret = wc_ParseCert(&ca, CERT_TYPE, 0, NULL);
    if (ret != 0) {
        printf("Parsing CA failed: %s (%d)\n", wc_GetErrorString(ret), ret);
        res = 1;
        goto exit;
    }
    /* Load the DER encoded certificate to verify. */
    certDerSz = load_file(verifyCert, certDer, (int)sizeof(certDer));
    if (certDerSz == 0) {
        printf("Failed to load certificate file\n");
        res = 1;
        goto exit;
    }

    /* Put the certificate data into the object. */
    wc_InitDecodedCert(&cert, certDer, certDerSz, NULL);
    /* Parse the certificate. A wolfCrypt only build has no certificate
     * manager to look the CA up in, so the validity period, issuer and
     * signature are all checked below. */
    ret = wc_ParseCert(&cert, CERT_TYPE, 0, NULL);
    if (ret != 0) {
        printf("Parsing certificate failed: %s (%d)\n", wc_GetErrorString(ret),
            ret);
        res = 1;
        goto exit;
    }
    /* The certificate must be within its validity period. */
    ret = check_cert_dates(&cert);
    if (ret != 0) {
        printf("Verification failed: %s (%d)\n", wc_GetErrorString(ret), ret);
        res = 1;
        goto exit;
    }
    /* The certificate must have been issued by the CA. */
    if (XMEMCMP(cert.issuerHash, ca.subjectHash, KEYID_SIZE) != 0) {
        printf("Verification failed: issuer is not the CA\n");
        res = 1;
        goto exit;
    }
    /* Verify the signature of the certificate with the CA's public key. */
    ret = wc_CheckCertSigPubKey(certDer, certDerSz, NULL, ca.publicKey,
        ca.pubKeySize, ca.keyOID);
    if (ret != 0) {
        printf("Verification failed: %s (%d)\n", wc_GetErrorString(ret), ret);
        res = 1;
        goto exit;
    }
    printf("Verification Successful!\n");

exit:
    wc_FreeDecodedCert(&cert);
    wc_FreeDecodedCert(&ca);
    wolfCrypt_Cleanup();
    return res;
}

