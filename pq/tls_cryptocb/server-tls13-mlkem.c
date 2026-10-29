/* server-tls13-mlkem.c
 *
 * Copyright (C) 2025 wolfSSL Inc.
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

/* TLS 1.3 server that offloads the ML-KEM key_share through a crypto callback.
 * The server encapsulates to the client's ML-KEM public key (ENCAPS) via the
 * registered device. It accepts a single connection, then exits. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>

#include "../mlkem-cryptocb.h"

#define DEFAULT_PORT 11111

#define CERT_FILE   "../../certs/server-cert.pem"
#define KEY_FILE    "../../certs/server-key.pem"

int main(void)
{
    int ret = 0;
#if defined(MLKEM_CRYPTOCB_ENABLED) && defined(WOLFSSL_TLS13)
    int                err = 0;
    int                sockfd = -1;
    int                connd = -1;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    const char*        reply = "I hear ya fa shizzle!\n";
    int                on = 1;
    WOLFSSL_CTX*       ctx = NULL;
    WOLFSSL*           ssl = NULL;
    int                devId = 1; /* anything besides -2 (INVALID_DEVID) */
    MlKemCryptoCbCtx   myCtx;

    memset(&myCtx, 0, sizeof(myCtx));

    /* Process exit status stays nonzero until every check below passes. Several
     * TLS setup calls return WOLFSSL_FAILURE (0) on error, so the raw return
     * value cannot be used as the exit status. */
    ret = 1;

    if ((err = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to initialize the library\n");
        goto exit;
    }

    err = wc_CryptoCb_RegisterDevice(devId, myMlKemCryptoCb, &myCtx);
    if (err != 0) {
        fprintf(stderr, "ERROR: wc_CryptoCb_RegisterDevice failed %d\n", err);
        goto exit;
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        goto exit;
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_port        = htons(DEFAULT_PORT);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        goto exit;
    }
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        goto exit;
    }

    ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
    if (ctx == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        goto exit;
    }

    if ((err = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE,
                WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", CERT_FILE);
        goto exit;
    }
    if ((err = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE,
                WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", KEY_FILE);
        goto exit;
    }

    /* Route the crypto callback device to this context. */
    wolfSSL_CTX_SetDevId(ctx, devId);

    printf("Waiting for a connection...\n");
    if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size)) == -1) {
        fprintf(stderr, "ERROR: failed to accept the connection\n");
        goto exit;
    }

    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        goto exit;
    }
    if ((err = wolfSSL_set_fd(ssl, connd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set the file descriptor\n");
        goto exit;
    }

    if ((err = wolfSSL_accept(ssl)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: wolfSSL_accept error %d\n",
                wolfSSL_get_error(ssl, err));
        goto exit;
    }

    printf("Negotiated group: %s\n", wolfSSL_get_curve_name(ssl));
    printf("ML-KEM via crypto callback: encapsulate=%d\n", myCtx.encapCount);

    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(ssl, buff, sizeof(buff) - 1) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto exit;
    }
    printf("Client: %s\n", buff);

    len = strlen(reply);
    if ((int)len != wolfSSL_write(ssl, reply, (int)len)) {
        fprintf(stderr, "ERROR: failed to write\n");
        goto exit;
    }

    /* The server must have encapsulated through the callback. */
    if (myCtx.encapCount < 1) {
        fprintf(stderr, "ERROR: ML-KEM did not route through the callback\n");
        goto exit;
    }

    /* Everything passed: report success. */
    ret = 0;

exit:
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (connd != -1)
        close(connd);
    if (sockfd != -1)
        close(sockfd);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

#else
    printf("Please configure wolfSSL with --enable-cryptocb --enable-mlkem "
           "and TLS 1.3 enabled, and try again\n");
#endif /* MLKEM_CRYPTOCB_ENABLED && WOLFSSL_TLS13 */

    return ret;
}
