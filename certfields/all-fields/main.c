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

#ifdef OPENSSL_EXTRA
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
#endif

int main(int argc, char** argv)
{
    int   ret, i;
#ifdef OPENSSL_EXTRA
    int   sigType;
    int   nameSz;
    int   derCertSz;
    byte  derCert[4096];
    word32 idx;
    FILE* file;
    char* certFile;
    int keyType;

    RsaKey pubKeyRsa;
    ecc_key pubKeyEcc;
    WOLFSSL_X509* cert;
    WOLFSSL_EVP_PKEY* pubKeyTmp;
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
    pubKeyTmp = wolfSSL_X509_get_pubkey(cert);
    if (pubKeyTmp == NULL)
        err_sys("wolfSSL_X509_get_pubkey failed", 0);


    /* always initialize both key structs since both are free'd at the end */
    ret =  wc_InitRsaKey(&pubKeyRsa, NULL);
    ret |= wc_ecc_init(&pubKeyEcc);
    if (ret != 0)
        err_sys("init key failed", ret);

    idx = 0;
    if (keyType == RSA_KEY_TYPE) {
        ret = wc_RsaPublicKeyDecode((byte*)pubKeyTmp->pkey.ptr, &idx,
                                    &pubKeyRsa, pubKeyTmp->pkey_sz);
    } else {
        ret = wc_EccPublicKeyDecode((byte*)pubKeyTmp->pkey.ptr, &idx,
                                    &pubKeyEcc, pubKeyTmp->pkey_sz);
    }

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

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, NID_commonName,
                                            commonName, sizeof(commonName));
    printf("CN = %s (%d)\n", commonName, nameSz);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, NID_countryName,
                                            countryName, sizeof(countryName));
    printf("COUNTRY = %s (%d)\n", countryName, nameSz);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, NID_localityName,
                                            localityName, sizeof(localityName));
    printf("LOCALITY = %s (%d)\n", localityName, nameSz);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, NID_stateOrProvinceName,
                                            stateName, sizeof(stateName));
    printf("STATE = %s (%d)\n", stateName, nameSz);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, NID_organizationName,
                                            orgName, sizeof(orgName));
    printf("ORG = %s (%d)\n", orgName, nameSz);

    nameSz = wolfSSL_X509_NAME_get_text_by_NID(name, NID_organizationalUnitName,
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
                    printf("%u.", (uint8_t)gn->d.ip->strData[j]);
                else
                    printf("%u", (uint8_t)gn->d.ip->strData[j]);
            }
            printf("\n");
        } else {
            printf("Other type: %d\n", gn->type);
        }
    }

    wolfSSL_EVP_PKEY_free(pubKeyTmp);
    wolfSSL_X509_free(cert);
    wc_FreeRsaKey(&pubKeyRsa);
    wc_ecc_free(&pubKeyEcc);
#else
    (void) i;
    printf("Please configure wolfSSL with --enable-opensslextra\n");
    ret = -1;
#endif
    return ret;
}

