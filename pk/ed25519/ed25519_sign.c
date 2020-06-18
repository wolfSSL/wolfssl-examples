/* ed25519_sign.c
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

/* Example to demonstrate Ed25519 Sign */
/* 
./configure --enable-ed25519 && make && sudo make install
gcc -lwolfssl -o ed25519_sign ed25519_sign.c
*/

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/integer.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#include <stdint.h>
#include <stdio.h>

#if defined(HAVE_ED25519) && defined(HAVE_ED25519_SIGN) && \
    defined(HAVE_ED25519_VERIFY)

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
    0x88, 0x0d, 0xaa, 0xde, 0x32, 0xaa, 0x93, 0x32, 
    0x79, 0xe2, 0x4e, 0x45, 0xa9, 0x1f, 0x26, 0xd0, 
    0x9b, 0x69, 0xdd, 0x08, 0x33, 0xc7, 0x14, 0xc1, 
    0x57, 0x7f, 0x20, 0xbe, 0x67, 0x4f, 0xb9, 0xeb, 
};
static const uint8_t kPubKey[] = {
    /* y */
    0x37, 0x3e, 0xd5, 0x8d, 0x22, 0x1a, 0x05, 0x81,
    0xbf, 0x24, 0x6e, 0xdc, 0x5a, 0x42, 0x08, 0x83,
    0xff, 0xac, 0xfb, 0x28, 0xd0, 0x83, 0xb8, 0x2d,
    0x1c, 0xb7, 0x04, 0xaf, 0xa8, 0x41, 0x79, 0x23
};


/* perform verify of signature and message using public key */
/* key is public y */
/* sig is r + s */
int crypto_ed25519_verify(const uint8_t *key, uint32_t keySz,
    const uint8_t *sig, uint32_t sigSz)
{
    int ret, verify_res = 0;
    ed25519_key ed25519;

    /* validate arguments */
    if (key == NULL || sig == NULL) {
        return BAD_FUNC_ARG;
    }

    /* Setup the Ed25519 key */
    ret = wc_ed25519_init(&ed25519);
    if (ret < 0) {
        return ret;
    }

    /* Import public key - compressed point */
    ret = wc_ed25519_import_public(
        (byte*)key, keySz,      /* Public "y" Coordinate */
        &ed25519                /* key context */
    );

    /* Verify Ed25519 Signature */
    if (ret == 0) {
        ret = wc_ed25519_verify_msg(
            sig, sigSz,         /* r/s encoded */
            kMsg, sizeof(kMsg), /* message */
            &verify_res,        /* verification result 1=success */
            &ed25519            /* key context */
        );
    }
    
    /* check verify result */
    if (ret == 0 && verify_res == 0) {
        ret = SIG_VERIFY_E;
    }

    wc_ed25519_free(&ed25519);

    return ret;
}

/* perform signature operation against message using private and public key */
int crypto_ed25519_sign(const uint8_t *key, uint32_t keySz,
    const uint8_t *pubKey, uint32_t pubKeySz, uint8_t *sig, uint32_t* sigSz)
{
    int ret;
    ed25519_key ed25519;

    /* validate arguments */
    if (key == NULL || sig == NULL || sigSz == NULL) {
        return BAD_FUNC_ARG;
    }

    /* Initialize signature result */
    memset(sig, 0, ED25519_SIG_SIZE);

    /* Setup the Ed25519 key */
    ret = wc_ed25519_init(&ed25519);
    if (ret < 0) {
        return ret;
    }

    /* Import private key "k" */
    ret = wc_ed25519_import_private_key(
        key, keySz,             /* private key "k" */
        pubKey, pubKeySz,       /* private key "k" */
        &ed25519                /* key context */
    );
    if (ret == 0) {
        /* Verify Ed25519 Signature */
        ret = wc_ed25519_sign_msg(
            kMsg, sizeof(kMsg), /* message */
            sig, sigSz,         /* r/s as encoded data */
            &ed25519            /* key context */
        );
    }
    
    wc_ed25519_free(&ed25519);

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
    uint8_t sig[ED25519_SIG_SIZE];
    uint32_t sigSz = 0;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    memset(sig, 0, sizeof(sig));

    /* Sign the message */
    sigSz = sizeof(sig);
    ret = crypto_ed25519_sign(
        kPrivKey, sizeof(kPrivKey), /* private key */
        kPubKey, sizeof(kPubKey),   /* public key compressed point y */
        sig, &sigSz                 /* signature r/s */
    );

    if (ret == 0) {
        /* Verify generated signature is valid */
        ret = crypto_ed25519_verify(
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
    printf("wolfSSL requires Ed25519\n");
    return 1;
}

#endif /* HAVE_ED25519 && HAVE_ED25519_SIGN && HAVE_ED25519_VERIFY */

