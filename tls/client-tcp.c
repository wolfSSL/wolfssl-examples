/* client-tcp.c
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
 */
#include    <stdio.h>
#include    <stdlib.h>                  
#include    <string.h>
#include    <errno.h>
#include    <arpa/inet.h>

#define MAXDATASIZE  4096   /* maximum acceptable amount of data */
#define SERV_PORT    11111  /* define default port number */

/*
 *  clients initial contact with server. Socket to connect to: sock
 */
int ClientGreet(int sock)
{
    /* data to send to the server, data recieved from the server */
    char    sendBuff[MAXDATASIZE], rcvBuff[MAXDATASIZE] = {0};
    int     err = 0;

    printf("Message for server:\t");
    fgets(sendBuff, MAXDATASIZE, stdin);

    if (write(sock, sendBuff, strlen(sendBuff)) != strlen(sendBuff)) {
        /* the message is not able to send, or error trying */
        err = errno;
        printf("Write error: errno: %i\n", err);
        return EXIT_FAILURE;
    }

    if (read(sock, rcvBuff, MAXDATASIZE) < 0) {
        /* the server fails to send data, or error trying */
        err = errno;
        printf("Read error. errno: %i\n", err);
        return EXIT_FAILURE;
    }
    printf("Recieved: \t%s\n", rcvBuff);
    return 0;
}
/* 
 * command line argumentCount and argumentValues 
 */
int main(int argc, char** argv) 
{
    int     sockfd;                         /* socket file descriptor */
    struct  sockaddr_in servAddr;           /* struct for server address */
    int     err;                            /* variable for error checks */

    if (argc != 2) {
        /* if the number of arguments is not two, error */
        printf("usage: ./client-tcp  <IP address>\n");
        return EXIT_FAILURE;
    }

    /* internet address family, stream based tcp, default protocol */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Failed to create socket. errono: %i\n", errno);
        return EXIT_FAILURE;
    }

    memset(&servAddr, 0, sizeof(servAddr)); /* clears memory block for use */
    servAddr.sin_family = AF_INET;          /* sets addressfamily to internet*/
    servAddr.sin_port = htons(SERV_PORT);   /* sets port to defined port */

    /* looks for the server at the entered address (ip in the command line) */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) < 1) {
        /* checks validity of address */
        err = errno;
        printf("Invalid Address. errno: %i\n", err);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        err = errno;
        printf("Connect error. errno: %i\n", err);
        return EXIT_FAILURE;
    }
    ClientGreet(sockfd);
    return 0;
}
