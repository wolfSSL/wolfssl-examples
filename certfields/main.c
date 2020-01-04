/* main.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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

/*
 * Example of parsing a DER-encoded certificate and extracting
 * public key and subject name information.
 *
 */

#include <stdio.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/asn.h>

static void err_sys(const char* msg, int ret)
{
    if (ret) {
        printf("ERROR: %s, ret = %d\n", msg, ret);
    } else {
        printf("ERROR: %s\n", msg);
    }
    exit(EXIT_FAILURE);
}

int main(void)
{
    int   ret, i;
    int   sigType;
    int   nameSz;
    int   derCertSz;
    byte  derCert[4096];
    word32 idx;
    FILE* file;

    RsaKey pubKey;
    WOLFSSL_X509* cert;
    WOLFSSL_EVP_PKEY* pubKeyTmp;
    WOLFSSL_X509_NAME* name;

    char commonName[80];
    char countryName[80];
    char localityName[80];
    char stateName[80];
    char orgName[80];
    char orgUnit[80];

    /* ------ PARSE ORIGINAL SELF-SIGNED CERTIFICATE ------ */

    /* open and read DER-formatted cert into buffer */
    file = fopen("../certs/client-cert.der", "rb");
    if (!file)
        err_sys("can't open client certificate", 0);

    derCertSz = fread(derCert, 1, sizeof(derCert), file);
    fclose(file);

    /* convert cert from DER to internal WOLFSSL_X509 struct */
    cert = wolfSSL_X509_d2i(&cert, derCert, derCertSz);
    if (cert == NULL)
        err_sys("Failed to convert DER to WOLFSSL_X509", 0);

    /* ------ EXTRACT CERTIFICATE ELEMENTS ------ */

    /* extract PUBLIC KEY from cert */
    pubKeyTmp = wolfSSL_X509_get_pubkey(cert);
    if (pubKeyTmp == NULL)
        err_sys("wolfSSL_X509_get_pubkey failed", 0);

    wc_InitRsaKey(&pubKey, 0);
    idx = 0;
    ret = wc_RsaPublicKeyDecode((byte*)pubKeyTmp->pkey.ptr, &idx, &pubKey,
                                pubKeyTmp->pkey_sz);
    if (ret != 0)
        err_sys("wc_RsaPublicKeyDecode failed", ret);

    printf("PUBLIC KEY:\n");
    for (i = 0; i < pubKeyTmp->pkey_sz; i++) {
        printf("%02X", pubKeyTmp->pkey.ptr[i] & 0xFF);
    } printf("\n");

    /* extract signatureType */
    sigType = wolfSSL_X509_get_signature_type(cert);
    if (sigType == 0)
        err_sys("wolfSSL_X509_get_signature_type failed", 0);
    printf("SIG TYPE = %d\n", sigType);

    /* extract subjectName info */
    name = wolfSSL_X509_get_subject_name(cert);
    if (name == NULL)
        err_sys("wolfSSL_X509_get_subject_name failed", 0);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, ASN_COMMON_NAME,
                                            commonName, sizeof(commonName));
    printf("CN = %s (%d)\n", commonName, nameSz);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, ASN_COUNTRY_NAME,
                                            countryName, sizeof(countryName));
    printf("COUNTRY = %s (%d)\n", countryName, nameSz);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, ASN_LOCALITY_NAME,
                                            localityName, sizeof(localityName));
    printf("LOCALITY = %s (%d)\n", localityName, nameSz);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, ASN_STATE_NAME,
                                            stateName, sizeof(stateName));
    printf("STATE = %s (%d)\n", stateName, nameSz);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, ASN_ORG_NAME,
                                            orgName, sizeof(orgName));
    printf("ORG = %s (%d)\n", orgName, nameSz);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, ASN_ORGUNIT_NAME,
                                            orgUnit, sizeof(orgUnit));
    printf("ORG UNIT = %s (%d)\n", orgUnit, nameSz);

    wolfSSL_EVP_PKEY_free(pubKeyTmp);
    wolfSSL_X509_free(cert);

    return 0;
}

