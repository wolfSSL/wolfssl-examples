/* pkcs11_csr.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_pkcs11.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

static WC_RNG rng;

#define LARGE_TEMP_SZ 4096
#define HEAP_HINT NULL

#if defined(HAVE_ECC) && defined(WOLFSSL_CERT_GEN) && \
    defined(WOLFSSL_CERT_REQ) && defined(WOLFSSL_CERT_REQ)
#define ENABLE_PKCS11_CSR

/* Check if the internal asn API's are available */
#if defined(WOLFSSL_TEST_CERT) || defined(OPENSSL_EXTRA) || \
    defined(OPENSSL_EXTRA_X509_SMALL)
    #define HAVE_DECODEDCERT
#endif
#endif


#ifdef ENABLE_PKCS11_CSR

static const char* keyPemFile = "ecc-key.pem";
static const char* csrPemFile = "ecc-csr.pem";
static const char* newCertOutput = "ecc-cert.der";
static const char* newCertPemFile = "ecc-cert.pem";
/* TODO: Use real certificate associated with PKCS11 key.
 * This is used to populate the issuer field */
static const char* caCertPemFile = "../certgen/ca-ecc-cert.pem";

static int gen_csr(void)
{
    int ret;
    int type = ECC_TYPE;
    ecc_key ecKey;
    void* keyPtr = NULL;
    Cert req;
    byte der[LARGE_TEMP_SZ];
    int  derSz;
#ifdef WOLFSSL_DER_TO_PEM
    byte pem[LARGE_TEMP_SZ];
    int  pemSz;
    FILE* file = NULL;
#endif

    XMEMSET(der, 0, LARGE_TEMP_SZ);
#ifdef WOLFSSL_DER_TO_PEM
    XMEMSET(pem, 0, LARGE_TEMP_SZ);
#endif

#ifdef HAVE_ECC
    keyPtr = &ecKey;
    ret = wc_ecc_init_ex(&ecKey, HEAP_HINT, INVALID_DEVID);
#endif
    if (ret != 0) {
        printf("Key initialization failed: %d\n", ret);
        goto exit;
    }

    ret = wc_ecc_make_key_ex(&rng, 32, &ecKey, ECC_SECP256R1);
    if (ret != 0) {
        printf("Key generation failed: %d\n", ret);
        goto exit;
    }

    ret = wc_EccKeyToDer(&ecKey, der, sizeof(der));
    if (ret <= 0) {
        printf("Key To DER failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

#ifdef WOLFSSL_DER_TO_PEM
    memset(pem, 0, sizeof(pem));
    ret = wc_DerToPem(der, derSz, pem, sizeof(pem), ECC_PRIVATEKEY_TYPE);
    if (ret <= 0) {
        printf("Key DER to PEM failed: %d\n", ret);
        goto exit;
    }
    pemSz = ret;
    printf("%s (%d)\n", pem, pemSz);

    printf("Saved Key PEM to \"%s\"\n", keyPemFile);
    file = fopen(keyPemFile, "wb");
    if (file) {
        ret = (int)fwrite(pem, 1, pemSz, file);
        fclose(file);
    }
#endif /* WOLFSSL_DER_TO_PEM */

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
    ret = wc_MakeCertReq_ex(&req, der, sizeof(der), type, keyPtr);
    if (ret <= 0) {
        printf("Make Cert Req failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

    req.sigType = CTC_SHA256wECDSA;
    ret = wc_SignCert_ex(req.bodySz, req.sigType, der, sizeof(der), type,
        keyPtr, &rng);
    if (ret <= 0) {
        printf("Sign Cert failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

#ifdef WOLFSSL_DER_TO_PEM
    memset(pem, 0, sizeof(pem));
    ret = wc_DerToPem(der, derSz, pem, sizeof(pem), CERTREQ_TYPE);
    if (ret <= 0) {
        printf("CSR DER to PEM failed: %d\n", ret);
        goto exit;
    }
    pemSz = ret;
    printf("%s (%d)\n", pem, pemSz);

    printf("Saved CSR PEM to \"%s\"\n", csrPemFile);
    file = fopen(csrPemFile, "wb");
    if (file) {
        ret = (int)fwrite(pem, 1, pemSz, file);
        fclose(file);
    }
#endif

    ret = 0; /* success */
    
exit:
    wc_ecc_free(&ecKey);

    return ret;
}

/* caKey: Used for signing the CSR */
static int do_csrsign(ecc_key* caKey)
{
    int ret = 0;
    int type = ECC_TYPE;

    Cert newCert;

    FILE* file = NULL;

    int derSz = 0;
    int pemSz = 0;
    int caCertSz = 0;

    byte* derBuf   = NULL;
    byte* pemBuf   = NULL;
    byte* caCertBuf = NULL;

    ecc_key csrKey;
    void* keyPtr = NULL;
    WC_RNG rng;
    int initRng = 0, initCsrKey = 0;
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
    /* Loading the CA Certificate PEM File, used for the issuer in the new certificate */
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
    InitDecodedCert(&decoded, derBuf, derSz, HEAP_HINT);
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
    /* Copy fields from CSR to new certificate */
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
        type, caKey, &rng);
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

int gen_ec_keys(Pkcs11Token* token, ecc_key* key, unsigned char* id, int idLen,
                int devId)
{
    int ret;

    ret = wc_ecc_init_id(key, id, idLen, HEAP_HINT, devId);
    if (ret != 0)
        fprintf(stderr, "Failed to initialize EC key: %d\n", ret);
    if (ret == 0) {
        ret = wc_ecc_make_key_ex2(&rng, 32, key, ECC_CURVE_DEF,
                                  WC_ECC_FLAG_DEC_SIGN);
        if (ret != 0)
            fprintf(stderr, "Failed to generate EC key: %d\n", ret);
    }
    return ret;
}

int do_csrgen_sign(int devId, Pkcs11Token* token)
{
    int     ret = 0;
    ecc_key signingKey;

    ret = wc_Pkcs11Token_Open(token, 1);
    if (ret == 0) {
        fprintf(stderr, "Generate EC Keys\n");

        /* Generate ECC key to sign with */
        /* TODO: Use existing key in HSM */
        ret = gen_ec_keys(token, &signingKey, (unsigned char*)"123ecc", 6, devId);
        if (ret == 0) {
            ret = gen_csr();
        }
        if (ret == 0) {
            ret = do_csrsign(&signingKey);
        }

        wc_Pkcs11Token_Close(token);
        wc_ecc_free(&signingKey);
    }
    return ret;
}

#endif /* ENABLE_PKCS11_CSR */

int main(int argc, char* argv[])
{
    int ret;
    const char* library;
    const char* slot;
    const char* tokenName;
    const char* userPin;
    Pkcs11Dev dev;
    Pkcs11Token token;
    int slotId;
    int devId = 1;

    if (argc != 4 && argc != 5) {
        fprintf(stderr,
                "Usage: pkcs11_csr <libname> <slot> <tokenname> [userpin]\n");
        return 1;
    }

    library = argv[1];
    slot = argv[2];
    tokenName = argv[3];
    userPin = (argc == 4) ? NULL : argv[4];
    slotId = atoi(slot);

#if defined(DEBUG_WOLFSSL)
    wolfSSL_Debugging_ON();
#endif
    wolfCrypt_Init();

    ret = wc_Pkcs11_Initialize(&dev, library, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize PKCS#11 library\n");
        ret = 2;
    }
    if (ret == 0) {
        ret = wc_Pkcs11Token_Init(&token, &dev, slotId, tokenName,
            (byte*)userPin, userPin == NULL ? 0 : strlen(userPin));
        if (ret != 0) {
            fprintf(stderr, "Failed to initialize PKCS#11 token\n");
            ret = 2;
        }
        if (ret == 0) {
            ret = wc_CryptoDev_RegisterDevice(devId, wc_Pkcs11_CryptoDevCb,
                                              &token);
            if (ret != 0) {
                fprintf(stderr, "Failed to register PKCS#11 token\n");
                ret = 2;
            }
            if (ret == 0) {
                wc_InitRng_ex(&rng, NULL, devId);

            #ifdef ENABLE_PKCS11_CSR
                ret = do_csrgen_sign(devId, &token);
                if (ret != 0)
                    ret = 1;
            #else
                printf("This example requires ECC and certificate req/gen support: "
                       "--enable-ecc --enable-certgen --enable-certreq --enable-certext\n");
            #endif

                wc_FreeRng(&rng);
            }
            wc_Pkcs11Token_Final(&token);
        }
        wc_Pkcs11_Finalize(&dev);
    }

    wolfCrypt_Cleanup();

    return ret;
}
