/* wc_sf.c
 *
 * Minimal RFC 8941 Structured Fields subset for RFC 9421.
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

#include "wc_sf.h"

#include <wolfssl/wolfcrypt/coding.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <limits.h>

/* --- Internal helpers --- */

static const char* skip_ows(const char* p)
{
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

/* RFC 8941 Section 3.1.2 key character predicates:
 * key = ( lcalpha / "*" ) *( lcalpha / DIGIT / "_" / "-" / "." / "*" ) */
static int is_key_start(int c)
{
    return (c >= 'a' && c <= 'z') || c == '*';
}

static int is_key_char(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
           c == '_' || c == '-' || c == '.' || c == '*';
}

/* Advance past a dictionary key (token) per RFC 8941 Section 3.1.2. */
static const char* parse_token(const char* p, char* out, int maxLen)
{
    int i = 0;
    if (!is_key_start((unsigned char)*p)) {
        out[0] = '\0';
        return p;
    }
    out[i++] = *p++;
    while (is_key_char((unsigned char)*p) && i < maxLen - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return p;
}

/* Parse a quoted sf-string: DQUOTE *chr DQUOTE
 * RFC 8941 Section 3.3.3: only \\ and \" are valid escapes. */
static const char* parse_sf_string(const char* p, char* out, int maxLen)
{
    int i = 0;
    if (*p != '"')
        return NULL;
    p++;
    while (*p && *p != '"' && i < maxLen - 1) {
        if (*p == '\\') {
            p++;
            if (*p != '\\' && *p != '"')
                return NULL;
        }
        out[i++] = *p++;
    }
    out[i] = '\0';
    if (*p != '"')
        return NULL;
    p++;
    return p;
}

/* Parse an sf-integer (RFC 8941: at least 1 digit, max 15 digits).
 * Detects overflow against LONG_MAX to handle both 32-bit and 64-bit
 * platforms correctly. On 32-bit (long = 32-bit), UNIX timestamps after
 * 2038-01-19 exceed LONG_MAX and will be rejected as overflow. */
static const char* parse_sf_integer(const char* p, long* val)
{
    int neg = 0;
    long v = 0;
    int digits = 0;
    int d;
    if (*p == '-') { neg = 1; p++; }
    while (*p >= '0' && *p <= '9' && digits < 15) {
        d = *p - '0';
        if (v > (LONG_MAX - d) / 10)
            return NULL;
        v = v * 10 + d;
        p++;
        digits++;
    }
    if (digits == 0)
        return NULL;
    if (*p >= '0' && *p <= '9')
        return NULL; /* exceeds 15-digit sf-integer limit */
    *val = neg ? -v : v;
    return p;
}

/* Parse parameters: *( ";" key ( "=" value ) )
 * Values can be integers or sf-strings. */
static const char* parse_params(const char* p, wc_SfParam* params,
                                int* paramCount, int maxParams)
{
    *paramCount = 0;
    while (*p == ';' && *paramCount < maxParams) {
        wc_SfParam* param = &params[*paramCount];
        p++; /* skip ';' */
        p = skip_ows(p);
        p = parse_token(p, param->name, WC_SF_MAX_LABEL);
        if (*p == '=') {
            p++;
            if (*p == '"') {
                param->type = WC_SF_PARAM_STRING;
                p = parse_sf_string(p, param->strVal, WC_SF_MAX_STRING);
                if (!p) return NULL;
            } else {
                param->type = WC_SF_PARAM_INTEGER;
                p = parse_sf_integer(p, &param->intVal);
                if (!p) return NULL;
            }
        } else {
            /* Boolean true parameter (no value) — treat as integer 1 */
            param->type = WC_SF_PARAM_INTEGER;
            param->intVal = 1;
            param->strVal[0] = '\0';
        }
        (*paramCount)++;
    }
    return p;
}

/* Skip a quoted sf-string without extracting its value. Handles
 * escaped characters (\" and \\) so embedded delimiters are not
 * misinterpreted by the caller. */
static const char* skip_sf_string(const char* p)
{
    if (*p != '"')
        return p;
    p++;
    while (*p && *p != '"') {
        if (*p == '\\' && *(p + 1))
            p++;
        p++;
    }
    if (*p == '"')
        p++;
    return p;
}

/* Skip trailing parameters on a dictionary member value.
 * Parameters can contain quoted strings with arbitrary characters. */
static const char* skip_params(const char* p)
{
    while (*p == ';') {
        p++;
        while (*p && *p != ';' && *p != ',') {
            if (*p == '"') {
                p = skip_sf_string(p);
            } else {
                p++;
            }
        }
    }
    return p;
}

/* Find a dictionary member by label.
 * Returns pointer to the value (after "label="), or NULL. */
static const char* find_dict_member(const char* input, const char* label,
                                    char* foundLabel, int labelMax)
{
    const char* p = skip_ows(input);
    while (*p) {
        char thisLabel[WC_SF_MAX_LABEL];
        p = skip_ows(p);
        p = parse_token(p, thisLabel, WC_SF_MAX_LABEL);
        if (*p != '=') return NULL;
        p++; /* skip '=' */

        if (label == NULL || XSTRCMP(thisLabel, label) == 0) {
            if (foundLabel) {
                XSTRNCPY(foundLabel, thisLabel, labelMax - 1);
                foundLabel[labelMax - 1] = '\0';
            }
            return p;
        }

        /* Skip value: inner list, byte sequence, or bare item */
        if (*p == '(') {
            int depth = 1;
            p++;
            while (*p && depth > 0) {
                if (*p == '"') {
                    p = skip_sf_string(p);
                } else {
                    if (*p == '(') depth++;
                    else if (*p == ')') depth--;
                    p++;
                }
            }
            p = skip_params(p);
        } else if (*p == ':') {
            p++;
            while (*p && *p != ':') p++;
            if (*p == ':') p++;
            p = skip_params(p);
        } else {
            while (*p && *p != ',' && *p != ';' && *p != ' ') p++;
            p = skip_params(p);
        }
        p = skip_ows(p);
        if (*p == ',') p++;
    }
    return NULL;
}

/* --- Public API ---
 *
 * Note: parse errors return ASN_PARSE_E. This reuses the wolfCrypt ASN.1
 * error code because wolfCrypt does not provide a generic parse error and
 * defining custom error codes in example code is not appropriate. */

int wc_SfParseSigInput(const char* headerVal, const char* label,
                       wc_SfSigInput* out)
{
    const char* p;

    if (!headerVal || !out)
        return BAD_FUNC_ARG;

    XMEMSET(out, 0, sizeof(*out));

    p = find_dict_member(headerVal, label, out->label, WC_SF_MAX_LABEL);
    if (!p)
        return ASN_PARSE_E;

    /* Expect inner list: ( ... ) */
    if (*p != '(')
        return ASN_PARSE_E;
    p++;

    /* Parse inner list items (sf-strings) */
    while (*p && *p != ')') {
        p = skip_ows(p);
        if (*p == ')') break;
        if (*p == '"') {
            if (out->itemCount >= WC_SF_MAX_ITEMS)
                return BUFFER_E;
            p = parse_sf_string(p,
                                out->items[out->itemCount],
                                WC_SF_MAX_STRING);
            if (!p) return ASN_PARSE_E;
            out->itemCount++;
        } else if (*p == ')') {
            break;
        } else {
            return ASN_PARSE_E;
        }
        p = skip_ows(p);
    }

    if (*p == ')')
        p++;

    /* Parse parameters */
    p = parse_params(p, out->params, &out->paramCount, WC_SF_MAX_PARAMS);
    if (!p)
        return ASN_PARSE_E;

    return 0;
}

int wc_SfExtractRawSigParams(const char* headerVal, const char* label,
                             char* rawOut, word32* rawOutSz)
{
    const char* p;
    const char* start;
    word32 len;

    if (!headerVal || !rawOut || !rawOutSz)
        return BAD_FUNC_ARG;

    p = find_dict_member(headerVal, label, NULL, 0);
    if (!p)
        return ASN_PARSE_E;

    start = p;

    if (*p != '(')
        return ASN_PARSE_E;
    p++;
    while (*p && *p != ')') {
        if (*p == '"')
            p = skip_sf_string(p);
        else
            p++;
    }
    if (*p == ')')
        p++;

    p = skip_params(p);

    len = (word32)(p - start);
    if (len + 1 > *rawOutSz)
        return BUFFER_E;

    XMEMCPY(rawOut, start, len);
    rawOut[len] = '\0';
    *rawOutSz = len;
    return 0;
}

/* Append a quoted sf-string with proper escaping of '"' and '\' per
 * RFC 8941 Section 3.3.3. Returns new position or -1 on overflow. */
static int sf_append_escaped(char* out, int pos, int maxSz, const char* s)
{
    if (pos >= maxSz) return -1;
    out[pos++] = '"';
    while (*s) {
        if (*s == '"' || *s == '\\') {
            if (pos >= maxSz) return -1;
            out[pos++] = '\\';
        }
        if (pos >= maxSz) return -1;
        out[pos++] = *s++;
    }
    if (pos >= maxSz) return -1;
    out[pos++] = '"';
    return pos;
}

int wc_SfGenSigParams(const wc_SfSigInput* in, char* out, word32* outSz)
{
    int i, pos = 0;
    word32 maxSz;

    if (!in || !out || !outSz)
        return BAD_FUNC_ARG;

    maxSz = *outSz;

    /* Inner list: ("item1" "item2" ...) */
    if (pos >= (int)maxSz) return BUFFER_E;
    out[pos++] = '(';

    for (i = 0; i < in->itemCount; i++) {
        if (i > 0) {
            if (pos >= (int)maxSz) return BUFFER_E;
            out[pos++] = ' ';
        }
        pos = sf_append_escaped(out, pos, (int)maxSz, in->items[i]);
        if (pos < 0) return BUFFER_E;
    }

    if (pos >= (int)maxSz) return BUFFER_E;
    out[pos++] = ')';

    /* Parameters: ;name=value */
    for (i = 0; i < in->paramCount; i++) {
        int n;
        if (in->params[i].type == WC_SF_PARAM_INTEGER) {
            n = XSNPRINTF(out + pos, maxSz - pos, ";%s=%ld",
                          in->params[i].name, in->params[i].intVal);
            if (n < 0 || pos + n >= (int)maxSz) return BUFFER_E;
            pos += n;
        } else {
            n = XSNPRINTF(out + pos, maxSz - pos, ";%s=",
                          in->params[i].name);
            if (n < 0 || pos + n >= (int)maxSz) return BUFFER_E;
            pos += n;
            pos = sf_append_escaped(out, pos, (int)maxSz,
                                    in->params[i].strVal);
            if (pos < 0) return BUFFER_E;
        }
    }

    if (pos >= (int)maxSz) return BUFFER_E;
    out[pos] = '\0';
    *outSz = (word32)pos;
    return 0;
}

int wc_SfGenSigInput(const wc_SfSigInput* in, char* out, word32* outSz)
{
    int ret;
    int labelLen;
    word32 remaining;
    word32 paramsLen;

    if (!in || !out || !outSz)
        return BAD_FUNC_ARG;

    /* label= */
    labelLen = XSNPRINTF(out, *outSz, "%s=", in->label);
    if (labelLen < 0 || (word32)labelLen >= *outSz)
        return BUFFER_E;

    remaining = *outSz - (word32)labelLen;
    paramsLen = remaining;
    ret = wc_SfGenSigParams(in, out + labelLen, &paramsLen);
    if (ret != 0)
        return ret;

    *outSz = (word32)labelLen + paramsLen;
    return 0;
}

int wc_SfParseSigValue(const char* headerVal, const char* label,
                       byte* out, word32* outSz)
{
    const char* p;
    const char* start;
    const char* end;
    word32 b64Len;

    if (!headerVal || !out || !outSz)
        return BAD_FUNC_ARG;

    p = find_dict_member(headerVal, label, NULL, 0);
    if (!p)
        return ASN_PARSE_E;

    /* Expect byte sequence: :base64: */
    if (*p != ':')
        return ASN_PARSE_E;
    p++;
    start = p;

    while (*p && *p != ':')
        p++;
    if (*p != ':')
        return ASN_PARSE_E;
    end = p;

    b64Len = (word32)(end - start);
    return Base64_Decode((const byte*)start, b64Len, out, outSz);
}

int wc_SfGenSigValue(const char* label, const byte* sig, word32 sigSz,
                     char* out, word32* outSz)
{
    int ret;
    int labelLen;
    word32 b64Len;
    word32 remaining;

    if (!label || !sig || !out || !outSz)
        return BAD_FUNC_ARG;

    /* label=: */
    labelLen = XSNPRINTF(out, *outSz, "%s=:", label);
    if (labelLen < 0 || (word32)labelLen >= *outSz)
        return BUFFER_E;

    remaining = *outSz - (word32)labelLen;
    if (remaining < 3)
        return BUFFER_E;
    b64Len = remaining - 2; /* reserve space for trailing ':' and '\0' */

    ret = Base64_Encode_NoNl(sig, sigSz,
                             (byte*)(out + labelLen), &b64Len);
    if (ret != 0)
        return ret;

    /* Append trailing ':' */
    if ((word32)labelLen + b64Len + 2 > *outSz)
        return BUFFER_E;

    out[labelLen + b64Len] = ':';
    out[labelLen + b64Len + 1] = '\0';
    *outSz = (word32)labelLen + b64Len + 1;

    return 0;
}
