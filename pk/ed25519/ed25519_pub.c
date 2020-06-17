/* ed25519_pub.c
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

/* example code for extracting public from private */
/* 
./configure && make && sudo make install
gcc -lwolfssl -o ed25519_pub ed25519_pub.c
*/

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/logging.h>

#include <stdint.h>
#include <stdio.h>

#ifdef HAVE_ED25519

/* Private key "k" only */
static const uint8_t kPrivKey[] = {
    /* k */
    0x1e, 0xe7, 0x70, 0x07, 0xd3, 0x30, 0x94, 0x39, 
    0x28, 0x90, 0xdf, 0x23, 0x88, 0x2c, 0x4a, 0x34, 
    0x15, 0xdb, 0x4c, 0x43, 0xcd, 0xfa, 0xe5, 0x1f, 
    0x3d, 0x4c, 0x37, 0xfe, 0x59, 0x3b, 0x96, 0xd8
};

static void print_hex(uint8_t* data, int sz)
{
    int i;
    for (i = 0; i < sz; i++) {
        printf("%02X ", data[i]);
        if (i > 0 && ((i+1) % 16) == 0)
            printf("\n");
    }
    printf("\n");
}

int main()
{
    int ret;
    ed25519_key ed25519;
    uint8_t pubKey[ED25519_PUB_KEY_SIZE];

    /* Setup the ECC key */
    ret = wc_ed25519_init(&ed25519);
    if (ret < 0) {
        return ret;
    }

    memset(pubKey, 0, sizeof(pubKey));

    /* Import private key "k" */
    ret = wc_ed25519_import_private_only(kPrivKey, sizeof(kPrivKey), &ed25519);

    /* Make public key into buffer */
    if (ret == 0) {
        ret = wc_ed25519_make_public(&ed25519, pubKey, ED25519_PUB_KEY_SIZE);
    }

    printf("Public Key: %d\n", ED25519_PUB_KEY_SIZE);
    print_hex(pubKey, ED25519_PUB_KEY_SIZE);

    wc_ed25519_free(&ed25519);
    return ret;
}

#else

int main()
{
    printf("wolfSSL needs to be built with Ed25519 enabled\n");
    return -1;
}

#endif /* HAVE_ED25519 */

