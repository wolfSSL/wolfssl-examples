/* http_client_signed.c
 *
 * Minimal HTTP client demonstrating RFC 9421 signed requests using
 * wolfCrypt Ed25519.
 *
 * Connects to localhost:8080 (http_server_verify), signs HTTP requests,
 * and sends them. Includes a tamper test to demonstrate rejection.
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
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common/wc_http_sig.h"

#if defined(HAVE_ED25519) && defined(HAVE_ED25519_SIGN)

/* Portable case-insensitive string comparison (avoids POSIX strcasecmp
 * which is unavailable on some toolchains). */
static int ci_strcmp(const char* a, const char* b)
{
    while (*a && *b) {
        int ca = (*a >= 'A' && *a <= 'Z') ? *a + ('a' - 'A') : *a;
        int cb = (*b >= 'A' && *b <= 'Z') ? *b + ('a' - 'A') : *b;
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

#define SERVER_PORT 8080
#define MAX_HTTP_SZ 4096

/* RFC 9421 Appendix B.1.4 — Ed25519 private key seed (demo only) */
static const byte kDemoPrivKey[ED25519_KEY_SIZE] = {
    0x9f, 0x83, 0x62, 0xf8, 0x7a, 0x48, 0x4a, 0x95,
    0x4e, 0x6e, 0x74, 0x0c, 0x5b, 0x4c, 0x0e, 0x84,
    0x22, 0x91, 0x39, 0xa2, 0x0a, 0xa8, 0xab, 0x56,
    0xff, 0x66, 0x58, 0x6f, 0x6a, 0x7d, 0x29, 0xc5
};

static int do_connect(void)
{
    int fd;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(SERVER_PORT);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect (is the server running?)");
        close(fd);
        return -1;
    }
    return fd;
}

static int read_response(int fd)
{
    char buf[1024];
    int n, status = 0;

    n = (int)recv(fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        printf("[Client] No response received\n");
        return -1;
    }
    buf[n] = '\0';

    if (sscanf(buf, "HTTP/1.1 %d", &status) != 1)
        status = 0;

    printf("[Client] Response: %d", status);

    {
        const char* body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            if (*body) printf(" — %s", body);
        }
    }

    return status;
}

/* Build an HTTP/1.1 request string with Signature headers.
 * If tamperHdr is not NULL, replaces that header's value with tamperVal
 * in the raw HTTP output (simulating a man-in-the-middle modification). */
static int build_http_request(
    const char* method, const char* path, const char* query,
    const char* authority,
    wc_HttpHeader* headers, int hdrCount,
    const char* sigValue, word32 sigValueLen,
    const char* sigInputValue, word32 sigInputLen,
    const char* tamperHdr, const char* tamperVal,
    char* out, int outSz)
{
    int pos = 0, n;
    int i;

#define SAFE_SNPRINTF(pos, out, outSz, ...) do {              \
    if ((pos) >= (outSz)) return -1;                          \
    n = snprintf((out) + (pos), (outSz) - (pos), __VA_ARGS__); \
    if (n < 0 || n >= (outSz) - (pos)) return -1;            \
    (pos) += n;                                               \
} while (0)

    SAFE_SNPRINTF(pos, out, outSz, "%s %s%s HTTP/1.1\r\n",
                  method, path, query ? query : "");
    SAFE_SNPRINTF(pos, out, outSz, "Host: %s\r\n", authority);

    for (i = 0; i < hdrCount; i++) {
        const char* val = headers[i].value;
        if (tamperHdr && ci_strcmp(headers[i].name, tamperHdr) == 0)
            val = tamperVal;
        SAFE_SNPRINTF(pos, out, outSz, "%s: %s\r\n",
                      headers[i].name, val);
    }

    SAFE_SNPRINTF(pos, out, outSz, "Signature-Input: %.*s\r\n",
                  (int)sigInputLen, sigInputValue);
    SAFE_SNPRINTF(pos, out, outSz, "Signature: %.*s\r\n",
                  (int)sigValueLen, sigValue);
    SAFE_SNPRINTF(pos, out, outSz, "\r\n");

#undef SAFE_SNPRINTF

    return pos;
}

static int demo_request(ed25519_key* key,
                        const char* label,
                        const char* method,
                        const char* path,
                        const char* query,
                        const char* authority,
                        wc_HttpHeader* headers, int hdrCount,
                        const char* tamperHdr, const char* tamperVal,
                        int expectStatus)
{
    char sigBuf[512];
    word32 sigBufSz = sizeof(sigBuf);
    char inputBuf[1024];
    word32 inputBufSz = sizeof(inputBuf);
    char httpReq[MAX_HTTP_SZ];
    int httpLen, fd, status, ret;

    printf("\n--- %s ---\n", label);
    printf("[Client] Signing: %s %s%s\n", method, path, query ? query : "");

    ret = wc_HttpSig_Sign(method, authority, path, query,
                          headers, hdrCount,
                          key, "test-key-ed25519", 0,
                          sigBuf, &sigBufSz,
                          inputBuf, &inputBufSz);
    if (ret != 0) {
        printf("[Client] Sign failed: %d (%s)\n", ret, wc_GetErrorString(ret));
        return -1;
    }

    if (tamperHdr)
        printf("[Client] Tampering: changing %s header after signing\n",
               tamperHdr);

    httpLen = build_http_request(
        method, path, query, authority,
        headers, hdrCount,
        sigBuf, sigBufSz,
        inputBuf, inputBufSz,
        tamperHdr, tamperVal,
        httpReq, sizeof(httpReq));
    if (httpLen < 0) {
        printf("[Client] Failed to build HTTP request\n");
        return -1;
    }

    fd = do_connect();
    if (fd < 0) return -1;

    send(fd, httpReq, (size_t)httpLen, 0);
    status = read_response(fd);
    close(fd);

    if (status == expectStatus) {
        printf("[Client] Result: correct (expected %d)\n", expectStatus);
        return 0;
    } else {
        printf("[Client] Result: UNEXPECTED (got %d, expected %d)\n",
               status, expectStatus);
        return -1;
    }
}

int main(void)
{
    int ret, failures = 0;
    ed25519_key key;

    wc_HttpHeader headers[2];
    headers[0].name  = "Date";
    headers[0].value = "Thu, 19 Mar 2026 12:00:00 GMT";
    headers[1].name  = "Content-Type";
    headers[1].value = "application/json";

    printf("=== RFC 9421 HTTP Client - Signed Requests ===\n");
    printf("[Client] Connecting to localhost:%d\n", SERVER_PORT);

    ret = wc_ed25519_init(&key);
    if (ret != 0) {
        printf("Failed to init key: %d\n", ret);
        return 1;
    }
    ret = wc_ed25519_import_private_only(kDemoPrivKey, ED25519_KEY_SIZE, &key);
    if (ret != 0) {
        printf("Failed to import private key: %d\n", ret);
        wc_ed25519_free(&key);
        return 1;
    }
    {
        byte pubBuf[ED25519_PUB_KEY_SIZE];
        word32 pubSz = ED25519_PUB_KEY_SIZE;
        ret = wc_ed25519_make_public(&key, pubBuf, pubSz);
        if (ret != 0) {
            printf("Failed to derive public key: %d\n", ret);
            wc_ed25519_free(&key);
            return 1;
        }
        ret = wc_ed25519_import_private_key(kDemoPrivKey, ED25519_KEY_SIZE,
                                            pubBuf, pubSz, &key);
        if (ret != 0) {
            printf("Failed to import keypair: %d\n", ret);
            wc_ed25519_free(&key);
            return 1;
        }
    }

    /* Demo 1: Valid signed request */
    ret = demo_request(&key,
        "Demo 1: Valid signed request",
        "POST", "/api/resource", "?action=update",
        "localhost:8080",
        headers, 2,
        NULL, NULL,
        200);
    if (ret != 0) failures++;

    /* Demo 2: Tampered header (Date modified after signing) */
    ret = demo_request(&key,
        "Demo 2: Tampered request (modified Date)",
        "POST", "/api/resource", "?action=update",
        "localhost:8080",
        headers, 2,
        "Date", "Fri, 20 Mar 2026 12:00:00 GMT",
        401);
    if (ret != 0) failures++;

    /* Demo 3: Valid GET without query */
    ret = demo_request(&key,
        "Demo 3: Valid GET request",
        "GET", "/status", NULL,
        "localhost:8080",
        headers, 1,
        NULL, NULL,
        200);
    if (ret != 0) failures++;

    printf("\n=== Results: %d/3 demos passed ===\n", 3 - failures);

    wc_ed25519_free(&key);
    return failures > 0 ? 1 : 0;
}

#else

int main(void)
{
    printf("This example requires wolfSSL compiled with --enable-ed25519\n");
    return 1;
}

#endif
