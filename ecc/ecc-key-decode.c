/* ecc-key-decode.c
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


#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/wc_port.h>

#include <stdio.h>

#if defined(WOLFSSL_TEST_CERT) || defined(OPENSSL_EXTRA) || \
    defined(OPENSSL_EXTRA_X509_SMALL)

static void check_ret(char*, int);

#ifndef MAX_BUF
#define MAX_BUF 4096
#endif

int main(void)
{
    DecodedCert decodedCert;
    FILE* file;
    byte derBuffer[MAX_BUF];
    size_t bytes;
    ecc_key eccKey;
    int ret;
    word32 inOutIdx = 0;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    wolfCrypt_Init();

    file = fopen("../certs/client-ecc-cert.der", "rb");
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

    ret = ParseCert(&decodedCert, CERT_TYPE, NO_VERIFY, NULL);
    check_ret("ParseCert", ret);

    ret = wc_ecc_init(&eccKey);
    check_ret("wc_ecc_init", ret);

    printf("decodedCert.pubKeySize %d\n", decodedCert.pubKeySize);
    WOLFSSL_BUFFER(decodedCert.publicKey, decodedCert.pubKeySize);

    ret = wc_EccPublicKeyDecode(decodedCert.publicKey, &inOutIdx, &eccKey, decodedCert.pubKeySize);
    check_ret("wc_EccPublicKeyDecode", ret);

    printf("publickey size: %d\n", wc_ecc_size(&eccKey));

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
    printf("Not compiled in: Must build wolfSSL using ./configure --enable-opensslextra or ./configure CFLAGS=-DWOLFSSL_TEST_CERT\n");
    return 0;
}

#endif
