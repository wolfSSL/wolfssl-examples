/* ecc_pub.c
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
gcc -lwolfssl -o ecc_pub ecc_pub.c
*/

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/logging.h>

#include <stdint.h>
#include <stdio.h>

#define ECC_CURVE_SZ 32 /* SECP256R1 curve size in bytes */
#define ECC_CURVE_ID ECC_SECP256R1

/* Private key "d" only */
static const uint8_t kPrivKey[] = {
    /* d */
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
#if defined(HAVE_ECC) && defined(HAVE_ECC_KEY_EXPORT)
    int ret;
    ecc_key ecc;
    uint8_t pubKey[ECC_CURVE_SZ*2];
    uint32_t pubQxSz = ECC_CURVE_SZ, pubQySz = ECC_CURVE_SZ;

    /* Setup the ECC key */
    ret = wc_ecc_init(&ecc);
    if (ret < 0) {
        return ret;
    }

    memset(pubKey, 0, sizeof(pubKey));

    /* Import private key "k" */
    ret = wc_ecc_import_private_key_ex(
        kPrivKey, sizeof(kPrivKey), /* private key "d" */
        NULL, 0,                    /* public (optional) */
        &ecc,
        ECC_CURVE_ID
    );

    /* Export public key */
    if (ret == 0) {
        ret = wc_ecc_make_pub(&ecc, NULL);
    }
    if (ret == 0) {
        ret = wc_ecc_export_public_raw(&ecc, 
            pubKey, &pubQxSz,               /* public Qx */
            pubKey+ECC_CURVE_SZ, &pubQySz   /* public Qy */
        );
    }

    printf("Public Key Qx: %d\n", pubQxSz);
    print_hex(pubKey, ECC_CURVE_SZ);
    printf("Public Key Qy: %d\n", pubQySz);
    print_hex(pubKey+ECC_CURVE_SZ, ECC_CURVE_SZ);

    wc_ecc_free(&ecc);
    return ret;
#else
    printf("wolfSSL needs to be built with ECC and key export enabled\n");
    return -1;
#endif /* HAVE_ECC && HAVE_ECC_KEY_EXPORT */
}