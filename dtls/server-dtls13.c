/* server-dtls13.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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
 * Bare-bones example of a DTLS 1.3 server for instructional/learning purposes.
 * This example can only accept one connection at a time.
 *
 * Define USE_DTLS12 to use DTLS 1.2 instead of DTLS 1.3
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

#include "dtls-common.h"

WOLFSSL_CTX*  ctx = NULL;
WOLFSSL*      ssl = NULL;
int           listenfd = INVALID_SOCKET;   /* Initialize our socket */

static void sig_handler(const int sig);
static void free_resources(void);

int main(int argc, char** argv)
{
    /* Loc short for "location" */
    int           exitVal = 1;
    struct sockaddr_in servAddr;        /* our server's address */
    struct sockaddr_in cliaddr;         /* the client's address */
    int           ret;
    int           err;
    int           recvLen = 0;    /* length of message */
    socklen_t     cliLen;
    char          buff[MAXLINE];   /* the incoming message */
    char          ack[] = "I hear you fashizzle!\n";

    /* Initialize wolfSSL before assigning ctx */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init error.\n");
        return exitVal;
    }

    /* No-op when debugging is not compiled in */
    wolfSSL_Debugging_ON();

    /* Set ctx to DTLS 1.3 */
    if ((ctx = wolfSSL_CTX_new(
#ifndef USE_DTLS12
            wolfDTLSv1_3_server_method()
#else
            wolfDTLSv1_2_server_method()
#endif
            )) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        goto cleanup;
    }
    /* Load CA certificates */
    if (wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0) !=
            SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", caCertLoc);
        goto cleanup;
    }
    /* Load server certificates */
    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != 
                                                                 SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", servCertLoc);
        goto cleanup;
    }
    /* Load server Keys */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc,
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", servKeyLoc);
        goto cleanup;
    }

    /* Create a UDP/IP socket */
    if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket()");
        goto cleanup;
    }
    printf("Socket allocated\n");
    memset((char *)&servAddr, 0, sizeof(servAddr));
    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(SERV_PORT);

    /* Bind Socket */
    if (bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind()");
        goto cleanup;
    }

    signal(SIGINT, sig_handler);

    while (1) {
        printf("Awaiting client connection on port %d\n", SERV_PORT);

        cliLen = sizeof(cliaddr);
        ret = (int)recvfrom(listenfd, (char *)&buff, sizeof(buff), MSG_PEEK,
                (struct sockaddr*)&cliaddr, &cliLen);

        if (ret < 0) {
            perror("recvfrom()");
            goto cleanup;
        }
        else if (ret == 0) {
            fprintf(stderr, "recvfrom zero return\n");
            goto cleanup;
        }

        /* Create the WOLFSSL Object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "wolfSSL_new error.\n");
            goto cleanup;
        }

        if (wolfSSL_dtls_set_peer(ssl, &cliaddr, cliLen) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_dtls_set_peer error.\n");
            goto cleanup;
        }

        if (wolfSSL_set_fd(ssl, listenfd) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_set_fd error.\n");
            break;
        }

        if (wolfSSL_accept(ssl) != SSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "SSL_accept failed.\n");
            goto cleanup;
        }
        showConnInfo(ssl);
        while (1) {
            if ((recvLen = wolfSSL_read(ssl, buff, sizeof(buff)-1)) > 0) {
                printf("heard %d bytes\n", recvLen);

                buff[recvLen] = '\0';
                printf("I heard this: \"%s\"\n", buff);
            }
            else if (recvLen <= 0) {
                err = wolfSSL_get_error(ssl, 0);
                if (err == WOLFSSL_ERROR_ZERO_RETURN) /* Received shutdown */
                    break;
                fprintf(stderr, "error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
                fprintf(stderr, "SSL_read failed.\n");
                goto cleanup;
            }
            printf("Sending reply.\n");
            if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
                err = wolfSSL_get_error(ssl, 0);
                fprintf(stderr, "error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
                fprintf(stderr, "wolfSSL_write failed.\n");
                goto cleanup;
            }
        }

        printf("reply sent \"%s\"\n", ack);

        /* Attempt a full shutdown */
        ret = wolfSSL_shutdown(ssl);
        if (ret == WOLFSSL_SHUTDOWN_NOT_DONE)
            ret = wolfSSL_shutdown(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "wolfSSL_shutdown failed\n");
        }
        wolfSSL_free(ssl);
        ssl = NULL;

        printf("Awaiting new connection\n");
    }
    
    exitVal = 0;
cleanup:
    free_resources();
    wolfSSL_Cleanup();

    return exitVal;
}


static void sig_handler(const int sig)
{
    (void)sig;
    free_resources();
    wolfSSL_Cleanup();
}

static void free_resources(void)
{
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
        ssl = NULL;
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
        ctx = NULL;
    }
    if (listenfd != INVALID_SOCKET) {
        close(listenfd);
        listenfd = INVALID_SOCKET;
    }
}
