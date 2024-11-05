/*
 * client-dtls.c
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
 * Bare-bones example of a DTLS client for instructional/learning purposes.
 */

#include <wolfssl/options.h>
#include <unistd.h>
#include <wolfssl/ssl.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/time.h>

/* WARNING: This function is not portable. */
static WC_INLINE double current_time(int reset)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    (void)reset;

    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
}

#define SERV_PORT 11111

typedef struct {
    int size;       /* Size of buffer to read. */
    WOLFSSL* ssl;   /* SSL object to use. */
} threadArgs;

static int cleanup;             /* To handle shutdown */
static int minRunTime = 1;      /* Minimum run time. Default: 1 second */

/* Reader thread.
 *
 * Only started after handshake complete.
 */
void* Reader(void* openSock)
{
    threadArgs* args = (threadArgs*)openSock;
    int                recvLen = 0;                /* length of message     */
    int                msgLen = args->size;        /* the size of message   */
    unsigned char      buff[msgLen];               /* the incoming message  */
    WOLFSSL*           ssl = args->ssl;
    int                result;
    fd_set             recvfds, errfds;
    struct timeval     timeout;
    int                currTimeout;
    int                nb_sockfd;
    int                nfds;

    nb_sockfd = (int) wolfSSL_get_fd(ssl);

    printf("Reading\n");

    while (!cleanup) { 
        currTimeout = wolfSSL_dtls_get_current_timeout(ssl);
        nfds = nb_sockfd + 1;
        timeout = (struct timeval) { (currTimeout > 0) ? currTimeout : 0, 0};

        /* Setup file descriptor list to monitor. */
        FD_ZERO(&recvfds);
        FD_SET(nb_sockfd, &recvfds);
        FD_ZERO(&errfds);
        FD_SET(nb_sockfd, &errfds);

        result = select(nfds, &recvfds, NULL, &errfds, &timeout);
        if ((result > 0) && FD_ISSET(nb_sockfd, &errfds)) {
            /* Tell other thread to cleanup. */
            cleanup = 1;
            /* Finished reading. */
            break;
        }

        /* If data waiting on receive file descriptor, read it. */
        if (result > 0 && FD_ISSET(nb_sockfd, &recvfds)) {
            /* Read application data. */
            if ((recvLen = wolfSSL_read(ssl, buff, msgLen-1)) < 0) {
                /* Handle errors. */
                int readErr = wolfSSL_get_error(ssl, 0);
                if (readErr != SSL_ERROR_WANT_READ) {
                    printf("SSL_read failed.\n");
                    /* Tell other threads to cleanup. */
                    cleanup = 1;
                    break;
                }
            }
        }
    }

    return NULL;
}

void* Writer(void* openSock)
{
    threadArgs* args = (threadArgs*)openSock;
    char        ack[] = "I hear you fashizzle!\n";
    WOLFSSL*    ssl = args->ssl;
    double      start = current_time(0);

    printf("Writing\n");

    /* Keep writing while not in cleanup. */
    while (!cleanup) {
        /* Write message and check for error. */
        if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
            printf("wolfSSL_write fail.\n");
            /* Tell other threads to cleanup. */
            cleanup = 1;
            break;
        }

        /* Stop writing after a minimum run time. */
        if (current_time(0) > start + minRunTime) {
            printf("wolfSSL_write done.\n");
            cleanup = 1;
            break;
        }
    }

    return NULL;
}

int main (int argc, char** argv)
{
    /* standard variables used in a dtls client*/
    int             n = 0;
    int             sockfd = 0;
    int             err1;
    struct          sockaddr_in servAddr;
    WOLFSSL*        ssl = 0;
    WOLFSSL_CTX*    ctx = 0;
    char            cert_array[]  = "certs/ca-cert.pem";
    char*           certs = cert_array;
    threadArgs      args;
    pthread_t       threadidReader;
    pthread_t       threadidWriter;

    /* Program argument checking */
    if (argc < 2 || argc > 3) {
        printf("usage: udpcli <IP address> [<min time>]\n");
        return 1;
    }

    if (argc == 3) {
        minRunTime = atoi(argv[2]);
    }

    /* Initialize wolfSSL before assigning ctx */
    wolfSSL_Init();
    wolfSSL_Debugging_ON();
  
    /* wolfSSL_Debugging_ON(); */

    if ( (ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return 1;
    }

    /* Load certificates into ctx variable */
    if (wolfSSL_CTX_load_verify_locations(ctx, certs, 0)
            != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", certs);
        return 1;
    }

    /* Assign ssl variable */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("unable to get ssl object");
        return 1;
    }

    /* servAddr setup */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) < 1) {
        printf("Error and/or invalid IP address\n");
        return 1;
    }

    wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       printf("cannot create a socket.\n");
       return 1;
    }

    /* Set the file descriptor for ssl and connect with ssl variable */
    wolfSSL_set_fd(ssl, sockfd);
    wolfSSL_dtls_set_using_nonblock(ssl, 1);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    while (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        err1 = wolfSSL_get_error(ssl, 0);
        if (err1 != SSL_ERROR_WANT_READ && err1 != SSL_ERROR_WANT_WRITE) {
            printf("err = %d, %s\n", err1,
                wolfSSL_ERR_reason_error_string(err1));
            printf("SSL_connect failed\n");
            return 1;
        }
    }

    cleanup = 0;
    args.size = 256;
    args.ssl = ssl;

    fcntl(wolfSSL_get_fd(ssl), F_SETFL, O_NONBLOCK);

    /* Create reader and writer threads. */
    pthread_create(&threadidReader, NULL, Reader, &args);
    pthread_create(&threadidWriter, NULL, Writer, &args);

    /* Wait for read/write threads to be done. */
    pthread_join(threadidReader, NULL);
    pthread_join(threadidWriter, NULL);

    /* Housekeeping */
    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return 0;
}
