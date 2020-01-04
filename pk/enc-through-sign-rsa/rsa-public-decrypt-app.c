/* rsa-public-decrypt-app.c
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
#define HEAP_HINT NULL

int main(void)
{
    int    ret;
    size_t bytes;
    RsaKey key;
    word32 idx = 0;
    const word32 plainSz = RSA_TEST_BYTES;

    byte tmp[sizeof_client_keypub_der_2048];
    byte out[RSA_TEST_BYTES];
    byte plain[RSA_TEST_BYTES];

    char fName[] = "encryptedAesKey";
    FILE* fStream;

    /* initialize stack structures */
    XMEMSET(&key, 0, sizeof(key));
    XMEMSET(&out, 0, sizeof(out));
    XMEMSET(&plain, 0, sizeof(plain));

    bytes = (size_t)sizeof_client_keypub_der_2048;

    /* Copy in existing Public RSA key into "tmp" to use for decrypting */
    XMEMCPY(tmp, client_keypub_der_2048, (size_t)sizeof_client_keypub_der_2048);

    /* Initialize the RSA key structure */
    ret = wc_InitRsaKey(&key, HEAP_HINT);
    check_ret(ret, "wc_InitRsaKey_ex");

    /* Decode the public key from buffer "tmp" into RsaKey stucture "key"  */
    ret = wc_RsaPublicKeyDecode(tmp, &idx, &key, (word32)bytes);
    check_ret(ret, "wc_RsaPublicKeyDecode");

    fStream = fopen(fName, "rb");
    if (!fStream) {
        printf("Failed to open file: %s\n", fName);
        return -99;
    }
    ret = (int) fread(out, 1, RSA_TEST_BYTES, fStream);
    fclose(fStream);
    if (ret <= 0) {
        printf("Something went wrong, please check the file: %s\n", fName);
        return ret;
    }

    idx = (word32)ret; /* number of bytes read in from the file */
    XMEMSET(plain, 0, plainSz);

    ret = wc_RsaSSL_Verify(out, idx, plain, plainSz, &key);
    check_ret(ret, "wc_RsaSSL_Verify");
    printf("Here is the recovered AES KEY!\n%s\n", plain);

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
