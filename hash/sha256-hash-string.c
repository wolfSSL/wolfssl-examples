/* sha256-hash-string.c
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
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

int main(int argc, char** argv)
{
    int ret = -1;
#ifndef NO_SHA256
    wc_Sha256 sha256;
    unsigned char hash[WC_SHA256_DIGEST_SIZE];
    const char* string = "String to hash";
    int i;

    printf("String to hash: '%s'\n", string);

    ret = wc_InitSha256(&sha256);
    if (ret != 0) {
        printf("Failed to initialize sha structure\n");
        goto prog_end;
    }

    ret = wc_Sha256Update(&sha256, (unsigned char*)string, XSTRLEN(string));
    if (ret != 0) {
        printf("Failed to update the hash\n");
        goto prog_end;
    }

    ret = wc_Sha256Final(&sha256, hash);
    if (ret != 0) {
        printf("ERROR: Hash operation failed");
        goto prog_end;
    }

    printf("Hash result is: ");
    for (i = 0; i < WC_SHA256_DIGEST_SIZE; i++)
        printf("%02x", hash[i]);
    printf("\n");

prog_end:
    wc_Sha256Free(&sha256);
#else
    printf("Please enable sha256 (--enable-sha256) in wolfCrypt\n");
#endif
    return ret;
}
