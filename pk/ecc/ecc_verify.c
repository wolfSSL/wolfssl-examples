/* ecc_verify.c
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

/* Example to demonstrate SHA-256 hashing and ECC Verify */
/* 
./configure CFLAGS="-DWOLFSSL_PUBLIC_MP" && make && sudo make install
gcc -lwolfssl -o ecc_verify ecc_verify.c
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

#ifdef WOLFSSL_PUBLIC_MP

#define ECC_CURVE_SZ 32 /* SECP256R1 curve size in bytes */
#define ECC_CURVE_ID ECC_SECP256R1

/* Test Vectors */
/* NIST P-256, SHA256 ECC Test Vector */
static const uint8_t kMsg[] = {
    0xba, 0xc1, 0x87, 0x53, 0x80, 0x9b, 0x27, 0xad, 0xa2, 0xb9, 0x4f, 0xd6, 0xac, 0x75, 0x39, 0xee, 
    0x12, 0x95, 0xe2, 0xec, 0xf3, 0x80, 0x25, 0x8e, 0x89, 0x42, 0xb6, 0xf0, 0xeb, 0xbc, 0xe1, 0xaa, 
    0xc9, 0x80, 0xfb, 0x0f, 0xe9, 0x36, 0xf9, 0x5d, 0xd1, 0x75, 0xc0, 0x27, 0x2d, 0x56, 0x07, 0x6a, 
    0x50, 0xab, 0xd8, 0x26, 0x24, 0x8a, 0x6e, 0xab, 0xc9, 0xf1, 0x06, 0x6f, 0x65, 0x42, 0xe1, 0xdb, 
    0xc7, 0x52, 0x1e, 0x3b, 0x0a, 0xfa, 0xc9, 0x74, 0x82, 0x39, 0x16, 0xfa, 0x2d, 0x8a, 0x23, 0x06, 
    0x42, 0x97, 0x84, 0x8c, 0x3d, 0x93, 0x96, 0x84, 0x1d, 0xea, 0x53, 0x34, 0x3a, 0xf2, 0x5f, 0xbc, 
    0xb1, 0x4d, 0x84, 0x85, 0xd5, 0x6c, 0xe3, 0x61, 0xce, 0x2a, 0x34, 0x47, 0x1c, 0x74, 0xeb, 0x7c, 
    0x11, 0x20, 0x1d, 0x7f, 0x22, 0xc0, 0x9b, 0x8c, 0x90, 0xff, 0x32, 0x1a, 0xf0, 0xbd, 0xa0, 0xdc
};
static const uint8_t kPubKey[] = {
    /* Qx */
    0x65, 0x6f, 0xfd, 0x63, 0x85, 0x88, 0x89, 0x35, 
    0x68, 0xe4, 0xa2, 0xdc, 0x72, 0x10, 0xdb, 0xdd, 
    0x4a, 0x9e, 0xef, 0x51, 0xfe, 0x04, 0xfb, 0xf8, 
    0x7c, 0xbb, 0x1a, 0x0d, 0x6f, 0xdc, 0x89, 0xec,
    /* Qy */
    0x37, 0x1e, 0x4e, 0x7f, 0x82, 0x75, 0x14, 0xb9, 
    0x17, 0x38, 0x40, 0xe3, 0x18, 0x34, 0xd4, 0x55, 
    0xc1, 0x44, 0x10, 0x30, 0xad, 0x77, 0x8a, 0x46, 
    0x2d, 0xc2, 0xff, 0x94, 0x98, 0xad, 0x1e, 0xd8
};
static const uint8_t kSigRS[] = {
    /* R */
    0x60, 0x7e, 0x20, 0xf0, 0x62, 0xa2, 0x9b, 0xa7, 
    0xf0, 0x93, 0x34, 0xc1, 0x14, 0xd4, 0x9f, 0x79, 
    0x4d, 0xb3, 0xf4, 0xf4, 0x6a, 0x79, 0xb0, 0x1a, 
    0x0b, 0x39, 0x20, 0x4d, 0x2d, 0x1f, 0x04, 0x30,
    /* S */
    0x9e, 0xb5, 0x1b, 0xfe, 0xa4, 0x7c, 0x14, 0x0d, 
    0xfd, 0x71, 0x5f, 0xfd, 0x52, 0x3b, 0xdb, 0xe7, 
    0x70, 0xa2, 0xd6, 0x29, 0x40, 0x74, 0xa9, 0xaa, 
    0x4f, 0x36, 0x46, 0xea, 0x06, 0x01, 0xe9, 0xcf
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
#endif /* HAVE_ECC */
#endif /* WOLFSSL_PUBLIC_MP*/

int main()
{
#if defined(HAVE_ECC) && defined(HAVE_ECC_VERIFY) && !defined(NO_SHA256) && \
    defined(WOLFSSL_PUBLIC_MP)
    int ret;
    uint8_t hash[WC_SHA256_DIGEST_SIZE];
    
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    printf("Running NIST P-256,SHA-256 Verify Test Vector\n");

    ret = crypto_sha256(
        kMsg, sizeof(kMsg), /* input message */
        hash, sizeof(hash), /* hash digest result */
        32                  /* configurable block / chunk size */
    );
    if (ret == 0) {
        ret = crypto_ecc_verify(
            kPubKey, sizeof(kPubKey), /* public key point x/y */
            hash, sizeof(hash),       /* computed hash digest */
            kSigRS, sizeof(kSigRS),   /* signature r/s */
            ECC_CURVE_SZ,             /* curve size in bytes */
            ECC_CURVE_ID              /* curve id */
        );
    }

    if (ret == 0) {
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
