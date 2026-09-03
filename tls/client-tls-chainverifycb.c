/* client-tls-chainverifycb.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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
 */

/* A TLS client whose root of trust lives outside wolfSSL.
 *
 * No CA is loaded into the WOLFSSL_CTX. A chain verify callback hands the
 * server's certificates, as raw DER, to a "trust service" that runs on its
 * own thread and owns the trust anchors. It stands in for an HSM or secure
 * element that holds the root of trust and does the chain verification. The
 * callback defers with CHAIN_VERIFY_WANT_E while the service works, and the
 * handshake resumes when wolfSSL_connect() is called again with a verdict
 * available.
 *
 * Requires wolfSSL built with --enable-chain-verify-cb.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>

#define DEFAULT_HOST     "127.0.0.1"
#define DEFAULT_PORT     11111
#define DEFAULT_ANCHOR   "../certs/ca-cert.pem"
#define MAX_CHAIN        8
#define SERVICE_DELAY_MS 300   /* pretend the anchors are slow to reach */
#define POLL_MS          100   /* how often the handshake is re-entered */
#define MAXDATASIZE      4096

#ifdef WOLFSSL_CHAIN_VERIFY_CB

/* The root of trust. The SSL object never sees these anchors; the service
 * verifies chains against them on its own thread. One job at a time is
 * enough for one connection. */
typedef struct TrustService {
    pthread_t       thread;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    unsigned char*  anchor;    /* PEM trust anchor(s) */
    long            anchorSz;
    int             rejectAll; /* demo switch: refuse every chain */
    int             stop;

    enum { JOB_NONE, JOB_PENDING, JOB_DONE } state;
    unsigned char*  der[MAX_CHAIN];
    unsigned int    derSz[MAX_CHAIN];
    int             count;
    int             verdict;   /* 0 accepted, otherwise a wolfSSL error */
} TrustService;

static void job_clear(TrustService* svc)
{
    int i;
    for (i = 0; i < svc->count; i++) {
        free(svc->der[i]);
        svc->der[i] = NULL;
    }
    svc->count = 0;
    svc->state = JOB_NONE;
}

/* Verify the chain the way an HSM holding the anchors would: walk from the
 * certificate nearest the anchors down to the server's own, each one checked
 * against what is trusted so far and then trusted for the next step. */
static int service_verify(const TrustService* svc)
{
    WOLFSSL_CERT_MANAGER* cm;
    int i;
    int ret;

    cm = wolfSSL_CertManagerNew();
    if (cm == NULL)
        return MEMORY_E;

    ret = wolfSSL_CertManagerLoadCABuffer(cm, svc->anchor, svc->anchorSz,
        WOLFSSL_FILETYPE_PEM);
    for (i = svc->count - 1; (ret == WOLFSSL_SUCCESS) && (i >= 1); i--) {
        ret = wolfSSL_CertManagerVerifyBuffer(cm, svc->der[i], svc->derSz[i],
            WOLFSSL_FILETYPE_ASN1);
        if (ret == WOLFSSL_SUCCESS) {
            ret = wolfSSL_CertManagerLoadCABuffer(cm, svc->der[i],
                svc->derSz[i], WOLFSSL_FILETYPE_ASN1);
        }
    }
    if (ret == WOLFSSL_SUCCESS) {
        ret = wolfSSL_CertManagerVerifyBuffer(cm, svc->der[0], svc->derSz[0],
            WOLFSSL_FILETYPE_ASN1);
    }
    wolfSSL_CertManagerFree(cm);

    return (ret == WOLFSSL_SUCCESS) ? 0 : ret;
}

static void* service_thread(void* arg)
{
    TrustService* svc = (TrustService*)arg;
    int verdict;

    pthread_mutex_lock(&svc->lock);
    while (!svc->stop) {
        if (svc->state != JOB_PENDING) {
            pthread_cond_wait(&svc->cond, &svc->lock);
            continue;
        }
        pthread_mutex_unlock(&svc->lock);

        usleep(SERVICE_DELAY_MS * 1000);
        verdict = svc->rejectAll ? ASN_NO_SIGNER_E : service_verify(svc);

        pthread_mutex_lock(&svc->lock);
        svc->verdict = verdict;
        svc->state = JOB_DONE;
        pthread_cond_broadcast(&svc->cond);
    }
    pthread_mutex_unlock(&svc->lock);
    return NULL;
}

static int service_start(TrustService* svc, const char* anchorFile,
    int rejectAll)
{
    FILE* f;
    long sz;

    memset(svc, 0, sizeof(*svc));
    svc->rejectAll = rejectAll;

    f = fopen(anchorFile, "rb");
    if (f == NULL) {
        fprintf(stderr, "trust service: cannot open %s: %s\n", anchorFile,
            strerror(errno));
        return -1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    svc->anchor = (unsigned char*)malloc((size_t)sz);
    if ((sz <= 0) || (svc->anchor == NULL) ||
            (fread(svc->anchor, 1, (size_t)sz, f) != (size_t)sz)) {
        fprintf(stderr, "trust service: cannot read %s\n", anchorFile);
        fclose(f);
        return -1;
    }
    fclose(f);
    svc->anchorSz = sz;

    pthread_mutex_init(&svc->lock, NULL);
    pthread_cond_init(&svc->cond, NULL);
    if (pthread_create(&svc->thread, NULL, service_thread, svc) != 0) {
        fprintf(stderr, "trust service: cannot start thread\n");
        return -1;
    }
    printf("trust service: anchors loaded from %s, running on its own thread\n",
        anchorFile);
    return 0;
}

static void service_stop(TrustService* svc)
{
    pthread_mutex_lock(&svc->lock);
    svc->stop = 1;
    pthread_cond_broadcast(&svc->cond);
    pthread_mutex_unlock(&svc->lock);
    pthread_join(svc->thread, NULL);
    job_clear(svc);
    free(svc->anchor);
    pthread_cond_destroy(&svc->cond);
    pthread_mutex_destroy(&svc->lock);
}

/* Give the service a little time to answer. A real application would go
 * back to its event loop instead and re-enter the handshake when told to. */
static void service_poll(TrustService* svc)
{
    struct timespec until;

    clock_gettime(CLOCK_REALTIME, &until);
    until.tv_nsec += POLL_MS * 1000000L;
    if (until.tv_nsec >= 1000000000L) {
        until.tv_sec++;
        until.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&svc->lock);
    if (svc->state == JOB_PENDING)
        pthread_cond_timedwait(&svc->cond, &svc->lock, &until);
    pthread_mutex_unlock(&svc->lock);
}

/* The chain verify callback. wolfSSL has already decoded the certificates;
 * it has built no chain and verified nothing. certs[0] is the server's own
 * certificate, the rest is the chain it sent. The buffers are only valid for
 * the duration of the call, so they are copied before the service gets them.
 */
static int chain_verify_cb(WOLFSSL* ssl, const WOLFSSL_BUFFER_INFO* certs,
    int certsSz, void* ctx)
{
    TrustService* svc = (TrustService*)ctx;
    int ret = 0;
    int i;

    (void)ssl;

    pthread_mutex_lock(&svc->lock);
    switch (svc->state) {
        case JOB_NONE:
            if (certsSz > MAX_CHAIN) {
                ret = MAX_CHAIN_ERROR;
                break;
            }
            for (i = 0; i < certsSz; i++) {
                svc->der[i] = (unsigned char*)malloc(certs[i].length);
                if (svc->der[i] == NULL) {
                    ret = MEMORY_E;
                    break;
                }
                memcpy(svc->der[i], certs[i].buffer, certs[i].length);
                svc->derSz[i] = certs[i].length;
                svc->count = i + 1;
            }
            if (ret != 0) {
                job_clear(svc);
                break;
            }
            svc->state = JOB_PENDING;
            pthread_cond_broadcast(&svc->cond);
            printf("callback: %d certificate(s) handed to the trust service, "
                   "deferring\n", certsSz);
            ret = CHAIN_VERIFY_WANT_E;
            break;

        case JOB_PENDING:
            printf("callback: no verdict yet, deferring again\n");
            ret = CHAIN_VERIFY_WANT_E;
            break;

        case JOB_DONE:
            ret = svc->verdict;
            if (ret == 0) {
                printf("callback: trust service accepted the chain\n");
            }
            else {
                printf("callback: trust service rejected the chain: %s\n",
                    wolfSSL_ERR_reason_error_string((unsigned long)ret));
            }
            job_clear(svc);
            break;
    }
    pthread_mutex_unlock(&svc->lock);

    return ret;
}

static int wait_socket(int sock, int forRead)
{
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    return select(sock + 1, forRead ? &fds : NULL, forRead ? NULL : &fds,
        NULL, &tv);
}

static int tcp_connect(const char* host, int port)
{
    struct sockaddr_in addr;
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "socket: %s\n", strerror(errno));
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        fprintf(stderr, "invalid address %s\n", host);
        close(sock);
        return -1;
    }
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "connect to %s:%d: %s\n", host, port, strerror(errno));
        close(sock);
        return -1;
    }
    /* Non-blocking from here on, so the handshake returns to us whenever it
     * waits for the network or for the trust service. */
    fcntl(sock, F_SETFL, O_NONBLOCK);
    return sock;
}

static void usage(void)
{
    printf("client-tls-chainverifycb [-n] [-x] [-a anchor.pem] [host] [port]\n"
           "  -n  install no callback: with no CA loaded the handshake must "
           "fail\n"
           "  -x  make the trust service reject every chain\n"
           "  -a  trust anchor file for the service (default %s)\n",
           DEFAULT_ANCHOR);
}

int main(int argc, char** argv)
{
    TrustService svc;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    const char* host = DEFAULT_HOST;
    const char* anchorFile = DEFAULT_ANCHOR;
    int port = DEFAULT_PORT;
    int noCallback = 0;
    int rejectAll = 0;
    int hostSet = 0;
    int sock = -1;
    int ret = 0;
    int err = 0;
    int exitCode = EXIT_FAILURE;
    int i;
    char buf[MAXDATASIZE];
    const char msg[] = "Hello from client-tls-chainverifycb";

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0)
            noCallback = 1;
        else if (strcmp(argv[i], "-x") == 0)
            rejectAll = 1;
        else if ((strcmp(argv[i], "-a") == 0) && (i + 1 < argc))
            anchorFile = argv[++i];
        else if (strcmp(argv[i], "-h") == 0) {
            usage();
            return EXIT_SUCCESS;
        }
        else if (argv[i][0] == '-') {
            usage();
            return EXIT_FAILURE;
        }
        else if (!hostSet) {
            host = argv[i];
            hostSet = 1;
        }
        else
            port = atoi(argv[i]);
    }

    /* A server that gives up on the handshake may close the socket while we
     * are still writing; report that as an error instead of dying. */
    signal(SIGPIPE, SIG_IGN);

    if (service_start(&svc, anchorFile, rejectAll) != 0)
        return EXIT_FAILURE;

    wolfSSL_Init();

    ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
    if (ctx == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new failed\n");
        goto cleanup;
    }

    /* Deliberately no wolfSSL_CTX_load_verify_locations(): the SSL object
     * holds no trust anchors at all. */
    if (!noCallback) {
        ret = wolfSSL_CTX_SetChainVerifyCb(ctx, chain_verify_cb);
        if (ret != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_CTX_SetChainVerifyCb failed: %s\n",
                wolfSSL_ERR_reason_error_string((unsigned long)ret));
            goto cleanup;
        }
        printf("client: no CA loaded, chain verify callback installed\n");
    }
    else {
        printf("client: no CA loaded, no callback: expecting the handshake "
               "to fail\n");
    }

    sock = tcp_connect(host, port);
    if (sock < 0)
        goto cleanup;

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        fprintf(stderr, "wolfSSL_new failed\n");
        goto cleanup;
    }
    wolfSSL_set_fd(ssl, sock);
    wolfSSL_SetChainVerifyCtx(ssl, &svc);

    /* Drive the handshake. Three things make wolfSSL_connect() return early:
     * waiting for the socket, and the callback waiting for the service. */
    for (;;) {
        ret = wolfSSL_connect(ssl);
        if (ret == WOLFSSL_SUCCESS)
            break;
        err = wolfSSL_get_error(ssl, ret);
        if (err == WOLFSSL_ERROR_WANT_READ)
            wait_socket(sock, 1);
        else if (err == WOLFSSL_ERROR_WANT_WRITE)
            wait_socket(sock, 0);
        else if (err == CHAIN_VERIFY_WANT_E)
            service_poll(&svc);
        else
            break;
    }
    if (ret != WOLFSSL_SUCCESS) {
        printf("client: handshake failed: %d (%s)\n", err,
            wolfSSL_ERR_reason_error_string((unsigned long)err));
        if (noCallback && (err == ASN_NO_SIGNER_E))
            exitCode = EXIT_SUCCESS; /* the expected outcome of -n */
        if (rejectAll && (err == CHAIN_VERIFY_CB_E))
            exitCode = EXIT_SUCCESS; /* the expected outcome of -x */
        goto cleanup;
    }
    printf("client: handshake done, %s\n", wolfSSL_get_version(ssl));

    do {
        ret = wolfSSL_write(ssl, msg, (int)strlen(msg));
        err = wolfSSL_get_error(ssl, ret);
    } while ((ret <= 0) && (err == WOLFSSL_ERROR_WANT_WRITE));
    if (ret <= 0) {
        fprintf(stderr, "wolfSSL_write failed: %d\n", err);
        goto cleanup;
    }

    do {
        ret = wolfSSL_read(ssl, buf, sizeof(buf) - 1);
        err = wolfSSL_get_error(ssl, ret);
        if ((ret <= 0) && (err == WOLFSSL_ERROR_WANT_READ))
            wait_socket(sock, 1);
    } while ((ret <= 0) && (err == WOLFSSL_ERROR_WANT_READ));
    if (ret <= 0) {
        fprintf(stderr, "wolfSSL_read failed: %d\n", err);
        goto cleanup;
    }
    buf[ret] = '\0';
    printf("server: %s\n", buf);
    exitCode = EXIT_SUCCESS;

cleanup:
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (sock >= 0)
        close(sock);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    service_stop(&svc);
    return exitCode;
}

#else

int main(void)
{
    fprintf(stderr, "wolfSSL was built without --enable-chain-verify-cb\n");
    return EXIT_FAILURE;
}

#endif /* WOLFSSL_CHAIN_VERIFY_CB */
