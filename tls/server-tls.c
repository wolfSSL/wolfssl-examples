/* server-tls.c
 *
 * Copyright (C) 2006-2014 wolfSSL Inc.
 *
 * This file is part of CyaSSL.
 *
 * CyaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CyaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  * USA
 * =============================================================================
 *
 * This is a super basic example of what a TCP Server secured with TLS 1.2
 * might look like. This server can also resume the session if a client 
 * nadvertantly disconnects. 
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>

/* include the CyaSSL library for our TLS 1.2 security */
#include <cyassl/ssl.h>

#define DEFAULT_PORT 11111

int AcceptAndRead(CYASSL_CTX* ctx, int sockfd, struct sockaddr_in clientAddr);

int AcceptAndRead(CYASSL_CTX* ctx, int sockfd, struct sockaddr_in clientAddr)
{
        /* Create our reply message */
    const char reply[]  = "I hear ya fa shizzle!\n";
    int         size    = sizeof(clientAddr);
    int         ret     = 0;
    int         err     = 0;
    CYASSL*     ssl;

    /* Wait until a client connects */
    int connd = accept(sockfd, (struct sockaddr *)&clientAddr, &size);

    /* If fails to connect, loop back up and wait for a new connection */
    if (connd == -1) {
        printf("failed to accept the connection..\n");
    }
    /* If it connects, read in and reply to the client */
    else {
        printf("Client connected successfully\n");

        if ( (ssl = CyaSSL_new(ctx)) == NULL) {
            fprintf(stderr, "CyaSSL_new error.\n");
            exit(EXIT_FAILURE);
        }

        /* direct our ssl to our clients connection */
        CyaSSL_set_fd(ssl, connd);

        printf("Using Non-Blocking I/O: %d\n", CyaSSL_get_using_nonblock(
            ssl));

        for ( ; ; ) {
            char buff[256];
            int  ret = 0;

            /* Clear the buffer memory for anything  possibly left over */
            memset(&buff, 0, sizeof(buff));

            /* Read the client data into our buff array */
            if ((ret = CyaSSL_read(ssl, buff, sizeof(buff)-1)) > 0) {
                /* Print any data the client sends to the console */
                printf("Client: %s\n", buff);
                
                /* Reply back to the client */
                if ((ret = CyaSSL_write(ssl, reply, sizeof(reply)-1)) 
                    < 0)
                {
                    printf("CyaSSL_write error = %d\n", CyaSSL_get_error(ssl, ret));
                }
            }
            /* if the client disconnects break the loop */
            else {
                if (ret < 0)
                    printf("CyaSSL_read error = %d\n", CyaSSL_get_error(ssl
                        ,ret));
                else if (ret == 0)
                    printf("The client has closed the connection.\n");

                break;
            }
        }
    }
    CyaSSL_free(ssl);           /* Free the CYASSL object */
    close(connd);               /* close the connected socket */

    return 0;
}


int main()
{
    /* Create a ctx pointer for our ssl */
    CYASSL_CTX* ctx;

    /* 
     * Creates a socket that uses an internet IP address,
     * Sets the type to be Stream based (TCP),
     * 0 means choose the default protocol.
     */
    int sockfd   = socket(AF_INET, SOCK_STREAM, 0);
    int loopExit = 0; /* 0 = False, 1 = True */
    int ret      = 0; /* Return value */
    /* Server and client socket address structures */
    struct sockaddr_in serverAddr, clientAddr;

    /* Initialize CyaSSL */
    CyaSSL_Init();

    /* If positive value, the socket is valid */
    if (sockfd < 0) {
        printf("ERROR: failed to create the socket\n");
        return EXIT_FAILURE;        /* Kill the server with exit status 1 */        
    }

    /* create and initialize CYASSL_CTX structure */
    if ((ctx = CyaSSL_CTX_new(CyaTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "CyaSSL_CTX_new error.\n");
        return EXIT_FAILURE;
    }

    /* Load server certificate into CYASSL_CTX */
    if (CyaSSL_CTX_use_certificate_file(ctx, "certs/server-cert.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-cert.pem, please check"
                "the file.\n");
        return EXIT_FAILURE;
    }

    /* Load server key into CYASSL_CTX */
    if (CyaSSL_CTX_use_PrivateKey_file(ctx, "certs/server-key.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-key.pem, please check"
                "the file.\n");
        return EXIT_FAILURE;
    }

    /* Initialize the server address struct to zero */
    memset((char *)&serverAddr, 0, sizeof(serverAddr)); 

    /* Fill the server's address family */
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port        = htons(DEFAULT_PORT);

    /* Attach the server socket to our port */
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr))
        < 0) {
        printf("ERROR: failed to bind\n");
        return EXIT_FAILURE;
    }

    printf("Waiting for a connection...\n");
    /* Continuously accept connects while not currently in an active connection
       or told to quit */
    while (loopExit == 0) {
        /* listen for a new connection, allow 5 pending connections */
        ret = listen(sockfd, 5);
        if (ret == 0) {

            /* Accept client connections and read from them */
            loopExit = AcceptAndRead(ctx, sockfd, clientAddr);
        }
    }

    CyaSSL_CTX_free(ctx);   /* Free CYASSL_CTX */
    CyaSSL_Cleanup();       /* Free CyaSSL */
    return EXIT_SUCCESS;
}
