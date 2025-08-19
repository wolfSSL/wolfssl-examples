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

/* wolfSSL settings */
#ifndef WOLFSSL_USER_SETTINGS
#define WOLFSSL_USER_SETTINGS
#endif

#ifndef WOLFSSL_TLS13
#define WOLFSSL_TLS13
#endif

#ifndef HAVE_TLS_EXTENSIONS
#define HAVE_TLS_EXTENSIONS
#endif

#ifndef HAVE_SUPPORTED_CURVES
#define HAVE_SUPPORTED_CURVES
#endif

#ifndef HAVE_AESGCM
#define HAVE_AESGCM
#endif

#ifndef HAVE_CHACHA
#define HAVE_CHACHA
#endif

#ifndef HAVE_POLY1305
#define HAVE_POLY1305
#endif

#ifndef HAVE_ECC
#define HAVE_ECC
#endif

#ifndef HAVE_CURVE25519
#define HAVE_CURVE25519
#endif

#ifndef HAVE_ED25519
#define HAVE_ED25519
#endif

/* Required for ED25519 */
#ifndef WOLFSSL_SHA512
#define WOLFSSL_SHA512
#endif

#ifndef TFM_TIMING_RESISTANT
#define TFM_TIMING_RESISTANT
#endif

#ifndef ECC_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT
#endif

#ifndef WC_RSA_BLINDING
#define WC_RSA_BLINDING
#endif

/* wolfIP settings */
#ifndef MAX_TCPSOCKETS
#define MAX_TCPSOCKETS 8
#endif

#ifndef MAX_UDPSOCKETS
#define MAX_UDPSOCKETS 8
#endif

#ifndef TXBUF_SIZE
#define TXBUF_SIZE 8192
#endif

#ifndef RXBUF_SIZE
#define RXBUF_SIZE 8192
#endif

#ifndef LINK_MTU
#define LINK_MTU 1500
#endif

/* Debug settings */
#ifndef WOLFSSL_DEBUG
#define WOLFSSL_DEBUG
#endif

#ifndef WOLFMQTT_DEBUG
#define WOLFMQTT_DEBUG
#endif

#ifndef WOLFIP_DEBUG
#define WOLFIP_DEBUG
#endif

/* Undefine DEBUG to avoid conflicts */
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

/* Undefine MAX_TIMERS to avoid conflicts */
#ifdef MAX_TIMERS
#undef MAX_TIMERS
#endif
#define MAX_TIMERS (MAX_TCPSOCKETS * 3)

/* MQTT TLS settings */
#ifndef ENABLE_MQTT_TLS
#define ENABLE_MQTT_TLS
#endif

#endif /* USER_SETTINGS_H */
