/*
 * benchmark-server-udp.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERV_PORT   11111
#define PACKET_SIZE 15360  /* 15KB = 15 * 1024 */
#define MAX_CHUNK_SIZE 1300  /* Match DTLS chunk size for fair comparison */

/* Send data in chunks to match DTLS behavior */
static int udp_send_chunked(int sockfd, const unsigned char* buf, int len,
                            const struct sockaddr* dest_addr, socklen_t addrlen)
{
    int total_sent = 0;
    int remaining = len;
    const unsigned char* ptr = buf;

    while (remaining > 0) {
        int chunk_size = (remaining > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE : remaining;
        int sent = sendto(sockfd, ptr, chunk_size, 0, dest_addr, addrlen);

        if (sent < 0) {
            return sent;  /* Error */
        }

        total_sent += sent;
        remaining -= sent;
        ptr += sent;
    }

    return total_sent;
}

/* Receive data in chunks to match DTLS behavior */
static int udp_recv_chunked(int sockfd, unsigned char* buf, int len,
                            struct sockaddr* src_addr, socklen_t* addrlen)
{
    int total_received = 0;
    int remaining = len;
    unsigned char* ptr = buf;

    while (remaining > 0) {
        int chunk_size = (remaining > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE : remaining;
        int received = recvfrom(sockfd, ptr, chunk_size, 0, src_addr, addrlen);

        if (received < 0) {
            return received;
        }

        if (received == 0) {
            break;  /* Connection closed */
        }

        total_received += received;
        remaining -= received;
        ptr += received;
    }

    return total_received;
}

int main(void)
{
    int           sockfd;
    int           recvLen;
    int           msgNum = 0;
    unsigned char buf[PACKET_SIZE];
    struct        sockaddr_in servAddr;
    struct        sockaddr_in cliAddr;
    socklen_t     cliAddrLen = sizeof(cliAddr);

    /* create a UDP/IP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket");
        return 1;
    }

    memset((char *)&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(SERV_PORT);

    if (bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind failed");
        return 1;
    }

    printf("UDP server listening on port %d\n", SERV_PORT);
    printf("Ready to receive %d byte packets\n", PACKET_SIZE);
    printf("Press Ctrl+C to stop\n\n");

    /* loop, listen for client, echo back to client */
    /* No printf/perror in the receive/send loop to avoid affecting timing */
    for (;;) {
        memset(buf, 0, sizeof(buf));

        /* Receive data in chunks to match DTLS behavior */
        recvLen = udp_recv_chunked(sockfd, buf, PACKET_SIZE,
                                   (struct sockaddr *)&cliAddr, &cliAddrLen);

        if (recvLen < 0) {
            /* Only log errors occasionally, not every packet */
            if (msgNum == 0 || (msgNum % 100 == 0)) {
                perror("recvfrom failed");
            }
            continue;
        }

        if (recvLen != PACKET_SIZE) {
            continue;  /* Incomplete packet */
        }

        msgNum++;

        /* Echo back the received data - do this immediately, no printf in between */
        /* Send data in chunks to match DTLS behavior */
        if (udp_send_chunked(sockfd, buf, PACKET_SIZE,
                            (struct sockaddr *)&cliAddr, cliAddrLen) < 0) {
            /* Only log errors occasionally */
            if (msgNum % 100 == 0) {
                perror("sendto failed");
            }
            /* Continue to next receive, don't exit */
        }
    }

    return 0;
}

