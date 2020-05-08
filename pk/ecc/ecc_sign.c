/* ecc_sign.c
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

/* Example to demonstrate SHA-256 hashing and ECC Sign */
/* 
./configure CFLAGS="-DWOLFSSL_PUBLIC_MP" && make && sudo make install
gcc -lwolfssl -o ecc_sign ecc_sign.c
*/

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/integer.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#include <stdint.h>
#include <stdio.h>

#define ECC_CURVE_SZ 32 /* SECP256R1 curve size in bytes */
#define ECC_CURVE_ID ECC_SECP256R1

#ifdef WOLFSSL_PUBLIC_MP

/* Test Vector */
/* NIST P-256, SHA256 ECC Test Vector */
static const uint8_t kMsg[] = {
    0x69, 0xbc, 0x9f, 0xce, 0x68, 0x17, 0xc2, 0x10, 0xea, 0xfc, 0x10, 0x65, 0x67, 0x52, 0xed, 0x78, 
    0x6e, 0xb8, 0x83, 0x9c, 0x9a, 0xb4, 0x56, 0x0d, 0xc1, 0x0d, 0x1f, 0x78, 0x6e, 0x75, 0xd7, 0xbe, 
    0x92, 0x6b, 0x12, 0xf6, 0x76, 0x60, 0x8e, 0xb1, 0xf4, 0x19, 0x0c, 0x81, 0xe7, 0x54, 0x5e, 0xbc, 
    0xe0, 0xae, 0xc2, 0x7d, 0x1b, 0xc4, 0x6e, 0xec, 0xb1, 0x99, 0x6c, 0xbf, 0x0e, 0x38, 0xa8, 0x01, 
    0xa6, 0x9a, 0x48, 0x12, 0xe4, 0xc9, 0x3b, 0xf0, 0x63, 0x46, 0x15, 0xb4, 0x61, 0xa8, 0x1a, 0x60, 
    0x71, 0x87, 0x98, 0xd7, 0x6f, 0x98, 0x7b, 0x2d, 0xb9, 0x19, 0x1b, 0x21, 0x9c, 0x70, 0x58, 0xe8, 
    0x0d, 0x0f, 0xe9, 0x2d, 0x9a, 0x9a, 0xf1, 0x55, 0xa0, 0x4c, 0xd3, 0x07, 0xbd, 0x97, 0x48, 0xec, 
    0x88, 0x0a, 0xaf, 0xb3, 0x80, 0x78, 0xa4, 0x59, 0x43, 0x57, 0xd3, 0xa7, 0x01, 0x66, 0x0e, 0xfc
};
static const uint8_t kPrivKey[] = {
    /* d */
    0x1e, 0xe7, 0x70, 0x07, 0xd3, 0x30, 0x94, 0x39, 
    0x28, 0x90, 0xdf, 0x23, 0x88, 0x2c, 0x4a, 0x34, 
    0x15, 0xdb, 0x4c, 0x43, 0xcd, 0xfa, 0xe5, 0x1f, 
    0x3d, 0x4c, 0x37, 0xfe, 0x59, 0x3b, 0x96, 0xd8
};
static const uint8_t kPubKey[] = {
    /* Qx */
    0x96, 0x93, 0x1c, 0x53, 0x0b, 0x43, 0x6c, 0x42, 
    0x0c, 0x52, 0x90, 0xe4, 0xa7, 0xec, 0x98, 0xb1, 
    0xaf, 0xd4, 0x14, 0x49, 0xd8, 0xc1, 0x42, 0x82, 
    0x04, 0x78, 0xd1, 0x90, 0xae, 0xa0, 0x6c, 0x07, 
    /* Qy */
    0xf2, 0x3a, 0xb5, 0x10, 0x32, 0x8d, 0xce, 0x9e, 
    0x76, 0xa0, 0xd2, 0x8c, 0xf3, 0xfc, 0xa9, 0x94, 
    0x43, 0x24, 0xe6, 0x82, 0x00, 0x40, 0xc6, 0xdb, 
    0x1c, 0x2f, 0xcd, 0x38, 0x4b, 0x60, 0xdd, 0x61
};


#ifndef NO_SHA256
/* perform hashing block by block */
int crypto_sha256(const uint8_t *buf, uint32_t len, uint8_t *hash, 
    uint32_t hashSz, uint32_t blkSz)
{
    int ret;
    uint32_t i = 0, chunk;
    wc_Sha256 sha256;
    
    /* validate arguments */
    if ((buf == NULL && len > 0) || hash == NULL || 
        hashSz < WC_SHA256_DIGEST_SIZE || blkSz == 0)
    {
        return BAD_FUNC_ARG;
    }

    /* Init Sha256 structure */
    ret = wc_InitSha256(&sha256);
    if (ret != 0) {
        return ret;
    }
    while (i < len) {
        chunk = blkSz;
        if ((chunk + i) > len)
            chunk = len - i;
        /* Perform chunked update */
        ret = wc_Sha256Update(&sha256, (buf + i), chunk);
        if (ret != 0) {
            break;
        }
        i += chunk;
    }
    if (ret == 0) {
        /* Get final digest result */
        ret = wc_Sha256Final(&sha256, hash);
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
    !defined(NO_SHA256) && defined(WOLFSSL_PUBLIC_MP)
    int ret;
    uint8_t hash[WC_SHA256_DIGEST_SIZE];
    uint8_t sig[ECC_CURVE_SZ*2];
    uint32_t sigSz = 0;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    printf("Running NIST P-256,SHA-256 Sign Test\n");

    memset(sig, 0, sizeof(sig));

    ret = crypto_sha256(
        kMsg, sizeof(kMsg), /* input message */
        hash, sizeof(hash), /* hash digest result */
        32                  /* configurable block / chunk size */
    );
    if (ret == 0) {
        /* Sign hash using private key */
        /* Note: result of an ECC sign varies for each call even with same 
            private key and hash. This is because a new random public key is 
            used for each operation. */ 
        sigSz = sizeof(sig);
        ret = crypto_ecc_sign(
            kPrivKey, sizeof(kPrivKey), /* private key */
            hash, sizeof(hash),         /* computed hash digest */
            sig, &sigSz,                /* signature r/s */
            ECC_CURVE_SZ,               /* SECP256R1 curve size in bytes */
            ECC_CURVE_ID                /* curve id */
        );
    }

    if (ret == 0) {
        /* Verify generated signature is valid */
        ret = crypto_ecc_verify(
            kPubKey, sizeof(kPubKey),   /* public key point x/y */
            hash, sizeof(hash),         /* computed hash digest */
            sig, sigSz,                 /* signature r/s */
            ECC_CURVE_SZ,               /* curve size in bytes */
            ECC_CURVE_ID                /* curve id */
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
    printf("wolfSSL requires ECC and SHA256 and WOLFSSL_PUBLIC_MP\n");
    return -1;
#endif
}
