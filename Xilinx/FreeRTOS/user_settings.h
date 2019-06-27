/* user_settings.h
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
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

/* custom user settings for build */
#ifndef __USER_SETTINGS_H
#define __USER_SETTINGS_H

/* for simple TLS demonstration */

/* Hardware acceleration to use */

/* Uncomment for ARMv8 hardware acceleration */
//#define WOLFSSL_ARMASM /* ARMv8 hardware acceleration */

/* Uncomment for Xilinx hardened crypto */
#define WOLFSSL_XILINX_CRYPT /* Xilinx hardware acceleration */

/* Math setup and testing macros */
#define WOLFSSL_GENSEED_FORTEST /* currently using testing seed generation */
#define NO_MAIN_DRIVER
#define BENCH_EMBEDDED
#define USE_FAST_MATH
#define FP_MAX_BITS 8192 /* handle 4096 bit RSA key */
//#define DEBUG_WOLFSSL

/* ECC and curve operations */
#define HAVE_ECC
#define ALT_ECC_SIZE
#define HAVE_CURVE25519
#define HAVE_ED25519
#define HAVE_TLS_EXTENSIONS

/* enable harden settings */
#define ECC_TIMING_RESISTANT
#define TFM_TIMING_RESISTANT
#define WC_RSA_BLINDING

/* SHA operations */
#define WOLFSSL_SHA3
#define WOLFSSL_SHA224
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512

/* AES operations */
#define HAVE_AESGCM
#define WOLFSSL_AES_DIRECT
#define WOLFSSL_CMAC

/* Use buffers and no file system */
#define USE_CERT_BUFFERS_2048
#define USE_CERT_BUFFERS_256
#define NO_FILESYSTEM

/* chacha - poly suites */
#define HAVE_CHACHA
#define HAVE_POLY1305
#define HAVE_ONE_TIME_AUTH


/* Can be uncommented in settings.h but to make example drop and
 * run am defining it here */
#define FREERTOS
#define WOLFSSL_XILINX
#define WOLFSSL_LWIP

#define XREALLOC(p, n, h, t) realloc((p), (n))

#endif
