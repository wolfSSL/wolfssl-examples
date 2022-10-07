/* pkcs12-create-example.c
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

#include <stdio.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/pkcs12.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/wc_port.h>

#define WC_RSA_KEYSIZE 2048
#define HEAP_HINT NULL
#ifndef CA_CERT
    #define CA_CERT "../../certs/ca-cert.der"
#endif
#ifndef CA_KEY
    #define CA_KEY "../../certs/ca-key.der"
#endif
static int devId = INVALID_DEVID; /* set to alternate dev id if wanted */

#if defined(HAVE_PKCS12) && defined(WOLFSSL_KEY_GEN) && \
    defined(WOLFSSL_CERT_GEN) && !defined(NO_RSA)
static int createKey(byte** keyDer, word32* keySz, RsaKey* key, WC_RNG* rng)
{
    int ret;

    *keyDer = NULL;
    *keySz  = 0;

    ret = wc_InitRsaKey_ex(key, HEAP_HINT, devId);
    if (ret == 0) {
        ret = wc_InitRng_ex(rng, HEAP_HINT, devId);
    }

    if (ret == 0) {
        ret = wc_MakeRsaKey(key, WC_RSA_KEYSIZE, WC_RSA_EXPONENT, rng);
    }

    if (ret == 0) {
        *keySz = wc_RsaKeyToDer(key, NULL, 0);
        if (*keySz < 0) {
            printf("unable to convert RSA key to DER\n");
            ret = *keySz;
        }
    }

    if (ret == 0) {
        *keyDer = (byte*)XMALLOC(*keySz, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (*keyDer == NULL) {
            ret = MEMORY_E;
        }
    }

    if (ret == 0) {
        *keySz = wc_RsaKeyToDer(key, *keyDer, *keySz);
        if (*keySz < 0) {
            printf("unable to convert RSA key to DER\n");
            ret = *keySz;
        }
    }

    return ret;
}


static void outputFile(byte* out, word32 outSz, char* fileName)
{
    FILE* file;

    file = fopen(fileName, "wb");
    if (!file) {
        printf("failed to open file: %s\n", fileName);
    }
    else {
        printf("printing out %d bytes\n", outSz);
        fwrite(out, 1, outSz, file);
        fclose(file);
    }
}


static int readFile(byte** out, word32* outSz, char* fileName)
{
    FILE* file;
    int   ret = 0;

    file = fopen(fileName, "rb");
    if (!file) {
        printf("failed to open file: %s\n", fileName);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    *outSz = ftell(file);
    rewind(file);

    *out = (byte*) XMALLOC(*outSz, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (*out == NULL) {
        ret = MEMORY_E;
    }
    else {
        *outSz = fread(*out, 1, *outSz, file);
        if (*outSz <= 0) {
            printf("Failed to read from file\n");
            ret = *outSz;
        }
        else {
            printf("Successfully read %d bytes\n", *outSz);
        }
    }
    fclose(file);

    return ret;
}


static int loadKey(byte** keyDer, word32* keySz, RsaKey* key, const char* f)
{
    int ret;

    *keyDer = NULL;
    *keySz  = 0;

    ret = wc_InitRsaKey_ex(key, HEAP_HINT, devId);
    if (ret == 0) {
        ret = readFile(keyDer, keySz, (char*)f);
    }

    if (ret == 0) {
        word32 idx = 0;
        ret = wc_RsaPrivateKeyDecode(*keyDer, &idx, key, *keySz);
        printf("return from loading in private key = %d\n", ret);
    }

    if (*keySz < 0) {
        printf("unable to decode private key\n");
        ret = *keySz;
    }

    return ret;
}


static int getCa(byte** caCert, word32* caCertSz, RsaKey* caKey)
{
    int   ret;
    char caCertFile[] = CA_CERT;
    char caKeyFile[]  = CA_KEY;

    byte* caKeyDer = NULL;
    word32 caKeyDerSz;
    word32 idx = 0;

    *caCert = NULL;
    *caCertSz = 0;

    printf("Getting the caKey from %s\n", caKeyFile);
    ret = readFile(&caKeyDer, &caKeyDerSz, caKeyFile);
    if (ret == 0) {
        ret = wc_InitRsaKey_ex(caKey, HEAP_HINT, devId);
    }

    if (ret == 0) {
        ret = wc_RsaPrivateKeyDecode(caKeyDer, &idx, caKey, caKeyDerSz);
        if (ret != 0) {
            printf("unable to decode RSA key\n");
        }
    }


    /* now get certificate */
    if (ret == 0) {
        printf("Getting the caCert from %s\n", caCertFile);
        ret = readFile(caCert, caCertSz, caCertFile);
    }

    XFREE(caKeyDer, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    return ret;
}


static int createCert(byte** certDer, word32* certSz, RsaKey* key, WC_RNG* rng)
{
    int ret;
    Cert newCert;
    RsaKey caKey;
    byte*  caCert = NULL;
    word32 caCertSz;

    *certDer = NULL;
    *certSz  = 4096;

    wc_InitCert(&newCert);

    strncpy(newCert.subject.country, "US", CTC_NAME_SIZE);
    strncpy(newCert.subject.state, "MT", CTC_NAME_SIZE);
    strncpy(newCert.subject.locality, "Bozeman", CTC_NAME_SIZE);
    strncpy(newCert.subject.org, "yourOrgNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.unit, "yourUnitNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.commonName, "www.yourDomain.com", CTC_NAME_SIZE);
    strncpy(newCert.subject.email, "yourEmail@yourDomain.com", CTC_NAME_SIZE);
    newCert.isCA    = 0;
    newCert.sigType = CTC_SHA256wRSA;

    ret = getCa(&caCert, &caCertSz, &caKey);
    if (ret == 0) {
        ret = wc_SetIssuerBuffer(&newCert, caCert, caCertSz);
    }

    if (ret == 0) {
        *certDer = (byte*)XMALLOC(*certSz, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (*certDer == NULL) {
            ret = MEMORY_E;
        }
        else {
            ret = wc_MakeCert(&newCert, *certDer, *certSz, key, NULL, rng);
        }
    }

    printf("MakeCert returned %d\n", ret);
    if (ret >= 0) {
        ret = wc_SignCert(newCert.bodySz, newCert.sigType, *certDer, *certSz,
                &caKey, NULL, rng);
        if (ret < 0) {
            printf("SignCert returned %d\n", ret);
        }
        else {
            *certSz = ret;
            ret = 0; /* return 0 on success */
            printf("Successfully created new certificate\n");
        }
    }
    XFREE(caCert, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    return ret;
}
#endif

int main(int argc, char* argv[])
{
#if defined(HAVE_PKCS12) && defined(WOLFSSL_KEY_GEN) && \
    defined(WOLFSSL_CERT_GEN) && !defined(NO_RSA)
    WC_PKCS12* pkcs12 = NULL;
    WC_RNG rng;
    RsaKey rsa;

    byte* keyDer = NULL;
    word32 keySz;
    byte* certDer = NULL;
    word32 certSz;

    if (wolfCrypt_Init() != 0) {
        printf("issue with wolfCrypt_Init()\n");
        return -1;
    }

    if (argc == 2) {
        if (loadKey(&keyDer, &keySz, &rsa, argv[1]) != 0) {
            printf("Unable to create RSA key\n");
            return -1;
        }
        else {
            printf("loaded in key %s\n", argv[1]);
            wc_InitRng_ex(&rng, HEAP_HINT, devId);
        }
    }
    else {
        if (createKey(&keyDer, &keySz, &rsa, &rng) != 0) {
            printf("Unable to create RSA key\n");
            return -1;
        }
    }

    if (createCert(&certDer, &certSz, &rsa, &rng) != 0) {
        printf("Unable to create certificate\n");
        wc_FreeRsaKey(&rsa);
        wc_FreeRng(&rng);
        XFREE(keyDer, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return -1;
    }

    pkcs12 = wc_PKCS12_create("wolfSSL test", strlen("wolfSSL test"),
        NULL, keyDer, keySz, certDer, certSz,
        NULL, PBE_SHA1_DES3, PBE_SHA1_DES3, 100, 100, 0, HEAP_HINT);

    if (pkcs12 != NULL) {
        int ret;
        int pkcs12DerSz = 0;
        byte* pkcs12Der = NULL;

        printf("Created new PKCS12 structure now converting to DER\n");
        if ((ret = wc_i2d_PKCS12(pkcs12, &pkcs12Der, &pkcs12DerSz)) < 0) {
            printf("unable to convert structure to DER\n");
        }
        else {
            char output[] = "output.p12";
            printf("Printing PKCS12 DER file to %s\n", output);
            outputFile(pkcs12Der, ret, output);
            XFREE(pkcs12Der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        }
        wc_PKCS12_free(pkcs12);
    }
    else {
        printf("Issue creating new PKCS12 structure\n");
    }

    wc_FreeRsaKey(&rsa);
    wc_FreeRng(&rng);
    XFREE(keyDer, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(certDer, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    wolfCrypt_Cleanup();
#else
    printf("pkcs12-create-key requires wolfssl to be built with:\n");
    printf("\t./configure --enable-pkcs12 --enable-pwdbased --enable-des3 --enable-keygen --enable-certgen\n");
#endif
    return 0;
}
