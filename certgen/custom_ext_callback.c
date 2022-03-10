/*
 * custom_ext_callback.c
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

/* This is based off of ecc-key-decode.c */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <stdio.h>

#if defined(WOLFSSL_ASN_TEMPLATE) && defined(WOLFSSL_CERT_REQ) && \
    defined(WOLFSSL_CERT_GEN) && defined(WOLFSSL_TEST_CERT) && \
    defined(HAVE_OID_DECODING) && defined(WOLFSSL_CUSTOM_OID) && \
    defined(WOLFSSL_CERT_EXT)

static int myCustomExtCallback(const word16* oid, word32 oidSz, int crit,
                               const unsigned char* der, word32 derSz) {
    word32 i;

    printf("Custom Extension found!\n");
    printf("(");
    for (i = 0; i < oidSz; i++) {
        printf("%d", oid[i]);
        if (i < oidSz - 1) {
            printf(".");
        }
    }
    printf(") : ");

    if (crit) {
        printf("CRITICAL");
    } else {
        printf("NOT CRITICAL");
    }
    printf(" : ");

    for (i = 0; i < derSz; i ++) {
        printf("%x ", der[i]);
    }
    printf("\n");

    /* NOTE: by returning zero, we are accepting this extension and informing
     *       wolfSSL that it is acceptable. If you find an extension that you
     *       do not find acceptable, you should return an error. The standard 
     *       behavior upon encountering an unknown extension with the critical
     *       flag set is to return ASN_CRIT_EXT_E. For the sake of brevity,
     *       this example is always accepting every extension; you should use
     *       different logic. */
    return 0;
}

static void check_ret(char*, int);

#ifndef MAX_BUF
#define MAX_BUF 4096
#endif

int main(int argc, char** argv)
{
    DecodedCert decodedCert;
    FILE* file;
    byte derBuffer[MAX_BUF];
    size_t bytes;
    int ret;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    if (argc != 2) {
        printf("Usage: %s certifcate.der\n", argv[0]);
        return 0;
    }

    wolfCrypt_Init();

    /* TODO: change this once we start generating our own. */ 
    file = fopen(argv[1], "rb");
    if (!file) {
        printf("Failed to open file\n");
        exit(-99);
    }

    bytes = fread(derBuffer, 1, sizeof(derBuffer), file);
    fclose(file);

    printf("read bytes = %d\n", (int) bytes);
    if (bytes <= 0) {
        return -1;
    }

    InitDecodedCert(&decodedCert, derBuffer, (word32) bytes, 0);

    ret = wc_SetUnknownExtCallback(&decodedCert, myCustomExtCallback);
    check_ret("wc_SetUnknownExtCallback", ret);

    ret = ParseCert(&decodedCert, CERT_TYPE, NO_VERIFY, NULL);
    check_ret("ParseCert", ret);

    FreeDecodedCert(&decodedCert);

    wolfCrypt_Cleanup();

    printf("Success\n");

    return 0;
}

static void check_ret(char* call, int ret)
{
    if (ret != 0) {
        printf("call: %s\n", call);
        printf("ret = %d\n", ret);
        exit(-99);
    }
    return;
}

#else

int main(void)
{
    printf("Not compiled in: Build wolfSSL using ./configure --enable-asn=template  --enable-certreq --enable-certgen CFLAGS=\"-DWOLFSSL_TEST_CERT -DHAVE_OID_DECODING -DWOLFSSL_CUSTOM_OID -DWOLFSSL_CERT_EXT\"\n");
    return 0;
}

#endif

