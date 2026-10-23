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

#include "mlkem-cryptocb.h"

#define DEFAULT_PORT 11111

#define CERT_FILE   "../../certs/server-cert.pem"
#define KEY_FILE    "../../certs/server-key.pem"

int main(void)
{
    int ret = 0;
#if defined(WOLF_CRYPTO_CB) && defined(WOLFSSL_HAVE_MLKEM)
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

    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to initialize the library\n");
        goto exit;
    }

    ret = wc_CryptoCb_RegisterDevice(devId, myMlKemCryptoCb, &myCtx);
    if (ret != 0) {
        fprintf(stderr, "ERROR: wc_CryptoCb_RegisterDevice failed %d\n", ret);
        goto exit;
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_port        = htons(DEFAULT_PORT);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto exit;
    }
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }

    ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
    if (ctx == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE,
                WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", CERT_FILE);
        goto exit;
    }
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE,
                WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", KEY_FILE);
        goto exit;
    }

    /* Route the crypto callback device to this context. */
    wolfSSL_CTX_SetDevId(ctx, devId);

    printf("Waiting for a connection...\n");
    if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size)) == -1) {
        fprintf(stderr, "ERROR: failed to accept the connection\n");
        ret = -1;
        goto exit;
    }

    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto exit;
    }
    wolfSSL_set_fd(ssl, connd);

    if ((ret = wolfSSL_accept(ssl)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: wolfSSL_accept error %d\n",
                wolfSSL_get_error(ssl, ret));
        goto exit;
    }

    printf("Negotiated group: %s\n", wolfSSL_get_curve_name(ssl));
    printf("ML-KEM via crypto callback: encapsulate=%d\n", myCtx.encapCount);

    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(ssl, buff, sizeof(buff) - 1) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        ret = -1;
        goto exit;
    }
    printf("Client: %s\n", buff);

    len = strlen(reply);
    if ((int)len != wolfSSL_write(ssl, reply, (int)len)) {
        fprintf(stderr, "ERROR: failed to write\n");
        ret = -1;
        goto exit;
    }

    /* The server must have encapsulated through the callback. */
    if (myCtx.encapCount < 1) {
        fprintf(stderr, "ERROR: ML-KEM did not route through the callback\n");
        ret = -1;
        goto exit;
    }

    ret = 0;

exit:
    if (ret == WOLFSSL_SUCCESS)
        ret = 0;
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
    printf("Please configure wolfSSL with "
           "--enable-cryptocb --enable-mlkem and try again\n");
#endif /* WOLF_CRYPTO_CB && WOLFSSL_HAVE_MLKEM */

    return ret;
}
