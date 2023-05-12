/* smime-verify.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>


#ifdef HAVE_SMIME

static int Verify(byte* smime, int smimeSz, byte* ca, int caSz, int detached)
{
    WOLFSSL_PKCS7* pkcs7Compat = NULL;
    WOLFSSL_BIO *in, *content = NULL;
    WOLFSSL_X509* x509 = NULL;
    WOLFSSL_X509_STORE* store = NULL;
    int ret = 0;

    in = wolfSSL_BIO_new(wolfSSL_BIO_s_mem());
    if (in == NULL) {
        ret = MEMORY_E;
    }

    if (ret == 0) {
        WOLFSSL_BIO *multi = NULL;

        wolfSSL_BIO_write(in, smime, smimeSz);
        pkcs7Compat = (WOLFSSL_PKCS7*)wolfSSL_SMIME_read_PKCS7(in, &multi);
        if (pkcs7Compat == NULL) {
            printf("Error parsing SMIME\n");
            ret = -1;
        }

        if (multi != NULL) {
            byte* pt;
            int   ptSz, i;

            printf("Multi part message, signed data is : ");
            ptSz = wolfSSL_BIO_get_mem_data(multi, &pt);
            for (i = 0; i < ptSz; i ++)
                printf("%02X", pt[i]);
            printf("\n");
            wolfSSL_BIO_free(multi);
        }
    }

    if (ret == 0) {
        const unsigned char* pt;

        /* set devID */
        pkcs7Compat->pkcs7.devId = INVALID_DEVID;

        pt   = ca;
        x509 = wolfSSL_d2i_X509(NULL, &pt, caSz);
        if (x509 == NULL) {
            printf("Error decoding signer\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        store = wolfSSL_X509_STORE_new();
        if (store == NULL) {
            printf("Error creating cert store\n");
            ret = MEMORY_E;
        }
        else {
            wolfSSL_X509_STORE_add_cert(store, x509);
        }
    }

    if (ret == 0) {
        content = wolfSSL_BIO_new(wolfSSL_BIO_s_mem());
        ret = wolfSSL_PKCS7_verify((PKCS7*)pkcs7Compat, NULL, store, NULL,
            content, detached);
        if (ret == WOLFSSL_SUCCESS) {
            ret = 0;
        }
    }


    if (ret == 0 && content != NULL) {
        byte* pt;
        int   ptSz, i;

        printf("Content found on verify : ");
        ptSz = wolfSSL_BIO_get_mem_data(content, &pt);
        for (i = 0; i < ptSz; i ++)
            printf("%c", pt[i]);
        printf("\n");
    }

    wolfSSL_BIO_free(in);
    wolfSSL_BIO_free(content);
    wolfSSL_PKCS7_free((PKCS7*)pkcs7Compat);
    wolfSSL_X509_free(x509);
    wolfSSL_X509_STORE_free(store);
    return ret;
}


/* read private smime and signer certificate in DER format */
static int ReadSmimeAndCert(char* smimeFile, char* certFile, byte* smime,
    int* smimeSz, byte* cert, int* certSz)
{
    int ret;
    XFILE f;

    f = XFOPEN(smimeFile, "rb");
    if (f == NULL) {
        printf("Error opening file %s\n", smimeFile);
        return -1;
    }
    else {
        ret = XFREAD(smime, 1, *smimeSz, f);
        if (ret >= 0) {
            *smimeSz = ret;
            ret = 0;
            XFCLOSE(f);
        }
    }

    f = XFOPEN(certFile, "rb");
    if (f == NULL) {
        printf("Error opening file %s\n", certFile);
        return -1;
    }
    else {
        ret = XFREAD(cert, 1, *certSz, f);
        if (ret >= 0) {
            *certSz = ret;
            ret = 0;
            XFCLOSE(f);
        }
    }

    return ret;
}

int main(int argc, char** argv)
{
    byte cert[2048];
    int certSz = 2048;

    byte smime[3072];
    int smimeSz = 3072;

    int ret;

    if (argc != 3) {
        printf("Use ./smime-verify <smime file> <der cert file>\n");
        return -1;
    }

    ret = ReadSmimeAndCert(argv[1], argv[2], smime, &smimeSz, cert, &certSz);
    if (ret == 0) {
        ret = Verify(smime, smimeSz, cert, certSz, 0);
        if (ret == 0) {
            printf("Verify Success\n");
        }
        else {
            printf("Verify Failed\n");
        }
    }

    return ret;
}
#else
int main()
{
    printf("wolfSSL was compiled with out HAVE_SMIME support\n");
    return 0;
}
#endif
