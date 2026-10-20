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

/* wolfSSL user settings for the SHA-2 (SHA-224/256/384/512, SHA-512/224, SHA-512/256) example on the TI C2000 C28x
 * (LAUNCHXL-F28P55X).  Selected by defining WOLFSSL_USER_SETTINGS.
 *
 * The C28x has 16-bit bytes (CHAR_BIT == 16): every wolfCrypt byte buffer
 * holds one octet per 16-bit cell.  The platform settings below are required
 * for that; the last section picks the hash algorithms.
 */

#ifndef WOLFSSL_USER_SETTINGS_C2000_SHA2_H
#define WOLFSSL_USER_SETTINGS_C2000_SHA2_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Platform / C28x toolchain                                                 */
/* ------------------------------------------------------------------------- */
#undef  SINGLE_THREADED
#define SINGLE_THREADED

#undef  WOLFCRYPT_ONLY          /* crypto only, no TLS layer */
#define WOLFCRYPT_ONLY

#undef  NO_FILESYSTEM
#define NO_FILESYSTEM

#undef  NO_WRITEV
#define NO_WRITEV

#undef  NO_MAIN_DRIVER          /* main() lives in the example source */
#define NO_MAIN_DRIVER

#undef  NO_DEV_RANDOM
#define NO_DEV_RANDOM

#undef  WOLFSSL_IGNORE_FILE_WARN
#define WOLFSSL_IGNORE_FILE_WARN

/* C28x integer widths: char/short/int = 16-bit, long = 32-bit,
 * long long = 64-bit. */
#undef  SIZEOF_LONG
#define SIZEOF_LONG       4
#undef  SIZEOF_LONG_LONG
#define SIZEOF_LONG_LONG  8

/* Use the toolchain's <limits.h> so CHAR_BIT is the real value (16 on C28x)
 * instead of wolfSSL's 8-bit fallback. */
#undef  HAVE_LIMITS_H
#define HAVE_LIMITS_H

/* 16-bit int: word16 = unsigned int, word32 = unsigned long.  Without this
 * wolfSSL assumes a 32-bit int and silently truncates 32-bit math. */
#undef  WC_16BIT_CPU
#define WC_16BIT_CPU

/* Portable byte-by-byte paths; a byte buffer must never be cast to a wider
 * word when a byte is 16 bits. */
#undef  WOLFSSL_GENERAL_ALIGNMENT
#define WOLFSSL_GENERAL_ALIGNMENT 2

/* Portable C cores - no assembly. */
#undef  WOLFSSL_NO_ASM
#define WOLFSSL_NO_ASM

/* Build misc.c as its own translation unit.  The TI compiler emits no
 * out-of-line copy of a plain `inline` definition, so the inline-misc path
 * leaves helpers such as xorbuf unresolved at link. */
#undef  NO_INLINE
#define NO_INLINE

/* Hashing only: no random number generator. */
#undef  WC_NO_RNG
#define WC_NO_RNG

/* ------------------------------------------------------------------------- */
/* SHA-2                                                                     */
/* ------------------------------------------------------------------------- */
/* SHA-256 is on by default (NO_SHA256 not defined). */
#undef  WOLFSSL_SHA224
#define WOLFSSL_SHA224
#undef  WOLFSSL_SHA384
#define WOLFSSL_SHA384
#undef  WOLFSSL_SHA512          /* also SHA-512/224 and SHA-512/256 */
#define WOLFSSL_SHA512

/* ------------------------------------------------------------------------- */
/* Everything else off: hashing only                                         */
/* ------------------------------------------------------------------------- */
#undef  NO_BIG_INT              /* no multi-precision math */
#define NO_BIG_INT
#undef  NO_RSA
#define NO_RSA
#undef  NO_DH
#define NO_DH
#undef  NO_DSA
#define NO_DSA
#undef  NO_ASN
#define NO_ASN
#undef  NO_CERTS
#define NO_CERTS
#undef  NO_PWDBASED
#define NO_PWDBASED
#undef  NO_PKCS7
#define NO_PKCS7
#undef  NO_PKCS12
#define NO_PKCS12
#undef  NO_SIG_WRAPPER
#define NO_SIG_WRAPPER
#undef  NO_AES
#define NO_AES
#undef  NO_DES3
#define NO_DES3
#undef  NO_RC4
#define NO_RC4
#undef  NO_MD4
#define NO_MD4
#undef  NO_MD5
#define NO_MD5
#undef  NO_SHA                  /* SHA-1 */
#define NO_SHA
#undef  NO_HMAC
#define NO_HMAC

#ifdef __cplusplus
}
#endif

#endif /* WOLFSSL_USER_SETTINGS_C2000_SHA2_H */
