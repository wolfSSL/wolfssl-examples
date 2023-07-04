/* server-tls-nonblocking.c
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

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>

#define DEFAULT_PORT 11111
#define SELECT_WAIT_SEC 1

#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"

enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_SEND_READY,
    TEST_ERROR_READY
};

static int tcp_select(SOCKET_T socketfd, int to_sec, int rx)
{
    fd_set fds, errfds;
    fd_set* recvfds = NULL;
    fd_set* sendfds = NULL;
    SOCKET_T nfds = socketfd + 1;
    struct timeval timeout;
    int result;

    FD_ZERO(&fds);
    FD_SET(socketfd, &fds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    if (rx)
        recvfds = &fds;
    else
        sendfds = &fds;

    result = select(nfds, recvfds, sendfds, &errfds, &timeout);

    if (result == 0)
        return TEST_TIMEOUT;
    else if (result > 0) {
        if (FD_ISSET(socketfd, &fds)) {
            if (rx)
                return TEST_RECV_READY;
            else
                return TEST_SEND_READY;
        }
        else if (FD_ISSET(socketfd, &errfds))
            return TEST_ERROR_READY;
    }

    return TEST_SELECT_FAIL;
}


int main()
{
    int ret, err;
    int                sockfd = SOCKET_INVALID;
    int                connd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    int                shutdown = 0;
    const char*        reply = "I hear ya fa shizzle!\n";

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;



    /* Initialize wolfSSL */
    wolfSSL_Init();
    //wolfSSL_Debugging_ON();


    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }

    /* Set the socket options to use nonblocking I/O */
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
        fprintf(stderr, "ERROR: failed to set socket options\n");
        ret = -1;
        goto exit; 
    }



    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* Load server certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto exit;
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        ret = -1;
        goto exit;        
    }



    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */



    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto exit;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        while ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
               == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* non-blocking, wait for read activity on socket */
                tcp_select(sockfd, SELECT_WAIT_SEC, 1);
                continue;
            } 
            else if (errno == EINPROGRESS || errno == EALREADY) {
                break;
            }
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1;
            goto exit;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1;
            goto exit;
        }

        /* Attach wolfSSL to the socket */
        if ((ret = wolfSSL_set_fd(ssl, connd)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
            goto exit;
        }
        
        /* Establish TLS connection */
        printf("wolfSSL_accepting\n");
       
        do {
            ret = wolfSSL_accept(ssl);
            err = wolfSSL_get_error(ssl, ret);
            if (err == WOLFSSL_ERROR_WANT_READ)
                tcp_select(sockfd, SELECT_WAIT_SEC, 1);
        } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);
        if (ret != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_accept error %d (%d)\n", err, ret);
            goto exit;
        }
        printf("client connected successfully\n");


        /* read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        do {
            ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
            err = wolfSSL_get_error(ssl, ret);
            
            if (err == WOLFSSL_ERROR_WANT_READ)
                tcp_select(sockfd, SELECT_WAIT_SEC, 1);
        }
        while (err == WOLFSSL_ERROR_WANT_READ);
        if (ret < 0) {
            fprintf(stderr, "ERROR %d: failed to read\n", ret);
            goto exit;
        }

        /* Print to stdout any data the client sends */
        printf("Client: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }



        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        /* Reply back to the client */
        do {
            ret = wolfSSL_write(ssl, reply, len);
            err = wolfSSL_get_error(ssl, ret);
            sleep(1);
        }
        while (err == WOLFSSL_ERROR_WANT_WRITE);
        if (ret < 0) {
            fprintf(stderr, "ERROR %d: failed to write\n", ret);
            goto exit;
        }

        /* send close notify */
        do {
            ret = wolfSSL_shutdown(ssl);
            err = wolfSSL_get_error(ssl, 0);
            if (err == WOLFSSL_ERROR_WANT_READ)
                tcp_select(sockfd, SELECT_WAIT_SEC, 1);
        } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);

        /* Cleanup after this connection */
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        ssl = NULL;
        close(connd);           /* Close the connection to the client   */
        connd = SOCKET_INVALID;
    }

    printf("Shutdown complete\n");

    ret = 0;

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (connd != SOCKET_INVALID)
        close(connd);           /* Close the connection to the client   */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket listening for clients   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */
   
    return ret;
}
