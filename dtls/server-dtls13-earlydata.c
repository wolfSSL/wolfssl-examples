/* server-dtls13-earlydata.c
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

/* Example DTLS 1.3 server using wolfSSL early data (0-RTT).
 * Receives early data from a client using wolfSSL_read_early_data().
 * It is recommended to build wolfSSL with WOLFSSL_DTLS13_NO_HRR_ON_RESUME
 * so that the server doesn't send a HelloRetryRequest when resuming sessions.
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
#define CERT_FILE    "../certs/server-cert.pem"
#define KEY_FILE     "../certs/server-key.pem"
#define CA_FILE      "../certs/client-cert.pem"

#define EARLY_DATA_BUF_SZ 256

#define EARLY_DATA_QUICK_REPLY "0.5-RTT data hello from early data DTLS server!"
#define EARLY_DATA_REPLY       "Normal data hello from early data DTLS server!"
#define EARLY_DATA_QUICK_REPLY_LEN (sizeof(EARLY_DATA_QUICK_REPLY) - 1)
#define EARLY_DATA_REPLY_LEN       (sizeof(EARLY_DATA_REPLY) - 1)

static int udp_listen(int port) {
    int sockfd;
    struct sockaddr_in servAddr;
    int on = 1;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
#ifdef SO_REUSEPORT
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&on, sizeof(on));
#endif

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind()");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main(int argc, char** argv)
{
    int ret = 1;
    int listenfd = -1;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;

    /* Initialize wolfSSL */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init failed\n");
        goto cleanup;
    }

    /* Create and configure context */
    ctx = wolfSSL_CTX_new(wolfDTLSv1_3_server_method());
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

    if (wolfSSL_CTX_set_max_early_data(ctx, 4096) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Failed to set max early data\n");
        goto cleanup;
    }

    /* Listen for connections */
    listenfd = udp_listen(DEFAULT_PORT);
    if (listenfd < 0) goto cleanup;

    printf("Listening on UDP port %d...\n", DEFAULT_PORT);

    while (1) {
        char earlyDataBuf[EARLY_DATA_BUF_SZ];
        int earlyDataLen = 0;
        int n;

        ssl = wolfSSL_new(ctx);
        if (!ssl) {
            fprintf(stderr, "wolfSSL_new failed\n");
            continue;
        }
        if (wolfSSL_set_fd(ssl, listenfd) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_set_fd failed\n");
            wolfSSL_free(ssl);
            continue;
        }
        if (wolfSSL_dtls13_no_hrr_on_resume(ssl, 1) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_dtls13_no_hrr_on_resume failed\n");
            wolfSSL_free(ssl);
            continue;
        }

        do {
            ret = wolfDTLS_accept_stateless(ssl);
            if (ret == WOLFSSL_FATAL_ERROR) {
                fprintf(stderr, "wolfDTLS_accept_stateless failed\n");
                goto cleanup;
            }
        } while (ret != WOLFSSL_SUCCESS);

        printf("Client connected, processing...\n");

        /* wolfSSL_read_early_data reads early data and advances the handshake */
        while ((n = wolfSSL_read_early_data(ssl, earlyDataBuf,
                sizeof(earlyDataBuf)-1, &earlyDataLen)) > 0) {
            if (earlyDataLen > 0) {
                earlyDataBuf[earlyDataLen] = '\0';
                printf("Received early data: \"%s\"\n", earlyDataBuf);
                /* Send 0.5-RTT data */
                if (wolfSSL_write(ssl, EARLY_DATA_QUICK_REPLY, EARLY_DATA_QUICK_REPLY_LEN) != EARLY_DATA_QUICK_REPLY_LEN) {
                    fprintf(stderr, "wolfSSL_write failed\n");
                } else {
                    printf("Sent 0.5 data reply to client.\n");
                }
            } else {
                printf("No early data received (n=%d, len=%d)\n", n, earlyDataLen);
            }
        }

        /* wolfSSL_read_early_data might have completed the handshake */
        if (!wolfSSL_is_init_finished(ssl)) {
            /* Complete handshake */
            if (wolfSSL_accept(ssl) != WOLFSSL_SUCCESS) {
                fprintf(stderr, "wolfSSL_accept failed\n");
                wolfSSL_free(ssl);
                continue;
            }
        }
        printf("Handshake complete.\n");
        
        if ((earlyDataLen = wolfSSL_read(ssl, earlyDataBuf, sizeof(earlyDataBuf)-1)) > 0) {
            earlyDataBuf[earlyDataLen] = '\0';
            printf("Received post-handshake data: \"%s\"\n", earlyDataBuf);
        }

        /* Send a reply */
        if (wolfSSL_write(ssl, EARLY_DATA_REPLY, EARLY_DATA_REPLY_LEN) != EARLY_DATA_REPLY_LEN) {
            fprintf(stderr, "wolfSSL_write failed\n");
        } else {
            printf("Sent reply to client.\n");
        }

        /* Attempt a full shutdown */
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
        ssl = NULL;
    }

    ret = 0;

cleanup:
    if (ssl) wolfSSL_free(ssl);
    if (ctx) wolfSSL_CTX_free(ctx);
    if (listenfd >= 0) close(listenfd);
    wolfSSL_Cleanup();
    return ret;
}
