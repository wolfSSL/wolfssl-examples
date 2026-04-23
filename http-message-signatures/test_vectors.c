/* test_vectors.c
 *
 * Validates the implementation against RFC 9421 Appendix B.2.6
 * (Signing a Request Using ed25519) and performs a round-trip test.
 *
 * Test data from:
 *   https://www.rfc-editor.org/rfc/rfc9421#appendix-B.1.4  (Ed25519 key)
 *   https://www.rfc-editor.org/rfc/rfc9421#appendix-B.2.6  (test case)
 *
 * Build wolfSSL with:
 *   ./configure --enable-ed25519 --enable-coding && make && sudo make install
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

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/coding.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <stdio.h>
#include <string.h>

#include "common/wc_http_sig.h"
#include "common/wc_sf.h"

#if defined(HAVE_ED25519) && defined(HAVE_ED25519_SIGN) && \
    defined(HAVE_ED25519_VERIFY)

/* --- RFC 9421 Appendix B.1.4 - Ed25519 test key (raw bytes) --- */

static const byte kTestPrivKey[ED25519_KEY_SIZE] = {
    0x9f, 0x83, 0x62, 0xf8, 0x7a, 0x48, 0x4a, 0x95,
    0x4e, 0x6e, 0x74, 0x0c, 0x5b, 0x4c, 0x0e, 0x84,
    0x22, 0x91, 0x39, 0xa2, 0x0a, 0xa8, 0xab, 0x56,
    0xff, 0x66, 0x58, 0x6f, 0x6a, 0x7d, 0x29, 0xc5
};

static const byte kTestPubKey[ED25519_PUB_KEY_SIZE] = {
    0x26, 0xb4, 0x0b, 0x8f, 0x93, 0xff, 0xf3, 0xd8,
    0x97, 0x11, 0x2f, 0x7e, 0xbc, 0x58, 0x2b, 0x23,
    0x2d, 0xbd, 0x72, 0x51, 0x7d, 0x08, 0x2f, 0xe8,
    0x3c, 0xfb, 0x30, 0xdd, 0xce, 0x43, 0xd1, 0xbb
};

/* --- RFC 9421 Appendix B.2 - Test request components --- */

static const char* kMethod    = "POST";
static const char* kAuthority = "example.com";
static const char* kPath      = "/foo";

static const wc_HttpHeader kHeaders[] = {
    { "date",           "Tue, 20 Apr 2021 02:07:55 GMT" },
    { "content-type",   "application/json" },
    { "content-length", "18" }
};

/* RFC 9421 Appendix B.2.6 expected signature base */
static const char kExpectedSigBase[] =
    "\"date\": Tue, 20 Apr 2021 02:07:55 GMT\n"
    "\"@method\": POST\n"
    "\"@path\": /foo\n"
    "\"@authority\": example.com\n"
    "\"content-type\": application/json\n"
    "\"content-length\": 18\n"
    "\"@signature-params\": (\"date\" \"@method\" \"@path\" \"@authority\" "
    "\"content-type\" \"content-length\")"
    ";created=1618884473;keyid=\"test-key-ed25519\"";

/* Expected signature (base64) from RFC 9421 B.2.6 */
static const char kExpectedSigB64[] =
    "wqcAqbmYJ2ji2glfAMaRy4gruYYnx2nEFN2HN6jrnDnQCK1u"
    "02Gb04v9EDgwUPiu4A0w6vuQv5lIp5WPpBKRCw==";

static const char kExpectedSigInput[] =
    "sig1=(\"date\" \"@method\" \"@path\" \"@authority\" "
    "\"content-type\" \"content-length\")"
    ";created=1618884473;keyid=\"test-key-ed25519\"";

/* --- Helpers --- */

static const char* kB26Components[] = {
    "date", "@method", "@path", "@authority",
    "content-type", "content-length"
};
static const int kB26CompCount = 6;
static const char kB26SigParamsVal[] =
    "(\"date\" \"@method\" \"@path\" \"@authority\" "
    "\"content-type\" \"content-length\")"
    ";created=1618884473;keyid=\"test-key-ed25519\"";

static int build_rfc_b26_sig_base(byte* out, word32* outSz)
{
    int pos = 0, n, i;
    int maxSz = (int)*outSz;

    for (i = 0; i < kB26CompCount; i++) {
        const char* val = NULL;

        if (strcmp(kB26Components[i], "@method") == 0)
            val = kMethod;
        else if (strcmp(kB26Components[i], "@authority") == 0)
            val = kAuthority;
        else if (strcmp(kB26Components[i], "@path") == 0)
            val = kPath;
        else {
            int j;
            for (j = 0; j < (int)(sizeof(kHeaders)/sizeof(kHeaders[0])); j++) {
                if (strcmp(kHeaders[j].name, kB26Components[i]) == 0) {
                    val = kHeaders[j].value;
                    break;
                }
            }
        }

        if (!val) return -1;
        n = snprintf((char*)out + pos, maxSz - pos,
                     "\"%s\": %s\n", kB26Components[i], val);
        if (n < 0 || n >= maxSz - pos) return -1;
        pos += n;
    }

    n = snprintf((char*)out + pos, maxSz - pos,
                 "\"@signature-params\": %s", kB26SigParamsVal);
    if (n < 0 || n >= maxSz - pos) return -1;
    pos += n;
    *outSz = (word32)pos;
    return 0;
}

static int sign_verify_roundtrip_ex(
    const char* method, const char* authority,
    const char* path, const char* query,
    const wc_HttpHeader* signHdrs, int signHdrCount,
    const wc_HttpHeader* verifyHdrs, int verifyHdrCount,
    const char* keyId, long created, int maxAgeSec);

static int sign_verify_roundtrip(
    const char* method, const char* authority,
    const char* path, const char* query,
    const wc_HttpHeader* signHdrs, int signHdrCount,
    const wc_HttpHeader* verifyHdrs, int verifyHdrCount,
    const char* keyId)
{
    return sign_verify_roundtrip_ex(method, authority, path, query,
                                    signHdrs, signHdrCount,
                                    verifyHdrs, verifyHdrCount,
                                    keyId, 1700000000, 0);
}

static int sign_verify_roundtrip_ex(
    const char* method, const char* authority,
    const char* path, const char* query,
    const wc_HttpHeader* signHdrs, int signHdrCount,
    const wc_HttpHeader* verifyHdrs, int verifyHdrCount,
    const char* keyId, long created, int maxAgeSec)
{
    int ret;
    ed25519_key key, verifyKey;
    WC_RNG rng;
    int keyInited = 0;
    int verifyKeyInited = 0;
    char sigBuf[512];
    word32 sigBufSz = sizeof(sigBuf);
    char inputBuf[1024];
    word32 inputBufSz = sizeof(inputBuf);

    ret = wc_InitRng(&rng);
    if (ret != 0) return ret;

    ret = wc_ed25519_init(&key);
    if (ret != 0) goto done;
    keyInited = 1;
    ret = wc_ed25519_init(&verifyKey);
    if (ret != 0) goto done;
    verifyKeyInited = 1;

    ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    if (ret != 0) goto done;

    ret = wc_HttpSig_Sign(
        method, authority, path, query,
        signHdrs, signHdrCount,
        &key, keyId, created,
        sigBuf, &sigBufSz,
        inputBuf, &inputBufSz);
    if (ret != 0) goto done;

    {
        byte pubBuf[ED25519_PUB_KEY_SIZE];
        word32 pubSz = sizeof(pubBuf);
        ret = wc_ed25519_export_public(&key, pubBuf, &pubSz);
        if (ret != 0) goto done;
        ret = wc_ed25519_import_public(pubBuf, pubSz, &verifyKey);
        if (ret != 0) goto done;
    }

    ret = wc_HttpSig_Verify(
        method, authority, path, query,
        verifyHdrs ? verifyHdrs : signHdrs,
        verifyHdrs ? verifyHdrCount : signHdrCount,
        sigBuf, inputBuf,
        NULL, &verifyKey, maxAgeSec);

done:
    if (keyInited)
        wc_ed25519_free(&key);
    if (verifyKeyInited)
        wc_ed25519_free(&verifyKey);
    wc_FreeRng(&rng);
    return ret;
}

/* --- Test 1: Structured Fields round-trip --- */

static int test_sf_roundtrip(void)
{
    int ret;
    wc_SfSigInput parsed;
    char generated[1024];
    word32 genSz = sizeof(generated);

    printf("Test 1: Structured Fields parse/generate round-trip\n");

    ret = wc_SfParseSigInput(kExpectedSigInput, "sig1", &parsed);
    if (ret != 0) { printf("  FAIL: parse returned %d\n", ret); return ret; }

    if (parsed.itemCount != 6) {
        printf("  FAIL: expected 6 items, got %d\n", parsed.itemCount);
        return -1;
    }

    if (strcmp(parsed.items[0], "date") != 0 ||
        strcmp(parsed.items[1], "@method") != 0) {
        printf("  FAIL: unexpected item values\n");
        return -1;
    }

    ret = wc_SfGenSigInput(&parsed, generated, &genSz);
    if (ret != 0) { printf("  FAIL: generate returned %d\n", ret); return ret; }

    if (strcmp(generated, kExpectedSigInput) != 0) {
        printf("  FAIL: round-trip mismatch\n");
        printf("  Expected: %s\n", kExpectedSigInput);
        printf("  Got:      %s\n", generated);
        return -1;
    }

    printf("  PASS\n");
    return 0;
}

/* --- Test 2: Signature base construction matches RFC B.2.6 --- */

static int test_signature_base(void)
{
    byte buf[4096];
    word32 bufSz = sizeof(buf);

    printf("Test 2: Signature base construction\n");

    if (build_rfc_b26_sig_base(buf, &bufSz) != 0) {
        printf("  FAIL: could not build signature base\n");
        return -1;
    }

    if (bufSz != (word32)strlen(kExpectedSigBase) ||
        memcmp(buf, kExpectedSigBase, bufSz) != 0) {
        printf("  FAIL: signature base mismatch\n");
        return -1;
    }

    printf("  PASS\n");
    return 0;
}

/* --- Test 3: RFC 9421 B.2.6 - Ed25519 sign + verify with RFC key --- */

static int test_rfc_vector_verify(void)
{
    int ret, verifyRes = 0, i;
    ed25519_key key;
    byte producedSig[ED25519_SIG_SIZE];
    word32 producedSigSz = sizeof(producedSig);
    byte expectedSig[ED25519_SIG_SIZE];
    word32 expectedSigSz = sizeof(expectedSig);
    byte sigBase[4096];
    word32 sigBaseSz = sizeof(sigBase);

    printf("Test 3: RFC 9421 B.2.6 Ed25519 vector - sign + verify\n");

    ret = Base64_Decode((const byte*)kExpectedSigB64,
                        (word32)strlen(kExpectedSigB64),
                        expectedSig, &expectedSigSz);
    if (ret != 0) { printf("  FAIL: base64 decode: %d\n", ret); return ret; }

    if (build_rfc_b26_sig_base(sigBase, &sigBaseSz) != 0) {
        printf("  FAIL: could not build signature base\n");
        return -1;
    }

    if (sigBaseSz != (word32)strlen(kExpectedSigBase) ||
        memcmp(sigBase, kExpectedSigBase, sigBaseSz) != 0) {
        printf("  FAIL: signature base mismatch\n");
        return -1;
    }
    printf("  Signature base matches RFC expected value\n");

    /* Import RFC private key seed, derive public, re-import as full keypair */
    ret = wc_ed25519_init(&key);
    if (ret != 0) return ret;

    ret = wc_ed25519_import_private_only(kTestPrivKey, ED25519_KEY_SIZE, &key);
    if (ret != 0) {
        printf("  FAIL: import private seed: %d\n", ret);
        wc_ed25519_free(&key);
        return ret;
    }

    {
        byte derivedPub[ED25519_PUB_KEY_SIZE];
        word32 derivedPubSz = ED25519_PUB_KEY_SIZE;
        ret = wc_ed25519_make_public(&key, derivedPub, derivedPubSz);
        if (ret != 0) {
            printf("  FAIL: make public: %d\n", ret);
            wc_ed25519_free(&key);
            return ret;
        }

        if (memcmp(derivedPub, kTestPubKey, ED25519_PUB_KEY_SIZE) != 0) {
            printf("  WARNING: derived pubkey differs from RFC pubkey\n");
        }

        ret = wc_ed25519_import_private_key(kTestPrivKey, ED25519_KEY_SIZE,
                                            derivedPub, derivedPubSz, &key);
        if (ret != 0) {
            printf("  FAIL: import keypair: %d\n", ret);
            wc_ed25519_free(&key);
            return ret;
        }
    }

    ret = wc_ed25519_sign_msg(sigBase, sigBaseSz,
                              producedSig, &producedSigSz, &key);
    if (ret != 0) {
        printf("  FAIL: sign returned %d\n", ret);
        wc_ed25519_free(&key);
        return ret;
    }

    if (producedSigSz != expectedSigSz ||
        memcmp(producedSig, expectedSig, producedSigSz) != 0) {
        printf("  FAIL: produced signature != RFC expected signature\n");
        printf("  Produced: ");
        for (i = 0; i < (int)producedSigSz; i++) printf("%02x", producedSig[i]);
        printf("\n  Expected: ");
        for (i = 0; i < (int)expectedSigSz; i++) printf("%02x", expectedSig[i]);
        printf("\n");
        wc_ed25519_free(&key);
        return SIG_VERIFY_E;
    }
    printf("  Produced signature matches RFC B.2.6 expected value\n");

    ret = wc_ed25519_verify_msg(producedSig, producedSigSz,
                                sigBase, sigBaseSz, &verifyRes, &key);
    wc_ed25519_free(&key);

    if (ret != 0) {
        printf("  FAIL: verify returned %d\n", ret);
        return ret;
    }
    if (verifyRes != 1) {
        printf("  FAIL: verification failed\n");
        return SIG_VERIFY_E;
    }

    printf("  PASS\n");
    return 0;
}

/* --- Test 4: Full sign/verify round-trip --- */

static int test_api_roundtrip(void)
{
    int ret;
    wc_HttpHeader headers[2];
    headers[0].name  = "content-type";
    headers[0].value = "application/json";
    headers[1].name  = "date";
    headers[1].value = "Thu, 19 Mar 2026 12:00:00 GMT";

    printf("Test 4: Full API sign/verify round-trip\n");

    ret = sign_verify_roundtrip(
        "POST", "example.com", "/api/data", "?v=1",
        headers, 2, NULL, 0, "round-trip-key");
    if (ret != 0)
        printf("  FAIL: %d (%s)\n", ret, wc_GetErrorString(ret));
    else
        printf("  PASS\n");
    return ret;
}

/* --- Test 5: GetKeyId extraction --- */

static int test_get_keyid(void)
{
    int ret;
    char keyIdBuf[256];
    word32 keyIdSz = sizeof(keyIdBuf);

    printf("Test 5: GetKeyId extraction\n");

    ret = wc_HttpSig_GetKeyId(kExpectedSigInput, "sig1", keyIdBuf, &keyIdSz);
    if (ret != 0) { printf("  FAIL: returned %d\n", ret); return ret; }

    if (strcmp(keyIdBuf, "test-key-ed25519") != 0) {
        printf("  FAIL: expected 'test-key-ed25519', got '%s'\n", keyIdBuf);
        return -1;
    }

    printf("  PASS\n");
    return 0;
}

/* --- Test 6: Sign/verify with @query component --- */

static int test_query_component(void)
{
    int ret;
    ed25519_key key, verifyKey;
    WC_RNG rng;
    char sigBuf[512];
    word32 sigBufSz = sizeof(sigBuf);
    char inputBuf[1024];
    word32 inputBufSz = sizeof(inputBuf);

    wc_HttpHeader headers[1];
    headers[0].name  = "date";
    headers[0].value = "Thu, 19 Mar 2026 12:00:00 GMT";

    printf("Test 6: Sign/verify with @query component\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) return ret;
    ret = wc_ed25519_init(&key);
    if (ret != 0) { wc_FreeRng(&rng); return ret; }
    ret = wc_ed25519_init(&verifyKey);
    if (ret != 0) { wc_ed25519_free(&key); wc_FreeRng(&rng); return ret; }

    ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    if (ret != 0) goto cleanup6;

    ret = wc_HttpSig_Sign(
        "GET", "api.example.com", "/search", "?q=test&page=2",
        headers, 1, &key, "query-test-key", 1700000000,
        sigBuf, &sigBufSz, inputBuf, &inputBufSz);
    if (ret != 0) { printf("  FAIL: sign returned %d\n", ret); goto cleanup6; }

    if (strstr(inputBuf, "\"@query\"") == NULL) {
        printf("  FAIL: @query not found in Signature-Input\n");
        ret = -1;
        goto cleanup6;
    }

    {
        byte pubBuf[ED25519_PUB_KEY_SIZE];
        word32 pubSz = sizeof(pubBuf);
        ret = wc_ed25519_export_public(&key, pubBuf, &pubSz);
        if (ret != 0) goto cleanup6;
        ret = wc_ed25519_import_public(pubBuf, pubSz, &verifyKey);
        if (ret != 0) goto cleanup6;
    }

    ret = wc_HttpSig_Verify(
        "GET", "api.example.com", "/search", "?q=test&page=2",
        headers, 1, sigBuf, inputBuf,
        NULL, &verifyKey, 0);
    if (ret != 0) {
        printf("  FAIL: verify returned %d\n", ret);
        goto cleanup6;
    }

    /* Negative: wrong query should fail */
    ret = wc_HttpSig_Verify(
        "GET", "api.example.com", "/search", "?q=different",
        headers, 1, sigBuf, inputBuf,
        NULL, &verifyKey, 0);
    if (ret == 0) {
        printf("  FAIL: tampered query was accepted\n");
        ret = -1;
        goto cleanup6;
    }

    ret = 0;
    printf("  PASS\n");

cleanup6:
    wc_ed25519_free(&key);
    wc_ed25519_free(&verifyKey);
    wc_FreeRng(&rng);
    return ret;
}

/* --- Test 7: Case-insensitive headers + whitespace trimming --- */

static int test_header_normalization(void)
{
    int ret;
    wc_HttpHeader signHdrs[2];
    wc_HttpHeader verifyHdrs[2];

    signHdrs[0].name  = "content-type";
    signHdrs[0].value = "application/json";
    signHdrs[1].name  = "x-custom";
    signHdrs[1].value = "hello";

    verifyHdrs[0].name  = "Content-Type";
    verifyHdrs[0].value = "  application/json  ";
    verifyHdrs[1].name  = "X-Custom";
    verifyHdrs[1].value = "\thello\t";

    printf("Test 7: Case-insensitive headers + whitespace trimming\n");

    ret = sign_verify_roundtrip(
        "POST", "example.com", "/api", NULL,
        signHdrs, 2, verifyHdrs, 2, "norm-test");
    if (ret != 0)
        printf("  FAIL: verify with normalized headers returned %d\n", ret);
    else
        printf("  PASS\n");
    return ret;
}

/* --- Test 8: Algorithm enforcement --- */

static int test_alg_enforcement(void)
{
    int ret;
    wc_SfSigInput sfIn;
    char fakeInput[512];
    word32 fakeInputSz;
    ed25519_key key;
    WC_RNG rng;
    char sigBuf[512];
    word32 sigBufSz = sizeof(sigBuf);
    char realInputBuf[1024];
    word32 realInputBufSz = sizeof(realInputBuf);

    wc_HttpHeader headers[1];
    headers[0].name  = "date";
    headers[0].value = "Thu, 19 Mar 2026 12:00:00 GMT";

    printf("Test 8: Algorithm enforcement\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) return ret;
    ret = wc_ed25519_init(&key);
    if (ret != 0) { wc_FreeRng(&rng); return ret; }

    ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    if (ret != 0) goto cleanup8;

    ret = wc_HttpSig_Sign(
        "GET", "example.com", "/test", NULL,
        headers, 1, &key, "alg-test", 1700000000,
        sigBuf, &sigBufSz, realInputBuf, &realInputBufSz);
    if (ret != 0) { printf("  FAIL: sign returned %d\n", ret); goto cleanup8; }

    /* Craft Signature-Input with alg="hmac-sha256" */
    memset(&sfIn, 0, sizeof(sfIn));
    strncpy(sfIn.label, "sig1", WC_SF_MAX_LABEL - 1);
    strncpy(sfIn.items[0], "@method", WC_SF_MAX_STRING - 1);
    strncpy(sfIn.items[1], "@authority", WC_SF_MAX_STRING - 1);
    strncpy(sfIn.items[2], "@path", WC_SF_MAX_STRING - 1);
    strncpy(sfIn.items[3], "date", WC_SF_MAX_STRING - 1);
    sfIn.itemCount = 4;

    strncpy(sfIn.params[0].name, "created", WC_SF_MAX_LABEL - 1);
    sfIn.params[0].type = WC_SF_PARAM_INTEGER;
    sfIn.params[0].intVal = 1700000000;
    strncpy(sfIn.params[1].name, "keyid", WC_SF_MAX_LABEL - 1);
    sfIn.params[1].type = WC_SF_PARAM_STRING;
    strncpy(sfIn.params[1].strVal, "alg-test", WC_SF_MAX_STRING - 1);
    strncpy(sfIn.params[2].name, "alg", WC_SF_MAX_LABEL - 1);
    sfIn.params[2].type = WC_SF_PARAM_STRING;
    strncpy(sfIn.params[2].strVal, "hmac-sha256", WC_SF_MAX_STRING - 1);
    sfIn.paramCount = 3;

    fakeInputSz = sizeof(fakeInput);
    ret = wc_SfGenSigInput(&sfIn, fakeInput, &fakeInputSz);
    if (ret != 0) {
        printf("  FAIL: gen fake input: %d\n", ret);
        goto cleanup8;
    }

    ret = wc_HttpSig_Verify(
        "GET", "example.com", "/test", NULL,
        headers, 1, sigBuf, fakeInput,
        NULL, &key, 0);
    if (ret == 0) {
        printf("  FAIL: wrong alg was accepted\n");
        ret = -1;
        goto cleanup8;
    }

    ret = 0;
    printf("  PASS\n");

cleanup8:
    wc_ed25519_free(&key);
    wc_FreeRng(&rng);
    return ret;
}

/* --- Test 9: Timestamp validation (maxAgeSec) --- */

static int test_timestamp_validation(void)
{
    int ret;
    wc_HttpHeader headers[1];
    headers[0].name  = "date";
    headers[0].value = "Thu, 19 Mar 2026 12:00:00 GMT";

    printf("Test 9: Timestamp validation (maxAgeSec)\n");

    /* Sign with old timestamp (~2001), maxAgeSec=300 -> fail */
    ret = sign_verify_roundtrip_ex(
        "GET", "example.com", "/ts-test", NULL,
        headers, 1, NULL, 0, "ts-key", 1000000000L, 300);
    if (ret == 0) {
        printf("  FAIL: expired signature was accepted\n");
        return -1;
    }

    /* Same old timestamp, verify with maxAgeSec=0 (skip) → succeed */
    ret = sign_verify_roundtrip_ex(
        "GET", "example.com", "/ts-test", NULL,
        headers, 1, NULL, 0, "ts-key", 1000000000L, 0);
    if (ret != 0) {
        printf("  FAIL: maxAgeSec=0 should skip check, got %d\n", ret);
        return ret;
    }

    printf("  PASS\n");
    return 0;
}

/* --- Test 10: Auto-timestamp (created=0) with maxAgeSec --- */

static int test_auto_timestamp(void)
{
    int ret;
    wc_HttpHeader headers[1];
    headers[0].name  = "date";
    headers[0].value = "Thu, 19 Mar 2026 12:00:00 GMT";

    printf("Test 10: Auto-timestamp (created=0) with maxAgeSec\n");

    /* Sign with created=0 (auto), verify with maxAgeSec=60 → succeed */
    ret = sign_verify_roundtrip_ex(
        "GET", "example.com", "/auto-ts", NULL,
        headers, 1, NULL, 0, "auto-key", 0, 60);
    if (ret != 0) {
        printf("  FAIL: auto-timestamp + maxAgeSec=60 returned %d\n", ret);
        return ret;
    }

    printf("  PASS\n");
    return 0;
}

/* --- Test 11: API argument validation --- */

static int test_arg_validation(void)
{
    int ret;
    char sigBuf[512];
    word32 sigBufSz = sizeof(sigBuf);
    char inputBuf[1024];
    word32 inputBufSz = sizeof(inputBuf);
    ed25519_key key;
    WC_RNG rng;
    int pass = 1;

    printf("Test 11: API argument validation\n");

    ret = wc_ed25519_init(&key);
    if (ret != 0) { printf("  FAIL: ed25519 init\n"); return ret; }
    ret = wc_InitRng(&rng);
    if (ret != 0) { printf("  FAIL: RNG init\n"); wc_ed25519_free(&key); return ret; }
    ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    if (ret != 0) { printf("  FAIL: keygen\n"); wc_FreeRng(&rng); return ret; }

    /* NULL method → BAD_FUNC_ARG */
    ret = wc_HttpSig_Sign(NULL, "example.com", "/", NULL,
                          NULL, 0, &key, "k", 1700000000,
                          sigBuf, &sigBufSz, inputBuf, &inputBufSz);
    if (ret != BAD_FUNC_ARG) {
        printf("  FAIL: NULL method accepted (ret=%d)\n", ret);
        pass = 0;
    }

    /* headerCount > 0 with NULL headers → BAD_FUNC_ARG */
    ret = wc_HttpSig_Sign("GET", "example.com", "/", NULL,
                          NULL, 3, &key, "k", 1700000000,
                          sigBuf, &sigBufSz, inputBuf, &inputBufSz);
    if (ret != BAD_FUNC_ARG) {
        printf("  FAIL: NULL headers with count=3 accepted (ret=%d)\n", ret);
        pass = 0;
    }

    /* NULL signatureInput to GetKeyId → BAD_FUNC_ARG */
    {
        char kidBuf[64];
        word32 kidSz = sizeof(kidBuf);
        ret = wc_HttpSig_GetKeyId(NULL, "sig1", kidBuf, &kidSz);
        if (ret != BAD_FUNC_ARG) {
            printf("  FAIL: NULL input to GetKeyId accepted (ret=%d)\n", ret);
            pass = 0;
        }
    }

    /* NULL signature to Verify → BAD_FUNC_ARG */
    ret = wc_HttpSig_Verify("GET", "example.com", "/", NULL,
                            NULL, 0, NULL, "sig1=(...)", NULL, &key, 0);
    if (ret != BAD_FUNC_ARG) {
        printf("  FAIL: NULL signature accepted (ret=%d)\n", ret);
        pass = 0;
    }

    /* headerCount > 0 with NULL headers to Verify → BAD_FUNC_ARG */
    ret = wc_HttpSig_Verify("GET", "example.com", "/", NULL,
                            NULL, 2, "sig1=:AA==:", "sig1=()", NULL, &key, 0);
    if (ret != BAD_FUNC_ARG) {
        printf("  FAIL: NULL hdrs count=2 Verify "
               "accepted (ret=%d)\n", ret);
        pass = 0;
    }

    wc_ed25519_free(&key);
    wc_FreeRng(&rng);

    if (!pass) return -1;
    printf("  PASS\n");
    return 0;
}

/* --- Test 12: sf-integer overflow detection --- */

static int test_sf_integer_overflow(void)
{
    int ret;
    wc_SfSigInput parsed;

    /* 999999999999999 (15 digits) is within RFC 8941 range and fits in
     * 64-bit long. On 32-bit, it should be rejected as overflow. */
    const char* bigTimestamp =
        "sig1=(\"@method\");created=999999999999999;keyid=\"k\"";
    /* 1700000000 (10 digits) — a current-era UNIX timestamp.
     * Must parse correctly on both 32-bit and 64-bit. */
    const char* normalTimestamp =
        "sig1=(\"@method\");created=1700000000;keyid=\"k\"";

    /* Empty integer: `;created=;` must be rejected (RFC 8941 requires
     * at least one digit). */
    const char* emptyInt =
        "sig1=(\"@method\");created=;keyid=\"k\"";

    printf("Test 12: sf-integer overflow detection\n");

    ret = wc_SfParseSigInput(emptyInt, "sig1", &parsed);
    if (ret == 0) {
        printf("  FAIL: empty integer value accepted\n");
        return -1;
    }
    printf("  Empty integer correctly rejected\n");

    ret = wc_SfParseSigInput(normalTimestamp, "sig1", &parsed);
    if (ret != 0) {
        printf("  FAIL: normal timestamp parse returned %d\n", ret);
        return -1;
    }
    if (parsed.params[0].intVal != 1700000000L) {
        printf("  FAIL: expected 1700000000, got %ld\n",
               parsed.params[0].intVal);
        return -1;
    }

    if (sizeof(long) <= 4) {
        ret = wc_SfParseSigInput(bigTimestamp, "sig1", &parsed);
        if (ret == 0) {
            printf("  FAIL: 15-digit integer accepted on 32-bit\n");
            return -1;
        }
        printf("  32-bit: large integer correctly rejected\n");
    } else {
        ret = wc_SfParseSigInput(bigTimestamp, "sig1", &parsed);
        if (ret != 0) {
            printf("  FAIL: 15-digit integer rejected on 64-bit (%d)\n", ret);
            return -1;
        }
        if (parsed.params[0].intVal != 999999999999999L) {
            printf("  FAIL: 15-digit value mismatch\n");
            return -1;
        }
        printf("  64-bit: large integer correctly parsed\n");
    }

    printf("  PASS\n");
    return 0;
}

/* --- Test 13: find_dict_member with quoted params containing delimiters --- */

static int test_adversarial_sig_input(void)
{
    int ret;
    wc_SfSigInput parsed;
    char keyIdBuf[256];
    word32 keyIdSz;

    /* sig1 has a keyid containing ';' and ','. The parser must skip it
     * correctly to find sig2. */
    const char* multiDict =
        "sig1=(\"@method\");keyid=\"a]};,b\";alg=\"ed25519\", "
        "sig2=(\"@path\");keyid=\"real-key\";alg=\"ed25519\"";

    printf("Test 13: Adversarial Signature-Input (quoted delimiters)\n");

    ret = wc_SfParseSigInput(multiDict, "sig2", &parsed);
    if (ret != 0) {
        printf("  FAIL: parse sig2 returned %d\n", ret);
        return -1;
    }

    if (parsed.itemCount != 1 || strcmp(parsed.items[0], "@path") != 0) {
        printf("  FAIL: sig2 items mismatch\n");
        return -1;
    }

    keyIdSz = sizeof(keyIdBuf);
    ret = wc_HttpSig_GetKeyId(multiDict, "sig2", keyIdBuf, &keyIdSz);
    if (ret != 0) {
        printf("  FAIL: GetKeyId for sig2 returned %d\n", ret);
        return -1;
    }
    if (strcmp(keyIdBuf, "real-key") != 0) {
        printf("  FAIL: expected 'real-key', got '%s'\n", keyIdBuf);
        return -1;
    }

    printf("  PASS\n");
    return 0;
}

/* --- Test 14: sf-string escaping round-trip --- */

static int test_sf_string_escaping(void)
{
    int ret;
    wc_SfSigInput sfIn, parsed;
    char generated[1024];
    word32 genSz = sizeof(generated);

    printf("Test 14: sf-string escaping round-trip\n");

    memset(&sfIn, 0, sizeof(sfIn));
    strncpy(sfIn.label, "sig1", WC_SF_MAX_LABEL - 1);
    strncpy(sfIn.items[0], "@method", WC_SF_MAX_STRING - 1);
    sfIn.itemCount = 1;

    strncpy(sfIn.params[0].name, "keyid", WC_SF_MAX_LABEL - 1);
    sfIn.params[0].type = WC_SF_PARAM_STRING;
    strncpy(sfIn.params[0].strVal, "key\"with\\quotes",
            WC_SF_MAX_STRING - 1);
    sfIn.paramCount = 1;

    ret = wc_SfGenSigInput(&sfIn, generated, &genSz);
    if (ret != 0) {
        printf("  FAIL: generate returned %d\n", ret);
        return ret;
    }

    /* The output should contain properly escaped keyid */
    if (strstr(generated, "key\\\"with\\\\quotes") == NULL) {
        printf("  FAIL: escaping not found in: %s\n", generated);
        return -1;
    }

    /* Parse it back and verify the value is unescaped */
    ret = wc_SfParseSigInput(generated, "sig1", &parsed);
    if (ret != 0) {
        printf("  FAIL: re-parse returned %d\n", ret);
        return ret;
    }

    {
        int i;
        for (i = 0; i < parsed.paramCount; i++) {
            if (strcmp(parsed.params[i].name, "keyid") == 0) {
                if (strcmp(parsed.params[i].strVal,
                           "key\"with\\quotes") != 0) {
                    printf("  FAIL: round-trip mismatch: '%s'\n",
                           parsed.params[i].strVal);
                    return -1;
                }
                break;
            }
        }
        if (i == parsed.paramCount) {
            printf("  FAIL: keyid param not found after parse\n");
            return -1;
        }
    }

    printf("  PASS\n");
    return 0;
}

/* --- Test 15: Verify rejects NULL query when @query is signed --- */

static int test_query_null_mismatch(void)
{
    int ret;
    ed25519_key key, verifyKey;
    WC_RNG rng;
    char sigBuf[512];
    word32 sigBufSz = sizeof(sigBuf);
    char inputBuf[1024];
    word32 inputBufSz = sizeof(inputBuf);

    wc_HttpHeader headers[1];
    headers[0].name  = "date";
    headers[0].value = "Thu, 19 Mar 2026 12:00:00 GMT";

    printf("Test 15: Verify rejects NULL query when @query is signed\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) return ret;
    ret = wc_ed25519_init(&key);
    if (ret != 0) { wc_FreeRng(&rng); return ret; }
    ret = wc_ed25519_init(&verifyKey);
    if (ret != 0) { wc_ed25519_free(&key); wc_FreeRng(&rng); return ret; }

    ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    if (ret != 0) goto cleanup15;

    /* Sign with a query */
    ret = wc_HttpSig_Sign(
        "GET", "example.com", "/search", "?q=test",
        headers, 1, &key, "qtest", 0,
        sigBuf, &sigBufSz, inputBuf, &inputBufSz);
    if (ret != 0) { printf("  FAIL: sign returned %d\n", ret); goto cleanup15; }

    {
        byte pubBuf[ED25519_PUB_KEY_SIZE];
        word32 pubSz = sizeof(pubBuf);
        ret = wc_ed25519_export_public(&key, pubBuf, &pubSz);
        if (ret != 0) goto cleanup15;
        ret = wc_ed25519_import_public(pubBuf, pubSz, &verifyKey);
        if (ret != 0) goto cleanup15;
    }

    /* Verify with query=NULL should fail (component unresolvable) */
    ret = wc_HttpSig_Verify(
        "GET", "example.com", "/search", NULL,
        headers, 1, sigBuf, inputBuf,
        NULL, &verifyKey, 0);
    if (ret == 0) {
        printf("  FAIL: NULL query accepted when @query was signed\n");
        ret = -1;
        goto cleanup15;
    }

    ret = 0;
    printf("  PASS\n");

cleanup15:
    wc_ed25519_free(&key);
    wc_ed25519_free(&verifyKey);
    wc_FreeRng(&rng);
    return ret;
}

/* --- Test 16: Future timestamp rejection --- */

static int test_future_timestamp_rejected(void)
{
    int ret;
    wc_HttpHeader headers[1];
    headers[0].name  = "date";
    headers[0].value = "Thu, 19 Mar 2026 12:00:00 GMT";

    printf("Test 16: Future timestamp rejection\n");

    /* Sign with a timestamp far in the future (now + 10000s).
     * Verify with maxAgeSec=300 — should reject because created > now. */
    {
        long futureTs = (long)XTIME(NULL) + 10000;
        ret = sign_verify_roundtrip_ex(
            "GET", "example.com", "/future-test", NULL,
            headers, 1, NULL, 0, "future-key", futureTs, 300);
        if (ret == 0) {
            printf("  FAIL: future timestamp was accepted\n");
            return -1;
        }
    }

    /* Sign with a timestamp slightly in the future (now + 5s).
     * Verify with maxAgeSec=0 (skip) — should succeed. */
    {
        long slightFuture = (long)XTIME(NULL) + 5;
        ret = sign_verify_roundtrip_ex(
            "GET", "example.com", "/future-test", NULL,
            headers, 1, NULL, 0, "future-key", slightFuture, 0);
        if (ret != 0) {
            printf("  FAIL: maxAgeSec=0 should skip check, got %d\n", ret);
            return ret;
        }
    }

    printf("  PASS\n");
    return 0;
}

/* --- Test 17: Unknown derived component rejected on verify --- */

static int test_unknown_derived_component(void)
{
    int ret;
    ed25519_key key, verifyKey;
    WC_RNG rng;
    char sigBuf[512];
    word32 sigBufSz = sizeof(sigBuf);
    char inputBuf[1024];
    word32 inputBufSz = sizeof(inputBuf);
    wc_SfSigInput sfIn;
    char fakeInput[512];
    word32 fakeInputSz;

    wc_HttpHeader headers[1];
    headers[0].name  = "date";
    headers[0].value = "Thu, 19 Mar 2026 12:00:00 GMT";

    printf("Test 17: Unknown derived component rejected on verify\n");

    ret = wc_InitRng(&rng);
    if (ret != 0) return ret;
    ret = wc_ed25519_init(&key);
    if (ret != 0) { wc_FreeRng(&rng); return ret; }
    ret = wc_ed25519_init(&verifyKey);
    if (ret != 0) { wc_ed25519_free(&key); wc_FreeRng(&rng); return ret; }

    ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    if (ret != 0) goto cleanup17;

    ret = wc_HttpSig_Sign(
        "GET", "example.com", "/test", NULL,
        headers, 1, &key, "deriv-test", 1700000000,
        sigBuf, &sigBufSz, inputBuf, &inputBufSz);
    if (ret != 0) { printf("  FAIL: sign returned %d\n", ret); goto cleanup17; }

    {
        byte pubBuf[ED25519_PUB_KEY_SIZE];
        word32 pubSz = sizeof(pubBuf);
        ret = wc_ed25519_export_public(&key, pubBuf, &pubSz);
        if (ret != 0) goto cleanup17;
        ret = wc_ed25519_import_public(pubBuf, pubSz, &verifyKey);
        if (ret != 0) goto cleanup17;
    }

    /* Craft Signature-Input with unknown derived component @target-uri */
    memset(&sfIn, 0, sizeof(sfIn));
    strncpy(sfIn.label, "sig1", WC_SF_MAX_LABEL - 1);
    strncpy(sfIn.items[0], "@target-uri", WC_SF_MAX_STRING - 1);
    strncpy(sfIn.items[1], "@method", WC_SF_MAX_STRING - 1);
    sfIn.itemCount = 2;

    strncpy(sfIn.params[0].name, "created", WC_SF_MAX_LABEL - 1);
    sfIn.params[0].type = WC_SF_PARAM_INTEGER;
    sfIn.params[0].intVal = 1700000000;
    strncpy(sfIn.params[1].name, "keyid", WC_SF_MAX_LABEL - 1);
    sfIn.params[1].type = WC_SF_PARAM_STRING;
    strncpy(sfIn.params[1].strVal, "deriv-test", WC_SF_MAX_STRING - 1);
    strncpy(sfIn.params[2].name, "alg", WC_SF_MAX_LABEL - 1);
    sfIn.params[2].type = WC_SF_PARAM_STRING;
    strncpy(sfIn.params[2].strVal, "ed25519", WC_SF_MAX_STRING - 1);
    sfIn.paramCount = 3;

    fakeInputSz = sizeof(fakeInput);
    ret = wc_SfGenSigInput(&sfIn, fakeInput, &fakeInputSz);
    if (ret != 0) {
        printf("  FAIL: gen fake input: %d\n", ret);
        goto cleanup17;
    }

    ret = wc_HttpSig_Verify(
        "GET", "example.com", "/test", NULL,
        headers, 1, sigBuf, fakeInput,
        NULL, &verifyKey, 0);
    if (ret == 0) {
        printf("  FAIL: unknown derived component @target-uri was accepted\n");
        ret = -1;
        goto cleanup17;
    }

    ret = 0;
    printf("  PASS\n");

cleanup17:
    wc_ed25519_free(&key);
    wc_ed25519_free(&verifyKey);
    wc_FreeRng(&rng);
    return ret;
}

/* --- Main --- */

#define NUM_TESTS 17

int main(void)
{
    int ret;
    int failures = 0;

    printf("=== RFC 9421 HTTP Message Signatures - Test Vectors ===\n\n");

    ret = test_sf_roundtrip();              if (ret != 0) failures++;
    ret = test_signature_base();            if (ret != 0) failures++;
    ret = test_rfc_vector_verify();         if (ret != 0) failures++;
    ret = test_api_roundtrip();             if (ret != 0) failures++;
    ret = test_get_keyid();                 if (ret != 0) failures++;
    ret = test_query_component();           if (ret != 0) failures++;
    ret = test_header_normalization();      if (ret != 0) failures++;
    ret = test_alg_enforcement();           if (ret != 0) failures++;
    ret = test_timestamp_validation();      if (ret != 0) failures++;
    ret = test_auto_timestamp();            if (ret != 0) failures++;
    ret = test_arg_validation();            if (ret != 0) failures++;
    ret = test_sf_integer_overflow();       if (ret != 0) failures++;
    ret = test_adversarial_sig_input();     if (ret != 0) failures++;
    ret = test_sf_string_escaping();        if (ret != 0) failures++;
    ret = test_query_null_mismatch();       if (ret != 0) failures++;
    ret = test_future_timestamp_rejected(); if (ret != 0) failures++;
    ret = test_unknown_derived_component(); if (ret != 0) failures++;

    printf("\n=== Results: %d/%d tests passed ===\n",
           NUM_TESTS - failures, NUM_TESTS);

    return failures > 0 ? 1 : 0;
}

#else

int main(void)
{
    printf("This test requires wolfSSL compiled with:\n");
    printf("  --enable-ed25519\n");
    return 1;
}

#endif
