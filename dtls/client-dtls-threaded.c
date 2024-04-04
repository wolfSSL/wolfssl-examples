/*
 * client-dtls-threaded.c
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
 * A simple dtls client with configurable threadpool, for
 * instructional/learning purposes. Utilizes DTLS 1.2.
 */

#include <wolfssl/options.h>
#include <unistd.h>
#include <wolfssl/ssl.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MSGLEN          4096
#define SERV_PORT       11111
#define DTLS_NUMTHREADS 16

typedef struct {
    WOLFSSL *     ssl;
    int           activefd;
    WOLFSSL_CTX * ctx;
} thread_args_t;

static void   safer_shutdown(thread_args_t * args);
static void * client_work(void * arg);

int
main(int    argc,
     char * argv[])
{
    WOLFSSL_CTX *   ctx = NULL; /* ctx shared by threads */
    const char *    certs = "../certs/ca-cert.pem";
    int             ret = 0;
    int             n_threads = 2;
    pthread_t       threads[DTLS_NUMTHREADS];
    thread_args_t   args[DTLS_NUMTHREADS];
    int             opt = 0;

    memset(threads, 0, sizeof(threads));
    memset(args, 0, sizeof(args));

    while ((opt = getopt(argc, argv, "t:?")) != -1) {
        switch (opt) {
        case 't':
            n_threads = atoi(optarg);
            break;

        case '?':
            printf("usage:\n");
            printf("  ./client-dtls-threaded [-t n]\n");
            printf("\n");
            printf("description:\n");
            printf("  A simple dtls client with configurable threadpool.\n");
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

    /* Initialize wolfSSL before assigning ctx */
    wolfSSL_Init();

    ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method());
    if (ctx == NULL) {
        printf("error: wolfSSL_CTX_new failed\n");
        return EXIT_FAILURE;
    }

    /* Load certificates into ctx variable */
    ret = wolfSSL_CTX_load_verify_locations(ctx, certs, 0);
    if (ret != SSL_SUCCESS) {
        printf("error: loading %s failed, please check the file\n", certs);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < n_threads; ++i) {
        args[i].ctx = ctx;
        ret = pthread_create(&threads[i], NULL, client_work, &args[i]);

        if (ret == 0 ) {
            printf("info: spawned thread: %ld\n", (long)threads[i]);
        }
        else {
            printf("error: pthread_create returned %d\n", ret);
            threads[i] = 0;
        }
    }

    for (size_t i = 0; i < n_threads; ++i) {
        if (threads[i]) {
            pthread_join(threads[i], NULL);
            printf("info: joined thread: %ld\n", (long)threads[i]);
            threads[i] = 0;
        }
    }

    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return EXIT_SUCCESS;
}

static void *
client_work(void * args)
{
    thread_args_t * thread_args = (thread_args_t *) args;
    int             n_bytes = 0;
    int             err1;
    struct          sockaddr_in servAddr;
    char            send_msg[MSGLEN];
    char            recv_msg[MSGLEN];
    int             ret = 0;
    const char *    localhost_ip = "127.0.0.1";

    /* Assign ssl variable */
    thread_args->ssl = wolfSSL_new(thread_args->ctx);
    if (thread_args->ssl == NULL) {
        printf("error: wolfSSL_new failed\n");
        return NULL;
    }

    /* servAddr setup */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);

    ret = inet_pton(AF_INET, localhost_ip, &servAddr.sin_addr);
    if (ret != 1) {
        printf("error: inet_pton %s returned %d\n", localhost_ip, ret);
        return NULL;
    }

    ret = wolfSSL_dtls_set_peer(thread_args->ssl, &servAddr, sizeof(servAddr));
    if (ret != SSL_SUCCESS) {
        printf("error: wolfSSL_dtls_set_peer returned %d\n", ret);
        return NULL;
    }

    thread_args->activefd = socket(AF_INET, SOCK_DGRAM, 0);
    if (thread_args->activefd <= 0) {
        printf("error: socket returned %d\n", thread_args->activefd);
        return NULL;
    }

    printf("info: opened socket: %d\n", thread_args->activefd);

    /* Set the file descriptor for ssl and connect with ssl variable */
    wolfSSL_set_fd(thread_args->ssl, thread_args->activefd);
    if (wolfSSL_connect(thread_args->ssl) != SSL_SUCCESS) {
        err1 = wolfSSL_get_error(thread_args->ssl, 0);
        printf("error: thread %ld: wolfSSL_connect returned = %d, %s\n",
                (long)pthread_self(), err1,
                wolfSSL_ERR_reason_error_string(err1));
        return NULL;
    }

    /* Simulate a simple protocol over DTLS that exchanges a
     * sequence of records, and checks we get correct records
     * from correct server threads. */
    long int     server_tid = 0;
    const char * tid_str = NULL; /* Str to thread id in response. */

    for (size_t i = 0; i < 4; ++i) {
        memset(send_msg, '\0', sizeof(send_msg));
        memset(recv_msg, '\0', sizeof(recv_msg));

        char seq = '0' + (int) i;
        sprintf(send_msg, "msg %zu from client thread %ld\n", i,
                (long)pthread_self());

        n_bytes = wolfSSL_write(thread_args->ssl, send_msg, strlen(send_msg));

        if (n_bytes != strlen(send_msg)) {
            printf("error: wolfSSL_write returned %d", n_bytes);
            break;
        }

        /* n is the # of bytes received */
        n_bytes = wolfSSL_read(thread_args->ssl, recv_msg, sizeof(recv_msg)-1);

        if (n_bytes < 0) {
            printf("error: wolfSSL_read returned %d", n_bytes);
            break;
        }

        recv_msg[n_bytes] = '\0';
        fputs(recv_msg, stdout);

        /* Check the server replied with the correct sequence record, e.g.:
         *   "msg 2 from server thread 140146322425536" */
        if (recv_msg[4] != seq) {
            printf("error: got msg %c, expected %c\n", recv_msg[4], seq);
            break;
        }

        tid_str = &recv_msg[25];
        if (server_tid == 0) {
            /* Save the server thread id on message 0. */
            server_tid = atol(tid_str);
        }
        else {
            /* Compare saved thread id. */
            if (server_tid != atol(tid_str)) {
                printf("error: got rsp from server thread %ld, expected %ld\n",
                       server_tid, atol(tid_str));
                break;
            }
            else {
                printf("info: got response from server thread %ld\n",
                       server_tid);
            }
        }

        sleep(1);
    }

    safer_shutdown(thread_args);

    return NULL;
}

/* Small shutdown wrapper to safely clean up a thread's
 * connection. */
static void
safer_shutdown(thread_args_t * args)
{
    if (args == NULL) {
        printf("error: safer_shutdown with null args\n");
        return;
    }

    if (args->ssl != NULL) {
        printf("info: closed tls session: %p\n", (void*) args->ssl);
        wolfSSL_shutdown(args->ssl);
        wolfSSL_free(args->ssl);
        args->ssl = NULL;
    }

    if (args->activefd > 0) {
        printf("info: closed socket: %d\n", args->activefd);
        close(args->activefd);
        args->activefd = 0;
    }

    return;
}
