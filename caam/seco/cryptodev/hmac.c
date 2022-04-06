/* hmac.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
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
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/port/caam/wolfcaam.h>

#include <stdio.h>

int main(int argc, char** argv)
{
    int ret, i;
    Hmac hmac;
    byte hash[WC_SHA256_DIGEST_SIZE];

    const byte key[]   = "Jefe";
    const byte input[] = "what do ya want for nothing?";
    const char expected[] =
               "\x5b\xdc\xc1\x46\xbf\x60\x75\x4e\x6a\x04\x24\x26\x08\x95\x75"
               "\xc7\x5a\x00\x3f\x08\x9d\x27\x39\x83\x9d\xec\x58\xb9\x64\xec"
               "\x38\x43";
    word32 inLen  = (word32)XSTRLEN((char*)input);
    int outLen = WC_SHA256_DIGEST_SIZE;

    XMEMSET(hash, 0, outLen);
    if (wolfCrypt_Init() != 0) {
        printf("Error initializing wolfSSL library\n");
        return -1;
    }

    /* initialize using WOLFSSL_CAAM_DEVID to use devcrypto code path */
    if (wc_HmacInit(&hmac, NULL, WOLFSSL_CAAM_DEVID) != 0) {
        printf("Issue initializing hmac\n");
    }

    ret = wc_HmacSetKey(&hmac, WC_SHA256, key, (word32)XSTRLEN((char*)key));
    if (ret != 0)
        printf("Issue with set key\n");

    ret = wc_HmacUpdate(&hmac, input, inLen);
    if (ret != 0)
        printf("Issue with update\n");

    ret = wc_HmacFinal(&hmac, hash);
    if (ret != 0)
        printf("Issue with hmac final\n");

    if (XMEMCMP(hash, expected, WC_SHA256_DIGEST_SIZE) != 0)
        printf("result did not match expected result\n");

    wc_HmacFree(&hmac);

    printf("Result : ");
    for (i = 0; i < outLen; i++)
        printf("%02X", hash[i]);
    printf("\n");

    wolfCrypt_Cleanup();
    return 0;
}
