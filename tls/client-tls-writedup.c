/* client-tls-writedup.c
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
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


/* run this example with
 valgrind --tool=drd ./client-tls-writedup 127.0.0.1
 against our examples/echoserver/echoserver

 if wolfSSL is not built with --enable-writedup we should see threading errors
 if it is built with --enable-writedup, no errors should be present
*/


#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <errno.h>
#include    <arpa/inet.h>
#include    <pthread.h>
#include    <wolfssl/options.h>      /* wolfSSL build options */
#include    <wolfssl/ssl.h>          /* wolfSSL security library */

#define MAXDATASIZE  256            /* maximum acceptable amount of data */
#define MAXTIMES     1024*4
#define SERV_PORT    11111          /* define default port number */

const char* cert = "../certs/ca-cert.pem";
const char* eccCert = "../certs/server-ecc.pem";


static void* ReadSSL(void* args)
{
    WOLFSSL* ssl = (WOLFSSL*)args;
    /* data recieved from the server */
    char    recvBuff[MAXDATASIZE] = {0};
    int     ret = 0;                /* variable for error checking */
    int     i;

    for (i = 0; i < MAXTIMES; i++) {
        if (wolfSSL_read(ssl, recvBuff, MAXDATASIZE) < 0) {
            /* the message is not able to send, or error trying */
            ret = wolfSSL_get_error(ssl, 0);
            printf("Read error: Error: %i\n", ret);
            break;
        }
    }

    return NULL;
}


/*
 * clients initial contact with server. (socket to connect, security layer)
 */
static int WriteSSL(WOLFSSL* ssl)
{
    /* data to send to the server, data recieved from the server */
    char    sendBuff[MAXDATASIZE] = {0};
    int     ret = 0;                /* variable for error checking */
    int     i;

    for (i = 0; i < sizeof(sendBuff); i++) {
        sendBuff[i] = i+1;
    }


    for (i = 0; i < MAXTIMES; i++) {
        if (wolfSSL_write(ssl, sendBuff, MAXDATASIZE) != MAXDATASIZE) {
            /* the message is not able to send, or error trying */
            ret = wolfSSL_get_error(ssl, 0);
            printf("Write error: Error: %i\n", ret);
            return EXIT_FAILURE;
        }
    }


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
    if (wolfSSL_CTX_load_verify_locations(ctx, cert, 0) != SSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", cert);
        return EXIT_FAILURE;
    }
    /* load ECC CA into wolfSSL_CTX. which will verify the server */
    if (wolfSSL_CTX_load_verify_locations(ctx, eccCert, 0) != SSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", eccCert);
        return EXIT_FAILURE;
    }
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        return EXIT_FAILURE;
    }
    wolfSSL_set_fd(ssl, sock);

    ret = wolfSSL_connect(ssl);
    if (ret == SSL_SUCCESS) {
        /* start client thread */

#ifdef HAVE_WRITE_DUP
        WOLFSSL* write_ssl = wolfSSL_write_dup(ssl);
        if (write_ssl == NULL) {
            printf("wolfSSL_write_dup error.\n");
            return EXIT_FAILURE;
        }
#else
        #warning "client-tls-writedup not built with HAVE_WRITE_DUP"
#endif

        pthread_t tid;
        pthread_create(&tid, NULL, ReadSSL, ssl);

        ret = WriteSSL(ssl);

        pthread_join(tid, NULL);
    } else {
        printf("wolfSSL_connect err = %d\n", wolfSSL_get_error(ssl, 0));
        return EXIT_FAILURE;
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

