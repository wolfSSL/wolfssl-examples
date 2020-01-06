/* server-dtls-callback.c
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
 * Bare-bones example of a DTLS server for instructional/learning purposes.
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
    struct sockaddr_in cliAddr;       /* server sockaddr */
    socklen_t          cliSz;         /* length of servAddr */
    byte               rxBuf[DTLS_MTU];
    word32             rxSz;
} SharedDtls;


int my_IORecv(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    SharedDtls* shared = (SharedDtls*)ctx;
    int recvd;
    struct sockaddr addr;
    socklen_t addrSz = sizeof(addr);

    printf("Server Recv fd %d, buf %d\n", shared->sd, sz);

    /* handle "peek" rx data */
    if (shared->rxSz > 0) {
        recvd = shared->rxSz;
        if (recvd > sz)
            recvd = sz;
        memcpy(buff, shared->rxBuf, recvd);
        shared->rxSz -= recvd;
        memcpy(shared->rxBuf, &shared->rxBuf[recvd], shared->rxSz);
    }
    else {
        /* Receive datagram */
        recvd = recvfrom(shared->sd, buff, sz, 0, &addr, &addrSz);
    }

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
        addr = (const struct sockaddr*)&shared->cliAddr;
        addrSz = shared->cliSz;
    }

    printf("Server Send fd %d, sz %d\n", shared->sd, sz);

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


int main(int argc, char** argv)
{
    int           ret = 0, err;
    const char    caCertLoc[] =   "../certs/ca-cert.pem";
    const char    servCertLoc[] = "../certs/server-cert.pem";
    const char    servKeyLoc[] =  "../certs/server-key.pem";
    WOLFSSL_CTX*  ctx = NULL;
    WOLFSSL*      ssl = NULL;
    char          buff[MSGLEN];   /* the incoming message */
    int           buffLen;
    int           listenfd = -1;   /* Initialize our socket */
    int           on;
    socklen_t     len;
    SharedDtls    shared;
    struct sockaddr_in servAddr;

    /* Code for handling signals */
    struct sigaction act, oact;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);

    /* "./config --enable-debug" and uncomment next line for debugging */
    wolfSSL_Debugging_ON();

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Set ctx to DTLS 1.2 */
    if ((ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method())) == NULL) {
        printf("wolfSSL_CTX_new error.\n");
        ret = -1;
        goto exit;
    }
    /* Load CA certificates */
    ret = wolfSSL_CTX_load_verify_locations(ctx, caCertLoc, NULL);
    if (ret != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        goto exit;
    }
    /* Load server certificates */
    ret = wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM);
    if (ret != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        goto exit;
    }
    /* Load server Keys */
    ret = wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, SSL_FILETYPE_PEM);
    if (ret != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        goto exit;
    }

    /* Register callbacks */
    wolfSSL_CTX_SetIORecv(ctx, my_IORecv);
    wolfSSL_CTX_SetIOSend(ctx, my_IOSend);

    /* Await Datagram */
    while (cleanup == 0) {
        /* Create a UDP/IP socket */
        ret = socket(AF_INET, SOCK_DGRAM, 0);
        if (ret < 0 ) {
            printf("Cannot create socket %d.\n", ret);
            goto exit;
        }
        listenfd = ret;
        printf("Socket allocated\n");

        /* setup servAddr */
        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family      = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servAddr.sin_port        = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        on = 1;
        len = sizeof(on);
        ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (ret < 0) {
            printf("Setsockopt SO_REUSEADDR failed %d.\n", errno);
            goto exit;
        }

        /* Bind Socket */
        ret = bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr));
        if (ret < 0) {
            printf("Bind failed %d.\n", errno);
            goto exit;
        }


        printf("Awaiting client connection on port %d\n", SERV_PORT);

        memset(&shared, 0, sizeof(shared));
        shared.sd = listenfd;
        shared.cliSz = sizeof(shared.cliAddr);

        ret = (int)recvfrom(listenfd, (char*)shared.rxBuf, sizeof(shared.rxBuf), 0,
                (struct sockaddr*)&shared.cliAddr, &shared.cliSz);
        if (ret < 0) {
            printf("No clients in queue, enter idle state\n");
            continue;
        }
        else if (ret > 0) {
            shared.rxSz = ret;
            ret = 0;
            printf("Connected!\n");
        }
        else {
            printf("Recvfrom failed %d.\n", errno);
            break;
        }

        /* Create the WOLFSSL Object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            printf("wolfSSL_new error.\n");
            break;
        }
        shared.ssl = ssl;

        wolfSSL_SetIOWriteCtx(ssl, &shared);
        wolfSSL_SetIOReadCtx(ssl, &shared);

        if (wolfSSL_accept(ssl) != SSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            printf("error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            printf("SSL_accept failed.\n");

            wolfSSL_shutdown(ssl);
            wolfSSL_free(ssl);
            ssl = NULL;
            close(listenfd);
            listenfd = -1;

            continue;
        }

        /* echo until shutdown received */
        while (cleanup == 0 && ret == 0) {
            buffLen = 0;
            ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
            if (ret > 0) {
                printf("heard %d bytes\n", ret);
                buffLen = ret;

                buff[buffLen] = '\0';
                printf("I heard this: \"%s\"\n", buff);
                ret = 0;
            }
            else {
                err = wolfSSL_get_error(ssl, 0);
                if (err != SSL_ERROR_WANT_READ) {
                    printf("error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
                    printf("SSL_read failed.\n");
                    break;
                }
            }

            printf("Sending echo reply\n");
            ret = wolfSSL_write(ssl, buff, buffLen);
            if (ret < 0) {
                err = wolfSSL_get_error(ssl, 0);
                if (err != SSL_ERROR_WANT_WRITE) {
                    printf("error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
                    printf("wolfSSL_write fail.\n");
                    break;
                }
            }
            ret = 0; /* success */
        }

        wolfSSL_free(ssl);
        ssl = NULL;
        close(listenfd);
        listenfd = -1;

        printf("Client done, waiting for next connection\n");
    }

exit:

    if (ssl) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (listenfd != -1) {
        close(listenfd);
    }
    if (ctx) {
        wolfSSL_CTX_free(ctx);
    }
    wolfSSL_Cleanup();

    return ret;
}

