/* client-tls13-resume.c
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
#include <wolfssl/wolfcrypt/error-crypt.h>

#define DEFAULT_PORT 11111

#define CERT_FILE "../certs/ca-cert.pem"


#if defined(WOLFSSL_TLS13) && defined(HAVE_SECRET_CALLBACK)

#ifndef WOLFSSL_SSLKEYLOGFILE_OUTPUT
    #define WOLFSSL_SSLKEYLOGFILE_OUTPUT "sslkeylog.log"
#endif

/* Callback function for TLS v1.3 secrets for use with Wireshark */
static int Tls13SecretCallback(WOLFSSL* ssl, int id, const unsigned char* secret,
    int secretSz, void* ctx)
{
    int i;
    const char* str = NULL;
    unsigned char clientRandom[32];
    int clientRandomSz;
    XFILE fp = stderr;
    if (ctx) {
        fp = XFOPEN((const char*)ctx, "ab");
        if (fp == XBADFILE) {
            return BAD_FUNC_ARG;
        }
    }

    clientRandomSz = (int)wolfSSL_get_client_random(ssl, clientRandom,
        sizeof(clientRandom));

    if (clientRandomSz <= 0) {
        printf("Error getting client random %d\n", clientRandomSz);
    }

#if 0
    printf("TLS Client Secret CB: Rand %d, Secret %d\n",
        clientRandomSz, secretSz);
#endif

    switch (id) {
        case CLIENT_EARLY_TRAFFIC_SECRET:
            str = "CLIENT_EARLY_TRAFFIC_SECRET"; break;
        case EARLY_EXPORTER_SECRET:
            str = "EARLY_EXPORTER_SECRET"; break;
        case CLIENT_HANDSHAKE_TRAFFIC_SECRET:
            str = "CLIENT_HANDSHAKE_TRAFFIC_SECRET"; break;
        case SERVER_HANDSHAKE_TRAFFIC_SECRET:
            str = "SERVER_HANDSHAKE_TRAFFIC_SECRET"; break;
        case CLIENT_TRAFFIC_SECRET:
            str = "CLIENT_TRAFFIC_SECRET_0"; break;
        case SERVER_TRAFFIC_SECRET:
            str = "SERVER_TRAFFIC_SECRET_0"; break;
        case EXPORTER_SECRET:
            str = "EXPORTER_SECRET"; break;
    }

    fprintf(fp, "%s ", str);
    for (i = 0; i < clientRandomSz; i++) {
        fprintf(fp, "%02x", clientRandom[i]);
    }
    fprintf(fp, " ");
    for (i = 0; i < secretSz; i++) {
        fprintf(fp, "%02x", secret[i]);
    }
    fprintf(fp, "\n");

    if (fp != stderr) {
        XFCLOSE(fp);
    }

    return 0;
}
#endif /* WOLFSSL_TLS13 && HAVE_SECRET_CALLBACK */


/* Please configure wolfssl with --enable-session-ticket. Failing to do so will
 * cause an error when resumption is attempted. */

int main(int argc, char** argv)
{
    int                ret = 0;
#ifdef WOLFSSL_TLS13
    int                sockfd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    /* declare objects for session resuming */
    WOLFSSL_SESSION* session = NULL;
    WOLFSSL*         sslRes  = NULL;


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
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method())) == NULL) {
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
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto exit;
    }

#ifdef HAVE_SECRET_CALLBACK
    /* required for getting random used */
    wolfSSL_KeepArrays(ssl);

    /* optional logging for wireshark */
    wolfSSL_set_tls13_secret_cb(ssl, Tls13SecretCallback,
        (void*)WOLFSSL_SSLKEYLOGFILE_OUTPUT);
#endif

    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        goto exit;
    }

#ifdef HAVE_SECRET_CALLBACK
    wolfSSL_FreeArrays(ssl);
#endif

#if 0
    /* Save the session
     *
     * NOTE: This is not an ideal solution. Please see below where we call
     * wolfSSL_get_session() just before disconnection. We do not need to
     * call wolfSSL_peek() in that case because we have already called
     * wolfSSL_read() so wolfSSL has already internally stored the session
     * ticket. For TLS 1.2 it is fine to put wolfSSL_get_session() right after
     * the connection is established because the resumption information is part
     * of the TLS 1.2 handshake. This is not the case for TLS 1.3. However, if
     * you are migrating from TLS 1.2 and are having a hard time moving your
     * call to wolfSSL_get_session(), you can try this approach.
     *
     * This approach can result in issues with I/O and is best used with non-
     * blocking mode sockets.
     */
    session = wolfSSL_get_session(ssl);
    if (session == NULL) {
        printf("Session not available yet... trying peek\n");
        wolfSSL_peek(ssl, buff, 1);
        session = wolfSSL_get_session(ssl);
        if (session != NULL) {
            printf("Session ticket found\n");
        }
    }
#endif

    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to to get message for server\n");
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
    if (wolfSSL_read(ssl, buff, sizeof(buff)-1) < 0) {
        fprintf(stderr, "ERROR: failed to read\n");
        ret = -1; 
        goto exit;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);



    /* Save the session */
    if (session == NULL) {
        session = wolfSSL_get_session(ssl);
    }

    /* Close the socket */
    wolfSSL_free(ssl); ssl = NULL;
    close(sockfd); sockfd = SOCKET_INVALID;



    /* --------------------------------------- *
     * we are now disconnected from the server *
     * --------------------------------------- */



    /* Create a new WOLFSSL object to resume with */
    if ((sslRes = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto exit;
    }

    /* Set up to resume the session */
    if ((ret = wolfSSL_set_session(sslRes, session)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Failed to set session, make sure session tickets "
                        "(--enable-session ticket) is enabled\n");
        /*goto exit;*/ /* not fatal */
    }



    /* Get a new socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1; 
        goto exit;
    }



    /* Reconnect to the server */
    if (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr))
        == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        ret = -1;
        goto exit;
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(sslRes, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto exit;
    }

#ifdef HAVE_SECRET_CALLBACK
    /* required for getting random used */
    wolfSSL_KeepArrays(sslRes);

    /* optional logging for wireshark */
    wolfSSL_set_tls13_secret_cb(sslRes, Tls13SecretCallback,
        (void*)WOLFSSL_SSLKEYLOGFILE_OUTPUT);
#endif

    /* Reconnect to wolfSSL */
    if ((ret = wolfSSL_connect(sslRes)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        goto exit;
    }

#ifdef HAVE_SECRET_CALLBACK
    wolfSSL_FreeArrays(ssl);
#endif


    /* Test if the resume was successful */
    if (wolfSSL_session_reused(sslRes)) {
        printf("Session ID reused; Successful resume\n");
    }
    else {
        printf("Session not resumed, full handshake done instead\n");
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
    if ((ret = wolfSSL_write(sslRes, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to write\n");
        goto exit;
    }



    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(sslRes, buff, sizeof(buff)-1) < 0) {
        fprintf(stderr, "ERROR: failed to read\n");
        ret = -1; 
        goto exit;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);
    ret = 0;

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (sslRes)
        wolfSSL_free(sslRes);      /* Free the wolfSSL object              */
#ifdef OPENSSL_EXTRA   
    if (session)
        wolfSSL_SESSION_free(session);
#endif    
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */
#else
    printf("Example requires TLS v1.3\n");
#endif

    return ret;                 /* Return reporting a success               */
}
