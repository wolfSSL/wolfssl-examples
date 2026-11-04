/* ocsp_nonblock_async.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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
 **/

#define _GNU_SOURCE /* getaddrinfo_a() */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>

#include <wolfssl/error-ssl.h>

#include <wolfssl/wolfcrypt/asn.h>

#define SERVER_NAME "www.digicert.com"

#define SERVER_PORT 443
#define ALPN_PROTOS "http/1.1"
#define HANDSHAKE_TIMEOUT_SEC 15 /* connect + handshake + OCSP lookup limit */
#ifndef HTTP_SCRATCH_BUFFER_SIZE
    #define HTTP_SCRATCH_BUFFER_SIZE 512
#endif
#ifndef MAX_URL_ITEM_SIZE
    #define MAX_URL_ITEM_SIZE 80
#endif

#if defined(WOLFSSL_ASYNC_CRYPT) && defined(HAVE_SNI) && defined(HAVE_ALPN) \
    && defined(WOLFSSL_NONBLOCK_OCSP) && defined(HAVE_CERTIFICATE_STATUS_REQUEST) \
    && defined(HAVE_CERTIFICATE_STATUS_REQUEST_V2)

/* Bundled root for digicert.com so the demo runs standalone. Pass a cert
 * file as argv[1] if that chain changes. */
static const char* sys_certs_file = "ca_certs/DigiCert_Global_Root_G2.pem";

/* One absolute deadline for the whole connection attempt: the TCP connect, the
 * TLS handshake and the OCSP responder lookup inside it. */
static time_t deadline;

/* Seconds left until the deadline, or 0 when it has passed. */
static int remaining_sec(void)
{
    time_t now = time(NULL);
    return (now < deadline) ? (int)(deadline - now) : 0;
}

/* Wait until sockfd is ready for the given poll events or the deadline passes.
 * Returns 0 when ready, -1 on timeout or error. */
static int wait_for_socket(int sockfd, short events)
{
    struct pollfd pfd;
    int remain = remaining_sec();
    int ret;

    if (remain == 0) {
        return -1;
    }
    pfd.fd = sockfd;
    pfd.events = events;
    pfd.revents = 0;
    ret = poll(&pfd, 1, remain * 1000);
    return (ret > 0) ? 0 : -1;
}

/* Everything an in-flight lookup refers to, in one heap block. If the lookup
 * cannot be cancelled it keeps using this after resolve_with_deadline()
 * returns, so nothing here may live on the caller's stack. */
struct resolve_req {
    struct gaicb req;
    struct addrinfo hints;
    char host[MAX_URL_ITEM_SIZE];
    char service[6];
};

/* Resolve host:service within the deadline. getaddrinfo() has no timeout, so
 * use glibc's asynchronous resolver and wait only for the time that is left.
 * Returns 0 with *res set (caller frees), -1 on failure. */
static int resolve_with_deadline(const char* host, const char* service,
                                 struct addrinfo** res)
{
    struct resolve_req* r;
    struct gaicb* list[1];
    struct timespec timeout;
    int ret;

    r = calloc(1, sizeof(*r));
    if (r == NULL) {
        fprintf(stderr, "out of memory\n");
        return -1;
    }
    if (snprintf(r->host, sizeof(r->host), "%s", host) >= (int)sizeof(r->host)
            || snprintf(r->service, sizeof(r->service), "%s", service)
                    >= (int)sizeof(r->service)) {
        fprintf(stderr, "host or port too long\n");
        free(r);
        return -1;
    }
    r->hints.ai_family = AF_INET;
    r->hints.ai_socktype = SOCK_STREAM;
    r->req.ar_name = r->host;
    r->req.ar_service = r->service;
    r->req.ar_request = &r->hints;
    list[0] = &r->req;

    if (getaddrinfo_a(GAI_NOWAIT, list, 1, NULL) != 0) {
        fprintf(stderr, "getaddrinfo_a() failed\n");
        free(r);
        return -1;
    }

    timeout.tv_sec = remaining_sec();
    timeout.tv_nsec = 0;
    ret = gai_suspend((const struct gaicb* const*)list, 1, &timeout);
    if (ret != 0) {
        fprintf(stderr, "failed to resolve %s within %d seconds\n", host,
                HANDSHAKE_TIMEOUT_SEC);
        ret = gai_cancel(&r->req);
        if (ret == EAI_ALLDONE && gai_error(&r->req) == 0) {
            freeaddrinfo(r->req.ar_result);
        }
        if (ret != EAI_NOTCANCELED) {
            free(r);
        }
        /* else still running: the block stays with it for good */
        return -1;
    }

    ret = gai_error(&r->req);
    if (ret != 0) {
        fprintf(stderr, "failed to resolve %s: %s\n", host, gai_strerror(ret));
        free(r);
        return -1;
    }
    *res = r->req.ar_result;
    free(r);
    return 0;
}

/* Make sockfd non-blocking. Returns 0 on success, -1 on failure. */
static int set_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);

    if (flags == -1) {
        fprintf(stderr, "fcntl(F_GETFL) failed\n");
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) != 0) {
        fprintf(stderr, "fcntl(F_SETFL) failed\n");
        return -1;
    }
    return 0;
}

/* Connect a non-blocking socket, waiting for completion against the deadline.
 * Returns 0 on success, -1 on failure. */
static int connect_nonblocking(int sockfd, const struct sockaddr* addr,
                               socklen_t addrLen)
{
    int err = 0;
    socklen_t errLen = sizeof(err);

    if (connect(sockfd, addr, addrLen) == 0) {
        return 0;
    }
    if (errno != EINPROGRESS) {
        return -1;
    }
    if (wait_for_socket(sockfd, POLLOUT) != 0) {
        return -1;
    }
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &errLen) != 0) {
        return -1;
    }
    return (err == 0) ? 0 : -1;
}

/* Resolve SERVER_NAME and connect a non-blocking socket within the deadline.
 * Returns the connected socket, or -1 on failure. */
static int open_socket(void)
{
    struct addrinfo* res = NULL;
    struct addrinfo* ai;
    char portStr[6];
    int sockfd = -1;
    int ret;

    ret = snprintf(portStr, sizeof(portStr), "%d", SERVER_PORT);
    if (ret < 0) {
        fprintf(stderr, "failed to format port %d\n", SERVER_PORT);
        return -1;
    }
    if ((size_t)ret >= sizeof(portStr)) {
        fprintf(stderr, "failed to format port %d\n", SERVER_PORT);
        return -1;
    }

    /* Resolve the server by name so it keeps working as the IP changes. */
    if (resolve_with_deadline(SERVER_NAME, portStr, &res) != 0) {
        return -1;
    }

    for (ai = res; ai != NULL; ai = ai->ai_next) {
        sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sockfd == -1) {
            continue;
        }
        /* Go non-blocking before connect() so the connect itself is held to
         * the deadline rather than the kernel's own timeout. */
        if (set_nonblocking(sockfd) == 0 &&
                connect_nonblocking(sockfd, ai->ai_addr, ai->ai_addrlen) == 0) {
            break;
        }
        if (close(sockfd) != 0) {
            fprintf(stderr, "warning: failed to close socket\n");
        }
        sockfd = -1;
    }
    freeaddrinfo(res);

    if (sockfd == -1) {
        fprintf(stderr, "failed to connect to %s:%d within %d seconds\n",
                SERVER_NAME, SERVER_PORT, HANDSHAKE_TIMEOUT_SEC);
        return -1;
    }

    return sockfd;
}

static int verify_cb(int preverify_ok, WOLFSSL_X509_STORE_CTX* store)
{
    printf("verify_cb()\n");
    printf("  preverify_ok = %d\n", preverify_ok);
    if (preverify_ok == 0) {
        printf("  VERIFY FAILED\n");
        printf("  store->error_depth: %d\n", store->error_depth);
        printf("  store->error: %d\n", store->error);
    }

    return preverify_ok;
}

#ifdef WOLFSSL_OCSP_SCREEN_RESPONDER
/* 1 if the IPv4 address is in a loopback, private, link-local or reserved
 * range that an OCSP responder must not live in, else 0. */
static int ipv4_blocked(const struct in_addr* in)
{
    const unsigned char* a = (const unsigned char*)&in->s_addr;

    return (a[0] == 0) ||                            /* 0.0.0.0/8 */
           (a[0] == 127) ||                          /* loopback */
           (a[0] == 10) ||                           /* 10/8 */
           (a[0] == 172 && (a[1] & 0xF0) == 16) ||   /* 172.16/12 */
           (a[0] == 192 && a[1] == 168) ||           /* 192.168/16 */
           (a[0] == 169 && a[1] == 254) ||           /* link-local */
           (a[0] == 100 && (a[1] & 0xC0) == 64) ||   /* 100.64/10 */
           (a[0] >= 224);                            /* multicast, reserved */
}
#endif

/* Resolve the responder once within the deadline, screen every address it
 * returned, then connect straight to an accepted address. Connecting to the
 * address rather than the name again means a changing DNS answer cannot slip
 * a blocked address past the screen. Returns the socket, or -1 on failure. */
static int connect_responder(const char* host, word16 port)
{
    struct addrinfo* res = NULL;
    struct addrinfo* ai;
    char portStr[6];
    int sockfd = -1;

    snprintf(portStr, sizeof(portStr), "%u", (unsigned int)port);
    if (resolve_with_deadline(host, portStr, &res) != 0) {
        return -1;
    }

#ifdef WOLFSSL_OCSP_SCREEN_RESPONDER
    /* Reject the responder if any of its addresses is off limits. */
    for (ai = res; ai != NULL; ai = ai->ai_next) {
        if (ipv4_blocked(&((struct sockaddr_in*)ai->ai_addr)->sin_addr)) {
            fprintf(stderr, "  OCSP responder %s not permitted\n", host);
            freeaddrinfo(res);
            return -1;
        }
    }
#endif

    for (ai = res; ai != NULL; ai = ai->ai_next) {
        sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sockfd == -1) {
            continue;
        }
        if (set_nonblocking(sockfd) == 0 &&
                connect_nonblocking(sockfd, ai->ai_addr, ai->ai_addrlen) == 0) {
            break;
        }
        close(sockfd);
        sockfd = -1;
    }
    freeaddrinfo(res);

    if (sockfd == -1) {
        fprintf(stderr, "  OCSP responder %s connection failed\n", host);
    }
    return sockfd;
}

/* Send all of buf on a non-blocking socket, waiting for room against the
 * deadline. Returns 0 on success, -1 on failure. */
static int send_all(int sockfd, const unsigned char* buf, int sz)
{
    int sent = 0;

    while (sent < sz) {
        int ret;

        if (wait_for_socket(sockfd, POLLOUT) != 0) {
            return -1;
        }
        ret = wolfIO_Send(sockfd, (char*)buf + sent, sz - sent, 0);
        if (ret < 0) {
            return -1;
        }
        sent += ret;
    }
    return 0;
}

/* Receive callback for the OCSP response parser: wait for data against the
 * deadline, then read. Returns the byte count, or a negative error. */
static int recv_with_deadline(char* buf, int sz, void* ctx)
{
    int sockfd = *(int*)ctx;
    ssize_t ret;

    if (wait_for_socket(sockfd, POLLIN) != 0) {
        fprintf(stderr, "  OCSP lookup timed out\n");
        return WOLFSSL_CBIO_ERR_TIMEOUT;
    }
    ret = recv(sockfd, buf, (size_t)sz, 0);
    return (ret < 0) ? WOLFSSL_CBIO_ERR_GENERAL : (int)ret;
}

/* Fetch the OCSP response over HTTP with wolfSSL's helpers, holding every step
 * to the shared deadline. Returns the response size, or a negative error. */
static int ocsp_lookup(const char* url, int urlSz, unsigned char* request,
                       int requestSz, unsigned char** response)
{
    char domainName[MAX_URL_ITEM_SIZE];
    char path[MAX_URL_ITEM_SIZE];
    word16 port = 0;
    unsigned char* httpBuf;
    int httpBufSz = HTTP_SCRATCH_BUFFER_SIZE;
    int sockfd;
    int ret = WOLFSSL_CBIO_ERR_GENERAL;

    if (wolfIO_DecodeUrl(url, urlSz, domainName, path, &port) < 0) {
        fprintf(stderr, "  unable to decode OCSP URL\n");
        return ret;
    }
    httpBuf = malloc(httpBufSz);
    if (httpBuf == NULL) {
        fprintf(stderr, "  out of memory\n");
        return ret;
    }
    httpBufSz = wolfIO_HttpBuildRequestOcsp(domainName, path, requestSz,
                                            httpBuf, httpBufSz);
    if (httpBufSz <= 0) {
        fprintf(stderr, "  unable to build OCSP request\n");
        free(httpBuf);
        return ret;
    }

    sockfd = connect_responder(domainName, port);
    if (sockfd == -1) {
        free(httpBuf);
        return ret;
    }

    if (send_all(sockfd, httpBuf, httpBufSz) != 0 ||
            send_all(sockfd, request, requestSz) != 0) {
        fprintf(stderr, "  OCSP request send failed\n");
    }
    else {
        /* The parser keeps no state between calls, so let it run once to the
         * end and have the receive callback do the waiting. */
        ret = wolfIO_HttpProcessResponseOcspGenericIO(recv_with_deadline,
                &sockfd, response, httpBuf, HTTP_SCRATCH_BUFFER_SIZE, NULL);
    }

    close(sockfd);
    free(httpBuf);
    return ret;
}

static int ocsp_cb(void* ctx, const char* url, int urlSz,
        unsigned char* request, int requestSz, unsigned char** response)
{
    /* Return "want read" once per request, then look up on retry. A static
     * flag holds the state, so no marker file is needed. */
    static int wantReadPending = 0;

    (void)ctx;
    printf("ocsp_cb(): %.*s\n", urlSz, url);

    if (!wantReadPending) {
        wantReadPending = 1;
        printf("  simulate 'want read'\n");
        return WOLFSSL_CBIO_ERR_WANT_READ;
    }
    wantReadPending = 0;

    return ocsp_lookup(url, urlSz, request, requestSz, response);
}

static void ocsp_free(void* ctx, unsigned char* response)
{
    /* wolfIO_HttpProcessResponseOcsp() allocates the response the same way
     * EmbedOcspLookup() does, so free it the same way. */
    EmbedOcspRespFree(ctx, response);
}

/* Apply the OCSP callback, TLS version policy, SNI, and ALPN.
 * Returns 0 on success, -1 on failure. */
static int configure_ssl(WOLFSSL* ssl)
{
    const long verMask = SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 |
                         SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2;
    long opt;
    int ret;

    ret = wolfSSL_SetOCSP_Cb(ssl, ocsp_cb, ocsp_free, NULL);
    if (ret != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_SetOCSP_Cb() failed with code %d\n", ret);
        return -1;
    }

    opt = wolfSSL_get_options(ssl);
    opt = (opt & ~verMask) | SSL_OP_NO_SSLv3;
    if (opt != wolfSSL_set_options(ssl, opt)) {
        fprintf(stderr, "Could not configure TLS versions on client stub\n");
        return -1;
    }

    ret = wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, SERVER_NAME,
            strlen(SERVER_NAME));
    if (ret != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_UseSNI() failed with code %d\n", ret);
        return -1;
    }

    /* Match the peer certificate name against SERVER_NAME; verifying the
     * chain alone would accept any cert issued under the loaded root. */
    ret = wolfSSL_check_domain_name(ssl, SERVER_NAME);
    if (ret != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_check_domain_name() failed with code %d\n",
                ret);
        return -1;
    }

    ret = wolfSSL_UseALPN(ssl, ALPN_PROTOS, strlen(ALPN_PROTOS),
            WOLFSSL_ALPN_FAILED_ON_MISMATCH);
    if (ret != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_UseALPN() failed with code %d\n", ret);
        return -1;
    }

    return 0;
}

/* Whether a failed wolfSSL_connect() should be retried. */
static int is_retryable(int errCode)
{
    if (errCode == SSL_ERROR_WANT_READ) {
        return 1;
    }
    if (errCode == SSL_ERROR_WANT_WRITE) {
        return 1;
    }
    if (errCode == OCSP_WANT_READ) {
        return 1;
    }
    if (errCode == WC_PENDING_E) {
        return 1;
    }
    return 0;
}

/* Drive the non-blocking handshake to completion, then shut down.
 * Returns 0 on success, -1 on failure. */
static int do_handshake(WOLFSSL* ssl)
{
    char errBuff[WOLFSSL_MAX_ERROR_SZ];
    int sockfd = wolfSSL_get_fd(ssl);
    int ret;
    int errCode;

    printf("Connecting...\n");

    for (;;) {
        ret = wolfSSL_connect(ssl);
        if (ret == SSL_SUCCESS) {
            break;
        }

        errCode = wolfSSL_get_error(ssl, ret);
        /* Only report genuine errors; the want-read/pending codes are normal
         * non-blocking progress. */
        if (!is_retryable(errCode)) {
            printf("wolfSSL_connect() returned %d (error code %d)\n", ret,
                    errCode);
        }

        if (errCode == WC_PENDING_E) {
            /* Native async: finish the pending crypto op, then retry. */
            if (wolfSSL_AsyncPoll(ssl, WOLF_POLL_FLAG_CHECK_HW) < 0) {
                fprintf(stderr, "wolfSSL_AsyncPoll() failed\n");
                return -1;
            }
        }

        if (!is_retryable(errCode)) {
            break;
        }

        /* Wait for the socket rather than spinning, and give up when the
         * deadline passes so a stalled peer fails instead of hanging. */
        if (errCode == SSL_ERROR_WANT_READ || errCode == SSL_ERROR_WANT_WRITE) {
            if (wait_for_socket(sockfd, errCode == SSL_ERROR_WANT_WRITE ?
                    POLLOUT : POLLIN) != 0) {
                fprintf(stderr, "handshake timed out after %d seconds\n",
                        HANDSHAKE_TIMEOUT_SEC);
                return -1;
            }
        }
        else if (remaining_sec() == 0) {
            fprintf(stderr, "handshake timed out after %d seconds\n",
                    HANDSHAKE_TIMEOUT_SEC);
            return -1;
        }
    }

    if (ret != SSL_SUCCESS) {
        errCode = wolfSSL_get_error(ssl, ret);
        fprintf(stderr, "wolfSSL_connect() failed with code %d\n", ret);
        fprintf(stderr, "ERROR %d: %s\n", errCode,
                wolfSSL_ERR_error_string((unsigned long) errCode, errBuff));
        if (errCode == FATAL_ERROR) {
            WOLFSSL_ALERT_HISTORY hist;
            const char* alertType;
            wolfSSL_get_alert_history(ssl, &hist);
            alertType = wolfSSL_alert_type_string_long(hist.last_rx.code);
            fprintf(stderr, "Last alert received: %d - %s\n",
                    hist.last_rx.code, alertType);
        }
        return -1;
    }

    printf("CONNECTED\n");

    printf("  Closing connection...\n");
    ret = wolfSSL_shutdown(ssl);
    if (ret == WOLFSSL_SUCCESS) {
        return 0;
    }
    /* Our close_notify was sent; on a non-blocking socket the peer's reply may
     * still be in flight, which is fine for this demo. */
    if (ret == WOLFSSL_SHUTDOWN_NOT_DONE) {
        return 0;
    }
    errCode = wolfSSL_get_error(ssl, ret);
    if (errCode == WOLFSSL_ERROR_WANT_READ) {
        return 0;
    }
    if (errCode == WOLFSSL_ERROR_WANT_WRITE) {
        return 0;
    }
    fprintf(stderr, "wolfSSL_shutdown() failed with code %d (error %d)\n",
            ret, errCode);
    return -1;
}

/* Create and configure the client CTX (verify, OCSP stapling, CA, async).
 * Returns the CTX, or NULL on failure. */
static WOLFSSL_CTX* create_ctx(int devId)
{
    WOLFSSL_METHOD* method;
    WOLFSSL_CTX* ctx;
    int err;

    method = wolfSSLv23_client_method();
    if (method == NULL) {
        fprintf(stderr, "failed to get client method\n");
        return NULL;
    }

    ctx = wolfSSL_CTX_new(method);
    if (ctx == NULL) {
        fprintf(stderr, "failed to create context object\n");
        return NULL;
    }

    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_cb);

    err = wolfSSL_CTX_UseOCSPStapling(ctx, WOLFSSL_CSR_OCSP,
            WOLFSSL_CSR_OCSP_USE_NONCE);
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_UseOCSPStapling() returned %d\n", err);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    err = wolfSSL_CTX_UseOCSPStaplingV2(ctx, WOLFSSL_CSR2_OCSP_MULTI,
            WOLFSSL_CSR2_OCSP_USE_NONCE);
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_UseOCSPStaplingV2() returned %d\n", err);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    err = wolfSSL_CTX_EnableOCSP(ctx,
            WOLFSSL_OCSP_CHECKALL | WOLFSSL_OCSP_NO_NONCE);
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_EnableOCSP() returned %d\n", err);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    err = wolfSSL_CTX_UnloadCAs(ctx);
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_UnloadCAs() returned %d\n", err);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    err = wolfSSL_CTX_load_verify_locations(ctx, sys_certs_file, NULL);
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_load_verify_locations() returned %d\n",
                err);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    if (wolfSSL_CTX_SetDevId(ctx, devId) != SSL_SUCCESS) {
        fprintf(stderr, "Could not set AsyncCrypt device on TLS context\n");
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

int test_connect(WOLFSSL_CTX* ctx)
{
    WOLFSSL* ssl;
    int sockfd;
    int result;

    deadline = time(NULL) + HANDSHAKE_TIMEOUT_SEC;

    sockfd = open_socket();
    if (sockfd == -1) {
        return -1;
    }

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        fprintf(stderr, "wolfSSL_new() failed\n");
        close(sockfd);
        printf("  CLOSED\n");
        return -1;
    }

    wolfSSL_set_fd(ssl, sockfd);
    wolfSSL_set_using_nonblock(ssl, 1);

    result = configure_ssl(ssl);
    if (result == 0) {
        result = do_handshake(ssl);
    }

    wolfSSL_free(ssl);
    close(sockfd);
    printf("  CLOSED\n");

    return result;
}
#endif

int main(int argc, char** argv)
{
 #if defined(WOLFSSL_ASYNC_CRYPT) && defined(HAVE_SNI) && defined(HAVE_ALPN) \
    && defined(WOLFSSL_NONBLOCK_OCSP) && defined(HAVE_CERTIFICATE_STATUS_REQUEST) \
    && defined(HAVE_CERTIFICATE_STATUS_REQUEST_V2)

    int result = 0;
    WC_RNG rng;
    int asyncCryptDevId = INVALID_DEVID;
    WOLFSSL_CTX* ctx;

    /* Use a caller-supplied cert file, else the bundled default. */
    if (argc == 2) {
        if (access(argv[1], F_OK) == -1) {
            fprintf(stderr, "Provided cert file %s doesn't exist."
                    " Please provide a valid path.\n", argv[1]);
            return -1;
        }
        sys_certs_file = argv[1];
    }
    else if (access(sys_certs_file, F_OK) == -1) {
        fprintf(stderr, "Default cert file %s doesn't exist."
                " Please provide a cert file path as shown below.\n",
                sys_certs_file);
        fprintf(stderr,
                "./ocsp_nonblock_async /etc/ssl/certs/ca-certificates.crt\n");
        return -1;
    }

    wolfSSL_Debugging_ON();

    if (wolfSSL_Init() != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init() failed\n");
        return -1;
    }

    wolfSSL_load_error_strings();

    printf("WolfSSL AsyncCrypt Enabled\n");
#if defined(HAVE_INTEL_QA)
    printf("WolfSSL AsyncCrypt with IntelQA Mode\n");
#elif defined(WOLFSSL_ASYNC_CRYPT_SW)
    printf("WolfSSL AsyncCrypt with Simulation Mode\n");
#else
    #error Unknown HW Acceleration device
#endif

    if (wolfAsync_DevOpen(&asyncCryptDevId) < 0) {
        fprintf(stderr, "Could not open AsyncCrypt device\n");
        wolfSSL_Cleanup();
        return -1;
    }

    if (wc_InitRng(&rng) != 0) {
        fprintf(stderr, "wc_InitRng() failed\n");
        result = -1;
    }
    else {
        ctx = create_ctx(asyncCryptDevId);
        if (ctx == NULL) {
            result = -1;
        }
        else {
            if (test_connect(ctx) != 0) {
                fprintf(stderr, "test_connect() failed\n");
                fprintf(stderr, "CONNECT FAILED\n");
                result = -1;
            }
            else {
                fprintf(stderr, "CONNECT PASSED\n");
            }
            wolfSSL_CTX_free(ctx);
        }
        wc_FreeRng(&rng);
    }

    printf("\nDONE\n\n");

    if (asyncCryptDevId != INVALID_DEVID) {
        wolfAsync_DevClose(&asyncCryptDevId);
    }
    wolfSSL_Cleanup();

    return result;
#else
    (void)argc;
    (void)argv;

    printf("Please compile wolfSSL with  ./configure --enable-asynccrypt --enable-sni"
           " --enable-alpn --enable-ocspstapling --enable-ocspstapling2 --enable-opensslextra"
           " CFLAGS=-DWOLFSSL_NONBLOCK_OCSP");
    return -1;
#endif
}
