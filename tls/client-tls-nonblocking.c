/* client-tls-nonblocking.c
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

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#define DEFAULT_PORT 11111

#define CONNECT_WAIT_MS 2

#define CERT_FILE "../certs/ca-cert.pem"

enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_SEND_READY,
    TEST_ERROR_READY
};


static WC_INLINE int tcp_select(SOCKET_T socketfd, int to_sec)
{
    int rx = 1;
    fd_set fds, errfds;
    fd_set* recvfds = NULL;
    fd_set* sendfds = NULL;
    SOCKET_T nfds = socketfd + 1;
#if !defined(__INTEGRITY)
    struct timeval timeout = {(to_sec > 0) ? to_sec : 0, 0};
#else
    struct timeval timeout;
#endif
    int result;

    FD_ZERO(&fds);
    FD_SET(socketfd, &fds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    if (rx)
        recvfds = &fds;
    else
        sendfds = &fds;

#if defined(__INTEGRITY)
    timeout.tv_sec = (long long)(to_sec > 0) ? to_sec : 0, 0;
#endif
    result = select(nfds, recvfds, sendfds, &errfds, &timeout);

    if (result == 0)
        return TEST_TIMEOUT;
    else if (result > 0) {
        if (FD_ISSET(socketfd, &fds)) {
            if (rx)
                return TEST_RECV_READY;
            else
                return TEST_SEND_READY;
        }
        else if(FD_ISSET(socketfd, &errfds))
            return TEST_ERROR_READY;
    }

    return TEST_SELECT_FAIL;
}

static int NonBlockingSSL_Shutdown(WOLFSSL* ssl)
{
    int error, status = WOLFSSL_SHUTDOWN_NOT_DONE;

    do {
        error = 0;
        if (tcp_select(wolfSSL_get_fd(ssl), CONNECT_WAIT_MS) == TEST_RECV_READY){
            status = wolfSSL_shutdown(ssl);    /* bidirectional shutdown */
            if (status == WOLFSSL_SUCCESS)
                printf("Bidirectional shutdown complete\n");
            else {
                error = wolfSSL_get_error(ssl, 0);
            }
        }
    } while (status == WOLFSSL_SHUTDOWN_NOT_DONE || error == WOLFSSL_ERROR_WANT_READ ||
             error == WOLFSSL_ERROR_WANT_WRITE);

    return status;
}


int main(int argc, char** argv)
{
    int                ret;
    int                sockfd; 
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;



    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        ret = 0;
        goto end;
    }



    /* Initialize wolfSSL */
    wolfSSL_Init();



    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto end; 
    }

    /* Set the socket options to use nonblocking I/O */
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
        fprintf(stderr, "ERROR: failed to set non-blocking\n");
        ret = -1;
        goto end;
    }



    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto ctx_cleanup;
    }

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CERT_FILE, NULL))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto ctx_cleanup;
    }



    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid Address\n");
        ret = -1;
        goto ctx_cleanup;
    }


    /* Connect to the server */
    while (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr))
           == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* no error, just non-blocking. Carry on. */
            continue;
        } else if(errno == EINPROGRESS || errno == EALREADY)
            break;
        /* just keep looping until a connection is made */
        fprintf(stderr, "ERROR: failed to connect %d\n\n", errno);
        ret = -1;
        goto socket_cleanup;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto socket_cleanup;
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto cleanup;
    }
    /* Connect to wolfSSL on the server side */
    while (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
        if (wolfSSL_want_read(ssl) || wolfSSL_want_write(ssl)) {
            /* no error, just non-blocking. Carry on. */
            printf("Waiting for connection...\n");
            sleep(1); /* cut down on spam */  
            continue;
        }
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        ret = -1;
        goto cleanup;
    }



    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto cleanup;
    }
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    while (wolfSSL_write(ssl, buff, len) != len) {
        if (wolfSSL_want_write(ssl)) {
            /* no error, just non-blocking. Carry on. */
            continue;
        }
        fprintf(stderr, "ERROR: failed to write\n");
        ret = -1;
        goto cleanup;
    }


    if (NonBlockingSSL_Shutdown(ssl) != 1) {
        printf("Shutdown not complete\n");
        ret = -1;
        goto cleanup;
    }
    printf("Shutdown complete\n");

cleanup:
    wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
socket_cleanup:
    close(sockfd);          /* Close the connection to the server       */
ctx_cleanup:
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
end:
    return ret;             /* Return reporting a success               */
}
