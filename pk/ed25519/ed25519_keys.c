/* ed25519_keys.c
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
gcc -lwolfssl -o ed25519_keys ecc_keys.c
*/

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/random.h>

#include <stdio.h>
#include <stdlib.h>

#define MAX_DER_SZ 128

#if defined(HAVE_ED25519) && defined(WOLFSSL_KEY_GEN)

/* private key filename */
static const char* privFile = "ed25519-key.der";
/* public key filename */
static const char* pubFile = "ed25519-pubkey.der";

int write_file(const char* name, const char* filename, byte* data, size_t sz)
{
    FILE* derFile;

    printf("writing %s to %s (%d bytes)\n", name, filename, (int)sz);
    derFile = fopen(filename, "w");
    if (!derFile) {
        printf("error opening file\n");
        return -1;
    }

    fwrite(data, 1, sz, derFile);
    fclose(derFile);

    return 0;
}

int read_file(const char* name, const char* filename, byte* buf, size_t* sz)
{
    FILE* derFile;

    printf("reading in %s - %s\n", name, filename);
    derFile = fopen(filename, "rb");
    if (!derFile) {
        printf("error opening file\n");
        return -1;
    }
    *sz = fread(buf, 1, *sz, derFile);
    printf("read %d bytes\n", (int)*sz);
    fclose(derFile);

    return 0;
}

int main()
{
    int ret;
    ed25519_key key;
    WC_RNG rng;
    byte der[MAX_DER_SZ];
    byte buf[MAX_DER_SZ];
    word32 idx;
    size_t sz;

    wc_InitRng(&rng);
    wc_ed25519_init(&key);

    /*
     * Make a private Ed25510 key
     */

    /* make a new key */
    ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    if (ret != 0) {
        printf("error %d making Ed25519 key\n", ret);
        return ret;
    }

    /*
     * Store private Ed25510 key
     */

    /* encode private key */
    ret = wc_Ed25519KeyToDer(&key, der, sizeof(der));
    if (ret < 0) {
        printf("error %d in Ed25519 to der\n", ret);
        return ret;
    }
    sz = ret;

    /* write private key to file */
    ret = write_file("private key", privFile, der, sz);
    if (ret < 0) {
        return ret;
    }

    wc_ed25519_free(&key);

    /*
     * Load private Ed25510 key
     */

    /* read pivate key from file */
    sz = sizeof(buf);
    ret = read_file("private key", privFile, buf, &sz);
    if (ret < 0) {
        return ret;
    }

    wc_ed25519_init(&key);

    /* Decode pivate key */
    printf("Decoding private key in Ed25519 struct\n");
    idx = 0;
    ret = wc_Ed25519PrivateKeyDecode(buf, &idx, &key, (word32)sz);
    if (ret != 0) {
        printf("error decoding private key\n");
        return ret;
    }

    wc_ed25519_free(&key);
    memset(buf, 0, sizeof(buf));

    /*
     *  Or the der file can be loaded into a TLS connection using something like
     *
     * int wc_DerToPem(der, sizeof(der), pemOut, sizeof(pemOut),
     *                                                     ECC_PRIVATEKEY_TYPE);
     *
     * int wolfSSL_use_PrivateKey_file(&ssl, pemOut, SSL_FILETYPE_PEM);
     *
     */

    /* make a new key */
    wc_ed25519_init(&key);
    ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    if (ret != 0) {
        printf("error %d making Ed25519 key\n", ret);
        return ret;
    }

    /*
     * Store public Ed25510 key
     */

    /* encode public key */
    printf("exporting public key\n");
    sz = sizeof(buf);
    ret = wc_ed25519_export_public(&key, buf, (word32*)&sz);
    if (ret != 0) {
        printf("error exporting public Ed25519 key\n");
        return ret;
    }

    /* write public key to file */
    ret = write_file("public key", pubFile, buf, sz);
    if (ret < 0) {
        return ret;
    }

    /* cleanup */
    wc_ed25519_free(&key);
    wc_FreeRng(&rng);

    return 0;
}

#else

int main()
{
    printf("wolfSSL requires Ed25519 and Key Generation\n");
    return 1;
}

#endif /* HAVE_ED25519 && WOLFSSL_KEY_GEN */

