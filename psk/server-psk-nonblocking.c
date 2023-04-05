/* server-psk-nonblocking.c
 * A server example using a TCP connection with PSK security and non blocking.
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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

#include <wolfssl/options.h> /* included for option sync */
#include <wolfssl/ssl.h>     /* include wolfSSL security */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>      /* needed for running non-blocking connections */
#include <time.h>       /* for time out on read loop */

#define MAXLINE     4096
#define LISTENQ     1024
#define SERV_PORT   11111
#define PSK_KEY_LEN 4
#define dhParamFile    "../certs/dh2048.pem"

/* states of the tcp connection */
enum{
    TEST_SELECT_FAIL = 0,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};

#ifndef NO_PSK
/*
 * Used for finding psk value.
 */
static inline unsigned int my_psk_server_cb(WOLFSSL* ssl, const char* identity,
                                   unsigned char* key, unsigned int key_max_len)
{
    (void)ssl;
    (void)key_max_len;

    if (strncmp(identity, "Client_identity", 15) != 0) {
        return 0;
    }

    key[0] = 26;
    key[1] = 43;
    key[2] = 60;
    key[3] = 77;

    return PSK_KEY_LEN;
}
#endif

int main()
{
    int n;              /* length of string read */
    int ret;
    int error;
    int result;
    int select_ret = TEST_SELECT_FAIL;
    int sockfd;
    int nfds;
    int currTimeout = 1;
    int listenfd, connfd;
    int opt;
    char   buff[MAXLINE];  /* buffer for tcp connection */
    char   buf[MAXLINE];   /* string read from client */
    char   response[] = "I hear ya for shizzle";
    char suites[]   =
#ifdef WOLFSSL_STATIC_PSK
                      "PSK-AES256-GCM-SHA384:"
                      "PSK-AES128-GCM-SHA256:"
                      "PSK-AES256-CBC-SHA384:"
                      "PSK-AES128-CBC-SHA256:"
                      "PSK-AES128-CBC-SHA:"
                      "PSK-AES256-CBC-SHA:"
                      "PSK-CHACHA20-POLY1305:"
#endif
#if defined(WOLFSSL_TLS13_DRAFT18) || defined(WOLFSSL_TLS13_DRAFT22) || \
    defined(WOLFSSL_TLS13_DRAFT23) || defined(WOLFSSL_TLS13_DRAFT26) || \
    defined(WOLFSSL_TLS13)
                      "TLS13-AES128-GCM-SHA256:"
                      "TLS13-AES256-GCM-SHA384:"
                      "TLS13-CHACHA20-POLY1305-SHA256:"
#endif
#ifndef NO_DH
                      "DHE-PSK-AES256-GCM-SHA384:"
                      "DHE-PSK-AES128-GCM-SHA256:"
                      "DHE-PSK-AES256-CBC-SHA384:"
                      "DHE-PSK-AES128-CBC-SHA256:"
                      "DHE-PSK-CHACHA20-POLY1305"
#endif
                      "ECDHE-PSK-AES128-CBC-SHA256:"
                      "ECDHE-PSK-CHACHA20-POLY1305:";

    fd_set recvfds, errfds;
    socklen_t           cliLen;
    WOLFSSL_CTX*         ctx;
    struct sockaddr_in  cliAddr, servAddr;
    struct timeval timeout = {currTimeout, 0};


    /* find a socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        printf("Fatal error : socket error\n");
        return 1;
    }

    /* set up server address and port */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(SERV_PORT);

    /* bind to a socket */
    opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt,
                   sizeof(int)) != 0) {
        printf("Fatal error : setsockopt error");
        return 1;
    }
    if (bind(listenfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        printf("Fatal error : bind error\n");
        return 1;
    }

    wolfSSL_Init();

    if ((ctx = wolfSSL_CTX_new(wolfSSLv23_server_method())) == NULL) {
        printf("Fatal error : wolfSSL_CTX_new error\n");
        return 1;
    }

#ifndef NO_PSK
    /* use psk suite for security */
    wolfSSL_CTX_set_psk_server_callback(ctx, my_psk_server_cb);

    wolfSSL_CTX_use_psk_identity_hint(ctx, "wolfssl server");
#else
    fprintf(stderr, "Warning: wolfSSL not built with PSK (--enable-psk)\n");
#endif

    if (wolfSSL_CTX_set_cipher_list(ctx, suites) != WOLFSSL_SUCCESS) {
        printf("Fatal error : server can't set cipher list\n");
        return 1;
    }

#ifndef NO_DH
    if ((ret = wolfSSL_CTX_SetTmpDH_file(ctx, dhParamFile, WOLFSSL_FILETYPE_PEM)
        ) != WOLFSSL_SUCCESS) {
        printf("Fatal error: server set temp DH params returned %d\n", ret);
        return ret;
    }
#endif


    /* main loop for accepting and responding to clients */
    for ( ; ; ) {
        WOLFSSL* ssl;

        /* listen to the socket */
        if (listen(listenfd, LISTENQ) < 0) {
            printf("Fatal error : listen error\n");
            return 1;
        }

        cliLen = sizeof(cliAddr);
        connfd = accept(listenfd, (struct sockaddr *) &cliAddr, &cliLen);
        if (connfd < 0) {
            if (errno != EINTR) {
                printf("Fatal error : accept error\n");
                return 1;
            }
        }
        else {
            printf("Connection from %s, port %d\n",
                inet_ntop(AF_INET, &cliAddr.sin_addr, buff, sizeof(buff)),
                ntohs(cliAddr.sin_port));

            /* create WOLFSSL object */
            if ((ssl = wolfSSL_new(ctx)) == NULL) {
                printf("Fatal error : wolfSSL_new error\n");
                return 1;
            }
            wolfSSL_set_fd(ssl, connfd);
            sockfd = wolfSSL_get_fd(ssl);

            /* set socket to non blocking and respond */
            if (fcntl(connfd, F_SETFL, O_NONBLOCK) < 0) {
                printf("Fatal error : fcntl set failed\n");
                return 1;
            }

            ret = wolfSSL_accept(ssl);
            error = wolfSSL_get_error(ssl, 0);

            /* clear buffer for client response to prevent unexpected output */
            memset(buf, 0, MAXLINE);
            do {

                while (ret != WOLFSSL_SUCCESS &&
                       (error == WOLFSSL_ERROR_WANT_READ ||
                        error == WOLFSSL_ERROR_WANT_WRITE)) {

                    /* print out for user notification */
                    if (error == WOLFSSL_ERROR_WANT_READ) {
                        printf("... server would read block\n");
                    }
                    else {
                        printf("... server would write block\n");
                    }

                    FD_ZERO(&recvfds);
                    FD_SET(sockfd, &recvfds);
                    FD_ZERO(&errfds);
                    FD_SET(sockfd, &errfds);

                    nfds = sockfd + 1;
                    result = select(nfds, &recvfds, NULL, &errfds, &timeout);

                    if (result == 0) {
                        select_ret = TEST_TIMEOUT;
                    }
                    else if (result > 0) {
                        if (FD_ISSET(sockfd, &recvfds)) {
                            select_ret = TEST_RECV_READY;
                        }
                        else if (FD_ISSET(sockfd, &errfds)) {
                            select_ret = TEST_ERROR_READY;
                        }
                    }
                    else {
                        select_ret = TEST_SELECT_FAIL;
                    }

          /* if tcp_select signal is ready try to accept else continue loop */
                    if ((select_ret == TEST_RECV_READY) ||
                        (select_ret == TEST_ERROR_READY)) {
                        ret = wolfSSL_accept(ssl);
                        error = wolfSSL_get_error(ssl, 0);
                    }
                    else if (select_ret == TEST_TIMEOUT) {
                        error = WOLFSSL_ERROR_WANT_READ;
                    }
                    else {
                        error = WOLFSSL_FATAL_ERROR;
                    }
                }
            /* failure to accept */
                if (ret != WOLFSSL_SUCCESS) {
                    printf("Fatal error : wolfSSL_accept failed\n");
                    ret = WOLFSSL_FATAL_ERROR;
                }

                if (ret != WOLFSSL_SUCCESS) {
                    return 1;
                }

                n = wolfSSL_read(ssl, buf, MAXLINE);
                if (n > 0) {
                    printf("%s\n", buf);
                }
            }
            while(n < 0);

            while (ret != WOLFSSL_SUCCESS &&
                   (error == WOLFSSL_ERROR_WANT_READ ||
                    error == WOLFSSL_ERROR_WANT_WRITE)) {

                /* print out for user notification */
                if (error == WOLFSSL_ERROR_WANT_READ) {
                    printf("... server would read block\n");
                }
                else {
                    printf("... server would write block\n");
                }

                FD_ZERO(&recvfds);
                FD_SET(sockfd, &recvfds);
                FD_ZERO(&errfds);
                FD_SET(sockfd, &errfds);

                nfds = sockfd + 1;
                result = select(nfds, &recvfds, NULL, &errfds, &timeout);

                if (result == 0) {
                    select_ret = TEST_TIMEOUT;
                }
                else if (result > 0) {
                    if (FD_ISSET(sockfd, &recvfds)) {
                        select_ret = TEST_RECV_READY;
                    }
                    else if (FD_ISSET(sockfd, &errfds)) {
                        select_ret = TEST_ERROR_READY;
                    }
                }
                else {
                    select_ret = TEST_SELECT_FAIL;
                }

        /* if tcp_select signals ready try to accept otherwise continue loop*/
                if ((select_ret == TEST_RECV_READY) ||
                    (select_ret == TEST_ERROR_READY)) {
                    ret = wolfSSL_accept(ssl);
                    error = wolfSSL_get_error(ssl, 0);
                }
                else if (select_ret == TEST_TIMEOUT) {
                    error = WOLFSSL_ERROR_WANT_READ;
                }
                else {
                    error = WOLFSSL_FATAL_ERROR;
                }
            }

        /* failure to accept */
            if (ret != WOLFSSL_SUCCESS) {
                printf("Fatal error : wolfSSL_accept failed\n");
                ret = WOLFSSL_FATAL_ERROR;
            }

            if (ret != WOLFSSL_SUCCESS) {
                return 1;
            }
            if ( wolfSSL_write(ssl, response, strlen(response)) !=
                                                strlen(response)) {
                printf("Fatal error : respond: write error\n");
                return 1;
            }

            /* closes the connections after responding */
            if (close(connfd) == -1) {
                printf("Fatal error : close error\n");
                return 1;
            }
            wolfSSL_shutdown(ssl);
            wolfSSL_free(ssl);
            ssl = NULL;
        }
    }

    /* free up memory used by wolfssl */
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return 0;
}

