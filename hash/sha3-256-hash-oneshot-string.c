/* sha3-256-hash-oneshot-string.c
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


#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

int main(int argc, char** argv)
{
    int ret = -1;
#ifdef WOLFSSL_SHA3
    unsigned char hash[WC_SHA3_256_DIGEST_SIZE];
    const char* string = "String to hash";
    int i;

    printf("String to hash: '%s'\n", string);

    ret = wc_Sha3_256Hash((unsigned char*)string, XSTRLEN(string), hash);
    if (ret != 0) {
        printf("ERROR: Hash operation failed");
        goto prog_end;
    }

    printf("Hash result is: ");
    for (i = 0; i < WC_SHA3_256_DIGEST_SIZE; i++)
        printf("%02x", hash[i]);
    printf("\n");

prog_end:
#else
    printf("Please enable sha3 (--enable-sha3) in wolfCrypt\n");
#endif
    return ret;
}
