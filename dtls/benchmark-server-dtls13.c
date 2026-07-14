/*
 * benchmark-server-dtls13.c
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#define USE_CERT_BUFFERS_256
#include <wolfssl/certs_test.h>

#define SERV_PORT   11111
#define PACKET_SIZE 15360  /* 15KB = 15 * 1024 */
#define DTLS_MAX_RECORD_SIZE 1300  /* Very small size to avoid DTLS datagram limit */

/* Write data in chunks to fit DTLS record size limits */
/* Loop over wolfSSL_write to send the 15KB packet in chunks */
static int dtls_write_chunked(WOLFSSL* ssl, const unsigned char* buf, int len)
{
    int total_written = 0;
    int remaining = len;
    const unsigned char* ptr = buf;

    while (remaining > 0) {
        int chunk_size = (remaining > DTLS_MAX_RECORD_SIZE) ? DTLS_MAX_RECORD_SIZE : remaining;
        int written = wolfSSL_write(ssl, ptr, chunk_size);

        if (written < 0) {
            return written;  /* Error */
        }

        if (written == 0) {
            /* Should not happen with blocking socket, but handle it */
            break;
        }

        total_written += written;
        remaining -= written;
        ptr += written;
    }

    return total_written;
}

/* Read data in chunks to handle DTLS record size limits */
static int dtls_read_chunked(WOLFSSL* ssl, unsigned char* buf, int len)
{
    int total_read = 0;
    int remaining = len;
    unsigned char* ptr = buf;

    while (remaining > 0) {
        int chunk_size = (remaining > DTLS_MAX_RECORD_SIZE) ? DTLS_MAX_RECORD_SIZE : remaining;
        int read = wolfSSL_read(ssl, ptr, chunk_size);

        if (read <= 0) {
            return read;  /* Error or EOF */
        }

        total_read += read;
        remaining -= read;
        ptr += read;
    }

    return total_read;
}

WOLFSSL_CTX*  ctx = NULL;
WOLFSSL*      ssl = NULL;
int           listenfd = INVALID_SOCKET;

static void sig_handler(const int sig);
static void free_resources(void);

int main(int argc, char** argv)
{
    int           exitVal = 1;
    struct sockaddr_in servAddr;
    struct sockaddr_in cliaddr;
    int           ret;
    int           err;
    int           recvLen = 0;
    socklen_t     cliLen;
    unsigned char buff[PACKET_SIZE];
    int           msgNum = 0;

    /* Initialize wolfSSL before assigning ctx */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init error.\n");
        return exitVal;
    }

    /* Set ctx to DTLS 1.3 */
    if ((ctx = wolfSSL_CTX_new(
#ifdef WOLFSSL_DTLS13
            wolfDTLSv1_3_server_method()
#else
            wolfDTLSv1_2_server_method()
#endif
            )) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        goto cleanup;
    }

    /* Load certificates from memory buffer */
    if (wolfSSL_CTX_load_verify_buffer(ctx, ca_ecc_cert_der_256, sizeof_ca_ecc_cert_der_256,
            WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error loading CA certificate from buffer.\n");
        goto cleanup;
    }
    if (wolfSSL_CTX_use_certificate_buffer(ctx, serv_ecc_der_256,
            sizeof_serv_ecc_der_256, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error loading server certificate from buffer.\n");
        goto cleanup;
    }
    if (wolfSSL_CTX_use_PrivateKey_buffer(ctx, ecc_key_der_256,
            sizeof_ecc_key_der_256, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error loading server private key from buffer.\n");
        goto cleanup;
    }

    /* Create a UDP/IP socket */
    if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket()");
        goto cleanup;
    }

    memset((char *)&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(SERV_PORT);

    /* Bind Socket */
    if (bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind()");
        goto cleanup;
    }

    signal(SIGINT, sig_handler);

    printf("DTLS 1.3 server listening on port %d\n", SERV_PORT);
    printf("Ready to receive %d byte packets\n", PACKET_SIZE);
    printf("Certificates: %s\n", "ECDSA");
    printf("Press Ctrl+C to stop\n\n");

    while (1) {
        printf("Awaiting client connection on port %d\n", SERV_PORT);

        cliLen = sizeof(cliaddr);
        ret = (int)recvfrom(listenfd, (char *)buff, sizeof(buff), MSG_PEEK,
                (struct sockaddr*)&cliaddr, &cliLen);

        if (ret < 0) {
            perror("recvfrom()");
            goto cleanup;
        }
        else if (ret == 0) {
            fprintf(stderr, "recvfrom zero return\n");
            goto cleanup;
        }

        /* Create the WOLFSSL Object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "wolfSSL_new error.\n");
            goto cleanup;
        }

        if (wolfSSL_dtls_set_peer(ssl, &cliaddr, cliLen) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_dtls_set_peer error.\n");
            goto cleanup;
        }

        if (wolfSSL_set_fd(ssl, listenfd) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_set_fd error.\n");
            break;
        }

        if (wolfSSL_accept(ssl) != WOLFSSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "error = %d, %s\n", err,
                wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "SSL_accept failed.\n");
            goto cleanup;
        }

        /* No printf/perror in the receive/send loop to avoid affecting timing */
        while (1) {
            recvLen = dtls_read_chunked(ssl, buff, PACKET_SIZE);
            if (recvLen > 0) {
                msgNum++;

                /* Echo back the received data - do this immediately, no printf in between */
                /* Loop over wolfSSL_write to send the 15KB packet in chunks */
                if (dtls_write_chunked(ssl, buff, PACKET_SIZE) < 0) {
                    err = wolfSSL_get_error(ssl, 0);
                    /* Only log errors occasionally */
                    if (msgNum % 100 == 0) {
                        fprintf(stderr, "error = %d, %s\n", err,
                            wolfSSL_ERR_reason_error_string(err));
                        fprintf(stderr, "wolfSSL_write failed.\n");
                    }
                    /* Continue to next receive, don't exit */
                }
            }
            else if (recvLen <= 0) {
                err = wolfSSL_get_error(ssl, 0);
                if (err == WOLFSSL_ERROR_ZERO_RETURN) /* Received shutdown */
                    break;
                /* Only log errors occasionally */
                if (msgNum == 0 || (msgNum % 100 == 0)) {
                    fprintf(stderr, "error = %d, %s\n", err,
                        wolfSSL_ERR_reason_error_string(err));
                    fprintf(stderr, "SSL_read failed.\n");
                }
                /* Continue to next receive */
            }
        }

        /* Attempt a full shutdown */
        ret = wolfSSL_shutdown(ssl);
        if (ret == WOLFSSL_SHUTDOWN_NOT_DONE)
            ret = wolfSSL_shutdown(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "err = %d, %s\n", err,
                wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "wolfSSL_shutdown failed\n");
        }
        wolfSSL_free(ssl);
        ssl = NULL;

        printf("Connection closed. Awaiting new connection\n");
    }

    exitVal = 0;
cleanup:
    free_resources();
    wolfSSL_Cleanup();

    return exitVal;
}

static void sig_handler(const int sig)
{
    (void)sig;
    free_resources();
    wolfSSL_Cleanup();
    exit(0);
}

static void free_resources(void)
{
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
        ssl = NULL;
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
        ctx = NULL;
    }
    if (listenfd != INVALID_SOCKET) {
        close(listenfd);
        listenfd = INVALID_SOCKET;
    }
}
