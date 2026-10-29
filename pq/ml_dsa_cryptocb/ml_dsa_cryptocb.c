/* ml_dsa_cryptocb.c
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

/* Offload ML-DSA key generation, signing and verification through the
 * wolfCrypt crypto callback (cryptocb) interface. wolfSSL routes the signature
 * operations to the registered callback under WC_ALGO_TYPE_PK, selecting
 * ML-DSA with WC_PQC_SIG_TYPE_MLDSA.
 *
 * The callback here delegates back to the wolfCrypt software implementation so
 * the example runs without hardware. A real device replaces each case body
 * with its own calls and keeps the private key on-chip.
 *
 * Build wolfSSL with: ./configure --enable-cryptocb --enable-dilithium
 */

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_mldsa.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WOLF_CRYPTO_CB) && defined(HAVE_DILITHIUM) && \
    !defined(WOLFSSL_MLDSA_NO_MAKE_KEY) && !defined(WOLFSSL_MLDSA_NO_SIGN) && \
    !defined(WOLFSSL_MLDSA_NO_VERIFY) && !defined(WOLFSSL_MLDSA_NO_CTX)

/* Any value except INVALID_DEVID (-2). */
#define ML_DSA_CB_DEVID 1

/* FIPS 204 security category: 2 = ML-DSA-44, 3 = ML-DSA-65, 5 = ML-DSA-87. */
#define ML_DSA_CB_LEVEL 2

typedef struct MlDsaCryptoCbCtx {
    int keygenCount;
    int signCount;
    int verifyCount;
} MlDsaCryptoCbCtx;

/* Crypto callback. Handles ML-DSA keygen/sign/verify (pure, empty-context) and
 * returns CRYPTOCB_UNAVAILABLE for everything else so wolfCrypt falls back to
 * software.
 *
 * Recursion is avoided by clearing key->devId before calling the public API
 * and restoring it afterwards: the signature dispatch in wc_mldsa.c only fires
 * when key->devId != INVALID_DEVID. */
static int myMlDsaCryptoCb(int devId, wc_CryptoInfo* info, void* ctx)
{
    int ret = CRYPTOCB_UNAVAILABLE;
    MlDsaCryptoCbCtx* myCtx = (MlDsaCryptoCbCtx*)ctx;
    MlDsaKey* key = NULL;
    int savedId = INVALID_DEVID;

    (void)devId;

    if (info->algo_type != WC_ALGO_TYPE_PK)
        return CRYPTOCB_UNAVAILABLE;

    switch (info->pk.type) {
        case WC_PK_TYPE_PQC_SIG_KEYGEN:
            if (info->pk.pqc_sig_kg.type != WC_PQC_SIG_TYPE_MLDSA)
                break;
            key = (MlDsaKey*)info->pk.pqc_sig_kg.key;
            savedId = key->devId;
            key->devId = INVALID_DEVID;
            ret = wc_MlDsaKey_MakeKey(key, info->pk.pqc_sig_kg.rng);
            key->devId = savedId;
            if (ret == 0 && myCtx != NULL)
                myCtx->keygenCount++;
            break;

        case WC_PK_TYPE_PQC_SIG_SIGN:
            if (info->pk.pqc_sign.type != WC_PQC_SIG_TYPE_MLDSA)
                break;
            /* Only the pure (non pre-hash) path is handled here; a pre-hash
             * request falls back to software. */
            if ((enum wc_HashType)info->pk.pqc_sign.preHashType !=
                    WC_HASH_TYPE_NONE)
                break;
            key = (MlDsaKey*)info->pk.pqc_sign.key;
            savedId = key->devId;
            key->devId = INVALID_DEVID;
            ret = wc_MlDsaKey_SignCtx(key, info->pk.pqc_sign.context,
                info->pk.pqc_sign.contextLen, info->pk.pqc_sign.out,
                info->pk.pqc_sign.outlen, info->pk.pqc_sign.in,
                info->pk.pqc_sign.inlen, info->pk.pqc_sign.rng);
            key->devId = savedId;
            if (ret == 0 && myCtx != NULL)
                myCtx->signCount++;
            break;

        case WC_PK_TYPE_PQC_SIG_VERIFY:
            if (info->pk.pqc_verify.type != WC_PQC_SIG_TYPE_MLDSA)
                break;
            if ((enum wc_HashType)info->pk.pqc_verify.preHashType !=
                    WC_HASH_TYPE_NONE)
                break;
            key = (MlDsaKey*)info->pk.pqc_verify.key;
            savedId = key->devId;
            key->devId = INVALID_DEVID;
            /* SIG_VERIFY_E passes back unchanged; it reports a bad signature. */
            ret = wc_MlDsaKey_VerifyCtx(key, info->pk.pqc_verify.sig,
                info->pk.pqc_verify.siglen, info->pk.pqc_verify.context,
                info->pk.pqc_verify.contextLen, info->pk.pqc_verify.msg,
                info->pk.pqc_verify.msglen, info->pk.pqc_verify.res);
            key->devId = savedId;
            if (ret == 0 && myCtx != NULL)
                myCtx->verifyCount++;
            break;

        default:
            break;
    }

    return ret;
}

int main(void)
{
    MlDsaKey key;
    WC_RNG rng;
    MlDsaCryptoCbCtx myCtx = { 0, 0, 0 };
    const char* msg = "wolfSSL ML-DSA crypto callback example";
    byte* sig = NULL;
    word32 sigLen = 0;
    int sigSz = 0;
    int verifyRes = 0;
    int ret;
    int wcInit = 0;
    int rngInit = 0;
    int keyInit = 0;
    int devRegistered = 0;

    printf("ML-DSA sign/verify through the wolfCrypt crypto callback\n\n");

    /* Initializes the crypto callback device table. */
    ret = wolfCrypt_Init();
    if (ret == 0)
        wcInit = 1;
    else
        printf("wolfCrypt_Init failed: %d\n", ret);

    if (ret == 0) {
        if (wc_CryptoCb_RegisterDevice(ML_DSA_CB_DEVID, myMlDsaCryptoCb, &myCtx) == 0)
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

    /* Key initialized with the callback devId so every operation dispatches to
     * myMlDsaCryptoCb. */
    if (ret == 0) {
        ret = wc_MlDsaKey_Init(&key, NULL, ML_DSA_CB_DEVID);
        if (ret == 0)
            keyInit = 1;
    }

    if (ret == 0)
        ret = wc_MlDsaKey_SetParams(&key, ML_DSA_CB_LEVEL);

    /* Key generation (callback). */
    if (ret == 0)
        ret = wc_MlDsaKey_MakeKey(&key, &rng);

    if (ret == 0)
        ret = wc_MlDsaKey_GetSigLen(&key, &sigSz);
    if (ret == 0 && sigSz > 0) {
        sig = (byte*)malloc((size_t)sigSz);
        if (sig == NULL)
            ret = MEMORY_E;
    }

    /* Sign (callback). ctx=NULL/ctxLen=0 is FIPS 204 signing with an empty
     * context. */
    if (ret == 0) {
        sigLen = (word32)sigSz;
        ret = wc_MlDsaKey_SignCtx(&key, NULL, 0, sig, &sigLen,
            (const byte*)msg, (word32)strlen(msg), &rng);
    }

    /* Verify (callback). */
    if (ret == 0)
        ret = wc_MlDsaKey_VerifyCtx(&key, sig, sigLen, NULL, 0,
            (const byte*)msg, (word32)strlen(msg), &verifyRes);

    if (ret == 0 && verifyRes != 1) {
        printf("Signature did not verify\n");
        ret = -1;
    }

    /* Prove the operations actually went through the callback. */
    if (ret == 0 && (myCtx.keygenCount < 1 || myCtx.signCount < 1 ||
                     myCtx.verifyCount < 1)) {
        printf("Crypto callback was not exercised (keygen=%d sign=%d "
               "verify=%d)\n", myCtx.keygenCount, myCtx.signCount,
               myCtx.verifyCount);
        ret = -1;
    }

    if (ret == 0) {
        printf("keygen=%d sign=%d verify=%d via crypto callback\n",
               myCtx.keygenCount, myCtx.signCount, myCtx.verifyCount);
        printf("Signature verified\n");
    }
    else {
        printf("An error occurred: %d\n", ret);
    }

    if (sig != NULL)
        free(sig);
    if (keyInit)
        wc_MlDsaKey_Free(&key);
    if (rngInit)
        wc_FreeRng(&rng);
    if (devRegistered)
        wc_CryptoCb_UnRegisterDevice(ML_DSA_CB_DEVID);
    if (wcInit)
        wolfCrypt_Cleanup();

    /* not ret: an exit code is masked to 8 bits, so -256 would read as 0 */
    return (ret == 0) ? 0 : 1;
}

#else

int main(void)
{
    printf("Example requires wolfSSL built with "
           "--enable-cryptocb --enable-dilithium\n");
    return 0;
}

#endif /* WOLF_CRYPTO_CB && HAVE_DILITHIUM && sign/verify/ctx */
