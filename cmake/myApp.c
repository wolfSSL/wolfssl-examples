/* myApp.c
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

#include <wolfssl/wolfcrypt/settings.h> /* using user_settings.h for example */
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <stdio.h>


int main(int argc, char** argv)
{
    wc_Sha256 sha;
    byte digest[WC_SHA256_DIGEST_SIZE];
    int i, ret;

    if (argc != 2) {
        printf("%s <to be hashed>\n", argv[0]);
        return -1;
    }

    ret = wolfCrypt_Init();
    if (ret == 0) {
        ret = wc_InitSha256(&sha);
    }
    if (ret == 0) {
        ret = wc_Sha256Update(&sha, (const byte*)argv[1],
            (word32)XSTRLEN(argv[1]));
    }
    if (ret == 0) {
        ret = wc_Sha256Final(&sha, digest);
    }

    if (ret != 0) {
        printf("Error %d\n", ret);
    }

    if (ret == 0) {
        printf("Hash in hex : ");
        for (i = 0; i < WC_SHA256_DIGEST_SIZE; i++) {
            printf("%02X", digest[i]);
        }
        printf("\n");
    }

    wolfCrypt_Cleanup();
    return 0;
}
