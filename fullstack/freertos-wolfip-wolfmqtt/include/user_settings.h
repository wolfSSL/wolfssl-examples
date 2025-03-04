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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* Security hardening */
#define WC_RSA_BLINDING
#define ECC_TIMING_RESISTANT
#define WC_TIMING_RESISTANT

/* TLS Certificate Paths */
#define MQTT_TLS_CA_CERT     "../../../wolfssl/certs/ca-cert.pem"
#define MQTT_TLS_CLIENT_CERT "../../../wolfssl/certs/client-cert.pem"
#define MQTT_TLS_CLIENT_KEY  "../../../wolfssl/certs/client-key.pem"

/* wolfSSL TLS configuration */
#define WOLFSSL_TLS13
#define WOLFSSL_TLS13_NO_OLD_TLS
#define WOLFSSL_CERT_GEN
#define WOLFSSL_DES_ECB
#define HAVE_HKDF
#define HAVE_AEAD
#define HAVE_SUPPORTED_CURVES
#define WOLFSSL_AES_DIRECT
#define HAVE_TLS_EXTENSIONS
#define HAVE_SNI
#define HAVE_OCSP
#define HAVE_CERTIFICATE_STATUS_REQUEST
#define WOLFSSL_ALWAYS_VERIFY_CB
#define WOLFSSL_VERIFY_CB_ALL_CERTS

/* wolfMQTT configuration */
#define WOLFMQTT_NO_ERROR_STRINGS
#define WOLFMQTT_NONBLOCK
#define WOLFMQTT_ENABLE_NONBLOCK
#define ENABLE_MQTT_TLS
#define MQTT_MAX_PACKET_SIZE 1024
#define MQTT_DEFAULT_CMD_TIMEOUT_MS 5000

/* wolfIP configuration */
#define WOLFIP_DEBUG
#define WOLFIP_CHECKSUM_VERIFY

/* Socket configuration */
#define WOLFIP_AF_INET     2
#define WOLFIP_SOCK_STREAM 1
#define WOLFIP_SOCK_DGRAM  2

#endif /* USER_SETTINGS_H */
