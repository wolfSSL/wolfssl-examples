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

/* wolfSSL configuration */
#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

#define WOLFSSL_TLS13
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_FFDHE_2048
#define HAVE_HKDF
#define HAVE_AEAD
#define HAVE_CHACHA
#define HAVE_POLY1305
#define WOLFSSL_AES_COUNTER
#define WOLFSSL_AES_DIRECT
#define HAVE_AES_ECB
#define HAVE_AES_CBC
#define HAVE_AES_GCM
#define HAVE_AESGCM
#define HAVE_CURVE25519
#define HAVE_ED25519
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512
#define WOLFSSL_SHA224
#define WOLFSSL_SHA3
#define WOLFSSL_SHAKE256

#endif /* USER_SETTINGS_H */
