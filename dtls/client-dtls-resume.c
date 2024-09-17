/*
 * client-dtls-resume.c
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

#define MAXLINE   4096
#define SERV_PORT 11111

static int new_udp_client_socket(WOLFSSL * ssl, const char * host);
static int talk_to_server(WOLFSSL * ssl, const char * msg);

int
main(int    argc,
     char * argv[])
{
    /* standard variables used in a dtls client*/
    int               sockfd = 0;
    int               err1;
    const char *      host = argv[1];
    WOLFSSL *         ssl = NULL; /* The ssl for original connection. */
    WOLFSSL *         ssl_res = NULL; /* The ssl for resuming connection. */
    WOLFSSL_CTX *     ctx = NULL;
    WOLFSSL_SESSION * session = NULL;
    char              cert_array[] = "../certs/ca-cert.pem";
    char              buffer[80];
    char *            certs = cert_array;
    int               ret = 0;

    /* variables used in a dtls client for session reuse*/
    if (argc != 2) {
        printf("usage: udpcli <IP address>\n");
        return EXIT_FAILURE;
    }

    wolfSSL_Init();

    /* Un-comment the following line to enable debugging */
    /* wolfSSL_Debugging_ON(); */

    if ( (ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return EXIT_FAILURE;
    }

    if (wolfSSL_CTX_load_verify_locations(ctx, certs, 0) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", certs);
        return EXIT_FAILURE;
    }

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("error: wolfSSL_new failed\n");
        return EXIT_FAILURE;
    }

    sockfd = new_udp_client_socket(ssl, host);

    if (sockfd <= 0) {
        printf("error: new_udp_client_socket failed\n");
        return EXIT_FAILURE;
    }

    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        err1 = wolfSSL_get_error(ssl, 0);
        memset(buffer, 0, 80);
        printf("err = %d, %s\n", err1, wolfSSL_ERR_error_string(err1, buffer));
        printf("SSL_connect failed");
        return EXIT_FAILURE;
    }

    /* Save the session */
    session = wolfSSL_get1_session(ssl);

    if (session == NULL) {
        printf("error: get session failed\n");
        return EXIT_FAILURE;
    }

    printf("info: saved session: %p\n", session);

    ret = talk_to_server(ssl, "first client message");

    if (ret) {
        return EXIT_FAILURE;
    }

    /* Close the socket */
    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(sockfd);

    ssl = NULL;
    sockfd = 0;

    /* Make a new WOLFSSL. */
    ssl_res = wolfSSL_new(ctx);
    if (ssl_res == NULL) {
        printf("error: wolfSSL_new failed\n");
        return EXIT_FAILURE;
    }

    /* Set up to resume the session */
    ret = wolfSSL_set_session(ssl_res, session);

    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "error: wolfSSL_set_session returned: %d\n", ret);
        return EXIT_FAILURE;
    }

    /* Open a new udp socket. */
    sockfd = new_udp_client_socket(ssl_res, host);

    if (sockfd <= 0) {
        printf("error: new_udp_client_socket failed\n");
        return EXIT_FAILURE;
    }

    /* Test if the resume was successful */
    if (wolfSSL_session_reused(ssl_res)) {
        printf("info: session ID reused; Successful resume\n");
    }
    else {
        printf("info: session ID not reused\n");
    }

    ret = talk_to_server(ssl_res, "client message after resume");

    if (ret) {
        return EXIT_FAILURE;
    }

    /* Cleanup memory used for storing the session information */
    wolfSSL_shutdown(ssl_res);
    wolfSSL_free(ssl_res);
    wolfSSL_SESSION_free(session);

    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    ssl_res = NULL;
    session = NULL;
    sockfd = 0;

    return 0;
}

/* Given an ssl structure and host, open a new udp
 * client socket and set it and the server address
 * to the ssl.
 **/
static int
new_udp_client_socket(WOLFSSL *    ssl,
                      const char * host)
{
    struct sockaddr_in  servAddr;
    int                 sockfd = 0;
    int                 ret = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd <= 0) {
        int errsave = errno;
        printf("error: socket returned %d\n", errsave);
        return -1;
    }

    /* servAddr setup */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);

    ret = inet_pton(AF_INET, host, &servAddr.sin_addr);

    if (ret != 1) {
        printf("error: inet_pton %s returned %d\n", host, ret);
        close(sockfd);
        sockfd = 0;
        return -1;
    }

    ret = wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));

    if (ret != SSL_SUCCESS) {
        printf("error: wolfSSL_dtls_set_peer returned %d\n", ret);
        close(sockfd);
        sockfd = 0;
        return -1;
    }

    wolfSSL_set_fd(ssl, sockfd);

    printf("info: opened socket: %d\n", sockfd);

    return sockfd;
}

/* Send a message to the server.
 **/
static int
talk_to_server(WOLFSSL *    ssl,
               const char * send_msg)
{
    char    recv_msg[MAXLINE];
    int     recv_len;

    memset(recv_msg, 0, sizeof(recv_msg));

    /* Attempt to send send_msg to the server */
    if ( ( wolfSSL_write(ssl, send_msg, strlen(send_msg))) !=
            strlen(send_msg) ) {
        printf("Error: wolfSSL_write failed.\n");
        return -1;
    }

    /* Attempt to read a message from server and store it in recv_msg */
    recv_len = wolfSSL_read(ssl, recv_msg, sizeof(recv_msg) - 1);

    /* Error checking wolfSSL_read */
    if (recv_len < 0) {
        int readErr = wolfSSL_get_error(ssl, 0);
        if (readErr != SSL_ERROR_WANT_READ) {
            printf("Error: wolfSSL_read failed.\n");
        }
        return -1;
    }

    recv_msg[recv_len] = '\0';
    printf("info: server response: %s", recv_msg);

    return 0;
}
