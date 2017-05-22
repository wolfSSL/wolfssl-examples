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
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <errno.h>
#include    <arpa/inet.h>
#include    <unistd.h>              /* TCP unsecure read/write methods */

#define MAXDATASIZE  4096           /* maximum acceptable amount of data */
#define SERV_PORT    11111          /* define default port number */



int main(int argc, char** argv)
{
    int     sockfd;                         /* socket file descriptor */
    struct  sockaddr_in servAddr;           /* struct for server address */
    int     ret = 0;                        /* variable for error checking */

    /* data to send to the server, data recieved from the server */
    char    sendBuff[MAXDATASIZE], rcvBuff[MAXDATASIZE] = {0};


    /* SECURE CODE SECTION will be here, see client-tls.c for comparison */


    if (argc != 2) {
        /* if the number of arguments is not two, error */
        printf("usage: ./client-tcp  <IP address>\n");
        return EXIT_FAILURE;
    }

    /* internet address family, stream based tcp, default protocol */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        printf("Failed to create socket. errno: %i\n", errno);
        return EXIT_FAILURE;
    }

    memset(&servAddr, 0, sizeof(servAddr)); /* clears memory block for use */
    servAddr.sin_family = AF_INET;          /* sets addressfamily to internet*/
    servAddr.sin_port = htons(SERV_PORT);   /* sets port to defined port */

    /* looks for the server at the entered address (ip in the command line) */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) < 1) {
        /* checks validity of address */
        ret = errno;
        printf("Invalid Address. errno: %i\n", ret);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        ret = errno;
        printf("Connect error. Error: %i\n", ret);
        return EXIT_FAILURE;
    }

    printf("Message for server:\t");
    if (fgets(sendBuff, MAXDATASIZE, stdin) != NULL) {
        /* sendBuff is assigned in condition */
    }

    /* see client-tls.c where secure code is inserted here ...










     SECURE CODE SECTION












    end of secure code section */

    if (write(sockfd, sendBuff, strlen(sendBuff)) != strlen(sendBuff)) {
        /* the message is not able to send, or error trying */
        ret = errno;
        printf("Write error: Error: %i\n", ret);
        return EXIT_FAILURE;
    }

    if (read(sockfd, rcvBuff, MAXDATASIZE) < 0) {
        /* the server failed to send data, or error trying */
        ret = errno;
        printf("Read error. Error: %i\n", ret);
        return EXIT_FAILURE;
    }
    printf("Recieved: \t%s\n", rcvBuff);






    return ret;
}
