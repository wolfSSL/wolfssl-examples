/* proxy-common.h
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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

/* Shared configuration for the PQC crypto-proxy examples.
 *
 * Two proxy directions are demonstrated:
 *
 *   1. Quantum-safe front door (pq-proxy):
 *     pq-client =(PQC TLS 1.3)=> pq-proxy =(legacy TLS 1.2)=> legacy-server
 *
 *   2. Quantum-safe upgrade (upgrade-proxy):
 *    legacy-client =(legacy TLS 1.2)=> upgrade-proxy =(PQC TLS 1.3)=> pq-server
 *
 * In direction 1 the modern client gets a post-quantum key exchange while the
 * legacy origin is left untouched. In direction 2 a legacy client that cannot
 * speak PQC is transparently upgraded to a quantum-safe connection to a modern
 * server. Either way the post-quantum (hybrid) key exchange protects the leg
 * crossing the untrusted network against "harvest now, decrypt later" attacks.
 *
 * For direction 1 the macros below are named by position (FRONTEND_* is the
 * PQC leg, BACKEND_* is the legacy leg). The neutral PQC_* / LEGACY_* aliases
 * near the end of this file let direction 2 refer to each leg by its
 * cryptography instead of its position.
 */

#ifndef PROXY_COMMON_H
#define PROXY_COMMON_H

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

/* Ports used by the examples. */
#define FRONTEND_PORT 11111   /* dir 1: clients connect to pq-proxy   */
#define BACKEND_PORT  11112   /* dir 1: the legacy origin listens here */
#define BACKEND_HOST  "127.0.0.1"

#define UPGRADE_PORT  22221   /* dir 2: legacy clients connect here    */
#define PQSERVER_PORT 22222   /* dir 2: the PQC origin listens here    */
#define PQSERVER_HOST "127.0.0.1"

/* ---- Frontend: post-quantum / hybrid TLS 1.3 ----------------------------- */

/* Hybrid key exchange: X25519-style ECDHE (here NIST P-256) combined with
 * ML-KEM-768. The classical half keeps interop guarantees while the ML-KEM
 * half provides the post-quantum protection. Override at build time with
 * -DFRONTEND_GROUP=WOLFSSL_SECP384R1MLKEM1024 etc. for a higher security
 * level, or a pure WOLFSSL_ML_KEM_768 group for non-hybrid PQC. */
#ifndef FRONTEND_GROUP
#define FRONTEND_GROUP      WOLFSSL_SECP256R1MLKEM768
#define FRONTEND_GROUP_NAME "P256_ML_KEM_768"
#endif
#ifndef FRONTEND_GROUP_NAME
#define FRONTEND_GROUP_NAME "(custom)"
#endif

/* Server authentication on the frontend.
 *
 * If the wolfSSL library was built with ML-DSA (Dilithium) support we use a
 * post-quantum certificate chain so that BOTH key exchange and authentication
 * are quantum-safe. Otherwise we fall back to a classical RSA certificate,
 * which still gives a post-quantum *key exchange* - the most common real-world
 * PQC deployment today, since CAs do not yet issue PQC certificates. */
#ifdef HAVE_DILITHIUM
    #define FRONTEND_CERT  "../../certs/mldsa65_entity_cert.pem"
    #define FRONTEND_KEY   "../../certs/mldsa65_entity_key.pem"
    #define FRONTEND_CA    "../../certs/mldsa65_root_cert.pem"
    #define FRONTEND_AUTH  "ML-DSA-65 (post-quantum)"
#else
    #define FRONTEND_CERT  "../../certs/server-cert.pem"
    #define FRONTEND_KEY   "../../certs/server-key.pem"
    #define FRONTEND_CA    "../../certs/ca-cert.pem"
    #define FRONTEND_AUTH  "RSA-2048 (classical)"
#endif

/* ---- Backend: legacy classical TLS 1.2 ----------------------------------- */

/* A deliberately old-school, non-PQC cipher suite. ECDHE-RSA still gives
 * forward secrecy but offers no protection against a quantum adversary. */
#define BACKEND_CIPHERS  "ECDHE-RSA-AES128-GCM-SHA256"
#define BACKEND_CA       "../../certs/ca-cert.pem"
#define BACKEND_CERT     "../../certs/server-cert.pem"
#define BACKEND_KEY      "../../certs/server-key.pem"

/* ---- Neutral aliases ----------------------------------------------------- */

/* Refer to each leg by its cryptography rather than its position, so the
 * upgrade-proxy (legacy frontend, PQC backend) reads naturally too. */
#define PQC_GROUP        FRONTEND_GROUP
#define PQC_GROUP_NAME   FRONTEND_GROUP_NAME
#define PQC_CERT         FRONTEND_CERT
#define PQC_KEY          FRONTEND_KEY
#define PQC_CA           FRONTEND_CA
#define PQC_AUTH         FRONTEND_AUTH

#define LEGACY_CIPHERS   BACKEND_CIPHERS
#define LEGACY_CERT      BACKEND_CERT
#define LEGACY_KEY       BACKEND_KEY
#define LEGACY_CA        BACKEND_CA

#define PROXY_BUFFER_SZ  4096

#endif /* PROXY_COMMON_H */
