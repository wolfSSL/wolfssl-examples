/* server-tls-writedup.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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

#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"



int main()
{
    int                ret = 0;
#ifdef HAVE_WRITE_DUP
    int                sockfd = SOCKET_INVALID;
    int                connd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    int                shutdown = 0;
    const char*        reply = "I hear ya fa shizzle!\n";

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL *ssl = NULL, *write_ssl = NULL;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

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
    if ((ctx = wolfSSL_CTX_new(wolfSSLv23_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* Load server certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto exit;
    }

    /* Load server key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        goto exit;
    }

    /* Set the protocol min / max */
    wolfSSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    wolfSSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */



    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1; 
        goto exit;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }



    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
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
        wolfSSL_set_fd(ssl, connd);

        /* Establish TLS connection */
        ret = wolfSSL_accept(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_accept error = %d\n",
                wolfSSL_get_error(ssl, ret));
            goto exit;
        }


        printf("Client connected successfully\n");



        write_ssl = wolfSSL_write_dup(ssl);
        if (write_ssl == NULL) {
            fprintf(stderr, "wolfSSL_write_dup failed\n");
            ret = -1;
            goto exit;
        }

        {
            int w_ver = wolfSSL_version(write_ssl);
            int r_ver = wolfSSL_version(ssl);
            if (w_ver != r_ver) {
                fprintf(stderr, "ERROR: SSL versions are different\n");
                goto exit;
            }
        }

        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
            fprintf(stderr, "ERROR: failed to read\n");
            goto exit;
        }

        /* Print to stdout any data the client sends */
        printf("Client: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }

        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        /* Reply back to the client */
        if ((ret = wolfSSL_write(write_ssl, buff, len)) != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            goto exit;
        }

        /* Notify the client that the connection is ending */
        wolfSSL_shutdown(write_ssl);
        wolfSSL_shutdown(ssl);
        printf("Shutdown complete\n");

        /* Cleanup after this connection */
        wolfSSL_free(write_ssl);/* Free the dup wolfSSL object          */
        write_ssl = NULL;
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        ssl = NULL;
        close(connd);           /* Close the connection to the client   */
    }

    ret = 0;

exit:
    /* Cleanup and return */
    if (write_ssl) {
        wolfSSL_free(write_ssl);/* Free the dup wolfSSL object          */
        write_ssl = NULL;
    }
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (connd != SOCKET_INVALID)
        close(connd);           /* Close the connection to the client   */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket listening for clients   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */
#else
    printf("wolfSSL not configured with --enable-writedup.\n"
           "Please re-configure and re-install wolfSSL to try out"
           "this example!\n");
#endif
    return ret;               /* Return reporting a success               */
}
