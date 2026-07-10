/* legacy-server.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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

/* A stand-in for a legacy origin service: it only speaks classical TLS 1.2.
 * The PQC proxy (pq-proxy) terminates the quantum-safe client connection and
 * forwards the plaintext to this server over a legacy cipher suite. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include "proxy-common.h"

static int mSockfd = SOCKET_INVALID;
static int mConnd  = SOCKET_INVALID;
static int mShutdown = 0;

static void sig_handler(const int sig)
{
    (void)sig;
    mShutdown = 1;
    if (mConnd != SOCKET_INVALID) {
        close(mConnd);
        mConnd = SOCKET_INVALID;
    }
    if (mSockfd != SOCKET_INVALID) {
        close(mSockfd);
        mSockfd = SOCKET_INVALID;
    }
}

/* Accept and service client connections until a shutdown is requested. */
static int accept_loop(WOLFSSL_CTX* ctx)
{
    int                ret = 0;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    const char*        reply = "[origin] legacy service reached over TLS 1.2\n";
    WOLFSSL*           ssl = NULL;

    while (!mShutdown) {
        int n;

        printf("Waiting for a connection...\n");
        if ((mConnd = accept(mSockfd, (struct sockaddr*)&clientAddr, &size))
                == -1) {
            if (mShutdown)
                break;
            fprintf(stderr, "ERROR: failed to accept the connection\n");
            ret = -1;
            break;
        }

        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1;
            break;
        }
        wolfSSL_set_fd(ssl, mConnd);

        if (wolfSSL_accept(ssl) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: wolfSSL_accept error %d\n",
                wolfSSL_get_error(ssl, 0));
            goto conn_cleanup;
        }

        printf("Proxy connected: %s %s\n",
            wolfSSL_get_version(ssl), wolfSSL_get_cipher(ssl));

        memset(buff, 0, sizeof(buff));
        if ((n = wolfSSL_read(ssl, buff, sizeof(buff) - 1)) > 0) {
            printf("Received: %s", buff);
            if (strncmp(buff, "shutdown", 8) == 0) {
                printf("Shutdown command issued!\n");
                mShutdown = 1;
            }
        }

        if (wolfSSL_write(ssl, reply, (int)strlen(reply)) < 0)
            fprintf(stderr, "ERROR: failed to write\n");

        wolfSSL_shutdown(ssl);

conn_cleanup:
        wolfSSL_free(ssl);
        ssl = NULL;
        if (mConnd != SOCKET_INVALID) {
            close(mConnd);
            mConnd = SOCKET_INVALID;
        }
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;
    int port = BACKEND_PORT;
    struct sockaddr_in servAddr;

    WOLFSSL_CTX* ctx = NULL;

    if (argc == 2) {
        port = atoi(argv[1]);
    }
    else if (argc != 1) {
        printf("usage: %s [<listen port>]\n", argv[0]);
        printf("Default listen port: %d\n", BACKEND_PORT);
        return 0;
    }

    signal(SIGINT, sig_handler);

    wolfSSL_Init();

    if ((mSockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }

    /* Deliberately pin to classical TLS 1.2 - cannot be upgraded. */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    if ((ret = wolfSSL_CTX_set_cipher_list(ctx, BACKEND_CIPHERS))
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set cipher list %s\n",
            BACKEND_CIPHERS);
        goto exit;
    }

    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, BACKEND_CERT,
            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", BACKEND_CERT);
        goto exit;
    }

    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, BACKEND_KEY,
            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", BACKEND_KEY);
        goto exit;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_port        = htons(port);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(mSockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto exit;
    }

    if (listen(mSockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }

    printf("Legacy origin server listening on port %d (%s)\n",
        port, BACKEND_CIPHERS);

    ret = accept_loop(ctx);
    if (ret == 0)
        printf("Shutdown complete\n");

exit:
    if (mConnd != SOCKET_INVALID)
        close(mConnd);
    if (mSockfd != SOCKET_INVALID)
        close(mSockfd);
    if (ctx)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return ret;
}
