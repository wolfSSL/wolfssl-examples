/* http_server_verify.c
 *
 * Minimal HTTP/1.1 server demonstrating RFC 9421 signature verification
 * using wolfCrypt Ed25519.
 *
 * Listens on localhost:8080, verifies Signature/Signature-Input headers,
 * and responds 200 (valid), 401 (invalid), or 400 (missing headers).
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
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common/wc_http_sig.h"

#if defined(HAVE_ED25519) && defined(HAVE_ED25519_VERIFY)

/* Portable case-insensitive string comparison with length limit
 * (avoids POSIX strncasecmp which is unavailable on some toolchains). */
static int ci_strncmp(const char* a, const char* b, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        int ca = (a[i] >= 'A' && a[i] <= 'Z') ? a[i] + ('a' - 'A') : a[i];
        int cb = (b[i] >= 'A' && b[i] <= 'Z') ? b[i] + ('a' - 'A') : b[i];
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
    }
    return 0;
}

#define LISTEN_PORT  8080
#define MAX_REQ_SZ   8192
#define MAX_HDRS     32
#define HDR_NAME_SZ  128
#define HDR_VAL_SZ   1024

/* RFC 9421 Appendix B.1.4 — Ed25519 public key (demo only) */
static const byte kDemoPubKey[ED25519_PUB_KEY_SIZE] = {
    0x26, 0xb4, 0x0b, 0x8f, 0x93, 0xff, 0xf3, 0xd8,
    0x97, 0x11, 0x2f, 0x7e, 0xbc, 0x58, 0x2b, 0x23,
    0x2d, 0xbd, 0x72, 0x51, 0x7d, 0x08, 0x2f, 0xe8,
    0x3c, 0xfb, 0x30, 0xdd, 0xce, 0x43, 0xd1, 0xbb
};
static const char* kDemoKeyId = "test-key-ed25519";

static volatile sig_atomic_t g_running = 1;
static void on_signal(int sig) { (void)sig; g_running = 0; }

typedef struct {
    char method[16];
    char path[512];
    char query[512];
    char authority[256];

    char hdrNames[MAX_HDRS][HDR_NAME_SZ];
    char hdrVals[MAX_HDRS][HDR_VAL_SZ];
    wc_HttpHeader hdrs[MAX_HDRS];
    int hdrCount;

    char sig[HDR_VAL_SZ * 2];
    char sigInput[HDR_VAL_SZ * 2];
    int hasSig;
    int hasSigInput;
} ParsedReq;

static int recv_request(int fd, char* buf, int sz)
{
    int total = 0;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (total < sz - 1) {
        int n = (int)recv(fd, buf + total, sz - 1 - total, 0);
        if (n <= 0) break;
        total += n;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) break;
    }
    return total;
}

static int parse_request(const char* raw, ParsedReq* req)
{
    const char* p = raw;
    const char* eol;
    char uri[1024];
    char* qm;
    int len;

    memset(req, 0, sizeof(*req));

    /* Request line: METHOD URI HTTP/1.x\r\n */
    eol = strstr(p, "\r\n");
    if (!eol) return -1;

    len = 0;
    while (p < eol && *p != ' ' && len < (int)sizeof(req->method) - 1)
        req->method[len++] = *p++;
    req->method[len] = '\0';
    if (*p == ' ') p++;

    len = 0;
    while (p < eol && *p != ' ' && len < (int)sizeof(uri) - 1)
        uri[len++] = *p++;
    uri[len] = '\0';

    qm = strchr(uri, '?');
    if (qm) {
        int pathLen = (int)(qm - uri);
        if (pathLen >= (int)sizeof(req->path))
            pathLen = (int)sizeof(req->path) - 1;
        memcpy(req->path, uri, pathLen);
        req->path[pathLen] = '\0';
        strncpy(req->query, qm, sizeof(req->query) - 1);
    } else {
        strncpy(req->path, uri, sizeof(req->path) - 1);
    }

    /* Headers */
    p = eol + 2;
    while (*p && !(p[0] == '\r' && p[1] == '\n')) {
        const char* colon;
        const char* valStart;
        int nameLen, valLen;

        eol = strstr(p, "\r\n");
        if (!eol) break;

        colon = memchr(p, ':', eol - p);
        if (!colon) { p = eol + 2; continue; }

        nameLen = (int)(colon - p);
        valStart = colon + 1;
        while (valStart < eol && *valStart == ' ') valStart++;
        valLen = (int)(eol - valStart);

        if (nameLen == 15 && ci_strncmp(p, "Signature-Input", 15) == 0) {
            if (valLen < (int)sizeof(req->sigInput)) {
                memcpy(req->sigInput, valStart, valLen);
                req->sigInput[valLen] = '\0';
                req->hasSigInput = 1;
            }
        } else if (nameLen == 9 && ci_strncmp(p, "Signature", 9) == 0) {
            if (valLen < (int)sizeof(req->sig)) {
                memcpy(req->sig, valStart, valLen);
                req->sig[valLen] = '\0';
                req->hasSig = 1;
            }
        } else {
            if (nameLen == 4 && ci_strncmp(p, "Host", 4) == 0) {
                if (valLen < (int)sizeof(req->authority)) {
                    memcpy(req->authority, valStart, valLen);
                    req->authority[valLen] = '\0';
                }
            }

            if (req->hdrCount < MAX_HDRS) {
                int idx = req->hdrCount;
                if (nameLen >= HDR_NAME_SZ) nameLen = HDR_NAME_SZ - 1;
                if (valLen >= HDR_VAL_SZ) valLen = HDR_VAL_SZ - 1;
                memcpy(req->hdrNames[idx], p, nameLen);
                req->hdrNames[idx][nameLen] = '\0';
                memcpy(req->hdrVals[idx], valStart, valLen);
                req->hdrVals[idx][valLen] = '\0';
                req->hdrs[idx].name = req->hdrNames[idx];
                req->hdrs[idx].value = req->hdrVals[idx];
                req->hdrCount++;
            }
        }
        p = eol + 2;
    }

    return 0;
}

static void send_response(int fd, int code,
                          const char* reason,
                          const char* body)
{
    char resp[1024];
    int len = snprintf(resp, sizeof(resp),
        "HTTP/1.1 %d %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        code, reason, (int)strlen(body), body);
    if (len < 0 || len >= (int)sizeof(resp))
        return;
    send(fd, resp, len, 0);
}

static int handle_request(int fd, ed25519_key* pubKey)
{
    char buf[MAX_REQ_SZ];
    ParsedReq req;
    int ret;
    char keyIdBuf[256];
    word32 keyIdSz = sizeof(keyIdBuf);

    ret = recv_request(fd, buf, sizeof(buf));
    if (ret <= 0) return -1;

    printf("[Server] --- New request ---\n");

    ret = parse_request(buf, &req);
    if (ret != 0) {
        printf("[Server] Failed to parse request\n");
        send_response(fd, 400, "Bad Request", "Malformed request\n");
        return -1;
    }

    printf("[Server] %s %s%s (Host: %s)\n",
           req.method, req.path, req.query, req.authority);

    if (!req.hasSig || !req.hasSigInput) {
        printf("[Server] Missing Signature or Signature-Input header\n");
        send_response(fd, 400, "Bad Request",
                      "Missing Signature or Signature-Input header\n");
        return -1;
    }

    ret = wc_HttpSig_GetKeyId(req.sigInput, NULL, keyIdBuf, &keyIdSz);
    if (ret != 0) {
        printf("[Server] Failed to extract keyid: %d\n", ret);
        send_response(fd, 400, "Bad Request", "Cannot extract keyid\n");
        return -1;
    }

    printf("[Server] Extracted keyid: \"%s\"\n", keyIdBuf);

    if (strcmp(keyIdBuf, kDemoKeyId) != 0) {
        printf("[Server] Unknown keyid → 401\n");
        send_response(fd, 401, "Unauthorized", "Unknown key\n");
        return 0;
    }

    ret = wc_HttpSig_Verify(
        req.method,
        req.authority,
        req.path,
        req.query[0] ? req.query : NULL,
        req.hdrs, req.hdrCount,
        req.sig,
        req.sigInput,
        NULL,
        pubKey,
        300
    );

    if (ret == 0) {
        printf("[Server] Signature VALID → 200 OK\n");
        send_response(fd, 200, "OK", "Signature verified\n");
    } else {
        printf("[Server] Signature INVALID (%d: %s) → 401\n",
               ret, wc_GetErrorString(ret));
        send_response(fd, 401, "Unauthorized", "Invalid signature\n");
    }

    return 0;
}

int main(void)
{
    int listenFd, clientFd, ret, opt = 1;
    struct sockaddr_in addr;
    ed25519_key pubKey;

    struct sigaction sa;

    setbuf(stdout, NULL);

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* no SA_RESTART so accept() returns EINTR */
    sigaction(SIGINT, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);

    ret = wc_ed25519_init(&pubKey);
    if (ret != 0) {
        printf("Failed to init public key: %d\n", ret);
        return 1;
    }
    ret = wc_ed25519_import_public(kDemoPubKey, ED25519_PUB_KEY_SIZE, &pubKey);
    if (ret != 0) {
        printf("Failed to import public key: %d\n", ret);
        return 1;
    }

    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) { perror("socket"); return 1; }

    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(LISTEN_PORT);

    if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listenFd);
        return 1;
    }

    if (listen(listenFd, 5) < 0) {
        perror("listen");
        close(listenFd);
        return 1;
    }

    printf("[Server] Listening on localhost:%d (Ctrl-C to stop)\n",
           LISTEN_PORT);
    printf("[Server] Accepting keyid: \"%s\"\n\n", kDemoKeyId);

    while (g_running) {
        clientFd = accept(listenFd, NULL, NULL);
        if (clientFd < 0) continue;
        handle_request(clientFd, &pubKey);
        close(clientFd);
        printf("\n");
    }

    printf("[Server] Shutting down\n");
    close(listenFd);
    wc_ed25519_free(&pubKey);
    return 0;
}

#else

int main(void)
{
    printf("This example requires wolfSSL compiled with --enable-ed25519\n");
    return 1;
}

#endif
