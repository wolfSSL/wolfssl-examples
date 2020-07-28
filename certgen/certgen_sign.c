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

#define USE_CERT_BUFFERS_2048
#include <wolfssl/certs_test.h>

int main(int argc, char* argv[])
{
    WOLFSSL_X509 *x509 = NULL, *ca = NULL;
    const WOLFSSL_EVP_MD* md;
    const unsigned char* keyp;
    WOLFSSL_EVP_PKEY *pkey = NULL, *cakey = NULL;
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
    keyp = client_key_der_2048;
    pkey = wolfSSL_d2i_PrivateKey(EVP_PKEY_RSA, NULL, &keyp,
            sizeof_client_key_der_2048);
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

    /* set issuer from CA cert */
    wolfSSL_X509_set_issuer_name(x509, wolfSSL_X509_get_issuer_name(ca));

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

end:
    wolfSSL_X509_free(x509);
    wolfSSL_X509_free(ca);
    wolfSSL_EVP_PKEY_free(pkey);
    wolfSSL_EVP_PKEY_free(cakey);

    return ret;
}

