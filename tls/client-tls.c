/* client-tcp.c
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

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

/* wolfSSL */
#include <wolfssl/ssl.h>


#define DEFAULT_PORT 11111

int main(int argc, char** argv)
{
    int     sockfd;
    struct  sockaddr_in servAddr;
    char    buff[256];
    size_t  len;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;


    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: ./client-tcp  <IPv4 address>\n");
        return 0;
    }


    /* Initialize wolfSSL */
    wolfSSL_Init();


    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }


    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to make WOLFSSL_CTX\n");
        return -1;
    }

    /* Load certificates into WOLFSSL_CTX */
    if (wolfSSL_CTX_load_verify_locations(ctx, "../certs/ca-cert.pem", NULL)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load certs/ca-cert.pem, please "
                "check the file.\n");
        return -1;
    }


    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;           /* use IPv4                  */
    servAddr.sin_port = htons(DEFAULT_PORT); /* look at port DEFAULT_PORT */

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid Address\n");
        return -1;
    }


    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0) {
        fprintf(stderr, "ERROR: failed to connect\n");
        return -1;
    }

    /* Make WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        return -1;
    }

    /* Hook wolfSSL up to the socket */
    wolfSSL_set_fd(ssl, sockfd);


    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    fgets(buff, sizeof(buff), stdin);
    len = strnlen(buff, sizeof(buff));

    /* Send the message */
    if (wolfSSL_write(ssl, buff, len) != len) {
        fprintf(stderr, "ERROR: failed to write\n");
        return -1;
    }


    /* Get a response from the server */
    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(ssl, buff, sizeof(buff)-1) < 0) {
        fprintf(stderr, "ERROR: failed to read\n");
        return -1;
    }

    /* Print the message to stdout */
    printf("Recieved:           %s\n", buff);


    /* Cleanup and return */
    wolfSSL_free(ssl);     /* Free the wolfSSL object                */
    wolfSSL_CTX_free(ctx); /* Free the wolfSSL context object        */
    wolfSSL_Cleanup();     /* Cleanup the wolfSSL environment        */
    return 0;              /* Return reporting a success             */
}
