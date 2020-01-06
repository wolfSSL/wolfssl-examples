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

int main (int argc, char** argv)
{
    /* standard variables used in a dtls client*/
    int                 sockfd = 0;
    int                 err1;
    int                 readErr;
    struct sockaddr_in  servAddr;
    const char*         host = argv[1];
    WOLFSSL*            ssl = 0;
    WOLFSSL_CTX*        ctx = 0;
    WOLFSSL*            sslResume = 0;
    WOLFSSL_SESSION*    session = 0;
    char*               srTest = "testing session resume";
    char                cert_array[] = "../certs/ca-cert.pem";
    char                buffer[80];
    char*               certs = cert_array;
    /* variables used in a dtls client for session reuse*/
    int     recvlen;
    char    sendLine[MAXLINE];
    char    recvLine[MAXLINE - 1];

    if (argc != 2) {
        printf("usage: udpcli <IP address>\n");
        return 1;
    }

    wolfSSL_Init();

    /* Un-comment the following line to enable debugging */
    /* wolfSSL_Debugging_ON(); */

    if ( (ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return 1;
    }

    if (wolfSSL_CTX_load_verify_locations(ctx, certs, 0) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", certs);
        return 1;
    }

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
    	printf("unable to get ssl object");
        return 1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if ( (inet_pton(AF_INET, host, &servAddr.sin_addr)) < 1) {
        printf("Error and/or invalid IP address");
        return 1;
    }

    wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       printf("cannot create a socket.");
       return 1;
    }

    wolfSSL_set_fd(ssl, sockfd);
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        err1 = wolfSSL_get_error(ssl, 0);
        memset(buffer, 0, 80);
        printf("err = %d, %s\n", err1, wolfSSL_ERR_error_string(err1, buffer));
        printf("SSL_connect failed");
        return 1;
    }

/*****************************************************************************/
/*                     Code for sending datagram to server                   */

    /* Loop while the user gives input or until an EOF is read */
    while( fgets(sendLine, MAXLINE, stdin) != NULL ) {

        /* Attempt to send sendLine to the server */
        if ( ( wolfSSL_write(ssl, sendLine, strlen(sendLine))) !=
                strlen(sendLine) ) {
            printf("Error: wolfSSL_write failed.\n");
        }

        /* Attempt to read a message from server and store it in recvLine */
        recvlen = wolfSSL_read(ssl, recvLine, sizeof(recvLine) - 1);

        /* Error checking wolfSSL_read */
        if (recvlen < 0) {
            readErr = wolfSSL_get_error(ssl, 0);
            if (readErr != SSL_ERROR_WANT_READ) {
                printf("Error: wolfSSL_read failed.\n");
            }
        }

        recvLine[recvlen] = '\0';
        fputs(recvLine, stdout);
    }
/*                                                                           */
/*****************************************************************************/

    /* Keep track of the old session information */
    wolfSSL_write(ssl, srTest, sizeof(srTest));
    session = wolfSSL_get_session(ssl);
    sslResume = wolfSSL_new(ctx);

    /* Cleanup the memory used by the old session & ssl object */
    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(sockfd);

    /* Perform setup with new variables/old session information */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if ( (inet_pton(AF_INET, host, &servAddr.sin_addr)) < 1) {
        printf("Error and/or invalid IP address");
        return 1;
    }

    wolfSSL_dtls_set_peer(sslResume, &servAddr, sizeof(servAddr));

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("cannot create a socket.");
        return 1;
    }

    wolfSSL_set_fd(sslResume, sockfd);

    /* New method call - specifies to the WOLFSSL object to use the  *
     * given WOLFSSL_SESSION object                                  */
    wolfSSL_set_session(sslResume, session);

    wolfSSL_set_fd(sslResume, sockfd);
    if (wolfSSL_connect(sslResume) != SSL_SUCCESS) {
        err1 = wolfSSL_get_error(sslResume, 0);
        memset(buffer, 0, 80);
        printf("err = %d, %s\n", err1, wolfSSL_ERR_error_string(err1, buffer));
        printf("SSL_connect failed on session reuse\n");
        return 1;
    }

    if (wolfSSL_session_reused(sslResume)) {
    	printf("reused session id\n");
    }
    else {
    	printf("didn't reuse session id!!!\n");
    }

/*****************************************************************************/
/*                     Code for sending datagram to server                   */
    /* Clear out variables for reuse */
    recvlen = 0;
    memset(sendLine, 0, MAXLINE);
    memset(recvLine, 0, MAXLINE - 1);

    /* Loop while the user gives input or until an EOF is read */
    while( fgets(sendLine, MAXLINE, stdin) != NULL ) {

        /* Attempt to send sendLine to the server */
        if ( ( wolfSSL_write(ssl, sendLine, strlen(sendLine))) !=
                strlen(sendLine) ) {
            printf("Error: wolfSSL_write failed.\n");
        }

        /* Attempt to read a message from server and store it in recvLine */
        recvlen = wolfSSL_read(ssl, recvLine, sizeof(recvLine) - 1);

        /* Error checking wolfSSL_read */
        if (recvlen < 0) {
            readErr = wolfSSL_get_error(ssl, 0);
            if (readErr != SSL_ERROR_WANT_READ) {
                printf("Error: wolfSSL_read failed.\n");
            }
        }

        recvLine[recvlen] = '\0';
        fputs(recvLine, stdout);
    }
/*                                                                           */
/*****************************************************************************/

    wolfSSL_write(sslResume, srTest, sizeof(srTest));

    /* Cleanup memory used for storing the session information */
    wolfSSL_shutdown(sslResume);
    wolfSSL_free(sslResume);

    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return 0;
}
