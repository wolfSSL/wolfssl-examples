/*
 * benchmark-client-udp.c
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

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define PACKET_SIZE 15360  /* 15KB = 15 * 1024 */
#define SERV_PORT 11111
#define DEFAULT_MAX_TRIES 100
#define SEND_INTERVAL_US 2000  /* 2ms = 2000 microseconds */
#define MAX_CHUNK_SIZE 1300  /* Match DTLS chunk size for fair comparison */

/* Get current time in microseconds */
static long long get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* Send data in chunks to match DTLS behavior */
static int udp_send_chunked(int sockfd, const unsigned char* buf, int len,
                            const struct sockaddr* dest_addr, socklen_t addrlen)
{
    int total_sent = 0;
    int remaining = len;
    const unsigned char* ptr = buf;

    while (remaining > 0) {
        int chunk_size;
        int sent;

        chunk_size = (remaining > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE :
            remaining;
        sent = sendto(sockfd, ptr, chunk_size, 0, dest_addr, addrlen);

        if (sent < 0) {
            return sent;
        }

        total_sent += sent;
        remaining  -= sent;
        ptr += sent;
    }

    return total_sent;
}

/* Receive data in chunks to match DTLS behavior */
static int udp_recv_chunked(int sockfd, unsigned char* buf, int len)
{
    int total_received = 0;
    int remaining = len;
    unsigned char* ptr = buf;

    while (remaining > 0) {
        int chunk_size;
        int received;

        chunk_size = (remaining > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE : remaining;
        received = recvfrom(sockfd, ptr, chunk_size, 0, NULL, NULL);

        if (received < 0) {
            return received;
        }

        /* Connection closed */
        if (received == 0) {
            break;
        }

        total_received += received;
        remaining      -= received;
        ptr += received;
    }

    return total_received;
}

int main(int argc, char** argv)
{
    int                     sockfd;
    int                     recvlen;
    int                     ret;
    struct sockaddr_in      servAddr;
    const struct sockaddr*  servAddr_in;
    socklen_t               servLen;
    unsigned char           sendBuf[PACKET_SIZE];
    unsigned char           recvBuf[PACKET_SIZE];
    int                     max_tries = DEFAULT_MAX_TRIES;
    long long               total_send_time = 0;
    long long               total_recv_time = 0;
    int                     successful_sends = 0;
    int                     successful_recvs = 0;
    int                     send_errors = 0;
    int                     recv_errors = 0;
    int                     size_mismatches = 0;
    long long               send_start, send_end, recv_start, recv_end;

    if (argc < 2 || argc > 3) {
        printf("usage: %s <IP address> [max_tries]\n", argv[0]);
        printf("  max_tries defaults to %d\n", DEFAULT_MAX_TRIES);
        return 1;
    }

    if (argc == 3) {
        max_tries = atoi(argv[2]);
        if (max_tries <= 0) {
            printf("Error: max_tries must be positive\n");
            return 1;
        }
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket");
        return 1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) <= 0) {
        printf("Error: invalid IP address\n");
        return 1;
    }

    servAddr_in = (struct sockaddr*) &servAddr;
    servLen = sizeof(servAddr);

    /* Initialize send buffer with some data */
    memset(sendBuf, 0xAA, PACKET_SIZE);

    printf("Starting UDP benchmark: %d packets of %d bytes, %d us interval\n",
           max_tries, PACKET_SIZE, SEND_INTERVAL_US);
    printf("Server: %s:%d\n\n", argv[1], SERV_PORT);

    /* Send and receive packets */
    for (int i = 0; i < max_tries; i++) {
        /* Measure send time - no printf/perror in this section */
        send_start = get_time_us();
        ret = udp_send_chunked(sockfd, sendBuf, PACKET_SIZE, servAddr_in,
            servLen);
        send_end = get_time_us();

        if (ret != PACKET_SIZE) {
            if (ret < 0) {
                send_errors++;
            } else {
                size_mismatches++;
            }
            continue;
        }
        total_send_time += (send_end - send_start);
        successful_sends++;

        /* Measure receive time - no printf/perror in this section */
        recv_start = get_time_us();
        recvlen = udp_recv_chunked(sockfd, recvBuf, PACKET_SIZE);
        recv_end = get_time_us();

        if (recvlen < 0) {
            recv_errors++;
        }
        else if (recvlen == PACKET_SIZE) {
            total_recv_time += (recv_end - recv_start);
            successful_recvs++;
        }
        else {
            size_mismatches++;
        }

        /* Wait 2ms before next send (except for the last iteration) */
        if (i < max_tries - 1) {
            usleep(SEND_INTERVAL_US);
        }
    }

    close(sockfd);

    /* Calculate and print averages - all output after benchmark */
    printf("\n=== Benchmark Results without DTLS 1.3===\n");
    printf("Total packets sent: %d\n", successful_sends);
    printf("Total packets received: %d\n", successful_recvs);

    if (send_errors > 0) {
        printf("Send errors: %d\n", send_errors);
    }
    if (recv_errors > 0) {
        printf("Receive errors: %d\n", recv_errors);
    }
    if (size_mismatches > 0) {
        printf("Size mismatches: %d\n", size_mismatches);
    }

    if (successful_sends > 0) {
        double avg_send_us = (double)total_send_time / successful_sends;
        printf("Average send time: %.3f microseconds (%.3f ms)\n",
               avg_send_us, avg_send_us / 1000.0);
    }
    else {
        printf("No successful sends to calculate average\n");
    }

    if (successful_recvs > 0) {
        double avg_recv_us = (double)total_recv_time / successful_recvs;
        printf("Average receive time: %.3f microseconds (%.3f ms)\n",
               avg_recv_us, avg_recv_us / 1000.0);
    } else {
        printf("No successful receives to calculate average\n");
    }

    return 0;
}
