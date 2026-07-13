/*
 * Copyright (C) 2024 wolfSSL Inc.
 *
 * This file is part of wolfHSM.
 *
 * Server I/O functions for the TLS/DTLS server demo.
 *
 * This module handles TLS/DTLS I/O operations. By default, DTLS is used.
 * The key integration point with wolfHSM is setting the device ID (WH_DEV_ID)
 * on the wolfSSL context so that crypto operations are forwarded to the HSM.
 */

#include "server.h"

/* wolfSSL includes */
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/cryptocb.h>

/* wolfHSM client includes */
#include "wolfhsm/wh_client.h"

/* Standard includes */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/* Test certificate buffers from wolfSSL for demo purposes */
#undef USE_CERT_BUFFERS_2048
#define USE_CERT_BUFFERS_2048
#undef USE_CERT_BUFFERS_256
#define USE_CERT_BUFFERS_256
#include "wolfssl/certs_test.h"

/* Default server port */
#define SERVER_PORT_DEFAULT 11111

/* Static wolfSSL context - shared across connections */
static WOLFSSL_CTX* g_ctx = NULL;

/* Static configuration - set during init */
static ServerConfig g_config = {0};

/* Server context structure */
struct SERVER_CONTEXT {
    WOLFSSL*           ssl;
    whClientContext*   hsm_client;
    int                listenfd;
    struct sockaddr_in cliaddr;
    socklen_t          cliLen;
    int                port;
};

/* Single static instance of the server */
static SERVER_CONTEXT g_server = {
    .ssl = NULL,
    .hsm_client = NULL,
    .listenfd = -1,
    .port = 0
};

/*
 * Initialize wolfSSL library and create a context.
 *
 * Uses DTLS server method with version negotiation (prefers DTLS 1.3).
 * This can be changed to TLS by modifying the method call below.
 *
 * The key here is setting the device ID to WH_DEV_ID so that all
 * crypto operations are forwarded to the wolfHSM server.
 */
static int initialize_wolfssl(WOLFSSL_CTX** ctx, whClientContext* hsm_client)
{
    int ret = 0;

    ret = wolfSSL_Init();
    if (ret != WOLFSSL_SUCCESS) {
        printf("Failed to initialize wolfSSL, ret = %d\n", ret);
        return -1;
    }

    /* Enable debug output */
    wolfSSL_Debugging_ON();

    /* Create a new context for server - DTLS with version negotiation
     * This will use DTLS 1.3 if both sides support it, otherwise falls back.
     * Note: For DTLS 1.3-only, use wolfDTLSv1_3_server_method() but ensure
     * the crypto callback supports TLS 1.3 key exchange operations. */
    *ctx = wolfSSL_CTX_new(wolfDTLS_server_method());
    if (*ctx == NULL) {
        printf("Failed to create wolfSSL context\n");
        return -1;
    }

    /* @TODO have an option for WH_DEV_ID_DMA */
    wolfSSL_CTX_SetDevId(*ctx, WH_DEV_ID);

    /* Load CA certificate for client verification */
    if (g_config.caCertFile != NULL) {
        /* wolfSSL_CTX_load_verify_locations auto-detects PEM vs DER format */
        ret = wolfSSL_CTX_load_verify_locations(*ctx, g_config.caCertFile, NULL);
        if (ret != WOLFSSL_SUCCESS) {
            printf("Failed to load CA certificate from %s, ret = %d\n",
                   g_config.caCertFile, ret);
            wolfSSL_CTX_free(*ctx);
            *ctx = NULL;
            return -1;
        }
    }
    else {
        /* Use built-in test certificate */
        ret = wolfSSL_CTX_load_verify_buffer(*ctx, ca_cert_der_2048,
                                             sizeof_ca_cert_der_2048,
                                             WOLFSSL_FILETYPE_ASN1);
        if (ret != WOLFSSL_SUCCESS) {
            printf("Failed to load built-in CA certificate, ret = %d\n", ret);
            wolfSSL_CTX_free(*ctx);
            *ctx = NULL;
            return -1;
        }
    }

    /* Load server certificate */
    if (g_config.serverCertFile != NULL) {
        ret = wolfSSL_CTX_use_certificate_file(*ctx, g_config.serverCertFile,
                                               WOLFSSL_FILETYPE_PEM);
        if (ret != WOLFSSL_SUCCESS) {
            /* Try DER format */
            ret = wolfSSL_CTX_use_certificate_file(*ctx, g_config.serverCertFile,
                                                   WOLFSSL_FILETYPE_ASN1);
        }
        if (ret != WOLFSSL_SUCCESS) {
            printf("Failed to load server certificate from %s, ret = %d\n",
                   g_config.serverCertFile, ret);
            wolfSSL_CTX_free(*ctx);
            *ctx = NULL;
            return -1;
        }
    }
    else {
        ret = wolfSSL_CTX_use_certificate_buffer(*ctx, server_cert_der_2048,
                                                 sizeof_server_cert_der_2048,
                                                 WOLFSSL_FILETYPE_ASN1);
        if (ret != WOLFSSL_SUCCESS) {
            printf("Failed to load built-in server certificate, ret = %d\n", ret);
            wolfSSL_CTX_free(*ctx);
            *ctx = NULL;
            return -1;
        }
    }

    /* Load server private key */
    if (g_config.serverKeyFile != NULL) {
        ret = wolfSSL_CTX_use_PrivateKey_file(*ctx, g_config.serverKeyFile,
                                              WOLFSSL_FILETYPE_PEM);
        if (ret != WOLFSSL_SUCCESS) {
            /* Try DER format */
            ret = wolfSSL_CTX_use_PrivateKey_file(*ctx, g_config.serverKeyFile,
                                                  WOLFSSL_FILETYPE_ASN1);
        }
        if (ret != WOLFSSL_SUCCESS) {
            printf("Failed to load server private key from %s, ret = %d\n",
                   g_config.serverKeyFile, ret);
            wolfSSL_CTX_free(*ctx);
            *ctx = NULL;
            return -1;
        }
    }
    else {
        ret = wolfSSL_CTX_use_PrivateKey_buffer(*ctx, server_key_der_2048,
                                                sizeof_server_key_der_2048,
                                                WOLFSSL_FILETYPE_ASN1);
        if (ret != WOLFSSL_SUCCESS) {
            printf("Failed to load built-in server private key, ret = %d\n", ret);
            wolfSSL_CTX_free(*ctx);
            *ctx = NULL;
            return -1;
        }
    }

    /* Require mutual authentication */
    wolfSSL_CTX_set_verify(*ctx,
                           WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                           NULL);

    (void)hsm_client;
    return 0;
}

/*
 * Initialize the UDP socket for DTLS.
 * For TLS, this would need to be changed to TCP socket initialization.
 */
static int initialize_udp_socket(SERVER_CONTEXT* ctx)
{
    int                ret;
    struct sockaddr_in servAddr;
    char               peekBuf[1];
    int                optval = 1;

    /* Create a UDP socket */
    ctx->listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->listenfd < 0) {
        perror("socket()");
        return -1;
    }

    /* Allow socket reuse */
    ret = setsockopt(ctx->listenfd, SOL_SOCKET, SO_REUSEADDR, &optval,
                     sizeof(optval));
    if (ret < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(ctx->listenfd);
        ctx->listenfd = -1;
        return -1;
    }

    /* Bind to all interfaces on the specified port */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(ctx->port);

    ret = bind(ctx->listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr));
    if (ret < 0) {
        perror("bind()");
        close(ctx->listenfd);
        ctx->listenfd = -1;
        return -1;
    }

    /* Wait for the first packet to get client address */
    printf("Waiting for client on port %d...\n", ctx->port);

    ctx->cliLen = sizeof(ctx->cliaddr);
    ret = recvfrom(ctx->listenfd, peekBuf, sizeof(peekBuf), MSG_PEEK,
                   (struct sockaddr*)&ctx->cliaddr, &ctx->cliLen);
    if (ret < 0) {
        perror("recvfrom()");
        close(ctx->listenfd);
        ctx->listenfd = -1;
        return -1;
    }

    printf("Received initial packet from %s:%d\n",
           inet_ntoa(ctx->cliaddr.sin_addr),
           ntohs(ctx->cliaddr.sin_port));

    return 0;
}

/*
 * Set up the WOLFSSL object for accepting a connection.
 * For DTLS, this includes setting the peer address.
 */
static int setup_ssl_accept(SERVER_CONTEXT* ctx)
{
    ctx->ssl = wolfSSL_new(g_ctx);
    if (ctx->ssl == NULL) {
        fprintf(stderr, "wolfSSL_new error - check debug output above\n");
        return -1;
    }

    /* Set the peer address (DTLS-specific) */
    if (wolfSSL_dtls_set_peer(ctx->ssl, &ctx->cliaddr,
                              ctx->cliLen) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_dtls_set_peer error\n");
        wolfSSL_free(ctx->ssl);
        ctx->ssl = NULL;
        return -1;
    }

    if (wolfSSL_set_fd(ctx->ssl, ctx->listenfd) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_set_fd error\n");
        wolfSSL_free(ctx->ssl);
        ctx->ssl = NULL;
        return -1;
    }

    return 0;
}

/*
 * Perform the TLS/DTLS handshake.
 *
 * All crypto operations during this handshake (key exchange, signatures,
 * encryption) will be performed by the wolfHSM server.
 */
static int perform_ssl_accept(SERVER_CONTEXT* ctx)
{
    int ret;

    printf("Starting handshake (crypto via wolfHSM)...\n");

    ret = wolfSSL_accept(ctx->ssl);
    if (ret != WOLFSSL_SUCCESS) {
        int err = wolfSSL_get_error(ctx->ssl, ret);
        fprintf(stderr, "wolfSSL_accept failed: error = %d, %s\n", err,
                wolfSSL_ERR_reason_error_string(err));
        return -1;
    }

    printf("Handshake successful!\n");
    printf("Cipher: %s\n", wolfSSL_get_cipher(ctx->ssl));

    return 0;
}

/* Public API implementations */

SERVER_CONTEXT* Server_Get(void)
{
    return &g_server;
}

int Server_Init(SERVER_CONTEXT* ctx, whClientContext* client,
                const ServerConfig* config)
{
    int ret;

    if (ctx == NULL || client == NULL) {
        return -1;
    }

    /* Store config in static variable for use by initialize_wolfssl */
    if (config != NULL) {
        g_config = *config;
    } else {
        memset(&g_config, 0, sizeof(g_config));
        g_config.port = SERVER_PORT_DEFAULT;
    }

    memset(ctx, 0, sizeof(SERVER_CONTEXT));
    ctx->listenfd = -1;  /* Initialize to invalid fd to prevent closing stdin */
    ctx->hsm_client = client;
    ctx->port = (g_config.port > 0) ? g_config.port : SERVER_PORT_DEFAULT;

    /* Initialize wolfSSL with HSM crypto offload */
    ret = initialize_wolfssl(&g_ctx, client);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize wolfSSL\n");
        return -1;
    }

    /* Initialize UDP socket and wait for client (DTLS mode) */
    ret = initialize_udp_socket(ctx);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize socket\n");
        wolfSSL_CTX_free(g_ctx);
        g_ctx = NULL;
        return -1;
    }

    /* Set up SSL for accepting connection */
    ret = setup_ssl_accept(ctx);
    if (ret != 0) {
        fprintf(stderr, "Failed to set up SSL accept\n");
        close(ctx->listenfd);
        ctx->listenfd = -1;
        wolfSSL_CTX_free(g_ctx);
        g_ctx = NULL;
        return -1;
    }

    return 0;
}

int Server_Accept(SERVER_CONTEXT* ctx)
{
    if (ctx == NULL || ctx->ssl == NULL) {
        return -1;
    }

    return perform_ssl_accept(ctx);
}

int Server_Read(SERVER_CONTEXT* ctx, unsigned char* data, size_t length)
{
    int ret;

    if (ctx == NULL || ctx->ssl == NULL || data == NULL) {
        return -1;
    }

    if (length > INT_MAX) {
        return -1;
    }

    ret = wolfSSL_read(ctx->ssl, data, (int)length);
    if (ret < 0) {
        int err = wolfSSL_get_error(ctx->ssl, ret);
        if (err != WOLFSSL_ERROR_WANT_READ) {
            fprintf(stderr, "wolfSSL_read failed: error = %d, %s\n", err,
                    wolfSSL_ERR_reason_error_string(err));
        }
        return -1;
    }

    return ret;
}

int Server_Write(SERVER_CONTEXT* ctx, unsigned char* data, size_t length)
{
    int ret;

    if (ctx == NULL || ctx->ssl == NULL || data == NULL) {
        return -1;
    }

    if (length > INT_MAX) {
        return -1;
    }

    ret = wolfSSL_write(ctx->ssl, data, (int)length);
    if (ret < 0) {
        int err = wolfSSL_get_error(ctx->ssl, ret);
        fprintf(stderr, "wolfSSL_write failed: error = %d, %s\n", err,
                wolfSSL_ERR_reason_error_string(err));
        return -1;
    }

    return ret;
}

void Server_Cleanup(SERVER_CONTEXT* ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->ssl != NULL) {
        wolfSSL_shutdown(ctx->ssl);
        wolfSSL_free(ctx->ssl);
        ctx->ssl = NULL;
    }

    if (ctx->listenfd >= 0) {
        close(ctx->listenfd);
        ctx->listenfd = -1;
    }

    if (g_ctx != NULL) {
        wolfSSL_CTX_free(g_ctx);
        g_ctx = NULL;
    }

    wolfSSL_Cleanup();
}

int Server_Close(SERVER_CONTEXT* ctx)
{
    Server_Cleanup(ctx);
    return 0;
}
