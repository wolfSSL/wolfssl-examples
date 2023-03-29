/* smime.c
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

/* Create an SMIME bundle, uses steps similar to PKCS7_sign() but this is
 * with native PKCS7 API allowing for control over the devID set */
static int Create(byte* smime, int* smimeSz, byte* key, int keySz,
    byte* signer, int signerSz, byte* content, int contentSz, int detatched)
{
    WOLFSSL_BIO *out, *in;
    WOLFSSL_PKCS7* pkcs7Compat;
    PKCS7* pkcs7;
    int ret = 0;

    out = wolfSSL_BIO_new(wolfSSL_BIO_s_mem());
    in  = wolfSSL_BIO_new(wolfSSL_BIO_s_mem());
    if (out == NULL || in == NULL) {
        printf("Failed to create bio's\n");
        return -1;
    }

    pkcs7Compat = (WOLFSSL_PKCS7*)wolfSSL_PKCS7_new();
    if (pkcs7Compat == NULL) {
        ret = MEMORY_E;
    }
    else {
        pkcs7 = &(pkcs7Compat->pkcs7);

        /* can change devID here and set signer */
        ret = wc_PKCS7_Init(pkcs7, NULL, INVALID_DEVID);
    }

    if (ret == 0) {
        ret = wc_PKCS7_InitWithCert(pkcs7, signer, signerSz);
    }

    if (ret == 0) {
        /* set signer private key, data types, defaults */
        pkcs7->privateKey   = key;
        pkcs7->privateKeySz = keySz;
        pkcs7->contentOID   = DATA;  /* inner content default is DATA */
        pkcs7->hashOID      = SHA256h;  /* default to SHA-256 hash type */

        /* type of SMIME */
        pkcs7Compat->type = SIGNED_DATA;

        /* add additional chain certs if provided */
        /* wc_PKCS7_AddCertificate(pkcs7, additionalCert, additionalCertSz); */

        /* set detached flag */
        if (detatched & PKCS7_DETACHED) {
            ret = wc_PKCS7_SetDetached(pkcs7, 1);
        }

        /* setup content to sign */
        if (ret == 0) {
            if (wolfSSL_BIO_write(in, content, contentSz) != contentSz) {
                ret = -1;
            }
        }

        if (ret == 0) {
            if (wolfSSL_SMIME_write_PKCS7(out, (PKCS7*)pkcs7Compat, in,
                    detatched) != WOLFSSL_SUCCESS) {
                printf("SMIME write failed!\n");
                ret = -1;
            }
        }
    }

    if (ret == 0) {
        ret = wolfSSL_BIO_read(out, smime, *smimeSz);
        if (ret > 0) {
            *smimeSz = ret;
            ret = 0;
        }
        else {
            ret = -1;
        }
    }

    wolfSSL_BIO_free(in);
    wolfSSL_BIO_free(out);
    wolfSSL_PKCS7_free((PKCS7*)pkcs7Compat);
    return ret;
}


/* read private key and signer certificate in DER format */
static int ReadKeyAndCert(char* keyFile, char* certFile, byte* key, int* keySz,
    byte* cert, int* certSz)
{
    int ret;
    XFILE f;

    f = XFOPEN(keyFile, "rb");
    if (f == NULL) {
        printf("Error opening file %s\n", keyFile);
        return -1;
    }
    else {
        ret = XFREAD(key, 1, *keySz, f);
        if (ret >= 0) {
            *keySz = ret;
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
    byte key[2048];
    int keySz = 2048;

    byte cert[2048];
    int certSz = 2048;

    byte smime[3072];
    int smimeSz = 3072;

    byte content[] = "Test content to sign";
    int contentSz = sizeof(content);

    int ret;
    int i;

    if (argc != 3) {
        printf("Use ./smime <der key file> <der cert file>\n");
        return -1;
    }

    ret = ReadKeyAndCert(argv[1], argv[2], key, &keySz, cert, &certSz);
    if (ret == 0)
        ret = Create(smime, &smimeSz, key, keySz, cert, certSz,
                content, contentSz, 0);
    if (ret == 0) {
        printf("Generated SMIME : ");
        for (i = 0; i < smimeSz; i++)
            printf("%02X", smime[i]);
        printf("\n");
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
