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


    /*
     * Creates a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol.
     */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        /* exit the program with -1 after printing the message to stderr */
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }


    /* Initialize the server address struct to zero */
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


    printf("Client connected successfully\n");

    /* Read in from the client while there is something to read */
    do {
        /* Clear the buffer memory for anything  possibly left over */
        memset(buff, 0, sizeof(buff));


        /* Read the client data into our buff array */
        if ((ret = read(connd, buff, sizeof(buff)-1)) > 0) {
            /* Print any data the client sends to the console */
            printf("Client: %s\n", buff);


            /* Write our reply into buff */
            memset(buff, 0, sizeof(buff));
            memcpy(buff, "I hear ya fa shizzle!\n", 23);


            /* Reply back to the client */
            if ((ret = write(connd, buff, sizeof(buff)-1)) < 0) {
                /* Write an error without exiting the program */
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
    }


    /* Cleanup and return */
    close(connd);  /* Close the connection to the client     */
    close(sockfd); /* Close the socket listening for clients */
    return 0;      /* Return reporting a success             */

}
