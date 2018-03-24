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
 *
 * -----------------------------------------------------------------------------
 * NOTE:
 * wolfSSL needs to be built with --enable-writedup, or else we'll see errors.
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

/* threads */
#include <pthread.h>

#define DEFAULT_PORT 11111

#define CERT_FILE "../certs/ca-cert.pem"


#ifdef HAVE_WRITE_DUP
void* ReadHandler(void* args)
{
    char     buff[256];
    WOLFSSL* ssl = (WOLFSSL*)args;

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(ssl, buff, sizeof(buff)-1) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        return NULL;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);

    return NULL;
}



void* WriteHandler(void* args)
{
    char     buff[256];
    size_t   len;
    WOLFSSL* ssl = (WOLFSSL*)args;

    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    fgets(buff, sizeof(buff), stdin);
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    if (wolfSSL_write(ssl, buff, len) != len) {
        fprintf(stderr, "ERROR: failed to write\n");
        return NULL;
    }

    return NULL;
}
#endif


int main(int argc, char** argv)
{
#ifdef HAVE_WRITE_DUP
    int                sockfd;
    struct sockaddr_in servAddr;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     read_ssl;
    WOLFSSL*     write_ssl;

    /* declare pthread variable */
    pthread_t read_thread;
    pthread_t write_thread;



    /* check for writedup */
    #ifndef HAVE_WRITE_DUP
        #warning wolfSSL must be configured and installed with --enable-writedup
        fprintf(stderr, "wolfSSL must be configured and installed with "
                "--enable-writedup");
        return -1;
    #endif

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
        return -1;
    }



    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    /* Load client certificates into WOLFSSL_CTX */
    if (wolfSSL_CTX_load_verify_locations(ctx, CERT_FILE, NULL)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        return -1;
    }



    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        return -1;
    }



    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr))
        == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        return -1;
    }



    /* Create a WOLFSSL object */
    if ((read_ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        return -1;
    }

    /* Attach wolfSSL to the socket */
    wolfSSL_set_fd(read_ssl, sockfd);

    /* Connect to wolfSSL on the server side */
    if (wolfSSL_connect(read_ssl) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        return -1;
    }

    /* Duplicate read_ssl, setting it to read-only,
     * creating write_ssl, which is write-only */
    if ((write_ssl = wolfSSL_write_dup(read_ssl)) == NULL) {
        fprintf(stderr, "ERROR: failed write dup\n");
        return -1;
    }



    /* Launch the threads */
    pthread_create(&read_thread, NULL, ReadHandler, read_ssl);
    pthread_create(&write_thread, NULL, WriteHandler, write_ssl);

    /* Rejoin the threads */
    pthread_join(write_thread, NULL);
    pthread_join(read_thread, NULL);



    /* Cleanup and return */
    wolfSSL_free(read_ssl); /* Free the read wolfSSL object             */
    wolfSSL_free(write_ssl);/* Free the write wolfSSL object            */
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
    close(sockfd);          /* Close the connection to the server       */
#else
    printf("wolfSSL not configured with --enable-writedup.\n"
           "Please re-configure and re-install wolfSSL to try out"
           "this example!\n");
#endif
    return 0;               /* Return reporting a success               */
}
