/*
 * Copyright (C) 2021 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 *
 * This file is subject to the terms and conditions of the GNU 
 * General Public License v2. See the file LICENSE in the top level
 * directory for more details.
 */
/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       wolfSSL server example
 *
 * @author      Daniele Lacamera <daniele@wolfssl.com>, Kaleb J. Himes <kaleb@wolfssl.com>
 *
 * @}
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "wolfssl/ssl.h"
#include "wolfssl/certs_test.h"


#define DEFAULT_PORT 11111


int tls_server(int argc, char *argv[])
{
    int                sockfd;
    char               buff[256];
    int                shutdown = 0;
    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;
#ifdef LWIP_IPV6
    struct sockaddr_in6 servAddr;
    struct sockaddr_in6 clientAddr;
    socklen_t          size = sizeof(clientAddr);
    /* Create a socket that uses an internet IPv6 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin6_family       = AF_INET6;             /* using IPv6      */
    servAddr.sin6_port         = htons(DEFAULT_PORT);  /* on DEFAULT_PORT */
#else
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family       = AF_INET;             /* using IPv6      */
    servAddr.sin_port         = htons(DEFAULT_PORT);  /* on DEFAULT_PORT */
#endif
    (void)argc;
    (void)argv;

    /* Bind the server socket to local port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        return -1;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        return -1;
    }

    puts("This is the TLS Server!");
    puts("Server is listening on port 11111");

    wolfSSL_Init();
    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    /* Load server certificates into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_certificate_buffer(ctx, server_cert_der_2048,
                                         sizeof_server_cert_der_2048,
                                         SSL_FILETYPE_ASN1) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load server_cert_der_2048\n");
        return -1;
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_buffer(ctx, server_key_der_2048,
                                        sizeof_server_key_der_2048,
                                        SSL_FILETYPE_ASN1) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load server_key_der_2048\n");
        return -1;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        int  connd;
        size_t len;
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        connd = accept(sockfd, (struct sockaddr *)&clientAddr, &size);
        if (connd < 0)  {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            continue;
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
        if (wolfSSL_read(ssl, buff, sizeof(buff)-1) == -1) {
            fprintf(stderr, "ERROR: failed to read, state: %d\n", wolfSSL_state(ssl));
            return -1;
        }

        /* Print to stdout any data the client sends */
        printf("Client sent a message!\n");
        printf("Client said: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }

        /* Write our reply into buff */
        strncpy(buff, "I hear ya fa shizzle!\n", sizeof(buff) - 1);
        len = strnlen(buff, sizeof(buff));
        printf("Sending reply to client, reply reads: %s\n", buff);

        /* Reply back to the client */
        if (wolfSSL_write(ssl, buff, len) != (int) len) {
            fprintf(stderr, "ERROR: failed to write\n");
            return -1;
        }

        /* Cleanup after this connection */
        close(connd);           /* Close the connection to the client   */
    }

    printf("Shutdown complete\n");

    /* Cleanup and return */
    close(sockfd);          /* Close the socket listening for clients   */
    return 0;               /* Return reporting a success               */
}
