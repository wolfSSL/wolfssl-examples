/* pq-client.c
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

/* A modern client that connects to the PQC crypto-proxy using a post-quantum
 * (hybrid ML-KEM) TLS 1.3 handshake. It neither knows nor cares that the proxy
 * forwards its request to a legacy TLS 1.2 origin behind the scenes. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include "proxy-common.h"

#if defined(WOLFSSL_TLS13) && defined(WOLFSSL_HAVE_MLKEM)

int main(int argc, char** argv)
{
    int ret = 0;
    int sockfd = SOCKET_INVALID;
    int port = FRONTEND_PORT;
    const char* host = "127.0.0.1";
    struct sockaddr_in servAddr;
    char buff[256];
    size_t len;

    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = atoi(argv[2]);
    if (argc > 3) {
        printf("usage: %s [<proxy host> [<proxy port>]]\n", argv[0]);
        printf("Defaults: 127.0.0.1 %d\n", FRONTEND_PORT);
        return 0;
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port   = htons(port);
    if (inet_pton(AF_INET, host, &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address %s\n", host);
        ret = -1;
        goto exit;
    }

    if (connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        ret = -1;
        goto exit;
    }

    wolfSSL_Init();

    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* Trust the proxy's frontend certificate. */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, FRONTEND_CA, NULL))
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", FRONTEND_CA);
        goto exit;
    }

    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto exit;
    }

    /* Offer the post-quantum (hybrid) group for key establishment. */
    if ((ret = wolfSSL_UseKeyShare(ssl, FRONTEND_GROUP)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set PQC key share (%s)\n",
            FRONTEND_GROUP_NAME);
        goto exit;
    }

    wolfSSL_set_fd(ssl, sockfd);

    if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: handshake failed: %d\n",
            wolfSSL_get_error(ssl, 0));
        ret = -1;
        goto exit;
    }

    printf("Connected to proxy: %s %s  kex=%s\n",
        wolfSSL_get_version(ssl), wolfSSL_get_cipher(ssl),
        wolfSSL_get_curve_name(ssl));

    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message\n");
        ret = -1;
        goto exit;
    }
    len = strnlen(buff, sizeof(buff));

    if (wolfSSL_write(ssl, buff, (int)len) != (int)len) {
        fprintf(stderr, "ERROR: failed to write\n");
        ret = -1;
        goto exit;
    }

    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(ssl, buff, sizeof(buff) - 1) < 0) {
        fprintf(stderr, "ERROR: failed to read\n");
        ret = -1;
        goto exit;
    }
    printf("Server: %s", buff);

    ret = 0;

exit:
    if (ssl) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (sockfd != SOCKET_INVALID)
        close(sockfd);
    if (ctx)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return ret;
}

#else

int main(void)
{
    printf("This example requires TLS 1.3 and ML-KEM (Kyber).\n");
    printf("Configure wolfSSL like this:\n");
    printf("    ./configure --enable-kyber --enable-tls13\n");
    return 0;
}

#endif /* WOLFSSL_TLS13 && WOLFSSL_HAVE_MLKEM */
