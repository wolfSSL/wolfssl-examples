/* certverify.c
 *
 * Copyright (C) 2006-2018 wolfSSL Inc.
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
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/test.h>

int main(int argc, char** argv)
{
    FILE* file;
    int ret;
    X509_STORE* store = NULL;
    X509_STORE_CTX* ctx = NULL;
    X509 *ca = NULL, *cert = NULL;

    const char* caCert     = "../certs/ca-cert.pem";
    const char* verifyCert = "../certs/server-cert.pem";

    /* check if CA and x509 were passed in */
    if (argc > 1 && strcmp(argv[1], "-h") == 0) {
        printf("%s <ca pem file> <cert pem file>\n", argv[0]);
        return 1;
    }

    if (argc == 3) {
        caCert = argv[1];
        verifyCert = argv[2];
    }

    wolfSSL_Init();
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    /* read in CA cert */
    file = fopen(caCert, "rb");
    if (file == NULL) {
        printf("Failed to open %s\n", caCert);
        ret = -1; goto exit;
    }
    ca = PEM_read_X509(file, NULL, NULL, NULL);
    fclose(file);
    if (ca == NULL) {
        printf("Failed to convert %s to X509\n", caCert);
        ret = -1; goto exit;
    }

    /* read in cert to verify */
    file = fopen(verifyCert, "rb");
    if (file == NULL) {
        printf("Failed to open %s\n", verifyCert);
        ret = -1; goto exit;
    }
    cert = PEM_read_X509(file, NULL, NULL, NULL);
    fclose(file);
    if (cert == NULL) {
        printf("Failed to convert %s to X509\n", verifyCert);
        ret = -1; goto exit;
    }

    /* setup the x509 cert store */
    store = X509_STORE_new();
    if (X509_STORE_add_cert(store, ca) != WOLFSSL_SUCCESS) {
        printf("Failed to add CA to X509 STORE\n");
        ret = -1; goto exit;
    }

    /* setup the x509 cert store contex */
    ctx = X509_STORE_CTX_new();
    if (X509_STORE_CTX_init(ctx, store, cert, NULL) != WOLFSSL_SUCCESS) {
        printf("Failed to initialize X509 STORE CTX\n");
        ret = -1; goto exit;
    }

    /* attempt verify */
    if (X509_verify_cert(ctx) == WOLFSSL_SUCCESS) {
        printf("Verified certificate %s\n", verifyCert);
    }
    else {
        int err = X509_STORE_CTX_get_error(ctx);
        printf("Failed to verified certificate %s\n", verifyCert);
        printf("Error [%d] reason [%s]\n", err, ERR_reason_error_string(err));
    }

exit:
    X509_STORE_free(store);
    X509_STORE_CTX_free(ctx);
    X509_free(ca);
    X509_free(cert);
    wolfSSL_Cleanup();

    return ret;
}
