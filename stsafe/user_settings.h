/* user_settings.h
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Platform Configuration                                                    */
/* ------------------------------------------------------------------------- */
#define SIZEOF_LONG_LONG 8

/* Use small stack (heap allocation for large buffers) */
#define WOLFSSL_SMALL_STACK

/* Threading - single threaded for simple testing */
/* #define SINGLE_THREADED */

/* Use /dev/urandom for random */
/* #define NO_DEV_RANDOM */

#define NO_OLD_TLS

/* ------------------------------------------------------------------------- */
/* Crypto Algorithms Required for STSAFE Platform                            */
/* ------------------------------------------------------------------------- */

/* AES - Required for CMAC, CBC, ECB */
#undef NO_AES
#define WOLFSSL_AES_DIRECT      /* Required for ECB mode */
#define HAVE_AES_CBC            /* Required for CBC mode */
#define WOLFSSL_CMAC            /* Required for AES-CMAC */

/* SHA-256 - Required for HMAC-SHA256 (HKDF) */
#undef NO_SHA256

/* SHA-384 - For P-384 operations */
#define WOLFSSL_SHA384

/* HMAC - Required for HKDF */
#undef NO_HMAC

/* ECC - For wolfSSL STSAFE integration */
#define HAVE_ECC
#define ECC_USER_CURVES
#undef NO_ECC256               /* Enable P-256 */
#define HAVE_ECC384            /* Enable P-384 */
#define ECC_TIMING_RESISTANT

/* ECC key import/export functions */
#define WOLFSSL_PUBLIC_MP      /* For mp_int access in ECC verify */

/* Math library */
#define WOLFSSL_SP_MATH_ALL
#define WOLFSSL_SP_384

/* ------------------------------------------------------------------------- */
/* Disable Unused Algorithms (reduce footprint)                              */
/* ------------------------------------------------------------------------- */
#define NO_RSA
#define NO_DH
#define NO_DSA
#define NO_RC4
#define NO_MD4
#define NO_MD5
#define NO_DES3
#define NO_HC128
#define NO_RABBIT
#define NO_PWDBASED

/* Keep SHA-1 disabled unless needed */
#define NO_SHA

/* ------------------------------------------------------------------------- */
/* Optional Features                                                         */
/* ------------------------------------------------------------------------- */

/* Debugging */
#define DEBUG_WOLFSSL

/* Error strings */
#undef NO_ERROR_STRINGS

/* ASN.1 */
#define WOLFSSL_ASN_TEMPLATE

/* ------------------------------------------------------------------------- */
/* STSAFE-A120 Integration                                                   */
/* ------------------------------------------------------------------------- */

/* Enable STSAFE-A120 support */
#define WOLFSSL_STSAFEA120

/* Enable crypto callbacks for STSAFE hardware acceleration */
#define WOLF_CRYPTO_CB

/* Use STSAFE for RNG seeding */
#define USE_STSAFE_RNG_SEED

/* Include STSELib header path set in Makefile */

#ifdef __cplusplus
}
#endif

#endif /* USER_SETTINGS_H */

