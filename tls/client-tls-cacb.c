/* client-tls-cacb.c
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
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>          /* wolfSSL security library */
#include <wolfssl/wolfcrypt/types.h>

#define MAXDATASIZE  4096           /* maximum acceptable amount of data */
#define SERV_PORT    11111          /* define default port number */

const char* cert = "../certs/ca-cert.pem";

/*
 * clients initial contact with server. (socket to connect, security layer)
 */
int ClientGreet(int sock, WOLFSSL* ssl)
{
    /* data to send to the server, data received from the server */
    char    sendBuff[MAXDATASIZE], rcvBuff[MAXDATASIZE] = {0};
    int     ret = 0;                /* variable for error checking */

    printf("Message for server:\t");
    if (fgets(sendBuff, MAXDATASIZE, stdin) == NULL) {
        printf("Input error: No message for server");
        return EXIT_FAILURE;
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
    printf("Received: \t%s\n", rcvBuff);

    return ret;
}

static void CaCb(unsigned char* der, int sz, int type)
{
#ifdef OPENSSL_EXTRA
    WOLFSSL_X509* x509;
#endif

    printf("CA cache add callback: derSz = %d, type = %d\n", sz, type);

#ifdef OPENSSL_EXTRA
    /* Load DER to Cert */
    x509 = wolfSSL_X509_d2i(NULL, der, sz);
    if (x509) {
        char* altName;
        char* issuer;
        char* subject;
        unsigned char serial[32];
        int   ret;
        int   sz = sizeof(serial);

        WOLFSSL_X509_NAME* issuerName = wolfSSL_X509_get_issuer_name(x509);
        WOLFSSL_X509_NAME* subjectName = wolfSSL_X509_get_subject_name(x509);
        issuer  = wolfSSL_X509_NAME_oneline(issuerName, 0, 0);
        subject = wolfSSL_X509_NAME_oneline(subjectName, 0, 0);

        printf("\tIssuer : %s\n\tSubject: %s\n", issuer, subject);

        while ( (altName = wolfSSL_X509_get_next_altname(x509)) != NULL) {
            printf("\tAltName = %s\n", altName);
        }

        ret = wolfSSL_X509_get_serial_number(x509, serial, &sz);
        if (ret == SSL_SUCCESS) {
            int  i;
            int  strLen;
            char serialMsg[80];

            /* testsuite has multiple threads writing to stdout, get output
               message ready to write once */
            strLen = sprintf(serialMsg, "\tSerial Number");
            for (i = 0; i < sz; i++)
                sprintf(serialMsg + strLen + (i*3), ":%02x ", serial[i]);
            printf("%s\n", serialMsg);
        }

        XFREE(subject, 0, DYNAMIC_TYPE_OPENSSL);
        XFREE(issuer,  0, DYNAMIC_TYPE_OPENSSL);

        wolfSSL_X509_free(x509);
    }
#endif

    (void)der;
}

/*
 * applies TLS 1.2 security layer to data being sent.
 */
int Security(int sock)
{
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;    /* create WOLFSSL object */
    int          ret = 0;

    wolfSSL_Init();      /* initialize wolfSSL */

    /* create and initialize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        printf("SSL_CTX_new error.\n");
        ret = EXIT_FAILURE;  
        goto exit;
    }

    /* set callback for action when CA's are added */
    wolfSSL_CTX_SetCACb(ctx, CaCb);

    /* load CA certificates into wolfSSL_CTX. which will verify the server */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, cert, 0)) 
            != WOLFSSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", cert);
        goto exit;
    }
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        ret = EXIT_FAILURE; 
        goto exit;
    }
    wolfSSL_set_fd(ssl, sock);

    ret = wolfSSL_connect(ssl);
    if (ret == SSL_SUCCESS) {
        ret = ClientGreet(sock, ssl);
    }

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */

    return ret;                 /* Return reporting a success               */
}

/*
 * Command line argumentCount and argumentValues
 */
int main(int argc, char** argv)
{
    int     sockfd = SOCKET_INVALID;        /* socket file descriptor */
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
        ret = EXIT_FAILURE; 
        goto exit;
    }

    memset(&servAddr, 0, sizeof(servAddr)); /* clears memory block for use */
    servAddr.sin_family = AF_INET;          /* sets addressfamily to internet*/
    servAddr.sin_port = htons(SERV_PORT);   /* sets port to defined port */

    /* looks for the server at the entered address (ip in the command line) */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) < 1) {
        /* checks validity of address */
        ret = errno;
        printf("Invalid Address. Error: %i\n", ret);
        goto exit;
    }

    if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        /* if socket fails to connect to the server*/
        ret = errno;
        printf("Connect error. Error: %i\n", ret);
        goto exit;
    }
    Security(sockfd);

    ret = 0;

exit:
    /* Cleanup and return */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket   */

    return ret;
}
