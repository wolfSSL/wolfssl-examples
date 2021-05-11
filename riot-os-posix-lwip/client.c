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

#define SERVER_PORT 11111


int tls_client(int argc, char *argv[])
{
    int                sockfd;
    char               buff[] = "Hello, TLS Server!\0";
    char               *server_ip;
    size_t             len;
#ifdef LWIP_IPV6
    struct sockaddr_in6 servAddr;
#else
    struct sockaddr_in servAddr;
#endif

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;

    puts("This is a TLS Client!");
    if (argc != 2) {
        fprintf(stderr, "Usage: %s IP_ADDR_SERVER\n", argv[0]);
        return -1;
    }
    printf("Client is connecting to server at address %s port 11111...\n", argv[1]);
    server_ip = argv[1];

#ifdef LWIP_IPV6
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
    servAddr.sin6_port         = htons(SERVER_PORT);  /* on SERVER_PORT */

    /* Get the server IPv6 address from the compile-time string parameter */
    if (inet_pton(AF_INET6, server_ip, &servAddr.sin6_addr.s6_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        return -1;
    }
#else
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
    servAddr.sin_family       = AF_INET;             /* using IPv4      */
    servAddr.sin_port         = htons(SERVER_PORT);  /* on SERVER_PORT */

    /* Get the server IPv4 address from the compile-time string parameter */
    if (inet_pton(AF_INET, server_ip, &servAddr.sin_addr.s_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        return -1;
    }
#endif
    
    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        return -1;
    }

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        exit(-1);
    }

    /* Load client certificates into WOLFSSL_CTX */
    if (wolfSSL_CTX_load_verify_buffer(ctx, ca_cert_der_2048,
                                       sizeof_ca_cert_der_2048,
                                       SSL_FILETYPE_ASN1) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load ca buffer\n");
        exit(-1);
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        exit(-1);
    }

    /* Attach wolfSSL to the socket */
    wolfSSL_set_fd(ssl, sockfd);

    /* Connect to wolfSSL on the server side */
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL: %d\n", wolfSSL_state(ssl));
        exit(-1);
    }

    printf("Message for server: %s\n", buff);
    len = strlen(buff);

    /* Send the message to the server */
    if (wolfSSL_write(ssl, buff, len) != (int) len) {
        fprintf(stderr, "ERROR: failed to write\n");
        return -1;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(ssl, buff, sizeof(buff)-1) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        return -1;
    }

    /* Print to stdout any data the server sends */
    printf("Server sent a reply!\n");
    printf("Server Response was:  %s\n", buff);

    /* Cleanup and exit */
    close(sockfd);          /* Close the connection to the server       */
    return 0;               /* Return reporting a success               */
}
