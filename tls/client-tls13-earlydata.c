/* client-tls13-earlydata.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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

/* Example TLS 1.3 client using wolfSSL early data (0-RTT) with session resumption.
 * Performs an initial handshake to obtain a session ticket, then reconnects and
 * sends early data using wolfSSL_write_early_data().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>

#define DEFAULT_PORT 11111
#define CERT_FILE    "../certs/client-cert.pem"
#define KEY_FILE     "../certs/client-key.pem"
#define CA_FILE      "../certs/ca-cert.pem"

#define EARLY_DATA_MSG "Early data hello from early data client!"
#define EARLY_DATA_MSG_LEN (sizeof(EARLY_DATA_MSG))

static int tcp_connect(const char* ip, int port) {
    int sockfd;
    struct sockaddr_in servAddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &servAddr.sin_addr) != 1) {
        perror("inet_pton()");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("connect()");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s <server-ip>\n", argv[0]);
        return 1;
    }

    const char* server_ip = argv[1];
    int ret = 1;
    int sockfd = -1;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    WOLFSSL_SESSION* session = NULL;
    char recvBuf[256];
    int len;
    int earlyDataSent = 0;

    /* Initialize wolfSSL */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init failed\n");
        goto cleanup;
    }

    /* Create and configure context */
    ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
    if (!ctx) {
        fprintf(stderr, "wolfSSL_CTX_new failed\n");
        goto cleanup;
    }

    if (wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS ||
        wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS ||
        wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Failed to load cert/key/CA\n");
        goto cleanup;
    }

    /* === 1st connection: perform handshake and get session ticket === */
    sockfd = tcp_connect(server_ip, DEFAULT_PORT);
    if (sockfd < 0) goto cleanup;

    ssl = wolfSSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "wolfSSL_new failed\n");
        goto cleanup;
    }
    if (wolfSSL_set_fd(ssl, sockfd) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_set_fd failed\n");
        goto cleanup;
    }

    if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_connect failed\n");
        goto cleanup;
    }

    /* Check if ticket was received */
    if (!wolfSSL_SessionIsSetup(wolfSSL_SSL_get0_session(ssl))) {
        /* Attempt to read a session ticket from server */
        (void)wolfSSL_peek(ssl, recvBuf, 0);
        if (!wolfSSL_SessionIsSetup(wolfSSL_SSL_get0_session(ssl))) {
            fprintf(stderr, "Session ticket not received from server\n");
            goto cleanup;
        }
    }

    /* Save session for resumption */
    session = wolfSSL_get1_session(ssl);
    if (!session) {
        fprintf(stderr, "wolfSSL_get1_session failed\n");
        goto cleanup;
    }
    
    len = wolfSSL_read(ssl, recvBuf, sizeof(recvBuf) - 1);
    if (len > 0) {
        recvBuf[len] = '\0';
        printf("Server sent: %s\n", recvBuf);
    }

    printf("Initial handshake complete, session ticket obtained.\n");

    /* Clean up first connection */
    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    ssl = NULL;
    close(sockfd);
    sockfd = -1;

    /* === 2nd connection: resume session and send early data === */
    sockfd = tcp_connect(server_ip, DEFAULT_PORT);
    if (sockfd < 0) goto cleanup;

    ssl = wolfSSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "wolfSSL_new (2nd) failed\n");
        goto cleanup;
    }
    if (wolfSSL_set_fd(ssl, sockfd) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_set_fd (2nd) failed\n");
        goto cleanup;
    }

    if (wolfSSL_set_session(ssl, session) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_set_session failed\n");
        goto cleanup;
    }

    ret = wolfSSL_write_early_data(ssl, EARLY_DATA_MSG, EARLY_DATA_MSG_LEN, &earlyDataSent);
    if (ret == EARLY_DATA_MSG_LEN && earlyDataSent == EARLY_DATA_MSG_LEN) {
        printf("Sent early data: \"%s\"\n", EARLY_DATA_MSG);
    } else {
        fprintf(stderr, "wolfSSL_write_early_data failed: ret=%d sent=%d\n", ret, earlyDataSent);
        goto cleanup;
    }

    /* Complete handshake */
    if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_connect (2nd) failed\n");
        goto cleanup;
    }
    printf("Handshake complete after early data.\n");

    /* Read server response */
    memset(recvBuf, 0, sizeof(recvBuf));
    while ((len = wolfSSL_read(ssl, recvBuf, sizeof(recvBuf) - 1)) > 0)
        printf("Server replied: %s\n", recvBuf);

    ret = 0; /* Success */

cleanup:
    if (ssl) wolfSSL_free(ssl);
    if (session) wolfSSL_SESSION_free(session);
    if (ctx) wolfSSL_CTX_free(ctx);
    if (sockfd >= 0) close(sockfd);
    wolfSSL_Cleanup();
    return ret;
}
