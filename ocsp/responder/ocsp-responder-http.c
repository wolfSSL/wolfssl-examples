/* ocsp-responder-http.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* Minimal HTTP OCSP responder. Accepts POST requests with DER-encoded OCSP
 * requests and returns DER-encoded OCSP responses. Intentionally kept as
 * small as possible. Not production-hardened.
 *
 * Usage:
 *   ./ocsp-responder-http <port> <ca-cert> <ca-key> [cert-to-mark-good ...]
 *
 * Any certificates listed after the CA key will have their serial numbers
 * registered as CERT_GOOD. Serials not registered get CERT_UNKNOWN.
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/ocsp.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/asn_public.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_OCSP) && defined(HAVE_OCSP_RESPONDER) && \
    !defined(NO_FILESYSTEM)

#include "ocsp-load-certs.h"

#include <strings.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SZ 65536

static volatile int running = 1;

/* Case-insensitive substring search (for HTTP headers per RFC 7230) */
static char* FindHeaderCI(const char* haystack, const char* needle)
{
    size_t nLen = strlen(needle);
    while (*haystack) {
        if (strncasecmp(haystack, needle, nLen) == 0)
            return (char*)haystack;
        haystack++;
    }
    return NULL;
}

/* Large buffers as static globals to avoid 128KB on the stack each iteration */
static byte httpBuf[BUF_SZ];
static byte respBuf[BUF_SZ];

static void sigHandler(int sig)
{
    (void)sig;
    running = 0;
}

/* Register a certificate's serial as CERT_GOOD in the responder by loading
 * and parsing it to extract the serial number. */
static int AddGoodCert(OcspResponder* resp, const char* caSubject,
                       word32 caSubjectSz, const char* certFile)
{
    byte* certDer;
    int certDerSz = 0;
    DecodedCert dc;
    byte serial[32];
    word32 serialSz = sizeof(serial);
    int ret;

    certDer = LoadCertDer(certFile, &certDerSz);
    if (!certDer)
        return -1;

    wc_InitDecodedCert(&dc, certDer, (word32)certDerSz, NULL);
    ret = wc_ParseCert(&dc, CERT_TYPE, 0, NULL);
    if (ret != 0) {
        wc_FreeDecodedCert(&dc);
        free(certDer);
        return ret;
    }

    ret = wc_GetDecodedCertSerial(&dc, serial, &serialSz);
    wc_FreeDecodedCert(&dc);
    free(certDer);
    if (ret != 0)
        return ret;

    return wc_OcspResponder_SetCertStatus(resp, caSubject, caSubjectSz,
               serial, serialSz, CERT_GOOD, 0, WC_CRL_REASON_UNSPECIFIED,
               86400);
}

/* Receive full HTTP request. */
static int RecvHttp(int fd, byte* buf, int bufSz)
{
    int total = 0, contentLen = 0, headerEnd = 0;

    while (total < bufSz - 1) {
        int n = (int)recv(fd, buf + total, (size_t)(bufSz - 1 - total), 0);
        if (n <= 0) break;
        total += n;
        buf[total] = '\0';

        if (!headerEnd) {
            char* hdrEnd = strstr((char*)buf, "\r\n\r\n");
            if (hdrEnd) {
                char* cl;
                headerEnd = (int)(hdrEnd - (char*)buf) + 4;
                cl = FindHeaderCI((char*)buf, "Content-Length:");
                if (cl) {
                    long val = strtol(cl + 15, NULL, 10);
                    if (val > 0 && val < bufSz)
                        contentLen = (int)val;
                }
            }
        }
        if (headerEnd && contentLen > 0 && total >= headerEnd + contentLen)
            break;
    }
    return total;
}

/* Extract POST body from HTTP request */
static int ParsePost(const byte* http, int httpSz,
                     const byte** body, int* bodySz)
{
    const char* hdr = (const char*)http;
    const char* end;
    const char* cl;
    int offset;

    *body = NULL;
    *bodySz = 0;

    if (strncmp(hdr, "POST ", 5) != 0)
        return -1;

    end = strstr(hdr, "\r\n\r\n");
    if (!end) return -1;
    offset = (int)(end - hdr) + 4;

    cl = FindHeaderCI(hdr, "Content-Length:");
    if (cl) {
        long val = strtol(cl + 15, NULL, 10);
        if (val <= 0 || val > httpSz - offset)
            return -1;
        *bodySz = (int)val;
    }
    else {
        *bodySz = httpSz - offset;
    }

    if (offset + *bodySz > httpSz)
        return -1;

    *body = http + offset;
    return 0;
}

static int SendAll(int fd, const void* data, int sz)
{
    const byte* p = (const byte*)data;
    int remaining = sz;
    while (remaining > 0) {
        int n = (int)send(fd, p, (size_t)remaining, 0);
        if (n <= 0) return -1;
        p += n;
        remaining -= n;
    }
    return sz;
}

static void SendOcspResp(int fd, const byte* resp, int respSz)
{
    char hdr[256];
    int hdrLen;

    hdrLen = snprintf(hdr, sizeof(hdr),
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: application/ocsp-response\r\n"
        "Content-Length: %d\r\n"
        "\r\n", respSz);

    SendAll(fd, hdr, hdrLen);
    SendAll(fd, resp, respSz);
}

static void SendHttpError(int fd, int code, const char* msg)
{
    char buf[256];
    int len = snprintf(buf, sizeof(buf),
        "HTTP/1.0 %d %s\r\nContent-Length: 0\r\n\r\n", code, msg);
    SendAll(fd, buf, len);
}

int main(int argc, char** argv)
{
    int port;
    const char* certFile;
    const char* keyFile;
    byte *caCertDer = NULL, *caKeyDer = NULL;
    int caCertDerSz = 0, caKeyDerSz = 0;
    OcspResponder* responder = NULL;
    DecodedCert caCert;
    int caCertInit = 0;
    char caSubject[256];
    word32 caSubjectSz = sizeof(caSubject);
    int sockfd = -1, clientfd, opt = 1, i, ret = 0;
    struct sockaddr_in addr;
    struct sigaction sa;

    if (argc < 4) {
        printf("Usage: %s <port> <ca-cert> <ca-key> [good-cert ...]\n\n"
               "  port         Listen port\n"
               "  ca-cert      CA certificate (PEM)\n"
               "  ca-key       CA private key (PEM)\n"
               "  good-cert    Certificate(s) to mark as GOOD\n",
               argv[0]);
        return 1;
    }

    port     = atoi(argv[1]);
    certFile = argv[2];
    keyFile  = argv[3];

    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init failed\n");
        return 1;
    }

    sa.sa_handler = sigHandler;
    sa.sa_flags = 0; /* No SA_RESTART so accept() returns on signal */
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Ignore SIGPIPE so client disconnections during writes don't crash */
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);

    caCertDer = LoadCertDer(certFile, &caCertDerSz);
    caKeyDer = LoadKeyDer(keyFile, &caKeyDerSz);
    if (!caCertDer || !caKeyDer) {
        fprintf(stderr, "Error loading cert/key\n");
        ret = -1;
        goto cleanup;
    }

    wc_InitDecodedCert(&caCert, caCertDer, (word32)caCertDerSz, NULL);
    caCertInit = 1;
    if (wc_ParseCert(&caCert, CERT_TYPE, 0, NULL) != 0) {
        fprintf(stderr, "Error parsing CA cert\n");
        ret = -1;
        goto cleanup;
    }

    if (wc_GetDecodedCertSubject(&caCert, caSubject, &caSubjectSz) != 0) {
        fprintf(stderr, "Error getting CA subject\n");
        ret = -1;
        goto cleanup;
    }

    responder = wc_OcspResponder_new(NULL, 1);
    if (!responder) {
        fprintf(stderr, "Error creating responder\n");
        ret = -1;
        goto cleanup;
    }

    if (wc_OcspResponder_AddSigner(responder, caCertDer, (word32)caCertDerSz,
                                   caKeyDer, (word32)caKeyDerSz,
                                   NULL, 0) != 0) {
        fprintf(stderr, "Error adding signer\n");
        ret = -1;
        goto cleanup;
    }

    /* Register any extra cert arguments as GOOD */
    for (i = 4; i < argc; i++) {
        if (AddGoodCert(responder, caSubject, caSubjectSz, argv[i]) == 0)
            printf("Registered GOOD: %s\n", argv[i]);
        else
            fprintf(stderr, "Warning: could not register %s\n", argv[i]);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        ret = -1;
        goto cleanup;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        ret = -1;
        goto cleanup;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        ret = -1;
        goto cleanup;
    }
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        ret = -1;
        goto cleanup;
    }
    printf("OCSP responder listening on port %d\n", port);

    while (running) {
        word32 respSz;
        const byte* ocspReq;
        int ocspReqSz, recvLen;
        struct timeval tv;

        clientfd = accept(sockfd, NULL, NULL);
        if (clientfd < 0) continue;

        /* Set receive timeout so incomplete requests don't block forever */
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        recvLen = RecvHttp(clientfd, httpBuf, BUF_SZ);
        if (recvLen <= 0 ||
            ParsePost(httpBuf, recvLen, &ocspReq, &ocspReqSz) != 0) {
            SendHttpError(clientfd, 400, "Bad Request");
            close(clientfd);
            continue;
        }

        respSz = sizeof(respBuf);
        if (wc_OcspResponder_WriteResponse(responder, ocspReq,
                (word32)ocspReqSz, respBuf, &respSz) != 0) {
            respSz = sizeof(respBuf);
            wc_OcspResponder_WriteErrorResponse(OCSP_INTERNAL_ERROR,
                                                respBuf, &respSz);
        }

        SendOcspResp(clientfd, respBuf, (int)respSz);
        close(clientfd);
    }

    printf("\nShutdown.\n");

cleanup:
    if (sockfd >= 0) close(sockfd);
    if (responder) wc_OcspResponder_free(responder);
    if (caCertInit) wc_FreeDecodedCert(&caCert);
    free(caCertDer);
    if (caKeyDer) {
        /* Zero the CA private key material before releasing the buffer. */
        wc_ForceZero(caKeyDer, (word32)caKeyDerSz);
        free(caKeyDer);
    }
    wolfSSL_Cleanup();
    return ret;
}

#else

int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    printf("This example requires --enable-ocsp --enable-ocsp-responder\n");
    return 0;
}

#endif /* HAVE_OCSP && HAVE_OCSP_RESPONDER && !NO_FILESYSTEM */
