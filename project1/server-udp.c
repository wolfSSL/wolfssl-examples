/* 
 * server-udp.c 
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  
 * USA
 *=============================================================================
 *
 * Bare-bones example of a UDP server for instructional/learning purposes.
 */

#include <stdio.h>                      /* standard in/out procedures */
#include <stdlib.h>                     /* defines system calls */
#include <string.h>                     /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>                 /* used for all socket calls */
#include <netinet/in.h>                 /* used for sockaddr_in */
#include <arpa/inet.h>


#define SERV_PORT   11111               /* define our server port number */
#define MSGLEN      80                  /* limit incoming message size */

int main (int argc, char** argv) 
{
    /* CREATE THE SOCKET */

    struct sockaddr_in servaddr;        /* our server's address */
    struct sockaddr_in cliaddr;         /* the client's address */
    int sockfd;                         /* Initialize our socket */
    socklen_t addrlen = sizeof(cliaddr);/* length of address' */
    int recvlen;                        /* number of bytes recieved */
    int msgnum = 0;                     /* the messages we reveive in order */
    char buf[MSGLEN];                   /* the incoming message */


    /* create a UDP/IP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket");
        return 0;
    }


    /* INADDR_ANY = IPaddr, socket =  11111, modify SERV_PORT to change */
    memset((char *)&servaddr, 0, sizeof(servaddr));


    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */
    servaddr.sin_family 	 = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port 		 = htons(SERV_PORT);


    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        return 0;
    }


    /* loop, listen for client, print received, reply to client */
    for ( ; ; ) {
        printf("waiting for client message on port %d\n", SERV_PORT);

        recvlen = recvfrom(sockfd, buf, MSGLEN, 0, 
                (struct sockaddr *)&cliaddr, &addrlen);

        printf("heard %d bytes\n", recvlen);

        if (recvlen > 0) {
            buf[recvlen] = 0;
            printf("I heard this: \"%s\"\n", buf);
        }

        else
            printf("lost the connection to client\n");

        sprintf(buf, "Message #%d received\n", msgnum++);
        printf("reply sent \"%s\"\n", buf);

        if (sendto(sockfd, buf, strlen(buf), 0, 
                    (struct sockaddr *)&cliaddr, addrlen) < 0)
            perror("sendto");
        
        /* continues to loop, use "Ctrl+C" to terminate listening */
    }
}
