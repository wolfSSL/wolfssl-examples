/* tls-sock-client-ca.c
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
#include <wolfssl/error-ssl.h>

#include "sockets.h"
#include "tls-info.h"
#include "certs.h"

#if !defined(NO_WOLFSSL_CLIENT)

/* Application data to send. */
static const char msgHTTPGet[] = "GET /index.html HTTP/1.0\r\n\r\n";

/* Create a new wolfSSL client context with a server CA certificate and a
 * client certificate and key.
 */
static int wolfssl_client_ctx_new(WOLFSSL_CTX** ctx)
{
    int ret = 0;
    WOLFSSL_CTX* client_ctx = NULL;

    /* Create and initialize WOLFSSL_CTX */
    if ((client_ctx = wolfSSL_CTX_new(wolfSSLv23_client_method())) == NULL) {
        printf("ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
    }

    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_load_verify_buffer(client_ctx, CA_CERTS, CA_CERTS_LEN,
                WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load CA certificate\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_use_certificate_buffer(client_ctx, CLIENT_CERT,
                CLIENT_CERT_LEN, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load client certificate\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_use_PrivateKey_buffer(client_ctx, CLIENT_KEY,
                CLIENT_KEY_LEN, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load client key\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Return newly created wolfSSL context */
        *ctx = client_ctx;
    }
    else {
        if (client_ctx != NULL)
            wolfSSL_CTX_free(client_ctx);
    }

    return ret;
}

/* Create a new wolfSSL client object. */
static int wolfssl_client_ssl_new(WOLFSSL_CTX* ctx, WOLFSSL** ssl)
{
    int       ret = 0;
    WOLFSSL*  client_ssl = NULL;

    /* Create a WOLFSSL object */
    if ((client_ssl = wolfSSL_new(ctx)) == NULL) {
        printf("ERROR: failed to create WOLFSSL object\n");
        ret = -1;
    }

    if (ret == 0) {
        /* Return newly created wolfSSL object */
        *ssl = client_ssl;
    }
    else {
        if (client_ssl != NULL)
            wolfSSL_free(client_ssl);
    }

    return ret;
}

/* Client connecting to server using TLS */
static int wolfssl_client_connect(WOLFSSL* ssl)
{
    int ret = 0;

    if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
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
    if (ret == -1 && err == SOCKET_PEER_CLOSED_E) {
        printf("Peer closed socket\n");
        ret = 0;
    }

    return ret;
}


/* TCP connect to the server. */
int wolfssl_client_connect_tcp(WOLFSSL* ssl, SOCKET_T* fd)
{
    int              ret = 0;

    ret = tcp_connect(fd, wolfSSLIP, wolfSSLPort, 0, 0, ssl);
    if (ret == 0)
        ret = tcp_set_nonblocking(fd);

    if (ret == 0) {
        if (wolfSSL_set_fd(ssl, *fd) != WOLFSSL_SUCCESS)
            ret = -1;
    }

    return ret;
}

/* Client connection operations */
static int client(WOLFSSL_CTX* client_ctx)
{
    int          ret = 0;
    WOLFSSL*     client_ssl = NULL;
    SOCKET_T     sockfd = WOLFSSL_SOCKET_INVALID;

    /* Create a new client connection */
    ret = wolfssl_client_ssl_new(client_ctx, &client_ssl);

    if (ret == 0)
        wolfssl_client_connect_tcp(client_ssl, &sockfd);

    /* Keep trying to connect until handshake finished */
    while (ret == 0) {
        ret = wolfssl_client_connect(client_ssl);
        if (ret == 0 && wolfSSL_is_init_finished(client_ssl))
            break;
    }
    if (ret == 0)
        showPeerEx(client_ssl, 0);

    /* Send HTTP request */
    if (ret == 0)
        ret = wolfssl_send(client_ssl, msgHTTPGet);
    /* Receive HTTP response */
    if (ret == 0)
        ret = wolfssl_recv(client_ssl);

    if (sockfd != WOLFSSL_SOCKET_INVALID)
        CloseSocket(sockfd);
    if (client_ssl != NULL)
        wolfSSL_free(client_ssl);

    return ret;
}

int main(int argc, char* argv[])
{
    int          ret = 0;
    WOLFSSL_CTX* client_ctx = NULL;

#if defined(DEBUG_WOLFSSL)
    wolfSSL_Debugging_ON();
#endif
    /* Initialise wolfSSL library */
    wolfSSL_Init();

    ret = wolfssl_client_ctx_new(&client_ctx);

    /* Do client */
    client(client_ctx);

    if (client_ctx != NULL)
        wolfSSL_CTX_free(client_ctx);

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
    printf("Must build wolfSSL with client enabled for this example\n");
    return 0;
}

#endif
