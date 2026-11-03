/* user_settings.h
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

#ifndef WOLFSSL_USER_SETTINGS_LPC55S6X_H
#define WOLFSSL_USER_SETTINGS_LPC55S6X_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fsl_debug_console.h"

/* Hardware acceleration.  WOLFSSL_NXP_LPC55S6X pulls in the whole LPC55S6x
 * port: the TRNG, HashCrypt (AES and SHA) and CASPER (RSA public key).  The
 * Makefile leaves it out for HW_CRYPTO=0, which builds the same algorithm set
 * in software to compare against. */
#ifndef WOLFSSL_LPC55S6X_SOFTWARE_ONLY
    #define WOLFSSL_NXP_LPC55S6X
#endif

/* The TRNG stays on in both builds -- it is the board's only entropy source,
 * and keeping it fixed means the HW_CRYPTO=0 comparison differs only in the
 * AES/SHA/RSA implementations. */
#define WOLFSSL_NXP_RNG_1

/* WOLFSSL_NXP_CASPER makes wc_port.h turn WOLFSSL_CRYPT_HW_MUTEX on, but
 * hashcrypt_port.c has an #error against exactly that, so the two halves of
 * WOLFSSL_NXP_LPC55S6X will not compile together at the default setting.
 * SINGLE_THREADED means the mutex is a no-op anyway, so pin it off here --
 * wolfSSL_HwPkMutexInit() then resolves to the stub in wc_port.h. */
#define WOLFSSL_CRYPT_HW_MUTEX 0

/* Platform */
#define WOLFSSL_GENERAL_ALIGNMENT 4
#define SINGLE_THREADED
#define WOLFCRYPT_ONLY
#define SIZEOF_LONG_LONG 8
#define NO_FILESYSTEM
#define NO_WOLFSSL_DIR
#define WOLFSSL_NO_CURRDIR
#define NO_WRITEV
#define NO_DEV_RANDOM
#define NO_MAIN_DRIVER
#define WOLFSSL_NO_SOCK
#define WOLFSSL_IGNORE_FILE_WARN
#define WOLFSSL_SMALL_STACK

/* No RTC on the board, so certificate validity dates cannot be checked. */
#define NO_ASN_TIME

/* Output and the benchmark timebase both come from the board: PRINTF is the
 * SDK debug console, current_time() is the SysTick counter in main.c. */
#define BENCH_EMBEDDED
#define WOLFSSL_USER_CURRTIME

/* Not PRINTF directly: the test and benchmark terminate lines with a bare \n,
 * and neither SDK debug console translates it.  wolf_printf() in main.c adds
 * the CR as it formats. */
int wolf_printf(const char* fmt, ...);
#define XPRINTF wolf_printf

/* Math.  CASPER only accelerates RSA *public* operations, and its hook in
 * wc_RsaFunctionSync() runs ahead of the SP path, so SP is what ends up doing
 * the private key half -- worth having the Cortex-M assembly for. */
#define WOLFSSL_SP_MATH_ALL
#define WOLFSSL_HAVE_SP_RSA
#define WOLFSSL_SP_ASM
#define WOLFSSL_SP_ARM_CORTEX_M_ASM
#define SP_WORD_SIZE 32

/*
 * Enabled algorithms.  These are exactly what the LPC55S6x port accelerates:
 *
 *   HashCrypt  AES-128/192/256 ECB, CBC and CTR
 *              SHA-1, SHA-256
 *   CASPER     RSA public key operations.  CASPER accelerates ECC too and
 *              casper_port.c implements casper_ecc_mulmod()/mul2add(), but
 *              nothing in ecc.c calls them, so ECC would be software-only
 *              here -- see README.md.
 *   RNG        TRNG, used to seed the Hash-DRBG
 *
 * HashCrypt also covers CFB and OFB, but only for whole 16-byte blocks, and
 * wolfcrypt_test() drives both with sub-block lengths (8 bytes in
 * aesofb_test, 4 and 27 in aescfb_test).  They are left out so the test suite
 * runs clean; see README.md.
 */
#define HAVE_AES_ECB
#define WOLFSSL_AES_COUNTER
#define WOLFSSL_AES_DIRECT
#define WOLFSSL_AES_128
#define WOLFSSL_AES_192
#define WOLFSSL_AES_256

#define HAVE_RSA
#define WC_RSA_BLINDING
#define RSA_LOW_MEM
#define USE_CERT_BUFFERS_2048

/* HMAC and the Hash-DRBG both sit on top of the accelerated SHA cores. */
#define HAVE_HASHDRBG

/* Disabled: nothing reaches hardware for any of these on this part. */
#define NO_MD4
#define NO_MD5
#define NO_RC4
#define NO_DES3
#define NO_DSA
#define NO_DH
#define NO_PWDBASED
#define NO_PKCS12
#define NO_PKCS7
#define NO_SIG_WRAPPER
#define NO_SESSION_CACHE
#define NO_OLD_TLS
#define NO_OLD_RNGNAME
#define WOLFSSL_NO_PEM

/* Hashes HashCrypt has no engine for */
#define WOLFSSL_NO_SHA224
#define NO_SHA512

#ifdef __cplusplus
}
#endif

#endif /* WOLFSSL_USER_SETTINGS_LPC55S6X_H */
