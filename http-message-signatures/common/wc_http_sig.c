/* wc_http_sig.c
 *
 * RFC 9421 HTTP Message Signatures using wolfCrypt Ed25519.
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

#include "wc_http_sig.h"
#include "wc_sf.h"

#ifdef HAVE_ED25519

#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <limits.h>

/* --- Internal helpers --- */

static int to_lower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

/* Lowercase a string in-place (RFC 9421 Section 2.1 requires lowercased
 * component identifiers for HTTP field names). */
static void str_to_lower(char* s)
{
    while (*s) {
        *s = (char)to_lower((unsigned char)*s);
        s++;
    }
}

/* Case-insensitive string comparison (RFC 9421 requires case-insensitive
 * header name matching per HTTP semantics). */
static int strcasecmp_internal(const char* a, const char* b)
{
    while (*a && *b) {
        int ca = to_lower((unsigned char)*a);
        int cb = to_lower((unsigned char)*b);
        if (ca != cb)
            return ca - cb;
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

/* Skip leading OWS (optional whitespace: SP / HTAB) per RFC 9110.
 * Intentionally duplicated from wc_sf.c for module independence. */
static const char* skip_ows(const char* p)
{
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

/* Return length of value excluding trailing OWS */
static int trimmed_len(const char* val)
{
    int len = (int)XSTRLEN(val);
    while (len > 0 && (val[len - 1] == ' ' || val[len - 1] == '\t'))
        len--;
    return len;
}

/* Check if a derived component name is one we support.
 * Returns 1 if known, 0 if unknown. */
static int is_known_derived(const char* name)
{
    return (XSTRCMP(name, "@method") == 0 ||
            XSTRCMP(name, "@authority") == 0 ||
            XSTRCMP(name, "@path") == 0 ||
            XSTRCMP(name, "@query") == 0);
}

/* --- Internal: resolve a component value from request data --- */

/* Lookup the value for a component identifier.
 * Derived components start with '@'; everything else is a header name.
 * Header name matching is case-insensitive per HTTP semantics. */
static const char* resolve_component(const char* name,
                                     const char* method,
                                     const char* authority,
                                     const char* path,
                                     const char* query,
                                     const wc_HttpHeader* headers,
                                     int headerCount)
{
    int i;

    if (name[0] == '@') {
        if (XSTRCMP(name, "@method") == 0)    return method;
        if (XSTRCMP(name, "@authority") == 0) return authority;
        if (XSTRCMP(name, "@path") == 0)      return path;
        if (XSTRCMP(name, "@query") == 0)     return query;
        return NULL;
    }

    /* Search headers case-insensitively */
    for (i = 0; i < headerCount; i++) {
        if (strcasecmp_internal(headers[i].name, name) == 0)
            return headers[i].value;
    }
    return NULL;
}

/* --- Internal: build signature base (RFC 9421 Section 2.5) --- */

/* Append to buffer with bounds checking. Returns new position or -1. */
static int buf_append(char* buf, int pos, int maxSz, const char* str)
{
    int len = (int)XSTRLEN(str);
    if (pos + len >= maxSz)
        return -1;
    XMEMCPY(buf + pos, str, len);
    return pos + len;
}

/* Build the signature base from components and request data.
 *
 * The signature base is:
 *   For each component in order:
 *     "<component-id>": <value>\n
 *   Final line (no trailing \n):
 *     "@signature-params": <sig-params-value>
 */
static int build_signature_base(
    const char**         componentNames,
    int                  componentCount,
    const char*          method,
    const char*          authority,
    const char*          path,
    const char*          query,
    const wc_HttpHeader* headers,
    int                  headerCount,
    const char*          sigParamsValue,
    char*                baseOut,
    word32*              baseOutSz)
{
    char* buf = baseOut;
    int pos = 0;
    int maxSz;
    int i;

    if (*baseOutSz > (word32)INT_MAX)
        return BAD_FUNC_ARG;
    maxSz = (int)*baseOutSz;

    for (i = 0; i < componentCount; i++) {
        const char* val;
        int valLen;

        val = resolve_component(componentNames[i],
                                method, authority, path, query,
                                headers, headerCount);
        if (!val)
            return BAD_FUNC_ARG;

        pos = buf_append(buf, pos, maxSz, "\"");
        if (pos < 0) return BUFFER_E;
        pos = buf_append(buf, pos, maxSz, componentNames[i]);
        if (pos < 0) return BUFFER_E;
        pos = buf_append(buf, pos, maxSz, "\": ");
        if (pos < 0) return BUFFER_E;

        if (componentNames[i][0] != '@') {
            val = skip_ows(val);
            valLen = trimmed_len(val);
            if (pos + valLen >= maxSz)
                return BUFFER_E;
            XMEMCPY(buf + pos, val, valLen);
            pos += valLen;
        } else {
            pos = buf_append(buf, pos, maxSz, val);
            if (pos < 0) return BUFFER_E;
        }

        pos = buf_append(buf, pos, maxSz, "\n");
        if (pos < 0) return BUFFER_E;
    }

    pos = buf_append(buf, pos, maxSz, "\"@signature-params\": ");
    if (pos < 0) return BUFFER_E;
    pos = buf_append(buf, pos, maxSz, sigParamsValue);
    if (pos < 0) return BUFFER_E;

    *baseOutSz = (word32)pos;
    return 0;
}

/* --- Public API: Sign --- */

#ifdef HAVE_ED25519_SIGN
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
    word32*              inputOutSz)
{
    int ret;
    wc_SfSigInput sfIn;
    char sigParams[1024];
    word32 sigParamsSz;
    char sigBase[WC_HTTPSIG_MAX_SIG_BASE];
    word32 sigBaseSz;
    byte rawSig[ED25519_SIG_SIZE];
    word32 rawSigSz = ED25519_SIG_SIZE;
    const char* componentNames[WC_HTTPSIG_MAX_COMPONENTS];
    int componentCount = 0;
    int i;

    if (!method || !authority || !path || !key || !keyId ||
        !sigOut || !sigOutSz || !inputOut || !inputOutSz)
        return BAD_FUNC_ARG;
    if (headerCount < 0 || (headerCount > 0 && !headers))
        return BAD_FUNC_ARG;

    if (created == 0)
        created = (long)XTIME(NULL);

    /* Build wc_SfSigInput structure with component names.
     * Derived components first, then headers.
     * Header names are lowercased per RFC 9421 Section 2.1. */
    XMEMSET(&sfIn, 0, sizeof(sfIn));
    XSTRNCPY(sfIn.label, WC_HTTPSIG_DEFAULT_LABEL, WC_SF_MAX_LABEL - 1);

    if (3 + (query ? 1 : 0) + headerCount > WC_HTTPSIG_MAX_COMPONENTS)
        return BUFFER_E;

    XSTRNCPY(sfIn.items[componentCount], "@method", WC_SF_MAX_STRING - 1);
    componentNames[componentCount] = sfIn.items[componentCount];
    componentCount++;

    XSTRNCPY(sfIn.items[componentCount], "@authority", WC_SF_MAX_STRING - 1);
    componentNames[componentCount] = sfIn.items[componentCount];
    componentCount++;

    XSTRNCPY(sfIn.items[componentCount], "@path", WC_SF_MAX_STRING - 1);
    componentNames[componentCount] = sfIn.items[componentCount];
    componentCount++;

    if (query) {
        XSTRNCPY(sfIn.items[componentCount], "@query", WC_SF_MAX_STRING - 1);
        componentNames[componentCount] = sfIn.items[componentCount];
        componentCount++;
    }

    for (i = 0; i < headerCount; i++) {
        if (componentCount >= WC_HTTPSIG_MAX_COMPONENTS)
            return BUFFER_E;
        XSTRNCPY(sfIn.items[componentCount], headers[i].name,
                 WC_SF_MAX_STRING - 1);
        str_to_lower(sfIn.items[componentCount]);
        componentNames[componentCount] = sfIn.items[componentCount];
        componentCount++;
    }
    sfIn.itemCount = componentCount;

    XSTRNCPY(sfIn.params[0].name, "created", WC_SF_MAX_LABEL - 1);
    sfIn.params[0].type = WC_SF_PARAM_INTEGER;
    sfIn.params[0].intVal = created;

    XSTRNCPY(sfIn.params[1].name, "keyid", WC_SF_MAX_LABEL - 1);
    sfIn.params[1].type = WC_SF_PARAM_STRING;
    XSTRNCPY(sfIn.params[1].strVal, keyId, WC_SF_MAX_STRING - 1);

    XSTRNCPY(sfIn.params[2].name, "alg", WC_SF_MAX_LABEL - 1);
    sfIn.params[2].type = WC_SF_PARAM_STRING;
    XSTRNCPY(sfIn.params[2].strVal, "ed25519", WC_SF_MAX_STRING - 1);
    sfIn.paramCount = 3;

    sigParamsSz = (word32)sizeof(sigParams);
    ret = wc_SfGenSigParams(&sfIn, sigParams, &sigParamsSz);
    if (ret != 0)
        return ret;

    sigBaseSz = (word32)sizeof(sigBase);
    ret = build_signature_base(
        componentNames, componentCount,
        method, authority, path, query,
        headers, headerCount,
        sigParams,
        sigBase, &sigBaseSz);
    if (ret != 0)
        return ret;

    ret = wc_ed25519_sign_msg((const byte*)sigBase, sigBaseSz,
                              rawSig, &rawSigSz, key);
    if (ret != 0)
        return ret;

    ret = wc_SfGenSigValue(WC_HTTPSIG_DEFAULT_LABEL,
                           rawSig, rawSigSz,
                           sigOut, sigOutSz);
    if (ret != 0)
        return ret;

    ret = wc_SfGenSigInput(&sfIn, inputOut, inputOutSz);
    if (ret != 0)
        return ret;

    return 0;
}

#endif /* HAVE_ED25519_SIGN */

/* --- Public API: GetKeyId --- */

int wc_HttpSig_GetKeyId(
    const char*          signatureInput,
    const char*          label,
    char*                keyIdOut,
    word32*              keyIdOutSz)
{
    int ret, i;
    wc_SfSigInput parsed;

    if (!signatureInput || !keyIdOut || !keyIdOutSz)
        return BAD_FUNC_ARG;

    ret = wc_SfParseSigInput(signatureInput, label, &parsed);
    if (ret != 0)
        return ret;

    for (i = 0; i < parsed.paramCount; i++) {
        if (XSTRCMP(parsed.params[i].name, "keyid") == 0) {
            word32 len;
            if (parsed.params[i].type != WC_SF_PARAM_STRING)
                return BAD_FUNC_ARG;
            len = (word32)XSTRLEN(parsed.params[i].strVal);
            if (len + 1 > *keyIdOutSz)
                return BUFFER_E;
            XMEMCPY(keyIdOut, parsed.params[i].strVal, len + 1);
            *keyIdOutSz = len;
            return 0;
        }
    }

    return MISSING_KEY;
}

/* --- Public API: Verify --- */

#ifdef HAVE_ED25519_VERIFY
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
    int                  maxAgeSec)
{
    int ret, i;
    wc_SfSigInput parsed;
    byte rawSig[ED25519_SIG_SIZE];
    word32 rawSigSz = ED25519_SIG_SIZE;
    char usedLabel[WC_SF_MAX_LABEL];
    char sigParams[1024];
    word32 sigParamsSz;
    char sigBase[WC_HTTPSIG_MAX_SIG_BASE];
    word32 sigBaseSz;
    const char* componentNames[WC_HTTPSIG_MAX_COMPONENTS];
    int verifyRes = 0;

    if (!method || !authority || !path || !signature || !signatureInput ||
        !pubKey)
        return BAD_FUNC_ARG;
    if (headerCount < 0 || (headerCount > 0 && !headers))
        return BAD_FUNC_ARG;

    /* Parse Signature-Input to get covered components and params */
    ret = wc_SfParseSigInput(signatureInput, label, &parsed);
    if (ret != 0)
        return ret;

    XSTRNCPY(usedLabel, parsed.label, WC_SF_MAX_LABEL - 1);
    usedLabel[WC_SF_MAX_LABEL - 1] = '\0';

    /* Check timestamp if requested */
    if (maxAgeSec > 0) {
        long createdTime = 0;
        int found = 0;
        for (i = 0; i < parsed.paramCount; i++) {
            if (XSTRCMP(parsed.params[i].name, "created") == 0 &&
                parsed.params[i].type == WC_SF_PARAM_INTEGER) {
                createdTime = parsed.params[i].intVal;
                found = 1;
                break;
            }
        }
        if (!found)
            return BAD_FUNC_ARG;

        {
            long now = (long)XTIME(NULL);
            long age = now - createdTime;
            if (age < 0 || age > maxAgeSec)
                return SIG_VERIFY_E;
        }
    }

    /* Enforce algorithm: if alg is present, it must be ed25519 */
    for (i = 0; i < parsed.paramCount; i++) {
        if (XSTRCMP(parsed.params[i].name, "alg") == 0) {
            if (parsed.params[i].type != WC_SF_PARAM_STRING ||
                XSTRCMP(parsed.params[i].strVal, "ed25519") != 0)
                return BAD_FUNC_ARG;
            break;
        }
    }

    /* Extract raw signature bytes */
    rawSigSz = ED25519_SIG_SIZE;
    ret = wc_SfParseSigValue(signature, usedLabel, rawSig, &rawSigSz);
    if (ret != 0)
        return ret;
    if (rawSigSz != ED25519_SIG_SIZE)
        return BAD_FUNC_ARG;

    /* Build the list of component names from the parsed inner list.
     * Reject unknown derived components (starting with '@') to prevent
     * a crafted Signature-Input from causing unexpected behavior. */
    if (parsed.itemCount > WC_HTTPSIG_MAX_COMPONENTS)
        return BUFFER_E;

    for (i = 0; i < parsed.itemCount; i++) {
        if (parsed.items[i][0] == '@' && !is_known_derived(parsed.items[i]))
            return BAD_FUNC_ARG;
        componentNames[i] = parsed.items[i];
    }

    /* Extract the raw signature-params value from the Signature-Input
     * header rather than re-serializing from parsed data. RFC 9421
     * Section 3.2 requires using the exact original value so that the
     * reconstructed signature base matches what the signer produced. */
    sigParamsSz = (word32)sizeof(sigParams);
    ret = wc_SfExtractRawSigParams(signatureInput, label,
                                   sigParams, &sigParamsSz);
    if (ret != 0)
        return ret;

    /* Build signature base */
    sigBaseSz = (word32)sizeof(sigBase);
    ret = build_signature_base(
        componentNames, parsed.itemCount,
        method, authority, path, query,
        headers, headerCount,
        sigParams,
        sigBase, &sigBaseSz);
    if (ret != 0)
        return ret;

    /* Verify Ed25519 signature */
    ret = wc_ed25519_verify_msg(rawSig, rawSigSz,
                                (const byte*)sigBase, sigBaseSz,
                                &verifyRes, pubKey);
    if (ret != 0)
        return ret;
    if (verifyRes != 1)
        return SIG_VERIFY_E;

    return 0;
}

#endif /* HAVE_ED25519_VERIFY */

#endif /* HAVE_ED25519 */
