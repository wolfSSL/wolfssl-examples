/* client-tls-resume.c
 *
 * Copyright (C) 2006-2014 wolfSSL Inc.
 *
 * This file is part of CyaSSL.
 *
 * CyaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CyaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include    <stdio.h>
#include    <unistd.h>
#include    <stdlib.h>                  
#include    <string.h>
#include    <errno.h>
#include    <arpa/inet.h>
#include    <cyassl/ssl.h>          /* CyaSSL security library */

#define MAXDATASIZE  4096           /* maximum acceptable amount of data */
#define SERV_PORT    11111          /* define default port number */

const char* cert = "../certs/ca-cert.pem";

/* 
 * clients initial contact with server. (socket to connect, security layer)
 */
int ClientGreet(int sock, CYASSL* ssl)
{
    /* data to send to the server, data recieved from the server */
    char sendBuff[MAXDATASIZE], rcvBuff[MAXDATASIZE] = {0};
    int ret = 0;

    printf("Message for server:\t");
    fgets(sendBuff, MAXDATASIZE, stdin);

    if (CyaSSL_write(ssl, sendBuff, strlen(sendBuff)) != strlen(sendBuff)) {
        /* the message is not able to send, or error trying */
        ret = CyaSSL_get_error(ssl, 0);
        printf("Write error: Error: %i\n", ret);
        return EXIT_FAILURE;
    }

    if (CyaSSL_read(ssl, rcvBuff, MAXDATASIZE) == 0) {
        /* the server failed to send data, or error trying */
        ret = CyaSSL_get_error(ssl, 0);
        printf("Read error. Error: %i\n", ret);
        return EXIT_FAILURE;
    }
    printf("Recieved: \t%s\n", rcvBuff);

    return ret;
}

/* 
 * applies TLS 1.2 security layer to data being sent.
 */
int Security(int sock, struct sockaddr_in addr)
{
    CYASSL_CTX*     ctx;        /* cyassl context */
    CYASSL*         ssl;        /* create CYASSL object */
    CYASSL_SESSION* session = 0;/* cyassl session */
    CYASSL*         sslResume;  /* create CYASSL object for connection loss */
    int             ret;

    CyaSSL_Init();              /* initialize CyaSSL (must be done first) */

    /* create and initiLize CYASSL_CTX structure */
    if ((ctx = CyaSSL_CTX_new(CyaTLSv1_2_client_method())) == NULL) {
        printf("SSL_CTX_new error.\n");
        return EXIT_FAILURE;
    }

    /* load CA certificates into CyaSSL_CTX. which will verify the server */
    if (CyaSSL_CTX_load_verify_locations(ctx, cert, 0) != SSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", cert);
        return EXIT_FAILURE;
    }

    if ((ssl = CyaSSL_new(ctx)) == NULL) {
        printf("CyaSSL_new error.\n");
        return EXIT_FAILURE;
    }

    CyaSSL_set_fd(ssl, sock);
    
    /* connects to CyaSSL */
    ret = CyaSSL_connect(ssl);
    if (ret != SSL_SUCCESS) {
        return ret;
    }
    
    ret = ClientGreet(sock, ssl);
    
    /* saves the session */
    session = CyaSSL_get_session(ssl);
    CyaSSL_free(ssl);

    /* closes the connection */
    close(sock);
    
    /* new ssl to reconnect to */
    sslResume = CyaSSL_new(ctx);
    
    /* makes a new socket to connect to */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    /* sets session to old session */
    CyaSSL_set_session(sslResume, session);
    
    /* connects to new socket */
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        /* if socket fails to connect to the server*/
        ret = CyaSSL_get_error(ssl, 0);
        printf("Connect error. Error: %i\n", ret);
        return EXIT_FAILURE;
    }
    
    /* sets new file discriptior */
    CyaSSL_set_fd(sslResume, sock);
    
    /* reconects to CyaSSL */
    ret = CyaSSL_connect(sslResume);
    if (ret != SSL_SUCCESS) {
        return ret;
    }
    
    /* checks to see if the new session is the same as the old session */
    if (CyaSSL_session_reused(sslResume))
        printf("Re-used session ID\n"); 
    else
        printf("Did not re-use session ID\n");
    
    /* regreet the client */
    ret = ClientGreet(sock, sslResume);
    
    /* closes the connection */
    close(sock);
    
    /* frees all data before client termination */
    CyaSSL_free(sslResume);
    CyaSSL_CTX_free(ctx);
    CyaSSL_Cleanup();

    return ret;
}

/* 
 * Command line argumentCount and argumentValues 
 */
int main(int argc, char** argv) 
{
    int     sockfd;                         /* socket file descriptor */
    struct  sockaddr_in servAddr;           /* struct for server address */
    int     ret;                            /* variable for error checking */

    if (argc != 2) {
        /* if the number of arguments is not two, error */
        printf("usage: ./client-tcp  <IP address>\n");
        return EXIT_FAILURE;
    }

    /* internet address family, stream based tcp, default protocol */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        printf("Failed to create socket. errono: %i\n", errno);
        return EXIT_FAILURE;
    }

    memset(&servAddr, 0, sizeof(servAddr)); /* clears memory block for use */  
    servAddr.sin_family = AF_INET;          /* sets addressfamily to internet*/
    servAddr.sin_port = htons(SERV_PORT);   /* sets port to defined port */

    /* looks for the server at the entered address (ip in the command line) */
    if ((ret = inet_pton(AF_INET, argv[1], &servAddr.sin_addr)) < 1) {
        /* checks validity of address */
        ret = errno;
        printf("Invalid Address. Error: %i\n", ret);
        return EXIT_FAILURE;
    }

    if ((ret = connect(sockfd, (struct sockaddr *) &servAddr, 
        sizeof(servAddr))) < 0) {
        /* if socket fails to connect to the server*/
        ret = errno;
        printf("Connect error. Error: %i\n", ret);
        return EXIT_FAILURE;
    }
    Security(sockfd, servAddr);

    return ret;
}
