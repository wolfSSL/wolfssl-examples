/* csr_example.c
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

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/asn_public.h>

#define LARGE_TEMP_SZ 4096

enum {
    EC_KEY_TYPE = 0,
    RSA_KEY_TYPE = 1,
};

void usage(void);
int gen_csr(int type);

int gen_csr(int type)
{
#if !defined(WOLFSSL_CERT_REQ) || !defined(WOLFSSL_CERT_GEN) && \
    !defined(WOLFSSL_KEY_GEN)
  printf("ERROR: Please compile wolfSSL with --enable-certreq"
         " --enable-certgen --enable-keygen\n");
  return 0;
#else
    int ret;
    ecc_key ecKey;
    RsaKey rsaKey;
    WC_RNG rng;
    Cert req;
    byte der[LARGE_TEMP_SZ], pem[LARGE_TEMP_SZ];
    int  derSz, pemSz;

    XMEMSET(der, 0, LARGE_TEMP_SZ);
    XMEMSET(pem, 0, LARGE_TEMP_SZ);

    if (type == EC_KEY_TYPE) {
        ret = wc_ecc_init(&ecKey);
        (void) rsaKey; /* Not using rsaKey in EC_KEY_TYPE case */
    } else {
        ret = wc_InitRsaKey(&rsaKey, NULL);
        (void) ecKey; /* Not using ecKey in RSA_KEY_TYPE case */
    }
    if (ret != 0) {
        printf("Key initialization failed: %d\n", ret);
        goto exit;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("RNG initialization failed: %d\n", ret);
        goto exit;
    }

    if (type == EC_KEY_TYPE) {
        ret = wc_ecc_make_key_ex(&rng, 32, &ecKey, ECC_SECP256R1);
    } else {
        ret = wc_MakeRsaKey(&rsaKey, 2048, WC_RSA_EXPONENT, &rng);
    }
    if (ret != 0) {
        printf("Key generation failed: %d\n", ret);
        goto exit;
    }

    if (type == EC_KEY_TYPE) {
        ret = wc_EccKeyToDer(&ecKey, der, sizeof(der));
    } else {
        ret = wc_RsaKeyToDer(&rsaKey, der, sizeof(der));
    }
    if (ret <= 0) {
        printf("Key To DER failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

    memset(pem, 0, sizeof(pem));
    if (type == EC_KEY_TYPE) {
        ret = wc_DerToPem(der, derSz, pem, sizeof(pem), ECC_PRIVATEKEY_TYPE);
    } else {
        ret = wc_DerToPem(der, derSz, pem, sizeof(pem), PRIVATEKEY_TYPE);
    }
    if (ret <= 0) {
        printf("DER to PEM failed: %d\n", ret);
        goto exit;
    }
    pemSz = ret;
    printf("%s (%d)", pem, pemSz);

    ret = wc_InitCert(&req);
    if (ret != 0) {
        printf("Init Cert failed: %d\n", ret);
        goto exit;
    }
    strncpy(req.subject.country, "US", CTC_NAME_SIZE);
    strncpy(req.subject.state, "OR", CTC_NAME_SIZE);
    strncpy(req.subject.locality, "Portland", CTC_NAME_SIZE);
    strncpy(req.subject.org, "wolfSSL", CTC_NAME_SIZE);
    strncpy(req.subject.unit, "Development", CTC_NAME_SIZE);
    strncpy(req.subject.commonName, "www.wolfssl.com", CTC_NAME_SIZE);
    strncpy(req.subject.email, "info@wolfssl.com", CTC_NAME_SIZE);
    if (type == EC_KEY_TYPE) {
        ret = wc_MakeCertReq(&req, der, sizeof(der), NULL, &ecKey);
    } else {
        ret = wc_MakeCertReq(&req, der, sizeof(der), &rsaKey, NULL);
    }
    if (ret <= 0) {
        printf("Make Cert Req failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

    if (type == EC_KEY_TYPE) {
        req.sigType = CTC_SHA256wECDSA;
        ret = wc_SignCert(req.bodySz, req.sigType, der, sizeof(der), NULL,
                          &ecKey, &rng);
    } else {
        req.sigType = CTC_SHA256wRSA;
        ret = wc_SignCert(req.bodySz, req.sigType, der, sizeof(der), &rsaKey,
                          NULL, &rng);
    }
    if (ret <= 0) {
        printf("Sign Cert failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

    ret = wc_DerToPem(der, derSz, pem, sizeof(pem), CERTREQ_TYPE);
    if (ret <= 0) {
        printf("DER to PEM failed: %d\n", ret);
        goto exit;
    }
    pemSz = ret;
    printf("%s (%d)", pem, pemSz);

exit:
    if (type == EC_KEY_TYPE) {
        wc_ecc_free(&ecKey);
    } else {
        wc_FreeRsaKey(&rsaKey);
    }
    wc_FreeRng(&rng);

    return ret;
#endif
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        usage();
        return 1;
    }

    if (XSTRNCMP(argv[1], "rsa", 3) == 0)
        return gen_csr(RSA_KEY_TYPE);
    else if (XSTRNCMP(argv[1], "ecc", 3) == 0)
        return gen_csr(EC_KEY_TYPE);
    else
        usage();

    return -1;
}

void usage(void)
{
    printf("Invalid input supplied try one of the below examples\n");
    printf("Examples:\n\n");
    printf("./csr_example rsa\n");
    printf("./csr_example ecc\n");
}
