/* server-pq-tls13.c
 *
 * Copyright (C) 2021 wolfSSL Inc.
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

#define HAVE_SIGNAL
#ifdef HAVE_SIGNAL
#include <signal.h>        /* signal */
#endif

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define DEFAULT_PORT 11111

#define CERT_FILE "../certs/falcon_level5_entity_cert.pem"
#define KEY_FILE  "../certs/falcon_level5_entity_key.pem"


#if defined(HAVE_SECRET_CALLBACK) && defined(WOLFSSL_TLS13) && \
    defined(HAVE_LIBOQS)

#ifndef WOLFSSL_SSLKEYLOGFILE_OUTPUT
    #define WOLFSSL_SSLKEYLOGFILE_OUTPUT "sslkeylog.log"
#endif

/* Callback function for TLS v1.3 secrets for use with Wireshark */
static int Tls13SecretCallback(WOLFSSL* ssl, int id, const unsigned char* secret,
    int secretSz, void* ctx)
{
    int i;
    const char* str = NULL;
    unsigned char serverRandom[32];
    int serverRandomSz;
    XFILE fp = stderr;
    if (ctx) {
        fp = XFOPEN((const char*)ctx, "ab");
        if (fp == XBADFILE) {
            return BAD_FUNC_ARG;
        }
    }

    serverRandomSz = (int)wolfSSL_get_server_random(ssl, serverRandom,
        sizeof(serverRandom));

    if (serverRandomSz <= 0) {
        printf("Error getting server random %d\n", serverRandomSz);
    }

#if 0
    printf("TLS Server Secret CB: Rand %d, Secret %d\n",
        serverRandomSz, secretSz);
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
    for (i = 0; i < (int)serverRandomSz; i++) {
        fprintf(fp, "%02x", serverRandom[i]);
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

#if defined(WOLFSSL_TLS13) && defined(HAVE_LIBOQS)
static int mSockfd = SOCKET_INVALID;
static int mConnd = SOCKET_INVALID;
static int mShutdown = 0;

#ifdef HAVE_SIGNAL
static void sig_handler(const int sig)
{
    fprintf(stderr, "SIGINT handled = %d.\n", sig);

    mShutdown = 1;
    if (mConnd != SOCKET_INVALID) {
        close(mConnd);           /* Close the connection to the client   */
        mConnd = SOCKET_INVALID;
    }
    if (mSockfd != SOCKET_INVALID) {
        close(mSockfd);          /* Close the socket listening for clients   */
        mSockfd = SOCKET_INVALID;
    }
}
#endif
#endif

int main(int argc, char** argv)
{
    int ret = 0;
#if defined(WOLFSSL_TLS13) && defined(HAVE_LIBOQS)
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    const char*        reply = "I hear ya fa shizzle!\n";

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    char *cert_file = CERT_FILE;
    char *key_file  = KEY_FILE;

#ifdef HAVE_SIGNAL
    signal(SIGINT, sig_handler);
#endif

    if(argc == 3) {
        cert_file = argv[1];
        key_file  = argv[2];
    } else if (argc != 1) {
        printf("usage: %s <IPv4 address> [<cert file> <key file>]\n", argv[0]);
        printf("Default cert file: %s, key file: %s\n", cert_file, key_file);
        return 0;
    }

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((mSockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        goto exit;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* Load server certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, cert_file, WOLFSSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                cert_file);
        goto exit;
    }

    /* Load server key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, key_file, WOLFSSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                key_file);
        goto exit;
    }


    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */


    /* Bind the server socket to our port */
    if (bind(mSockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        goto exit;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(mSockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        goto exit;
    }

    /* Continue to accept clients until mShutdown is issued */
    while (!mShutdown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((mConnd = accept(mSockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1; goto exit;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1; goto exit;
        }

        ret = wolfSSL_UseKeyShare(ssl, WOLFSSL_P521_KYBER_LEVEL5);
        if (ret < 0) {
            fprintf(stderr, "ERROR: failed to set the requested group to "
                            "P521_KYBER_LEVEL5.\n");
            ret = -1; goto exit;
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, mConnd);

    #ifdef HAVE_SECRET_CALLBACK
        /* required for getting random used */
        wolfSSL_KeepArrays(ssl);

        /* optional logging for wireshark */
        wolfSSL_set_tls13_secret_cb(ssl, Tls13SecretCallback,
            (void*)WOLFSSL_SSLKEYLOGFILE_OUTPUT);
    #endif

        /* Establish TLS connection */
        if ((ret = wolfSSL_accept(ssl)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_accept error = %d\n",
                wolfSSL_get_error(ssl, ret));
            goto exit;
        }

        printf("Client connected successfully\n");

    #ifdef HAVE_SECRET_CALLBACK
        wolfSSL_FreeArrays(ssl);
    #endif

        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) < 0) {
            fprintf(stderr, "ERROR: failed to read\n");
            goto exit;
        }

        /* Print to stdout any data the client sends */
        printf("Client: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            mShutdown = 1;
        }

        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        /* Reply back to the client */
        if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            goto exit;
        }

        /* Cleanup after this connection */
        wolfSSL_shutdown(ssl);
        if (ssl) {
            wolfSSL_free(ssl);      /* Free the wolfSSL object              */
            ssl = NULL;
        }
        if (mConnd != SOCKET_INVALID) {
            close(mConnd);           /* Close the connection to the client   */
            mConnd = SOCKET_INVALID;
        }
    }

    printf("Shutdown complete\n");

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (mConnd != SOCKET_INVALID) {
        close(mConnd);           /* Close the connection to the client   */
        mConnd = SOCKET_INVALID;
    }
    if (mSockfd != SOCKET_INVALID) {
        close(mSockfd);          /* Close the socket listening for clients   */
        mSockfd = SOCKET_INVALID;
    }
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */

#else
    printf("Example requires TLS v1.3 and liboqs.\n");
    printf("Configure wolfssl like this: ./configure --with-liboqs\n");
#endif /* WOLFSSL_TLS13 */

    (void)argc;
    (void)argv;

    return ret;
}
