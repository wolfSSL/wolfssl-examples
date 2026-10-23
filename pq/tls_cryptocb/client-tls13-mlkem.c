/* client-tls13-mlkem.c
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

/* TLS 1.3 client that offloads its ML-KEM key_share through a crypto callback.
 * The client generates the ML-KEM key pair (KEYGEN) and recovers the handshake
 * shared secret (DECAPS) via the registered device. */

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

#define CERT_FILE   "../../certs/client-cert.pem"
#define KEY_FILE    "../../certs/client-key.pem"
#define CA_FILE     "../../certs/ca-cert.pem"

int main(int argc, char** argv)
{
    int ret = 0;
#if defined(WOLF_CRYPTO_CB) && defined(WOLFSSL_HAVE_MLKEM)
    int                sockfd = -1;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;
    WOLFSSL_CTX*       ctx = NULL;
    WOLFSSL*           ssl = NULL;
    int                devId = 1; /* anything besides -2 (INVALID_DEVID) */
    MlKemCryptoCbCtx   myCtx;

    memset(&myCtx, 0, sizeof(myCtx));

    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        return 0;
    }

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

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port   = htons(DEFAULT_PORT);
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        ret = -1;
        goto exit;
    }

    if (connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        ret = -1;
        goto exit;
    }

    ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
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
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL))
                != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", CA_FILE);
        goto exit;
    }

    /* Route the crypto callback device to this context. */
    wolfSSL_CTX_SetDevId(ctx, devId);

    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto exit;
    }

    /* Request the ML-KEM-768 hybrid group (ECC P-256 + ML-KEM-768) for the key
     * share. This hybrid is on by default with --enable-mlkem; its ML-KEM half
     * is what dispatches to the crypto callback. For a pure (non-hybrid) group
     * such as WOLFSSL_ML_KEM_768, build wolfSSL with
     * --enable-tls-mlkem-standalone. */
    if ((ret = wolfSSL_UseKeyShare(ssl, WOLFSSL_SECP256R1MLKEM768))
                != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set ML-KEM key share group\n");
        goto exit;
    }

    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set the file descriptor\n");
        goto exit;
    }

    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        goto exit;
    }

    printf("Negotiated group: %s\n", wolfSSL_get_curve_name(ssl));
    printf("ML-KEM via crypto callback: keygen=%d decapsulate=%d\n",
           myCtx.keygenCount, myCtx.decapCount);

    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto exit;
    }
    len = strnlen(buff, sizeof(buff));

    if ((int)len != wolfSSL_write(ssl, buff, (int)len)) {
        fprintf(stderr, "ERROR: failed to write entire message\n");
        ret = -1;
        goto exit;
    }

    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(ssl, buff, sizeof(buff) - 1) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        ret = -1;
        goto exit;
    }
    printf("Server: %s\n", buff);

    /* The client must have generated and decapsulated through the callback. */
    if (myCtx.keygenCount < 1 || myCtx.decapCount < 1) {
        fprintf(stderr, "ERROR: ML-KEM did not route through the callback\n");
        ret = -1;
        goto exit;
    }

    ret = 0;

exit:
    if (ret == WOLFSSL_SUCCESS)
        ret = 0;
    if (ssl != NULL)
        wolfSSL_free(ssl);
    if (sockfd != -1)
        close(sockfd);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

#else
    (void)argc;
    (void)argv;
    printf("Please configure wolfSSL with "
           "--enable-cryptocb --enable-mlkem and try again\n");
#endif /* WOLF_CRYPTO_CB && WOLFSSL_HAVE_MLKEM */

    return ret;
}
