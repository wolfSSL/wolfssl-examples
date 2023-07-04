/* certgen_ca_example.c
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
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/wc_port.h>

#ifdef WOLFSSL_CAAM
    #include <wolfssl/wolfcrypt/port/caam/wolfcaam.h>
    static int devId = WOLFSSL_CAAM_DEVID;
#else
    static int devId = INVALID_DEVID;
#endif

#if defined(WOLFSSL_CERT_REQ) && defined(WOLFSSL_CERT_GEN) && \
    defined(WOLFSSL_KEY_GEN) && defined(HAVE_ECC)

#define HEAP_HINT NULL
#define LARGE_TEMP_SZ 4096

static int do_cagen(int argc, char** argv)
{
    int ret = 0;

    Cert newCert;

    FILE* file;
    int derBufSz;
    int caKeySz = 4096;

    byte* derBuf   = NULL;
    byte* pemBuf   = NULL;

    /* for MakeCert and SignCert */
    WC_RNG rng;
    RsaKey newKey;
    int initRng = 0, initNewKey = 0;
    char newCertOutput[] = "./ca-rsa-cert.der";
    char newKeyOutput[] = "./ca-rsa-key.der";

#ifdef WOLFSSL_DER_TO_PEM
    char pemOutput[] = "./ca-rsa-cert.pem";
    char pemKeyOutput[] = "./ca-rsa-key.pem";
    int pemBufSz;
#endif

    ret = wolfCrypt_Init();
    if (ret != 0) goto exit;

    ret = wc_InitRng(&rng);
    if (ret != 0) goto exit;
    initRng = 1;

    printf("Creating the CA RSA private key of size %d\n", caKeySz);
    ret = wc_InitRsaKey_ex(&newKey, HEAP_HINT, devId);
    if (ret != 0) goto exit;
    initNewKey = 1;

    ret = wc_MakeRsaKey(&newKey, caKeySz, WC_RSA_EXPONENT, &rng);
    if (ret != 0) goto exit;

#ifdef WOLFSSL_CAAM
    printf("Black key value = %u\n", newKey.blackKey);
#endif

    printf("Successfully created CA Key\n\n");

    derBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (derBuf == NULL) goto exit;

    /*------------------------------------------------------------------------*/
    /* write the new key to file in der format */
    /*------------------------------------------------------------------------*/
    printf("Writing newly generated DER key to file \"%s\"\n",
        newKeyOutput);
    file = fopen(newKeyOutput, "wb");
    if (!file) {
        printf("failed to open file: %s\n", newKeyOutput);
        goto exit;
    }

    ret = wc_RsaKeyToDer(&newKey, derBuf, LARGE_TEMP_SZ);
    if (ret < 0) {
        goto exit;
    }
    derBufSz = ret;

    ret = (int)fwrite(derBuf, 1, derBufSz, file);
    fclose(file);
    printf("Successfully output %d bytes\n", ret);

#ifdef WOLFSSL_DER_TO_PEM
    /*------------------------------------------------------------------------*/
    /* convert the der to a pem and write it to a file */
    /*------------------------------------------------------------------------*/
    printf("Convert the DER key to PEM formatted key\n");

    pemBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (pemBuf == NULL) goto exit;
    XMEMSET(pemBuf, 0, LARGE_TEMP_SZ);

    pemBufSz = wc_DerToPem(derBuf, derBufSz, pemBuf, LARGE_TEMP_SZ, PRIVATEKEY_TYPE);
    if (pemBufSz < 0) goto exit;

    printf("Resulting PEM buffer is %d bytes\n", pemBufSz);

    file = fopen(pemKeyOutput, "wb");
    if (!file) {
        printf("failed to open file: %s\n", pemKeyOutput);
        goto exit;
    }
    fwrite(pemBuf, 1, pemBufSz, file);
    fclose(file);
    printf("Successfully converted the DER to PEM to \"%s\"\n\n",
        pemKeyOutput);
    XFREE(pemBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    pemBuf = NULL;
#endif

    /*------------------------------------------------------------------------*/
    /* Create a new certificate using SUBJECT information from ca cert
     * for ISSUER information in generated cert */
    /*------------------------------------------------------------------------*/
    printf("Setting up new cert\n");

    wc_InitCert(&newCert);

    strncpy(newCert.subject.country, "US", CTC_NAME_SIZE);
    strncpy(newCert.subject.state, "MT", CTC_NAME_SIZE);
    strncpy(newCert.subject.locality, "Bozeman", CTC_NAME_SIZE);
    strncpy(newCert.subject.org, "yourOrgNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.unit, "yourUnitNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.commonName, "www.yourDomain.com", CTC_NAME_SIZE);
    strncpy(newCert.subject.email, "yourEmail@yourDomain.com", CTC_NAME_SIZE);

    newCert.isCA    = 1;
    newCert.sigType = CTC_SHA256wRSA;

    ret = wc_MakeSelfCert(&newCert, derBuf, LARGE_TEMP_SZ, &newKey, &rng);
    if (ret < 0) goto exit;
    printf("Make Self Cert returned %d\n", ret);
    derBufSz = ret;

    printf("Successfully created new ca certificate\n\n");

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

    if (initNewKey)
        wc_FreeRsaKey(&newKey);
    if (initRng) {
        wc_FreeRng(&rng);
    }

    if (ret == 0)
        printf("Tests passed\n");
    else
        printf("Failure code was %d\n", ret);

    wolfCrypt_Cleanup();
    return ret;
}
#endif

int main(int argc, char** argv)
{
#if !defined(WOLFSSL_CERT_REQ) || !defined(WOLFSSL_CERT_GEN) || \
    !defined(WOLFSSL_KEY_GEN) || defined(NO_RSA)
    printf("Please compile wolfSSL with --enable-certreq --enable-certgen "
           "--enable-keygen --enable-rsa\n");
    return 0;
#else
    return do_cagen(argc, argv);
#endif
}
