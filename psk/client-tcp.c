
/* client-tcp.c
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
 **/

#include <sys/socket.h>	/* basic socket definitions */
#include <netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>

#define     MAXLINE     256    /* max text line length */
#define     SERV_PORT   11111

/*
 * this function will send the inputted string to the server and then
 * receive the string from the server outputing it to the terminal
 */

int main(int argc, char **argv)
{
    int sockfd, ret;
    char sendline[MAXLINE]="Hello Server"; /* string to send to the server */
    char recvline[MAXLINE]; /* string received from the server */
    struct sockaddr_in servaddr;

    /* must include an ip address or this will flag */
    if (argc != 2) {
        printf("Usage: tcpClient <IPaddress>\n");
        return -1;
    }

    /* create a stream socket using tcp,internet protocal IPv4,
     * full-duplex stream */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    /* places n zero-valued bytes in the address servaddr */
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(SERV_PORT);

    /* converts IPv4 addresses from text to binary form */
    ret = inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    if (ret != 1) {
        printf("Not a Valid network address");
        return -1;
    }

    /* attempts to make a connection on a socket */
    ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    if (ret != 0) {
        ret = -1;
        goto exit;
    }

    /* takes inputting string and outputs it to the server */
    /* write string to the server */
    if (write(sockfd, sendline, strlen(sendline)) != strlen(sendline)) {
        printf("Write Error to Server\n");
        ret = -1;
        goto exit;
    }

    /* flags if the server stopped before the client could end */
    if (read(sockfd, recvline, MAXLINE) == 0) {
        printf("Client: Server Terminated Prematurely!\n");
        ret = -1;
        goto exit;
    }

    printf("Server Message: %s\n", recvline);

    ret = 0;

exit:
    /* close socket and connection */
    close(sockfd);

    return ret;
}
