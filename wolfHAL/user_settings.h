/* user_settings.h
 *
 * wolfCrypt configuration for the STM32WB55 Nucleo + wolfHAL example.
 * AES is offloaded to the AES1 peripheral, SHA-256 stays in software.
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

#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

/* wolfHAL port. Enabling it pulls in wolfhal_settings.h, which turns on
 * WOLF_CRYPTO_CB and points WC_USE_DEVID at the wolfHAL device, so the stock
 * wolfcrypt test routes through the hardware without changes. */
#define WOLFSSL_WOLFHAL

#define WOLFSSL_WOLFHAL_RNG

/* wolfCrypt only — no TLS */
#define WOLFCRYPT_ONLY

/* Small embedded target */
#define SINGLE_THREADED
#define NO_FILESYSTEM
#define NO_WOLFSSL_DIR
#define NO_WRITEV
#define NO_MAIN_DRIVER
#define NO_DEV_RANDOM

/* Benchmark: BENCH_EMBEDDED shrinks the working buffers, WOLFSSL_USER_CURRTIME
 * makes benchmark.c call the current_time() this example supplies rather than
 * reaching for a host clock it has no way to find. */
#define BENCH_EMBEDDED
#define WOLFSSL_USER_CURRTIME

/* AES — offloaded to AES1 via wolfHAL. wolfHAL_board.h names a device for each
 * of these, so all four are dispatched to hardware; a mode left unnamed there
 * would fall back to wolfCrypt's software implementation instead. */
#define HAVE_AES_CBC
#define HAVE_AESGCM
#define HAVE_AESCCM
#define HAVE_AES_ECB
#define WOLFSSL_AES_DIRECT
#define NO_AES_192

/* Disable everything else */
#define NO_RSA
#define NO_DSA
#define NO_DH
#define NO_RC4
#define NO_MD4
#define NO_MD5
#define NO_SHA
#define NO_DES3
#define NO_PSK
#define NO_PWDBASED
#define NO_OLD_TLS
#define NO_ASN
#define NO_CODING
#define NO_SIG_WRAPPER
#define NO_HMAC
#define WOLFSSL_NO_PEM

#endif /* USER_SETTINGS_H */
