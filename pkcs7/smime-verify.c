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

static int Verify(byte* smime, int smimeSz, byte* ca, int caSz, byte* contentIn, int contentInSz, int detached)
{
    WOLFSSL_PKCS7* pkcs7Compat = NULL;
    WOLFSSL_BIO *in, *content = NULL;
    WOLFSSL_BIO *multi = NULL;
    WOLFSSL_X509* x509 = NULL;
    WOLFSSL_X509_STORE* store = NULL;
    int ret = 0;

    in = wolfSSL_BIO_new(wolfSSL_BIO_s_mem());
    if (in == NULL) {
        ret = MEMORY_E;
    }

    if (ret == 0) {

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

    if (ret == 0 && contentIn != NULL) {
        pkcs7Compat->pkcs7.content  = contentIn;
        pkcs7Compat->pkcs7.contentSz = contentInSz;
        wc_PKCS7_SetDetached(&pkcs7Compat->pkcs7, 1);
    }

    if (ret == 0) {
        content = wolfSSL_BIO_new(wolfSSL_BIO_s_mem());
        ret = wolfSSL_PKCS7_verify((PKCS7*)pkcs7Compat, NULL, store, multi,
            content, detached);
        if (ret == WOLFSSL_SUCCESS) {
            ret = 0;
        }
        else {
            /* print out certificate that could not be verified */
            int i;
            byte* pt = pkcs7Compat->pkcs7.verifyCert;

            printf("Could not verify certificate :");
            for (i = 0; i < pkcs7Compat->pkcs7.verifyCertSz; i++) {
                printf("%02X", pt[i]);
            }
            printf("\n");
            ret = -1;
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
    wolfSSL_BIO_free(multi);
    wolfSSL_PKCS7_free((PKCS7*)pkcs7Compat);
    wolfSSL_X509_free(x509);
    wolfSSL_X509_STORE_free(store);
    return ret;
}


/* read private smime and signer certificate in DER format */
static int ReadSmimeAndCert(char* smimeFile, char* certFile, char* contentFile,
    byte* smime,
    int* smimeSz, byte* cert, int* certSz, byte* content, int* contentSz)
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
            if (ret == *smimeSz) {
                printf("smime read in was larger than buffer\n");
                XFCLOSE(f);
                return -1;
            }
            else {
                *smimeSz = ret;
                ret = 0;
                XFCLOSE(f);
            }
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
            if (ret == *certSz) {
                printf("Cert read in was larger than buffer\n");
                XFCLOSE(f);
                return -1;
            }
            else {
                *certSz = ret;
                ret = 0;
                XFCLOSE(f);
            }
        }
    }

    f = XFOPEN(contentFile, "rb");
    if (f == NULL) {
        printf("Error opening file %s\n", contentFile);
        return -1;
    }
    else {
        ret = XFREAD(content, 1, *contentSz, f);
        if (ret >= 0) {
            if (ret == *contentSz) {
                printf("Cert read in was larger than buffer\n");
                XFCLOSE(f);
                return -1;
            }
            else {
                *contentSz = ret;
                ret = 0;
                XFCLOSE(f);
            }
        }
    }

    return ret;
}

int main(int argc, char** argv)
{
    byte cert[4096];
    int certSz = 4096;

    byte smime[10000];
    int smimeSz = 10000;

    byte content[10000];
    int contentSz = 10000;

    int ret;

    if (argc != 4) {
        printf("Use ./smime-verify <smime file> <der cert file> <content file>\n");
        return -1;
    }

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        printf("Failure to initialize wolfSSL library\n");
        return -1;
    }

    ret = ReadSmimeAndCert(argv[1], argv[2], argv[3], smime, &smimeSz, cert,
        &certSz, content, &contentSz);
    if (ret == 0) {
        ret = Verify(smime, smimeSz, cert, certSz, content, contentSz, 0);
        if (ret == 0) {
            printf("Verify Success\n");
        }
        else {
            printf("Verify Failed\n");
        }
    }

    wolfSSL_Cleanup();
    return ret;
}
#else
int main()
{
    printf("wolfSSL was compiled with out HAVE_SMIME support\n");
    return 0;
}
#endif
