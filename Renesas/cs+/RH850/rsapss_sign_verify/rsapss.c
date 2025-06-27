/* rsapss.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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
#include "rh_string.h"
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/signature.h>

/* enable one of two */
#define USE_RSA2048
/*#define USE_RSA3072*/

#if defined(USE_RSA2048)
    #include "rsa_private_2048.c"
    /* Key size in bits. */
    #define RSA_KEY_SIZE 2048
#else
    #include "rsa_private_3072.c"
    /* Key size in bits. */
    #define RSA_KEY_SIZE 3072
#endif



byte hash[WC_SHA256_DIGEST_SIZE];
byte pSignature[RSA_KEY_SIZE/8];

static int hash_msg(const char* msg, byte* hash)
{
    wc_Sha256 sha256;
    int ret;
    
    printf("Hashing message: %s\n", msg);
    
    /* Initialize hash for use */
    ret = wc_InitSha256(&sha256);
    if (ret < 0) {
        printf("    error initializing SHA-256 hash", ret);
        goto hash_end;
    }
    
    /* Hash the message string. */
    ret = wc_Sha256Update(&sha256, (unsigned char*)msg, XSTRLEN(msg));
    if (ret < 0) {
        printf("  error SHA-256 hashing msg ret=%d\n", ret);
        goto hash_end;
    }
    
    /* Generate hash value */
    ret = wc_Sha256Final(&sha256, hash);
    if (ret < 0) {
        printf("  error creating SHA-256 hash ret=%d\n", ret);
        goto hash_end;
    }
    
    ret = 0;
hash_end:
    return ret;
}
/* load rsa private key */
static int load_rsa_private_key(RsaKey* pRsaKey)
{
    int ret;
    word32 idx = 0;
    
    /* Read Private key from pem format */
    const byte* keyData = 
        #if defined(USE_RSA2048)
            (const byte*)rsa_private_2048;
        #else
            (const byte*)rsa_private_3072;
        #endif
    
    word32 keySz = 
        #if defined(USE_RSA2048)
            sizeof_rsa_private_2048;
        #else
            sizeof_rsa_private_3072;
        #endif

    if ((ret = wc_RsaPrivateKeyDecode(keyData, &idx, pRsaKey, keySz)) != 0) {
        wc_FreeRsaKey(pRsaKey);
        ret = -1;
    }
    
    return ret;
}

static int sign_with_rsa_key(RsaKey* pRsaKey, WC_RNG* rng, const char* msg)
{
    int ret;
    
    /* Hash message to b signed */
    if (hash_msg(msg, hash) != 0 ) {
        goto sign_end;
    }
    
    printf("Signing hash of message\n");
    /* RSA-PSS sign */
    ret = wc_RsaPSS_Sign(hash, sizeof(hash), pSignature, sizeof(pSignature),
        WC_HASH_TYPE_SHA256, WC_MGF1SHA256, pRsaKey, rng);
    if (ret <= 0) {
        printf("  RSA private encryption failed with error %d\n");
        goto sign_end;
    }
    
    ret = 0;
sign_end:
    return ret;
}

static int verify_with_rsa_public_key(RsaKey* pRsaKey, const char* msg)
{
    int ret;
    int sz = sizeof(pSignature);
    byte  pDecrypted[RSA_KEY_SIZE/8];
    byte* pt;
    /* Hash message to be signed. */
    if (hash_msg(msg, hash) != 0) {
        goto verify_end;
    }
    
    printf("Verify hash of message\n");

    /* Verify hash against signature with RSA public key. */
    pt = pDecrypted;
    /* Verify the signature decrypts. */
    ret = wc_RsaPSS_VerifyInline(pSignature, sz, &pt,
        WC_HASH_TYPE_SHA256, WC_MGF1SHA256, pRsaKey);
    if (ret < 0) {
        printf("  error verify signature ret=%d\n", ret);
        goto verify_end;
    }
     sz = ret;

    /* Check PSS padding on decrypted signature. */
    ret = wc_RsaPSS_CheckPadding(hash, sizeof(hash), pt, sz,
        WC_HASH_TYPE_SHA256);
    if (ret < 0) {
        printf("  error checking padding ret=%d\n", ret);
        goto verify_end;
    }

    printf("RSA PSS verify success\n");
    
verify_end:
    return ret;
}

int rsapss_sign_verify()
{
    RsaKey RsaKey;
    WC_RNG rng;
    const char* msg = "This is the string to be signed";
    int ret = 0;
    
    /* Initialize wolfCrypt for operations. */
    wolfCrypt_Init();
    
     /* Initialize an random number generator for key generation and signing. */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Init RNG failed ret = %d\n", ret);
        goto func_end;
    }
    
    /* Initialize RSA key. */
    ret = wc_InitRsaKey(&RsaKey, NULL);
    if (ret != 0) {
        printf("Init RSA key failed ret=%d\n", ret);
        goto func_end;
    }
    
    /* Set the random number generator against RSA key for signing. */
    ret = wc_RsaSetRNG(&RsaKey, &rng);
    if (ret != 0) {
        printf("Set RSA RNG failed ret=%d\n", ret);
        goto func_end;
    }
    
    /* Load the RSA private key including Public */
    if (load_rsa_private_key(&RsaKey) != 0) {
        ret = -1;
        goto func_end;
    }
    
    /* Sign the message with the RSA private key. */
    if (sign_with_rsa_key(&RsaKey, &rng, msg) != 0) {
        ret = -1;
        goto func_end;
    }
    /* Verify the message with the RSA public key. */
    if (verify_with_rsa_public_key(&RsaKey, msg) != 0) {
        ret = -1;
        goto func_end;
    }
    
func_end:
    /* Zeroize and free any internally allocated data in RSA key. */
    wc_FreeRsaKey(&RsaKey);
    /* Free any internally allocated data in random number generator. */
    wc_FreeRng(&rng);

    /* Cleanup wolfCrypt after operations. */
    wolfCrypt_Cleanup();
    return ret;
}

