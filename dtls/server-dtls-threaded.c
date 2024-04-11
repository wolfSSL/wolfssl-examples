/* server-dtls-threaded.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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
 * A simple dtls server example with configurable threadpool, for
 * instructional/learning purposes. Utilizes DTLS 1.2.  Please note that if
 * multiple client hellos arrive at the same time, the server might drop some of
 * them. A production-ready server needs a more sophisticated mechanism to
 * multiplex packets from different clients to the same port.
 */

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
/* Uncomment if you want to build with the less portable
 * non-blocking pthread_tryjoin_np.*/
/* #define USE_NONBLOCK_JOIN */
#ifdef USE_NONBLOCK_JOIN
    #define _GNU_SOURCE
#endif
#include <pthread.h>

#include "dtls-common.h"

#define MSGLEN          4096
#define DTLS_NUMTHREADS 32

typedef struct {
    WOLFSSL * ssl;
    int       activefd;
    int       peer_port;
    int       done;
} thread_args_t;

static WOLFSSL_CTX * ctx = NULL;
static volatile int  stop_server = 0;

static int    new_udp_listen_socket(void);
static void   safer_shutdown(thread_args_t * args);
static void * server_work(void * thread_args);
static void   sig_handler(const int sig);
static void   cleanup_threadpool(pthread_t * threads, thread_args_t * args,
                                 int n_threads);

int
main(int   argc,
     char* argv[])
{
    char               caCertLoc[] =   "../certs/ca-cert.pem";
    char               servCertLoc[] = "../certs/server-cert.pem";
    char               servKeyLoc[] =  "../certs/server-key.pem";
    int                ret = 0;
    /* Variables for awaiting datagram */
    int                listenfd = 0;   /* Initialize our socket */
    struct sockaddr_in cliaddr;         /* the client's address */
    socklen_t          cliLen = sizeof(cliaddr);
    /* variables needed for threading */
    int                n_threads = 2;
    pthread_t          threads[DTLS_NUMTHREADS];
    thread_args_t      args[DTLS_NUMTHREADS];
    int                opt = 0;

    memset(threads, 0, sizeof(threads));
    memset(args, 0, sizeof(args));

    while ((opt = getopt(argc, argv, "t:?")) != -1) {
        switch (opt) {
        case 't':
            n_threads = atoi(optarg);
            break;

        case '?':
            printf("usage:\n");
            printf("  ./server-dtls-threaded [-t n]\n");
            printf("\n");
            printf("description:\n");
            printf("  A simple dtls server with configurable threadpool.\n");
            printf("  Num allowed threads is: 1 <= n <= %d\n",
                    DTLS_NUMTHREADS);
        default:
            return EXIT_FAILURE;
        }
    }

    if (n_threads <= 0 || n_threads > DTLS_NUMTHREADS) {
        printf("error: invalid n_threads: %d\n", n_threads);
        return EXIT_FAILURE;
    }

    /* Code for handling signals */
    struct sigaction act, oact;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);

    /* Uncomment if you want debugging. */
    /* wolfSSL_Debugging_ON(); */

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Set ctx to DTLS 1.2 */
    ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method());
    if (ctx == NULL) {
        printf("error: wolfSSL_CTX_new error.\n");
        return EXIT_FAILURE;
    }

    /* Load CA certificates */
    ret = wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0);
    if (ret != SSL_SUCCESS) {
        printf("error: error loading %s, please check the file.\n", caCertLoc);
        return EXIT_FAILURE;
    }

    /* Load server certificates */
    ret = wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM);
    if (ret != SSL_SUCCESS) {
        printf("error: error loading %s, please check the file.\n", servCertLoc);
        return EXIT_FAILURE;
    }

    /* Load server Keys */
    ret = wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, SSL_FILETYPE_PEM);

    if (ret != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return EXIT_FAILURE;
    }

    /* Create a UDP/IP socket */
    listenfd = new_udp_listen_socket();

    if (listenfd <= 0 ) {
        printf("error: cannot create socket: %d\n", listenfd);
        return EXIT_FAILURE;
    }

    printf("info: awaiting client dtls on port %d\n", SERV_PORT);

    while (stop_server != 1) {
        for (size_t i = 0; i < n_threads; ++i) {
            if (stop_server) {
                break;
            }

            if (threads[i] != 0) {
                /* Skip threads already spawned. */
                continue;
            }

            ret = recvfrom(listenfd, NULL, 0, MSG_PEEK,
                           (struct sockaddr *)&cliaddr, &cliLen);
            if (ret < 0)
                continue;

            printf("Received a packet from %s:%d\n",
                   inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

            memset(&args[i], 0, sizeof(thread_args_t));
            args[i].activefd = listenfd;
            listenfd = new_udp_listen_socket();
            /* Avoid future packets from other peers to be received over
             * args[i].activefd. Please note that packets from other clients
             * already received might be returned from future invocations of
             * recvfrom()/read(). The args[i].ssl object will discard those packets
             * that don't match the set DTLS peer. */
            ret = connect(args[i].activefd, (const struct sockaddr *)&cliaddr, cliLen);
            if (ret != 0) {
                printf("error: connect returned: %d\n", ret);
                break;
            }

            args[i].ssl = wolfSSL_new(ctx);
            if (args[i].ssl == NULL) {
                printf("error: wolfSSL_new returned null\n");
                break;
            }

            ret = wolfSSL_set_fd(args[i].ssl, args[i].activefd);
            if (ret != SSL_SUCCESS) {
                printf("error: wolfSSL_set_fd: %d\n", ret);
                break;
            }

            ret = wolfSSL_dtls_set_peer(args[i].ssl, &cliaddr, cliLen);
            if (ret != WOLFSSL_SUCCESS) {
                printf("error: wolfSSL_dtls_set_peer: %d\n", ret);
                break;
            }

            args[i].peer_port = ntohs(cliaddr.sin_port);
            ret = pthread_create(&threads[i], NULL, server_work, &args[i]);

            if (ret == 0 ) {
                printf("info: spawned thread: %ld\n", (long)threads[i]);
            }
            else {
                printf("error: pthread_create returned %d\n", ret);
                threads[i] = 0;
            }
        }

        cleanup_threadpool(threads, args, n_threads);
    }

    /* Do a final blocking join. */
    for (size_t i = 0; i < n_threads; ++i) {
        if (threads[i]) {
            pthread_join(threads[i], NULL);
            printf("info: joined thread: %ld\n", (long)threads[i]);
            threads[i] = 0;
        }
    }

    /* All threads exited. Do a final cleanup pass just in case. */
    for (size_t i = 0; i < n_threads; ++i) {
        safer_shutdown(&args[i]);
    }

    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return EXIT_SUCCESS;
}

static int
new_udp_listen_socket(void)
{
    struct sockaddr_in listen_addr;        /* our server's address */
    int                sockfd = 0;
    int                ret = 0;
    int                on = 1;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd <= 0) {
        int errsave = errno;
        printf("error: socket returned %d\n", errsave);
        return -1;
    }

    memset(&listen_addr, 0, sizeof(listen_addr));

    listen_addr.sin_family      = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port        = htons(SERV_PORT);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) != 0) {
        printf("error: setsockopt() with SO_REUSEADDR");
        close(sockfd);
        sockfd = 0;
        return -1;
    }
#ifdef SO_REUSEPORT
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&on, sizeof(on)) != 0) {
        printf("error: setsockopt() with SO_REUSEPORT");
        close(sockfd);
        sockfd = 0;
        return -1;
    }
#endif

    ret = bind(sockfd, (const struct sockaddr *)&listen_addr,
               sizeof(listen_addr));

    if (ret != 0) {
        int errsave = errno;
        printf("error: bind returned %d\n", errsave);
        close(sockfd);
        sockfd = 0;
        return -1;
    }

    printf("info: opened socket: %d\n", sockfd);

    return sockfd;
}

static void *
server_work(void * args)
{
    thread_args_t * thread_args = (thread_args_t *) args;
    int             n_bytes = 0;
    char            recv_msg[MSGLEN];
    char            send_msg[MSGLEN];
    int ret;

    ret = wolfSSL_accept(thread_args->ssl);
    if (ret != SSL_SUCCESS)
    {
        printf("error: wolfSSL_accept returned %d\n", ret);
        pthread_exit(NULL);
        /* we should never reach here */
        return NULL;
    }

    printf("info: new dtls session: %p, %d\n", (void *)thread_args->ssl,
           thread_args->peer_port);

    for (size_t i = 0; i < 4; ++i) {
        if (stop_server) {
            break;
        }

        sprintf(send_msg, "msg %zu from server thread %ld\n", i,
                (long)pthread_self());

        n_bytes = wolfSSL_read(thread_args->ssl, recv_msg, sizeof(recv_msg) - 1);

        if (n_bytes > 0) {
            recv_msg[n_bytes] = 0;
            printf("%s", recv_msg);
        }
        else {
            printf("error: wolfSSL_read returned: %d\n", n_bytes);

            int readErr = wolfSSL_get_error(thread_args->ssl, 0);
            if(readErr != SSL_ERROR_WANT_READ) {
                printf("SSL_read failed: %d\n", readErr);
                break;
            }
        }

        n_bytes = wolfSSL_write(thread_args->ssl, send_msg, strlen(send_msg));

        if (n_bytes > 0) {
            if (n_bytes != strlen(send_msg)) {
                printf("error: sent %d, expected %zu bytes\n", n_bytes,
                       strlen(send_msg));
            }
        }
        else {
            printf("error: wolfSSL_write returned: %d\n", n_bytes);

            int readErr = wolfSSL_get_error(thread_args->ssl, 0);
            if(readErr != SSL_ERROR_WANT_WRITE) {
                printf("SSL_write failed: %d\n", readErr);
            }

            break;
        }
    }

    safer_shutdown(thread_args);
    printf("info: exiting thread %ld\n", (long)pthread_self());
    pthread_exit(NULL);
}

/* Small shutdown wrapper to safely clean up a thread's
 * connection. */
static void
safer_shutdown(thread_args_t * args)
{
    int ret;

    if (args == NULL) {
        printf("error: safer_shutdown with null args\n");
        return;
    }

    if (args->ssl != NULL) {
        printf("info: closed dtls session: %p\n", (void*) args->ssl);
        ret = wolfSSL_shutdown(args->ssl);

        /* bidirectional shutdown */
        if (ret != WOLFSSL_SUCCESS)
            ret = wolfSSL_shutdown(args->ssl);

        wolfSSL_free(args->ssl);
        args->ssl = NULL;
    }

    if (args->activefd > 0) {
        printf("info: closed socket: %d\n", args->activefd);
        close(args->activefd);
        args->activefd = 0;
    }

    args->done = 1;

    return;
}

static void
sig_handler(const int sig)
{
    printf("info: SIGINT %d handled\n", sig);
    stop_server = 1;
    return;
}

static void
cleanup_threadpool(pthread_t *     threads,
                   thread_args_t * args,
                   int             n_threads)
{
#ifdef USE_NONBLOCK_JOIN
    for (size_t i = 0; i < n_threads; ++i) {
        if (threads[i]) {
            pthread_tryjoin_np(threads[i], NULL);
            printf("info: joined thread: %ld\n", (long)threads[i]);
            threads[i] = 0;
        }
    }
#else
    for (size_t i = 0; i < n_threads; ++i) {
        if (threads[i] && args[i].done == 1) {
            pthread_join(threads[i], NULL);
            printf("info: joined thread: %ld\n", (long)threads[i]);
            threads[i] = 0;
        }
    }
#endif

    return;
}
