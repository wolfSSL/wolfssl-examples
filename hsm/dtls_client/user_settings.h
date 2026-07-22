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

#ifndef USER_SETTINGS_H_
#define USER_SETTINGS_H_


/* POSIX system headers */
#define HAVE_SYS_TIME_H

/** wolfHSM Client required settings */
/* CryptoCB support - required for offloading crypto to HSM */
#define WOLF_CRYPTO_CB
#define HAVE_ANONYMOUS_INLINE_AGGREGATES 1

/* PK callbacks - required for TLS-level HSM key operations */
#define HAVE_PK_CALLBACKS

/* Enable DTLS support */
#define WOLFSSL_DTLS
#define WOLFSSL_DTLS13
#define WOLFSSL_TLS13
#define HAVE_TLS_EXTENSIONS
#define WOLFSSL_SEND_HRR_COOKIE

/* Remove old TLS versions */
#define NO_OLD_TLS

/** Crypto Algorithm Options */

/* ECC for ECDHE key exchange and ECDSA authentication */
#define HAVE_ECC
#define HAVE_SUPPORTED_CURVES

/* AES-GCM for symmetric encryption */
#define HAVE_AESGCM

/* HKDF for key derivation */
#define HAVE_HKDF

/* Timing resistance / side-channel attack protection */
#define TFM_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT
#define WC_RSA_BLINDING

/* Use wolfSSL's internal string comparison instead of system strcasecmp */
#define USE_WOLF_STRCASECMP

/* Remove unneeded features */
#define NO_MAIN_DRIVER
#define NO_DO178
#define NO_RSA
#define NO_DH

#endif /* USER_SETTINGS_H_ */
