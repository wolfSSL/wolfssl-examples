/* ecdh_gen_secret.c
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

#include <stdio.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/curve25519.h>
#include <wolfssl/wolfcrypt/curve448.h>

#define ECC_256_BIT_FIELD 32 // 256-bit curve field
#define ECC_448_BIT_FIELD 56 // 448-bit curve field

void print_secret(char* who, byte* s, int sLen);
void Usage(int* curveChoice);
int do_ecc(void);
int do_25519(void);
int do_448(void);

/* Constant-time buffer compare for secret material.
 *
 * wolfSSL's own constant-time compare (ConstantCompare() in
 * wolfcrypt/src/misc.c) is WOLFSSL_LOCAL and not exported, so it can't be
 * called from application code. This is the pattern to use instead of
 * XMEMCMP/memcmp whenever comparing secrets, MACs, or derived key material:
 * every byte is compared regardless of where a mismatch occurs, so the
 * runtime doesn't leak position information via early-exit timing.
 *
 * Returns 0 if the buffers are equal, non-zero otherwise. */
#if defined(HAVE_ECC) || defined(HAVE_CURVE25519) || defined(HAVE_CURVE448)
static int const_time_memcmp(const void* a, const void* b, size_t len)
{
    const byte* pa = (const byte*)a;
    const byte* pb = (const byte*)b;
    byte diff = 0;
    size_t i;

    for (i = 0; i < len; i++) {
        diff |= (byte)(pa[i] ^ pb[i]);
    }

    return (int)diff;
}
#endif /* HAVE_ECC || HAVE_CURVE25519 || HAVE_CURVE448 */

int main(int argc, char** argv)
{
    int curveChoice = 0;
    int ret;

    if (argc < 2)
        Usage(&curveChoice);
    else
        sscanf(argv[1], "%d", &curveChoice);

    if (curveChoice == 1) {
        ret = do_ecc();
    }
    else if (curveChoice == 2) {
        ret = do_25519();
    }
    else {
        ret = do_448();
    }

    return ret;
}

int do_ecc(void)
{
    int ret;
#ifdef HAVE_ECC
    WC_RNG rng;
    ecc_key AliceKey, BobKey;

    byte AliceSecret[ECC_256_BIT_FIELD] = {0};
    byte BobSecret[ECC_256_BIT_FIELD] = {0};
    word32 secretLen = 0;

    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;

    ret = wc_ecc_init(&AliceKey);
    if (ret != 0)
        goto only_rng;

    ret = wc_ecc_init(&BobKey);
    if (ret != 0)
        goto alice_and_rng;

    ret = wc_ecc_make_key(&rng, ECC_256_BIT_FIELD, &AliceKey);
    if (ret != 0)
        goto all_three;
#ifdef ECC_TIMING_RESISTANT
    ret = wc_ecc_set_rng(&AliceKey, &rng);
    if (ret != 0)
        goto all_three;
#endif

    ret = wc_ecc_make_key(&rng, ECC_256_BIT_FIELD, &BobKey);
    if (ret != 0)
        goto all_three;
#ifdef ECC_TIMING_RESISTANT
    ret = wc_ecc_set_rng(&BobKey, &rng);
    if (ret != 0)
        goto all_three;
#endif

    secretLen = ECC_256_BIT_FIELD; /* explicit set */
    wc_ecc_set_rng(&AliceKey, &rng);
    ret = wc_ecc_shared_secret(&AliceKey, &BobKey, AliceSecret, &secretLen);
    if (ret != 0)
        goto all_three;

    secretLen = ECC_256_BIT_FIELD; /* explicit reset for best practice */
    wc_ecc_set_rng(&BobKey, &rng);
    ret = wc_ecc_shared_secret(&BobKey, &AliceKey, BobSecret, &secretLen);
    if (ret == 0) {
        /* Use a constant-time compare (not XMEMCMP) since these buffers
         * hold secret key material. See const_time_memcmp() above. */
        if (const_time_memcmp(AliceSecret, BobSecret, secretLen)) {
            printf("Failed to generate a common secret\n");
            ret = -1;
            goto all_three;
        }
    } else {
        goto all_three;
    }

    printf("Successfully generated a common secret\n");
    print_secret("Alice", AliceSecret, (int) secretLen);
    print_secret("Bob", BobSecret, (int) secretLen);

all_three:
    wc_ecc_free(&BobKey);
alice_and_rng:
    wc_ecc_free(&AliceKey);
only_rng:
    wc_FreeRng(&rng);
#else
    printf("Configure wolfSSL with --enable-ecc and try again\n");
    ret = -1;
#endif
    /* not ret: an exit code is masked to 8 bits, so -256 would read as 0 */
    return (ret == 0) ? 0 : 1;
}

int do_25519(void)
{
    int ret;
#ifdef HAVE_CURVE25519
    WC_RNG rng;
    curve25519_key AliceKey, BobKey;

    byte AliceSecret[ECC_256_BIT_FIELD] = {0};
    byte BobSecret[ECC_256_BIT_FIELD] = {0};
    word32 secretLen = 0;

    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;

    ret = wc_curve25519_init(&AliceKey);
    if (ret != 0)
        goto only_rng;

    ret = wc_curve25519_init(&BobKey);
    if (ret != 0)
        goto alice_and_rng;

    ret = wc_curve25519_make_key(&rng, ECC_256_BIT_FIELD, &AliceKey);
    if (ret != 0)
        goto all_three;
    ret = wc_curve25519_make_key(&rng, ECC_256_BIT_FIELD, &BobKey);
    if (ret != 0)
        goto all_three;

    secretLen = ECC_256_BIT_FIELD; /* explicit set */
    ret = wc_curve25519_shared_secret(&AliceKey, &BobKey, AliceSecret, &secretLen);
    if (ret != 0)
        goto all_three;

    secretLen = ECC_256_BIT_FIELD; /* explicit reset for best practice */
    ret = wc_curve25519_shared_secret(&BobKey, &AliceKey, BobSecret, &secretLen);
    if (ret == 0) {
        /* Use a constant-time compare (not XMEMCMP) since these buffers
         * hold secret key material. See const_time_memcmp() above. */
        if (const_time_memcmp(AliceSecret, BobSecret, secretLen)) {
            printf("Failed to generate a common secret\n");
            ret = -1;
            goto all_three;
        }
    } else {
        goto all_three;
    }

    printf("Successfully generated a common 25519 secret\n");
    print_secret("Alice", AliceSecret, (int) secretLen);
    print_secret("Bob", BobSecret, (int) secretLen);

all_three:
    wc_curve25519_free(&BobKey);
alice_and_rng:
    wc_curve25519_free(&AliceKey);
only_rng:
    wc_FreeRng(&rng);
#else
    printf("Configure wolfssl with --enable-curve25519 and try again\n");
    ret = -1;
#endif
    /* not ret: an exit code is masked to 8 bits, so -256 would read as 0 */
    return (ret == 0) ? 0 : 1;
}

int do_448(void)
{
    int ret;
#ifdef HAVE_CURVE448
    WC_RNG rng;
    curve448_key AliceKey, BobKey;

    byte AliceSecret[ECC_448_BIT_FIELD] = {0};
    byte BobSecret[ECC_448_BIT_FIELD] = {0};
    word32 secretLen = 0;

    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;

    ret = wc_curve448_init(&AliceKey);
    if (ret != 0)
        goto only_rng;

    ret = wc_curve448_init(&BobKey);
    if (ret != 0)
        goto alice_and_rng;

    ret = wc_curve448_make_key(&rng, ECC_448_BIT_FIELD, &AliceKey);
    if (ret != 0)
        goto all_three;
    ret = wc_curve448_make_key(&rng, ECC_448_BIT_FIELD, &BobKey);
    if (ret != 0)
        goto all_three;

    secretLen = ECC_448_BIT_FIELD; /* explicit set */
    ret = wc_curve448_shared_secret(&AliceKey, &BobKey, AliceSecret, &secretLen);
    if (ret != 0)
        goto all_three;

    secretLen = ECC_448_BIT_FIELD; /* explicit reset for best practice */
    ret = wc_curve448_shared_secret(&BobKey, &AliceKey, BobSecret, &secretLen);
    if (ret == 0) {
        /* Use a constant-time compare (not XMEMCMP) since these buffers
         * hold secret key material. See const_time_memcmp() above. */
        if (const_time_memcmp(AliceSecret, BobSecret, secretLen)) {
            printf("Failed to generate a common secret\n");
            ret = -1;
            goto all_three;
        }
    } else {
        goto all_three;
    }

    printf("Successfully generated a common 448 secret\n");
    print_secret("Alice", AliceSecret, (int) secretLen);
    print_secret("Bob", BobSecret, (int) secretLen);

all_three:
    wc_curve448_free(&BobKey);
alice_and_rng:
    wc_curve448_free(&AliceKey);
only_rng:
    wc_FreeRng(&rng);
#else
    printf("Configure wolfssl with --enable-curve448 and try again\n");
    ret = -1;
#endif
    /* not ret: an exit code is masked to 8 bits, so -256 would read as 0 */
    return (ret == 0) ? 0 : 1;
}

void print_secret(char* who, byte* s, int sLen)
{
    int i;
    printf("%ss' Secret: ", who);
    for (i = 0; i < sLen; i++)
        printf("%02x", s[i]);
    printf("\n");
}

void Usage(int* curveChoice)
{
    char answer = 0;
    char input[10];

    printf("Optional, specify a curve:\n");
    printf("1) ECC SECP256R1\n"
           "2) Curve25519\n\n"
           "3) Curve448\n\n"
           "Contact support@wolfssl.com if you'd like to see other options\n\n"
           "Example usage:\n"
           "./ecdh_gen_secret 1\n"
           "./ecdh_gen_secret 2\n\n"
           "Selecting no (n) will default to option 1, SECP256R1\n"
           "Would you like to select a curve now? (y/n) > "
          );
    answer = getchar();

    if (answer == 'y') {
        printf("\nEnter an option from the above curve list > ");
        if (scanf("%9s", input) != 1) {
            printf("Invalid input\n");
            exit(-1);
        }
        printf("You entered: %s\n", input);
        sscanf(input, "%d", curveChoice);
        if (*curveChoice != 1 && *curveChoice != 2) {
            printf("Invalid choice, select either: 1 or 2\n");
            exit(-1);
        }
    } else
        *curveChoice = 1;
    printf("curveChoice indicated: %d\n", *curveChoice);
}
