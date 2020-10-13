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
#include <wolfssl/openssl/x509v3.h>

enum {
    RSA_KEY_TYPE = 2,
    ECC_KEY_TYPE = 3,
};

static void err_sys(const char* msg, int ret)
{
    if (ret) {
        printf("ERROR: %s, ret = %d\n", msg, ret);
    } else {
        printf("ERROR: %s\n", msg);
    }
    exit(EXIT_FAILURE);
}


/* print out the public key */
static void printPublicKey(WOLFSSL_X509* cert)
{
    RsaKey  pubKeyRsa;
    ecc_key pubKeyEcc;
    byte *buf = NULL;
    int  bufSz;
    word32 idx = 0;
    int  keyType, i, ret;

#if 0
    /* if wanting EVP_PKEY structure for public key */
    WOLFSSL_EVP_PKEY* pubKeyTmp;
    pubKeyTmp = wolfSSL_X509_get_pubkey(cert);
    if (pubKeyTmp == NULL)
        err_sys("wolfSSL_X509_get_pubkey failed", 0);
    wolfSSL_EVP_PKEY_free(pubKeyTmp);
#endif
    wc_InitRsaKey(&pubKeyRsa, 0);
    wc_ecc_init(&pubKeyEcc);

    /* get public key buffer */
    ret = wolfSSL_X509_get_pubkey_buffer(cert, NULL, &bufSz);
    if (ret != WOLFSSL_SUCCESS) {
        printf("issue getting size of buffer needed\n");
        goto end_pk;
    }

    buf = (byte*)malloc(bufSz);
    ret = wolfSSL_X509_get_pubkey_buffer(cert, buf, &bufSz);
    if (ret != WOLFSSL_SUCCESS) {
        printf("issue getting public key buffer\n");
        goto end_pk;
    }

    /* buffer contains public key in the format used with certificates */
    printf("PUBLIC KEY: (With Certificate ASN1 Syntax)\n");
    for (i = 0; i < bufSz; i++) {
        printf("%02X", buf[i] & 0xFF);
    } printf("\n");

    /* transforming to other more raw versions of the key */
    keyType = wolfSSL_X509_get_pubkey_type(cert);
    if (keyType == RSAk) {
        byte *e, *n;
        word32 eSz, nSz;

        printf("\nPUBLIC KEY: (RSA e + n)\n");

        /* load buffer up as internal RSA structure */
        ret = wc_RsaPublicKeyDecode(buf, &idx, &pubKeyRsa, bufSz);
        if (ret != 0) {
            printf("Issue decoding RSA key\n");
            goto end_pk;
        }
        free(buf); buf = NULL;

        eSz = nSz = wc_RsaEncryptSize(&pubKeyRsa);
        e = (byte*)malloc(eSz);
        if (e == NULL) {
            goto end_pk;
        }

        n = (byte*)malloc(nSz);
        if (n == NULL) {
            free(e);
            goto end_pk;
        }

        ret = wc_RsaFlattenPublicKey(&pubKeyRsa, e, &eSz, n, &nSz);

        if (ret == 0) {
            printf("n: ");
            for (i = 0; i < nSz; i++) {
                printf("%02X", n[i] & 0xFF);
            } printf("\n");

            printf("e: ");
            for (i = 0; i < eSz; i++) {
                printf("%02X", e[i] & 0xFF);
            } printf("\n");
        }

        free(e);
        free(n);
    }
    else {
        printf("\nPUBLIC KEY: (EC x963 Format)\n");

        /* load buffer up as internal ECC structure */
        ret = wc_EccPublicKeyDecode(buf, &idx, &pubKeyEcc, bufSz);
        if (ret != 0) {
            printf("Issue decoding EC key\n");
            goto end_pk;
        }
        free(buf); buf = NULL;

        /* example of x963 format if wanted */
        ret = wc_ecc_export_x963(&pubKeyEcc, NULL, (word32*)&bufSz);
        if (ret != LENGTH_ONLY_E) {
            printf("issue getting x963 buffer size\n");
            goto end_pk;
        }
        buf = (byte*)malloc(bufSz);

        ret = wc_ecc_export_x963(&pubKeyEcc, buf, (word32*)&bufSz);
        if (ret != 0) {
            printf("issue getting x963 buffer\n");
            goto end_pk;
        }

        for (i = 0; i < bufSz; i++) {
            printf("%02X", buf[i] & 0xFF);
        } printf("\n");
    }
    printf("\n");

end_pk:
    free(buf);
    wc_FreeRsaKey(&pubKeyRsa);
    wc_ecc_free(&pubKeyEcc);
}


int main(int argc, char** argv)
{
    int   i;
    int   sigType;
    int   nameSz;
    int   derCertSz;
    byte  derCert[4096];
    FILE* file;
    char* certFile;
    int keyType;

    WOLFSSL_X509* cert;
    WOLFSSL_X509_NAME* name;

    char commonName[80];
    char countryName[80];
    char localityName[80];
    char stateName[80];
    char orgName[80];
    char orgUnit[80];

    STACK_OF(GENERAL_NAME)* sk;

    /* ------ PARSE ORIGINAL SELF-SIGNED CERTIFICATE ------ */

    if (argc < 3) {
        printf("USAGE:\n"
               "./app derCert keyType\n"
               "EXAMPLE(s):\n"
               "            ./app myCert.der RSA\n"
               "            ./app myCert.der ECC\n");
        err_sys("invalid input", -1);
    }
    certFile = argv[1]; /* certFile to check */
    keyType = (XMEMCMP(argv[2], "RSA", 3) == 0) ? RSA_KEY_TYPE :
                (XMEMCMP(argv[2], "ECC", 3) == 0) ? ECC_KEY_TYPE : 0; /* key */
    if (keyType == 0)
        err_sys("unsupported keyType", -1);

    /* open and read DER-formatted cert into buffer */
    file = fopen(certFile, "rb");
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
    printPublicKey(cert);

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

    sk = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);

    for (i = 0; i < sk_GENERAL_NAME_num(sk); i++) {
        GENERAL_NAME* gn = sk_GENERAL_NAME_value(sk, i);
        if (gn->type == GEN_DNS) {
            printf("DNS:%s\n", gn->d.dNSName->strData);
        } else if (gn->type == GEN_IPADD) {
            printf("IP:");
            int j;
            for (j = 0; j < gn->d.ip->length; j++) {
                if (j < gn->d.ip->length - 1)
                    printf("%d.", (int) gn->d.ip->strData[j]);
                else
                    printf("%d", (int) gn->d.ip->strData[j]);
            }
            printf("\n");
        } else {
            printf("Other type: %d\n", gn->type);
        }
    }

    wolfSSL_X509_free(cert);

    return 0;
}

