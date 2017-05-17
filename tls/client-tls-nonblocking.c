/* client-tls-nonblocking.c
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
#include    <wolfssl/ssl.h>          /* wolfSSL security library */
#include    <fcntl.h>               /* nonblocking I/O library */
#include    <sys/select.h>

#define MAXDATASIZE  4096           /* maximum acceptable amount of data */
#define SERV_PORT    11111          /* define default port number */

const char* cert = "../certs/ca-cert.pem";

/*
 * enum used for tcp_select function 
 */
enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};

static inline int tcp_select(int socketfd, int to_sec)
{
    fd_set recvfds, errfds;
    int nfds = socketfd + 1;
    struct timeval timeout = { (to_sec > 0) ? to_sec : 0, 0};
    int result;

    FD_ZERO(&recvfds);
    FD_SET(socketfd, &recvfds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    result = select(nfds, &recvfds, NULL, &errfds, &timeout);

    if (result == 0) {
        return TEST_TIMEOUT;
    }
    else if (result > 0) {
        if (FD_ISSET(socketfd, &recvfds)) {
            return TEST_RECV_READY;
        }
        else if(FD_ISSET(socketfd, &errfds)) {
            return TEST_ERROR_READY;
        }
    }

    return TEST_SELECT_FAIL;
}
int NonBlockConnect(WOLFSSL* ssl)
{
    int ret = wolfSSL_connect(ssl);

    int error = wolfSSL_get_error(ssl, 0);
    int sockfd = (int)wolfSSL_get_fd(ssl);
    int select_ret;

    while (ret != SSL_SUCCESS && (error == SSL_ERROR_WANT_READ ||
                                  error == SSL_ERROR_WANT_WRITE)) {
        int currTimeout = 1;

        if (error == SSL_ERROR_WANT_READ) {
            printf("... server would read block\n");
        }
        else {
            printf("... server would write block\n");
        }

        select_ret = tcp_select(sockfd, currTimeout);

        if ((select_ret == TEST_RECV_READY) ||
                (select_ret == TEST_ERROR_READY)) {
            ret = wolfSSL_connect(ssl);
            error = wolfSSL_get_error(ssl, 0);
        }
        else if (select_ret == TEST_TIMEOUT) {
            error = SSL_ERROR_WANT_READ;
        }
        else {
            error = SSL_FATAL_ERROR;
        }
    }
    if (ret != SSL_SUCCESS){
        printf("SSL_connect failed\n");
        exit(0);
    }
    return ret;
}

/* 
 * clients initial contact with server. (socket to connect, security layer)
 */
int ClientGreet(WOLFSSL* ssl)
{
    /* data to send to the server, data recieved from the server */
    char sendBuff[MAXDATASIZE], rcvBuff[MAXDATASIZE] = {0};
    int ret = 0;
int count = 0;

    printf("Message for server:\t");
    fgets(sendBuff, MAXDATASIZE, stdin);

    if (wolfSSL_write(ssl, sendBuff, strlen(sendBuff)) != strlen(sendBuff)) {
        /* the message is not able to send, or error trying */
        ret = wolfSSL_get_error(ssl, 0);
        printf("Write error: Error: %d\n", ret);
        return EXIT_FAILURE;
    }

    ret = wolfSSL_read(ssl, rcvBuff, MAXDATASIZE);   
    if (ret <= 0) {
        /* the server failed to send data, or error trying */
        ret = wolfSSL_get_error(ssl, 0);
        while (ret == SSL_ERROR_WANT_READ) {
count++;
            ret = wolfSSL_read(ssl, rcvBuff, MAXDATASIZE);
            ret = wolfSSL_get_error(ssl, 0);
        }
printf("counter %d\n", count);
        if (ret < 0) {
            ret = wolfSSL_get_error(ssl, 0);
            printf("Read error. Error: %d\n", ret);
            return EXIT_FAILURE;
        }
    }
    printf("Recieved: \t%s\n", rcvBuff);

    return ret;
}

/* 
 * applies TLS 1.2 security layer to data being sent.
 */
int Security(int sock)
{
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;    /* create WOLFSSL object */ 
    int         ret = 0;

    wolfSSL_Init();      /* initialize wolfSSL */

    /* create and initiLize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        printf("SSL_CTX_new error.\n");
        return EXIT_FAILURE;
    }

    /* load CA certificates into wolfSSL_CTX. which will verify the server */
    if (wolfSSL_CTX_load_verify_locations(ctx, cert, 0) != 
            SSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", cert);
        return EXIT_FAILURE;
    }

    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        return EXIT_FAILURE;
    }

    wolfSSL_set_fd(ssl, sock);
    wolfSSL_set_using_nonblock(ssl, 1);
    ret = NonBlockConnect(ssl);
    if (ret == SSL_SUCCESS) {
        ret = ClientGreet(ssl);
    }
    /* frees all data before client termination */
    wolfSSL_free(ssl);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    
    return ret;
}

/* 
 * Command line argumentCount and argumentValues 
 */
int main(int argc, char** argv) 
{
    int     sockfd;                         /* socket file descriptor */
    struct  sockaddr_in servAddr;           /* struct for server address */
    int ret = 10;                           /* variable for error checks */

    if (argc != 2) {
        /* if the number of arguments is not two, error */
        printf("usage: ./client-tcp  <IP address>\n");
        return EXIT_FAILURE;
    }

    /* internet address family, stream based tcp, default protocol */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ret = errno;
        printf("Failed to create socket. Error: %i\n", ret);
        return EXIT_FAILURE;
    }
    
    fcntl(sockfd, F_SETFL, O_NONBLOCK);     /* sets socket to non-blocking */
    memset(&servAddr, 0, sizeof(servAddr)); /* clears memory block for use */
    servAddr.sin_family = AF_INET;          /* sets addressfamily to internet*/
    servAddr.sin_port = htons(SERV_PORT);   /* sets port to defined port */  

    /* looks for the server at the entered address (ip in the command line) */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) < 1) {
        /* checks validity of address */
        ret = errno;
        printf("Invalid Address. Error: %i\n", ret);
        return EXIT_FAILURE;
    }

    /* keeps trying to connect to the socket until it is able to do so */
    while (ret != 0) 
        ret = connect(sockfd, (struct sockaddr *) &servAddr, 
            sizeof(servAddr)); 

    Security(sockfd);

    return ret;
}
