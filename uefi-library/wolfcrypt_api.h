#ifndef WOLFCRYPT_API_H_INCLUDED
#define WOLFCRYPT_API_H_INCLUDED

#include <efi.h>
#include <efilib.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/pwdbased.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/sha.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/sha512.h>
#include <wolfssl/wolfcrypt/sha3.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/mlkem.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/dh.h>
#include <wolfssl/wolfcrypt/chacha.h>
#include <wolfssl/wolfcrypt/poly1305.h>
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
#include <wolfssl/wolfcrypt/curve25519.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/cmac.h>
#ifdef HAVE_DILITHIUM
#include <wolfssl/wolfcrypt/dilithium.h>
#endif
#ifdef HAVE_FALCON
#include <wolfssl/wolfcrypt/falcon.h>
#endif

/* UUID: generated via uuidgen -r */
/* UUID: a3f2c1d7-8e4b-4f9a-b6c3-1d5e7f0a2b48 */
#define WOLFCRYPT_PROTOCOL_GUID \
    { 0xa3f2c1d7, 0x8e4b, 0x4f9a, { 0xb6, 0xc3, 0x1d, 0x5e, 0x7f, 0x0a, 0x2b, 0x48 } }

#ifndef WC_RNG_SEED_CB
typedef int (*wc_RngSeed_Cb)(OS_Seed* os, byte* seed, word32 sz);
#endif

#ifndef WOLFSSL_HAVE_MLKEM
typedef struct MlKemKey MlKemKey;
#endif

/* ------------------------------------------------------------------ */
/* AES */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_AesInit_API)(Aes* aes, void* heap, int devId);
typedef int  (EFIAPI *wc_AesSetKey_API)(Aes* aes, const byte* key, word32 len,
                                        const byte* iv, int dir);
typedef int  (EFIAPI *wc_AesEcbEncrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 sz);
typedef int  (EFIAPI *wc_AesEcbDecrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 sz);
typedef int  (EFIAPI *wc_AesCbcEncrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 sz);
typedef int  (EFIAPI *wc_AesCbcDecrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 sz);
typedef int  (EFIAPI *wc_AesCfbEncrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 sz);
typedef int  (EFIAPI *wc_AesCfbDecrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 sz);
typedef int  (EFIAPI *wc_AesSetIV_API)(Aes* aes, const byte* iv);
typedef void (EFIAPI *wc_AesFree_API)(Aes* aes);
typedef int  (EFIAPI *wc_AesGcmSetKey_API)(Aes* aes, const byte* key, word32 len);
typedef int  (EFIAPI *wc_AesGcmEncrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 sz, const byte* iv, word32 ivSz,
                                            byte* authTag, word32 authTagSz,
                                            const byte* authIn, word32 authInSz);
typedef int  (EFIAPI *wc_AesGcmDecrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 sz, const byte* iv, word32 ivSz,
                                            const byte* authTag, word32 authTagSz,
                                            const byte* authIn, word32 authInSz);
typedef int  (EFIAPI *wc_AesCcmSetKey_API)(Aes* aes, const byte* key, word32 keySz);
typedef int  (EFIAPI *wc_AesCcmEncrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 inSz, const byte* nonce, word32 nonceSz,
                                            byte* authTag, word32 authTagSz,
                                            const byte* authIn, word32 authInSz);
typedef int  (EFIAPI *wc_AesCcmDecrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 inSz, const byte* nonce, word32 nonceSz,
                                            const byte* authTag, word32 authTagSz,
                                            const byte* authIn, word32 authInSz);
typedef int  (EFIAPI *wc_AesCtrEncrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 sz);
typedef int  (EFIAPI *wc_AesOfbEncrypt_API)(Aes* aes, byte* out, const byte* in,
                                            word32 sz);

/* ------------------------------------------------------------------ */
/* RNG */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_InitRng_API)(WC_RNG* rng);
typedef int  (EFIAPI *wc_FreeRng_API)(WC_RNG* rng);
typedef int  (EFIAPI *wc_RNG_GenerateBlock_API)(WC_RNG* rng, byte* output,
                                                word32 sz);
typedef int  (EFIAPI *wc_RNG_GenerateByte_API)(WC_RNG* rng, byte* b);
typedef int  (EFIAPI *wc_SetSeed_Cb_API)(wc_RngSeed_Cb cb);
typedef int  (EFIAPI *wc_RNG_TestSeed_API)(const byte* seed, word32 seedSz);
typedef int  (EFIAPI *wc_GenerateSeed_IntelRD_API)(OS_Seed* os, byte* output,
                                                   word32 sz);
typedef int  (EFIAPI *wc_GenerateSeed_API)(OS_Seed* os, byte* output,
                                           word32 sz);
typedef int  (EFIAPI *wc_GenerateRand_IntelRD_API)(OS_Seed* os, byte* output,
                                                   word32 sz);
typedef void (EFIAPI *wc_InitRng_IntelRD_API)(void);

/* ------------------------------------------------------------------ */
/* RSA */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_InitRsaKey_API)(RsaKey* key, void* heap);
typedef int  (EFIAPI *wc_MakeRsaKey_API)(RsaKey* key, int size, long e,
                                         WC_RNG* rng);
typedef int  (EFIAPI *wc_RsaKeyToDer_API)(RsaKey* key, byte* output,
                                          word32 inLen);
typedef int  (EFIAPI *wc_FreeRsaKey_API)(RsaKey* key);
typedef int  (EFIAPI *wc_RsaEncryptSize_API)(const RsaKey* key);
typedef int  (EFIAPI *wc_RsaPrivateKeyDecode_API)(const byte* input,
                                                  word32* inOutIdx,
                                                  RsaKey* key, word32 inSz);
typedef int  (EFIAPI *wc_RsaPublicKeyDecode_API)(const byte* input,
                                                 word32* inOutIdx,
                                                 RsaKey* key, word32 inSz);
typedef int  (EFIAPI *wc_RsaPrivateDecryptEx_API)(const byte* in, word32 inLen,
                                                  byte* out, word32 outLen,
                                                  RsaKey* key, int type,
                                                  enum wc_HashType hash, int mgf,
                                                  byte* label, word32 labelSz);
typedef int  (EFIAPI *wc_RsaPrivateDecrypt_API)(const byte* in, word32 inLen,
                                                byte* out, word32 outLen,
                                                RsaKey* key);
typedef int  (EFIAPI *wc_RsaPublicEncrypt_API)(const byte* in, word32 inLen,
                                               byte* out, word32 outLen,
                                               RsaKey* key, WC_RNG* rng);
typedef int  (EFIAPI *wc_RsaPublicEncryptEx_API)(const byte* in, word32 inLen,
                                                 byte* out, word32 outLen,
                                                 RsaKey* key, WC_RNG* rng,
                                                 int type, enum wc_HashType hash,
                                                 int mgf, byte* label,
                                                 word32 labelSz);
typedef int  (EFIAPI *wc_RsaExportKey_API)(RsaKey* key,
                                           byte* e, word32* eSz,
                                           byte* n, word32* nSz,
                                           byte* d, word32* dSz,
                                           byte* p, word32* pSz,
                                           byte* q, word32* qSz);
typedef void (EFIAPI *wc_RsaCleanup_API)(RsaKey* key);
typedef int  (EFIAPI *wc_CheckRsaKey_API)(RsaKey* key);
typedef int  (EFIAPI *wc_RsaPublicKeyDerSize_API)(RsaKey* key, int withHeader);
typedef int  (EFIAPI *wc_RsaKeyToPublicDer_API)(RsaKey* key, byte* output,
                                                word32 inLen);
typedef int  (EFIAPI *wc_RsaSetRNG_API)(RsaKey* key, WC_RNG* rng);

/* ------------------------------------------------------------------ */
/* HMAC */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_HmacSetKey_API)(Hmac* hmac, int type, const byte* key,
                                         word32 length);
typedef int  (EFIAPI *wc_HmacUpdate_API)(Hmac* hmac, const byte* msg,
                                         word32 length);
typedef int  (EFIAPI *wc_HmacFinal_API)(Hmac* hmac, byte* hash);

/* ------------------------------------------------------------------ */
/* SHA-1 */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_InitSha_API)(wc_Sha* sha);
typedef int  (EFIAPI *wc_ShaUpdate_API)(wc_Sha* sha, const byte* data,
                                        word32 len);
typedef int  (EFIAPI *wc_ShaFinal_API)(wc_Sha* sha, byte* hash);
typedef void (EFIAPI *wc_ShaFree_API)(wc_Sha* sha);

/* ------------------------------------------------------------------ */
/* SHA-256 / SHA-224 */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_InitSha256_API)(wc_Sha256* sha);
typedef int  (EFIAPI *wc_InitSha256_ex_API)(wc_Sha256* sha, void* heap,
                                            int devId);
typedef int  (EFIAPI *wc_Sha256Update_API)(wc_Sha256* sha, const byte* data,
                                           word32 len);
typedef int  (EFIAPI *wc_Sha256Final_API)(wc_Sha256* sha256, byte* hash);
typedef void (EFIAPI *wc_Sha256Free_API)(wc_Sha256* sha256);

/* SHA-224 reuses wc_Sha256 struct with different init */
typedef int  (EFIAPI *wc_InitSha224_API)(wc_Sha224* sha);
typedef int  (EFIAPI *wc_Sha224Update_API)(wc_Sha224* sha, const byte* data,
                                           word32 len);
typedef int  (EFIAPI *wc_Sha224Final_API)(wc_Sha224* sha, byte* hash);
typedef void (EFIAPI *wc_Sha224Free_API)(wc_Sha224* sha);

/* ------------------------------------------------------------------ */
/* SHA-384 / SHA-512 */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_InitSha384_API)(wc_Sha384* sha);
typedef int  (EFIAPI *wc_Sha384Update_API)(wc_Sha384* sha, const byte* data,
                                           word32 len);
typedef int  (EFIAPI *wc_Sha384Final_API)(wc_Sha384* sha, byte* hash);
typedef void (EFIAPI *wc_Sha384Free_API)(wc_Sha384* sha);

typedef int  (EFIAPI *wc_InitSha512_API)(wc_Sha512* sha);
typedef int  (EFIAPI *wc_Sha512Update_API)(wc_Sha512* sha, const byte* data,
                                           word32 len);
typedef int  (EFIAPI *wc_Sha512Final_API)(wc_Sha512* sha, byte* hash);
typedef void (EFIAPI *wc_Sha512Free_API)(wc_Sha512* sha);

/* ------------------------------------------------------------------ */
/* SHA-3 (256/384/512) and SHAKE128/256 */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_InitSha3_256_API)(wc_Sha3* sha, void* heap, int devId);
typedef int  (EFIAPI *wc_Sha3_256_Update_API)(wc_Sha3* sha, const byte* data, word32 len);
typedef int  (EFIAPI *wc_Sha3_256_Final_API)(wc_Sha3* sha, byte* hash);
typedef void (EFIAPI *wc_Sha3_256_Free_API)(wc_Sha3* sha);

typedef int  (EFIAPI *wc_InitSha3_384_API)(wc_Sha3* sha, void* heap, int devId);
typedef int  (EFIAPI *wc_Sha3_384_Update_API)(wc_Sha3* sha, const byte* data, word32 len);
typedef int  (EFIAPI *wc_Sha3_384_Final_API)(wc_Sha3* sha, byte* hash);
typedef void (EFIAPI *wc_Sha3_384_Free_API)(wc_Sha3* sha);

typedef int  (EFIAPI *wc_InitSha3_512_API)(wc_Sha3* sha, void* heap, int devId);
typedef int  (EFIAPI *wc_Sha3_512_Update_API)(wc_Sha3* sha, const byte* data, word32 len);
typedef int  (EFIAPI *wc_Sha3_512_Final_API)(wc_Sha3* sha, byte* hash);
typedef void (EFIAPI *wc_Sha3_512_Free_API)(wc_Sha3* sha);

typedef int  (EFIAPI *wc_InitShake128_API)(wc_Shake* shake, void* heap, int devId);
typedef int  (EFIAPI *wc_Shake128_Update_API)(wc_Shake* shake, const byte* data, word32 len);
typedef int  (EFIAPI *wc_Shake128_Final_API)(wc_Shake* shake, byte* hash, word32 hashLen);
typedef int  (EFIAPI *wc_Shake128_SqueezeBlocks_API)(wc_Shake* shake, byte* out, word32 blockCnt);
typedef void (EFIAPI *wc_Shake128_Free_API)(wc_Shake* shake);

typedef int  (EFIAPI *wc_InitShake256_API)(wc_Shake* shake, void* heap, int devId);
typedef int  (EFIAPI *wc_Shake256_Update_API)(wc_Shake* shake, const byte* data, word32 len);
typedef int  (EFIAPI *wc_Shake256_Final_API)(wc_Shake* shake, byte* hash, word32 hashLen);
typedef int  (EFIAPI *wc_Shake256_SqueezeBlocks_API)(wc_Shake* shake, byte* out, word32 blockCnt);
typedef void (EFIAPI *wc_Shake256_Free_API)(wc_Shake* shake);

/* ------------------------------------------------------------------ */
/* ECC */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_ecc_init_API)(ecc_key* key);
typedef void (EFIAPI *wc_ecc_free_API)(ecc_key* key);
typedef int  (EFIAPI *wc_ecc_make_key_API)(WC_RNG* rng, int keysize, ecc_key* key);
typedef int  (EFIAPI *wc_ecc_set_rng_API)(ecc_key* key, WC_RNG* rng);
typedef int  (EFIAPI *wc_ecc_shared_secret_API)(ecc_key* priv, ecc_key* pub,
                                                byte* out, word32* outlen);
typedef int  (EFIAPI *wc_ecc_sign_hash_API)(const byte* in, word32 inlen,
                                            byte* out, word32* outlen,
                                            WC_RNG* rng, ecc_key* key);
typedef int  (EFIAPI *wc_ecc_verify_hash_API)(const byte* sig, word32 siglen,
                                              const byte* hash, word32 hashlen,
                                              int* stat, ecc_key* key);
typedef int  (EFIAPI *wc_ecc_export_x963_API)(ecc_key* key, byte* out, word32* outLen);
typedef int  (EFIAPI *wc_ecc_import_x963_API)(const byte* in, word32 inLen,
                                              ecc_key* key);
typedef int  (EFIAPI *wc_EccKeyToDer_API)(ecc_key* key, byte* output, word32 inLen);
typedef int  (EFIAPI *wc_EccPublicKeyDecode_API)(const byte* input, word32* inOutIdx,
                                                 ecc_key* key, word32 inSz);
typedef int  (EFIAPI *wc_EccPrivateKeyDecode_API)(const byte* input, word32* inOutIdx,
                                                  ecc_key* key, word32 inSz);

/* ------------------------------------------------------------------ */
/* DH */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_DhGenerateParams_API)(WC_RNG* rng, int modSz, DhKey* key);
typedef int  (EFIAPI *wc_DhGenerateKeyPair_API)(DhKey* key, WC_RNG* rng,
                                                byte* priv, word32* privSz,
                                                byte* pub, word32* pubSz);
typedef int  (EFIAPI *wc_DhAgree_API)(DhKey* key, byte* agree, word32* agreeSz,
                                      const byte* priv, word32 privSz,
                                      const byte* otherPub, word32 pubSz);

/* ------------------------------------------------------------------ */
/* ChaCha20 */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_Chacha_SetKey_API)(ChaCha* ctx, const byte* key, word32 keySz);
typedef int  (EFIAPI *wc_Chacha_Process_API)(ChaCha* ctx, byte* output,
                                             const byte* input, word32 msglen);

/* ------------------------------------------------------------------ */
/* Poly1305 */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_Poly1305SetKey_API)(Poly1305* ctx, const byte* key, word32 keySz);
typedef int  (EFIAPI *wc_Poly1305Update_API)(Poly1305* ctx, const byte* m, word32 bytes);
typedef int  (EFIAPI *wc_Poly1305Final_API)(Poly1305* ctx, byte* tag);

/* ------------------------------------------------------------------ */
/* ChaCha20-Poly1305 AEAD */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_ChaCha20Poly1305_Encrypt_API)(
    const byte* inKey, const byte* inIV,
    const byte* inAAD, word32 inAADLen,
    const byte* inPlaintext, word32 inPlaintextLen,
    byte* outCiphertext, byte* outAuthTag);
typedef int  (EFIAPI *wc_ChaCha20Poly1305_Decrypt_API)(
    const byte* inKey, const byte* inIV,
    const byte* inAAD, word32 inAADLen,
    const byte* inCiphertext, word32 inCiphertextLen,
    const byte* inAuthTag, byte* outPlaintext);

/* ------------------------------------------------------------------ */
/* Curve25519 */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_curve25519_init_API)(curve25519_key* key);
typedef void (EFIAPI *wc_curve25519_free_API)(curve25519_key* key);
typedef int  (EFIAPI *wc_curve25519_make_key_API)(WC_RNG* rng, int keysize,
                                                  curve25519_key* key);
typedef int  (EFIAPI *wc_curve25519_shared_secret_API)(curve25519_key* priv,
                                                       curve25519_key* pub,
                                                       byte* out, word32* outlen);
typedef int  (EFIAPI *wc_curve25519_export_key_raw_API)(curve25519_key* key,
                                                        byte* priv, word32* privSz,
                                                        byte* pub, word32* pubSz);
typedef int  (EFIAPI *wc_curve25519_import_public_API)(const byte* in, word32 inLen,
                                                       curve25519_key* key);

/* ------------------------------------------------------------------ */
/* Ed25519 */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_ed25519_init_API)(ed25519_key* key);
typedef void (EFIAPI *wc_ed25519_free_API)(ed25519_key* key);
typedef int  (EFIAPI *wc_ed25519_make_key_API)(WC_RNG* rng, int keysize,
                                               ed25519_key* key);
typedef int  (EFIAPI *wc_ed25519_sign_msg_API)(const byte* in, word32 inlen,
                                               byte* out, word32* outlen,
                                               ed25519_key* key);
typedef int  (EFIAPI *wc_ed25519_verify_msg_API)(const byte* sig, word32 siglen,
                                                 const byte* msg, word32 msglen,
                                                 int* stat, ed25519_key* key);
typedef int  (EFIAPI *wc_ed25519_export_key_API)(ed25519_key* key,
                                                 byte* priv, word32* privSz,
                                                 byte* pub, word32* pubSz);
typedef int  (EFIAPI *wc_ed25519_import_public_API)(const byte* in, word32 inLen,
                                                    ed25519_key* key);

/* ------------------------------------------------------------------ */
/* ML-KEM */
/* ------------------------------------------------------------------ */
#ifdef WOLFSSL_HAVE_MLKEM
typedef int (EFIAPI *wc_MlKemKey_Init_API)(MlKemKey* key, int type, void* heap,
                                           int devId);
typedef int (EFIAPI *wc_MlKemKey_Free_API)(MlKemKey* key);
typedef int (EFIAPI *wc_MlKemKey_MakeKey_API)(MlKemKey* key, WC_RNG* rng);
typedef int (EFIAPI *wc_MlKemKey_CipherTextSize_API)(MlKemKey* key, word32* len);
typedef int (EFIAPI *wc_MlKemKey_SharedSecretSize_API)(MlKemKey* key,
                                                        word32* len);
typedef int (EFIAPI *wc_MlKemKey_Encapsulate_API)(MlKemKey* key,
                                                  unsigned char* ct,
                                                  unsigned char* ss,
                                                  WC_RNG* rng);
typedef int (EFIAPI *wc_MlKemKey_Decapsulate_API)(MlKemKey* key,
                                                  unsigned char* ss,
                                                  const unsigned char* ct,
                                                  word32 len);
typedef int (EFIAPI *wc_MlKemKey_DecodePrivateKey_API)(MlKemKey* key,
                                                       const unsigned char* in,
                                                       word32 len);
typedef int (EFIAPI *wc_MlKemKey_DecodePublicKey_API)(MlKemKey* key,
                                                      const unsigned char* in,
                                                      word32 len);
typedef int (EFIAPI *wc_MlKemKey_PrivateKeySize_API)(MlKemKey* key,
                                                     word32* len);
typedef int (EFIAPI *wc_MlKemKey_PublicKeySize_API)(MlKemKey* key,
                                                    word32* len);
typedef int (EFIAPI *wc_MlKemKey_EncodePrivateKey_API)(MlKemKey* key,
                                                       unsigned char* out,
                                                       word32 len);
typedef int (EFIAPI *wc_MlKemKey_EncodePublicKey_API)(MlKemKey* key,
                                                      unsigned char* out,
                                                      word32 len);
#endif /* WOLFSSL_HAVE_MLKEM */

/* ------------------------------------------------------------------ */
/* Dilithium (ML-DSA) */
/* ------------------------------------------------------------------ */
#ifdef HAVE_DILITHIUM
typedef int  (EFIAPI *wc_dilithium_init_API)(dilithium_key* key);
typedef void (EFIAPI *wc_dilithium_free_API)(dilithium_key* key);
typedef int  (EFIAPI *wc_dilithium_set_level_API)(dilithium_key* key, byte level);
typedef int  (EFIAPI *wc_dilithium_make_key_API)(dilithium_key* key, WC_RNG* rng);
typedef int  (EFIAPI *wc_dilithium_sign_msg_API)(const byte* in, word32 inLen,
                                                 byte* out, word32* outLen,
                                                 dilithium_key* key, WC_RNG* rng);
typedef int  (EFIAPI *wc_dilithium_verify_msg_API)(const byte* sig, word32 sigLen,
                                                   const byte* msg, word32 msgLen,
                                                   int* res, dilithium_key* key);
typedef int  (EFIAPI *wc_dilithium_export_key_API)(dilithium_key* key,
                                                   byte* priv, word32* privSz,
                                                   byte* pub, word32* pubSz);
typedef int  (EFIAPI *wc_dilithium_import_key_API)(const byte* priv, word32 privSz,
                                                   const byte* pub, word32 pubSz,
                                                   dilithium_key* key);
#endif /* HAVE_DILITHIUM */

/* ------------------------------------------------------------------ */
/* Falcon */
/* ------------------------------------------------------------------ */
#ifdef HAVE_FALCON
typedef int  (EFIAPI *wc_falcon_init_API)(falcon_key* key);
typedef void (EFIAPI *wc_falcon_free_API)(falcon_key* key);
typedef int  (EFIAPI *wc_falcon_make_key_API)(falcon_key* key, WC_RNG* rng);
typedef int  (EFIAPI *wc_falcon_sign_msg_API)(const byte* in, word32 inLen,
                                              byte* out, word32* outLen,
                                              falcon_key* key, WC_RNG* rng);
typedef int  (EFIAPI *wc_falcon_verify_msg_API)(const byte* sig, word32 sigLen,
                                                const byte* msg, word32 msgLen,
                                                int* res, falcon_key* key);
typedef int  (EFIAPI *wc_falcon_export_key_API)(falcon_key* key,
                                                byte* priv, word32* privSz,
                                                byte* pub, word32* pubSz);
typedef int  (EFIAPI *wc_falcon_import_key_API)(const byte* priv, word32 privSz,
                                                const byte* pub, word32 pubSz,
                                                falcon_key* key);
#endif /* HAVE_FALCON */

/* ------------------------------------------------------------------ */
/* CMAC */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_InitCmac_API)(Cmac* cmac, const byte* key, word32 keySz,
                                       int type, void* unused);
typedef int  (EFIAPI *wc_CmacUpdate_API)(Cmac* cmac, const byte* in, word32 inSz);
typedef int  (EFIAPI *wc_CmacFinal_API)(Cmac* cmac, byte* out, word32* outSz);

/* ------------------------------------------------------------------ */
/* PBKDF2 / PKCS12 / PKCS7 / HKDF */
/* ------------------------------------------------------------------ */
typedef int  (EFIAPI *wc_PBKDF2_API)(byte* output, const byte* passwd, int pLen,
                                     const byte* salt, int sLen, int iterations,
                                     int kLen, int hashType);
typedef int  (EFIAPI *wc_PKCS12_PBKDF_API)(byte* output, const byte* passwd,
                                           int passLen, const byte* salt,
                                           int saltLen, int iterations,
                                           int kLen, int hashType, int id);
typedef int  (EFIAPI *wc_PKCS7_PadData_API)(byte* in, word32 inSz, byte* out,
                                            word32 outSz, word32 blockSz);
typedef int  (EFIAPI *wc_HKDF_API)(int type, const byte* inKey, word32 inKeySz,
                                   const byte* salt, word32 saltSz,
                                   const byte* info, word32 infoSz,
                                   byte* out, word32 outSz);

/* ------------------------------------------------------------------ */
/* Logging / Error / Version */
/* ------------------------------------------------------------------ */
typedef int                (EFIAPI *wolfSSL_Debugging_ON_API)(void);
typedef int                (EFIAPI *wolfSSL_SetLoggingCb_API)(wolfSSL_Logging_cb cb);
typedef wolfSSL_Logging_cb (EFIAPI *wolfSSL_GetLoggingCb_API)(void);
typedef void               (EFIAPI *wolfSSL_Debugging_OFF_API)(void);

typedef const char* (EFIAPI *wc_GetErrorString_API)(int error);
typedef void        (EFIAPI *wc_ErrorString_API)(int err, char* buff);

/* ------------------------------------------------------------------ */
/* Protocol struct */
/* ------------------------------------------------------------------ */
extern EFI_GUID g_wolfcrypt_protocol_guid;
typedef struct {
    UINT32 Version;

    /* AES */
    wc_AesInit_API                wc_AesInit;
    wc_AesSetKey_API              wc_AesSetKey;
    wc_AesEcbEncrypt_API          wc_AesEcbEncrypt;
    wc_AesEcbDecrypt_API          wc_AesEcbDecrypt;
    wc_AesCbcEncrypt_API          wc_AesCbcEncrypt;
    wc_AesCbcDecrypt_API          wc_AesCbcDecrypt;
    wc_AesCfbEncrypt_API          wc_AesCfbEncrypt;
    wc_AesCfbDecrypt_API          wc_AesCfbDecrypt;
    wc_AesSetIV_API               wc_AesSetIV;
    wc_AesFree_API                wc_AesFree;
    wc_AesGcmSetKey_API           wc_AesGcmSetKey;
    wc_AesGcmEncrypt_API          wc_AesGcmEncrypt;
    wc_AesGcmDecrypt_API          wc_AesGcmDecrypt;
    wc_AesCcmSetKey_API           wc_AesCcmSetKey;
    wc_AesCcmEncrypt_API          wc_AesCcmEncrypt;
    wc_AesCcmDecrypt_API          wc_AesCcmDecrypt;
    wc_AesCtrEncrypt_API          wc_AesCtrEncrypt;
    wc_AesOfbEncrypt_API          wc_AesOfbEncrypt;

    /* RNG */
    wc_InitRng_API                wc_InitRng;
    wc_FreeRng_API                wc_FreeRng;
    wc_RNG_GenerateBlock_API      wc_RNG_GenerateBlock;
    wc_RNG_GenerateByte_API       wc_RNG_GenerateByte;
    wc_SetSeed_Cb_API             wc_SetSeed_Cb;
    wc_RNG_TestSeed_API           wc_RNG_TestSeed;
    wc_GenerateSeed_IntelRD_API   wc_GenerateSeed_IntelRD;
    wc_GenerateSeed_API           wc_GenerateSeed;
    wc_GenerateRand_IntelRD_API   wc_GenerateRand_IntelRD;
    wc_InitRng_IntelRD_API        wc_InitRng_IntelRD;

    /* RSA */
    wc_InitRsaKey_API             wc_InitRsaKey;
    wc_MakeRsaKey_API             wc_MakeRsaKey;
    wc_RsaKeyToDer_API            wc_RsaKeyToDer;
    wc_FreeRsaKey_API             wc_FreeRsaKey;
    wc_RsaEncryptSize_API         wc_RsaEncryptSize;
    wc_RsaPrivateKeyDecode_API    wc_RsaPrivateKeyDecode;
    wc_RsaPublicKeyDecode_API     wc_RsaPublicKeyDecode;
    wc_RsaPrivateDecryptEx_API    wc_RsaPrivateDecryptEx;
    wc_RsaPrivateDecrypt_API      wc_RsaPrivateDecrypt;
    wc_RsaPublicEncrypt_API       wc_RsaPublicEncrypt;
    wc_RsaPublicEncryptEx_API     wc_RsaPublicEncryptEx;
    wc_RsaExportKey_API           wc_RsaExportKey;
    wc_RsaCleanup_API             wc_RsaCleanup;
    wc_CheckRsaKey_API            wc_CheckRsaKey;
    wc_RsaPublicKeyDerSize_API    wc_RsaPublicKeyDerSize;
    wc_RsaKeyToPublicDer_API      wc_RsaKeyToPublicDer;
    wc_RsaSetRNG_API              wc_RsaSetRNG;

    /* HMAC */
    wc_HmacSetKey_API             wc_HmacSetKey;
    wc_HmacUpdate_API             wc_HmacUpdate;
    wc_HmacFinal_API              wc_HmacFinal;

    /* SHA-1 */
    wc_InitSha_API                wc_InitSha;
    wc_ShaUpdate_API              wc_ShaUpdate;
    wc_ShaFinal_API               wc_ShaFinal;
    wc_ShaFree_API                wc_ShaFree;

    /* SHA-224 */
    wc_InitSha224_API             wc_InitSha224;
    wc_Sha224Update_API           wc_Sha224Update;
    wc_Sha224Final_API            wc_Sha224Final;
    wc_Sha224Free_API             wc_Sha224Free;

    /* SHA-256 */
    wc_InitSha256_API             wc_InitSha256;
    wc_InitSha256_ex_API          wc_InitSha256_ex;
    wc_Sha256Update_API           wc_Sha256Update;
    wc_Sha256Final_API            wc_Sha256Final;
    wc_Sha256Free_API             wc_Sha256Free;

    /* SHA-384 */
    wc_InitSha384_API             wc_InitSha384;
    wc_Sha384Update_API           wc_Sha384Update;
    wc_Sha384Final_API            wc_Sha384Final;
    wc_Sha384Free_API             wc_Sha384Free;

    /* SHA-512 */
    wc_InitSha512_API             wc_InitSha512;
    wc_Sha512Update_API           wc_Sha512Update;
    wc_Sha512Final_API            wc_Sha512Final;
    wc_Sha512Free_API             wc_Sha512Free;

    /* SHA-3 */
    wc_InitSha3_256_API           wc_InitSha3_256;
    wc_Sha3_256_Update_API        wc_Sha3_256_Update;
    wc_Sha3_256_Final_API         wc_Sha3_256_Final;
    wc_Sha3_256_Free_API          wc_Sha3_256_Free;
    wc_InitSha3_384_API           wc_InitSha3_384;
    wc_Sha3_384_Update_API        wc_Sha3_384_Update;
    wc_Sha3_384_Final_API         wc_Sha3_384_Final;
    wc_Sha3_384_Free_API          wc_Sha3_384_Free;
    wc_InitSha3_512_API           wc_InitSha3_512;
    wc_Sha3_512_Update_API        wc_Sha3_512_Update;
    wc_Sha3_512_Final_API         wc_Sha3_512_Final;
    wc_Sha3_512_Free_API          wc_Sha3_512_Free;

    /* SHAKE128/256 */
    wc_InitShake128_API           wc_InitShake128;
    wc_Shake128_Update_API        wc_Shake128_Update;
    wc_Shake128_Final_API         wc_Shake128_Final;
    wc_Shake128_SqueezeBlocks_API wc_Shake128_SqueezeBlocks;
    wc_Shake128_Free_API          wc_Shake128_Free;
    wc_InitShake256_API           wc_InitShake256;
    wc_Shake256_Update_API        wc_Shake256_Update;
    wc_Shake256_Final_API         wc_Shake256_Final;
    wc_Shake256_SqueezeBlocks_API wc_Shake256_SqueezeBlocks;
    wc_Shake256_Free_API          wc_Shake256_Free;

    /* ECC */
    wc_ecc_init_API               wc_ecc_init;
    wc_ecc_free_API               wc_ecc_free;
    wc_ecc_make_key_API           wc_ecc_make_key;
    wc_ecc_set_rng_API            wc_ecc_set_rng;
    wc_ecc_shared_secret_API      wc_ecc_shared_secret;
    wc_ecc_sign_hash_API          wc_ecc_sign_hash;
    wc_ecc_verify_hash_API        wc_ecc_verify_hash;
    wc_ecc_export_x963_API        wc_ecc_export_x963;
    wc_ecc_import_x963_API        wc_ecc_import_x963;
    wc_EccKeyToDer_API            wc_EccKeyToDer;
    wc_EccPublicKeyDecode_API     wc_EccPublicKeyDecode;
    wc_EccPrivateKeyDecode_API    wc_EccPrivateKeyDecode;

    /* DH */
    wc_DhGenerateParams_API       wc_DhGenerateParams;
    wc_DhGenerateKeyPair_API      wc_DhGenerateKeyPair;
    wc_DhAgree_API                wc_DhAgree;

    /* ChaCha20 */
    wc_Chacha_SetKey_API          wc_Chacha_SetKey;
    wc_Chacha_Process_API         wc_Chacha_Process;

    /* Poly1305 */
    wc_Poly1305SetKey_API         wc_Poly1305SetKey;
    wc_Poly1305Update_API         wc_Poly1305Update;
    wc_Poly1305Final_API          wc_Poly1305Final;

    /* ChaCha20-Poly1305 AEAD */
    wc_ChaCha20Poly1305_Encrypt_API wc_ChaCha20Poly1305_Encrypt;
    wc_ChaCha20Poly1305_Decrypt_API wc_ChaCha20Poly1305_Decrypt;

    /* Curve25519 */
    wc_curve25519_init_API        wc_curve25519_init;
    wc_curve25519_free_API        wc_curve25519_free;
    wc_curve25519_make_key_API    wc_curve25519_make_key;
    wc_curve25519_shared_secret_API wc_curve25519_shared_secret;
    wc_curve25519_export_key_raw_API wc_curve25519_export_key_raw;
    wc_curve25519_import_public_API wc_curve25519_import_public;

    /* Ed25519 */
    wc_ed25519_init_API           wc_ed25519_init;
    wc_ed25519_free_API           wc_ed25519_free;
    wc_ed25519_make_key_API       wc_ed25519_make_key;
    wc_ed25519_sign_msg_API       wc_ed25519_sign_msg;
    wc_ed25519_verify_msg_API     wc_ed25519_verify_msg;
    wc_ed25519_export_key_API     wc_ed25519_export_key;
    wc_ed25519_import_public_API  wc_ed25519_import_public;

    /* ML-KEM */
#ifdef WOLFSSL_HAVE_MLKEM
    wc_MlKemKey_Init_API          wc_MlKemKey_Init;
    wc_MlKemKey_Free_API          wc_MlKemKey_Free;
    wc_MlKemKey_MakeKey_API       wc_MlKemKey_MakeKey;
    wc_MlKemKey_CipherTextSize_API wc_MlKemKey_CipherTextSize;
    wc_MlKemKey_SharedSecretSize_API wc_MlKemKey_SharedSecretSize;
    wc_MlKemKey_Encapsulate_API   wc_MlKemKey_Encapsulate;
    wc_MlKemKey_Decapsulate_API   wc_MlKemKey_Decapsulate;
    wc_MlKemKey_DecodePrivateKey_API wc_MlKemKey_DecodePrivateKey;
    wc_MlKemKey_DecodePublicKey_API  wc_MlKemKey_DecodePublicKey;
    wc_MlKemKey_PrivateKeySize_API   wc_MlKemKey_PrivateKeySize;
    wc_MlKemKey_PublicKeySize_API    wc_MlKemKey_PublicKeySize;
    wc_MlKemKey_EncodePrivateKey_API wc_MlKemKey_EncodePrivateKey;
    wc_MlKemKey_EncodePublicKey_API  wc_MlKemKey_EncodePublicKey;
#endif

    /* Dilithium */
#ifdef HAVE_DILITHIUM
    wc_dilithium_init_API         wc_dilithium_init;
    wc_dilithium_free_API         wc_dilithium_free;
    wc_dilithium_set_level_API    wc_dilithium_set_level;
    wc_dilithium_make_key_API     wc_dilithium_make_key;
    wc_dilithium_sign_msg_API     wc_dilithium_sign_msg;
    wc_dilithium_verify_msg_API   wc_dilithium_verify_msg;
    wc_dilithium_export_key_API   wc_dilithium_export_key;
    wc_dilithium_import_key_API   wc_dilithium_import_key;
#endif

    /* Falcon */
#ifdef HAVE_FALCON
    wc_falcon_init_API            wc_falcon_init;
    wc_falcon_free_API            wc_falcon_free;
    wc_falcon_make_key_API        wc_falcon_make_key;
    wc_falcon_sign_msg_API        wc_falcon_sign_msg;
    wc_falcon_verify_msg_API      wc_falcon_verify_msg;
    wc_falcon_export_key_API      wc_falcon_export_key;
    wc_falcon_import_key_API      wc_falcon_import_key;
#endif

    /* CMAC */
    wc_InitCmac_API               wc_InitCmac;
    wc_CmacUpdate_API             wc_CmacUpdate;
    wc_CmacFinal_API              wc_CmacFinal;

    /* PBKDF2 / PKCS12 / PKCS7 / HKDF */
    wc_PBKDF2_API                 wc_PBKDF2;
    wc_PKCS12_PBKDF_API           wc_PKCS12_PBKDF;
    wc_PKCS7_PadData_API          wc_PKCS7_PadData;
    wc_HKDF_API                   wc_HKDF;

    /* Logging / Error */
    wolfSSL_Debugging_ON_API      wolfSSL_Debugging_ON;
    wolfSSL_SetLoggingCb_API      wolfSSL_SetLoggingCb;
    wolfSSL_GetLoggingCb_API      wolfSSL_GetLoggingCb;
    wolfSSL_Debugging_OFF_API     wolfSSL_Debugging_OFF;

    wc_GetErrorString_API         wc_GetErrorString;
    wc_ErrorString_API            wc_ErrorString;
} WOLFCRYPT_PROTOCOL;

#endif /* WOLFCRYPT_API_H_INCLUDED */
