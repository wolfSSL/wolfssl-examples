/* curve25519_test.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
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

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/curve25519.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

/*
./configure --enable-curve25519 --enable-ed25519 && make && sudo make install
gcc -o curve25519_test -lwolfssl curve25519_test.c

./configure --enable-curve25519 --enable-ed25519 --enable-debug --disable-shared && make
gcc -g -o curve25519_test -I. ./src/.libs/libwolfssl.a curve25519_test.c
 */

#ifdef HAVE_CURVE25519
static size_t hex2bin(byte* p, const char* hex)
{
    size_t len, i;
    int x;

    len = strlen(hex);
    if ((len & 1) != 0) {
        return 0;
    }

    for (i=0; i<len; i++) {
        if (isxdigit((int)hex[i]) == 0) {
            return 0;
        }
    }

    for (i=0; i<len / 2; i++) {
        sscanf(&hex[i * 2], "%2x", &x);
        p[i] = (byte)x;
    }
    return len / 2;
}

static void print_secret(char* desc, byte* s, int sLen)
{
    int i;
    printf("%s: ", desc);
    for (i = 0; i < sLen; i++)
        printf("%02x", s[i]);
    printf("\n");
}

int curve25519_secret(const byte* priv, const byte* pub, byte* secret,
    word32* secretsz, int endianess)
{
    int ret;
    curve25519_key privKey, pubKey;

    ret = wc_curve25519_init(&privKey);
    if (ret == 0)
        ret = wc_curve25519_init(&pubKey);

    if (ret == 0) {
        ret = wc_curve25519_import_private_ex(priv, 32, &privKey, endianess);
        if (ret != 0) {
            printf("wc_curve25519_import_private alice failed\n");
        }
    }

    if (ret == 0) {
        ret = wc_curve25519_check_public(pub, 32, endianess);
        if (ret != 0) {
            printf("wc_curve25519_check_public bob failed\n");
        }
    }
    if (ret == 0)
        ret = wc_curve25519_import_public_ex(pub, 32, &pubKey, endianess);
    if (ret == 0) {
        ret = wc_curve25519_shared_secret_ex(&privKey, &pubKey, secret,
            secretsz, endianess);
    }

    wc_curve25519_free(&pubKey);
    wc_curve25519_free(&privKey);
    return ret;
}

int main(void)
{
    int ret;

    /* RFC 7748 Curve25519 Test Vectors - in little endian format */
    const char* alice_prv_hexstr = "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a";
    const char* alice_pub_hexstr = "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a";
    const char* bob_prv_hexstr =   "5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb";
    const char* bob_pub_hexstr =   "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f";
    const char* secret_hexstr =    "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742";

    byte alice_prv[32], alice_pub[32];
    byte bob_prv[32],   bob_pub[32];
    byte alice_secret[32], bob_secret[32], secret_exp[32];
    word32 alice_secretsz = 32, bob_secretsz = 32, secret_expsz;

    hex2bin(alice_prv, alice_prv_hexstr);
    hex2bin(alice_pub, alice_pub_hexstr);
    hex2bin(bob_prv, bob_prv_hexstr);
    hex2bin(bob_pub, bob_pub_hexstr);
    secret_expsz = hex2bin(secret_exp, secret_hexstr);

    /* test vectors are in little endian */
    ret = curve25519_secret(alice_prv, bob_pub, alice_secret, &alice_secretsz,
        EC25519_LITTLE_ENDIAN);
    if (ret == 0) {
        ret = curve25519_secret(bob_prv, alice_pub, bob_secret, &bob_secretsz,
            EC25519_LITTLE_ENDIAN);
    }

    if (ret == 0) {
        print_secret("Secret Generated Alice", alice_secret, (int)alice_secretsz);
        print_secret("Secret Generated Bob", bob_secret, (int)bob_secretsz);
        print_secret("Secret Expected", secret_exp, (int)secret_expsz);

        if (alice_secretsz != secret_expsz || memcmp(secret_exp, alice_secret, secret_expsz) != 0 ||
            bob_secretsz != secret_expsz ||   memcmp(secret_exp, bob_secret, secret_expsz) != 0
        ) {
            ret = -1;
        }
    }

    if (ret == 0) {
        printf("Curve25519 test success\n");
    }
    else {
        printf("Curve25519 test error %d: %s\n", ret, wc_GetErrorString(ret));
    }

    return ret;
}

#else
int main(void)
{
    printf("Please build wolfssl with ./configure --enable-curve25519\n");
    return 0;
}
#endif
