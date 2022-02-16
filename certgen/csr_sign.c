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

/* Check if the internal asn API's are available */
#if defined(WOLFSSL_TEST_CERT) || defined(OPENSSL_EXTRA) || \
    defined(OPENSSL_EXTRA_X509_SMALL)
    #define HAVE_DECODEDCERT
#endif

#if defined(WOLFSSL_CERT_REQ) && defined(WOLFSSL_CERT_GEN) && \
    defined(HAVE_ECC)

#define HEAP_HINT NULL
#define LARGE_TEMP_SZ 4096

static void usage(void)
{
    printf("Usage: ./csr_sign csr.pem ca-cert.pem ca-key.pem\n");
    printf("Example:\n");
    printf("./csr_sign ecc-csr.pem ca-ecc-cert.pem ca-ecc-key.pem\n");
}

static int do_csrsign(int argc, char** argv)
{
    int ret = 0;
    int type = ECC_TYPE;

    Cert newCert;

    FILE* file;
    const char* csrPemFile = argv[1];
    const char* caCertPemFile = argv[2];
    const char* caKeyPemFile = argv[3];
        
    const char* newCertOutput = "./newCert.der";
    const char* newCertPemFile = "./newCert.pem";

    int derSz = 0;
    int pemSz = 0;
    int caCertSz = 0;

    byte* derBuf   = NULL;
    byte* pemBuf   = NULL;
    byte* caCertBuf = NULL;

#ifdef HAVE_ECC
    ecc_key csrKey;
    ecc_key caKey;
#endif
    void* keyPtr = NULL;
    WC_RNG rng;
    word32 idx;
    int initRng = 0, initCaKey = 0, initCsrKey = 0;
#ifdef HAVE_DECODEDCERT
    DecodedCert decoded;
    int initDecode = 0;
#endif

    derBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (derBuf == NULL) goto exit;
    XMEMSET(derBuf, 0, LARGE_TEMP_SZ);

    pemBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (pemBuf == NULL) goto exit;
    XMEMSET(pemBuf, 0, LARGE_TEMP_SZ);

    caCertBuf = (byte*)XMALLOC(LARGE_TEMP_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (caCertBuf == NULL) goto exit;
    XMEMSET(caCertBuf, 0, LARGE_TEMP_SZ);


    /*------------------------------------------------------------------------*/
    /* Loading the CA Certificate PEM File */
    /*------------------------------------------------------------------------*/
    printf("Loading CA certificate\n");

    file = fopen(caCertPemFile, "rb");
    if (!file) {
        printf("failed to open file: %s\n", caCertPemFile);
        goto exit;
    }
    pemSz = fread(pemBuf, 1, LARGE_TEMP_SZ, file);
    fclose(file);
    printf("Read %d bytes from %s\n\n", pemSz, caCertPemFile);

    ret = wc_CertPemToDer(pemBuf, pemSz, caCertBuf, LARGE_TEMP_SZ, CERT_TYPE);
    if (ret == ASN_NO_PEM_HEADER) {
        memcpy(caCertBuf, pemBuf, pemSz);
        memset(pemBuf, 0, LARGE_TEMP_SZ);
        caCertSz = pemSz;
        printf("CA Cert file detected as DER\n\n");
    }
    else if (ret >= 0) {
        caCertSz = ret;
        ret = 0;
        printf("Converted CA Cert PEM to DER %d bytes\n\n", caCertSz);
    }
    else {
        goto exit;
    }
    
    /*------------------------------------------------------------------------*/
    /* Load the CA Key PEM File */
    /*------------------------------------------------------------------------*/
    printf("Loading the CA key\n");

    file = fopen(caKeyPemFile, "rb");
    if (!file) {
        printf("failed to open file: %s\n", caKeyPemFile);
        goto exit;
    }
    pemSz = fread(pemBuf, 1, LARGE_TEMP_SZ, file);
    fclose(file);
    printf("Read %d bytes from %s\n", pemSz, caKeyPemFile);

    ret = wc_KeyPemToDer(pemBuf, pemSz, derBuf, LARGE_TEMP_SZ, NULL);
    if (ret == ASN_NO_PEM_HEADER) {
        memcpy(derBuf, pemBuf, pemSz);
        memset(pemBuf, 0, LARGE_TEMP_SZ);
        derSz = pemSz;
        printf("CA Key file detected as DER\n\n");
    }
    else if (ret >= 0) {
        derSz = ret;
        ret = 0;
        printf("Converted CA Key PEM to DER %d bytes\n\n", derSz);
    }
    else {
        goto exit;
    }

    printf("Loading CA key to ecc_key struct\n");
    ret = wc_ecc_init(&caKey);
    if (ret != 0) goto exit;
    initCaKey = 1;

    idx = 0;
    ret = wc_EccPrivateKeyDecode(derBuf, &idx, &caKey, derSz);
    if (ret != 0) goto exit;

    /*------------------------------------------------------------------------*/
    /* Load CSR PEM */
    /*------------------------------------------------------------------------*/
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
    printf("Converted CSR Cert PEM to DER %d bytes\n", derSz);

#ifdef HAVE_DECODEDCERT
    /* Code for parsing a CSR to a DecodedCert struct */
    /* Note: These are not public API's unless WOLFSSL_TEST_CERT or the compat 
     * layer is enabled */
    InitDecodedCert(&decoded, derBuf, derSz, NULL);
    ret = ParseCert(&decoded, CERTREQ_TYPE, NO_VERIFY, NULL);
    if (ret != 0) goto exit;
    printf("Loaded CSR to DecodedCert struct\n\n");
    initDecode = 1;

    /* Decode public key into csrKey */
    printf("Decoding Public Key\n");
    ret = wc_ecc_init(&csrKey);
    if (ret != 0) goto exit;
    initCsrKey = 1;

    idx = 0;
    ret = wc_EccPublicKeyDecode(decoded.publicKey, &idx, &csrKey,
        decoded.pubKeySize);
    if (ret != 0) goto exit;
#endif

    /*------------------------------------------------------------------------*/
    /* Create a new certificate using SUBJECT information from CSR and ISSUER 
     * from CA cert */
    /*------------------------------------------------------------------------*/
    wc_InitCert(&newCert);

    newCert.isCA    = 0;
    newCert.sigType = CTC_SHA256wECDSA;

    printf("Setting certificate subject\n");
#ifdef HAVE_DECODEDCERT
    if (decoded.subjectC)
        strncpy(newCert.subject.country, decoded.subjectC, decoded.subjectCLen);
    if (decoded.subjectST)
        strncpy(newCert.subject.state, decoded.subjectST, decoded.subjectSTLen);
    if (decoded.subjectL)
        strncpy(newCert.subject.locality, decoded.subjectL, decoded.subjectLLen);
    if (decoded.subjectO)
        strncpy(newCert.subject.org, decoded.subjectO, decoded.subjectOLen);
    if (decoded.subjectOU)
        strncpy(newCert.subject.unit, decoded.subjectOU, decoded.subjectOULen);
    if (decoded.subjectSN)
        strncpy(newCert.subject.sur, decoded.subjectSN, decoded.subjectSNLen);
    if (decoded.subjectSND)
        strncpy(newCert.subject.serialDev, decoded.subjectSND, decoded.subjectSNDLen);
    if (decoded.subjectCN)
        strncpy(newCert.subject.commonName, decoded.subjectCN, decoded.subjectCNLen);
    if (decoded.subjectEmail)
        strncpy(newCert.subject.email, decoded.subjectEmail, decoded.subjectEmailLen);
#else
    /* This can be used if the DER is an X.509 certificate (not CSR) */
    //ret = wc_SetSubjectBuffer(&newCert, derBuf, derSz);
    //if (ret != 0) goto exit;

    strncpy(newCert.subject.country, "US", CTC_NAME_SIZE);
    strncpy(newCert.subject.state, "MT", CTC_NAME_SIZE);
    strncpy(newCert.subject.locality, "Bozeman", CTC_NAME_SIZE);
    strncpy(newCert.subject.org, "yourOrgNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.unit, "yourUnitNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.commonName, "www.yourDomain.com", CTC_NAME_SIZE);
    strncpy(newCert.subject.email, "yourEmail@yourDomain.com", CTC_NAME_SIZE);
#endif

    /* Set issuer using CA certificate */
    printf("Setting certificate issuer\n");
    ret = wc_SetIssuerBuffer(&newCert, caCertBuf, caCertSz);
    if (ret != 0) goto exit;

    /* Serial Number will be randomly generated if not provided */
    newCert.serial[0] = 0; /* up to 20 bytes */
    newCert.serialSz = 0;

    /* Days before certificate expires */
    newCert.daysValid = 365;

    /* Key Usage */
    ret = wc_SetKeyUsage(&newCert, "digitalSignature"); /* comma sep list */
    if (ret != 0) goto exit;
    ret = wc_SetExtKeyUsage(&newCert, "serverAuth,clientAuth");
    if (ret != 0) goto exit;

    ret = wc_InitRng(&rng);
    if (ret != 0) goto exit;
    initRng = 1;

    /* Create X.509 Certificate */    
    printf("Creating certificate...\n");
    ret = wc_MakeCert_ex(&newCert, derBuf, LARGE_TEMP_SZ, type, &csrKey, &rng);
    if (ret < 0) goto exit;
    printf("Successfully created certificate %d\n\n", newCert.bodySz);

    /* Sign with CA Key */
    printf("Signing certificate...\n");
    ret = wc_SignCert_ex(newCert.bodySz, newCert.sigType, derBuf, LARGE_TEMP_SZ,
        type, &caKey, &rng);
    if (ret < 0) goto exit;
    derSz = ret;
    printf("Successfully signed certificate %d\n\n", derSz);

    /*------------------------------------------------------------------------*/
    /* write the new cert to file in DER format */
    /*------------------------------------------------------------------------*/
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
    /*------------------------------------------------------------------------*/
    /* convert the DER to a PEM and write it to a file */
    /*------------------------------------------------------------------------*/
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
    XFREE(caCertBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

#ifdef HAVE_DECODEDCERT
    if (initDecode) {
        FreeDecodedCert(&decoded);
    }
#endif
    if (initCaKey)
        wc_ecc_free(&caKey);
    if (initCsrKey)
        wc_ecc_free(&csrKey);
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
#endif

int main(int argc, char** argv)
{
#if !defined(WOLFSSL_CERT_REQ) || !defined(WOLFSSL_CERT_GEN) || \
    !defined(HAVE_ECC)
    printf("Please compile wolfSSL with --enable-certreq --enable-certgen "
           "--enable-ecc CFLAGS=-DOPENSSL_EXTRA_X509_SMALL\n");
    return 0;
#else
    if (argc != 4) {
        usage();
        return 1;
    }
    return do_csrsign(argc, argv);
#endif
}
