/* server-tls.c
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
#include <wolfssl/ssl.h>


#define DEFAULT_PORT 11111

#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"
#define DH_FILE   "../certs/dh2048.pem"

int main()
{
    int                sockfd;
    int                connd;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;


    /* Initialize wolfSSL */
    wolfSSL_Init();


    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }

    /* Set the socket options to use nonblocking I/O */
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
        fprintf(stderr, "ERROR: failed to set socket options\n");
        return -1;
    }


    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    /* Load server certificates into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        return -1;
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        return -1;
    }

    /* Set DH params for WOLFSSL_CTX */
    if (wolfSSL_CTX_SetTmpDH_file(ctx, DH_FILE, SSL_FILETYPE_PEM)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set DH parameters.\n");
        return -1;
    }


    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */


    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        fprintf(stderr, "ERROR: failed to bind\n");
        return -1;
    }


    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        return -1;
    }


    printf("Waiting for a connection...\n");

    /* Accept client connections */
    while ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size)) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* no error, just non-blocking. Carry on. */
            continue;
        }
        fprintf(stderr, "ERROR: failed to accept the connection\n\n");
        return -1;
    }


    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        return -1;
    }


    /* Attach wolfSSL to the socket */
    wolfSSL_set_fd(ssl, connd);


    printf("Client connected successfully\n");

    /* Read the client data into our buff array */
    memset(buff, 0, sizeof(buff));
    while (wolfSSL_read(ssl, buff, sizeof(buff)-1) < 0) {
        if (wolfSSL_want_read(ssl)) {
            /* no error, just non-blocking. Carry on. */
            continue;
        }
        fprintf(stderr, "ERROR: failed to read\n");
        return -1;
    }

    /* Print to stdout any data the client sends */
    printf("Client: %s\n", buff);


    /* Write our reply into buff */
    memset(buff, 0, sizeof(buff));
    memcpy(buff, "I hear ya fa shizzle!\n", sizeof(buff));
    len = strnlen(buff, sizeof(buff));

    /* Reply back to the client */
    while (wolfSSL_write(ssl, buff, len) != len) {
        if (wolfSSL_want_write(ssl)) {
            /* no error, just non-blocking. Carry on. */
            continue;
        }
        fprintf(stderr, "ERROR: failed to write\n");
        return -1;
    }


    /* Cleanup and return */
    wolfSSL_free(ssl);     /* Free the wolfSSL object                */
    wolfSSL_CTX_free(ctx); /* Free the wolfSSL context object        */
    wolfSSL_Cleanup();     /* Cleanup the wolfSSL environment        */
    return 0;              /* Return reporting a success             */
}
