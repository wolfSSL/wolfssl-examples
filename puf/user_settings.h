/* user_settings.h
 *
 * Minimal wolfCrypt configuration for bare-metal SRAM PUF example.
 * Target: Cortex-M (tested on NUCLEO-H563ZI)
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

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Platform */
/* ------------------------------------------------------------------------- */
#define WOLFCRYPT_ONLY
#define SINGLE_THREADED
#define WOLFSSL_SMALL_STACK
#define WOLFSSL_GENERAL_ALIGNMENT   4
#define WOLFSSL_USER_IO
#ifndef USE_WOLF_ARM_STARTUP
#define USE_WOLF_ARM_STARTUP
#endif

/* ------------------------------------------------------------------------- */
/* PUF (Physically Unclonable Function) */
/* ------------------------------------------------------------------------- */
#define WOLFSSL_PUF
#define WOLFSSL_PUF_SRAM

/* Enable test mode: allows synthetic SRAM data for testing without hardware.
 * Comment out to use real SRAM PUF on actual hardware. */
#define WOLFSSL_PUF_TEST

/* ------------------------------------------------------------------------- */
/* Required Dependencies */
/* ------------------------------------------------------------------------- */
/* HKDF is required for PUF key derivation (wc_PufDeriveKey) */
#define HAVE_HKDF

/* SHA-256 is required for PUF identity and HKDF */
/* (enabled by default, do not define NO_SHA256) */

/* Uncomment to use SHA3-256 instead of SHA-256 for PUF identity and HKDF */
/* #define WC_PUF_SHA3 */
/* #define WOLFSSL_SHA3 */

/* ------------------------------------------------------------------------- */
/* Math Configuration */
/* ------------------------------------------------------------------------- */
#define WOLFSSL_SP_MATH_ALL
#define WOLFSSL_SP_SMALL
#define SP_WORD_SIZE 32

/* ------------------------------------------------------------------------- */
/* Disable Unused Features */
/* ------------------------------------------------------------------------- */
#define NO_RSA
#define NO_DH
#define NO_DSA
#define NO_DES3
#define NO_RC4
#define NO_MD4
#define NO_MD5
#define NO_SHA
#define NO_PSK
#define NO_OLD_TLS
#define NO_PWDBASED
#define NO_CERTS
#define NO_ASN
#define NO_CODING
#define NO_SIG_WRAPPER
#define NO_AES

/* ------------------------------------------------------------------------- */
/* System */
/* ------------------------------------------------------------------------- */
#define NO_FILESYSTEM
#define NO_WRITEV
#define NO_MAIN_DRIVER
#define NO_DEV_RANDOM

#define BENCH_EMBEDDED
#define WOLFSSL_IGNORE_FILE_WARN

/* Provide custom time function */
#define USER_TICKS

/* Use STM32H5 hardware TRNG (implemented in stm32.c) */
extern int custom_rand_gen_block(unsigned char *output, unsigned int sz);
#define CUSTOM_RAND_GENERATE_BLOCK custom_rand_gen_block

#ifdef __cplusplus
}
#endif

#endif /* WOLFSSL_USER_SETTINGS_H */
