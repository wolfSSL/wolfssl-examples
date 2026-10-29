/* user_settings.h
 *
 * wolfCrypt configuration for the PSOC Edge E8 (CM33) test and benchmark.
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#ifndef WOLFSSL_USER_SETTINGS_H
#define WOLFSSL_USER_SETTINGS_H

/* Platform */
#define WOLFCRYPT_ONLY
#define SINGLE_THREADED
#define NO_FILESYSTEM
#define NO_WRITEV
#define NO_MAIN_DRIVER
#define WOLFSSL_USER_IO
#define WOLFSSL_GENERAL_ALIGNMENT 4
#define SIZEOF_LONG_LONG 8

/* Crypto engine (MXCRYPTO), set by the Makefile: WOLFSSL_PSOC6_CRYPTO for
 * SHA-1/2/3 and AES ECB/CBC/CFB/GCM (PSOC_CRYPTO=1), and NO_PSOC6_CRYPTO_ECC
 * to keep ECDSA verify in software, which the SP math assembly runs faster
 * (unless PSOC_CRYPTO_ECC=1). */

/* Math */
#define WOLFSSL_SP_MATH_ALL
#define WOLFSSL_HAVE_SP_ECC
#define WOLFSSL_SP_ARM_CORTEX_M_ASM
/* The full-size SP tables are slow to read from XIP flash. */
#define WOLFSSL_SP_SMALL
#define WOLFSSL_SP_384
#define WOLFSSL_SP_521
#define SP_WORD_SIZE 32

/* ECC */
#define HAVE_ECC
#define ECC_USER_CURVES
#define HAVE_ECC384
#define HAVE_ECC521
#define ECC_TIMING_RESISTANT
#define ECC_SHAMIR

/* Hashes */
#define WOLFSSL_SHA224
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512
#define WOLFSSL_SHA3
#define WOLFSSL_SHAKE128
#define WOLFSSL_SHAKE256

/* AES */
#define HAVE_AESGCM
#define GCM_TABLE_4BIT
#define WOLFSSL_AES_DIRECT
#define HAVE_AES_ECB
#define WOLFSSL_AES_CFB
#define WOLFSSL_AES_COUNTER

/* RNG: Hash-DRBG seeded from the crypto engine TRNG (board.c). */
#define HAVE_HASHDRBG
extern int board_rng_seed(unsigned char *out, unsigned int sz);
#define CUSTOM_RAND_GENERATE_SEED board_rng_seed

/* Time */
#define WOLFSSL_USER_CURRTIME
extern unsigned long board_time(unsigned long *t);
#define XTIME board_time
#define NO_ASN_TIME

/* Test and benchmark */
#define BENCH_EMBEDDED
#define USE_CERT_BUFFERS_256
#define WOLFSSL_NO_FLOAT_FMT

/* Disabled */
#define NO_RSA
#define NO_DH
#define NO_DSA
#define NO_DES3
#define NO_RC4
#define NO_MD4
#define NO_MD5
#define NO_PSK
#define NO_PWDBASED
#define NO_OLD_TLS
#define WOLFSSL_NO_SHAKE256_TEST_LARGE

#endif /* WOLFSSL_USER_SETTINGS_H */
