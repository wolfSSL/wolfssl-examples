/* client-tls-bio.c
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

/* standard library */
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
#define CERT_FILE "../certs/ca-cert.pem"

/* Example Usage:
 * ./examples/server/server -d
 *
 * ./client-tls-bio 127.0.0.1
 * creating new WOLFSSL BIO type to write and read with
 * Message for server: asdf
 * Server: I hear you fa shizzle!
 */

int main(int argc, char** argv)
{
    int                ret = 0;
#ifdef OPENSSL_EXTRA
    int                sockfd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;
    WOLFSSL_BIO* bio = NULL;

    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        return 0;
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
        fprintf(stderr, "ERROR: invalid address\n");
        ret = -1; 
        goto exit;
    }

    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr))
        == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
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
    wolfSSL_set_fd(ssl, sockfd);

    printf("creating new WOLFSSL BIO type to write and read with\n");
    bio = wolfSSL_BIO_new(wolfSSL_BIO_f_ssl());
    if (bio == NULL) {
        ret = -1; 
        goto exit;
    }
    wolfSSL_BIO_set_ssl(bio, ssl, BIO_CLOSE); /* with BIO_CLOSE wolfSSL_BIO_free will free ssl */

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
    if (wolfSSL_BIO_write(bio, buff, len) != len) {
        fprintf(stderr, "ERROR: failed to write\n");
        ret = -1; 
        goto exit;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_BIO_read(bio, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n"); 
        goto exit;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);

    ret = 0;

exit:
    /* Cleanup and return */
    if (bio)
       wolfSSL_BIO_free(bio);
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket listening for clients   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */

#else
    printf("This example requires ./configure --enable-opensslextra or OPENSSL_EXTRA to be defined\n");
#endif
    return ret;               /* Return reporting a success               */
}
