/* rsa-private-encrypt-app.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/ssl.h>
/* certs_test.h contains der formatted key buffer rsa_key_der_2048 */
#ifdef USE_CERT_BUFFERS_2048
#include <wolfssl/certs_test.h>
#else
    #error "Please define USE_CERT_BUFFERS_2048 when building wolfSSL!"
#endif

void check_ret(int val, char* fail);

#define RSA_TEST_BYTES 256 /* 256 bytes * 8 = 2048-bit key length */
#define AES_KEY_SZ 32 /* 32*8 = 256-bit AES KEY */
#define HEAP_HINT NULL

int main(void)
{
    int    ret;
    size_t bytes;
    WC_RNG rng;
    RsaKey key;
    word32 idx = 0;
    const word32 outSz   = RSA_TEST_BYTES;
    const byte in[] = "Thisismyfakeaeskeythatis32bytes!";
    word32 inLen = XSTRLEN((const char*)in);

    byte tmp[sizeof_client_key_der_2048];
    byte out[RSA_TEST_BYTES];
    byte plain[RSA_TEST_BYTES];

    char fName[] = "encryptedAesKey";
    FILE* fStream;

    /* initialize stack structures */
    XMEMSET(&rng, 0, sizeof(rng));
    XMEMSET(&key, 0, sizeof(key));
    XMEMSET(&out, 0, sizeof(out));
    XMEMSET(&plain, 0, sizeof(plain));

    bytes = (size_t)sizeof_client_key_der_2048;

    /* Copy in existing Private RSA key into "tmp" to use for encrypting */
    XMEMCPY(tmp, client_key_der_2048, (size_t)sizeof_client_key_der_2048);

    /* Initialize the RSA key */
    ret = wc_InitRsaKey(&key, HEAP_HINT);
    check_ret(ret, "wc_InitRsaKey_ex");

    /* Decode the private key from buffer "tmp" into RsaKey stucture "key"  */
    ret = wc_RsaPrivateKeyDecode(tmp, &idx, &key, (word32)bytes);
    check_ret(ret, "wc_RsaPrivateKeyDecode");

    /* Initialize the RNG structure */
    ret = wc_InitRng(&rng);
    check_ret(ret, "wc_InitRng");

    /* Implement RSA blinding to defeat side-channel attacks */
#ifdef WC_RSA_BLINDING /* HIGHLY RECOMMENDED! */
    ret = wc_RsaSetRNG(&key, &rng);
    check_ret(ret, "wc_RsaSetRNG");
#endif

    /* Sign the AES key effectively "Encrypting it with the private key" */
    ret = wc_RsaSSL_Sign(in, inLen, out, outSz, &key, &rng);
    check_ret(ret, "wc_RsaSSL_Sign");

    /* Output to file */
    fStream = fopen(fName, "wb");
    if (!fStream) {
        printf("Failed to open file: %s\n", fName);
        return -99;
    }
    ret = (int) fwrite(out, 1, RSA_TEST_BYTES, fStream);
    fclose(fStream);
    if (ret <= 0) {
        printf("Something went wrong writing to file %s\n", fName);
    }

    return ret;
}

void check_ret(int val, char* fail)
{
    if (val < 0) {
        printf("%s Failed with error %d\n", fail, val);
        exit(-99);
    }
    return;
}
