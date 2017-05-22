/* client-tls-ecdhe.c
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
#include    <wolfssl/options.h>
#include    <wolfssl/ssl.h>          /* wolfSSL security library */
#include    <wolfssl/test.h>

#define MAXDATASIZE  4096           /* maximum acceptable amount of data */
#define SERV_PORT    11111          /* define default port number */

const char* cert = "../certs/server-ecc.pem";

/*
 * clients initial contact with server. (socket to connect, security layer)
 */
int ClientGreet(int sock, WOLFSSL* ssl)
{
    /* data to send to the server, data recieved from the server */
    char    sendBuff[MAXDATASIZE], rcvBuff[MAXDATASIZE] = {0};
    int     ret = 0;                /* variable for error checking */

    printf("Message for server:\t");
    if((fgets(sendBuff, MAXDATASIZE, stdin)) != NULL ){
        /* sendBuff is assigned in condition */
    }

    if (wolfSSL_write(ssl, sendBuff, strlen(sendBuff)) != strlen(sendBuff)) {
        /* the message is not able to send, or error trying */
        ret = wolfSSL_get_error(ssl, 0);
        printf("Write error: Error: %i\n", ret);
        return EXIT_FAILURE;
    }

    if (wolfSSL_read(ssl, rcvBuff, MAXDATASIZE) < 0) {
        /* the server failed to send data, or error trying */
        ret = wolfSSL_get_error(ssl, 0);
        printf("Read error. Error: %i\n", ret);
        return EXIT_FAILURE;
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
    const char* myCert        = "../certs/client-ecc-cert.pem";
    const char* myKey         = "../certs/ecc-client-key.pem";
    char* cipherList          = "ECDHE-ECDSA-CHACHA20-POLY1305";
    char buffer[WOLFSSL_MAX_ERROR_SZ];

    wolfSSL_Init();      /* initialize wolfSSL */

    /* create and initiLize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        printf("SSL_CTX_new error.\n");
        return EXIT_FAILURE;
    }

    if (wolfSSL_CTX_set_cipher_list(ctx, cipherList) != SSL_SUCCESS)
            err_sys("client can't set cipher list 1");

    if (wolfSSL_CTX_use_certificate_chain_file(ctx, myCert)
                                                               != SSL_SUCCESS)
            err_sys("can't load client cert file, check file and run from"
                    " wolfSSL home dir");

    if (wolfSSL_CTX_use_PrivateKey_file(ctx, myKey, SSL_FILETYPE_PEM)
                                         != SSL_SUCCESS)
            err_sys("can't load client private key file, check file and run "
                    "from wolfSSL home dir");

    /* load CA certificates into wolfSSL_CTX. which will verify the server */
    if (wolfSSL_CTX_load_verify_locations(ctx, cert, 0) != SSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", cert);
        return EXIT_FAILURE;
    }

    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        return EXIT_FAILURE;
    }
    wolfSSL_set_fd(ssl, sock);

    ret = wolfSSL_connect(ssl);
    if (ret == SSL_SUCCESS) {
        ret = ClientGreet(sock, ssl);
    } else {
        printf("Failure:");
        ret = wolfSSL_get_error(ssl, 0);
        printf(" ret = %d", ret);
        printf(" %s\n", wolfSSL_ERR_error_string(ret, buffer));
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
    int     ret = 0;                        /* variable for error checking */

    if (argc != 2) {
        /* if the number of arguments is not two, error */
        printf("usage: ./client-tcp  <IP address>\n");
        return EXIT_FAILURE;
    }

    /* internet address family, stream based tcp, default protocol */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        printf("Failed to create socket. Error: %i\n", errno);
        return EXIT_FAILURE;
    }

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

    if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        /* if socket fails to connect to the server*/
        ret = errno;
        printf("Connect error. Error: %i\n", ret);
        return EXIT_FAILURE;
    }
    Security(sockfd);

    return ret;
}

