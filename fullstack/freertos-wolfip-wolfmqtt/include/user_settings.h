/* user_settings.h
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

/* Debug */
#ifndef DEBUG_WOLFSSL
#define DEBUG_WOLFSSL
#endif

/* Math */
#define USE_FAST_MATH
#define TFM_TIMING_RESISTANT

/* TLS */
#define WOLFSSL_TLS13
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_EXTENDED_MASTER
#define HAVE_ENCRYPT_THEN_MAC

/* Crypto */
#define HAVE_AESGCM
#define HAVE_CHACHA
#define HAVE_POLY1305
#define WOLFSSL_SHA512
#define HAVE_ECC
#define HAVE_CURVE25519
#define HAVE_ED25519
#define ECC_TIMING_RESISTANT
#define WC_RSA_BLINDING

/* Features */
#define WOLFSSL_CERT_GEN
#define WOLFSSL_KEY_GEN
#define WOLFSSL_DES_ECB
#define HAVE_HKDF
#define HAVE_X963_KDF
#define WOLFSSL_BASE64_ENCODE

/* Disable */
#define NO_DSA
#define NO_RC4
#define NO_HC128
#define NO_RABBIT
#define NO_PSK
#define NO_MD4
#define NO_PWDBASED

#endif /* USER_SETTINGS_H */
