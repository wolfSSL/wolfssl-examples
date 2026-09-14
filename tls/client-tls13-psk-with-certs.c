/* client-tls13-psk-with-certs.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

/* TLS 1.3 client using an external PSK *and* certificate authentication,
 * as described by RFC 9973 (which obsoletes the experimental RFC 8773).
 *
 * Requires wolfSSL built with:
 *     ./configure --enable-psk --enable-cert-with-extern-psk
 *
 * The feature is turned on with wolfSSL_CTX_set_cert_with_extern_psk(ctx, 1)
 * (or wolfSSL_set_cert_with_extern_psk(ssl, 1) for a single session).
 * Note that (EC)DHE key exchange must be used with the PSK - do not call
 * wolfSSL_CTX_no_dhe_psk().
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

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>

#define DEFAULT_PORT 11111

#define CERT_FILE "../certs/client-cert.pem"
#define KEY_FILE  "../certs/client-key.pem"
#define CA_FILE   "../certs/ca-cert.pem"

#if defined(WOLFSSL_TLS13) && defined(WOLFSSL_CERT_WITH_EXTERN_PSK) && \
    !defined(NO_PSK)

/* The external PSK shared with the server. Both sides must agree on the
 * identity, the key and the ciphersuite the key is bound to. */
#define PSK_IDENTITY   "Client_identity"
#define PSK_CIPHERSUITE "TLS13-AES128-GCM-SHA256"

static unsigned int my_psk_client_tls13_cb(WOLFSSL* ssl, const char* hint,
    char* identity, unsigned int id_max_len, unsigned char* key,
    unsigned int key_max_len, const char** ciphersuite)
{
    unsigned int i;
    unsigned int keySz = 32;

    (void)ssl;
    (void)hint;

    if (strlen(PSK_IDENTITY) + 1 > id_max_len || keySz > key_max_len)
        return 0;

    strcpy(identity, PSK_IDENTITY);

    /* Dummy key - a real application must use a securely provisioned one */
    for (i = 0; i < keySz; i++)
        key[i] = (unsigned char)(i + 1);

    *ciphersuite = PSK_CIPHERSUITE;

    return keySz;
}
#endif /* WOLFSSL_TLS13 && WOLFSSL_CERT_WITH_EXTERN_PSK && !NO_PSK */

int main(int argc, char** argv)
{
    int ret = 0;
#if defined(WOLFSSL_TLS13) && defined(WOLFSSL_CERT_WITH_EXTERN_PSK) && \
    !defined(NO_PSK)
    int                sockfd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;

    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        return 0;
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1; goto exit;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;
    servAddr.sin_port   = htons(DEFAULT_PORT);

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        ret = -1; goto exit;
    }

    if ((ret = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)))
         == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto exit;
    }

    /*---------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------*/
#if 0
    wolfSSL_Debugging_ON();
#endif

    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto exit;
    }

    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1; goto exit;
    }

    /* Set up the external PSK */
    wolfSSL_CTX_set_psk_client_tls13_callback(ctx, my_psk_client_tls13_cb);

    /* Ask for certificate messages to also be used with the external PSK. */
    if ((ret = wolfSSL_CTX_set_cert_with_extern_psk(ctx, 1))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to enable cert_with_extern_psk\n");
        goto exit;
    }

    if ((ret = wolfSSL_CTX_set_cipher_list(ctx, PSK_CIPHERSUITE))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set cipher list\n");
        goto exit;
    }

    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE,
        WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto exit;
    }

    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE,
        WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        goto exit;
    }

    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL))
         != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CA_FILE);
        goto exit;
    }

    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1; goto exit;
    }

    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto exit;
    }

    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL, err = %d\n",
                wolfSSL_get_error(ssl, ret));
        goto exit;
    }

    printf("Connected using %s\n", wolfSSL_get_cipher(ssl));

    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1; goto exit;
    }
    len = strnlen(buff, sizeof(buff));

    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to write entire message\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto exit;
    }

    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) < 0) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto exit;
    }

    printf("Server: %s\n", buff);

    ret = 0;

exit:
    if (sockfd != SOCKET_INVALID)
        close(sockfd);
    if (ssl)
        wolfSSL_free(ssl);
    if (ctx)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
#else
    printf("Example requires TLS v1.3, PSK and "
           "--enable-cert-with-extern-psk\n");
#endif
    (void)argc;
    (void)argv;

    return ret;
}
