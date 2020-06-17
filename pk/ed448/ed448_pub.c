/* ed448_pub.c
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
gcc -lwolfssl -o ed448_pub ed448_pub.c
*/

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ed448.h>
#include <wolfssl/wolfcrypt/logging.h>

#include <stdint.h>
#include <stdio.h>

#ifdef HAVE_ED448

/* Private key "k" only */
static const uint8_t kPrivKey[] = {
    /* k */
    0x5C, 0xEF, 0xDE, 0xFE, 0x14, 0xBD, 0xB4, 0x82,
    0x14, 0x15, 0x35, 0x9C, 0xD0, 0xE8, 0x0E, 0x07,
    0xFD, 0xFE, 0x24, 0xEC, 0xDF, 0x59, 0x28, 0x97,
    0x0A, 0xE7, 0xE1, 0xD6, 0xD5, 0x38, 0x15, 0xE7,
    0xA3, 0xFB, 0x56, 0x79, 0xE5, 0x17, 0x6F, 0x47,
    0xE8, 0x87, 0x6C, 0x8F, 0x32, 0xF0, 0x3F, 0x70,
    0xF5, 0x3F, 0xEB, 0x92, 0x2C, 0x4B, 0xFD, 0xBE,
    0x07
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
    ed448_key ed448;
    uint8_t pubKey[ED448_PUB_KEY_SIZE];

    /* Setup the ECC key */
    ret = wc_ed448_init(&ed448);
    if (ret < 0) {
        return ret;
    }

    memset(pubKey, 0, sizeof(pubKey));

    /* Import private key "k" */
    ret = wc_ed448_import_private_only(kPrivKey, sizeof(kPrivKey), &ed448);

    /* Make public key into buffer */
    if (ret == 0) {
        ret = wc_ed448_make_public(&ed448, pubKey, ED448_PUB_KEY_SIZE);
    }

    printf("Public Key: %d\n", ED448_PUB_KEY_SIZE);
    print_hex(pubKey, ED448_PUB_KEY_SIZE);

    wc_ed448_free(&ed448);
    return ret;
}

#else

int main()
{
    printf("wolfSSL needs to be built with ECC and key export enabled\n");
    return -1;
}

#endif /* HAVE_D448 */

