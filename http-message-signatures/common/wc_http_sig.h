/* wc_http_sig.h
 *
 * RFC 9421 HTTP Message Signatures using wolfCrypt Ed25519.
 *
 * Minimal v1 implementation covering:
 *   - Derived components: @method, @authority, @path, @query
 *   - Arbitrary HTTP header fields
 *   - Ed25519 signing and verification
 *   - Single signature (sig1)
 *   - Timestamp-based replay protection
 *
 * Note: The wc_ prefix is used for consistency with wolfCrypt
 * naming but these are example functions, not part of wolfCrypt.
 * Linking this code alongside a future wolfCrypt that adds
 * identically-named symbols would cause conflicts.
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

#ifndef WC_HTTP_SIG_H
#define WC_HTTP_SIG_H

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/types.h>

#ifdef HAVE_ED25519

#include <wolfssl/wolfcrypt/ed25519.h>

/* Maximum sizes for internal buffers.
 * Reduce WC_HTTPSIG_MAX_SIG_BASE on stack-constrained embedded targets. */
#define WC_HTTPSIG_MAX_SIG_BASE    4096
#define WC_HTTPSIG_MAX_COMPONENTS    16
#define WC_HTTPSIG_MAX_LABEL         64

/* Default signature label */
#define WC_HTTPSIG_DEFAULT_LABEL   "sig1"

/* HTTP header name-value pair.
 * Header name matching is case-insensitive. Leading and trailing
 * whitespace on values is trimmed during signature base construction.
 * Duplicate headers must be provided as separate entries. */
typedef struct {
    const char* name;
    const char* value;
} wc_HttpHeader;

#ifdef HAVE_ED25519_SIGN
/* Sign an HTTP request per RFC 9421.
 *
 * method      - HTTP method, e.g. "POST" (uppercase)
 * authority   - Host authority, e.g. "example.com" (lowercase)
 * path        - Request path, e.g. "/foo" (no query string)
 * query       - Query string including "?", e.g. "?x=1", or NULL to omit
 * headers     - Array of additional HTTP headers to include in signature
 * headerCount - Number of entries in headers array
 * key         - Ed25519 private key (must have both private and public parts)
 * keyId       - Key identifier string, included in Signature-Input
 * created     - UNIX timestamp for signature creation (0 = use current time)
 *               Note: on platforms where long is 32-bit, timestamps after
 *               2038-01-19 will overflow. See Y2038.
 *
 * sigOut      - Output buffer for Signature header value
 * sigOutSz    - In: buffer size. Out: bytes written (NUL-term).
 * inputOut    - Output buffer for Signature-Input value
 * inputOutSz  - In: buffer size. Out: bytes written (NUL-term).
 *
 * Header names are automatically lowercased per RFC 9421 Section 2.1.
 *
 * Returns 0 on success, negative on error. */
int wc_HttpSig_Sign(
    const char*          method,
    const char*          authority,
    const char*          path,
    const char*          query,
    const wc_HttpHeader* headers,
    int                  headerCount,
    ed25519_key*         key,
    const char*          keyId,
    long                 created,
    char*                sigOut,
    word32*              sigOutSz,
    char*                inputOut,
    word32*              inputOutSz
);
#endif /* HAVE_ED25519_SIGN */

/* Extract the keyid parameter from a Signature-Input header value.
 *
 * signatureInput - The Signature-Input header value
 * label          - Signature label to look up, or NULL for first member
 * keyIdOut       - Output buffer for the extracted keyid string
 * keyIdOutSz     - On input: buffer size. On output: bytes written.
 *
 * Returns 0 on success, negative on error. */
int wc_HttpSig_GetKeyId(
    const char*          signatureInput,
    const char*          label,
    char*                keyIdOut,
    word32*              keyIdOutSz
);

#ifdef HAVE_ED25519_VERIFY
/* Verify an HTTP request signature per RFC 9421.
 *
 * method          - HTTP method (uppercase)
 * authority       - Host authority (lowercase)
 * path            - Request path (no query string)
 * query           - Query string including "?", or NULL
 * headers         - Array of HTTP headers from the request
 * headerCount     - Number of entries in headers array
 * signature       - Signature header value (e.g. "sig1=:b64:")
 * signatureInput  - Signature-Input header value
 * label           - Signature label to verify, or NULL for first member
 * pubKey          - Ed25519 public key
 * maxAgeSec       - Maximum allowed age of signature in seconds (0 = skip)
 *
 * Returns 0 on success (valid signature), negative on error. */
int wc_HttpSig_Verify(
    const char*          method,
    const char*          authority,
    const char*          path,
    const char*          query,
    const wc_HttpHeader* headers,
    int                  headerCount,
    const char*          signature,
    const char*          signatureInput,
    const char*          label,
    ed25519_key*         pubKey,
    int                  maxAgeSec
);
#endif /* HAVE_ED25519_VERIFY */

#endif /* HAVE_ED25519 */

#endif /* WC_HTTP_SIG_H */
