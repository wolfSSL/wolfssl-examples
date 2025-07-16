/* tls-sock-server-ca.c
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

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

#include "sockets.h"
#include "tls-info.h"
#include "certs.h"

#if !defined(NO_WOLFSSL_SERVER)

/* Application data to send. */
static const char msgHTTPIndex[] =
    "HTTP/1.1 200 OK\n"
    "Content-Type: text/html\n"
    "Connection: close\n"
    "\n"
    "<html>\n"
    "<head>\n"
    "<title>Welcome to wolfSSL!</title>\n"
    "</head>\n"
    "<body>\n"
    "<p>wolfSSL has successfully performed handshake!</p>\n"
    "</body>\n"
    "</html>\n";


/* Create a new wolfSSL CTX server with a certificate for authentication and a
 * client certificate for client authentication.
 */
static int wolfssl_server_ctx_new(WOLFSSL_CTX** ctx)
{
    int ret = 0;
    WOLFSSL_CTX* server_ctx = NULL;

    /* Create and initialize WOLFSSL_CTX */
    if ((server_ctx = wolfSSL_CTX_new(wolfSSLv23_server_method())) == NULL) {
        printf("ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
    }

    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_use_certificate_buffer(server_ctx, SERVER_CERT,
                SERVER_CERT_LEN, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load server certificate\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_use_PrivateKey_buffer(server_ctx, SERVER_KEY,
                SERVER_KEY_LEN, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load server key\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        wolfSSL_CTX_set_verify(server_ctx, WOLFSSL_VERIFY_PEER |
                                        WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0);

        /* Load server certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_load_verify_buffer(server_ctx, CLIENT_CERT,
                CLIENT_CERT_LEN, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load CA certificate\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Return newly created wolfSSL context */
        *ctx = server_ctx;
    }
    else {
        if (server_ctx != NULL)
            wolfSSL_CTX_free(server_ctx);
    }
    return ret;
}

/* Create a new wolfSSL server with a certificate for authentication. */
static int wolfssl_server_ssl_new(WOLFSSL_CTX* ctx, WOLFSSL** ssl)
{
    int ret = 0;
    WOLFSSL*     server_ssl = NULL;

    if (ret == 0) {
        /* Create a WOLFSSL object */
        if ((server_ssl = wolfSSL_new(ctx)) == NULL) {
            printf("ERROR: failed to create WOLFSSL object\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Return newly created wolfSSL context and object */
        *ssl = server_ssl;
    }
    else {
        if (server_ssl != NULL)
            wolfSSL_free(server_ssl);
    }

    return ret;
}

/* Server accepting a client using TLS */
static int wolfssl_server_accept(WOLFSSL* ssl)
{
    int ret = 0;

    if (wolfSSL_accept(ssl) != WOLFSSL_SUCCESS) {
        ret = wolfSSL_get_error(ssl, 0);
        if (ret == WOLFSSL_ERROR_WANT_READ)
            ret = 0;
        else if (ret > 0)
            ret = -1;
    }

    return ret;
}


/* Send application data. */
static int wolfssl_send(WOLFSSL* ssl, const char* msg)
{
    int ret = 0;
    int len;

    printf("Sending:\n%s\n", msg);
    len = wolfSSL_write(ssl, msg, XSTRLEN(msg));
    if (len < 0)
        ret = len;
    else if (len != XSTRLEN(msg))
        ret = -1;

    return ret;
}

/* Receive application data. */
static int wolfssl_recv(WOLFSSL* ssl)
{
    int  ret;
    int  err = 0;
    byte reply[80];
    int  total = 0;

    printf("Receive:\n");
    do {
        ret = wolfSSL_read(ssl, reply, sizeof(reply)-1);
        if (ret > 0) {
            reply[ret] = '\0';
            printf("%s", reply);
            total += ret;
            err = 0;
        }
        else if (ret == -1)
            err = wolfSSL_get_error(ssl, 0);
    }
    while (err == 0 || (total == 0 && err == WOLFSSL_ERROR_WANT_READ));

    if (total > 0 && err == WOLFSSL_ERROR_WANT_READ)
        ret = 0;

    return ret;
}

/* Establish a socket to listen on. */
static int wolfssl_server_listen_tcp(SOCKET_T* fd)
{
    word16 port = wolfSSLPort;

    return tcp_listen(fd, &port, 1, 0, 0);
}

/* Accept the TCP connection from the client. */
static int wolfssl_server_accept_tcp(WOLFSSL* ssl, SOCKET_T fd,
                                     SOCKET_T* acceptfd)
{
    int      ret = 0;
    SOCKET_T clientfd = WOLFSSL_SOCKET_INVALID;

    ret = tcp_accept(&fd, &clientfd);
    if (ret == 0) {
        *acceptfd = clientfd;
        ret = tcp_set_nonblocking(&clientfd);
    }

    if (ret == 0) {
        if (wolfSSL_set_fd(ssl, clientfd) != WOLFSSL_SUCCESS)
            ret = -1;
    }

    return ret;
}


/* Server operations. */
static int server(WOLFSSL_CTX* server_ctx, SOCKET_T sockfd)
{
    int      ret;
    WOLFSSL* server_ssl = NULL;
    SOCKET_T clientfd = WOLFSSL_SOCKET_INVALID;

    do {
        ret = wolfssl_server_ssl_new(server_ctx, &server_ssl);
        if (ret == 0)
            ret = wolfssl_server_accept_tcp(server_ssl, sockfd, &clientfd);

        while (ret == 0) {
            ret = wolfssl_server_accept(server_ssl);
            if (ret == 0 && wolfSSL_is_init_finished(server_ssl))
                break;
        }

        if (ret == 0)
            showPeerEx(server_ssl, 0);

        /* Receive HTTP request */
        if (ret == 0)
            ret = wolfssl_recv(server_ssl);
        /* Send HTTP response */
        if (ret == 0)
            ret = wolfssl_send(server_ssl, msgHTTPIndex);

        if (server_ssl != NULL)
            wolfSSL_free(server_ssl);

        if (clientfd != WOLFSSL_SOCKET_INVALID)
            CloseSocket(clientfd);
    }
    while (ret == 0);

    return ret;
}

/* Main entry point. */
int main(int argc, char* argv[])
{
    int          ret = 0;
    WOLFSSL_CTX* server_ctx = NULL;
    SOCKET_T     sockfd   = WOLFSSL_SOCKET_INVALID;

#if defined(DEBUG_WOLFSSL)
    wolfSSL_Debugging_ON();
#endif
    /* Initialise wolfSSL library */
    wolfSSL_Init();

    ret = wolfssl_server_ctx_new(&server_ctx);
    if (ret == 0)
        ret = wolfssl_server_listen_tcp(&sockfd);

    /* Do server */
    ret = server(server_ctx, sockfd);

    if (server_ctx != NULL)
        wolfSSL_CTX_free(server_ctx);
    if (sockfd != WOLFSSL_SOCKET_INVALID)
        CloseSocket(sockfd);

    /* Cleanup wolfSSL library */
    wolfSSL_Cleanup();

    if (ret == 0)
        printf("Done\n");
    else {
        char buffer[80];
        printf("Error: %d, %s\n", ret, wolfSSL_ERR_error_string(ret, buffer));
    }

    return (ret == 0) ? 0 : 1;
}

#else

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    printf("Must build wolfSSL with server enabled for this example\n");
    return 0;
}

#endif
