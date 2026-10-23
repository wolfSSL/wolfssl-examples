/* ml_kem_cryptocb.c
 *
 * Copyright (C) 2025 wolfSSL Inc.
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

/* Offload ML-KEM key generation, encapsulation and decapsulation through the
 * wolfCrypt crypto callback (cryptocb) interface. This is the path a secure
 * element such as a TLS 1.3 peer would use: wolfSSL routes the KEM operations
 * to the registered callback under WC_ALGO_TYPE_PK, selecting ML-KEM with
 * WC_PQC_KEM_TYPE_MLKEM. In TLS 1.3 a client needs KEYGEN and DECAPS; a server
 * needs ENCAPS.
 *
 * The callback here delegates back to the wolfCrypt software implementation so
 * the example runs without hardware. A real device replaces each case body
 * with its own calls and keeps the private key on-chip.
 *
 * Build wolfSSL with: ./configure --enable-cryptocb --enable-mlkem
 */

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <stdio.h>

#if defined(WOLF_CRYPTO_CB) && defined(WOLFSSL_HAVE_MLKEM) && \
    !defined(WOLFSSL_MLKEM_NO_MAKE_KEY) && \
    !defined(WOLFSSL_MLKEM_NO_ENCAPSULATE) && \
    !defined(WOLFSSL_MLKEM_NO_DECAPSULATE)

/* Any value except INVALID_DEVID (-2). Passed to wc_MlKemKey_Init and matched
 * by the registered callback. */
#define ML_KEM_CB_DEVID 1

typedef struct MyCryptoCbCtx {
    int keygenCount;
    int encapCount;
    int decapCount;
} MyCryptoCbCtx;

/* Crypto callback. Handles the three ML-KEM KEM operations and returns
 * CRYPTOCB_UNAVAILABLE for everything else so wolfCrypt falls back to
 * software.
 *
 * Recursion is avoided by clearing key->devId before calling the public API
 * and restoring it afterwards: the KEM dispatch in wc_mlkem.c only fires when
 * key->devId != INVALID_DEVID, so the delegated call runs in software and the
 * key still dispatches here on the next operation. */
static int myPqcKemCb(int devId, wc_CryptoInfo* info, void* ctx)
{
    int ret = CRYPTOCB_UNAVAILABLE;
    MyCryptoCbCtx* myCtx = (MyCryptoCbCtx*)ctx;
    MlKemKey* key = NULL;
    int savedId = INVALID_DEVID;

    (void)devId;

    if (info->algo_type != WC_ALGO_TYPE_PK)
        return CRYPTOCB_UNAVAILABLE;

    switch (info->pk.type) {
        case WC_PK_TYPE_PQC_KEM_KEYGEN:
            if (info->pk.pqc_kem_kg.type != WC_PQC_KEM_TYPE_MLKEM)
                break;
            key = (MlKemKey*)info->pk.pqc_kem_kg.key;
            savedId = key->devId;
            key->devId = INVALID_DEVID;
            ret = wc_MlKemKey_MakeKey(key, info->pk.pqc_kem_kg.rng);
            key->devId = savedId;
            if (ret == 0 && myCtx != NULL)
                myCtx->keygenCount++;
            break;

        case WC_PK_TYPE_PQC_KEM_ENCAPS:
            if (info->pk.pqc_encaps.type != WC_PQC_KEM_TYPE_MLKEM)
                break;
            key = (MlKemKey*)info->pk.pqc_encaps.key;
            savedId = key->devId;
            key->devId = INVALID_DEVID;
            ret = wc_MlKemKey_Encapsulate(key, info->pk.pqc_encaps.ciphertext,
                info->pk.pqc_encaps.sharedSecret, info->pk.pqc_encaps.rng);
            key->devId = savedId;
            if (ret == 0 && myCtx != NULL)
                myCtx->encapCount++;
            break;

        case WC_PK_TYPE_PQC_KEM_DECAPS:
            if (info->pk.pqc_decaps.type != WC_PQC_KEM_TYPE_MLKEM)
                break;
            key = (MlKemKey*)info->pk.pqc_decaps.key;
            savedId = key->devId;
            key->devId = INVALID_DEVID;
            ret = wc_MlKemKey_Decapsulate(key, info->pk.pqc_decaps.sharedSecret,
                info->pk.pqc_decaps.ciphertext, info->pk.pqc_decaps.ciphertextLen);
            key->devId = savedId;
            if (ret == 0 && myCtx != NULL)
                myCtx->decapCount++;
            break;

        default:
            break;
    }

    return ret;
}

int main(void)
{
    MlKemKey aliceKey;
    MlKemKey bobKey;
    WC_RNG rng;
    MyCryptoCbCtx myCtx = { 0, 0, 0 };
    int ret;
    int wcInit = 0;
    int rngInit = 0;
    int aliceInit = 0;
    int bobInit = 0;
    int devRegistered = 0;
    int i;
    byte diff = 0;
    byte alicePub[WC_ML_KEM_512_PUBLIC_KEY_SIZE];
    byte bobCt[WC_ML_KEM_512_CIPHER_TEXT_SIZE];
    byte aliceSs[WC_ML_KEM_SS_SZ];
    byte bobSs[WC_ML_KEM_SS_SZ];

    printf("ML-KEM-512 key exchange through the wolfCrypt crypto callback\n\n");

    /* Initializes the crypto callback device table. */
    ret = wolfCrypt_Init();
    if (ret == 0)
        wcInit = 1;
    else
        printf("wolfCrypt_Init failed: %d\n", ret);

    if (ret == 0) {
        if (wc_CryptoCb_RegisterDevice(ML_KEM_CB_DEVID, myPqcKemCb, &myCtx) == 0)
            devRegistered = 1;
        else {
            printf("wc_CryptoCb_RegisterDevice failed\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        ret = wc_InitRng(&rng);
        if (ret == 0)
            rngInit = 1;
    }

    /* Keys are initialized with the callback devId so every KEM operation
     * dispatches to myPqcKemCb. */
    if (ret == 0) {
        ret = wc_MlKemKey_Init(&aliceKey, WC_ML_KEM_512, NULL, ML_KEM_CB_DEVID);
        if (ret == 0)
            aliceInit = 1;
    }

    if (ret == 0) {
        ret = wc_MlKemKey_Init(&bobKey, WC_ML_KEM_512, NULL, ML_KEM_CB_DEVID);
        if (ret == 0)
            bobInit = 1;
    }

    /* Alice: key generation (callback). */
    if (ret == 0)
        ret = wc_MlKemKey_MakeKey(&aliceKey, &rng);
    if (ret == 0)
        ret = wc_MlKemKey_EncodePublicKey(&aliceKey, alicePub,
            WC_ML_KEM_512_PUBLIC_KEY_SIZE);

    /* Bob: encapsulate to Alice's public key (callback). */
    if (ret == 0)
        ret = wc_MlKemKey_DecodePublicKey(&bobKey, alicePub,
            WC_ML_KEM_512_PUBLIC_KEY_SIZE);
    if (ret == 0)
        ret = wc_MlKemKey_Encapsulate(&bobKey, bobCt, bobSs, &rng);

    /* Alice: decapsulate Bob's ciphertext (callback). */
    if (ret == 0)
        ret = wc_MlKemKey_Decapsulate(&aliceKey, aliceSs, bobCt,
            WC_ML_KEM_512_CIPHER_TEXT_SIZE);

    /* Both sides must derive the same secret. Accumulate rather than break
     * early; the secrets are secret data. */
    if (ret == 0) {
        for (i = 0; i < WC_ML_KEM_SS_SZ; i++)
            diff |= (byte)(aliceSs[i] ^ bobSs[i]);
        if (diff != 0) {
            printf("Shared secrets do not match\n");
            ret = -1;
        }
    }

    /* Prove the operations actually went through the callback. */
    if (ret == 0 && (myCtx.keygenCount < 1 || myCtx.encapCount < 1 ||
                     myCtx.decapCount < 1)) {
        printf("Crypto callback was not exercised (keygen=%d encaps=%d "
               "decaps=%d)\n", myCtx.keygenCount, myCtx.encapCount,
               myCtx.decapCount);
        ret = -1;
    }

    if (ret == 0) {
        printf("keygen=%d encapsulate=%d decapsulate=%d via crypto callback\n",
               myCtx.keygenCount, myCtx.encapCount, myCtx.decapCount);
        printf("Shared secrets match\n");
    }
    else {
        printf("An error occurred: %d\n", ret);
    }

    wc_ForceZero(aliceSs, sizeof(aliceSs));
    wc_ForceZero(bobSs, sizeof(bobSs));

    if (aliceInit)
        wc_MlKemKey_Free(&aliceKey);
    if (bobInit)
        wc_MlKemKey_Free(&bobKey);
    if (rngInit)
        wc_FreeRng(&rng);
    if (devRegistered)
        wc_CryptoCb_UnRegisterDevice(ML_KEM_CB_DEVID);
    if (wcInit)
        wolfCrypt_Cleanup();

    /* not ret: an exit code is masked to 8 bits, so -256 would read as 0 */
    return (ret == 0) ? 0 : 1;
}

#else

int main(void)
{
    printf("Example requires wolfSSL built with "
           "--enable-cryptocb --enable-mlkem\n");
    return 0;
}

#endif /* WOLF_CRYPTO_CB && WOLFSSL_HAVE_MLKEM && make/encaps/decaps */
