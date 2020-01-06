/*
 * client-udp.c
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
 * Bare-bones example of a UDP client for instructional/learning purposes.
 */

#include <wolfssl/options.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE   4096
#define SERV_PORT 11111

int main(int argc, char** argv)
{
    /* standard variables used in a udp client */
    int                     sockfd;
    int                     recvlen;
    struct sockaddr_in      servAddr;
    const struct sockaddr*  servAddr_in;
    socklen_t               servLen;
    char                    sendLine[MAXLINE];
    char                    recvLine[MAXLINE + 1];

    if (argc != 2) {
        printf("usage: udpcli <IP address>\n");
        return 1;
    }

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       printf("cannot create a socket.");
       return 1;
    }

    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, argv[1], &servAddr.sin_addr);

/****************************************************************************/
/*               Code for sending the datagram to the server                */
    servAddr_in = (struct sockaddr*) &servAddr;
    servLen = sizeof(servAddr);

    /* Loop while user is giving input or until EOF is read */
    while (fgets(sendLine, MAXLINE, stdin) != NULL) {

	/* Attempt to send sendLine to the server */
        if ( ( sendto(sockfd, sendLine, strlen(sendLine) - 1, 0, servAddr_in,
                        servLen)) == -1) {
            printf("Error in sending.\n");
        }

	/* Attempt to receive recvLine from the server */
        if ( (recvlen = recvfrom(sockfd, recvLine, MAXLINE, 0, NULL, NULL))
                == -1) {
            printf("Error in receiving.\n");
        }

        recvLine[recvlen] = '\0';
        fputs(recvLine, stdout);
    }
/*                                                                          */
/****************************************************************************/

    return 0;
}
