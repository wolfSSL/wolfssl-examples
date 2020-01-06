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


#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/ecc.h>

#include <stdio.h>
#include <stdlib.h>

int main()
{
    ecc_key key;
    byte der[4096];
    byte buf[4096];
    word32 idx = 0;
    FILE* derFile;
    size_t sz;

    RNG rng;

    wc_InitRng(&rng);
    wc_ecc_init(&key);

    if (wc_ecc_make_key(&rng, 32, &key) != 0) {
        printf("error making ecc key\n");
        return -1;
    }

    /* write private key */
    if (wc_EccKeyToDer(&key, der, sizeof(der)) < 0) {
        printf("error in ecc to der\n");
        return -1;
    }
    printf("writing private key to ecc-key.der\n");
    derFile = fopen("ecc-key.der", "w");
    if (!derFile) {
        printf("error loading file\n");
        return -1;
    }

    sz = fwrite(der, 1, 4096, derFile);
    fclose(derFile);
    wc_ecc_free(&key);

    /* open and read from der file */
    printf("reading in private key\n");
    derFile = fopen("ecc-key.der", "rb");
    if (!derFile) {
        printf("error reading from file\n");
        return -1;
    }

    sz = fread(buf, 1, 4096, derFile);
    fclose(derFile);

    /* load private ecc key */
    printf("storing private key in ecc struct\n");
    wc_ecc_init(&key);
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
    if (wc_ecc_make_key(&rng, 32, &key) != 0) {
        printf("error making ecc key\n");
        return -1;
    }

    printf("storing public key into ecc-public.x963\n");
    memset(buf, 0, sizeof(buf));
    idx = sizeof(buf);
    if (wc_ecc_export_x963(&key, buf, &idx) != 0) {
        printf("error exporting public ecc key\n");
        return -1;
    }

    derFile = fopen("ecc-public.x963", "w"); /* reused the derFile pointer */
    if (!derFile) {
        printf("error loading file\n");
        return -1;
    }
    sz = fwrite(buf, 1, idx, derFile);

    /* close stuff up */
    fclose(derFile);
    wc_ecc_free(&key);
    wc_FreeRng(&rng);
    return 0;
}

