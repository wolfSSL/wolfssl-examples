/*
 * server-udp.c
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
 *
 *=============================================================================
 *
 * Bare-bones example of a UDP server for instructional/learning purposes.
 */

#include <stdio.h>                          /* standard in/out procedures */
#include <stdlib.h>                         /* defines system calls */
#include <string.h>                         /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>                     /* used for all socket calls */
#include <netinet/in.h>                     /* used for sockaddr_in */
#include <arpa/inet.h>

#define SERV_PORT   11111                   /* define our server port number */
#define MSGLEN      4096                    /* limit incoming message size */

int main (void)
{
    int           sockfd;                   /* Initialize our socket */
    int           recvLen;                  /* number of bytes received */
    int           msgNum = 0;               /* number of msg received */
    unsigned char buf[MSGLEN];              /* the incoming message */
    struct        sockaddr_in servAddr;     /* our server's address */
    struct        sockaddr_in cliAddr;      /* the client's address */
    socklen_t cliAddrLen = sizeof(cliAddr); /* length of address' */

    /* create a UDP/IP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket");
        return 0;
    }

    memset((char *)&servAddr, 0, sizeof(servAddr));

    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(SERV_PORT);

    if (bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind failed");
        return 0;
    }

    /* loop, listen for client, print received, reply to client */
    for (;;) {
        memset(buf, 0, sizeof(buf));
        printf("waiting for client message on port %d\n", SERV_PORT);

        recvLen = recvfrom(sockfd, buf, MSGLEN, 0,
                (struct sockaddr *)&cliAddr, &cliAddrLen);

        printf("heard %d bytes\n", recvLen);

        if (recvLen > 0) {
            buf[recvLen] = 0;
            printf("I heard this: \"%s\"\n", buf);
        }
        else
            printf("lost the connection to client\n");

        printf("Message #%d received\n", msgNum++);
        printf("reply sent \"%s\"\n", buf);

        if (sendto(sockfd, buf, sizeof(buf), 0,
                    (struct sockaddr *)&cliAddr, cliAddrLen) < 0) {
            printf("\"sendto\" failed.\n");
            return 1;
        }
        /* continues to loop, use "Ctrl+C" to terminate listening */
    }
    return 0;
}

