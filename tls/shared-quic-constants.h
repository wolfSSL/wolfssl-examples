/* shared-quic-constants.h
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

/* Constants shared by client-quic and server-quic.
 *
 * The two programs talk over UDP, but not in QUIC. Each datagram is simply
 *
 *     [encryption level, 1 byte][TLS handshake bytes]
 *
 * which is the least a stand-in transport can carry and still let wolfSSL
 * do its part. Application data gets its own format, described below.
 * Where a real QUIC stack would build packets, encrypt them and
 * track acknowledgements, the demos leave a comment saying so instead.
 */

#ifndef SHARED_QUIC_CONSTANTS_H
#define SHARED_QUIC_CONSTANTS_H

#define QUIC_DEMO_PORT          11111
#define QUIC_DEMO_DGRAM_MAX     1200    /* the minimum datagram size every QUIC
                                         * path must carry (RFC 9000 14.1) */
#define QUIC_DEMO_TIMEOUT_SEC   5

#define QUIC_DEMO_CA_FILE       "../certs/ca-cert.pem"
#define QUIC_DEMO_CERT_FILE     "../certs/server-cert.pem"
#define QUIC_DEMO_KEY_FILE      "../certs/server-key.pem"

/* After the handshake the client sends one protected message, the server
 * answers it, and the client follows up with QUIC_DEMO_SHUTDOWN. Each is
 *
 *     [QUIC_DEMO_APP_PACKET][packet number, 1 byte][ciphertext + tag]
 *
 * The type sits above every encryption level so the two never mix. */
#define QUIC_DEMO_APP_PACKET    0x04
#define QUIC_DEMO_APP_HDR_LEN   2
#define QUIC_DEMO_SECRET_MAX    64      /* SHA-384 secrets are the largest */
#define QUIC_DEMO_KEY_MAX       32      /* AES-256 and ChaCha20 */
#define QUIC_DEMO_IV_LEN        12      /* every QUIC AEAD uses a 12-byte IV */
#define QUIC_DEMO_TAG_MAX       16

/* A client message that ends the connection and stops the server */
#define QUIC_DEMO_SHUTDOWN      "shutdown"

/* QUIC insists on ALPN, so both sides must agree on a protocol name */
#define QUIC_DEMO_ALPN          "quic-demo"
#define QUIC_DEMO_ALPN_SZ       (sizeof(QUIC_DEMO_ALPN) - 1)

/* Printable name for a WOLFSSL_ENCRYPTION_LEVEL */
#define QUIC_DEMO_LEVEL_NAME(lvl)                                   \
    ((lvl) == wolfssl_encryption_initial     ? "initial"     :      \
     (lvl) == wolfssl_encryption_early_data  ? "early_data"  :      \
     (lvl) == wolfssl_encryption_handshake   ? "handshake"   :      \
     (lvl) == wolfssl_encryption_application ? "application" : "unknown")

#endif /* SHARED_QUIC_CONSTANTS_H */
