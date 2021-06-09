/* ecc-key-export.c
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


#include <stdio.h>
#include <stdint.h>

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

/* Build wolfSSL with:
./configure --enable-ecccustcurves CFLAGS="-DWOLFSSL_DER_TO_PEM -DHAVE_ECC_KOBLITZ"
make
sudo make install
*/

#define MAX_CERT_SIZE       4096
#define TEST_ECC_KEY_SZ     32
#ifdef HAVE_ECC_KOBLITZ
#define TEST_ECC_KEY_CURVE  ECC_SECP256K1
#else
#define TEST_ECC_KEY_CURVE  ECC_SECP256R1
#endif

#define XSTRINGIFY(a) STRINGIFY(a)
#define STRINGIFY(a) #a

int main(void)
{
    int ret;
    byte der[MAX_CERT_SIZE];
    word32 derSz;
#ifdef WOLFSSL_DER_TO_PEM
    byte pem[MAX_CERT_SIZE];
    word32 pemSz;
#endif
    WC_RNG rng;
    ecc_key key;
    FILE *fp;

    wolfSSL_Debugging_ON();

    ret = wolfCrypt_Init();
    if (ret != 0) {
        printf("wolfCrypt_Init error %s (%d)\n", wc_GetErrorString(ret), ret);
        return -1;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("wc_InitRng error %s (%d)\n", wc_GetErrorString(ret), ret);
        return -1;
    }

    ret = wc_ecc_init(&key);
    if (ret != 0) {
        printf("wc_ecc_init error %s (%d)\n", wc_GetErrorString(ret), ret);
        return -1;
    }

    ret = wc_ecc_make_key_ex(&rng, TEST_ECC_KEY_SZ, &key, TEST_ECC_KEY_CURVE);
    if (ret != 0) {
        printf("wc_ecc_make_key_ex error %s (%d)\n", wc_GetErrorString(ret), ret);
        return -1;
    }
    printf("ECC Key Generated: %d bits, curve %s\n", TEST_ECC_KEY_SZ * 8, XSTRINGIFY(TEST_ECC_KEY_CURVE));

    memset(der, 0, sizeof(der));
    ret = wc_EccKeyToDer(&key, der, sizeof(der));
    if (ret < 0) {
        printf("wc_EccKeyToDer error %s (%d)\n", wc_GetErrorString(ret), ret);
        return -1;
    }
    derSz = ret;

    fp = fopen("./" XSTRINGIFY(TEST_ECC_KEY_CURVE) ".der", "wb");
    if (!fp) {
        printf("Error opening %s for write\n",
            "./" XSTRINGIFY(TEST_ECC_KEY_CURVE) ".der");
        return -1;
    }
    fwrite(der, derSz, 1, fp);
    fclose(fp);

    printf("ECC Private Key Exported to %s\n",
        "./" XSTRINGIFY(TEST_ECC_KEY_CURVE) ".der");

#ifdef WOLFSSL_DER_TO_PEM
    memset(pem, 0, sizeof(pem));
    ret = wc_DerToPem(der, derSz, pem, sizeof(pem), ECC_PRIVATEKEY_TYPE);
    if (ret < 0) {
        printf("wc_DerToPem error %s (%d)\n", wc_GetErrorString(ret), ret);
        return -1;
    }
    pemSz = ret;

    fp = fopen("./" XSTRINGIFY(TEST_ECC_KEY_CURVE) ".pem", "wb");
    if (!fp) {
        printf("Error opening %s for write\n",
            "./" XSTRINGIFY(TEST_ECC_KEY_CURVE) ".pem");
        return -1;
    }
    fwrite(pem, pemSz, 1, fp);
    fclose(fp);

    printf("ECC Private Key Exported to %s\n",
        "./" XSTRINGIFY(TEST_ECC_KEY_CURVE) ".pem");
#endif

    memset(der, 0, sizeof(der));
    ret = wc_EccPublicKeyToDer(&key, der, sizeof(der), TEST_ECC_KEY_CURVE);
    if (ret < 0) {
        printf("wc_EccPublicKeyToDer error %s (%d)\n", wc_GetErrorString(ret), ret);
        return -1;
    }
    derSz = ret;

    fp = fopen("./" XSTRINGIFY(TEST_ECC_KEY_CURVE) "_pub.der", "wb");
    if (!fp) {
        printf("Error opening %s for write\n",
            "./" XSTRINGIFY(TEST_ECC_KEY_CURVE) "_pub.der");
        return -1;
    }
    fwrite(der, derSz, 1, fp);
    fclose(fp);

    printf("ECC Public Key Exported to %s\n",
        "./" XSTRINGIFY(TEST_ECC_KEY_CURVE) "_pub.der");

#ifdef WOLFSSL_DER_TO_PEM
    memset(pem, 0, sizeof(pem));
    ret = wc_DerToPem(der, derSz, pem, sizeof(pem), ECC_PUBLICKEY_TYPE);
    if (ret < 0) {
        /* try old type */
        ret = wc_DerToPem(der, derSz, pem, sizeof(pem), PUBLICKEY_TYPE);
    }
    if (ret < 0) {
        printf("wc_DerToPem error %s (%d)\n", wc_GetErrorString(ret), ret);
        return -1;
    }
    pemSz = ret;

    fp = fopen("./" XSTRINGIFY(TEST_ECC_KEY_CURVE) "_pub.pem", "wb");
    if (!fp) {
        printf("Error opening %s for write\n",
            "./" XSTRINGIFY(TEST_ECC_KEY_CURVE) "_pub.pem");
        return -1;
    }
    fwrite(pem, pemSz, 1, fp);
    fclose(fp);

    printf("ECC Public Key Exported to %s\n",
        "./" XSTRINGIFY(TEST_ECC_KEY_CURVE) "_pub.pem");
#endif

    wc_ecc_free(&key);
    wc_FreeRng(&rng);
    wolfCrypt_Cleanup();

    return 0;
}