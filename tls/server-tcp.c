/* server-tcp.c
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 * =============================================================================
 *
 * This is a super basic example of what a TCP Server might look like that is
 * not actively using any form of security. 
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>

#define DEFAULT_PORT 11111

int AcceptAndRead();

int AcceptAndRead(int sockfd, struct sockaddr_in clientAddr)
{
 	int size = sizeof(clientAddr);

    /* Wait until a client connects */
    int connd = accept(sockfd, (struct sockaddr *)&clientAddr, &size);

    /* If fails to connect, loop back up and wait for a new connection */
    if (connd == -1){
        printf("failed to accept the connection..\n");
    }
    /* If it connects, read in and reply to the client */
    else{

        printf("Client connected successfully\n");

        for ( ; ; ){

            char buff[256];

            /* Clear the buffer memory for anything  possibly left over */
            bzero(&buff, sizeof(buff));
            
            /* Read the client data into our buff array */
            if (read(connd, buff, sizeof(buff)-1) > 0){
                /* Print any data the client sends to the console */
                printf("Client: %s\n", buff);

                /* Create our reply message */
                char reply[] = "I hear ya fa shizzle!\n";
                
                /* Reply back to the client */
                write(connd, reply, sizeof(reply)-1);
            }
            /* If the client disconnects break the loop */
            else
                break;
        }
    }

    /* Close the socket */
    close(connd);
    
    return 0;
}


int main()
{
    /* 
     * Creates a socket that uses an internet IP address,
     * Sets the type to be Stream based (TCP),
     * 0 means choose the default protocol.
     */

     /* Identify and access the sockets */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    int exit   = 0; 	/* 0 = false, 1 = true */

    /* If positive value, the socket is valid */
    if(sockfd < 0){
        printf("ERROR: failed to create the socket\n");
        return 1;        /* Kill the server with exit status 1 */        
    }

    /* Server and client socket address structures */
	struct sockaddr_in serverAddr, clientAddr;

	/* Initialize the server address struct to zero */
    memset((char *)&serverAddr, 0, sizeof(serverAddr)); 

    /* Fill the server's address family */
    serverAddr.sin_family	   = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port        = htons(DEFAULT_PORT);

    /* Attach the server socket to our port */
    if(bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
        printf("ERROR: failed to bind\n");
        return 1;
    }

    /* Continuously accept connections while not currently in an active connection or told to quit */
    while (exit == 0){
        /* Listen for a new connection, allow 5 pending connections */
        listen(sockfd, 5);
        printf("Waiting for a connection...\n");

        /* Accept client connections and read from them */
    	exit = AcceptAndRead(sockfd, clientAddr);
    }

    /* Close the open sockets */
    close(sockfd);
    return 0;

}
