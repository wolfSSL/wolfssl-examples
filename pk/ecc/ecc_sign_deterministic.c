/* ecc_sign_deterministic.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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

/* Example to demonstrate SHA-256 hashing and ECC Sign */
/*
./configure CFLAGS="-DWOLFSSL_PUBLIC_MP" && make && sudo make install
gcc -lwolfssl -o ecc_sign ecc_sign.c
*/

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/sha.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/integer.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/hash.h>

#include <stdint.h>
#include <stdio.h>

#ifdef WOLFSSL_PUBLIC_MP

/* message = "sample":  */
static const char kMsg[] = "sample";
#define DIGEST_SZ WC_SHA256_DIGEST_SIZE
#define HASH_FUNC crypto_sha256

#if 0

    /* SECP256R1 */
    #define ECC_KEY_CURVE ECC_SECP256R1
    #define ECC_KEY_SIZE  32

    /* Test Vector: ECDSA, 256 Bits (Prime Field) */
    static const uint8_t kPrivKey[] = {
        /* d */
        0xC9, 0xAF, 0xA9, 0xD8, 0x45, 0xBA, 0x75, 0x16,
        0x6B, 0x5C, 0x21, 0x57, 0x67, 0xB1, 0xD6, 0x93,
        0x4E, 0x50, 0xC3, 0xDB, 0x36, 0xE8, 0x9B, 0x12,
        0x7B, 0x8A, 0x62, 0x2B, 0x12, 0x0F, 0x67, 0x21
    };
    static const uint8_t kPubKey[] = {
        /* Qx */
        0x60, 0xFE, 0xD4, 0xBA, 0x25, 0x5A, 0x9D, 0x31,
        0xC9, 0x61, 0xEB, 0x74, 0xC6, 0x35, 0x6D, 0x68,
        0xC0, 0x49, 0xB8, 0x92, 0x3B, 0x61, 0xFA, 0x6C,
        0xE6, 0x69, 0x62, 0x2E, 0x60, 0xF2, 0x9F, 0xB6,
        /* Qy */
        0x79, 0x03, 0xFE, 0x10, 0x08, 0xB8, 0xBC, 0x99,
        0xA4, 0x1A, 0xE9, 0xE9, 0x56, 0x28, 0xBC, 0x64,
        0xF2, 0xF1, 0xB2, 0x0C, 0x2D, 0x7E, 0x9F, 0x51,
        0x77, 0xA3, 0xC2, 0x94, 0xD4, 0x46, 0x22, 0x99
    };

#elif 0

    /* SECP384R1 */
    #define ECC_KEY_CURVE ECC_SECP384R1
    #define ECC_KEY_SIZE  48

    /* Test Vector: ECDSA, 384 Bits (Prime Field) */
    static const uint8_t kPrivKey[] = {
        /* d */
        0x6B, 0x9D, 0x3D, 0xAD, 0x2E, 0x1B, 0x8C, 0x1C,
        0x05, 0xB1, 0x98, 0x75, 0xB6, 0x65, 0x9F, 0x4D,
        0xE2, 0x3C, 0x3B, 0x66, 0x7B, 0xF2, 0x97, 0xBA,
        0x9A, 0xA4, 0x77, 0x40, 0x78, 0x71, 0x37, 0xD8,
        0x96, 0xD5, 0x72, 0x4E, 0x4C, 0x70, 0xA8, 0x25,
        0xF8, 0x72, 0xC9, 0xEA, 0x60, 0xD2, 0xED, 0xF5
    };
    static const uint8_t kPubKey[] = {
        /* Qx */
        0xEC, 0x3A, 0x4E, 0x41, 0x5B, 0x4E, 0x19, 0xA4,
        0x56, 0x86, 0x18, 0x02, 0x9F, 0x42, 0x7F, 0xA5,
        0xDA, 0x9A, 0x8B, 0xC4, 0xAE, 0x92, 0xE0, 0x2E,
        0x06, 0xAA, 0xE5, 0x28, 0x6B, 0x30, 0x0C, 0x64,
        0xDE, 0xF8, 0xF0, 0xEA, 0x90, 0x55, 0x86, 0x60,
        0x64, 0xA2, 0x54, 0x51, 0x54, 0x80, 0xBC, 0x13,
        /* Qy */
        0x80, 0x15, 0xD9, 0xB7, 0x2D, 0x7D, 0x57, 0x24,
        0x4E, 0xA8, 0xEF, 0x9A, 0xC0, 0xC6, 0x21, 0x89,
        0x67, 0x08, 0xA5, 0x93, 0x67, 0xF9, 0xDF, 0xB9,
        0xF5, 0x4C, 0xA8, 0x4B, 0x3F, 0x1C, 0x9D, 0xB1,
        0x28, 0x8B, 0x23, 0x1C, 0x3A, 0xE0, 0xD4, 0xFE,
        0x73, 0x44, 0xFD, 0x25, 0x33, 0x26, 0x47, 0x20
    };

#elif 1

    /* SECP521R1 */
    #define ECC_KEY_CURVE ECC_SECP521R1
    #define ECC_KEY_SIZE  66

    /* Test Vector: ECDSA, 521 Bits (Prime Field) */
    static const uint8_t kPrivKey[] = {
        /* d */
        0x0F, 0xAD, 0x06, 0xDA, 0xA6, 0x2B, 0xA3, 0xB2, 0x5D, 0x2F, 0xB4,
        0x01, 0x33, 0xDA, 0x75, 0x72, 0x05, 0xDE, 0x67, 0xF5, 0xBB, 0x00,
        0x18, 0xFE, 0xE8, 0xC8, 0x6E, 0x1B, 0x68, 0xC7, 0xE7, 0x5C, 0xAA,
        0x89, 0x6E, 0xB3, 0x2F, 0x1F, 0x47, 0xC7, 0x08, 0x55, 0x83, 0x6A,
        0x6D, 0x16, 0xFC, 0xC1, 0x46, 0x6F, 0x6D, 0x8F, 0xBE, 0xC6, 0x7D,
        0xB8, 0x9E, 0xC0, 0xC0, 0x8B, 0x0E, 0x99, 0x6B, 0x83, 0x53, 0x08
    };
    static const uint8_t kPubKey[] = {
        /* Qx */
        0x18, 0x94, 0x55, 0x0D, 0x07, 0x85, 0x93, 0x2E, 0x00, 0xEA, 0xA2,
        0x3B, 0x69, 0x4F, 0x21, 0x3F, 0x8C, 0x31, 0x21, 0xF8, 0x6D, 0xC9,
        0x7A, 0x04, 0xE5, 0xA7, 0x16, 0x7D, 0xB4, 0xE5, 0xBC, 0xD3, 0x71,
        0x12, 0x3D, 0x46, 0xE4, 0x5D, 0xB6, 0xB5, 0xD5, 0x37, 0x0A, 0x7F,
        0x20, 0xFB, 0x63, 0x31, 0x55, 0xD3, 0x8F, 0xFA, 0x16, 0xD2, 0xBD,
        0x76, 0x1D, 0xCA, 0xC4, 0x74, 0xB9, 0xA2, 0xF5, 0x02, 0x3A, 0x04,
        /* Qy */
        0x04, 0x93, 0x10, 0x1C, 0x96, 0x2C, 0xD4, 0xD2, 0xFD, 0xDF, 0x78,
        0x22, 0x85, 0xE6, 0x45, 0x84, 0x13, 0x9C, 0x2F, 0x91, 0xB4, 0x7F,
        0x87, 0xFF, 0x82, 0x35, 0x4D, 0x66, 0x30, 0xF7, 0x46, 0xA2, 0x8A,
        0x0D, 0xB2, 0x57, 0x41, 0xB5, 0xB3, 0x4A, 0x82, 0x80, 0x08, 0xB2,
        0x2A, 0xCC, 0x23, 0xF9, 0x24, 0xFA, 0xAF, 0xBD, 0x4D, 0x33, 0xF8,
        0x1E, 0xA6, 0x69, 0x56, 0xDF, 0xEA, 0xA2, 0xBF, 0xDF, 0xCF, 0x05
    };

#endif

#ifndef NO_SHA256
/* perform hashing block by block */
int crypto_sha256(const uint8_t *buf, uint32_t len, uint8_t *hash,
    uint32_t hashSz, uint32_t blkSz)
{
    int ret;
    uint32_t i = 0, chunk;
    wc_Sha256 sha;

    /* validate arguments */
    if ((buf == NULL && len > 0) || hash == NULL ||
        hashSz < WC_SHA256_DIGEST_SIZE || blkSz == 0)
    {
        return BAD_FUNC_ARG;
    }

    /* Init Sha256 structure */
    ret = wc_InitSha256(&sha);
    if (ret != 0) {
        return ret;
    }
    while (i < len) {
        chunk = blkSz;
        if ((chunk + i) > len)
            chunk = len - i;
        /* Perform chunked update */
        ret = wc_Sha256Update(&sha, (buf + i), chunk);
        if (ret != 0) {
            break;
        }
        i += chunk;
    }
    if (ret == 0) {
        /* Get final digest result */
        ret = wc_Sha256Final(&sha, hash);
    }
    return ret;
}
#endif

#ifdef WOLFSSL_SHA384
/* perform hashing block by block */
int crypto_sha384(const uint8_t *buf, uint32_t len, uint8_t *hash,
    uint32_t hashSz, uint32_t blkSz)
{
    int ret;
    uint32_t i = 0, chunk;
    wc_Sha384 sha;

    /* validate arguments */
    if ((buf == NULL && len > 0) || hash == NULL ||
        hashSz < WC_SHA384_DIGEST_SIZE || blkSz == 0)
    {
        return BAD_FUNC_ARG;
    }

    /* Init structure */
    ret = wc_InitSha384(&sha);
    if (ret != 0) {
        return ret;
    }
    while (i < len) {
        chunk = blkSz;
        if ((chunk + i) > len)
            chunk = len - i;
        /* Perform chunked update */
        ret = wc_Sha384Update(&sha, (buf + i), chunk);
        if (ret != 0) {
            break;
        }
        i += chunk;
    }
    if (ret == 0) {
        /* Get final digest result */
        ret = wc_Sha384Final(&sha, hash);
    }
    return ret;
}
#endif

#ifdef WOLFSSL_SHA512
/* perform hashing block by block */
int crypto_sha512(const uint8_t *buf, uint32_t len, uint8_t *hash,
    uint32_t hashSz, uint32_t blkSz)
{
    int ret;
    uint32_t i = 0, chunk;
    wc_Sha512 sha;

    /* validate arguments */
    if ((buf == NULL && len > 0) || hash == NULL ||
        hashSz < WC_SHA512_DIGEST_SIZE || blkSz == 0)
    {
        return BAD_FUNC_ARG;
    }

    /* Init structure */
    ret = wc_InitSha512(&sha);
    if (ret != 0) {
        return ret;
    }
    while (i < len) {
        chunk = blkSz;
        if ((chunk + i) > len)
            chunk = len - i;
        /* Perform chunked update */
        ret = wc_Sha512Update(&sha, (buf + i), chunk);
        if (ret != 0) {
            break;
        }
        i += chunk;
    }
    if (ret == 0) {
        /* Get final digest result */
        ret = wc_Sha512Final(&sha, hash);
    }
    return ret;
}
#endif

#ifdef HAVE_ECC
#ifdef HAVE_ECC_VERIFY
/* perform verify of signature and hash using public key */
/* key is public Qx + public Qy */
/* sig is r + s */
int crypto_ecc_verify(const uint8_t *key, uint32_t keySz,
    const uint8_t *hash, uint32_t hashSz, const uint8_t *sig, uint32_t sigSz,
    int curveSz, int curveId)
{
    int ret, verify_res = 0;
    mp_int r, s;
    ecc_key ecc;

    /* validate arguments */
    if (key == NULL || hash == NULL || sig == NULL || curveSz == 0 ||
        hashSz == 0 || keySz < (curveSz*2) || sigSz < (curveSz*2))
    {
        return BAD_FUNC_ARG;
    }

    /* Setup the ECC key */
    ret = wc_ecc_init(&ecc);
    if (ret < 0) {
        return ret;
    }

    /* Setup the signature r/s variables */
    ret = mp_init(&r);
    if (ret != MP_OKAY) {
        wc_ecc_free(&ecc);
        return ret;
    }
    ret = mp_init(&s);
    if (ret != MP_OKAY) {
        mp_clear(&r);
        wc_ecc_free(&ecc);
        return ret;
    }

    /* Import public key x/y */
    ret = wc_ecc_import_unsigned(
        &ecc,
        (byte*)key,             /* Public "x" Coordinate */
        (byte*)(key + curveSz), /* Public "y" Coordinate */
        NULL,                   /* Private "d" (optional) */
        curveId                 /* ECC Curve Id */
    );
    /* Make sure it was a public key imported */
    if (ret == 0 && ecc.type != ECC_PUBLICKEY) {
        ret = ECC_BAD_ARG_E;
    }

    /* Import signature r/s */
    if (ret == 0) {
        ret = mp_read_unsigned_bin(&r, sig,  curveSz);
    }
    if (ret == 0) {
        ret = mp_read_unsigned_bin(&s, sig + curveSz, curveSz);
    }

    /* Verify ECC Signature */
    if (ret == 0) {
        ret = wc_ecc_verify_hash_ex(
            &r, &s,       /* r/s as mp_int */
            hash, hashSz, /* computed hash digest */
            &verify_res,  /* verification result 1=success */
            &ecc
        );
    }

    /* check verify result */
    if (ret == 0 && verify_res == 0) {
        ret = SIG_VERIFY_E;
    }

    mp_clear(&r);
    mp_clear(&s);
    wc_ecc_free(&ecc);

    return ret;
}
#endif /* HAVE_ECC_VERIFY */

#ifdef HAVE_ECC_SIGN
/* perform signature operation against hash using private key */
int crypto_ecc_sign(const uint8_t *key, uint32_t keySz,
    const uint8_t *hash, uint32_t hashSz, uint8_t *sig, uint32_t* sigSz,
    int curveSz, int curveId)
{
    int ret;
    mp_int r, s;
    ecc_key ecc;
    WC_RNG rng;

    /* validate arguments */
    if (key == NULL || hash == NULL || sig == NULL || sigSz == NULL ||
        curveSz == 0 || hashSz == 0 || keySz < curveSz || *sigSz < (curveSz*2))
    {
        return BAD_FUNC_ARG;
    }

    /* Initialize signature result */
    memset(sig, 0, curveSz*2);

    /* Setup the RNG */
    ret = wc_InitRng(&rng);
    if (ret < 0) {
        return ret;
    }

    /* Setup the ECC key */
    ret = wc_ecc_init(&ecc);
    if (ret < 0) {
        wc_FreeRng(&rng);
        return ret;
    }

    /* enable deterministic signing */
    /* result will always be the same, no random point is used */
    wc_ecc_set_deterministic(&ecc, 1);

    /* Setup the signature r/s variables */
    ret = mp_init(&r);
    if (ret != MP_OKAY) {
        wc_ecc_free(&ecc);
        wc_FreeRng(&rng);
        return ret;
    }
    ret = mp_init(&s);
    if (ret != MP_OKAY) {
        mp_clear(&r);
        wc_ecc_free(&ecc);
        wc_FreeRng(&rng);
        return ret;
    }

    /* Import private key "k" */
    ret = wc_ecc_import_private_key_ex(
        key, keySz, /* private key "d" */
        NULL, 0,    /* public (optional) */
        &ecc,
        curveId     /* ECC Curve Id */
    );
    if (ret == 0) {
        /* Verify ECC Signature */
        ret = wc_ecc_sign_hash_ex(
            hash, hashSz, /* computed hash digest */
            &rng, &ecc,   /* random and key context */
            &r, &s        /* r/s as mp_int */
        );

        /* export r/s */
        mp_to_unsigned_bin(&r, sig);
        mp_to_unsigned_bin(&s, sig + curveSz);
    }

    mp_clear(&r);
    mp_clear(&s);
    wc_ecc_free(&ecc);
    wc_FreeRng(&rng);

    return ret;
}
#endif /* HAVE_ECC_SIGN */
#endif /* HAVE_ECC */

static void print_hex(uint8_t* data, int sz)
{
    int i;
    for (i = 0; i < sz; i++) {
        printf("%02X ", data[i]);
        if (i > 0 && ((i+1) % 16) == 0)
            printf("\n");
    }
    printf("\n");
}

#endif /* WOLFSSL_PUBLIC_MP */

int main()
{
#if defined(HAVE_ECC) && defined(HAVE_ECC_SIGN) && defined(HAVE_ECC_VERIFY) && \
    defined(WOLFSSL_SHA512) && defined(WOLFSSL_PUBLIC_MP)
    int ret;
    uint8_t hash[DIGEST_SZ];
    uint8_t sig[ECC_KEY_SIZE*2];
    uint32_t sigSz = 0;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

if (ECC_KEY_SIZE == 66)
    printf("Running NIST P-%d,SHA-%d Deterministic Sign Test\n", (ECC_KEY_SIZE*8)-7, DIGEST_SZ*8);
else
    printf("Running NIST P-%d,SHA-%d Deterministic Sign Test\n", (ECC_KEY_SIZE*8), DIGEST_SZ*8);

    memset(sig, 0, sizeof(sig));

    ret = HASH_FUNC(
        (uint8_t*)kMsg, sizeof(kMsg)-1, /* input message sizeof()-1 to get actual length without null termination */
        hash, sizeof(hash), /* hash digest result */
        32                  /* configurable block / chunk size */
    );
    if (ret == 0) {
        printf("Digest %d\n", (int)sizeof(hash));
        print_hex(hash, sizeof(hash));

        /* Sign hash using private key */
        /* Note: result of an ECC sign varies for each call even with same
            private key and hash. This is because a new random public key is
            used for each operation. */
        sigSz = sizeof(sig);
        ret = crypto_ecc_sign(
            kPrivKey, sizeof(kPrivKey), /* private key */
            hash, sizeof(hash),         /* computed hash digest */
            sig, &sigSz,                /* signature r/s */
            ECC_KEY_SIZE,               /* SECP256R1 curve size in bytes */
            ECC_KEY_CURVE               /* curve id */
        );
    }

    if (ret == 0) {
        /* Verify generated signature is valid */
        ret = crypto_ecc_verify(
            kPubKey, sizeof(kPubKey),   /* public key point x/y */
            hash, sizeof(hash),         /* computed hash digest */
            sig, sigSz,                 /* signature r/s */
            ECC_KEY_SIZE,               /* curve size in bytes */
            ECC_KEY_CURVE               /* curve id */
        );
    }

    if (ret == 0) {
        printf("Signature %d\n", sigSz);
        print_hex(sig, sigSz);

        printf("Success\n");
    }
    else {
        printf("Failure %d: %s\n", ret, wc_GetErrorString(ret));
        ret = -1;
    }

    return ret;
#else
    printf("wolfSSL requires ECC and SHA512 and WOLFSSL_PUBLIC_MP\n");
    return -1;
#endif
}