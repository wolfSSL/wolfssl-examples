/* server-tcp.c
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
#include <netinet/in.h>
#include <unistd.h>

/* wolfSSL */
#include <wolfssl/ssl.h>


#define DEFAULT_PORT 11111

int main()
{
    int                sockfd;
    int                connd;
    char               buff[256];
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    int                ret = 0;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;


    /* initialize wolfSSL */
    wolfSSL_Init();


    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }


    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    /* Load server certificate into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_certificate_file(ctx, "../certs/server-cert.pem",
                                         SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to loadcerts/server-cert.pem, please "
                "check the file.\n");
        return -1;
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, "../certs/server-key.pem",
                                        SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load certs/server-key.pem, please "
                "check the file.\n");
        return -1;
    }

    /* Set DH params for WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_SetTmpDH_file(ctx, "../certs/dh2048.pem",
                                         SSL_FILETYPE_PEM)) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set DH parameters.\n");
        return -1;
    }


    /* Initialize the server address struct with zeros */
    memset(&serverAddr, 0, sizeof(serverAddr));

    /* Fill the server's address family */
    serverAddr.sin_family      = AF_INET;             /* using IPv4      */
    serverAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */
    serverAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */


    /* Attach the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
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
    if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size)) == -1) {
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

    /* Read in from the client while there is something to read */
    do {
        /* Clear the buffer memory for anything possibly left over */
        memset(buff, 0, sizeof(buff));


        /* Read the client data into our buff array */
        if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) > 0) {
            /* Print any data the client sends to the console */
            printf("Client: %s\n", buff);


            /* Write our reply into buff */
            memset(buff, 0, sizeof(buff));
            memcpy(buff, "I hear ya fa shizzle!\n", 23);


            /* Reply back to the client */
            if ((ret = wolfSSL_write(ssl, buff, sizeof(buff)-1)) < 0) {
                fprintf(stderr, "ERROR: failed to write\n");
            }
        }
    } while (ret > 0);


    /* Check for a read error condition */
    if (ret == 0) {
        printf("Client has closed the connection.\n");
    }
    else {
        fprintf(stderr, "ERROR: failed to read\n");
        return -1;
    }


    /* Cleanup and return */
    wolfSSL_free(ssl);     /* Free the wolfSSL object                */
    wolfSSL_CTX_free(ctx); /* Free the wolfSSL context object        */
    wolfSSL_Cleanup();     /* Cleanup the wolfSSL environment        */
    return 0;              /* Return reporting a success             */

}
