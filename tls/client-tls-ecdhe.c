/* client-tls-ecdhe.c
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

#define CERT_FILE "../certs/ca-ecc-cert.pem"

#define ECC_FILE    "../certs/client-ecc-cert.pem"
#define KEY_FILE    "../certs/ecc-client-key.pem"
#define CIPHER_LIST "ECDHE-ECDSA-CHACHA20-POLY1305"



int main(int argc, char** argv)
{
    int                ret; 
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

    /* Load client ecc certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_chain_file(ctx, ECC_FILE)) != 
            WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                ECC_FILE);
        goto exit;
    }

    /* Load client ecc key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        goto exit;
    }

    /* Set cipher list */
    if ((ret = wolfSSL_CTX_set_cipher_list(ctx, CIPHER_LIST))
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set cipher list\n");
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

    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
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
    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to write\n");
        goto exit;
    }



    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto exit;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);

    ret = 0;

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);           /* Close the connection to the server   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */

    return ret;               /* Return reporting a success               */
}
