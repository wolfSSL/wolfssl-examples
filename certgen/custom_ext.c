/* custom_ext.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
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
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#if defined(WOLFSSL_ASN_TEMPLATE) && defined(WOLFSSL_CERT_GEN) && \
    defined(WOLFSSL_KEY_GEN) && defined(WOLFSSL_CUSTOM_OID) && \
    defined(HAVE_OID_ENCODING) && defined(WOLFSSL_CERT_EXT) && \
    !defined(NO_RSA)

#define HEAP_HINT NULL
#define LARGE_TEMP_SZ 4096

static int do_certgen(int argc, char** argv)
{
    int ret = 0;

    Cert newCert;

    FILE* file;
    char certToUse[] = "./ca-ecc-cert.der";
    char caKeyFile[] = "./ca-ecc-key.der";
    char newCertOutput[] = "./newCert.der";

    int derBufSz;
    int caKeySz;

    byte* derBuf   = NULL;
    byte* pemBuf   = NULL;
    byte* caKeyBuf = NULL;

    /* for MakeCert and SignCert */
    WC_RNG rng;
    ecc_key caKey;
    RsaKey newKey;
    word32 idx = 0;
    int initRng = 0, initCaKey = 0, initNewKey = 0;

#ifdef WOLFSSL_DER_TO_PEM
    char pemOutput[] = "./newCert.pem";
    int pemBufSz;
#endif

    /*------------------------------------------------------------------------*/
    /* open the CA der formatted certificate, we need to get it's subject line
     * to use in the new cert we're creating as the "Issuer" line */
    /*------------------------------------------------------------------------*/
    printf("Loading CA certificate\n");

    derBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (derBuf == NULL) goto exit;
    XMEMSET(derBuf, 0, LARGE_TEMP_SZ);

    file = fopen(certToUse, "rb");
    if (!file) {
        printf("failed to open file: %s\n", certToUse);
        goto exit;
    }
    derBufSz = fread(derBuf, 1, LARGE_TEMP_SZ, file);
    fclose(file);

    printf("Successfully read %d bytes from %s\n\n", derBufSz, certToUse);

    /*------------------------------------------------------------------------*/
    /* open caKey file and get the caKey, we need it to sign our new cert */
    /*------------------------------------------------------------------------*/
    printf("Loading the CA key\n");

    caKeyBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (caKeyBuf == NULL) goto exit;
    XMEMSET(caKeyBuf, 0, LARGE_TEMP_SZ);

    file = fopen(caKeyFile, "rb");
    if (!file) {
        printf("failed to open file: %s\n", caKeyFile);
        goto exit;
    }
    caKeySz = fread(caKeyBuf, 1, LARGE_TEMP_SZ, file);
    fclose(file);
    if (caKeySz <= 0) {
        printf("Failed to read caKey from file\n");
        goto exit;
    }

    printf("Successfully read %d bytes from %s\n", caKeySz, caKeyFile);

    wc_ecc_init(&caKey);
    initCaKey = 1;

    printf("Decoding the CA private key\n");
    idx = 0;
    ret = wc_EccPrivateKeyDecode(caKeyBuf, &idx, &caKey, (word32)caKeySz);
    if (ret != 0) goto exit;

    printf("Successfully loaded CA Key\n\n");

    /*------------------------------------------------------------------------*/
    /* Generate new private key to go with our new cert */
    /*------------------------------------------------------------------------*/
    ret = wc_InitRng(&rng);
    if (ret != 0) goto exit;
    initRng = 1;

    printf("Generating a new RSA key\n");
    ret = wc_InitRsaKey(&newKey, NULL);
    if (ret != 0) goto exit;
    initNewKey = 1;

    wc_MakeRsaKey(&newKey, 2048, WC_RSA_EXPONENT, &rng);
    if (ret != 0) goto exit;

    printf("Successfully created new RSA key\n\n");

    /*------------------------------------------------------------------------*/
    /* Create a new certificate using SUBJECT information from ca cert
     * for ISSUER information in generated cert */
    /*------------------------------------------------------------------------*/
    printf("Setting new cert issuer to subject of signer\n");

    wc_InitCert(&newCert);

    strncpy(newCert.subject.country, "US", CTC_NAME_SIZE);
    strncpy(newCert.subject.state, "MT", CTC_NAME_SIZE);
    strncpy(newCert.subject.locality, "Bozeman", CTC_NAME_SIZE);
    strncpy(newCert.subject.org, "yourOrgNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.unit, "yourUnitNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.commonName, "www.yourDomain.com", CTC_NAME_SIZE);
    strncpy(newCert.subject.email, "yourEmail@yourDomain.com", CTC_NAME_SIZE);

    newCert.isCA    = 0;
    newCert.sigType = CTC_SHA256wECDSA;

    ret = wc_SetIssuerBuffer(&newCert, derBuf, derBufSz);
    if (ret != 0) goto exit;

    ret = wc_SetCustomExtension(&newCert, 1, "1.2.3.4.5", (const byte *)"This is a critical extension", 28);
    if (ret < 0) goto exit;

    ret = wc_SetCustomExtension(&newCert, 0, "1.2.3.4.6", (const byte *)"This is NOT a critical extension", 32);
    if (ret < 0) goto exit;

    ret = wc_MakeCert(&newCert, derBuf, LARGE_TEMP_SZ, &newKey, NULL, &rng);
    if (ret < 0) goto exit;
    printf("Make Cert returned %d\n", ret);

    ret = wc_SignCert(newCert.bodySz, newCert.sigType, derBuf, LARGE_TEMP_SZ,
        NULL, &caKey, &rng);
    if (ret < 0) goto exit;
    printf("Signed Cert returned %d\n", ret);

    derBufSz = ret;

    printf("Successfully created new certificate\n\n");

    /*------------------------------------------------------------------------*/
    /* write the new cert to file in der format */
    /*------------------------------------------------------------------------*/
    printf("Writing newly generated DER certificate to file \"%s\"\n",
        newCertOutput);
    file = fopen(newCertOutput, "wb");
    if (!file) {
        printf("failed to open file: %s\n", newCertOutput);
        goto exit;
    }

    ret = (int)fwrite(derBuf, 1, derBufSz, file);
    fclose(file);
    printf("Successfully output %d bytes\n", ret);

#ifdef WOLFSSL_DER_TO_PEM
    /*------------------------------------------------------------------------*/
    /* convert the der to a pem and write it to a file */
    /*------------------------------------------------------------------------*/
    printf("Convert the DER cert to PEM formatted cert\n");

    pemBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (pemBuf == NULL) goto exit;
    XMEMSET(pemBuf, 0, LARGE_TEMP_SZ);

    pemBufSz = wc_DerToPem(derBuf, derBufSz, pemBuf, LARGE_TEMP_SZ, CERT_TYPE);
    if (pemBufSz < 0) goto exit;

    printf("Resulting PEM buffer is %d bytes\n", pemBufSz);

    file = fopen(pemOutput, "wb");
    if (!file) {
        printf("failed to open file: %s\n", pemOutput);
        goto exit;
    }
    fwrite(pemBuf, 1, pemBufSz, file);
    fclose(file);
    printf("Successfully converted the DER to PEM to \"%s\"\n\n",
        pemOutput);
#endif

    ret = 0; /* success */

exit:

    XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(pemBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(caKeyBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    if (initCaKey)
        wc_ecc_free(&caKey);
    if (initNewKey)
        wc_FreeRsaKey(&newKey);
    if (initRng) {
        wc_FreeRng(&rng);
    }

    if (ret == 0)
        printf("Tests passed\n");
    else
        printf("Failure code was %d\n", ret);
    return ret;
}

int main(int argc, char** argv)
{
    return do_certgen(argc, argv);
}

#else

int main(int argc, char** argv)
{
    printf("Please compile wolfSSL with --enable-asn=template --enable-certgen "
           "--enable-keygen CFLAGS=\"-DWOLFSSL_CUSTOM_OID -DHAVE_OID_ENCODING "
           "-DWOLFSSL_CERT_EXT\"");
    return 0;
}

#endif
