/*
 * Copyright (C) 2024 wolfSSL Inc.
 *
 * This file is part of wolfHSM.
 *
 * TLS/DTLS Server Demo using wolfHSM for Crypto Operations
 *
 * This example demonstrates a server that offloads all cryptographic
 * operations to a wolfHSM server running on the POSIX transport with
 * DMA support. By default, DTLS is used but the code can be adapted
 * for TLS connections.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "server.h"

/* wolfHSM client includes */
#include "wolfhsm/wh_error.h"
#include "wolfhsm/wh_comm.h"
#include "wolfhsm/wh_client.h"
#include "wolfhsm/wh_client_cryptocb.h"
#include "port/posix/posix_transport_shm.h"

/* wolfSSL includes for crypto callback registration */
#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/wolfcrypt/cryptocb.h"

/* Shared POSIX example configuration */
#include "examples/posix/wh_posix_cfg.h"

/* Global wolfHSM client context */
static whClientContext g_client[1] = {{0}};
static posixTransportShmClientContext g_shm_client_ctx;
static posixTransportShmConfig g_shm_config;
static whCommClientConfig g_comm_config;
static whClientConfig g_client_config;

#ifdef WOLFHSM_CFG_DMA
/* DMA configuration for large data transfers */
static whClientDmaConfig g_dma_config;
#endif

/*
 * Connect to the wolfHSM server over shared memory DMA transport.
 *
 * This establishes a connection to the HSM server and registers the
 * crypto callback so that wolfSSL crypto operations are forwarded to
 * the HSM.
 */
static int connect_to_hsm_server(void)
{
    int ret;
    static const whTransportClientCb shm_cb = POSIX_TRANSPORT_SHM_CLIENT_CB;


    /* Initialize transport context */
    memset(&g_shm_client_ctx, 0, sizeof(g_shm_client_ctx));
    memset(&g_comm_config, 0, sizeof(g_comm_config));
    memset(&g_client_config, 0, sizeof(g_client_config));

    /* Configure shared memory transport with DMA */
    g_shm_config.name = WH_POSIX_SHARED_MEMORY_NAME;
    g_shm_config.req_size = WH_POSIX_REQ_SIZE;
    g_shm_config.resp_size = WH_POSIX_RESP_SIZE;
    g_shm_config.dma_size = WH_POSIX_DMA_SIZE;

    /* Configure comm layer */
    g_comm_config.transport_cb = &shm_cb;
    g_comm_config.transport_context = (void*)&g_shm_client_ctx;
    g_comm_config.transport_config = (void*)&g_shm_config;
    g_comm_config.client_id = WH_POSIX_CLIENT_ID;

#ifdef WOLFHSM_CFG_DMA
    /* Configure DMA callbacks for static memory operations */
    g_dma_config.cb = posixTransportShm_ClientStaticMemDmaCallback;
    g_dma_config.dmaAddrAllowList = NULL;
    g_client_config.dmaConfig = &g_dma_config;
#endif

    /* Configure client */
    g_client_config.comm = &g_comm_config;

    /* Initialize the client */
    ret = wh_Client_Init(g_client, &g_client_config);
    if (ret != WH_ERROR_OK) {
        fprintf(stderr, "Failed to initialize wolfHSM client: %d\n", ret);
        return -1;
    }

    /* Initialize communication with the server */
    ret = wh_Client_CommInit(g_client, NULL, NULL);
    if (ret != WH_ERROR_OK) {
        fprintf(stderr, "Failed to initialize wolfHSM client communication: %d\n", ret);
        wh_Client_Cleanup(g_client);
        return -1;
    }

    /* Register crypto callback for non-DMA operations */
    ret = wc_CryptoCb_RegisterDevice(WH_DEV_ID, wh_Client_CryptoCb,
                                     (void*)g_client);
    if (ret != 0) {
        fprintf(stderr, "Failed to register crypto callback: %d\n", ret);
        wh_Client_CommClose(g_client);
        wh_Client_Cleanup(g_client);
        return -1;
    }

#ifdef WOLFHSM_CFG_DMA
    /* Register crypto callback for DMA operations */
    ret = wc_CryptoCb_RegisterDevice(WH_DEV_ID_DMA, wh_Client_CryptoCbDma,
                                     (void*)g_client);
    if (ret != 0) {
        fprintf(stderr, "Failed to register DMA crypto callback: %d\n", ret);
        wc_CryptoCb_UnRegisterDevice(WH_DEV_ID);
        wh_Client_CommClose(g_client);
        wh_Client_Cleanup(g_client);
        return -1;
    }
#endif

    printf("Connected to wolfHSM server successfully\n");
    return 0;
}

/* Disconnect from wolfHSM server */
static void disconnect_from_hsm_server(void)
{
    printf("Disconnecting from wolfHSM server...\n");

#ifdef WOLFHSM_CFG_DMA
    wc_CryptoCb_UnRegisterDevice(WH_DEV_ID_DMA);
#endif
    wc_CryptoCb_UnRegisterDevice(WH_DEV_ID);

    wh_Client_CommClose(g_client);
    wh_Client_Cleanup(g_client);
}

/* Print usage information */
static void print_usage(const char* progname)
{
    printf("Usage: %s [options]\n", progname);
    printf("\n");
    printf("TLS/DTLS server demo using wolfHSM for crypto operations.\n");
    printf("By default, uses DTLS over UDP.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -A <file>    CA certificate file (PEM or DER format)\n");
    printf("               If not specified, uses built-in test certificate\n");
    printf("  -c <file>    Server certificate file (PEM or DER format)\n");
    printf("  -k <file>    Server private key file (PEM or DER format)\n");
    printf("  -p <port>    Port to listen on (default: 11111)\n");
    printf("  -h           Show this help message\n");
    printf("\n");
    printf("Prerequisites:\n");
    printf("  Start the wolfHSM server first:\n");
    printf("    cd examples/posix/wh_posix_server\n");
    printf("    ./Build/wh_posix_server.elf --type dma\n");
    printf("\n");
    printf("Example:\n");
    printf("  %s -A ca-cert.pem\n", progname);
    printf("\n");
}

/*
 * Main application entry point.
 *
 * This sets up the wolfHSM client connection, then initializes the
 * server which will use wolfHSM for all crypto operations.
 */
int main(int argc, char** argv)
{
    SERVER_CONTEXT* ctx;
    ServerConfig config = {0};
    int ret;
    int exit_code = 0;
    unsigned char buf[256];
    int bytesRead;
    int opt;

    /* Set default port */
    config.port = 11111;

    /* Parse command line arguments */
    while ((opt = getopt(argc, argv, "A:c:k:p:h")) != -1) {
        switch (opt) {
            case 'A':
                config.caCertFile = optarg;
                break;
            case 'c':
                config.serverCertFile = optarg;
                break;
            case 'k':
                config.serverKeyFile = optarg;
                break;
            case 'p':
                config.port = atoi(optarg);
                if (config.port <= 0 || config.port > 65535) {
                    fprintf(stderr, "Invalid port number: %s\n", optarg);
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    printf("DTLS server starting on port %d...\n", config.port);

    /* Initialize wolfCrypt */
    ret = wolfCrypt_Init();
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize wolfCrypt: %d\n", ret);
        return 1;
    }

    /* Connect to the wolfHSM server */
    if (connect_to_hsm_server() != 0) {
        fprintf(stderr, "Failed to connect to wolfHSM server\n");
        fprintf(stderr, "Make sure wh_posix_server is running with --type dma\n");
        wolfCrypt_Cleanup();
        return 1;
    }

    /* Get the server context and initialize it */
    ctx = Server_Get();
    ret = Server_Init(ctx, g_client, &config);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize server: %d\n", ret);
        exit_code = 1;
        goto cleanup;
    }

    /* Accept a connection */
    printf("Waiting for client...\n");
    ret = Server_Accept(ctx);
    if (ret != 0) {
        fprintf(stderr, "Failed to accept connection: %d\n", ret);
        exit_code = 1;
        goto cleanup;
    }

    printf("Connected. Entering echo mode...\n");
    while (1) {
        memset(buf, 0, sizeof(buf));
        bytesRead = Server_Read(ctx, buf, sizeof(buf) - 1);
        if (bytesRead <= 0) {
            printf("Client disconnected\n");
            break;
        }

        printf("Received %d bytes: %s\n", bytesRead, buf);

        /* Echo back */
        ret = Server_Write(ctx, buf, bytesRead);
        if (ret < 0) {
            fprintf(stderr, "Failed to write response\n");
            break;
        }
    }

cleanup:
    Server_Cleanup(ctx);
    disconnect_from_hsm_server();
    wolfCrypt_Cleanup();
    return exit_code;
}
