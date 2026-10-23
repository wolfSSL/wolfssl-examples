/* server-tls13-psk-with-certs.c
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

/* TLS 1.3 server using an external PSK *and* certificate authentication,
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

#ifdef HAVE_SIGNAL
#include <signal.h>
#endif

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>

#define DEFAULT_PORT 11111

#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"
#define CA_FILE   "../certs/client-cert.pem"

#if defined(WOLFSSL_TLS13) && defined(WOLFSSL_CERT_WITH_EXTERN_PSK) && \
    !defined(NO_PSK)

/* The external PSK shared with the client. Both sides must agree on the
 * identity, the key and the ciphersuite the key is bound to. */
#define PSK_IDENTITY    "Client_identity"
#define PSK_CIPHERSUITE "TLS13-AES128-GCM-SHA256"

static unsigned int my_psk_server_tls13_cb(WOLFSSL* ssl, const char* identity,
    unsigned char* key, unsigned int key_max_len, const char** ciphersuite)
{
    unsigned int i;
    unsigned int keySz = 32;

    (void)ssl;

    if (identity == NULL || strcmp(identity, PSK_IDENTITY) != 0)
        return 0;
    if (keySz > key_max_len)
        return 0;

    /* Dummy key - a real application must use a securely provisioned one */
    for (i = 0; i < keySz; i++)
        key[i] = (unsigned char)(i + 1);

    *ciphersuite = PSK_CIPHERSUITE;

    return keySz;
}

static int mSockfd = SOCKET_INVALID;
static int mConnd = SOCKET_INVALID;
static int mShutdown = 0;

#ifdef HAVE_SIGNAL
static void sig_handler(const int sig)
{
    fprintf(stderr, "SIGINT handled = %d.\n", sig);

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
#endif
#endif /* WOLFSSL_TLS13 && WOLFSSL_CERT_WITH_EXTERN_PSK && !NO_PSK */

int main(int argc, char** argv)
{
    int ret = 0;
#if defined(WOLFSSL_TLS13) && defined(WOLFSSL_CERT_WITH_EXTERN_PSK) && \
    !defined(NO_PSK)
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    const char*        reply = "I hear ya fa shizzle!\n";
    int                on;

    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

#ifdef HAVE_SIGNAL
    signal(SIGINT, sig_handler);
#endif

    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin_family      = AF_INET;
    servAddr.sin_port        = htons(DEFAULT_PORT);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if ((mSockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1; goto exit;
    }

    /* make sure server is setup for reuse addr/port */
    on = 1;
    setsockopt(mSockfd, SOL_SOCKET, SO_REUSEADDR,
            (char*)&on, (socklen_t)sizeof(on));
#ifdef SO_REUSEPORT
    setsockopt(mSockfd, SOL_SOCKET, SO_REUSEPORT,
               (char*)&on, (socklen_t)sizeof(on));
#endif

    if (bind(mSockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1; goto exit;
    }

    if (listen(mSockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1; goto exit;
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

    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1; goto exit;
    }

    /* Set up the external PSK */
    wolfSSL_CTX_set_psk_server_tls13_callback(ctx, my_psk_server_tls13_cb);
    wolfSSL_CTX_use_psk_identity_hint(ctx, "wolfssl server");

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

    wolfSSL_CTX_set_verify(ctx,
        WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

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

    while (!mShutdown) {
        printf("Waiting for a connection...\n");

        if ((mConnd = accept(mSockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1; goto exit;
        }

        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1; goto exit;
        }

        wolfSSL_set_fd(ssl, mConnd);

        if ((ret = wolfSSL_accept(ssl)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_accept error = %d\n",
                wolfSSL_get_error(ssl, ret));
            goto exit;
        }

        printf("Client connected successfully using %s\n",
               wolfSSL_get_cipher(ssl));

        memset(buff, 0, sizeof(buff));
        if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) < 0) {
            fprintf(stderr, "ERROR: failed to read\n");
            goto exit;
        }

        printf("Client: %s\n", buff);

        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            mShutdown = 1;
        }

        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            goto exit;
        }

        wolfSSL_shutdown(ssl);
        if (ssl) {
            wolfSSL_free(ssl);
            ssl = NULL;
        }
        if (mConnd != SOCKET_INVALID) {
            close(mConnd);
            mConnd = SOCKET_INVALID;
        }
    }

    printf("Shutdown complete\n");
    ret = 0;

exit:
    if (ssl)
        wolfSSL_free(ssl);
    if (mConnd != SOCKET_INVALID) {
        close(mConnd);
        mConnd = SOCKET_INVALID;
    }
    if (mSockfd != SOCKET_INVALID) {
        close(mSockfd);
        mSockfd = SOCKET_INVALID;
    }
    if (ctx)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

#else
    printf("Example requires TLS v1.3, PSK and "
           "--enable-cert-with-extern-psk\n");
#endif /* WOLFSSL_TLS13 && WOLFSSL_CERT_WITH_EXTERN_PSK && !NO_PSK */

    (void)argc;
    (void)argv;

    return ret;
}
