/*
 * client-dtls13.c
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
 * Bare-bones example of a DTLS 1.3 client for instructional/learning purposes.
 * This example uses blocking sockets for simplicity.
 *
 * To exit the sending loop enter "end" or Ctrl+D
 *
 * Define USE_DTLS12 to use DTLS 1.2 instead of DTLS 1.3
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

#include "dtls-common.h"

int main (int argc, char** argv)
{
    /* standard variables used in a dtls client*/
    int             n = 0;
    int             sockfd = INVALID_SOCKET;
    int             err;
    int             ret;
    int             exitVal = 1;
    struct          sockaddr_in servAddr;
    WOLFSSL*        ssl = NULL;
    WOLFSSL_CTX*    ctx = NULL;
    char            sendLine[MAXLINE];
    char            recvLine[MAXLINE - 1];

    /* Program argument checking */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <IP address>\n", argv[0]);
        return exitVal;
    }

    /* Initialize wolfSSL before assigning ctx */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return exitVal;
    }
  
    /* No-op when debugging is not compiled in */
    wolfSSL_Debugging_ON();

    if ( (ctx = wolfSSL_CTX_new(
#ifndef USE_DTLS12
            wolfDTLSv1_3_client_method()
#else
            wolfDTLSv1_2_client_method()
#endif
            )) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        goto cleanup;
    }

    /* Load certificates into ctx variable */
    if (wolfSSL_CTX_load_verify_locations(ctx, caCertLoc, 0)
	    != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", caCertLoc);
        goto cleanup;
    }

    /* Assign ssl variable */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        fprintf(stderr, "unable to get ssl object\n");
        goto cleanup;
    }

    /* servAddr setup */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) < 1) {
        perror("inet_pton()");
        goto cleanup;
    }

    if (wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr))
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_dtls_set_peer failed\n");
        goto cleanup;
    }

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
       perror("socket()");
       goto cleanup;
    }

    /* Set the file descriptor for ssl */
    if (wolfSSL_set_fd(ssl, sockfd) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "cannot set socket file descriptor\n");
        goto cleanup;
    }

    /* Perform SSL connection */
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        err = wolfSSL_get_error(ssl, 0);
        fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
        fprintf(stderr, "wolfSSL_connect failed\n");
        goto cleanup;
    }

    showConnInfo(ssl);

/*****************************************************************************/
/*                  Code for sending datagram to server                      */
    while (1) {
        if (fgets(sendLine, MAXLINE, stdin) == NULL)
            break;

        if (strncmp(sendLine, "end", 3) == 0)
            break;

        /* Send sendLine to the server */
        if (wolfSSL_write(ssl, sendLine, strlen(sendLine)) != strlen(sendLine)) {
            err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "wolfSSL_write failed\n");
            goto cleanup;
        }

        /* n is the # of bytes received */
        n = wolfSSL_read(ssl, recvLine, sizeof(recvLine)-1);

        if (n > 0) {
            /* Add a terminating character to the generic server message */
            recvLine[n] = '\0';
            printf("%s\n", recvLine);
        }
        else {
            err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "wolfSSL_read failed\n");
            goto cleanup;
        }
    }
/*                End code for sending datagram to server                    */
/*****************************************************************************/

    exitVal = 0;
cleanup:
    if (ssl != NULL) {
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
    }
    if (sockfd != INVALID_SOCKET)
        close(sockfd);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return exitVal;
}

