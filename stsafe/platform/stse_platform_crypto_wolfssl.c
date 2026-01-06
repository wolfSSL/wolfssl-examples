/* stse_platform_crypto_wolfssl.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

#include "core/stse_platform.h"
#include "stse_conf.h"

/* wolfSSL includes */
#ifdef WOLFSSL_USER_SETTINGS
    #include "user_settings.h"
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/cmac.h>
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/sha512.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/ecc.h>

#include <string.h>
#include <stdlib.h>

/* ========================================================================== */
/* Helper Macros                                                              */
/* ========================================================================== */

#ifndef MIN
#define MIN(a,b)           ((a)<(b) ? (a) : (b))
#endif
#define _MSB(x)            ((((x)[0]&0x80)==0x80) ? 1 : 0)

#define ZERO_STRUCT(x)     memset((void*)&(x), 0, sizeof(x))
#define ZERO_STRUCTP(x)    do { if ((x) != NULL) { memset((void*)(x), 0, sizeof(*(x)));} } while(0)

/* ========================================================================== */
/* AES CMAC Constants (RFC 4493)                                              */
/* ========================================================================== */

static const uint8_t const_Rb[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
};
static const uint8_t const_Zero[16] = {0x00};

/* ========================================================================== */
/* AES CMAC Context Structure                                                 */
/* ========================================================================== */

typedef struct {
    Aes aes;
    uint8_t k1[16];
    uint8_t k2[16];
    uint8_t x[16];
    uint8_t last[16];
    size_t last_len;
    PLAT_UI16 exp_tag_size;
} stse_cmac_ctx_t;

/* Global CMAC context for init/append/finish pattern */
static stse_cmac_ctx_t* g_cmac_ctx = NULL;

/* ========================================================================== */
/* Internal Helper Functions                                                  */
/* ========================================================================== */

static inline void aes_cmac_left_shift_1(const uint8_t in[16], uint8_t out[16])
{
    int8_t i;
    uint8_t overflow = 0;

    for (i = 15; i >= 0; i--) {
        out[i] = in[i] << 1;
        out[i] |= overflow;
        overflow = _MSB(&in[i]);
    }
}

static inline void aes_cmac_xor(const uint8_t in1[16], const uint8_t in2[16], uint8_t out[16])
{
    int8_t i;
    for (i = 0; i < 16; i++) {
        out[i] = in1[i] ^ in2[i];
    }
}

static int aes_cmac_generate_subkeys(Aes* aes, uint8_t k1[16], uint8_t k2[16])
{
    uint8_t L[16];
    uint8_t tmp_block[16];
    uint8_t iv[16] = {0};
    int ret;

    /* Generate L = AES-128(K, const_Zero) */
    ret = wc_AesSetIV(aes, iv);
    if (ret != 0) return ret;

    ret = wc_AesCbcEncrypt(aes, L, const_Zero, 16);
    if (ret != 0) return ret;

    /* Generate K1 */
    if (_MSB(L) == 0) {
        aes_cmac_left_shift_1(L, k1);
    } else {
        aes_cmac_left_shift_1(L, tmp_block);
        aes_cmac_xor(tmp_block, const_Rb, k1);
    }

    /* Generate K2 */
    if (_MSB(k1) == 0) {
        aes_cmac_left_shift_1(k1, k2);
    } else {
        aes_cmac_left_shift_1(k1, tmp_block);
        aes_cmac_xor(tmp_block, const_Rb, k2);
    }

    ZERO_STRUCT(L);
    ZERO_STRUCT(tmp_block);

    return 0;
}

/* ========================================================================== */
/* Platform Crypto Initialization                                             */
/* ========================================================================== */

stse_ReturnCode_t stse_platform_crypto_init(void)
{
    /* wolfSSL library initialization if needed */
    return STSE_OK;
}

/* ========================================================================== */
/* Hash Functions                                                             */
/* ========================================================================== */

stse_ReturnCode_t stse_platform_hash_compute(stse_hash_algorithm_t hash_algo,
                                             PLAT_UI8 *pPayload, PLAT_UI16 payload_length,
                                             PLAT_UI8 *pHash, PLAT_UI16 *hash_length)
{
    int ret = -1;

    if (pPayload == NULL || pHash == NULL || hash_length == NULL) {
        return STSE_PLATFORM_HASH_ERROR;
    }

    switch (hash_algo) {
#ifdef STSE_CONF_HASH_SHA_256
        case STSE_SHA_256:
            if (*hash_length >= WC_SHA256_DIGEST_SIZE) {
                ret = wc_Sha256Hash(pPayload, payload_length, pHash);
                *hash_length = WC_SHA256_DIGEST_SIZE;
            }
            break;
#endif
#ifdef STSE_CONF_HASH_SHA_384
        case STSE_SHA_384:
            if (*hash_length >= WC_SHA384_DIGEST_SIZE) {
                ret = wc_Sha384Hash(pPayload, payload_length, pHash);
                *hash_length = WC_SHA384_DIGEST_SIZE;
            }
            break;
#endif
        default:
            return STSE_PLATFORM_HASH_ERROR;
    }

    if (ret != 0) {
        return STSE_PLATFORM_HASH_ERROR;
    }

    return STSE_OK;
}

/* ========================================================================== */
/* ECC Verification Functions                                                 */
/* ========================================================================== */

/**
 * \brief Get key size in bytes for a given STSE ECC key type
 */
static int stse_get_ecc_key_size(stse_ecc_key_type_t key_type)
{
    switch (key_type) {
#ifdef STSE_CONF_ECC_NIST_P_256
        case STSE_ECC_KT_NIST_P_256:
            return 32;
#endif
#ifdef STSE_CONF_ECC_BRAINPOOL_P_256
        case STSE_ECC_KT_BP_P_256:
            return 32;
#endif
#ifdef STSE_CONF_ECC_CURVE_25519
        case STSE_ECC_KT_CURVE25519:
            return 32;
#endif
#ifdef STSE_CONF_ECC_EDWARD_25519
        case STSE_ECC_KT_ED25519:
            return 32;
#endif
#ifdef STSE_CONF_ECC_NIST_P_384
        case STSE_ECC_KT_NIST_P_384:
            return 48;
#endif
#ifdef STSE_CONF_ECC_BRAINPOOL_P_384
        case STSE_ECC_KT_BP_P_384:
            return 48;
#endif
#ifdef STSE_CONF_ECC_BRAINPOOL_P_512
        case STSE_ECC_KT_BP_P_512:
            return 64;
#endif
#ifdef STSE_CONF_ECC_NIST_P_521
        case STSE_ECC_KT_NIST_P_521:
            return 66;
#endif
        default:
            return 0;
    }
}

/**
 * \brief Get wolfSSL curve ID for a given STSE ECC key type
 */
static int stse_get_wolfssl_curve_id(stse_ecc_key_type_t key_type)
{
    switch (key_type) {
#ifdef STSE_CONF_ECC_NIST_P_256
        case STSE_ECC_KT_NIST_P_256:
            return ECC_SECP256R1;
#endif
#ifdef STSE_CONF_ECC_NIST_P_384
        case STSE_ECC_KT_NIST_P_384:
            return ECC_SECP384R1;
#endif
#if defined(STSE_CONF_ECC_NIST_P_521) && defined(HAVE_ECC521)
        case STSE_ECC_KT_NIST_P_521:
            return ECC_SECP521R1;
#endif
#if defined(STSE_CONF_ECC_BRAINPOOL_P_256) && defined(HAVE_ECC_BRAINPOOL)
        case STSE_ECC_KT_BP_P_256:
            return ECC_BRAINPOOLP256R1;
#endif
#if defined(STSE_CONF_ECC_BRAINPOOL_P_384) && defined(HAVE_ECC_BRAINPOOL)
        case STSE_ECC_KT_BP_P_384:
            return ECC_BRAINPOOLP384R1;
#endif
#if defined(STSE_CONF_ECC_BRAINPOOL_P_512) && defined(HAVE_ECC_BRAINPOOL)
        case STSE_ECC_KT_BP_P_512:
            return ECC_BRAINPOOLP512R1;
#endif
#if defined(STSE_CONF_ECC_CURVE_25519) && defined(HAVE_CURVE25519)
        case STSE_ECC_KT_CURVE25519:
            return ECC_X25519;
#endif
#if defined(STSE_CONF_ECC_EDWARD_25519) && defined(HAVE_ED25519)
        case STSE_ECC_KT_ED25519:
            return ECC_ED25519;
#endif
        default:
            return ECC_SECP256R1; /* Default to P-256 */
    }
}

stse_ReturnCode_t stse_platform_ecc_verify(stse_ecc_key_type_t key_type,
                                           const PLAT_UI8 *pPubKey,
                                           PLAT_UI8 *pDigest,
                                           PLAT_UI16 digestLen,
                                           PLAT_UI8 *pSignature)
{
    ecc_key eccKey;
    int ret;
    int verified = 0;
    int key_size;
    int curve_id;
    byte derSig[ECC_MAX_SIG_SIZE];
    word32 derSigLen = sizeof(derSig);

    if (pPubKey == NULL || pDigest == NULL || pSignature == NULL) {
        return STSE_PLATFORM_ECC_VERIFY_ERROR;
    }

    key_size = stse_get_ecc_key_size(key_type);
    if (key_size == 0) {
        return STSE_PLATFORM_ECC_VERIFY_ERROR;
    }

    curve_id = stse_get_wolfssl_curve_id(key_type);

    /* Initialize ECC key */
    ret = wc_ecc_init(&eccKey);
    if (ret != 0) {
        return STSE_PLATFORM_ECC_VERIFY_ERROR;
    }

    /* Import public key (X || Y format) */
    ret = wc_ecc_import_unsigned(&eccKey,
                                  (byte*)pPubKey,              /* X coordinate */
                                  (byte*)(pPubKey + key_size), /* Y coordinate */
                                  NULL,                        /* No private key */
                                  curve_id);
    if (ret != 0) {
        wc_ecc_free(&eccKey);
        return STSE_PLATFORM_ECC_VERIFY_ERROR;
    }

    /* Convert R || S signature to DER format */
    ret = wc_ecc_rs_raw_to_sig(pSignature, key_size,
                               pSignature + key_size, key_size,
                               derSig, &derSigLen);
    if (ret != 0) {
        wc_ecc_free(&eccKey);
        return STSE_PLATFORM_ECC_VERIFY_ERROR;
    }

    /* Verify signature */
    ret = wc_ecc_verify_hash(derSig, derSigLen, pDigest, digestLen,
                             &verified, &eccKey);

    wc_ecc_free(&eccKey);

    if (ret != 0 || verified != 1) {
        return STSE_PLATFORM_ECC_VERIFY_ERROR;
    }

    return STSE_OK;
}

/* ========================================================================== */
/* AES ECB Functions                                                          */
/* ========================================================================== */

#if defined(STSE_CONF_USE_HOST_KEY_ESTABLISHMENT) || \
    defined(STSE_CONF_USE_SYMMETRIC_KEY_ESTABLISHMENT) || \
    defined(STSE_CONF_USE_HOST_SESSION)

stse_ReturnCode_t stse_platform_aes_ecb_enc(const PLAT_UI8 *pPlaintext,
                                            PLAT_UI16 plaintext_length,
                                            const PLAT_UI8 *pKey,
                                            PLAT_UI16 key_length,
                                            PLAT_UI8 *pEncryptedtext,
                                            PLAT_UI16 *pEncryptedtext_length)
{
    Aes aes;
    int ret;
    PLAT_UI16 i;

    if (pPlaintext == NULL || pKey == NULL || pEncryptedtext == NULL ||
        pEncryptedtext_length == NULL) {
        return STSE_PLATFORM_AES_ECB_ENCRYPT_ERROR;
    }

    /* AES ECB requires block-aligned data */
    if ((plaintext_length % AES_BLOCK_SIZE) != 0) {
        return STSE_PLATFORM_AES_ECB_ENCRYPT_ERROR;
    }

    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        return STSE_PLATFORM_AES_ECB_ENCRYPT_ERROR;
    }

    ret = wc_AesSetKey(&aes, pKey, key_length, NULL, AES_ENCRYPTION);
    if (ret != 0) {
        wc_AesFree(&aes);
        return STSE_PLATFORM_AES_ECB_ENCRYPT_ERROR;
    }

    /* ECB mode: encrypt each block independently */
    for (i = 0; i < plaintext_length; i += AES_BLOCK_SIZE) {
        ret = wc_AesEncryptDirect(&aes, pEncryptedtext + i, pPlaintext + i);
        if (ret != 0) {
            wc_AesFree(&aes);
            return STSE_PLATFORM_AES_ECB_ENCRYPT_ERROR;
        }
    }

    *pEncryptedtext_length = plaintext_length;

    wc_AesFree(&aes);
    return STSE_OK;
}

/* ========================================================================== */
/* AES CBC Functions                                                          */
/* ========================================================================== */

stse_ReturnCode_t stse_platform_aes_cbc_enc(const PLAT_UI8 *pPlaintext,
                                            PLAT_UI16 plaintext_length,
                                            PLAT_UI8 *pInitial_value,
                                            const PLAT_UI8 *pKey,
                                            PLAT_UI16 key_length,
                                            PLAT_UI8 *pEncryptedtext,
                                            PLAT_UI16 *pEncryptedtext_length)
{
    Aes aes;
    int ret;

    if (pPlaintext == NULL || pKey == NULL || pEncryptedtext == NULL ||
        pEncryptedtext_length == NULL || pInitial_value == NULL) {
        return STSE_PLATFORM_AES_CBC_ENCRYPT_ERROR;
    }

    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        return STSE_PLATFORM_AES_CBC_ENCRYPT_ERROR;
    }

    ret = wc_AesSetKey(&aes, pKey, key_length, pInitial_value, AES_ENCRYPTION);
    if (ret != 0) {
        wc_AesFree(&aes);
        return STSE_PLATFORM_AES_CBC_ENCRYPT_ERROR;
    }

    ret = wc_AesCbcEncrypt(&aes, pEncryptedtext, pPlaintext, plaintext_length);
    if (ret != 0) {
        wc_AesFree(&aes);
        return STSE_PLATFORM_AES_CBC_ENCRYPT_ERROR;
    }

    *pEncryptedtext_length = plaintext_length;

    wc_AesFree(&aes);
    return STSE_OK;
}

stse_ReturnCode_t stse_platform_aes_cbc_dec(const PLAT_UI8 *pEncryptedtext,
                                            PLAT_UI16 encryptedtext_length,
                                            PLAT_UI8 *pInitial_value,
                                            const PLAT_UI8 *pKey,
                                            PLAT_UI16 key_length,
                                            PLAT_UI8 *pPlaintext,
                                            PLAT_UI16 *pPlaintext_length)
{
    Aes aes;
    int ret;

    if (pEncryptedtext == NULL || pKey == NULL || pPlaintext == NULL ||
        pPlaintext_length == NULL || pInitial_value == NULL) {
        return STSE_PLATFORM_AES_CBC_DECRYPT_ERROR;
    }

    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        return STSE_PLATFORM_AES_CBC_DECRYPT_ERROR;
    }

    ret = wc_AesSetKey(&aes, pKey, key_length, pInitial_value, AES_DECRYPTION);
    if (ret != 0) {
        wc_AesFree(&aes);
        return STSE_PLATFORM_AES_CBC_DECRYPT_ERROR;
    }

    ret = wc_AesCbcDecrypt(&aes, pPlaintext, pEncryptedtext, encryptedtext_length);
    if (ret != 0) {
        wc_AesFree(&aes);
        return STSE_PLATFORM_AES_CBC_DECRYPT_ERROR;
    }

    *pPlaintext_length = encryptedtext_length;

    wc_AesFree(&aes);
    return STSE_OK;
}

/* ========================================================================== */
/* AES CMAC Functions                                                         */
/* ========================================================================== */

stse_ReturnCode_t stse_platform_aes_cmac_init(const PLAT_UI8 *pKey,
                                              PLAT_UI16 key_length,
                                              PLAT_UI16 exp_tag_size)
{
    int ret;
    uint8_t iv[16] = {0};

    /* Free any existing context */
    if (g_cmac_ctx != NULL) {
        ZERO_STRUCTP(g_cmac_ctx);
        free(g_cmac_ctx);
        g_cmac_ctx = NULL;
    }

    /* Allocate new context */
    g_cmac_ctx = (stse_cmac_ctx_t*)malloc(sizeof(stse_cmac_ctx_t));
    if (g_cmac_ctx == NULL) {
        return STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;
    }
    ZERO_STRUCTP(g_cmac_ctx);
    g_cmac_ctx->exp_tag_size = exp_tag_size;

    /* Initialize AES context */
    ret = wc_AesInit(&g_cmac_ctx->aes, NULL, INVALID_DEVID);
    if (ret != 0) {
        free(g_cmac_ctx);
        g_cmac_ctx = NULL;
        return STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;
    }

    ret = wc_AesSetKey(&g_cmac_ctx->aes, pKey, key_length, iv, AES_ENCRYPTION);
    if (ret != 0) {
        wc_AesFree(&g_cmac_ctx->aes);
        free(g_cmac_ctx);
        g_cmac_ctx = NULL;
        return STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;
    }

    /* Generate subkeys K1 and K2 */
    ret = aes_cmac_generate_subkeys(&g_cmac_ctx->aes, g_cmac_ctx->k1, g_cmac_ctx->k2);
    if (ret != 0) {
        wc_AesFree(&g_cmac_ctx->aes);
        free(g_cmac_ctx);
        g_cmac_ctx = NULL;
        return STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;
    }

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_aes_cmac_append(PLAT_UI8 *pInput, PLAT_UI16 length)
{
    uint8_t iv[16] = {0};
    uint8_t tmp_block[16];
    uint8_t Y[16];
    int ret;

    if (g_cmac_ctx == NULL) {
        return STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;
    }

    if (pInput == NULL || length == 0) {
        return STSE_OK;
    }

    /* Copy the remembered last block */
    ZERO_STRUCT(tmp_block);
    if (g_cmac_ctx->last_len > 0) {
        memcpy(tmp_block, g_cmac_ctx->last, g_cmac_ctx->last_len);
    }

    /* Check if we expand the block */
    if (g_cmac_ctx->last_len < 16) {
        size_t len = MIN(16 - g_cmac_ctx->last_len, length);
        memcpy(&tmp_block[g_cmac_ctx->last_len], pInput, len);
        memcpy(g_cmac_ctx->last, tmp_block, 16);
        pInput += len;
        length -= len;
        g_cmac_ctx->last_len += len;
    }

    if (length == 0) {
        /* Still the last block, we are done */
        return STSE_OK;
    }

    /* It is not the last block anymore */
    ZERO_STRUCT(g_cmac_ctx->last);
    g_cmac_ctx->last_len = 0;

    /* Checksum everything but the last block */
    ret = wc_AesSetIV(&g_cmac_ctx->aes, iv);
    if (ret != 0) return STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;

    aes_cmac_xor(g_cmac_ctx->x, tmp_block, Y);
    ret = wc_AesCbcEncrypt(&g_cmac_ctx->aes, g_cmac_ctx->x, Y, 16);
    if (ret != 0) return STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;

    while (length > 16) {
        memcpy(tmp_block, pInput, 16);
        pInput += 16;
        length -= 16;

        aes_cmac_xor(g_cmac_ctx->x, tmp_block, Y);
        ret = wc_AesSetIV(&g_cmac_ctx->aes, iv);
        if (ret != 0) return STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;
        ret = wc_AesCbcEncrypt(&g_cmac_ctx->aes, g_cmac_ctx->x, Y, 16);
        if (ret != 0) return STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;
    }

    /* Copy the last block for processing in finish */
    memcpy(g_cmac_ctx->last, pInput, length);
    g_cmac_ctx->last_len = length;

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_aes_cmac_compute_finish(PLAT_UI8 *pTag, PLAT_UI8 *pTagLen)
{
    uint8_t iv[16] = {0};
    uint8_t tmp_block[16];
    uint8_t Y[16];
    uint8_t out_mac[16];
    int ret;

    if (g_cmac_ctx == NULL || pTag == NULL || pTagLen == NULL) {
        return STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;
    }

    /* Final block processing */
    if (g_cmac_ctx->last_len < 16) {
        /* Incomplete block: pad and XOR with K2 */
        g_cmac_ctx->last[g_cmac_ctx->last_len] = 0x80;
        aes_cmac_xor(g_cmac_ctx->last, g_cmac_ctx->k2, tmp_block);
    } else {
        /* Complete block: XOR with K1 */
        aes_cmac_xor(g_cmac_ctx->last, g_cmac_ctx->k1, tmp_block);
    }

    aes_cmac_xor(tmp_block, g_cmac_ctx->x, Y);
    ret = wc_AesSetIV(&g_cmac_ctx->aes, iv);
    if (ret != 0) {
        goto cleanup;
    }
    ret = wc_AesCbcEncrypt(&g_cmac_ctx->aes, out_mac, Y, 16);
    if (ret != 0) {
        goto cleanup;
    }

    /* Copy requested tag length */
    *pTagLen = (PLAT_UI8)MIN(g_cmac_ctx->exp_tag_size, 16);
    memcpy(pTag, out_mac, *pTagLen);

cleanup:
    /* Clean up */
    wc_AesFree(&g_cmac_ctx->aes);
    ZERO_STRUCTP(g_cmac_ctx);
    free(g_cmac_ctx);
    g_cmac_ctx = NULL;

    return (ret == 0) ? STSE_OK : STSE_PLATFORM_AES_CMAC_COMPUTE_ERROR;
}

stse_ReturnCode_t stse_platform_aes_cmac_verify_finish(PLAT_UI8 *pTag)
{
    uint8_t computed_tag[16];
    PLAT_UI8 tag_len = 16;
    stse_ReturnCode_t ret;

    if (pTag == NULL) {
        return STSE_PLATFORM_AES_CMAC_VERIFY_ERROR;
    }

    ret = stse_platform_aes_cmac_compute_finish(computed_tag, &tag_len);
    if (ret != STSE_OK) {
        return STSE_PLATFORM_AES_CMAC_VERIFY_ERROR;
    }

    /* Constant-time comparison */
    uint8_t diff = 0;
    for (int i = 0; i < tag_len; i++) {
        diff |= computed_tag[i] ^ pTag[i];
    }

    ZERO_STRUCT(computed_tag);

    if (diff != 0) {
        return STSE_PLATFORM_AES_CMAC_VERIFY_ERROR;
    }

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_aes_cmac_compute(const PLAT_UI8 *pPayload,
                                                 PLAT_UI16 payload_length,
                                                 const PLAT_UI8 *pKey,
                                                 PLAT_UI16 key_length,
                                                 PLAT_UI16 exp_tag_size,
                                                 PLAT_UI8 *pTag,
                                                 PLAT_UI16 *pTag_length)
{
    stse_ReturnCode_t ret;
    PLAT_UI8 tag_len;

    ret = stse_platform_aes_cmac_init(pKey, key_length, exp_tag_size);
    if (ret != STSE_OK) {
        return ret;
    }

    ret = stse_platform_aes_cmac_append((PLAT_UI8*)pPayload, payload_length);
    if (ret != STSE_OK) {
        /* Clean up on error */
        if (g_cmac_ctx != NULL) {
            wc_AesFree(&g_cmac_ctx->aes);
            free(g_cmac_ctx);
            g_cmac_ctx = NULL;
        }
        return ret;
    }

    ret = stse_platform_aes_cmac_compute_finish(pTag, &tag_len);
    if (ret == STSE_OK) {
        *pTag_length = tag_len;
    }

    return ret;
}

stse_ReturnCode_t stse_platform_aes_cmac_verify(const PLAT_UI8 *pPayload,
                                                PLAT_UI16 payload_length,
                                                const PLAT_UI8 *pKey,
                                                PLAT_UI16 key_length,
                                                const PLAT_UI8 *pTag,
                                                PLAT_UI16 tag_length)
{
    uint8_t computed_tag[16];
    PLAT_UI16 computed_len = 16;
    stse_ReturnCode_t ret;
    uint8_t diff = 0;

    ret = stse_platform_aes_cmac_compute(pPayload, payload_length, pKey, key_length,
                                         tag_length, computed_tag, &computed_len);
    if (ret != STSE_OK) {
        return STSE_PLATFORM_AES_CMAC_VERIFY_ERROR;
    }

    /* Constant-time comparison */
    for (PLAT_UI16 i = 0; i < MIN(tag_length, computed_len); i++) {
        diff |= computed_tag[i] ^ pTag[i];
    }

    ZERO_STRUCT(computed_tag);

    if (diff != 0) {
        return STSE_PLATFORM_AES_CMAC_VERIFY_ERROR;
    }

    return STSE_OK;
}

#endif /* STSE_CONF_USE_HOST_KEY_ESTABLISHMENT || STSE_CONF_USE_SYMMETRIC_KEY_ESTABLISHMENT || STSE_CONF_USE_HOST_SESSION */

/* ========================================================================== */
/* HMAC-SHA256 Functions (RFC 5869 HKDF)                                      */
/* ========================================================================== */

stse_ReturnCode_t stse_platform_hmac_sha256_compute(PLAT_UI8 *pSalt, PLAT_UI16 salt_length,
                                                    PLAT_UI8 *pInput_keying_material, PLAT_UI16 input_keying_material_length,
                                                    PLAT_UI8 *pInfo, PLAT_UI16 info_length,
                                                    PLAT_UI8 *pOutput_keying_material, PLAT_UI16 output_keying_material_length)
{
    PLAT_UI8 prk[WC_SHA256_DIGEST_SIZE];
    stse_ReturnCode_t ret;

    /* HKDF-Extract */
    ret = stse_platform_hmac_sha256_extract(pSalt, salt_length,
                                            pInput_keying_material, input_keying_material_length,
                                            prk, WC_SHA256_DIGEST_SIZE);
    if (ret != STSE_OK) {
        return ret;
    }

    /* HKDF-Expand */
    ret = stse_platform_hmac_sha256_expand(prk, WC_SHA256_DIGEST_SIZE,
                                           pInfo, info_length,
                                           pOutput_keying_material, output_keying_material_length);

    ZERO_STRUCT(prk);
    return ret;
}

stse_ReturnCode_t stse_platform_hmac_sha256_extract(PLAT_UI8 *pSalt, PLAT_UI16 salt_length,
                                                    PLAT_UI8 *pInput_keying_material, PLAT_UI16 input_keying_material_length,
                                                    PLAT_UI8 *pPseudorandom_key, PLAT_UI16 pseudorandom_key_expected_length)
{
    Hmac hmac;
    int ret;
    uint8_t default_salt[WC_SHA256_DIGEST_SIZE] = {0};

    if (pInput_keying_material == NULL || pPseudorandom_key == NULL) {
        return STSE_PLATFORM_HKDF_ERROR;
    }

    if (pseudorandom_key_expected_length < WC_SHA256_DIGEST_SIZE) {
        return STSE_PLATFORM_HKDF_ERROR;
    }

    /* If salt is NULL or zero-length, use default salt of HashLen zeros */
    if (pSalt == NULL || salt_length == 0) {
        pSalt = default_salt;
        salt_length = WC_SHA256_DIGEST_SIZE;
    }

    ret = wc_HmacInit(&hmac, NULL, INVALID_DEVID);
    if (ret != 0) {
        return STSE_PLATFORM_HKDF_ERROR;
    }

    /* PRK = HMAC-Hash(salt, IKM) */
    ret = wc_HmacSetKey(&hmac, WC_SHA256, pSalt, salt_length);
    if (ret != 0) {
        wc_HmacFree(&hmac);
        return STSE_PLATFORM_HKDF_ERROR;
    }

    ret = wc_HmacUpdate(&hmac, pInput_keying_material, input_keying_material_length);
    if (ret != 0) {
        wc_HmacFree(&hmac);
        return STSE_PLATFORM_HKDF_ERROR;
    }

    ret = wc_HmacFinal(&hmac, pPseudorandom_key);
    wc_HmacFree(&hmac);

    if (ret != 0) {
        return STSE_PLATFORM_HKDF_ERROR;
    }

    return STSE_OK;
}

stse_ReturnCode_t stse_platform_hmac_sha256_expand(PLAT_UI8 *pPseudorandom_key, PLAT_UI16 pseudorandom_key_length,
                                                   PLAT_UI8 *pInfo, PLAT_UI16 info_length,
                                                   PLAT_UI8 *pOutput_keying_material, PLAT_UI16 output_keying_material_length)
{
    Hmac hmac;
    int ret;
    uint8_t T[WC_SHA256_DIGEST_SIZE];
    PLAT_UI16 T_len = 0;
    PLAT_UI16 out_index = 0;
    uint8_t counter = 1;
    PLAT_UI16 N;

    if (pPseudorandom_key == NULL || pOutput_keying_material == NULL) {
        return STSE_PLATFORM_HKDF_ERROR;
    }

    /* RFC 5869: L <= 255*HashLen */
    N = (output_keying_material_length + WC_SHA256_DIGEST_SIZE - 1) / WC_SHA256_DIGEST_SIZE;
    if (N > 255) {
        return STSE_PLATFORM_HKDF_ERROR;
    }

    ret = wc_HmacInit(&hmac, NULL, INVALID_DEVID);
    if (ret != 0) {
        return STSE_PLATFORM_HKDF_ERROR;
    }

    while (out_index < output_keying_material_length) {
        PLAT_UI16 copy_len;

        ret = wc_HmacSetKey(&hmac, WC_SHA256, pPseudorandom_key, pseudorandom_key_length);
        if (ret != 0) break;

        /* T(i) = HMAC-Hash(PRK, T(i-1) | info | i) */
        if (T_len > 0) {
            ret = wc_HmacUpdate(&hmac, T, T_len);
            if (ret != 0) break;
        }

        if (pInfo != NULL && info_length > 0) {
            ret = wc_HmacUpdate(&hmac, pInfo, info_length);
            if (ret != 0) break;
        }

        ret = wc_HmacUpdate(&hmac, &counter, 1);
        if (ret != 0) break;

        ret = wc_HmacFinal(&hmac, T);
        if (ret != 0) break;

        /* Copy to output */
        copy_len = MIN(WC_SHA256_DIGEST_SIZE, output_keying_material_length - out_index);
        memcpy(pOutput_keying_material + out_index, T, copy_len);

        T_len = WC_SHA256_DIGEST_SIZE;
        out_index += WC_SHA256_DIGEST_SIZE;
        counter++;
    }

    wc_HmacFree(&hmac);
    ZERO_STRUCT(T);

    if (ret != 0) {
        memset(pOutput_keying_material, 0, output_keying_material_length);
        return STSE_PLATFORM_HKDF_ERROR;
    }

    return STSE_OK;
}

/* ========================================================================== */
/* NIST Key Wrap (Optional - stub for now)                                    */
/* ========================================================================== */

stse_ReturnCode_t stse_platform_nist_kw_encrypt(PLAT_UI8 *pPayload, PLAT_UI32 payload_length,
                                                PLAT_UI8 *pKey, PLAT_UI8 key_length,
                                                PLAT_UI8 *pOutput, PLAT_UI32 *pOutput_length)
{
    (void)pPayload;
    (void)payload_length;
    (void)pKey;
    (void)key_length;
    (void)pOutput;
    (void)pOutput_length;

    /* TODO: Implement using wc_AesKeyWrap if needed */
    return STSE_PLATFORM_AES_ECB_ENCRYPT_ERROR;
}

