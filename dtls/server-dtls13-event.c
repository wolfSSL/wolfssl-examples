/* server-dtls13-event.c
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
 *
 *=============================================================================
 *
 * Single threaded example of a DTLS 1.3 server for instructional/learning
 * purposes. This example can handle multiple simultaneous connections by using
 * the libevent library to handle the event loop. Please note that this example
 * is not thread safe as access to global objects is not protected.
 *
 * Define USE_DTLS12 to use DTLS 1.2 instead of DTLS 1.3
 */

#include <wolfssl/options.h>
#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

/* Requires libevent */
#include <event2/event.h>

#include "dtls-common.h"

#define QUICK_MULT  4               /* Our quick timeout multiplier */
#define CHGOODCB_E  (-1000)         /* An error outside the range of wolfSSL
                                     * errors */
#define CONN_TIMEOUT 10             /* How long we wait for peer data before
                                     * closing the connection */

typedef struct conn_ctx {
    struct conn_ctx* next;
    WOLFSSL* ssl;
    struct event* readEv;
    struct event* writeEv;
    char waitingOnData:1;
} conn_ctx;

WOLFSSL_CTX*  ctx = NULL;
struct event_base* base = NULL;
WOLFSSL*      pendingSSL = NULL;
int           listenfd = INVALID_SOCKET;   /* Initialize our socket */
conn_ctx* active = NULL;
struct event* newConnEvent = NULL;

static void sig_handler(const int sig);
static void free_resources(void);
static void newConn(evutil_socket_t fd, short events, void* arg);
static void dataReady(evutil_socket_t fd, short events, void* arg);
static int chGoodCb(WOLFSSL* ssl, void*);
static int hsDoneCb(WOLFSSL* ssl, void*);
static int newPendingSSL(void);
static int newFD(void);
static void conn_ctx_free(conn_ctx* connCtx);

int main(int argc, char** argv)
{
    int           exitVal = 1;

    /* Initialize wolfSSL before assigning ctx */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init error.\n");
        return exitVal;
    }

    /* No-op when debugging is not compiled in */
    wolfSSL_Debugging_ON();

    /* Set ctx to DTLS 1.3 */
    if ((ctx = wolfSSL_CTX_new(
#ifndef USE_DTLS12
            wolfDTLSv1_3_server_method()
#else
            wolfDTLSv1_2_server_method()
#endif
            )) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        goto cleanup;
    }
    /* Load CA certificates */
    if (wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0) !=
            SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", caCertLoc);
        goto cleanup;
    }
    /* Load server certificates */
    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != 
                                                                 SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", servCertLoc);
        goto cleanup;
    }
    /* Load server Keys */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc,
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", servKeyLoc);
        goto cleanup;
    }

    listenfd = newFD();
    if (listenfd == INVALID_SOCKET)
        goto cleanup;

    if (!newPendingSSL())
        goto cleanup;

    signal(SIGINT, sig_handler);

    base = event_base_new();
    if (base == NULL) {
        perror("event_base_new failed");
        exit(EXIT_FAILURE);
    }

    newConnEvent = event_new(base, listenfd, EV_READ|EV_PERSIST, newConn, NULL);
    if (newConnEvent == NULL) {
        fprintf(stderr, "event_new failed for srvEvent\n");
        goto cleanup;
    }
    if (event_add(newConnEvent, NULL) != 0) {
        fprintf(stderr, "event_add failed\n");
        goto cleanup;
    }

    printf("running event loop\n");

    if (event_base_dispatch(base) == -1) {
        fprintf(stderr, "event_base_dispatch failed\n");
        goto cleanup;
    }

    printf("done with dispatching\n");


    exitVal = 0;
cleanup:
    free_resources();
    wolfSSL_Cleanup();

    return exitVal;
}

static int newFD(void)
{
    int fd;
    int on = 1;
    struct sockaddr_in servAddr;        /* our server's address */

    /* Create a UDP/IP socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket()");
        return INVALID_SOCKET;
    }
    memset((char *)&servAddr, 0, sizeof(servAddr));
    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(SERV_PORT);

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) != 0) {
        perror("setsockopt() with SO_REUSEADDR");
        goto cleanup;
    }
#ifdef SO_REUSEPORT
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char*)&on, sizeof(on)) != 0) {
        perror("setsockopt() with SO_REUSEPORT");
        goto cleanup;
    }
#endif
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl");
        goto cleanup;
    }

    /* Bind Socket */
    if (bind(fd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind()");
        goto cleanup;
    }
    return fd;
cleanup:
    if (fd != INVALID_SOCKET) {
        close(fd);
        fd = INVALID_SOCKET;
    }
    return INVALID_SOCKET;
}

static int newPendingSSL(void)
{
    WOLFSSL* ssl;

    /* Create the pending WOLFSSL Object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "wolfSSL_new error.\n");
        return 0;
    }

    wolfSSL_dtls_set_using_nonblock(ssl, 1);

    if (wolfDTLS_SetChGoodCb(ssl, chGoodCb, NULL) != WOLFSSL_SUCCESS ) {
        fprintf(stderr, "wolfDTLS_SetChGoodCb error.\n");
        wolfSSL_free(ssl);
        return 0;
    }

    if (wolfSSL_SetHsDoneCb(ssl, hsDoneCb, NULL) != WOLFSSL_SUCCESS ) {
        fprintf(stderr, "wolfSSL_SetHsDoneCb error.\n");
        wolfSSL_free(ssl);
        return 0;
    }

    if (wolfSSL_set_fd(ssl, listenfd) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_set_fd error.\n");
        wolfSSL_free(ssl);
        return 0;
    }

#if !defined(USE_DTLS12) && defined(WOLFSSL_SEND_HRR_COOKIE)
    {
        /* Applications should update this secret periodically */
        char *secret = "My secret";
        if (wolfSSL_send_hrr_cookie(ssl, (byte*)secret, strlen(secret))
                != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_send_hrr_cookie error.\n");
            wolfSSL_free(ssl);
            return 0;
        }
    }
#endif

    pendingSSL = ssl;

    return 1;
}

static void newConn(evutil_socket_t fd, short events, void* arg)
{
    int                ret;
    int                err;
    /* Store pointer because pendingSSL can be modified in chGoodCb */
    WOLFSSL*           ssl = pendingSSL;

    (void)events;
    (void)arg;

    ret = wolfSSL_accept(ssl);
    if (ret != WOLFSSL_SUCCESS) {
        err = wolfSSL_get_error(ssl, 0);
        if (err != WOLFSSL_ERROR_WANT_READ) {
            fprintf(stderr, "error = %d, %s\n", err,
                    wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "SSL_accept failed.\n");
            free_resources();
            wolfSSL_Cleanup();
            exit(1);
        }
    }
}

static void setHsTimeout(WOLFSSL* ssl, struct timeval *tv)
{
    int timeout = wolfSSL_dtls_get_current_timeout(ssl);
#ifndef USE_DTLS12
    if (wolfSSL_dtls13_use_quick_timeout(ssl)) {
        if (timeout >= QUICK_MULT)
            tv->tv_sec = timeout / QUICK_MULT;
        else
            tv->tv_usec = timeout * 1000000 / QUICK_MULT;
    }
    else
#endif
        tv->tv_sec = timeout;
}

/* Called when we have verified a connection */
static int chGoodCb(WOLFSSL* ssl, void* arg)
{
    int fd = INVALID_SOCKET;
    struct sockaddr_in cliaddr;         /* the client's address */
    socklen_t          cliLen = sizeof(cliaddr);
    conn_ctx* connCtx = (conn_ctx*)calloc(1, sizeof(conn_ctx));
    struct timeval tv;

    (void)arg;

    if (connCtx == NULL) {
        fprintf(stderr, "Out of memory!\n");
        goto error;
    }

    /* Push to active connection stack */
    connCtx->next = active;
    active = connCtx;

    if (wolfSSL_dtls_get_peer(ssl, &cliaddr, &cliLen) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_dtls_get_peer failed\n");
        goto error;
    }

    /* We need to change the SFD here so that the ssl object doesn't drop any
     * new connections */
    fd = newFD();
    if (fd == INVALID_SOCKET)
        goto error;

    /* Limit new SFD to only this connection */
    if (connect(fd, (const struct sockaddr*)&cliaddr, cliLen) != 0) {
        perror("connect()");
        goto error;
    }

    if (wolfSSL_set_dtls_fd_connected(ssl, fd) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_set_dtls_fd_connected error.\n");
        goto error;
    }

    connCtx->writeEv = event_new(base, fd, EV_WRITE, dataReady, connCtx);
    if (connCtx->writeEv == NULL) {
        fprintf(stderr, "event_new failed for srvEvent\n");
        goto error;
    }
    connCtx->readEv = event_new(base, fd, EV_READ, dataReady, connCtx);
    if (connCtx->readEv == NULL) {
        fprintf(stderr, "event_new failed for srvEvent\n");
        goto error;
    }
    memset(&tv, 0, sizeof(tv));
    setHsTimeout(ssl, &tv);
    /* We are using non-blocking sockets so we will definitely be waiting for
     * the peer. Start the timer now. */
    if (event_add(connCtx->readEv, &tv) != 0) {
        fprintf(stderr, "event_add failed\n");
        goto error;
    }

    /* Promote the pending connection to an active connection */
    if (!newPendingSSL())
        goto error;
    connCtx->ssl = ssl;

    return 0;
error:
    if (fd != INVALID_SOCKET) {
        close(fd);
        fd = INVALID_SOCKET;
    }
    if (connCtx != NULL) {
        connCtx->ssl = NULL;
        conn_ctx_free(connCtx);
    }
    (void)wolfSSL_set_fd(ssl, INVALID_SOCKET);
    return CHGOODCB_E;
}

static int hsDoneCb(WOLFSSL* ssl, void* arg)
{
    showConnInfo(ssl);
    (void)arg;
    return 0;
}

static void dataReady(evutil_socket_t fd, short events, void* arg)
{
    conn_ctx* connCtx = (conn_ctx*)arg;
    int ret;
    int err;
    struct timeval tv;
    char msg[MAXLINE];
    int msgSz;
    const char* ack = "I hear you fashizzle!\n";

    memset(&tv, 0, sizeof(tv));
    if (events & EV_TIMEOUT) {
        /* A timeout occurred */
        if (!wolfSSL_is_init_finished(connCtx->ssl)) {
            if (wolfSSL_dtls_got_timeout(connCtx->ssl) != WOLFSSL_SUCCESS) {
                fprintf(stderr, "wolfSSL_dtls_got_timeout failed\n");
                goto error;
            }
            setHsTimeout(connCtx->ssl, &tv);
            if (event_add(connCtx->readEv, &tv) != 0) {
                fprintf(stderr, "event_add failed\n");
                goto error;
            }
        }
        else {
            if (connCtx->waitingOnData) {
                /* Too long waiting for peer data. Shutdown the connection.
                 * Don't wait for a response from the peer. */
                printf("Closing connection after timeout\n");
                (void)wolfSSL_shutdown(connCtx->ssl);
                goto error;
            }
            else {
                tv.tv_sec = CONN_TIMEOUT;
                connCtx->waitingOnData = 1;
                if (event_add(connCtx->readEv, &tv) != 0) {
                    fprintf(stderr, "event_add failed\n");
                    goto error;
                }
            }
        }
    }
    else if (events & (EV_READ|EV_WRITE)) {
        ret = wolfSSL_read(connCtx->ssl, msg, sizeof(msg) - 1);
        if (ret > 0) {
            msgSz = ret;
            msg[msgSz] = '\0';
            printf("Received message: %s\n", msg);
            ret = wolfSSL_write(connCtx->ssl, ack, strlen(ack));
        }

        if (ret <= 0) {
            err = wolfSSL_get_error(connCtx->ssl, 0);
            if (err == WOLFSSL_ERROR_WANT_READ ||
                    err == WOLFSSL_ERROR_WANT_WRITE) {
                setHsTimeout(connCtx->ssl, &tv);
                if (event_add(err == WOLFSSL_ERROR_WANT_READ ?
                        connCtx->readEv : connCtx->writeEv, &tv) != 0) {
                    fprintf(stderr, "event_add failed\n");
                    goto error;
                }
            }
            else if (err == WOLFSSL_ERROR_ZERO_RETURN) {
                /* Peer closed connection. Let's do the same. */
                printf("peer closed connection\n");
                ret = wolfSSL_shutdown(connCtx->ssl);
                if (ret != WOLFSSL_SUCCESS) {
                    fprintf(stderr, "wolfSSL_shutdown failed (%d)\n", ret);
                }
                goto error;
            }
            else {
                fprintf(stderr, "error = %d, %s\n", err,
                        wolfSSL_ERR_reason_error_string(err));
                fprintf(stderr, "wolfSSL_read or wolfSSL_write failed\n");
                goto error;
            }
        }
        else {
            tv.tv_sec = CONN_TIMEOUT;
            connCtx->waitingOnData = 1;
            if (event_add(connCtx->readEv, &tv) != 0) {
                fprintf(stderr, "event_add failed\n");
                goto error;
            }
        }
    }
    else {
        fprintf(stderr, "Unexpected events %d\n", events);
        goto error;
    }


    return;
error:
    /* Free the connection */
    conn_ctx_free(connCtx);
    close(fd);
}

static void conn_ctx_free(conn_ctx* connCtx)
{
    if (connCtx != NULL) {
        /* Remove from active stack */
        if (active != NULL) {
            conn_ctx** prev = &active;
            while (*prev != NULL) {
                if (*prev == connCtx) {
                    *prev = connCtx->next;
                    break;
                }
                prev = &(*prev)->next;
            }
        }
        if (connCtx->ssl != NULL)
            wolfSSL_free(connCtx->ssl);
        if (connCtx->readEv != NULL) {
            (void)event_del(connCtx->readEv);
            event_free(connCtx->readEv);
        }
        if (connCtx->writeEv != NULL) {
            (void)event_del(connCtx->writeEv);
            event_free(connCtx->writeEv);
        }
        free(connCtx);
    }
}

static void sig_handler(const int sig)
{
    printf("Received signal %d. Cleaning up.\n", sig);
    free_resources();
    wolfSSL_Cleanup();
    exit(0);
}

static void free_resources(void)
{
    conn_ctx* connCtx = active;
    while (connCtx != NULL) {
        active = active->next;
        conn_ctx_free(connCtx);
        connCtx = active;
    }
    if (pendingSSL != NULL) {
        wolfSSL_shutdown(pendingSSL);
        wolfSSL_free(pendingSSL);
        pendingSSL = NULL;
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
        ctx = NULL;
    }
    if (listenfd != INVALID_SOCKET) {
        close(listenfd);
        listenfd = INVALID_SOCKET;
    }
    if (newConnEvent != NULL) {
        (void)event_del(newConnEvent);
        event_free(newConnEvent);
        newConnEvent = NULL;
    }
    if (base != NULL) {
        event_base_free(base);
        base = NULL;
    }
}
