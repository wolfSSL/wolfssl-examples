/* certgen_sign.c
 *
 * Copyright (C) 2006-2019 wolfSSL Inc.
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

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#ifdef HAVE_PKCS12
#include <wolfssl/wolfcrypt/pkcs12.h>
#endif

#define USE_CERT_BUFFERS_2048
#include <wolfssl/certs_test.h>

#ifdef HAVE_PKCS12
static void printPKCS12(WOLFSSL_X509 *x509, WOLFSSL_RSA *rsa)
{
    WOLFSSL_BIO *bio;
    WC_PKCS12 *pkcs12;
    byte *der = NULL;
    byte *key = NULL;
    int  derSz;
    int  keySz;
    char *outFile = "test-pkcs12.p12";

    /* write RSA key */
    bio = wolfSSL_BIO_new_fp(stdout, BIO_NOCLOSE);
    if (bio != NULL)
        wolfSSL_PEM_write_bio_RSAPrivateKey(bio, rsa, NULL, NULL, 0, NULL, NULL);
    wolfSSL_BIO_free(bio);


    /* get private key from WOLFSSL_RSA */
    keySz = wolfSSL_i2d_RSAPrivateKey(rsa, &key);
    if (keySz <= 0) {
        printf("unable to get private RSA key\n");
        return;
    }

    derSz = wolfSSL_i2d_X509(x509, &der);
    if (derSz <= 0) {
        printf("unable to get X509 DER\n");
        free(key);
        return;
    }

    /* create a PKCS12 bundle from the cert + key */
    pkcs12 = wc_PKCS12_create("test password", strlen("test password"),
            NULL, key, keySz, der, derSz, NULL, PBE_SHA1_DES3, PBE_SHA1_DES3,
            2048, 2048, 0, NULL);
    if (pkcs12 == NULL) {
        printf("issue with creating pkcs12 bundle\n");
    }
    else {
        int ret;
        int pkcs12DerSz = 0;
        byte* pkcs12Der = NULL;

        printf("\nCreated new PKCS12 structure now converting to DER\n");
        if ((ret = wc_i2d_PKCS12(pkcs12, &pkcs12Der, &pkcs12DerSz)) < 0) {
            printf("unable to convert structure to DER, error = %d\n", ret);
        }
        else {
            FILE* f;

            printf("putting PKCS12 DER in to the file %s\n", outFile);
            printf("password : \"test password\"\n");
            f = fopen(outFile, "wb");
            fwrite(pkcs12Der, 1, ret, f);
            fclose(f);
            free(pkcs12Der);
        }
        wc_PKCS12_free(pkcs12);
    }
    free(key);
    free(der);
}
#endif

int main(int argc, char* argv[])
{
    WOLFSSL_X509 *x509 = NULL, *ca = NULL;
    const WOLFSSL_EVP_MD* md;
    WOLFSSL_EVP_PKEY *pkey = NULL, *cakey = NULL;
    WOLFSSL_RSA *rsa;
    WOLFSSL_BIO* bio;
    int ret = 0;
    FILE* f;
    unsigned char buffer[4096];
    long bufferSz;
    const unsigned char* der;

    WOLFSSL_X509_NAME* subject;
    WOLFSSL_ASN1_TIME *notAfter, *notBefore;

    if (argc != 3) {
        printf("Error on input arguments, expected:\n");
        printf("./certgen_sign <DER RSA CA file name> <DER RSA CA private key name>\n");
        return -1;
    }
    wolfSSL_Debugging_ON();

    /* parse in CA private key */
    f = fopen (argv[2], "rb");
    if (f == NULL) {
        printf("unable to open private key %s\n", argv[2]);
        ret = -1;
        goto end;
    }
    bufferSz = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);
    der = buffer;

    cakey = wolfSSL_d2i_PrivateKey(EVP_PKEY_RSA, NULL, &der, bufferSz);
    if (cakey == NULL) {
        printf("unable to parse key\n");
        ret = -1;
        goto end;
    }

    /* parse in CA cert */
    f = fopen (argv[1], "rb");
    if (f == NULL) {
        printf("unable to open cert%s\n", argv[1]);
        ret = -1;
        goto end;
    }
    bufferSz = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);
    der = buffer;

    ca = wolfSSL_d2i_X509(NULL, &der, bufferSz);
    if (ca == NULL) {
        printf("unable to parse cert\n");
        ret = -1;
        goto end;
    }

    /* use example public key */
    rsa = wolfSSL_RSA_generate_key(1024, 65537, NULL, NULL);
    if (rsa == NULL) {
        printf("error generating RSA key\n");
        ret = -1;
        goto end;
    }

    pkey = wolfSSL_EVP_PKEY_new();
    ret = wolfSSL_EVP_PKEY_assign_RSA(pkey, rsa);
    if (pkey == NULL) {
        printf("unable to get public rsa key\n");
        ret = -1;
        goto end;
    }

    /* create new certificate signed by CA */
    x509 = wolfSSL_X509_new();
    if (x509 == NULL) {
        printf("unable to create x509 struct\n");
        ret = -1;
        goto end;
    }

    wolfSSL_X509_set_version(x509, 3);

    /* create new subject name for certificate */
    subject = wolfSSL_X509_NAME_new();
    if (subject == NULL) {
        printf("error creating new subject subject\n");
        ret = -1;
        goto end;
    }

    /* for ease of readability am not checking the return value here, return
     * values should be 1 on success */
    wolfSSL_X509_NAME_add_entry_by_txt(subject, "C", MBSTRING_ASC,
                (unsigned char*)"MT", -1, -1, 0);
    wolfSSL_X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC,
            (unsigned char*)"test sign", -1, -1, 0);
    wolfSSL_X509_NAME_add_entry_by_txt(subject, "OU", MBSTRING_ASC,
            (unsigned char*)"test sign OU1", -1, -1, 0);
    wolfSSL_X509_NAME_add_entry_by_txt(subject, "OU", MBSTRING_ASC,
            (unsigned char*)"test sign OU2", -1, -1, 0);

    if (wolfSSL_X509_set_subject_name(x509, subject) != WOLFSSL_SUCCESS) {
        printf("error setting new subject subject\n");
        ret = -1;
        goto end;
    }

    /* set serial number */
    {
        WOLFSSL_ASN1_INTEGER *serial;
        serial = wolfSSL_ASN1_INTEGER_new();
        if (serial == NULL) {
            printf("error creating serial number struct\n");
            ret = -1;
            goto end;
        }

        if (wolfSSL_ASN1_INTEGER_set(serial, 1) != WOLFSSL_SUCCESS) {
            wolfSSL_ASN1_INTEGER_free(serial);
            printf("error setting serial number value\n");
            ret = -1;
            goto end;
        }

        if (wolfSSL_X509_set_serialNumber(x509, serial) != WOLFSSL_SUCCESS) {
            wolfSSL_ASN1_INTEGER_free(serial);
            printf("error setting serial number value in x509\n");
            ret = -1;
            goto end;
        }
        wolfSSL_ASN1_INTEGER_free(serial);
    }

    /* set issuer from CA cert */
    wolfSSL_X509_set_issuer_name(x509, wolfSSL_X509_get_issuer_name(ca));

    wolfSSL_X509_add_altname(x509, "example alt name", ASN_DNS_TYPE);

    /* set valid date */
    notAfter  = wolfSSL_ASN1_TIME_adj(NULL, time(NULL), 365, 0);
    notBefore = wolfSSL_ASN1_TIME_adj(NULL, time(NULL), 0, 0);
    wolfSSL_X509_set_notAfter(x509, notAfter);
    wolfSSL_X509_set_notBefore(x509, notBefore);

    wolfSSL_X509_set_pubkey(x509, pkey);
    md = wolfSSL_EVP_sha256();
    if (wolfSSL_X509_sign(x509, cakey, md) <= 0) {
        printf("error signing cert\n");
        ret = -1;
        goto end;
    }

    /* write bio out */
    bio = wolfSSL_BIO_new_fp(stdout, BIO_NOCLOSE);
    if (bio != NULL)
        wolfSSL_PEM_write_bio_X509(bio, x509);
    wolfSSL_BIO_free(bio);


#ifdef HAVE_PKCS12
    /* create a PKCS12 bundle from the cert + key */
    printPKCS12(x509, rsa);
#endif
end:
    wolfSSL_X509_free(x509);
    wolfSSL_X509_free(ca);
    wolfSSL_EVP_PKEY_free(pkey);
    wolfSSL_EVP_PKEY_free(cakey);

    return ret;
}

