
/* ocsp_nonblock.c
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

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>
#include <wolfssl/error-ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

/* Requires wolfSSL: --enable-ocsp --enable-opensslextra --enable-sni,
 * CFLAGS -DWOLFSSL_NONBLOCK_OCSP -DHAVE_IO_TIMEOUT -DKEEP_PEER_CERT. */
static const char* kCACertsDir = "ca_certs"; /* put CA certs into this directory */
static const char* kServerHost = "www.digicert.com";
static const word16 kServerPort = 443;

#ifndef HTTP_SCRATCH_BUFFER_SIZE
    #define HTTP_SCRATCH_BUFFER_SIZE 512
#endif
#ifndef MAX_URL_ITEM_SIZE
    #define MAX_URL_ITEM_SIZE   80
#endif
#ifndef DEFAULT_TIMEOUT_SEC
    #define DEFAULT_TIMEOUT_SEC 10 /* cert fetch / OCSP connect limit (sec) */
#endif

static int io_timeout_sec = DEFAULT_TIMEOUT_SEC;


static SOCKET_T sfd = SOCKET_INVALID;
static word16   port;
static char     path[MAX_URL_ITEM_SIZE];
static char     domainName[MAX_URL_ITEM_SIZE];
static int      nonBlockCnt = 0;
static byte*    httpBuf;

/* Switch fd to non-blocking so I/O on it can be driven against a deadline.
 * Returns 0 on success, -1 on error. */
static int set_nonblocking(SOCKET_T fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
        return -1;
    }
    return 0;
}

/* Wait until fd is ready for the direction the caller needs, or until the
 * deadline passes. Returns 0 when ready, -1 on timeout or error. */
static int wait_for_socket(SOCKET_T fd, int wantWrite, time_t deadline)
{
    struct pollfd pfd;
    time_t now = time(NULL);
    int ret;

    if (now >= deadline) {
        return -1;
    }
    pfd.fd = fd;
    pfd.events = wantWrite ? POLLOUT : POLLIN;
    pfd.revents = 0;
    ret = poll(&pfd, 1, (int)(deadline - now) * 1000);
    return (ret > 0) ? 0 : -1;
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

/* Resolve host once, screen every address it returned, then connect straight
 * to an accepted address within timeoutSec. Connecting to the address rather
 * than the name again means a changing DNS answer cannot slip a blocked
 * address past the screen. Returns 0 with *fd set, -1 on failure. */
static int connect_responder(const char* host, word16 port, int timeoutSec,
                             SOCKET_T* fd)
{
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    struct addrinfo* ai;
    char portStr[6];
    time_t deadline = time(NULL) + timeoutSec;
    int s = -1;

    snprintf(portStr, sizeof(portStr), "%u", (unsigned int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, portStr, &hints, &res) != 0) {
        printf("Unable to resolve OCSP responder %s\n", host);
        return -1;
    }

#ifdef WOLFSSL_OCSP_SCREEN_RESPONDER
    /* Reject the responder if any of its addresses is off limits. */
    for (ai = res; ai != NULL; ai = ai->ai_next) {
        if (ipv4_blocked(&((struct sockaddr_in*)ai->ai_addr)->sin_addr)) {
            printf("OCSP responder %s not permitted\n", host);
            freeaddrinfo(res);
            return -1;
        }
    }
#endif

    for (ai = res; ai != NULL && s == -1; ai = ai->ai_next) {
        int err = 0;
        socklen_t errLen = sizeof(err);

        s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s == -1) {
            continue;
        }
        if (set_nonblocking(s) == 0 &&
                (connect(s, ai->ai_addr, ai->ai_addrlen) == 0 ||
                 (errno == EINPROGRESS &&
                  wait_for_socket(s, 1, deadline) == 0 &&
                  getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &errLen) == 0 &&
                  err == 0))) {
            break;
        }
        close(s);
        s = -1;
    }
    freeaddrinfo(res);

    if (s == -1) {
        printf("OCSP responder connection failed\n");
        return -1;
    }
    *fd = s;
    return 0;
}

/* Return size of the OCSP response or negative for error */
static int OcspLookupNonBlockCb(void* ctx, const char* url, int urlSz,
                        byte* ocspReqBuf, int ocspReqSz, byte** ocspRespBuf)
{
    int ret = WOLFSSL_CBIO_ERR_GENERAL;

    if (sfd != SOCKET_INVALID) {
        ret = wolfIO_HttpProcessResponseOcsp(sfd, ocspRespBuf,
                        httpBuf, HTTP_SCRATCH_BUFFER_SIZE, NULL);
        nonBlockCnt++;
        if (ret == OCSP_WANT_READ)
            return WOLFSSL_CBIO_ERR_WANT_READ;
        printf("OCSP Response: ret %d, nonblock count %d\n",
            ret, nonBlockCnt);
        XFREE(httpBuf, NULL, DYNAMIC_TYPE_OCSP);
        httpBuf = NULL;
        return ret;
    }

    if (ocspReqBuf == NULL || ocspReqSz == 0) {
        printf("OCSP request is required for lookup\n");
    }
    else if (ocspRespBuf == NULL) {
        printf("Cannot save OCSP response\n");
    }
    else if (wolfIO_DecodeUrl(url, urlSz, domainName, path, &port) < 0) {
        printf("Unable to decode OCSP URL\n");
    }
    else {
        /* Note: This is free'd in OcspRespFreeCb callback */
        int   httpBufSz = HTTP_SCRATCH_BUFFER_SIZE;
        httpBuf         = (byte*)XMALLOC(httpBufSz, NULL, DYNAMIC_TYPE_OCSP);

        printf("OCSP Lookup:\n");
        printf("\tURL: %s\n", url);
        printf("\tDomain: %s\n", domainName);
        printf("\tPath: %s\n", path);
        printf("\tPort: %d\n", port);

        if (httpBuf == NULL) {
            printf("Unable to create OCSP response buffer\n");
        }
        else {
            httpBufSz = wolfIO_HttpBuildRequestOcsp(domainName, path, ocspReqSz,
                                                            httpBuf, httpBufSz);

            /* The socket comes back non-blocking, which the response read
             * below relies on to report OCSP_WANT_READ. */
            ret = connect_responder(domainName, port, io_timeout_sec, &sfd);
            if (ret == 0) {
                if (wolfIO_Send(sfd, (char*)httpBuf, httpBufSz, 0) !=
                                                                    httpBufSz) {
                    printf("OCSP http request failed\n");
                }
                else if (wolfIO_Send(sfd, (char*)ocspReqBuf, ocspReqSz, 0) !=
                                                                    ocspReqSz) {
                    printf("OCSP ocsp request failed\n");
                }
                else {
                    do {
                        ret = wolfIO_HttpProcessResponseOcsp(sfd, ocspRespBuf,
                                        httpBuf, HTTP_SCRATCH_BUFFER_SIZE, NULL);
                        nonBlockCnt++;
                        if (ret == OCSP_WANT_READ)
                            return WOLFSSL_CBIO_ERR_WANT_READ;
                    } while (ret == OCSP_WANT_READ);
                    printf("OCSP Response: ret %d, nonblock count %d\n",
                        ret, nonBlockCnt);
                }
            }
            else {
                printf("OCSP Responder connection failed\n");
            }
            if (sfd != SOCKET_INVALID)
                CloseSocket(sfd);
            XFREE(httpBuf, NULL, DYNAMIC_TYPE_OCSP);
            httpBuf = NULL;
        }
    }
    (void)ctx;
    printf("Resp ret: %d\n", ret);
    return ret;
}

static void OcspRespFreeCb(void* ctx, byte *resp)
{
    if (resp)
        XFREE(resp, NULL, DYNAMIC_TYPE_OCSP);
    httpBuf = NULL;

    (void)ctx;
}

/* Duplicate an X509's DER encoding into a newly allocated buffer sized to the
 * cert. Returns it (caller XFREEs) and sets *derSz, or NULL on error. */
static byte* dup_x509_der(WOLFSSL_X509* x509, int* derSz)
{
    int sz = 0;
    const byte* d = wolfSSL_X509_get_der(x509, &sz);
    byte* copy;

    if (d == NULL) {
        return NULL;
    }
    if (sz <= 0) {
        return NULL;
    }
    copy = (byte*)XMALLOC(sz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (copy == NULL) {
        return NULL;
    }
    XMEMCPY(copy, d, sz);
    *derSz = sz;
    return copy;
}

/* Drive wolfSSL_connect() on a non-blocking socket until it finishes or the
 * deadline passes. wolfIO_TcpConnect() only times out the TCP connect, and
 * per-call socket timeouts can be kept alive by a peer that trickles data, so
 * the whole handshake is held to one absolute deadline instead. Returns
 * WOLFSSL_SUCCESS, or the wolfSSL_connect() result with *timedOut set when the
 * deadline passed. */
static int connect_with_deadline(WOLFSSL* ssl, SOCKET_T fd, int timeoutSec,
                                 int* timedOut)
{
    time_t deadline = time(NULL) + timeoutSec;
    int ret;
    int err;

    *timedOut = 0;
    for (;;) {
        ret = wolfSSL_connect(ssl);
        if (ret == WOLFSSL_SUCCESS) {
            break;
        }
        err = wolfSSL_get_error(ssl, ret);
        if (err != WOLFSSL_ERROR_WANT_READ && err != WOLFSSL_ERROR_WANT_WRITE) {
            break;
        }
        if (wait_for_socket(fd, err == WOLFSSL_ERROR_WANT_WRITE, deadline)
                != 0) {
            *timedOut = 1;
            break;
        }
    }
    return ret;
}

/* Open a TLS connection to host:tlsPort and return the peer's leaf certificate
 * DER in a newly allocated buffer (caller XFREEs), or NULL on error. */
static byte* fetch_leaf_der(const char* host, word16 tlsPort, const char* caDir,
                            int* derSz)
{
    WOLFSSL_CTX* ctx;
    WOLFSSL* ssl;
    WOLFSSL_X509* peer;
    SOCKET_T fd = SOCKET_INVALID;
    byte* der;
    int ret;
    int timedOut;

    ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
    if (ctx == NULL) {
        printf("Unable to create client context\n");
        return NULL;
    }
    /* Verify the peer during this fetch so we get the genuine host cert; an
     * unverified fetch would let an attacker swap in any CA-issued cert. */
    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    if (wolfSSL_CTX_load_verify_locations(ctx, NULL, caDir)
            != WOLFSSL_SUCCESS) {
        printf("Unable to load CA certs from %s\n", caDir);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    if (wolfIO_TcpConnect(&fd, host, tlsPort, io_timeout_sec) != 0) {
        printf("TCP connect to %s failed\n", host);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }
    if (set_nonblocking(fd) != 0) {
        printf("Unable to make socket non-blocking\n");
        CloseSocket(fd);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("Unable to create SSL object\n");
        CloseSocket(fd);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }
    wolfSSL_set_fd(ssl, fd);

    if (wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, host,
            (word16)XSTRLEN(host)) != WOLFSSL_SUCCESS) {
        printf("wolfSSL_UseSNI failed\n");
        wolfSSL_free(ssl);
        CloseSocket(fd);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    /* Bind the cert to the hostname; a chain check alone is not enough. */
    if (wolfSSL_check_domain_name(ssl, host) != WOLFSSL_SUCCESS) {
        printf("wolfSSL_check_domain_name failed\n");
        wolfSSL_free(ssl);
        CloseSocket(fd);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    ret = connect_with_deadline(ssl, fd, io_timeout_sec, &timedOut);
    if (ret != WOLFSSL_SUCCESS) {
        if (timedOut) {
            printf("TLS connect to %s timed out after %d seconds\n", host,
                    io_timeout_sec);
        }
        else {
            printf("TLS connect to %s failed (%d)\n", host,
                    wolfSSL_get_error(ssl, ret));
        }
        wolfSSL_free(ssl);
        CloseSocket(fd);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    peer = wolfSSL_get_peer_certificate(ssl);
    if (peer == NULL) {
        printf("No peer certificate\n");
        wolfSSL_free(ssl);
        CloseSocket(fd);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    der = dup_x509_der(peer, derSz);
    if (der == NULL) {
        printf("Peer certificate DER unavailable\n");
    }

    wolfSSL_shutdown(ssl); /* clean TLS close before teardown */
    wolfSSL_FreeX509(peer);
    wolfSSL_free(ssl);
    CloseSocket(fd);
    wolfSSL_CTX_free(ctx);
    return der;
}

int main(int argc, char** argv)
{
    int ret;
    WOLFSSL_CERT_MANAGER* pCm;
    /* Allocated by fetch_leaf_der to the exact cert size; freed below. */
    byte* der = NULL;
    int derSz = 0;
    const char* caDir = kCACertsDir;

#if 0
    wolfSSL_Debugging_ON();
#endif

    /* Optional argv[1] overrides the CA directory if the chain changes. */
    if (argc > 1) {
        caDir = argv[1];
    }

    ret = wolfSSL_Init();
    if (ret != WOLFSSL_SUCCESS) {
        printf("wolfSSL_Init failed (%d)\n", ret);
        return ret;
    }

    /* Fetch the live leaf certificate so nothing static can expire. */
    der = fetch_leaf_der(kServerHost, kServerPort, caDir, &derSz);
    if (der == NULL) {
        printf("Could not fetch %s certificate\n", kServerHost);
        wolfSSL_Cleanup();
        return -1;
    }
    printf("Fetched %s certificate (%d bytes)\n", kServerHost, derSz);

    /* Create certificate manager context */
    pCm = wolfSSL_CertManagerNew();
    if (pCm == NULL) {
        XFREE(der, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        wolfSSL_Cleanup();
        return MEMORY_E;
    }

#ifdef HAVE_OCSP
    /* Enable OCSP */
    ret = wolfSSL_CertManagerEnableOCSP(pCm, 0);
    if (ret == WOLFSSL_SUCCESS) {
        /* Setup callbacks for OCSP */
        ret = wolfSSL_CertManagerSetOCSP_Cb(pCm,
            OcspLookupNonBlockCb,
            OcspRespFreeCb,
            NULL /* optional context */
        );
    }
#else
    ret = WOLFSSL_SUCCESS;
#endif

    if (ret == WOLFSSL_SUCCESS) {
        /* Load root CAs into Certificate Manager */
        ret = wolfSSL_CertManagerLoadCA(pCm, NULL, caDir);
        printf("Loaded Trusted CA dir %s (ret %d)\n", caDir, ret);
    }

    if (ret == WOLFSSL_SUCCESS) {
        /* Verify the fetched certificate against the loaded CAs */
        ret = wolfSSL_CertManagerVerifyBuffer(pCm, der, derSz,
            WOLFSSL_FILETYPE_ASN1);
        printf("Verify %s cert: %d\n", kServerHost, ret);
    }

#ifdef HAVE_OCSP
    if (ret == WOLFSSL_SUCCESS) {
        /* Check OCSP for the certificate */
        do {
            ret = wolfSSL_CertManagerCheckOCSP(pCm, der, derSz);
        } while (ret == OCSP_WANT_READ);
        printf("Check OCSP for %s (ret %d)\n", kServerHost, ret);
    }
#endif

    wolfSSL_CertManagerFree(pCm);
    XFREE(der, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    wolfSSL_Cleanup();

    printf("Ret = %d: %s\n",
        ret, (ret == WOLFSSL_SUCCESS) ?
            "success" :
            wc_GetErrorString(ret));

    /* Exit 0 on success so scripts and CI can tell pass from fail. */
    if (ret == WOLFSSL_SUCCESS) {
        return 0;
    }
    return ret;
}
