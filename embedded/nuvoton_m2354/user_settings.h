/* user_settings.h
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

/* wolfCrypt configuration for the NuMaker-M2354 (M2354KJFAE, Cortex-M23,
 * 1 MB flash, 256 KB SRAM). Sized for the wolfcrypt_test and benchmark
 * application in this directory. */

#ifndef WOLFSSL_USER_SETTINGS_H
#define WOLFSSL_USER_SETTINGS_H

#ifdef __cplusplus
    extern "C" {
#endif

/* ---- The port ------------------------------------------------------- */

/* Turns the port on and, with no engine named, offloads all of them. See
 * wolfssl/wolfcrypt/port/nuvoton/nuvoton_settings.h for the switches. */
#define WOLFSSL_NUVOTON_M2354


/* build.sh sets WOLFSSL_NUVOTON_SECURE or WOLFSSL_NUVOTON_NSC on the command
 * line. Left alone, the port defaults to the secure world, which is where a
 * non-TrustZone M2354 application runs. */

/* ---- Platform ------------------------------------------------------- */

#define SINGLE_THREADED
/* app.c owns main(); test.c and benchmark.c are called from it. */
#define NO_MAIN_DRIVER
#define NO_FILESYSTEM
#define WOLFSSL_USER_IO
#define NO_WRITEV
#define WOLFSSL_NO_SOCK
#define WOLFSSL_GENERAL_ALIGNMENT 4
#define WOLFSSL_SMALL_STACK
/* The benchmark otherwise asks for a 1 MB block buffer. */
#define BENCH_EMBEDDED
#define WOLFSSL_IGNORE_FILE_WARN

/* Entropy comes from the port's wc_GenerateSeed(), which reads the TRNG.
 * nuvoton_settings.h sets NO_DEV_RANDOM for the same reason.
 *
 * There is no RTC set up here, so the wall clock and the low resolution timer
 * both come from the SysTick based helpers in app.c. */
#define WOLFSSL_USER_CURRTIME
#define USER_TICKS
#define NO_ASN_TIME

/* ---- Crypto --------------------------------------------------------- */

#define HAVE_HASHDRBG

#define WOLFSSL_SHA224
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512
#define WOLFSSL_NO_SHAKE128
#define WOLFSSL_NO_SHAKE256
#define NO_MD4
#define NO_MD5
#define NO_DES3
#define NO_RC4
#define NO_PSK
#define NO_DSA

#define HAVE_AES_CBC
#define HAVE_AES_ECB
#define WOLFSSL_AES_DIRECT
#define WOLFSSL_AES_COUNTER
#define HAVE_AESGCM
#define HAVE_AESCCM
#define GCM_TABLE_4BIT
#define HAVE_AES_KEYWRAP

/* Single-precision math for the curves and key sizes in use, with the Thumb-1
 * assembly. This is what the software fallback paths run on: the accelerator
 * does not cover every curve or operation, and without these the leftovers go
 * through the generic bignum code, which on a 96 MHz Cortex-M23 is slow enough
 * to dominate anything it touches. sp_armthumb.c is the correct assembly for
 * this core; sp_cortexm.c is Thumb-2 and will not assemble. */
#define WOLFSSL_HAVE_SP_ECC
#define WOLFSSL_HAVE_SP_RSA
#define WOLFSSL_SP_ARM_THUMB
#define WOLFSSL_SP_ARM_THUMB_ASM

#define HAVE_ECC
#define ECC_USER_CURVES
#define HAVE_ECC256
#define HAVE_ECC384
#define ECC_SHAMIR
#define HAVE_ECC_DHE
#define HAVE_ECC_SIGN
#define HAVE_ECC_VERIFY
#define HAVE_ECC_KEY_EXPORT
#define HAVE_ECC_KEY_IMPORT

#define WC_RSA_BLINDING
/* Key generation is off by default, and the suite is usable because of it.
 *
 * WOLFSSL_KEY_GEN turns on RSA key generation and, in wolfcrypt_test, a call
 * to wc_DhGenerateParams() for a fresh 2048-bit parameter set. Both are
 * software primality searches that no part of this accelerator takes part in,
 * and on a 96 MHz Cortex-M23 the DH one runs for hours: a capture that stops
 * after the RSA test with no further output is usually this, not a fault. RSA
 * key generation alone measures around 450 seconds at 3072 bits here.
 *
 * Turn it on if you want that coverage and are prepared for the wait. It has
 * not been run to completion on this part, so whether those two tests pass
 * here is untested rather than known. */
#if 0
    #define WOLFSSL_KEY_GEN
#endif

#define HAVE_HKDF
#define WOLFSSL_BASE64_ENCODE

/* Hardening. On by default in an autotools build, so keep it here too. */
#define TFM_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT

/* ---- TLS ------------------------------------------------------------ */

/* This application only exercises wolfCrypt, so the TLS layer is left out and
 * build.sh compiles wolfcrypt/src alone. For a TLS build, drop WOLFCRYPT_ONLY
 * and add the sources under src/ as well; the settings below are already the
 * ones TLS 1.3 wants. */
#define WOLFCRYPT_ONLY

#define WOLFSSL_TLS13
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_EXTENDED_MASTER
#define WC_RSA_PSS
#define NO_OLD_TLS

/* ---- Size ----------------------------------------------------------- */

#define NO_ERROR_STRINGS
#define WOLFSSL_NO_CURRDIR

#ifdef __cplusplus
    }  /* extern "C" */
#endif

#endif /* WOLFSSL_USER_SETTINGS_H */
