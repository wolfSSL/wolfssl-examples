/* client-dtls-callback.c
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
 * Utilizes DTLS 1.2 and custom IO callbacks.
 */

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#define SERV_PORT   11111           /* define our server port number */
#define MSGLEN      4096
#define DTLS_MTU    1500

static int cleanup = 0;                /* To handle shutdown */
static void sig_handler(const int sig)
{
    printf("\nSIGINT %d handled\n", sig);
    cleanup = 1;
    return;
}

typedef struct SharedDtls {
    WOLFSSL*           ssl;           /* WOLFSSL object being shared */
    int                sd;            /* socket fd */
    struct sockaddr_in servAddr;      /* server sockaddr */
    socklen_t          servSz;        /* length of servAddr */
} SharedDtls;


int my_IORecv(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    SharedDtls* shared = (SharedDtls*)ctx;
    int recvd;
    struct sockaddr addr;
    socklen_t addrSz = sizeof(addr);

    printf("my_IORecv fd %d, buf %d\n", shared->sd, sz);

    /* Receive datagram */
    recvd = recvfrom(shared->sd, buff, sz, 0, &addr, &addrSz);
    if (recvd == -1) {
        /* error encountered. Be responsible and report it in wolfSSL terms */

        fprintf(stderr, "IO RECEIVE ERROR: %d\n", errno);
        switch (errno) {
        #if EAGAIN != EWOULDBLOCK
        case EAGAIN: /* EAGAIN == EWOULDBLOCK on some systems, but not others */
        #endif
        case EWOULDBLOCK:
            if (!wolfSSL_dtls(ssl) || wolfSSL_get_using_nonblock(ssl)) {
                fprintf(stderr, "would block\n");
                return WOLFSSL_CBIO_ERR_WANT_READ;
            }
            else {
                fprintf(stderr, "socket timeout\n");
                return WOLFSSL_CBIO_ERR_TIMEOUT;
            }
        case ECONNRESET:
            fprintf(stderr, "connection reset\n");
            return WOLFSSL_CBIO_ERR_CONN_RST;
        case EINTR:
            fprintf(stderr, "socket interrupted\n");
            cleanup = 1;
            return WOLFSSL_CBIO_ERR_GENERAL;
            /* NOTE: Don't return WOLFSSL_CBIO_ERR_ISR. It keeps trying to recv */
        case ECONNREFUSED:
            fprintf(stderr, "connection refused\n");
            return WOLFSSL_CBIO_ERR_WANT_READ;
        case ECONNABORTED:
            fprintf(stderr, "connection aborted\n");
            return WOLFSSL_CBIO_ERR_CONN_CLOSE;
        default:
            fprintf(stderr, "general error\n");
            return WOLFSSL_CBIO_ERR_GENERAL;
        }
    }
    else if (recvd == 0) {
        printf("Connection closed\n");
        return WOLFSSL_CBIO_ERR_CONN_CLOSE;
    }

    /* successful receive */
    printf("my_IORecv: received %d bytes from %d\n", recvd, shared->sd);
    return recvd;
}


int my_IOSend(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    SharedDtls* shared = (SharedDtls*)ctx;
    int sent;
    const struct sockaddr* addr = NULL;
    socklen_t addrSz = 0;

    if (shared) {
        addr = (const struct sockaddr*)&shared->servAddr;
        addrSz = shared->servSz;
    }

    printf("my_IOSend fd %d, buf %d\n", shared->sd, sz);

    /* Send datagram */
    sent = sendto(shared->sd, buff, sz, 0, addr, addrSz);
    if (sent == -1) {
        /* error encountered. Be responsible and report it in wolfSSL terms */

        fprintf(stderr, "IO SEND ERROR: %d\n", errno);
        switch (errno) {
        #if EAGAIN != EWOULDBLOCK
        case EAGAIN: /* EAGAIN == EWOULDBLOCK on some systems, but not others */
        #endif
        case EWOULDBLOCK:
            fprintf(stderr, "would block\n");
            return WOLFSSL_CBIO_ERR_WANT_READ;
        case ECONNRESET:
            fprintf(stderr, "connection reset\n");
            return WOLFSSL_CBIO_ERR_CONN_RST;
        case EINTR:
            fprintf(stderr, "socket interrupted\n");
            cleanup = 1;
            return WOLFSSL_CBIO_ERR_GENERAL;
            /* NOTE: Don't return WOLFSSL_CBIO_ERR_ISR. It keeps trying to send */
        case EPIPE:
            fprintf(stderr, "socket EPIPE\n");
            return WOLFSSL_CBIO_ERR_CONN_CLOSE;
        default:
            fprintf(stderr, "general error\n");
            return WOLFSSL_CBIO_ERR_GENERAL;
        }
    }
    else if (sent == 0) {
        printf("Connection closed\n");
        return 0;
    }

    /* successful send */
    printf("my_IOSend: sent %d bytes to %d\n", sent, shared->sd);
    return sent;
}


int main (int argc, char** argv)
{
    /* standard variables used in a dtls client */
    int           ret = 0, err;
    int           sockfd = -1;
    WOLFSSL*      ssl = NULL;
    WOLFSSL_CTX*  ctx = NULL;
    const char*   ca_cert  = "../certs/ca-cert.pem";
    char          buff[MSGLEN];
    int           buffLen;
    SharedDtls    shared;

    /* Program argument checking */
    if (argc != 2) {
        printf("usage: udpcli <IP address>\n");
        return 1;
    }

    /* Code for handling signals */
    struct sigaction act, oact;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);

    wolfSSL_Debugging_ON();

    /* Initialize wolfSSL before assigning ctx */
    wolfSSL_Init();


    if ( (ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        goto exit;
    }

    /* Register callbacks */
    wolfSSL_CTX_SetIORecv(ctx, my_IORecv);
    wolfSSL_CTX_SetIOSend(ctx, my_IOSend);


    /* Load CA certificates into ctx variable */
    if (wolfSSL_CTX_load_verify_locations(ctx, ca_cert, 0)
	    != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", ca_cert);
        goto exit;
    }

    /* Assign ssl variable */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("unable to get ssl object");
        goto exit;
    }
    memset(&shared, 0, sizeof(shared));
    shared.ssl = ssl;


    /* servAddr setup */
    shared.servSz = sizeof(shared.servAddr);
    shared.servAddr.sin_family = AF_INET;
    shared.servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, argv[1], &shared.servAddr.sin_addr) < 1) {
        printf("Error and/or invalid IP address");
        goto exit;
    }

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       printf("cannot create a socket.");
       goto exit;
    }
    shared.sd = sockfd;

    wolfSSL_SetIOWriteCtx(ssl, &shared);
    wolfSSL_SetIOReadCtx(ssl, &shared);

    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
	    err = wolfSSL_get_error(ssl, 0);
	    printf("err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
	    printf("SSL_connect failed\n");
        goto exit;
    }

    /**************************************************************************/
    /*                  Code for sending datagram to server                   */
    if (fgets(buff, sizeof(buff), stdin) != NULL) {

        /* Send buffer to the server */
        buffLen = strlen(buff);
        if (( wolfSSL_write(ssl, buff, buffLen)) != buffLen) {
            err = wolfSSL_get_error(ssl, 0);
            if (err != SSL_ERROR_WANT_WRITE) {
                printf("err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
                printf("SSL_write failed\n");
                goto exit;
            }
        }

        /* Receive message from server */
        ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
        if (ret < 0) {
            err = wolfSSL_get_error(ssl, 0);
            if (err != SSL_ERROR_WANT_READ) {
                printf("err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
                printf("SSL_read failed\n");
                goto exit;
            }
        }
        buffLen = ret;
        ret = 0;

        /* Add a terminating character to the generic server message */
        buff[buffLen] = '\0';
        fputs(buff, stdout);
    }
    /*                End code for sending datagram to server                 */
    /**************************************************************************/

exit:
    /* Housekeeping */
    if (ssl) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (sockfd != -1) {
        close(sockfd);
    }
    if (ctx) {
        wolfSSL_CTX_free(ctx);
    }
    wolfSSL_Cleanup();

    return ret;
}
