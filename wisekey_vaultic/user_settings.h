/* user_settings.h
 *
 * Custom configuration for wolfCrypt/wolfSSL.
 * Enabled via WOLFSSL_USER_SETTINGS.
 *
 *
 * Copyright (C) 2023 wolfSSL Inc.
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

#ifndef H_USER_SETTINGS_
#define H_USER_SETTINGS_

/* Include wolfSSL library's options */
#include "wolfssl/options.h"

#if !defined(WOLF_CRYPTO_CB)
#error "Missing WOLF_CRYPTO_CB.  Reconfigure wolfssl with --enable-cryptocb"
#endif

#include "wisekey_vaultic_defs.h"

/* Test/Benchmark Options */
#define BENCH_EMBEDDED
#define BENCH_DEVID
#define NO_MAIN_DRIVER
#define FORCE_DEVID WISEKEY_VAULTIC420_DEVID

#endif /* !H_USER_SETTINGS_ */
