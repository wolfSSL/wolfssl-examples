/* wc_sf.h
 *
 * Minimal RFC 8941 Structured Fields subset for RFC 9421.
 *
 * Only implements the structured field types needed by HTTP Message
 * Signatures: dictionary lookup, inner lists with string items,
 * parameters (integer and string), and byte sequences.
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

#ifndef WC_SF_H
#define WC_SF_H

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/types.h>

#define WC_SF_MAX_ITEMS      16
#define WC_SF_MAX_PARAMS      8
#define WC_SF_MAX_STRING    256
#define WC_SF_MAX_LABEL      64

/* Signature parameter types */
typedef enum {
    WC_SF_PARAM_INTEGER,
    WC_SF_PARAM_STRING
} wc_SfParamType;

/* A single key=value parameter */
typedef struct {
    char            name[WC_SF_MAX_LABEL];
    wc_SfParamType  type;
    long            intVal;
    char            strVal[WC_SF_MAX_STRING];
} wc_SfParam;

/* Parsed Signature-Input member: inner list + parameters.
 * Represents: ("@method" "@authority" ...);created=123;keyid="k" */
typedef struct {
    char       label[WC_SF_MAX_LABEL];
    char       items[WC_SF_MAX_ITEMS][WC_SF_MAX_STRING];
    int        itemCount;
    wc_SfParam params[WC_SF_MAX_PARAMS];
    int        paramCount;
} wc_SfSigInput;

/* Parse a Signature-Input header value, extracting the named member.
 * If label is NULL, parses the first member found.
 * Returns 0 on success, negative on error. */
int wc_SfParseSigInput(const char* headerVal, const char* label,
                       wc_SfSigInput* out);

/* Generate a Signature-Input member string.
 * Output: label=(items);params
 * Returns 0 on success, negative on error. */
int wc_SfGenSigInput(const wc_SfSigInput* in, char* out, word32* outSz);

/* Generate just the signature-params value (no label=).
 * Output: (items);params
 * Used for the @signature-params line in the signature base. */
int wc_SfGenSigParams(const wc_SfSigInput* in, char* out, word32* outSz);

/* Extract the raw signature-params value for a dictionary member.
 * Copies the verbatim inner-list + parameters substring (e.g.
 * ("@method" "@authority");created=123;keyid="k") without
 * re-serialization. Used by the verify path per RFC 9421 Section 3.2. */
int wc_SfExtractRawSigParams(const char* headerVal, const char* label,
                             char* rawOut, word32* rawOutSz);

/* Parse a Signature header value, extracting a byte sequence for the
 * named member. Decodes the base64 content.
 * Returns 0 on success, negative on error. */
int wc_SfParseSigValue(const char* headerVal, const char* label,
                       byte* out, word32* outSz);

/* Generate a Signature header member: label=:base64(sig):
 * Returns 0 on success, negative on error. */
int wc_SfGenSigValue(const char* label, const byte* sig, word32 sigSz,
                     char* out, word32* outSz);

#endif /* WC_SF_H */
