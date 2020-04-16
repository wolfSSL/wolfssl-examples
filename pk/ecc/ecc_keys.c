/* ecc_keys.c
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

/* 
./configure && make && sudo make install
gcc -lwolfssl -o ecc_pub ecc_pub.c
*/

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/random.h>

#include <stdio.h>
#include <stdlib.h>

#define ECC_CURVE_SZ 32 /* SECP256R1 curve size in bytes */
#define ECC_CURVE_ID ECC_SECP256R1

#define MAX_DER_SZ 256

int main()
{
    int ret;
    ecc_key key;
    WC_RNG rng;
    byte der[MAX_DER_SZ];
    byte buf[MAX_DER_SZ];
    word32 idx;
    FILE* derFile;
    size_t sz;

    wc_InitRng(&rng);
    wc_ecc_init(&key);

    ret = wc_ecc_make_key_ex(&rng, ECC_CURVE_SZ, &key, ECC_CURVE_ID);
    if (ret != 0) {
        printf("error %d making ecc key\n", ret);
        return ret;
    }

    /* write private key */
    ret = wc_EccKeyToDer(&key, der, sizeof(der));
    if (ret < 0) {
        printf("error %d in ecc to der\n", ret);
        return ret;
    }
    sz = ret;

    printf("writing private key to ecc-key.der (%d bytes)\n", (int)sz);
    derFile = fopen("ecc-key.der", "w");
    if (!derFile) {
        printf("error loading file\n");
        return -1;
    }

    fwrite(der, 1, sz, derFile);
    fclose(derFile);
    wc_ecc_free(&key);

    /* open and read from der file */
    printf("reading in private key\n");
    derFile = fopen("ecc-key.der", "rb");
    if (!derFile) {
        printf("error reading from file\n");
        return -1;
    }
    sz = fread(buf, 1, sizeof(buf), derFile);
    fclose(derFile);

    /* load private ecc key */
    printf("loading private key in ecc struct\n");
    wc_ecc_init(&key);
    idx = 0;
    if (wc_EccPrivateKeyDecode(buf, &idx, &key, (word32)sz) != 0) {
        printf("error decoding private key\n");
        return -1;
    }
    wc_ecc_free(&key);

    /* Or the der file can be loaded into a TLS connection using something like

       int wc_DerToPem(der, sizeof(der), pemOut, sizeof(pemOut),
                                                           ECC_PRIVATEKEY_TYPE);

       int wolfSSL_use_PrivateKey_file(&ssl, pemOut, SSL_FILETYPE_PEM);

       */

    /* to store a public key */
    wc_ecc_init(&key);
    ret = wc_ecc_make_key_ex(&rng, ECC_CURVE_SZ, &key, ECC_CURVE_ID);
    if (ret != 0) {
        printf("error %d making ecc key\n", ret);
        return ret;
    }

    printf("exporting public key\n");
    memset(buf, 0, sizeof(buf));
    sz = sizeof(buf);
    if (wc_ecc_export_x963(&key, buf, (word32*)&sz) != 0) {
        printf("error exporting public ecc key\n");
        return -1;
    }

    printf("storing public key into ecc-public.x963 (%d bytes)\n", (int)sz);
    derFile = fopen("ecc-public.x963", "w"); /* reused the derFile pointer */
    if (!derFile) {
        printf("error loading file\n");
        return -1;
    }
    fwrite(buf, 1, sz, derFile);

    /* close stuff up */
    fclose(derFile);
    wc_ecc_free(&key);
    wc_FreeRng(&rng);
    return 0;
}
