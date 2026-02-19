/* driver.c
 *
 * wolfCrypt as standalone bootloader driver for UEFI
 *
 * Copyright (C) 2026 wolfSSL Inc.
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

#include <efi.h>
#include <efilib.h>

#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/memory.h>
#include <wolfssl/wolfcrypt/misc.h>
#include <wolfssl/version.h>
#include <wolfssl/wolfcrypt/pwdbased.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/sha3.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/dh.h>
#include <wolfssl/wolfcrypt/chacha.h>
#include <wolfssl/wolfcrypt/poly1305.h>
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
#include <wolfssl/wolfcrypt/curve25519.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/cmac.h>
#include <wolfssl/wolfcrypt/kdf.h>
#ifdef HAVE_DILITHIUM
#include <wolfssl/wolfcrypt/dilithium.h>
#endif
#ifdef HAVE_FALCON
#include <wolfssl/wolfcrypt/falcon.h>
#endif
#ifdef UEFI_HW_ACCEL
#include <wolfssl/wolfcrypt/cpuid.h>
#endif

#include "wolfcrypt_api.h"

#ifdef GNUEFI
#define EFI_API_TAG EFIAPI
#else
#define EFI_API_TAG __attribute__((ms_abi))
#endif


#define WRAP_FUNC(ret_type, name, args, call_args) \
    static ret_type EFI_API_TAG name##_EfiAPI args { \
        return name call_args; \
    }

#define WRAP_VOID(name, args, call_args) \
    static void EFI_API_TAG name##_EfiAPI args { \
        name call_args; \
    }

#define WRAP_FUNC_ALIAS(ret_type, alias, target, args, call_args) \
    static ret_type EFI_API_TAG alias##_EfiAPI args { \
        return target call_args; \
    }

#define WRAP_VOID_ALIAS(alias, target, args, call_args) \
    static void EFI_API_TAG alias##_EfiAPI args { \
        target call_args; \
    }

#define WC_UNUSED(x) ((void)(x))

WOLFSSL_API int wc_RsaSetRNG(RsaKey* key, WC_RNG* rng);

static EFI_HANDLE g_wolfcrypt_handle = NULL;
EFI_GUID g_wolfcrypt_protocol_guid = WOLFCRYPT_PROTOCOL_GUID;

/* ------------------------------------------------------------------ */
/* AES wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_AesInit, (Aes* aes, void* heap, int devId), (aes, heap, devId))
WRAP_FUNC(int, wc_AesSetKey, (Aes* aes, const byte* key, word32 len, const byte* iv, int dir),
          (aes, key, len, iv, dir))
WRAP_FUNC(int, wc_AesEcbEncrypt, (Aes* aes, byte* out, const byte* in, word32 sz),
          (aes, out, in, sz))
WRAP_FUNC(int, wc_AesEcbDecrypt, (Aes* aes, byte* out, const byte* in, word32 sz),
          (aes, out, in, sz))
WRAP_FUNC(int, wc_AesCbcEncrypt, (Aes* aes, byte* out, const byte* in, word32 sz),
          (aes, out, in, sz))
WRAP_FUNC(int, wc_AesCbcDecrypt, (Aes* aes, byte* out, const byte* in, word32 sz),
          (aes, out, in, sz))
WRAP_FUNC(int, wc_AesCfbEncrypt, (Aes* aes, byte* out, const byte* in, word32 sz),
          (aes, out, in, sz))
WRAP_FUNC(int, wc_AesCfbDecrypt, (Aes* aes, byte* out, const byte* in, word32 sz),
          (aes, out, in, sz))
WRAP_FUNC(int, wc_AesSetIV, (Aes* aes, const byte* iv), (aes, iv))
WRAP_VOID(wc_AesFree, (Aes* aes), (aes))
WRAP_FUNC(int, wc_AesGcmSetKey, (Aes* aes, const byte* key, word32 len), (aes, key, len))
WRAP_FUNC(int, wc_AesGcmEncrypt, (Aes* aes, byte* out, const byte* in, word32 sz,
          const byte* iv, word32 ivSz, byte* authTag, word32 authTagSz,
          const byte* authIn, word32 authInSz),
          (aes, out, in, sz, iv, ivSz, authTag, authTagSz, authIn, authInSz))
WRAP_FUNC(int, wc_AesGcmDecrypt, (Aes* aes, byte* out, const byte* in, word32 sz,
          const byte* iv, word32 ivSz, const byte* authTag, word32 authTagSz,
          const byte* authIn, word32 authInSz),
          (aes, out, in, sz, iv, ivSz, authTag, authTagSz, authIn, authInSz))
WRAP_FUNC(int, wc_AesCcmSetKey, (Aes* aes, const byte* key, word32 keySz), (aes, key, keySz))
WRAP_FUNC(int, wc_AesCcmEncrypt, (Aes* aes, byte* out, const byte* in, word32 inSz,
          const byte* nonce, word32 nonceSz, byte* authTag, word32 authTagSz,
          const byte* authIn, word32 authInSz),
          (aes, out, in, inSz, nonce, nonceSz, authTag, authTagSz, authIn, authInSz))
WRAP_FUNC(int, wc_AesCcmDecrypt, (Aes* aes, byte* out, const byte* in, word32 inSz,
          const byte* nonce, word32 nonceSz, const byte* authTag, word32 authTagSz,
          const byte* authIn, word32 authInSz),
          (aes, out, in, inSz, nonce, nonceSz, authTag, authTagSz, authIn, authInSz))
WRAP_FUNC(int, wc_AesCtrEncrypt, (Aes* aes, byte* out, const byte* in, word32 sz),
          (aes, out, in, sz))
WRAP_FUNC(int, wc_AesOfbEncrypt, (Aes* aes, byte* out, const byte* in, word32 sz),
          (aes, out, in, sz))

/* ------------------------------------------------------------------ */
/* RNG wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_InitRng, (WC_RNG* rng), (rng))
WRAP_FUNC(int, wc_FreeRng, (WC_RNG* rng), (rng))
WRAP_FUNC(int, wc_RNG_GenerateBlock, (WC_RNG* rng, byte* output, word32 sz), (rng, output, sz))
WRAP_FUNC(int, wc_RNG_GenerateByte, (WC_RNG* rng, byte* b), (rng, b))
WRAP_FUNC(int, wc_SetSeed_Cb, (wc_RngSeed_Cb cb), (cb))
WRAP_FUNC(int, wc_RNG_TestSeed, (const byte* seed, word32 seedSz), (seed, seedSz))
WRAP_FUNC(int, wc_GenerateSeed, (OS_Seed* os, byte* output, word32 sz), (os, output, sz))

static int EFIAPI wc_GenerateSeed_IntelRD_EfiAPI(OS_Seed* os, byte* output, word32 sz)
{
    return wc_GenerateSeed(os, output, sz);
}

static int EFIAPI wc_GenerateRand_IntelRD_EfiAPI(OS_Seed* os, byte* output, word32 sz)
{
    return wc_GenerateSeed(os, output, sz);
}

static void EFIAPI wc_InitRng_IntelRD_EfiAPI(void)
{
    /* All entropy sources are assumed ready in this configuration. */
}

/* ------------------------------------------------------------------ */
/* RSA wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_InitRsaKey, (RsaKey* key, void* heap), (key, heap))
WRAP_FUNC(int, wc_MakeRsaKey, (RsaKey* key, int size, long e, WC_RNG* rng), (key, size, e, rng))
WRAP_FUNC(int, wc_RsaKeyToDer, (RsaKey* key, byte* output, word32 inLen), (key, output, inLen))
WRAP_FUNC(int, wc_FreeRsaKey, (RsaKey* key), (key))
WRAP_FUNC(int, wc_RsaEncryptSize, (const RsaKey* key), (key))
WRAP_FUNC(int, wc_RsaPrivateKeyDecode, (const byte* input, word32* inOutIdx, RsaKey* key, word32 inSz),
          (input, inOutIdx, key, inSz))
WRAP_FUNC(int, wc_RsaPublicKeyDecode, (const byte* input, word32* inOutIdx, RsaKey* key, word32 inSz),
          (input, inOutIdx, key, inSz))
WRAP_FUNC(int, wc_RsaPrivateDecrypt_ex,
          (const byte* in, word32 inLen, byte* out, word32 outLen, RsaKey* key, int type,
           enum wc_HashType hash, int mgf, byte* label, word32 labelSz),
          (in, inLen, out, outLen, key, type, hash, mgf, label, labelSz))
WRAP_FUNC(int, wc_RsaPrivateDecrypt, (const byte* in, word32 inLen, byte* out, word32 outLen, RsaKey* key),
          (in, inLen, out, outLen, key))
WRAP_FUNC(int, wc_RsaPublicEncrypt, (const byte* in, word32 inLen, byte* out, word32 outLen, RsaKey* key, WC_RNG* rng),
          (in, inLen, out, outLen, key, rng))
WRAP_FUNC(int, wc_RsaPublicEncrypt_ex,
          (const byte* in, word32 inLen, byte* out, word32 outLen, RsaKey* key, WC_RNG* rng,
           int type, enum wc_HashType hash, int mgf, byte* label, word32 labelSz),
          (in, inLen, out, outLen, key, rng, type, hash, mgf, label, labelSz))
WRAP_FUNC(int, wc_RsaExportKey,
          (RsaKey* key, byte* e, word32* eSz, byte* n, word32* nSz,
           byte* d, word32* dSz, byte* p, word32* pSz, byte* q, word32* qSz),
          (key, e, eSz, n, nSz, d, dSz, p, pSz, q, qSz))
WRAP_FUNC(int, wc_CheckRsaKey, (RsaKey* key), (key))
WRAP_FUNC(int, wc_RsaPublicKeyDerSize, (RsaKey* key, int withHeader), (key, withHeader))
WRAP_FUNC(int, wc_RsaKeyToPublicDer, (RsaKey* key, byte* output, word32 inLen), (key, output, inLen))
WRAP_FUNC(int, wc_RsaSetRNG, (RsaKey* key, WC_RNG* rng), (key, rng))

static void EFIAPI wc_RsaCleanup_EfiAPI(RsaKey* key)
{
    if (key == NULL) {
        return;
    }

    if (key->data != NULL && key->dataLen > 0 &&
        (key->type == RSA_PRIVATE_DECRYPT || key->type == RSA_PRIVATE_ENCRYPT)) {
        ForceZero(key->data, key->dataLen);
    }

    if (key->dataIsAlloc) {
        XFREE(key->data, key->heap, DYNAMIC_TYPE_WOLF_BIGINT);
        key->dataIsAlloc = 0;
    }

    key->data = NULL;
    key->dataLen = 0;
}

/* ------------------------------------------------------------------ */
/* HMAC wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_HmacSetKey, (Hmac* hmac, int type, const byte* key, word32 length), (hmac, type, key, length))
WRAP_FUNC(int, wc_HmacUpdate, (Hmac* hmac, const byte* msg, word32 length), (hmac, msg, length))
WRAP_FUNC(int, wc_HmacFinal, (Hmac* hmac, byte* hash), (hmac, hash))

/* ------------------------------------------------------------------ */
/* SHA-1 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_InitSha, (wc_Sha* sha), (sha))
WRAP_FUNC(int, wc_ShaUpdate, (wc_Sha* sha, const byte* data, word32 len), (sha, data, len))
WRAP_FUNC(int, wc_ShaFinal, (wc_Sha* sha, byte* hash), (sha, hash))
WRAP_VOID(wc_ShaFree, (wc_Sha* sha), (sha))

/* ------------------------------------------------------------------ */
/* SHA-224 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_InitSha224, (wc_Sha224* sha), (sha))
WRAP_FUNC(int, wc_Sha224Update, (wc_Sha224* sha, const byte* data, word32 len), (sha, data, len))
WRAP_FUNC(int, wc_Sha224Final, (wc_Sha224* sha, byte* hash), (sha, hash))
WRAP_VOID(wc_Sha224Free, (wc_Sha224* sha), (sha))

/* ------------------------------------------------------------------ */
/* SHA-256 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_InitSha256, (wc_Sha256* sha), (sha))
WRAP_FUNC(int, wc_InitSha256_ex, (wc_Sha256* sha, void* heap, int devId), (sha, heap, devId))
WRAP_FUNC(int, wc_Sha256Update, (wc_Sha256* sha, const byte* data, word32 len), (sha, data, len))
WRAP_FUNC(int, wc_Sha256Final, (wc_Sha256* sha256, byte* hash), (sha256, hash))
WRAP_VOID(wc_Sha256Free, (wc_Sha256* sha), (sha))

/* ------------------------------------------------------------------ */
/* SHA-384 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_InitSha384, (wc_Sha384* sha), (sha))
WRAP_FUNC(int, wc_Sha384Update, (wc_Sha384* sha, const byte* data, word32 len), (sha, data, len))
WRAP_FUNC(int, wc_Sha384Final, (wc_Sha384* sha, byte* hash), (sha, hash))
WRAP_VOID(wc_Sha384Free, (wc_Sha384* sha), (sha))

/* ------------------------------------------------------------------ */
/* SHA-512 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_InitSha512, (wc_Sha512* sha), (sha))
WRAP_FUNC(int, wc_Sha512Update, (wc_Sha512* sha, const byte* data, word32 len), (sha, data, len))
WRAP_FUNC(int, wc_Sha512Final, (wc_Sha512* sha, byte* hash), (sha, hash))
WRAP_VOID(wc_Sha512Free, (wc_Sha512* sha), (sha))

/* ------------------------------------------------------------------ */
/* SHA-3 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_InitSha3_256, (wc_Sha3* sha, void* heap, int devId), (sha, heap, devId))
WRAP_FUNC(int, wc_Sha3_256_Update, (wc_Sha3* sha, const byte* data, word32 len), (sha, data, len))
WRAP_FUNC(int, wc_Sha3_256_Final, (wc_Sha3* sha, byte* hash), (sha, hash))
WRAP_VOID(wc_Sha3_256_Free, (wc_Sha3* sha), (sha))

WRAP_FUNC(int, wc_InitSha3_384, (wc_Sha3* sha, void* heap, int devId), (sha, heap, devId))
WRAP_FUNC(int, wc_Sha3_384_Update, (wc_Sha3* sha, const byte* data, word32 len), (sha, data, len))
WRAP_FUNC(int, wc_Sha3_384_Final, (wc_Sha3* sha, byte* hash), (sha, hash))
WRAP_VOID(wc_Sha3_384_Free, (wc_Sha3* sha), (sha))

WRAP_FUNC(int, wc_InitSha3_512, (wc_Sha3* sha, void* heap, int devId), (sha, heap, devId))
WRAP_FUNC(int, wc_Sha3_512_Update, (wc_Sha3* sha, const byte* data, word32 len), (sha, data, len))
WRAP_FUNC(int, wc_Sha3_512_Final, (wc_Sha3* sha, byte* hash), (sha, hash))
WRAP_VOID(wc_Sha3_512_Free, (wc_Sha3* sha), (sha))

/* ------------------------------------------------------------------ */
/* SHAKE128/256 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_InitShake128, (wc_Shake* shake, void* heap, int devId), (shake, heap, devId))
WRAP_FUNC(int, wc_Shake128_Update, (wc_Shake* shake, const byte* data, word32 len), (shake, data, len))
WRAP_FUNC(int, wc_Shake128_Final, (wc_Shake* shake, byte* hash, word32 hashLen), (shake, hash, hashLen))
WRAP_FUNC(int, wc_Shake128_SqueezeBlocks, (wc_Shake* shake, byte* out, word32 blockCnt), (shake, out, blockCnt))
WRAP_VOID(wc_Shake128_Free, (wc_Shake* shake), (shake))

WRAP_FUNC(int, wc_InitShake256, (wc_Shake* shake, void* heap, int devId), (shake, heap, devId))
WRAP_FUNC(int, wc_Shake256_Update, (wc_Shake* shake, const byte* data, word32 len), (shake, data, len))
WRAP_FUNC(int, wc_Shake256_Final, (wc_Shake* shake, byte* hash, word32 hashLen), (shake, hash, hashLen))
WRAP_FUNC(int, wc_Shake256_SqueezeBlocks, (wc_Shake* shake, byte* out, word32 blockCnt), (shake, out, blockCnt))
WRAP_VOID(wc_Shake256_Free, (wc_Shake* shake), (shake))

/* ------------------------------------------------------------------ */
/* ECC wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_ecc_init, (ecc_key* key), (key))
WRAP_VOID(wc_ecc_free, (ecc_key* key), (key))
WRAP_FUNC(int, wc_ecc_make_key, (WC_RNG* rng, int keysize, ecc_key* key), (rng, keysize, key))
WRAP_FUNC(int, wc_ecc_set_rng, (ecc_key* key, WC_RNG* rng), (key, rng))
WRAP_FUNC(int, wc_ecc_shared_secret, (ecc_key* priv, ecc_key* pub, byte* out, word32* outlen),
          (priv, pub, out, outlen))
WRAP_FUNC(int, wc_ecc_sign_hash, (const byte* in, word32 inlen, byte* out, word32* outlen,
          WC_RNG* rng, ecc_key* key), (in, inlen, out, outlen, rng, key))
WRAP_FUNC(int, wc_ecc_verify_hash, (const byte* sig, word32 siglen, const byte* hash,
          word32 hashlen, int* stat, ecc_key* key), (sig, siglen, hash, hashlen, stat, key))
WRAP_FUNC(int, wc_ecc_export_x963, (ecc_key* key, byte* out, word32* outLen), (key, out, outLen))
WRAP_FUNC(int, wc_ecc_import_x963, (const byte* in, word32 inLen, ecc_key* key), (in, inLen, key))
WRAP_FUNC(int, wc_EccKeyToDer, (ecc_key* key, byte* output, word32 inLen), (key, output, inLen))
WRAP_FUNC(int, wc_EccPublicKeyDecode, (const byte* input, word32* inOutIdx, ecc_key* key, word32 inSz),
          (input, inOutIdx, key, inSz))
WRAP_FUNC(int, wc_EccPrivateKeyDecode, (const byte* input, word32* inOutIdx, ecc_key* key, word32 inSz),
          (input, inOutIdx, key, inSz))

/* ------------------------------------------------------------------ */
/* DH wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_DhGenerateParams, (WC_RNG* rng, int modSz, DhKey* key), (rng, modSz, key))
WRAP_FUNC(int, wc_DhGenerateKeyPair, (DhKey* key, WC_RNG* rng, byte* priv, word32* privSz,
          byte* pub, word32* pubSz), (key, rng, priv, privSz, pub, pubSz))
WRAP_FUNC(int, wc_DhAgree, (DhKey* key, byte* agree, word32* agreeSz,
          const byte* priv, word32 privSz, const byte* otherPub, word32 pubSz),
          (key, agree, agreeSz, priv, privSz, otherPub, pubSz))

/* ------------------------------------------------------------------ */
/* ChaCha20 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_Chacha_SetKey, (ChaCha* ctx, const byte* key, word32 keySz), (ctx, key, keySz))
WRAP_FUNC(int, wc_Chacha_Process, (ChaCha* ctx, byte* output, const byte* input, word32 msglen),
          (ctx, output, input, msglen))

/* ------------------------------------------------------------------ */
/* Poly1305 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_Poly1305SetKey, (Poly1305* ctx, const byte* key, word32 keySz), (ctx, key, keySz))
WRAP_FUNC(int, wc_Poly1305Update, (Poly1305* ctx, const byte* m, word32 bytes), (ctx, m, bytes))
WRAP_FUNC(int, wc_Poly1305Final, (Poly1305* ctx, byte* tag), (ctx, tag))

/* ------------------------------------------------------------------ */
/* ChaCha20-Poly1305 AEAD wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_ChaCha20Poly1305_Encrypt,
          (const byte* inKey, const byte* inIV,
           const byte* inAAD, word32 inAADLen,
           const byte* inPlaintext, word32 inPlaintextLen,
           byte* outCiphertext, byte* outAuthTag),
          (inKey, inIV, inAAD, inAADLen, inPlaintext, inPlaintextLen,
           outCiphertext, outAuthTag))
WRAP_FUNC(int, wc_ChaCha20Poly1305_Decrypt,
          (const byte* inKey, const byte* inIV,
           const byte* inAAD, word32 inAADLen,
           const byte* inCiphertext, word32 inCiphertextLen,
           const byte* inAuthTag, byte* outPlaintext),
          (inKey, inIV, inAAD, inAADLen, inCiphertext, inCiphertextLen,
           inAuthTag, outPlaintext))

/* ------------------------------------------------------------------ */
/* Curve25519 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_curve25519_init, (curve25519_key* key), (key))
WRAP_VOID(wc_curve25519_free, (curve25519_key* key), (key))
WRAP_FUNC(int, wc_curve25519_make_key, (WC_RNG* rng, int keysize, curve25519_key* key),
          (rng, keysize, key))
WRAP_FUNC(int, wc_curve25519_shared_secret, (curve25519_key* priv, curve25519_key* pub,
          byte* out, word32* outlen), (priv, pub, out, outlen))
WRAP_FUNC(int, wc_curve25519_export_key_raw, (curve25519_key* key, byte* priv, word32* privSz,
          byte* pub, word32* pubSz), (key, priv, privSz, pub, pubSz))
WRAP_FUNC(int, wc_curve25519_import_public, (const byte* in, word32 inLen, curve25519_key* key),
          (in, inLen, key))

/* ------------------------------------------------------------------ */
/* Ed25519 wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_ed25519_init, (ed25519_key* key), (key))
WRAP_VOID(wc_ed25519_free, (ed25519_key* key), (key))
WRAP_FUNC(int, wc_ed25519_make_key, (WC_RNG* rng, int keysize, ed25519_key* key),
          (rng, keysize, key))
WRAP_FUNC(int, wc_ed25519_sign_msg, (const byte* in, word32 inlen, byte* out, word32* outlen,
          ed25519_key* key), (in, inlen, out, outlen, key))
WRAP_FUNC(int, wc_ed25519_verify_msg, (const byte* sig, word32 siglen, const byte* msg,
          word32 msglen, int* stat, ed25519_key* key), (sig, siglen, msg, msglen, stat, key))
WRAP_FUNC(int, wc_ed25519_export_key, (ed25519_key* key, byte* priv, word32* privSz,
          byte* pub, word32* pubSz), (key, priv, privSz, pub, pubSz))
WRAP_FUNC(int, wc_ed25519_import_public, (const byte* in, word32 inLen, ed25519_key* key),
          (in, inLen, key))

/* ------------------------------------------------------------------ */
/* ML-KEM wrappers */
/* ------------------------------------------------------------------ */
#ifdef WOLFSSL_HAVE_MLKEM
WRAP_FUNC(int, wc_MlKemKey_Init, (MlKemKey* key, int type, void* heap, int devId), (key, type, heap, devId))
WRAP_FUNC(int, wc_MlKemKey_Free, (MlKemKey* key), (key))
WRAP_FUNC(int, wc_MlKemKey_MakeKey, (MlKemKey* key, WC_RNG* rng), (key, rng))
WRAP_FUNC(int, wc_MlKemKey_CipherTextSize, (MlKemKey* key, word32* len), (key, len))
WRAP_FUNC(int, wc_MlKemKey_SharedSecretSize, (MlKemKey* key, word32* len), (key, len))
WRAP_FUNC(int, wc_MlKemKey_Encapsulate,
          (MlKemKey* key, unsigned char* ct, unsigned char* ss, WC_RNG* rng),
          (key, ct, ss, rng))
WRAP_FUNC(int, wc_MlKemKey_Decapsulate,
          (MlKemKey* key, unsigned char* ss, const unsigned char* ct, word32 len),
          (key, ss, ct, len))
WRAP_FUNC(int, wc_MlKemKey_DecodePrivateKey,
          (MlKemKey* key, const unsigned char* in, word32 len), (key, in, len))
WRAP_FUNC(int, wc_MlKemKey_DecodePublicKey,
          (MlKemKey* key, const unsigned char* in, word32 len), (key, in, len))
WRAP_FUNC(int, wc_MlKemKey_PrivateKeySize, (MlKemKey* key, word32* len), (key, len))
WRAP_FUNC(int, wc_MlKemKey_PublicKeySize, (MlKemKey* key, word32* len), (key, len))
WRAP_FUNC(int, wc_MlKemKey_EncodePrivateKey,
          (MlKemKey* key, unsigned char* out, word32 len), (key, out, len))
WRAP_FUNC(int, wc_MlKemKey_EncodePublicKey,
          (MlKemKey* key, unsigned char* out, word32 len), (key, out, len))
#endif /* WOLFSSL_HAVE_MLKEM */

/* ------------------------------------------------------------------ */
/* Dilithium wrappers */
/* ------------------------------------------------------------------ */
#ifdef HAVE_DILITHIUM
WRAP_FUNC(int, wc_dilithium_init, (dilithium_key* key), (key))
WRAP_VOID(wc_dilithium_free, (dilithium_key* key), (key))
WRAP_FUNC(int, wc_dilithium_set_level, (dilithium_key* key, byte level), (key, level))
WRAP_FUNC(int, wc_dilithium_make_key, (dilithium_key* key, WC_RNG* rng), (key, rng))
WRAP_FUNC(int, wc_dilithium_sign_msg, (const byte* in, word32 inLen, byte* out, word32* outLen,
          dilithium_key* key, WC_RNG* rng), (in, inLen, out, outLen, key, rng))
WRAP_FUNC(int, wc_dilithium_verify_msg, (const byte* sig, word32 sigLen, const byte* msg,
          word32 msgLen, int* res, dilithium_key* key), (sig, sigLen, msg, msgLen, res, key))
WRAP_FUNC(int, wc_dilithium_export_key, (dilithium_key* key, byte* priv, word32* privSz,
          byte* pub, word32* pubSz), (key, priv, privSz, pub, pubSz))
WRAP_FUNC(int, wc_dilithium_import_key, (const byte* priv, word32 privSz,
          const byte* pub, word32 pubSz, dilithium_key* key),
          (priv, privSz, pub, pubSz, key))
#endif /* HAVE_DILITHIUM */

/* ------------------------------------------------------------------ */
/* Falcon wrappers */
/* ------------------------------------------------------------------ */
#ifdef HAVE_FALCON
WRAP_FUNC(int, wc_falcon_init, (falcon_key* key), (key))
WRAP_VOID(wc_falcon_free, (falcon_key* key), (key))
WRAP_FUNC(int, wc_falcon_make_key, (falcon_key* key, WC_RNG* rng), (key, rng))
WRAP_FUNC(int, wc_falcon_sign_msg, (const byte* in, word32 inLen, byte* out, word32* outLen,
          falcon_key* key, WC_RNG* rng), (in, inLen, out, outLen, key, rng))
WRAP_FUNC(int, wc_falcon_verify_msg, (const byte* sig, word32 sigLen, const byte* msg,
          word32 msgLen, int* res, falcon_key* key), (sig, sigLen, msg, msgLen, res, key))
WRAP_FUNC(int, wc_falcon_export_key, (falcon_key* key, byte* priv, word32* privSz,
          byte* pub, word32* pubSz), (key, priv, privSz, pub, pubSz))
WRAP_FUNC(int, wc_falcon_import_key, (const byte* priv, word32 privSz,
          const byte* pub, word32 pubSz, falcon_key* key),
          (priv, privSz, pub, pubSz, key))
#endif /* HAVE_FALCON */

/* ------------------------------------------------------------------ */
/* CMAC wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_InitCmac, (Cmac* cmac, const byte* key, word32 keySz, int type, void* unused),
          (cmac, key, keySz, type, unused))
WRAP_FUNC(int, wc_CmacUpdate, (Cmac* cmac, const byte* in, word32 inSz), (cmac, in, inSz))
WRAP_FUNC(int, wc_CmacFinal, (Cmac* cmac, byte* out, word32* outSz), (cmac, out, outSz))

/* ------------------------------------------------------------------ */
/* KDF / PKCS wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wc_PBKDF2,
          (byte* output, const byte* passwd, int pLen, const byte* salt, int sLen,
           int iterations, int kLen, int hashType),
          (output, passwd, pLen, salt, sLen, iterations, kLen, hashType))
WRAP_FUNC(int, wc_PKCS12_PBKDF,
          (byte* output, const byte* passwd, int passLen, const byte* salt, int saltLen,
           int iterations, int kLen, int hashType, int id),
          (output, passwd, passLen, salt, saltLen, iterations, kLen, hashType, id))
WRAP_FUNC(int, wc_PKCS7_PadData, (byte* in, word32 inSz, byte* out, word32 outSz, word32 blockSz),
          (in, inSz, out, outSz, blockSz))
WRAP_FUNC(int, wc_HKDF, (int type, const byte* inKey, word32 inKeySz,
          const byte* salt, word32 saltSz, const byte* info, word32 infoSz,
          byte* out, word32 outSz),
          (type, inKey, inKeySz, salt, saltSz, info, infoSz, out, outSz))

/* ------------------------------------------------------------------ */
/* Logging / Error wrappers */
/* ------------------------------------------------------------------ */
WRAP_FUNC(int, wolfSSL_Debugging_ON, (void), ())
WRAP_FUNC(int, wolfSSL_SetLoggingCb, (wolfSSL_Logging_cb cb), (cb))
WRAP_FUNC(wolfSSL_Logging_cb, wolfSSL_GetLoggingCb, (void), ())
WRAP_VOID(wolfSSL_Debugging_OFF, (void), ())

WRAP_FUNC(const char*, wc_GetErrorString, (int error), (error))
WRAP_VOID(wc_ErrorString, (int err, char* buff), (err, buff))

/* ------------------------------------------------------------------ */
/* Protocol instance */
/* ------------------------------------------------------------------ */
static WOLFCRYPT_PROTOCOL g_wolfcrypt_api = {
    .Version = LIBWOLFSSL_VERSION_HEX,

    /* AES */
    .wc_AesInit = wc_AesInit_EfiAPI,
    .wc_AesSetKey = wc_AesSetKey_EfiAPI,
    .wc_AesEcbEncrypt = wc_AesEcbEncrypt_EfiAPI,
    .wc_AesEcbDecrypt = wc_AesEcbDecrypt_EfiAPI,
    .wc_AesCbcEncrypt = wc_AesCbcEncrypt_EfiAPI,
    .wc_AesCbcDecrypt = wc_AesCbcDecrypt_EfiAPI,
    .wc_AesCfbEncrypt = wc_AesCfbEncrypt_EfiAPI,
    .wc_AesCfbDecrypt = wc_AesCfbDecrypt_EfiAPI,
    .wc_AesSetIV = wc_AesSetIV_EfiAPI,
    .wc_AesFree = wc_AesFree_EfiAPI,
    .wc_AesGcmSetKey = wc_AesGcmSetKey_EfiAPI,
    .wc_AesGcmEncrypt = wc_AesGcmEncrypt_EfiAPI,
    .wc_AesGcmDecrypt = wc_AesGcmDecrypt_EfiAPI,
    .wc_AesCcmSetKey = wc_AesCcmSetKey_EfiAPI,
    .wc_AesCcmEncrypt = wc_AesCcmEncrypt_EfiAPI,
    .wc_AesCcmDecrypt = wc_AesCcmDecrypt_EfiAPI,
    .wc_AesCtrEncrypt = wc_AesCtrEncrypt_EfiAPI,
    .wc_AesOfbEncrypt = wc_AesOfbEncrypt_EfiAPI,

    /* RNG */
    .wc_InitRng = wc_InitRng_EfiAPI,
    .wc_FreeRng = wc_FreeRng_EfiAPI,
    .wc_RNG_GenerateBlock = wc_RNG_GenerateBlock_EfiAPI,
    .wc_RNG_GenerateByte = wc_RNG_GenerateByte_EfiAPI,
    .wc_SetSeed_Cb = wc_SetSeed_Cb_EfiAPI,
    .wc_RNG_TestSeed = wc_RNG_TestSeed_EfiAPI,
    .wc_GenerateSeed_IntelRD = wc_GenerateSeed_IntelRD_EfiAPI,
    .wc_GenerateSeed = wc_GenerateSeed_EfiAPI,
    .wc_GenerateRand_IntelRD = wc_GenerateRand_IntelRD_EfiAPI,
    .wc_InitRng_IntelRD = wc_InitRng_IntelRD_EfiAPI,

    /* RSA */
    .wc_InitRsaKey = wc_InitRsaKey_EfiAPI,
    .wc_MakeRsaKey = wc_MakeRsaKey_EfiAPI,
    .wc_RsaKeyToDer = wc_RsaKeyToDer_EfiAPI,
    .wc_FreeRsaKey = wc_FreeRsaKey_EfiAPI,
    .wc_RsaEncryptSize = wc_RsaEncryptSize_EfiAPI,
    .wc_RsaPrivateKeyDecode = wc_RsaPrivateKeyDecode_EfiAPI,
    .wc_RsaPublicKeyDecode = wc_RsaPublicKeyDecode_EfiAPI,
    .wc_RsaPrivateDecryptEx = wc_RsaPrivateDecrypt_ex_EfiAPI,
    .wc_RsaPrivateDecrypt = wc_RsaPrivateDecrypt_EfiAPI,
    .wc_RsaPublicEncrypt = wc_RsaPublicEncrypt_EfiAPI,
    .wc_RsaPublicEncryptEx = wc_RsaPublicEncrypt_ex_EfiAPI,
    .wc_RsaExportKey = wc_RsaExportKey_EfiAPI,
    .wc_RsaCleanup = wc_RsaCleanup_EfiAPI,
    .wc_CheckRsaKey = wc_CheckRsaKey_EfiAPI,
    .wc_RsaPublicKeyDerSize = wc_RsaPublicKeyDerSize_EfiAPI,
    .wc_RsaKeyToPublicDer = wc_RsaKeyToPublicDer_EfiAPI,
    .wc_RsaSetRNG = wc_RsaSetRNG_EfiAPI,

    /* HMAC */
    .wc_HmacSetKey = wc_HmacSetKey_EfiAPI,
    .wc_HmacUpdate = wc_HmacUpdate_EfiAPI,
    .wc_HmacFinal = wc_HmacFinal_EfiAPI,

    /* SHA-1 */
    .wc_InitSha = wc_InitSha_EfiAPI,
    .wc_ShaUpdate = wc_ShaUpdate_EfiAPI,
    .wc_ShaFinal = wc_ShaFinal_EfiAPI,
    .wc_ShaFree = wc_ShaFree_EfiAPI,

    /* SHA-224 */
    .wc_InitSha224 = wc_InitSha224_EfiAPI,
    .wc_Sha224Update = wc_Sha224Update_EfiAPI,
    .wc_Sha224Final = wc_Sha224Final_EfiAPI,
    .wc_Sha224Free = wc_Sha224Free_EfiAPI,

    /* SHA-256 */
    .wc_InitSha256 = wc_InitSha256_EfiAPI,
    .wc_InitSha256_ex = wc_InitSha256_ex_EfiAPI,
    .wc_Sha256Update = wc_Sha256Update_EfiAPI,
    .wc_Sha256Final = wc_Sha256Final_EfiAPI,
    .wc_Sha256Free = wc_Sha256Free_EfiAPI,

    /* SHA-384 */
    .wc_InitSha384 = wc_InitSha384_EfiAPI,
    .wc_Sha384Update = wc_Sha384Update_EfiAPI,
    .wc_Sha384Final = wc_Sha384Final_EfiAPI,
    .wc_Sha384Free = wc_Sha384Free_EfiAPI,

    /* SHA-512 */
    .wc_InitSha512 = wc_InitSha512_EfiAPI,
    .wc_Sha512Update = wc_Sha512Update_EfiAPI,
    .wc_Sha512Final = wc_Sha512Final_EfiAPI,
    .wc_Sha512Free = wc_Sha512Free_EfiAPI,

    /* SHA-3 */
    .wc_InitSha3_256 = wc_InitSha3_256_EfiAPI,
    .wc_Sha3_256_Update = wc_Sha3_256_Update_EfiAPI,
    .wc_Sha3_256_Final = wc_Sha3_256_Final_EfiAPI,
    .wc_Sha3_256_Free = wc_Sha3_256_Free_EfiAPI,
    .wc_InitSha3_384 = wc_InitSha3_384_EfiAPI,
    .wc_Sha3_384_Update = wc_Sha3_384_Update_EfiAPI,
    .wc_Sha3_384_Final = wc_Sha3_384_Final_EfiAPI,
    .wc_Sha3_384_Free = wc_Sha3_384_Free_EfiAPI,
    .wc_InitSha3_512 = wc_InitSha3_512_EfiAPI,
    .wc_Sha3_512_Update = wc_Sha3_512_Update_EfiAPI,
    .wc_Sha3_512_Final = wc_Sha3_512_Final_EfiAPI,
    .wc_Sha3_512_Free = wc_Sha3_512_Free_EfiAPI,

    /* SHAKE */
    .wc_InitShake128 = wc_InitShake128_EfiAPI,
    .wc_Shake128_Update = wc_Shake128_Update_EfiAPI,
    .wc_Shake128_Final = wc_Shake128_Final_EfiAPI,
    .wc_Shake128_SqueezeBlocks = wc_Shake128_SqueezeBlocks_EfiAPI,
    .wc_Shake128_Free = wc_Shake128_Free_EfiAPI,
    .wc_InitShake256 = wc_InitShake256_EfiAPI,
    .wc_Shake256_Update = wc_Shake256_Update_EfiAPI,
    .wc_Shake256_Final = wc_Shake256_Final_EfiAPI,
    .wc_Shake256_SqueezeBlocks = wc_Shake256_SqueezeBlocks_EfiAPI,
    .wc_Shake256_Free = wc_Shake256_Free_EfiAPI,

    /* ECC */
    .wc_ecc_init = wc_ecc_init_EfiAPI,
    .wc_ecc_free = wc_ecc_free_EfiAPI,
    .wc_ecc_make_key = wc_ecc_make_key_EfiAPI,
    .wc_ecc_set_rng = wc_ecc_set_rng_EfiAPI,
    .wc_ecc_shared_secret = wc_ecc_shared_secret_EfiAPI,
    .wc_ecc_sign_hash = wc_ecc_sign_hash_EfiAPI,
    .wc_ecc_verify_hash = wc_ecc_verify_hash_EfiAPI,
    .wc_ecc_export_x963 = wc_ecc_export_x963_EfiAPI,
    .wc_ecc_import_x963 = wc_ecc_import_x963_EfiAPI,
    .wc_EccKeyToDer = wc_EccKeyToDer_EfiAPI,
    .wc_EccPublicKeyDecode = wc_EccPublicKeyDecode_EfiAPI,
    .wc_EccPrivateKeyDecode = wc_EccPrivateKeyDecode_EfiAPI,

    /* DH */
    .wc_DhGenerateParams = wc_DhGenerateParams_EfiAPI,
    .wc_DhGenerateKeyPair = wc_DhGenerateKeyPair_EfiAPI,
    .wc_DhAgree = wc_DhAgree_EfiAPI,

    /* ChaCha20 */
    .wc_Chacha_SetKey = wc_Chacha_SetKey_EfiAPI,
    .wc_Chacha_Process = wc_Chacha_Process_EfiAPI,

    /* Poly1305 */
    .wc_Poly1305SetKey = wc_Poly1305SetKey_EfiAPI,
    .wc_Poly1305Update = wc_Poly1305Update_EfiAPI,
    .wc_Poly1305Final = wc_Poly1305Final_EfiAPI,

    /* ChaCha20-Poly1305 AEAD */
    .wc_ChaCha20Poly1305_Encrypt = wc_ChaCha20Poly1305_Encrypt_EfiAPI,
    .wc_ChaCha20Poly1305_Decrypt = wc_ChaCha20Poly1305_Decrypt_EfiAPI,

    /* Curve25519 */
    .wc_curve25519_init = wc_curve25519_init_EfiAPI,
    .wc_curve25519_free = wc_curve25519_free_EfiAPI,
    .wc_curve25519_make_key = wc_curve25519_make_key_EfiAPI,
    .wc_curve25519_shared_secret = wc_curve25519_shared_secret_EfiAPI,
    .wc_curve25519_export_key_raw = wc_curve25519_export_key_raw_EfiAPI,
    .wc_curve25519_import_public = wc_curve25519_import_public_EfiAPI,

    /* Ed25519 */
    .wc_ed25519_init = wc_ed25519_init_EfiAPI,
    .wc_ed25519_free = wc_ed25519_free_EfiAPI,
    .wc_ed25519_make_key = wc_ed25519_make_key_EfiAPI,
    .wc_ed25519_sign_msg = wc_ed25519_sign_msg_EfiAPI,
    .wc_ed25519_verify_msg = wc_ed25519_verify_msg_EfiAPI,
    .wc_ed25519_export_key = wc_ed25519_export_key_EfiAPI,
    .wc_ed25519_import_public = wc_ed25519_import_public_EfiAPI,

    /* ML-KEM */
#ifdef WOLFSSL_HAVE_MLKEM
    .wc_MlKemKey_Init = wc_MlKemKey_Init_EfiAPI,
    .wc_MlKemKey_Free = wc_MlKemKey_Free_EfiAPI,
    .wc_MlKemKey_MakeKey = wc_MlKemKey_MakeKey_EfiAPI,
    .wc_MlKemKey_CipherTextSize = wc_MlKemKey_CipherTextSize_EfiAPI,
    .wc_MlKemKey_SharedSecretSize = wc_MlKemKey_SharedSecretSize_EfiAPI,
    .wc_MlKemKey_Encapsulate = wc_MlKemKey_Encapsulate_EfiAPI,
    .wc_MlKemKey_Decapsulate = wc_MlKemKey_Decapsulate_EfiAPI,
    .wc_MlKemKey_DecodePrivateKey = wc_MlKemKey_DecodePrivateKey_EfiAPI,
    .wc_MlKemKey_DecodePublicKey = wc_MlKemKey_DecodePublicKey_EfiAPI,
    .wc_MlKemKey_PrivateKeySize = wc_MlKemKey_PrivateKeySize_EfiAPI,
    .wc_MlKemKey_PublicKeySize = wc_MlKemKey_PublicKeySize_EfiAPI,
    .wc_MlKemKey_EncodePrivateKey = wc_MlKemKey_EncodePrivateKey_EfiAPI,
    .wc_MlKemKey_EncodePublicKey = wc_MlKemKey_EncodePublicKey_EfiAPI,
#endif

    /* Dilithium */
#ifdef HAVE_DILITHIUM
    .wc_dilithium_init = wc_dilithium_init_EfiAPI,
    .wc_dilithium_free = wc_dilithium_free_EfiAPI,
    .wc_dilithium_set_level = wc_dilithium_set_level_EfiAPI,
    .wc_dilithium_make_key = wc_dilithium_make_key_EfiAPI,
    .wc_dilithium_sign_msg = wc_dilithium_sign_msg_EfiAPI,
    .wc_dilithium_verify_msg = wc_dilithium_verify_msg_EfiAPI,
    .wc_dilithium_export_key = wc_dilithium_export_key_EfiAPI,
    .wc_dilithium_import_key = wc_dilithium_import_key_EfiAPI,
#endif

    /* Falcon */
#ifdef HAVE_FALCON
    .wc_falcon_init = wc_falcon_init_EfiAPI,
    .wc_falcon_free = wc_falcon_free_EfiAPI,
    .wc_falcon_make_key = wc_falcon_make_key_EfiAPI,
    .wc_falcon_sign_msg = wc_falcon_sign_msg_EfiAPI,
    .wc_falcon_verify_msg = wc_falcon_verify_msg_EfiAPI,
    .wc_falcon_export_key = wc_falcon_export_key_EfiAPI,
    .wc_falcon_import_key = wc_falcon_import_key_EfiAPI,
#endif

    /* CMAC */
    .wc_InitCmac = wc_InitCmac_EfiAPI,
    .wc_CmacUpdate = wc_CmacUpdate_EfiAPI,
    .wc_CmacFinal = wc_CmacFinal_EfiAPI,

    /* KDF / PKCS */
    .wc_PBKDF2 = wc_PBKDF2_EfiAPI,
    .wc_PKCS12_PBKDF = wc_PKCS12_PBKDF_EfiAPI,
    .wc_PKCS7_PadData = wc_PKCS7_PadData_EfiAPI,
    .wc_HKDF = wc_HKDF_EfiAPI,

    /* Logging / Error */
    .wolfSSL_Debugging_ON = wolfSSL_Debugging_ON_EfiAPI,
    .wolfSSL_SetLoggingCb = wolfSSL_SetLoggingCb_EfiAPI,
    .wolfSSL_GetLoggingCb = wolfSSL_GetLoggingCb_EfiAPI,
    .wolfSSL_Debugging_OFF = wolfSSL_Debugging_OFF_EfiAPI,
    .wc_GetErrorString = wc_GetErrorString_EfiAPI,
    .wc_ErrorString = wc_ErrorString_EfiAPI,
};

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_LOADED_IMAGE *loaded_image = NULL;
    volatile int halted = 1;
    EFI_STATUS status;

    InitializeLib(ImageHandle, SystemTable);
    wolfSSL_Debugging_ON();
    wolfSSL_SetLoggingCb(logging_cb);

#ifdef UEFI_HW_ACCEL
    /* hw build requires AES-NI; exit gracefully if absent so startup.nsh can
     * fall back to the nohw driver. */
    {
        cpuid_flags_t cpu_flags = 0;
        cpuid_get_flags_ex(&cpu_flags);

        if (!IS_INTEL_AESNI(cpu_flags)) {
            Print(L"Error: hw driver requires AES-NI support (not detected)\n");
            Print(L"Please load libwolfcrypt-nohw.efi instead\n");
            return EFI_UNSUPPORTED;
        }
        Print(L"CPU AES-NI support detected, loading hw driver\n");
    }
#endif

    status = uefi_call_wrapper(SystemTable->BootServices->HandleProtocol,
                               3,
                               ImageHandle,
                               &LoadedImageProtocol,
                               (void **)&loaded_image);
    Print(L"wolfCrypt image loaded.\n");
    if (loaded_image != NULL) {
        Print(L"Image size: 0x%lx\n", loaded_image->ImageSize);
        Print(L"Image base: 0x%lx\n", loaded_image->ImageBase);
    }
    Print(L"status: 0x%lx\n", status);

#if WAIT_FOR_GDB
    Print(L"Debugging is enabled. Type the following in gdb to continue\n");
    if (loaded_image != NULL) {
        Print(L"symbol-file libwolfcrypt.elf -o 0x%lx\n", loaded_image->ImageBase);
    }
    Print(L"set halted = 0\n");
    while (halted) {
    }
#else
    WC_UNUSED(halted);
#endif

    /* Runtime relocation guard for API pointers.
     * If pointers are already relocated by the EFI loader we leave them as-is.
     * If pointers still look unrelocated, apply image-base adjustment once. */
    {
        UINTN *wc_api_ptr = (UINTN *)&g_wolfcrypt_api;
        UINTN wc_api_size = (sizeof(g_wolfcrypt_api) / sizeof(UINTN));
        UINTN image_base = (UINTN)loaded_image->ImageBase;
        UINTN relocated_count = 0;
        UINTN unrelocated_count = 0;
        int apply_fixup = 0;
        int i;

        for (i = 1; i < (int)wc_api_size; i++) {
            UINTN p = wc_api_ptr[i];
            if (p == 0) {
                continue;
            }
            if (p >= image_base) {
                relocated_count++;
            }
            else {
                unrelocated_count++;
            }
        }

        /* If most non-zero API entries are still below ImageBase, treat the
         * table as unrelocated and apply image-base adjustment. */
        apply_fixup = (unrelocated_count > relocated_count);

        if (apply_fixup) {
            for (i = 1; i < (int)wc_api_size; i++) {
                if (wc_api_ptr[i] != 0 && wc_api_ptr[i] < image_base) {
                    wc_api_ptr[i] += image_base;
                }
            }
            Print(L"wolfCrypt API symbols adjusted to image base\n");
        }
        else {
            Print(L"wolfCrypt API symbols verified at correct location\n");
        }
    }

    status = uefi_call_wrapper(BS->InstallProtocolInterface, 4,
                               &g_wolfcrypt_handle,
                               &g_wolfcrypt_protocol_guid,
                               EFI_NATIVE_INTERFACE,
                               &g_wolfcrypt_api);

    Print(L"InstallProtocolInterface status: 0x%lx\n", status);

    if (EFI_ERROR(status)) {
        Print(L"ERROR: Failed to install wolfCrypt protocol\n");
        return status;
    }

    Print(L"wolfCrypt driver loaded successfully (version 0x%08x)\n",
          g_wolfcrypt_api.Version);

    return EFI_SUCCESS;
}
