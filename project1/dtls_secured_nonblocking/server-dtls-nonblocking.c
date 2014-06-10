/* server-dtls.c 
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  
 * USA
 *=============================================================================
 *
 * Bare-bones example of a DTLS erver for instructional/learning purposes.
 * Utilizes DTLS 1.2.
 */

#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <cyassl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#define SERV_PORT   11111           /* define our server port number */
#define MSGLEN      4096

static int cleanup;                 /* To handle shutdown */
CYASSL_CTX* ctx;

void AwaitDGram();                  /* Separate out Handling Datagrams */
void sig_handler(const int sig);    /* handles ctrl+c termination */
void NonBlockingSSL_Accept(CYASSL*);/* non-blocking accept */
void dtls_set_nonblocking(int*);    /* set the socket non-blocking */
int udp_read_connect(int);          /* broken out to improve readability */
int dtls_select();

/* costumes for select_ret to wear */
enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};

void sig_handler(const int sig) 
{
    printf("\nSIGINT handled.\n");
    cleanup = 1;
    CyaSSL_CTX_free(ctx);
    CyaSSL_Cleanup();
    exit(0);
}

void AwaitDGram()
{
    int     on = 1;
    int     res = 1;
    int     recvLen;                 /* length of string read */
    int     readWriteErr; 
    int     listenfd = 0;            /* Initialize our socket */
    int     clientfd = 0;            /* client connection */
    int     currTimeout = 1;
    int     len = sizeof(on);
    char    buff[MSGLEN];                /* string read from client */
    CYASSL* ssl = NULL;              /* Initialize ssl object */
    struct sockaddr_in servAddr;     /* our server's address */
    char    ack[] = "I hear you fashizzle\n";


    while (cleanup != 1) {

        /* Create a UDP/IP socket */
        if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            printf("Cannot create socket.\n");
            cleanup = 1;
        }
        
        printf("Socket allocated\n");
//        memset((char *)&servAddr, 0, sizeof(servAddr));

        /* host-to-network-long conversion (htonl) */
        /* host-to-network-short conversion (htons) */
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servAddr.sin_port = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (res < 0) {
            printf("Setsockopt SO_REUSEADDR failed.\n");
            cleanup = 1;
        }

        /*Bind Socket*/
        if (bind(listenfd, 
                    (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
            printf("Bind failed.\n");
            cleanup = 1;
        }

        printf("Awaiting client connection on port %d\n", SERV_PORT);

        clientfd = udp_read_connect(listenfd);
        dtls_set_nonblocking(&clientfd);

        /* Create the CYASSL Object */
        if (( ssl = CyaSSL_new(ctx)) == NULL) {
            printf("CyaSSL_new error.\n");
            cleanup = 1;
        }

        /* set clilen to |cliAddr| */
        printf("Connected!\n");

        /* set the/ session ssl to client connection port */
        CyaSSL_set_fd(ssl, clientfd);

        CyaSSL_set_using_nonblock(ssl, 1);
        NonBlockingSSL_Accept(ssl);

        /* Begin: Reply to the client */
        recvLen = CyaSSL_read(ssl, buff, sizeof(buff)-1);

        currTimeout = CyaSSL_dtls_get_current_timeout(ssl);

        readWriteErr = CyaSSL_get_error(ssl, 0);       
        do {
            if (cleanup == 1) {
                memset(buff, 0, sizeof(buff));
                break;
            }
            if (recvLen < 0) {
                readWriteErr = CyaSSL_get_error(ssl, 0);
                if (readWriteErr != SSL_ERROR_WANT_READ) {
                    printf("Read Error, error was: %d.\n", readWriteErr);
                    cleanup = 1;
                } else {
                    recvLen = CyaSSL_read(ssl, buff, sizeof(buff)-1);
                }
            }
        } while (readWriteErr == SSL_ERROR_WANT_READ && recvLen < 0 && 
                currTimeout >= 0 && cleanup != 1);

        if (recvLen > 0) {
            buff[recvLen] = 0;
            printf("I heard this:\"%s\"\n", buff);
        } 
        else {
            printf("Connection Timed Out.\n");
        }

        if (CyaSSL_write(ssl, ack, sizeof(ack)) < 0) {
            printf("Write error.\n");
            cleanup = 1;
        }
        printf("Reply sent:\"%s\"\n", ack);

        /* free allocated memory */
        memset(buff, 0, sizeof(buff));
        CyaSSL_free(ssl);

        /* End: Reply to the Client */
    }
}

int udp_read_connect(int listenfd)
{
    int connfd;
    unsigned char  b[1500];
    struct sockaddr_in cliAddr;
    socklen_t clilen = sizeof(cliAddr);

    /* ensure b is empty upon each call */
    memset(b, 0, sizeof(b));

    connfd = (int)recvfrom(listenfd, (char*)b, sizeof(b), MSG_PEEK,
            (struct sockaddr*)&cliAddr, &clilen);
    if (connfd > 0) {
        if (connect(listenfd, (const struct sockaddr*)&cliAddr, 
                    sizeof(cliAddr)) != 0) {
            printf("udp connect failed.\n");
        }
    }
    else {
        printf("recvfrom failed.\n");
    }
    return listenfd;
}

void NonBlockingSSL_Accept(CYASSL* ssl)
{
    int select_ret;
    int currTimeout = 1;
    int ret = CyaSSL_accept(ssl);
    int error = CyaSSL_get_error(ssl, 0);
    int listenfd = (int)CyaSSL_get_fd(ssl);

    while (cleanup != 1 && (ret != SSL_SUCCESS && 
                (error == SSL_ERROR_WANT_READ ||
                 error == SSL_ERROR_WANT_WRITE))) {
        if (cleanup == 1) {
            CyaSSL_free(ssl);
            CyaSSL_shutdown(ssl);
            break;
        }

        if (error == SSL_ERROR_WANT_READ)
            printf("... server would read block\n");
        else
            printf("... server would write block\n");

        currTimeout = CyaSSL_dtls_get_current_timeout(ssl);
        select_ret = dtls_select(listenfd, currTimeout);

        if ((select_ret == TEST_RECV_READY) ||
                (select_ret == TEST_ERROR_READY)) {
            ret = CyaSSL_accept(ssl);
            error = CyaSSL_get_error(ssl, 0);
        }
        else if (select_ret == TEST_TIMEOUT && !CyaSSL_dtls(ssl)) {
            error = SSL_ERROR_WANT_READ;
        }
        else if (select_ret == TEST_TIMEOUT && CyaSSL_dtls(ssl) &&
                CyaSSL_dtls_got_timeout(ssl) >= 0) {
            error = SSL_ERROR_WANT_READ;
        }
        else {
            error = SSL_FATAL_ERROR;
        }
    }
    if (ret != SSL_SUCCESS) {
        printf("SSL_accept failed.\n");
    }
}

void dtls_set_nonblocking(int* listenfd)
{
    int flags = fcntl(*listenfd, F_GETFL, 0);
    if (flags < 0) {
        printf("fcntl get failed");
        cleanup = 1;
    }            
    flags = fcntl(*listenfd, F_SETFL, flags | O_NONBLOCK);            
    if (flags < 0) {
        printf("fcntl set failed.\n");
        cleanup = 1;
    }
}

int dtls_select(int socketfd, int toSec)
{
    int result;
    int nfds = socketfd + 1;
    fd_set  recvfds, errfds;
    struct timeval timeout = { (toSec > 0) ? toSec : 0, 0};

    FD_ZERO(&recvfds);
    FD_SET(socketfd, &recvfds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    result = select(nfds, &recvfds, NULL, &errfds, &timeout);

    if (result == 0)
        return TEST_TIMEOUT;
    else if (result > 0) {
        if (FD_ISSET(socketfd, &recvfds))
            return TEST_RECV_READY;
        else if(FD_ISSET(socketfd, &errfds))
            return TEST_ERROR_READY;
    }

    return TEST_SELECT_FAIL;
}

int main(int argc, char** argv)
{
    /* structures for signal handling */
    struct sigaction    act, oact;

    /* Define a signal handler for when the user closes the program
     * with Ctrl-C. Also, turn off SA_RESTART so that the OS doesn't 
     * restart the call to accept() after the signal is handled. 
     */
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);

    /* CyaSSL_Debugging_ON(); */

    /* Initialize CyaSSL */
    CyaSSL_Init();

    /* Set ctx to DTLS 1.2 */
    if ((ctx = CyaSSL_CTX_new(CyaDTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "CyaSSL_CTX_new error.\n");
        exit(EXIT_FAILURE);
    }
    /* Load CA certificates */
    if (CyaSSL_CTX_load_verify_locations(ctx,"../certs/ca-cert.pem",0) != 
            SSL_SUCCESS) {
        fprintf(stderr, "Error loading ../certs/ca-cert.pem, "
                "please check the file.\n");
        exit(EXIT_FAILURE);
    }
    /* Load server certificates */
    if (CyaSSL_CTX_use_certificate_file(ctx,"../certs/server-cert.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading ../certs/server-cert.pem, "
                "please check the file.\n");
        exit(EXIT_FAILURE);
    }
    /* Load server Keys */
    if (CyaSSL_CTX_use_PrivateKey_file(ctx,"../certs/server-key.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading ../certs/server-key.pem, "
                "please check the file.\n");
        exit(EXIT_FAILURE);
    }

    AwaitDGram();
    if (cleanup == 1) {
        CyaSSL_CTX_free(ctx);
        CyaSSL_Cleanup();
    }
    return 0;
}
