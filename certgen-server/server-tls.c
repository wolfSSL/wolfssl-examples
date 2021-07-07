/* server-tls.c
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
#include <wolfssl/certs_test.h>
#include <wolfssl/wolfcrypt/asn_public.h>

/* example header */
#include "server-tls.h"

#define DEFAULT_PORT 11111

#ifndef USE_CERT_BUFFERS_2048
    #error please build wolfSSL with USE_CERT_BUFFERS_2048
#endif


int main()
{
    int                sockfd;
    int                connd;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               command[256];
    char               buffer[256];
    int                shutDown = 0;
    int                ret, err, firstRead, gotFirstG, echoSz;

    unsigned char serverDer[2048];
    int serverDerSz = sizeof(serverDer);

    /* PEM certificate buffers */
    unsigned char server[2048];
    unsigned char serveK[2048];
    unsigned char cert[4096]; /* certificate chain to send */
    int serverSz = sizeof(server);
    int serveKSz = sizeof(serveK);
    int certSz   = sizeof(cert);

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;

    wolfSSL_Debugging_ON();

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* create new certificate with IP address as common name */
    if (createSignedCert(
                (unsigned char*)server_cert_der_2048, sizeof_server_cert_der_2048,
                (unsigned char*)server_key_der_2048, sizeof_server_key_der_2048,
                serverDer, &serverDerSz,
                server,    &serverSz,
                serveK,    &serveKSz,
                "127.0.0.1", 0) != 0) {
        fprintf(stderr, "Failure creating new certificate\n");
        return -1;
    }
    XMEMCPY(cert, server, serverSz);

    /* convert CA to PEM format */
    ret = wc_DerToPem((unsigned char*)server_cert_der_2048,
            sizeof_server_cert_der_2048, cert + serverSz, certSz - serverSz,
            CERT_TYPE);
    if (ret <= 0) {
        fprintf(stderr, "error converting CA to PEM format.\n");
        return -1;
    }
    certSz = ret + serverSz;

    {
        /* for debugging print out created certificate to files */
        FILE* f = fopen("created_chain.pem", "wb");
        if (f != NULL ) {
            fwrite(cert, 1, certSz, f);
            fclose(f);
        }
        f = fopen("created_cert.der", "wb");
        if (f != NULL ) {
            fwrite(server, 1, serverSz, f);
            fclose(f);
        }
        f = fopen("created_key.der", "wb");
        if (f != NULL ) {
            fwrite(serveK, 1, serveKSz, f);
            fclose(f);
        }
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    /* For this example load certificate chain into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_certificate_chain_buffer(ctx, cert, certSz)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load certificate chain.\n");
        return -1;
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_buffer(ctx, serveK, serveKSz,
                SSL_FILETYPE_ASN1)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load server key.\n");
        return -1;
    }


    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */


    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        return -1;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        return -1;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutDown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            return -1;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            return -1;
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);

        printf("Client connected successfully\n");

        /* Very basic HTTP GET command -- intended to be used as an example.
         * read and write from wolfssl-root/examples/echoserver/echoserver.c */
        while (1) {
            err = 0; /* reset error */
            ret = wolfSSL_read(ssl, command, sizeof(command)-1);
            if (ret <= 0) {
                err = wolfSSL_get_error(ssl, 0);
                if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_ZERO_RETURN){
                    printf("SSL_read echo error %d, %s!\n", err,
                        wolfSSL_ERR_error_string(err, buffer));
                }
                break;
            }

            echoSz = ret;

            if (firstRead == 1) {
                firstRead = 0;  /* browser may send 1 byte 'G' to start */
                if (echoSz == 1 && command[0] == 'G') {
                    gotFirstG = 1;
                    continue;
                }
            }
            else if (gotFirstG == 1 && strncmp(command, "ET /", 4) == 0) {
                strncpy(command, "GET", 4);
                /* fall through to normal GET */
            }

            if ( strncmp(command, "quit", 4) == 0) {
                printf("client sent quit command: shutting down!\n");
                shutDown = 1;
                break;
            }
            if ( strncmp(command, "break", 5) == 0) {
                printf("client sent break command: closing session!\n");
                break;
            }
            if ( strncmp(command, "GET", 3) == 0) {
                char type[]   = "HTTP/1.0 200 ok\r\nContent-type:"
                                " text/html\r\n\r\n";
                char header[] = "<html><body BGCOLOR=\"#ffffff\">\n<pre>\n";
                char body[]   = "greetings from wolfSSL\n";
                char footer[] = "</body></html>\r\n\r\n";

                strncpy(command, type, sizeof(type));
                echoSz = sizeof(type) - 1;

                strncpy(&command[echoSz], header, sizeof(header));
                echoSz += (int)sizeof(header) - 1;
                strncpy(&command[echoSz], body, sizeof(body));
                echoSz += (int)sizeof(body) - 1;
                strncpy(&command[echoSz], footer, sizeof(footer));
                echoSz += (int)sizeof(footer);

                err = 0; /* reset error */
                ret = wolfSSL_write(ssl, command, echoSz);
                if (ret <= 0) {
                    err = wolfSSL_get_error(ssl, 0);
                }
                if (ret != echoSz) {
                    printf("SSL_write get error = %d, %s\n", err,
                        wolfSSL_ERR_error_string(err, buffer));
                    printf("SSL_write get failed\n");
                }
                break;
            }
            command[echoSz] = 0;

            ret = wolfSSL_write(ssl, command, echoSz);
            if (ret <= 0) {
                printf("Error %d\n", wolfSSL_get_error(ssl, 0));
                break;
            }

            if (ret != echoSz) {
                printf("SSL_write echo error = %d, %s\n", err,
                        wolfSSL_ERR_error_string(err, buffer));
                printf("SSL_write echo failed\n");
            }
        }

        /* Cleanup after this connection */
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        close(connd);           /* Close the connection to the client   */
    }

    printf("Shutdown complete\n");

    /* Cleanup and return */
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
    close(sockfd);          /* Close the socket listening for clients   */
    return 0;               /* Return reporting a success               */
}
