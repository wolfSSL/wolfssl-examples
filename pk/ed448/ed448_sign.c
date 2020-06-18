/* ed448_sign.c
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

/* Example to demonstrate Ed448 Sign */
/* 
./configure --enable-ed448 && make && sudo make install
gcc -lwolfssl -o ed448_sign ed448_sign.c
*/

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/ed448.h>
#include <wolfssl/wolfcrypt/integer.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#include <stdint.h>
#include <stdio.h>

#if defined(HAVE_ED448) && defined(HAVE_ED448_SIGN) && \
    defined(HAVE_ED448_VERIFY)

/* Test Vector */
static const uint8_t kMsg[] = {
    0x69, 0xbc, 0x9f, 0xce, 0x68, 0x17, 0xc2, 0x10,
    0xea, 0xfc, 0x10, 0x65, 0x67, 0x52, 0xed, 0x78, 
    0x6e, 0xb8, 0x83, 0x9c, 0x9a, 0xb4, 0x56, 0x0d,
    0xc1, 0x0d, 0x1f, 0x78, 0x6e, 0x75, 0xd7, 0xbe, 
    0x92, 0x6b, 0x12, 0xf6, 0x76, 0x60, 0x8e, 0xb1,
    0xf4, 0x19, 0x0c, 0x81, 0xe7, 0x54, 0x5e, 0xbc, 
    0xe0, 0xae, 0xc2, 0x7d, 0x1b, 0xc4, 0x6e, 0xec,
    0xb1, 0x99, 0x6c, 0xbf, 0x0e, 0x38, 0xa8, 0x01, 
    0xa6, 0x9a, 0x48, 0x12, 0xe4, 0xc9, 0x3b, 0xf0,
    0x63, 0x46, 0x15, 0xb4, 0x61, 0xa8, 0x1a, 0x60, 
    0x71, 0x87, 0x98, 0xd7, 0x6f, 0x98, 0x7b, 0x2d,
    0xb9, 0x19, 0x1b, 0x21, 0x9c, 0x70, 0x58, 0xe8, 
    0x0d, 0x0f, 0xe9, 0x2d, 0x9a, 0x9a, 0xf1, 0x55,
    0xa0, 0x4c, 0xd3, 0x07, 0xbd, 0x97, 0x48, 0xec, 
    0x88, 0x0a, 0xaf, 0xb3, 0x80, 0x78, 0xa4, 0x59,
    0x43, 0x57, 0xd3, 0xa7, 0x01, 0x66, 0x0e, 0xfc
};
static const uint8_t kPrivKey[] = {
    /* k */
    0x5C, 0xEF, 0xDE, 0xFE, 0x14, 0xBD, 0xB4, 0x82,
    0x14, 0x15, 0x35, 0x9C, 0xD0, 0xE8, 0x0E, 0x07,
    0xFD, 0xFE, 0x24, 0xEC, 0xDF, 0x59, 0x28, 0x97,
    0x0A, 0xE7, 0xE1, 0xD6, 0xD5, 0x38, 0x15, 0xE7,
    0xA3, 0xFB, 0x56, 0x79, 0xE5, 0x17, 0x6F, 0x47,
    0xE8, 0x87, 0x6C, 0x8F, 0x32, 0xF0, 0x3F, 0x70,
    0xF5, 0x3F, 0xEB, 0x92, 0x2C, 0x4B, 0xFD, 0xBE,
    0x07
};
static const uint8_t kPubKey[] = {
    /* y */
    0x56, 0xBC, 0x28, 0x00, 0x16, 0x93, 0x41, 0xDB,
    0xBB, 0xAE, 0x4F, 0x95, 0x61, 0x90, 0x6E, 0x10,
    0xE7, 0xD6, 0x12, 0xC5, 0x4E, 0xA1, 0x7D, 0xDA,
    0xA0, 0xDD, 0xD3, 0x00, 0x8F, 0x28, 0xA3, 0x05,
    0x9D, 0xDB, 0xC2, 0x0E, 0x39, 0x0F, 0x6E, 0x31,
    0x1A, 0x16, 0xE5, 0xC6, 0x5F, 0x69, 0x30, 0x9D,
    0xC4, 0x03, 0xB7, 0x43, 0x37, 0x03, 0x50, 0xAD,
    0x00
};


/* perform verify of signature and message using public key */
/* key is public y */
/* sig is r + s */
int crypto_ed448_verify(const uint8_t *key, uint32_t keySz,
    const uint8_t *sig, uint32_t sigSz)
{
    int ret, verify_res = 0;
    ed448_key ed448;

    /* validate arguments */
    if (key == NULL || sig == NULL) {
        return BAD_FUNC_ARG;
    }

    /* Setup the Ed448 key */
    ret = wc_ed448_init(&ed448);
    if (ret < 0) {
        return ret;
    }

    /* Import public key - compressed point */
    ret = wc_ed448_import_public(
        (byte*)key, keySz,      /* Public "y" Coordinate */
        &ed448                  /* key context */
    );

    /* Verify Ed448 Signature */
    if (ret == 0) {
        ret = wc_ed448_verify_msg(
            sig, sigSz,         /* r/s encoded */
            kMsg, sizeof(kMsg), /* message */
            &verify_res,        /* verification result 1=success */
            &ed448,             /* key context */
            NULL, 0             /* contex of signature */
        );
    }
    
    /* check verify result */
    if (ret == 0 && verify_res == 0) {
        ret = SIG_VERIFY_E;
    }

    wc_ed448_free(&ed448);

    return ret;
}

/* perform signature operation against message using private and public key */
int crypto_ed448_sign(const uint8_t *key, uint32_t keySz,
    const uint8_t *pubKey, uint32_t pubKeySz, uint8_t *sig, uint32_t* sigSz)
{
    int ret;
    ed448_key ed448;

    /* validate arguments */
    if (key == NULL || sig == NULL || sigSz == NULL) {
        return BAD_FUNC_ARG;
    }

    /* Initialize signature result */
    memset(sig, 0, ED448_SIG_SIZE);

    /* Setup the Ed448 key */
    ret = wc_ed448_init(&ed448);
    if (ret < 0) {
        return ret;
    }

    /* Import private key "k" */
    ret = wc_ed448_import_private_key(
        key, keySz,             /* private key "k" */
        pubKey, pubKeySz,       /* private key "k" */
        &ed448                  /* key context */
    );

    if (ret == 0) {
        /* Verify Ed448 Signature */
        ret = wc_ed448_sign_msg(
            kMsg, sizeof(kMsg), /* message */
            sig, sigSz,         /* r/s as encoded data */
            &ed448,             /* key context */
            NULL, 0             /* contex of signature */
        );
    }
    
    wc_ed448_free(&ed448);

    return ret;
}

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

int main()
{
    int ret;
    uint8_t sig[ED448_SIG_SIZE];
    uint32_t sigSz = 0;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    memset(sig, 0, sizeof(sig));

    /* Sign the message */
    sigSz = sizeof(sig);
    ret = crypto_ed448_sign(
        kPrivKey, sizeof(kPrivKey), /* private key */
        kPubKey, sizeof(kPubKey),   /* public key compressed point y */
        sig, &sigSz                 /* signature r/s */
    );

    if (ret == 0) {
        /* Verify generated signature is valid */
        ret = crypto_ed448_verify(
            kPubKey, sizeof(kPubKey),   /* public key point x/y */
            sig, sigSz                  /* signature r/s */
        );
    }

    if (ret == 0) {
        printf("Signature %d\n", sigSz);
        print_hex(sig, sigSz);

        printf("Success\n");
    }
    else {
        printf("Failure %d: %s\n", ret, wc_GetErrorString(ret));
        ret = 1;
    }

    return ret;
}

#else

int main()
{
    printf("wolfSSL requires Ed448\n");
    return 1;
}

#endif /* HAVE_ED448 && HAVE_ED448_SIGN && HAVE_ED448_VERIFY */

