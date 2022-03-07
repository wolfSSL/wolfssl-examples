/* server-tcp.c
 * A server example using a TCP connection.
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
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAXLINE     4096
#define LISTENQ     1024
#define SERV_PORT   11111

/*
 * Fatal error detected, print out and exit.
 */
void err_sys(const char *err, ...)
{
    printf("Fatal error : %s\n", err);
}

int main()
{
    int  listenfd, connfd;
    int  opt;
    int  n;              /* length of string read */
    char buff[MAXLINE];
    char buf[MAXLINE];   /* string read from client */
    char response[22] = "I hear ya for shizzle";
    struct sockaddr_in  cliAddr, servAddr;

    socklen_t cliLen;

    /* find a socket , 0 for using TCP option */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err_sys("socket error");
    }

    /* set up server address and port */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(SERV_PORT);

    /* bind to a socket */
    opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt,
               sizeof(int));
    if (bind(listenfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        err_sys("bind error");
    }

    /* listen to the socket */
    if (listen(listenfd, LISTENQ) < 0) {
        err_sys("listen error");
        return 1;
    }

    /* main loop for accepting and responding to clients */
    for ( ; ; ) {
        cliLen = sizeof(cliAddr);
        connfd = accept(listenfd, (struct sockaddr *) &cliAddr, &cliLen);
        if (connfd < 0) {
            err_sys("accept error");
            break;
        }
        else {
            printf("Connection from %s, port %d\n",
                   inet_ntop(AF_INET, &cliAddr.sin_addr, buff, sizeof(buff)),
                   ntohs(cliAddr.sin_port));

            /* empty response buffer to avoid unexpected output */
            memset(buf, 0, MAXLINE);
            n = read(connfd, buf, MAXLINE);
            if (n > 0) {
                printf("%s\n", buf);
                if (write(connfd, response, 22) > 22) {
                    err_sys("write error");
                }
            }
            if (n < 0) {
                err_sys("respond: read error");
            }
            /* closes the connections after responding */
            if (close(connfd) == -1) {
                err_sys("close error");
                break;
            }
        }
    }
    return 0;
}

