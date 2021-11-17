/* csr_sign.c
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
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define HEAP_HINT NULL
#define LARGE_TEMP_SZ 4096

/* Check if the internal asn API's are available */
#if defined(WOLFSSL_TEST_CERT) || defined(OPENSSL_EXTRA) || \
    defined(OPENSSL_EXTRA_X509_SMALL)
    #define HAVE_DECODEDCERT
#endif

static void usage(void)
{
    printf("Usage: ./csr_sign [type] [csr.pem] [ca-cert.pem] [ca-key.pem]\n");
    printf("Example:\n");
    printf("./csr_sign ecc ecc-csr.pem ca-ecc-cert.der ca-ecc-key.der\n");
}

static int do_csrsign(int argc, char** argv)
{
    int ret = 0;
    int type;

    Cert newCert;

    FILE* file;
    const char* typeStr = argv[1];
    const char* csrPemFile = argv[2];
    const char* caCertPemFile = argv[3];
    const char* caKeyPemFile = argv[4];
        
    const char* newCertOutput = "./newCert.der";
    const char* newCertPemFile = "./newCert.pem";

    int derSz = 0;
    int pemSz = 0;
    int caKeySz = 0;
    int caCertSz = 0;

    byte* derBuf   = NULL;
    byte* pemBuf   = NULL;
    byte* caKeyBuf = NULL;
    byte* caCertBuf = NULL;

#ifdef HAVE_ECC
    ecc_key newKey;
    ecc_key caKey;
#endif
    void* keyPtr = NULL;
    WC_RNG rng;
    int initRng = 0, initCaKey = 0, initNewKey = 0;

    if (XSTRNCMP(typeStr, "rsa", 3) == 0)
        type = RSA_TYPE;
    else if (XSTRNCMP(typeStr, "ecc", 3) == 0)
        type = ECC_TYPE;
    else if (XSTRNCMP(typeStr, "ed25519", 7) == 0)
        type = ED25519_TYPE;
    else
        return NOT_COMPILED_IN;

    derBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (derBuf == NULL) goto exit;
    XMEMSET(derBuf, 0, LARGE_TEMP_SZ);

    pemBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (pemBuf == NULL) goto exit;
    XMEMSET(pemBuf, 0, LARGE_TEMP_SZ);

    caKeyBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (caKeyBuf == NULL) goto exit;
    XMEMSET(caKeyBuf, 0, LARGE_TEMP_SZ);

    caCertBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (caCertBuf == NULL) goto exit;
    XMEMSET(caCertBuf, 0, LARGE_TEMP_SZ);


    /*---------------------------------------------------------------------------*/
    /* Loading the CA Certificate PEM File */
    /*---------------------------------------------------------------------------*/
    printf("Loading CA certificate\n");

    file = fopen(caCertPemFile, "rb");
    if (!file) {
        printf("failed to open file: %s\n", caCertPemFile);
        goto exit;
    }
    pemSz = fread(pemBuf, 1, LARGE_TEMP_SZ, file);
    fclose(file);
    printf("Successfully read %d bytes from %s\n\n", pemSz, caCertPemFile);

    ret = wc_CertPemToDer(pemBuf, pemSz, caCertBuf, LARGE_TEMP_SZ, CERT_TYPE);
    if (ret >= 0) {
        caCertSz = ret;
        ret = 0;
    }
    printf("Converted CA Cert PEM to DER %d bytes\n\n", caCertSz);

    /*---------------------------------------------------------------------------*/
    /* Load the CA Key PEM File */
    /*---------------------------------------------------------------------------*/
    printf("Loading the CA key\n");

    file = fopen(caKeyPemFile, "rb");
    if (!file) {
        printf("failed to open file: %s\n", caKeyPemFile);
        goto exit;
    }
    pemSz = fread(pemBuf, 1, LARGE_TEMP_SZ, file);
    fclose(file);
    if (caKeySz <= 0) {
        printf("Failed to read caKey from file\n");
        goto exit;
    }
    printf("Successfully read %d bytes from %s\n", pemSz, caKeyPemFile);

    ret = wc_KeyPemToDer(pemBuf, pemSz, caKeyBuf, LARGE_TEMP_SZ, NULL);
    if (ret >= 0) {
        caKeySz = ret;
        ret = 0;
    }
    printf("Converted Key PEM to DER %d bytes\n\n", caKeySz);

    /*---------------------------------------------------------------------------*/
    /* Load CSR PEM */
    /*---------------------------------------------------------------------------*/
    printf("Loading CSR certificate\n");

    file = fopen(csrPemFile, "rb");
    if (!file) {
        printf("failed to open file: %s\n", csrPemFile);
        goto exit;
    }
    pemSz = fread(pemBuf, 1, LARGE_TEMP_SZ, file);
    fclose(file);
    printf("Successfully read %d bytes from %s\n\n", pemSz, csrPemFile);

    ret = wc_CertPemToDer(pemBuf, pemSz, derBuf, LARGE_TEMP_SZ, CERTREQ_TYPE);
    if (ret >= 0) {
        derSz = ret;
        ret = 0;
    }
    printf("Converted CSR Cert PEM to DER %d bytes\n\n", derSz);


    /*---------------------------------------------------------------------------*/
    /* Create a new certificate using SUBJECT information from ca cert
     * for ISSUER information in generated cert */
    /*---------------------------------------------------------------------------*/
    printf("Setting new cert issuer to subject of signer\n");

    wc_InitCert(&newCert);

    newCert.isCA    = 0;
    newCert.sigType = CTC_SHA256wECDSA;

    ret = wc_SetSubjectBuffer(&newCert, derBuf, derSz);
    if (ret != 0) goto exit;
    
    ret = wc_SetIssuerBuffer(&newCert, caCertBuf, caCertSz);
    if (ret != 0) goto exit;

    ret = wc_InitRng(&rng);
    if (ret != 0) goto exit;
    initRng = 1;

    ret = wc_MakeCert(&newCert, derBuf, LARGE_TEMP_SZ, NULL, &newKey, &rng);
    if (ret < 0) goto exit;
    printf("Make Cert returned %d\n", ret);

    ret = wc_SignCert_ex(newCert.bodySz, newCert.sigType, derBuf, LARGE_TEMP_SZ, type, &caKey, &rng);
    if (ret < 0) goto exit;
    derSz = ret;

    printf("Signed Cert returned %d\n", derSz);

    printf("Successfully signed certificate\n\n");

    /*---------------------------------------------------------------------------*/
    /* write the new cert to file in DER format */
    /*---------------------------------------------------------------------------*/
    printf("Writing newly generated DER certificate to file \"%s\"\n",
        newCertOutput);
    file = fopen(newCertOutput, "wb");
    if (!file) {
        printf("failed to open file: %s\n", newCertOutput);
        goto exit;
    }

    ret = (int)fwrite(derBuf, 1, derSz, file);
    fclose(file);
    printf("Successfully output %d bytes\n", ret);

#ifdef WOLFSSL_DER_TO_PEM
    /*---------------------------------------------------------------------------*/
    /* convert the DER to a PEM and write it to a file */
    /*---------------------------------------------------------------------------*/
    printf("Convert the DER cert to PEM formatted cert\n");

    pemSz = wc_DerToPem(derBuf, derSz, pemBuf, LARGE_TEMP_SZ, CERT_TYPE);
    if (pemSz < 0) goto exit;

    printf("Resulting PEM buffer is %d bytes\n", pemSz);

    file = fopen(newCertPemFile, "wb");
    if (!file) {
        printf("failed to open file: %s\n", newCertPemFile);
        goto exit;
    }
    fwrite(pemBuf, 1, pemSz, file);
    fclose(file);
    printf("Successfully converted the DER to PEM to \"%s\"\n\n",
        newCertPemFile);
#endif

    ret = 0; /* success */

exit:

    XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(pemBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(caKeyBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(caCertBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    if (initCaKey)
        wc_ecc_free(&caKey);
    if (initNewKey)
        wc_ecc_free(&newKey);
    if (initRng) {
        wc_FreeRng(&rng);
    }

    if (ret == 0)
        printf("Tests passed\n");
    else
        printf("Failure code was %d\n", ret);

    (void)keyPtr;

    return ret;
}

int main(int argc, char** argv)
{
#if !defined(WOLFSSL_CERT_REQ) || !defined(WOLFSSL_CERT_GEN) || !defined(HAVE_DECODEDCERT)
    printf("Please compile wolfSSL with --enable-certreq --enable-certgen CFLAGS=-DOPENSSL_EXTRA_X509_SMALL\n");
    return 0;
#else
    if (argc != 5) {
        usage();
        return 1;
    }
    return do_csrsign(argc, argv);
#endif
}
