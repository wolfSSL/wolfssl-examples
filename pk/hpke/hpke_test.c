/* hpke_test.c
 *
 * Copyright (C) 2022 wolfSSL Inc.
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
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/hpke.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#if defined(HAVE_HPKE) && (defined(HAVE_ECC) || defined(HAVE_CURVE25519)) && \
    defined(HAVE_AESGCM)
int test_hpke(Hpke* hpke)
{
    int ret = 0;
    int rngRet = 0;
    WC_RNG rng[1];
    const char* start_text = "this is a test";
    const char* info_text = "info";
    const char* aad_text = "aad";
    byte ciphertext[MAX_HPKE_LABEL_SZ];
    byte plaintext[MAX_HPKE_LABEL_SZ];
    void* receiverKey = NULL;
    void* ephemeralKey = NULL;
    byte pubKey[HPKE_Npk_MAX]; /* public key */
    word16 pubKeySz = (word16)sizeof(pubKey);

    rngRet = ret = wc_InitRng(rng);

    if (ret != 0)
        return ret;

    /* generate the keys */
    if (ret == 0)
        ret = wc_HpkeGenerateKeyPair(hpke, &ephemeralKey, rng);

    if (ret == 0)
        ret = wc_HpkeGenerateKeyPair(hpke, &receiverKey, rng);

    /* seal */
    if (ret == 0)
        ret = wc_HpkeSealBase(hpke, ephemeralKey, receiverKey,
            (byte*)info_text, (word32)XSTRLEN(info_text),
            (byte*)aad_text, (word32)XSTRLEN(aad_text),
            (byte*)start_text, (word32)XSTRLEN(start_text),
            ciphertext);

    /* export ephemeral key */
    if (ret == 0)
        ret = wc_HpkeSerializePublicKey(hpke, ephemeralKey, pubKey, &pubKeySz);

    /* open with exported ephemeral key */
    if (ret == 0)
        ret = wc_HpkeOpenBase(hpke, receiverKey, pubKey, pubKeySz,
            (byte*)info_text, (word32)XSTRLEN(info_text),
            (byte*)aad_text, (word32)XSTRLEN(aad_text),
            ciphertext, (word32)XSTRLEN(start_text),
            plaintext);

    if (ret == 0)
        ret = XMEMCMP(plaintext, start_text, XSTRLEN(start_text));

    if (ephemeralKey != NULL)
        wc_HpkeFreeKey(hpke, hpke->kem, ephemeralKey, NULL);

    if (receiverKey != NULL)
        wc_HpkeFreeKey(hpke, hpke->kem, receiverKey, NULL);

    if (rngRet == 0)
        wc_FreeRng(rng);

    return ret;
}

int main(void)
{
    int ret = 0;
    Hpke hpke[1];

#if defined(HAVE_ECC)
    /* p256 */
    ret = wc_HpkeInit(hpke, DHKEM_P256_HKDF_SHA256, HKDF_SHA256,
        HPKE_AES_128_GCM, NULL);

    if (ret != 0)
        goto fail;

    ret = test_hpke(hpke);

    if (ret != 0)
        goto fail;

    /* p384 */
    ret = wc_HpkeInit(hpke, DHKEM_P384_HKDF_SHA384, HKDF_SHA384,
        HPKE_AES_128_GCM, NULL);

    if (ret != 0)
        goto fail;

    ret = test_hpke(hpke);

    if (ret != 0)
        goto fail;

    /* p521 */
    ret = wc_HpkeInit(hpke, DHKEM_P521_HKDF_SHA512, HKDF_SHA512,
        HPKE_AES_128_GCM, NULL);

    if (ret != 0)
        goto fail;

    ret = test_hpke(hpke);

    if (ret != 0)
        goto fail;
#endif

#if defined(HAVE_CURVE25519)
    /* test with curve25519 and aes256 */
    ret = wc_HpkeInit(hpke, DHKEM_X25519_HKDF_SHA256, HKDF_SHA256,
        HPKE_AES_256_GCM, NULL);

    if (ret != 0)
        goto fail;

    ret = test_hpke(hpke);

    if (ret != 0)
        goto fail;
#endif

    printf("HPKE test success\n");

    return ret;

fail:
    printf("HPKE test error %d: %s\n", ret, wc_GetErrorString(ret));
/* x448 and chacha20 are unimplemented */
    return 1;
}
#else
int main(void)
{
    printf("Please build wolfssl with ./configure --enable-hpke --enable-aesgcm --enable-curve25519 --enable-ecc\n");
    return 0;
}
#endif
