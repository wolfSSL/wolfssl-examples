/* server-callback.c
 *
 * Copyright (C) 2006-2015 wolfSSL Inc.
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

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/options.h>

#include <wolfssl/ssl.h>
#include <wolfssl/test.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*look in new location for certs*/
#if defined(svrCertFile) || defined(svrKeyFile)
    #undef svrCertFile
    #undef svrKeyFile
    #define svrCertFile    "../certs/server-cert.pem"
    #define svrKeyFile     "../certs/server-key.pem"
#endif

#define MAXSZ     1024
#define YASSLPORT 11111

#define SOCKET_EWOULDBLOCK EWOULDBLOCK
#define SOCKET_EAGAIN      EAGAIN
#define SOCKET_ECONNRESET  ECONNRESET
#define SOCKET_EINTR       EINTR
#define SOCKET_EPIPE       EPIPE
#define SOCKET_ECONNREFUSED ECONNREFUSED
#define SOCKET_ECONNABORTED ECONNABORTED

/*
 * function with specific parameters : inbetween process of receiving msg
 * based from embeded receive in src/io.c
 */
int CbIORecv(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int recvd;
    int sd = *(int*)ctx;

    recvd = recv(sd, buf, sz, 0);

    if (recvd < 0) {
        if (errno == SOCKET_EWOULDBLOCK || errno == SOCKET_EAGAIN) {
            if (!wolfSSL_dtls(ssl) || wolfSSL_get_using_nonblock(ssl)) {
                printf(" Would block\n");
                return WOLFSSL_CBIO_ERR_WANT_READ;
            }
            else {
                printf("Socket timeout\n");
                return WOLFSSL_CBIO_ERR_TIMEOUT;
            }
        }
        else if (errno == SOCKET_ECONNRESET) {
            printf("Connection reset\n");
            return WOLFSSL_CBIO_ERR_CONN_RST;
        }
        else if (errno == SOCKET_EINTR) {
            printf("Socket interrupted\n");
            return WOLFSSL_CBIO_ERR_ISR;
        }
        else if (errno == SOCKET_ECONNREFUSED) {
            printf("Connection refused\n");
            return WOLFSSL_CBIO_ERR_WANT_READ;
        }
        else if (errno == SOCKET_ECONNABORTED) {
            printf("Connection aborted\n");
            return WOLFSSL_CBIO_ERR_CONN_CLOSE;
        }
        else {
            printf("General error\n");
            return WOLFSSL_CBIO_ERR_GENERAL;
        }
    }
    else if (recvd == 0) {
        printf("Embed receive connection closed\n");
        return WOLFSSL_CBIO_ERR_CONN_CLOSE;
    }

    printf("Received %d bytes\n", sz);
    return recvd;
}


/*
 * function with specific parameters : inbetween process of sending out msg
 * based from embeded receive in src/io.c
 */
int CbIOSend(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int sd = *(int*)ctx;
    int sent;
    int len = sz;

    sent =send(sd, &buf[sz - len], len, 0);

    if (sent < 0) {
        printf("IO Send error\n");
        if (errno == SOCKET_EWOULDBLOCK || errno == SOCKET_EAGAIN) {
            printf("Would Block\n");
            return WOLFSSL_CBIO_ERR_WANT_WRITE;
        }
        else if (errno == SOCKET_ECONNRESET) {
            printf("Connection reset\n");
            return WOLFSSL_CBIO_ERR_CONN_RST;
        }
        else if (errno == SOCKET_EINTR) {
            printf("Socket interrupted\n");
            return WOLFSSL_CBIO_ERR_ISR;
        }
        else if (errno == SOCKET_EPIPE) {
            printf("Socket EPIPE\n");
            return WOLFSSL_CBIO_ERR_CONN_CLOSE;
        }
        else {
            printf("General error\n");
            return WOLFSSL_CBIO_ERR_GENERAL;
        }
    }

    printf("CbIOSend: sent %d bytes to %d\n", sz, sd);
    return sent;
}


int Server(word16 port)
{
    char        msg[MAXSZ];
    const char  reply[]  = "I hear ya fa shizzle!\n";
    int         n, listenfd, connfd;
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;

    wolfSSL_Init();

    /* create ctx and configure certificates */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL)
        err_sys("Fatal error : wolfSSL_CTX_new error");

    if (wolfSSL_CTX_use_certificate_file(ctx, svrCertFile, SSL_FILETYPE_PEM)
                != SSL_SUCCESS)
        err_sys("can't load server cert file,"
                    "Please run from wolfSSL home dir");

    if (wolfSSL_CTX_use_PrivateKey_file(ctx, svrKeyFile, SSL_FILETYPE_PEM)
                != SSL_SUCCESS)
        err_sys("can't load server key file, "
                    "Please run from wolfSSL home dir");

    /*sets the IO callback methods*/
    wolfSSL_SetIORecv(ctx, CbIORecv);
    wolfSSL_SetIOSend(ctx, CbIOSend);

    tcp_accept(&listenfd, &connfd, NULL, port, 1, 0, 0, 0, 1);

    if (connfd < 0) {
        err_sys("Fatal error : accept error");
    }
    else {
        /* create WOLFSSL object and respond */
        if ((ssl = wolfSSL_new(ctx)) == NULL)
            err_sys("Fatal error : wolfSSL_new error");

        wolfSSL_set_fd(ssl, connfd);

	    memset(msg, 0, MAXSZ);
	    n = wolfSSL_read(ssl, msg, MAXSZ - 1);
	    if (n > 0) {
	        msg[n] = '\0';
	        printf("Client sent : %s\n", msg);
	        if (wolfSSL_write(ssl, reply, strlen(reply)) > strlen(reply))
	            err_sys("Fatal error : respond: write error");
	    }

	    if (n < 0)
	        err_sys("Fatal error :respond: read error");

        /* closes the connections after responding */
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
        if (close(listenfd) == -1 && close(connfd) == -1)
            err_sys("Fatal error : close error");
    }

    /* free up memory used by wolfSSL */
    wolfSSL_CTX_free(ctx);

    return 0;
}
int main(int argc, char* argv[])
{
    if (argc > 2) {
        printf("./server\nor ./server <port>\n");
        return 1;
    }

    wolfSSL_Init();
    if (argc < 2) {
        if (Server(YASSLPORT) != 0)
            err_sys("error creating server");
        }
    else {
        if (Server((word16)atoi(argv[1])) != 0)
            err_sys("error creating server");
    }
    wolfSSL_Cleanup();

    return 0;
}

