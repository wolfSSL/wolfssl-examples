/* server-dtls-nonblocking.c
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
 *
 *=============================================================================
 *
 * Bare-bones example of a nonblocking DTLS server for instructional/learning
 * purposes. Utilizes DTLS 1.2.
 */

#include <wolfssl/options.h>
#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <wolfssl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#define SERV_PORT   11111           /* define our server port number */
#define MSGLEN      4096

static int cleanup;                 /* To handle shutdown */

/* costumes for select_ret to wear */
enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};

int main(int argc, char** argv)
{
    /* cont short for "continue?", Loc short for "location" */
    int         cont = 0;
    char        caCertLoc[] = "../certs/ca-cert.pem";
    char        servCertLoc[] = "../certs/server-cert.pem";
    char        servKeyLoc[] = "../certs/server-key.pem";
    WOLFSSL_CTX* ctx;
    /* Await Datagram variables */
    int     on = 1;
    int     res = 1;
    int     recvLen;                 /* length of string read */
    int     readWriteErr;
    int     listenfd = 0;            /* Initialize our socket */
    int     clientfd = 0;            /* client connection */
    int     len = sizeof(on);
    char    buff[MSGLEN];            /* string read from client */
    WOLFSSL* ssl = NULL;              /* Initialize ssl object */
    struct sockaddr_in servAddr;     /* our server's address */
    char    ack[] = "I hear you fashizzle\n";
    /* DTLS set nonblocking flag */
    int flags = fcntl(*(&listenfd), F_GETFL, 0);

    /* "./config --enable-debug" and uncomment next line for debugging */
    /* wolfSSL_Debugging_ON(); */

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Set ctx to DTLS 1.2 */
    if ((ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method())) == NULL) {
        printf("wolfSSL_CTX_new error.\n");
        return 1;
    }
    /* Load CA certificates */
    if (wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0) !=
            SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        return 1;
    }
    /* Load server certificates */
    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != 
            SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        return 1;
    }
    /* Load server Keys */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc,
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return 1;
    }

    /* Await Datagram */
    cont = 0;
    while (cleanup != 1) {
        /* NonBlockingSSL_Accept variables */
        int ret;
        int select_ret;
        int currTimeout;
        int error;
        int result;
        int nfds;
        fd_set  recvfds, errfds;
        struct timeval timeout = { (currTimeout > 0) ? currTimeout : 0, 0};
        /* udp-read-connect variables */
        int bytesRecvd;
        unsigned char  b[MSGLEN];
        struct sockaddr_in cliAddr;
        socklen_t clilen = sizeof(cliAddr);

        /* Create a UDP/IP socket */
        if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            printf("Cannot create socket.\n");
            cont = 1;
        }

        printf("Socket allocated\n");

        /* DTLS set nonblocking */
        if (flags < 0) {
            printf("fcntl get failed");
            cleanup = 1;
        }
        flags = fcntl(*(&listenfd), F_SETFL, flags | O_NONBLOCK);
        if (flags < 0) {
            printf("fcntl set failed.\n");
            cleanup = 1;
        }

        memset((char *)&servAddr, 0, sizeof(servAddr));

        /* host-to-network-long conversion (htonl) */
        /* host-to-network-short conversion (htons) */
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servAddr.sin_port = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (res < 0) {
            printf("Setsockopt SO_REUSEADDR failed.\n");
            cont = 1;
        }

        /*Bind Socket*/
        if (bind(listenfd,
                    (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
            printf("Bind failed.\n");
            cont = 1;
        }

        printf("Awaiting client connection on port %d\n", SERV_PORT);

        /* UDP-read-connect */
        do {
            bytesRecvd = (int)recvfrom(listenfd, (char*)b, sizeof(b), MSG_PEEK,
                (struct sockaddr*)&cliAddr, &clilen);
        } while (bytesRecvd <= 0);

        if (bytesRecvd > 0) {
            if (connect(listenfd, (const struct sockaddr*)&cliAddr,
                        sizeof(cliAddr)) != 0) {
                printf("udp connect failed.\n");
            }
        }
        else {
            printf("recvfrom failed.\n");
        }

        printf("Connected!\n");
        /* ensure b is empty upon each call */
        memset(&b, 0, sizeof(b));
        clientfd = listenfd;

        /* Create the WOLFSSL Object */
        if (( ssl = wolfSSL_new(ctx)) == NULL) {
            printf("wolfSSL_new error.\n");
            cont = 1;
        }

        /* set clilen to |cliAddr| */
        printf("Connected!\n");

        /* set the/ session ssl to client connection port */
        wolfSSL_set_fd(ssl, clientfd);

        wolfSSL_set_using_nonblock(ssl, 1);

        /* NonBlockingSSL_Accept */
        ret = wolfSSL_accept(ssl);
        currTimeout = 1;
        error = wolfSSL_get_error(ssl, 0);
        listenfd = (int)wolfSSL_get_fd(ssl);
        nfds = listenfd + 1;
        while (cleanup != 1 && (ret != SSL_SUCCESS &&
                    (error == SSL_ERROR_WANT_READ ||
                     error == SSL_ERROR_WANT_WRITE))) {
            if (cleanup == 1) {
                wolfSSL_free(ssl);
                wolfSSL_shutdown(ssl);
                break;
            }

            if (error == SSL_ERROR_WANT_READ)
                printf("... server would read block\n");
            else
                printf("... server would write block\n");

            currTimeout = wolfSSL_dtls_get_current_timeout(ssl);

            FD_ZERO(&recvfds);
            FD_SET(listenfd, &recvfds);
            FD_ZERO(&errfds);
            FD_SET(listenfd, &errfds);

            result = select(nfds, &recvfds, NULL, &errfds, &timeout);

            if (result == 0) {
                select_ret = TEST_TIMEOUT;
            }
            else if (result > 0) {
                if (FD_ISSET(listenfd, &recvfds)) {
                    select_ret = TEST_RECV_READY;
                }
                else if(FD_ISSET(listenfd, &errfds)) {
                    select_ret = TEST_ERROR_READY;
                }
            }
            else {
                select_ret = TEST_SELECT_FAIL;
            }

            if ((select_ret == TEST_RECV_READY) ||
                (select_ret == TEST_ERROR_READY)) {
                ret = wolfSSL_accept(ssl);
                error = wolfSSL_get_error(ssl, 0);
            }
            else if (select_ret == TEST_TIMEOUT && !wolfSSL_dtls(ssl)) {
                error = SSL_ERROR_WANT_READ;
            }
            else if (select_ret == TEST_TIMEOUT && wolfSSL_dtls(ssl) &&
                    wolfSSL_dtls_got_timeout(ssl) >= 0) {
                error = SSL_ERROR_WANT_READ;
            }
            else {
                error = SSL_FATAL_ERROR;
            }
        }
        if (ret != SSL_SUCCESS) {
            printf("SSL_accept failed.\n");
            cont = 1;
        }
        else {
            cont = 0;
        }

        if (cont != 0) {
            printf("NonBlockingSSL_Accept failed.\n");
            cont = 1;
        }

        /* Begin: Reply to the client */
        recvLen = wolfSSL_read(ssl, buff, sizeof(buff)-1);

        /* Begin do-while read */
        do {
            if (cleanup == 1) {
                memset(buff, 0, sizeof(buff));
                break;
            }
            if (recvLen < 0) {
                readWriteErr = wolfSSL_get_error(ssl, 0);
                if (readWriteErr != SSL_ERROR_WANT_READ) {
                    printf("Read Error, error was: %d.\n", readWriteErr);
                    cleanup = 1;
                }
                else {
                    recvLen = wolfSSL_read(ssl, buff, sizeof(buff)-1);
                }
            }
        } while (readWriteErr == SSL_ERROR_WANT_READ &&
                                         recvLen < 0 &&
                                         cleanup != 1);
        /* End do-while read */

        if (recvLen > 0) {
            buff[recvLen] = 0;
            printf("I heard this:\"%s\"\n", buff);
        }
        else {
            printf("Connection Timed Out.\n");
        }

        /* Begin do-while write */
        do {
            if (cleanup == 1) {
                memset(&buff, 0, sizeof(buff));
                break;
            }
            readWriteErr = wolfSSL_get_error(ssl, 0);
            if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
                printf("Write error.\n");
                cleanup = 1;
            }
            printf("Reply sent:\"%s\"\n", ack);
        } while(readWriteErr == SSL_ERROR_WANT_WRITE && cleanup != 1);
        /* End do-while write */

        /* free allocated memory */
        memset(buff, 0, sizeof(buff));
        wolfSSL_free(ssl);

        /* End: Reply to the Client */
    }

    if (cont == 1) {
        wolfSSL_CTX_free(ctx);
        wolfSSL_Cleanup();
    }

    return 0;
}
