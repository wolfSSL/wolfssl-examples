/* client-tls-nonblocking.c
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

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#define DEFAULT_PORT 11111

#define CONNECT_WAIT_MS 2

#define CERT_FILE "../certs/ca-cert.pem"

int main(int argc, char** argv)
{
    int                ret, err;
    int                sockfd = SOCKET_INVALID; 
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;



    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        ret = 0;
        goto exit;
    }



    /* Initialize wolfSSL */
    wolfSSL_Init();



    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit; 
    }

    /* Set the socket options to use nonblocking I/O */
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
        fprintf(stderr, "ERROR: failed to set non-blocking\n");
        ret = -1;
        goto exit;
    }



    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CERT_FILE, NULL))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto exit;
    }



    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid Address\n");
        ret = -1;
        goto exit;
    }


    /* Connect to the server */
    while (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr))
           == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* no error, just non-blocking. Carry on. */
            continue;
        } else if(errno == EINPROGRESS || errno == EALREADY)
            break;
        /* just keep looping until a connection is made */
        fprintf(stderr, "ERROR: failed to connect %d\n\n", errno);
        ret = -1;
        goto exit;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto exit;
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto exit;
    }
 
    /* Connect to wolfSSL on the server side */
    do {                                                                    
        ret = wolfSSL_connect(ssl);                                         
        err = wolfSSL_get_error(ssl, ret);                                  
    } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE); 
  
    if (ret != WOLFSSL_SUCCESS){
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        goto exit;
    }



    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto exit;
    }
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    do {
        ret = wolfSSL_write(ssl, buff, len);
        err = wolfSSL_get_error(ssl, ret);
    }
    while (err == WOLFSSL_ERROR_WANT_WRITE);
    if (ret < 0) {
        fprintf(stderr, "ERROR %d: failed to write\n", ret);
        goto exit;
    }
    
    /* Read the reply from server */
    memset(buff, 0, sizeof(buff));
    do {
        ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WOLFSSL_ERROR_WANT_READ);
    if (ret < 0) {
        fprintf(stderr, "ERROR %d: failed to read\n", ret);
        goto exit;
    }
    
    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);

    /* send close notify */
    do {
        ret = wolfSSL_shutdown(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            ret = wolfSSL_get_error(ssl, 0);
        }
    } while (ret == WOLFSSL_ERROR_WANT_READ ||
             ret == WOLFSSL_ERROR_WANT_WRITE);
    printf("Shutdown complete\n");

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */

    return ret;                 /* Return reporting a success               */
}
