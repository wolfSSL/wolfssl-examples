/*
 * Copyright (C) 2024 wolfSSL Inc.
 *
 * This file is part of wolfHSM.
 *
 * wolfHSM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfHSM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with wolfHSM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * wh_dtls_client.c
 *
 * wolfHSM DTLS Client Example
 *
 * This example demonstrates a DTLS client that uses wolfHSM as a cryptographic
 * backend. ECC signing operations are performed on the HSM - private keys
 * never leave the secure environment.
 *
 * The client makes two connections:
 * 1. TCP to wolfHSM server - for all cryptographic operations
 * 2. UDP to external DTLS server - for secure application communication
 *
 * Usage:
 *   1. Start wolfHSM server with client private key:
 *      ./wh_posix_server.elf --type tcp \
 *          --key <path-to-ecc-client-key.der> --id 1 --client 12
 *
 *   2. Start external DTLS server (from wolfssl directory):
 *      ./examples/server/server -u -v 3 \
 *          -c ./certs/server-ecc-cert.pem -k ./certs/ecc-key.pem \
 *          -A ./certs/client-ecc-cert.pem -V
 *
 *   3. Run this DTLS client:
 *      ./wh_dtls_client.elf 127.0.0.1
 *
 * Note: Client certificates are built-in from certs_test.h, eliminating
 *       the need for a fixed wolfssl directory location.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* wolfSSL headers */
#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/ssl.h"
#include "wolfssl/wolfcrypt/ecc.h"
#include "wolfssl/wolfcrypt/cryptocb.h"

/* Enable 256-bit ECC certificate buffers */
#define USE_CERT_BUFFERS_256
#include "wolfssl/certs_test.h"

/* wolfHSM headers */
#include "wolfhsm/wh_error.h"
#include "wolfhsm/wh_comm.h"
#include "wolfhsm/wh_client.h"
#include "wolfhsm/wh_client_crypto.h"
#include "wolfhsm/wh_client_cryptocb.h"

/* posix headers */
#include "wolfhsm/port/posix/posix_transport_tcp.h"

/* Configuration constants (wolfHSM posix default port wh_posix_cfg.h) */
#define HSM_POSIX_SERVER_TCP_PORT     23456
#define HSM_POSIX_SERVER_TCP_IPSTRING "127.0.0.1"
#define HSM_CLIENT_ID           12

/* DTLS server connection parameters */
#define DTLS_SERVER_PORT 11111
#define MAX_LINE         4096

/* Default HSM key ID for client private key (must match server --id parameter) */
#define HSM_KEY_ID 1

/* Context passed to crypto callback containing wolfHSM client info */
typedef struct {
    whClientContext* whClient;   /* wolfHSM client context */
    whKeyId          clientKeyId; /* HSM key ID for client private key */
} DtlsHsmCtx;

static DtlsHsmCtx hsmCtx;

static posixTransportTcpClientContext tccTcp;
static posixTransportTcpConfig        tcpConfig;
static whCommClientConfig             commConfig;
static whTransportClientCb            tcpCb = PTT_CLIENT_CB;

/* wolfHSM client configuration setup for TCP transport to wolfHSM server */
static int InitWolfHSMPosixTcpConfig(whClientConfig* conf)
{

    memset(&tccTcp, 0, sizeof(posixTransportTcpClientContext));
    memset(&commConfig, 0, sizeof(whCommClientConfig));

    tcpConfig.server_ip_string = HSM_POSIX_SERVER_TCP_IPSTRING;
    tcpConfig.server_port      = HSM_POSIX_SERVER_TCP_PORT;

    commConfig.transport_cb      = &tcpCb;
    commConfig.transport_context = (void*)&tccTcp;
    commConfig.transport_config  = (void*)&tcpConfig;
    commConfig.client_id         = HSM_CLIENT_ID;
    conf->comm                 = &commConfig;

    return WH_ERROR_OK;
}
/**
 * ECC Sign callback - called during TLS CertificateVerify
 *
 * This callback intercepts ECC signing operations during TLS handshake.
 * Instead of using a local private key, it delegates signing to the HSM
 * where the private key is stored.
 *
 * @param ssl     wolfSSL session pointer (unused)
 * @param in      Hash to sign
 * @param inSz    Size of hash
 * @param out     Output buffer for signature
 * @param outSz   Input/output signature size
 * @param keyDer  DER-encoded key (unused - we use HSM key)
 * @param keySz   Size of DER key (unused)
 * @param ctx     User context containing HSM client and key ID
 * @return        0 on success, negative on error
 */
static int myEccSignCb(WOLFSSL* ssl, const unsigned char* in, unsigned int inSz,
                       unsigned char* out, word32* outSz,
                       const unsigned char* keyDer, unsigned int keySz,
                       void* ctx)
{
    DtlsHsmCtx* hsmCtx = (DtlsHsmCtx*)ctx;
    ecc_key     eccKey[1];
    int         ret;
    uint16_t    sigLen;

    (void)ssl;
    (void)keyDer;
    (void)keySz; /* unused - we use HSM key */

    /* Initialize ECC key structure with HSM device ID */
    ret = wc_ecc_init_ex(eccKey, NULL, WH_DEV_ID);
    if (ret != 0) {
        printf("  EccSignCb: wc_ecc_init_ex failed: %d\n", ret);
        return ret;
    }

    /* Set curve parameters (P-256) */
    ret = wc_ecc_set_curve(eccKey, 32, ECC_SECP256R1);
    if (ret != 0) {
        printf("  EccSignCb: wc_ecc_set_curve failed: %d\n", ret);
        wc_ecc_free(eccKey);
        return ret;
    }

    /* Associate the HSM key ID with this ecc_key structure */
    ret = wh_Client_EccSetKeyId(eccKey, hsmCtx->clientKeyId);
    if (ret != 0) {
        printf("  EccSignCb: wh_Client_EccSetKeyId failed: %d\n", ret);
        wc_ecc_free(eccKey);
        return ret;
    }

    /* Perform signing on HSM - private key never leaves the HSM */
    sigLen = (uint16_t)*outSz;
    ret    = wh_Client_EccSign(hsmCtx->whClient, eccKey, in, (uint16_t)inSz, out,
                               &sigLen);
    if (ret == 0) {
        *outSz = sigLen;
    }
    else {
        printf("  EccSignCb: wh_Client_EccSign failed: %d\n", ret);
    }

    wc_ecc_free(eccKey);
    return ret;
}

static void Usage(const char* progName)
{
    printf("Usage: %s <DTLS server IP>\n", progName);
    printf("  DTLS server IP: IP address of the external DTLS server\n");
    printf("\nBefore running this client, start the servers in separate terminals:\n");
    printf("\n  Terminal 1 - Start wolfHSM server:\n");
    printf("    ./wh_posix_server.elf --type tcp \\\n");
    printf("        --key <path-to-ecc-client-key.der> --id 1 --client 12\n");
    printf("\n  Terminal 2 - Start wolfSSL DTLS server (from wolfssl directory):\n");
    printf("    ./examples/server/server -u -v 4 \\\n");
    printf("        -c ./certs/server-ecc.pem -k ./certs/ecc-key.pem \\\n");
    printf("        -A ./certs/client-ecc-cert.pem -p 11111 -i\n");
    printf("\n  Terminal 3 - Run this client:\n");
    printf("    %s 127.0.0.1\n", progName);
}

int main(int argc, char** argv)
{
    int             ret;
    int             sockfd     = -1;
    const char*     dtlsServerIp;
    whKeyId         hsmKeyId   = HSM_KEY_ID;
    struct sockaddr_in servAddr;
    char            sendLine[MAX_LINE];
    char            recvLine[MAX_LINE];
    int             n;

    /* wolfHSM client */
    whClientContext whClient[1];
    whClientConfig  whConfig[1];

    /* wolfSSL/DTLS */
    WOLFSSL_CTX*    ctx = NULL;
    WOLFSSL*        ssl = NULL;
    int             err;

    /* Parse command line arguments */
    if (argc != 2) {
        Usage(argv[0]);
        return 1;
    }
    dtlsServerIp = argv[1];

    printf("wolfHSM DTLS Client Example\n");
    printf("  HSM Key ID: %u\n", hsmKeyId);
    printf("  DTLS Server: %s:%d\n", dtlsServerIp, DTLS_SERVER_PORT);

    /*
     * Initialize wolfHSM client (TCP connection to HSM server)
     */
    printf("\n[1] Connecting to wolfHSM server...\n");
    memset(whConfig, 0, sizeof(whClientConfig));
    ret = InitWolfHSMPosixTcpConfig(whConfig);
    if (ret != 0) {
        printf("ERROR: wh_PosixDtlsClient_TcpConfig failed with %d\n", ret);
        return 1;
    }

    ret = wh_Client_Init(whClient, whConfig);
    if (ret != 0) {
        printf("ERROR: wh_Client_Init failed with %d\n", ret);
        return 1;
    }

    ret = wh_Client_CommInit(whClient, NULL, NULL);
    if (ret != 0) {
        printf("ERROR: wh_Client_CommInit failed with %d\n", ret);
        goto cleanup_whclient;
    }
    printf("  Connected to wolfHSM server\n");

    /*
     * Set up a global context to be used in the callbacks
     */
    hsmCtx.whClient    = whClient;
    hsmCtx.clientKeyId = hsmKeyId;

    /*
     * Initialize wolfSSL library
     */
    ret = wolfSSL_Init();
    if (ret != WOLFSSL_SUCCESS) {
        printf("ERROR: wolfSSL_Init failed\n");
        goto cleanup;
    }

    /*
     * Register wolfHSM crypto callback This callback will intercepts wolfCrypt
     * cryptographic operations and offloads them to HSM.
     */
    ret = wc_CryptoCb_RegisterDevice(WH_DEV_ID, wh_Client_CryptoCb, whClient);
    if (ret != 0) {
        printf("ERROR: wc_CryptoCb_RegisterDevice failed with %d\n", ret);
        goto cleanup;
    }
    printf("  Registered wolfHSM crypto callback (devId=0x%08X)\n", WH_DEV_ID);

    /*
     * Create DTLS client context
     */
    printf("\n[2] Creating DTLS context...\n");
    ctx = wolfSSL_CTX_new(wolfDTLS_client_method());
    if (ctx == NULL) {
        printf("ERROR: wolfSSL_CTX_new failed\n");
        goto cleanup;
    }

    /*
     * Configure crypto callback device ID
     * This tells wolfSSL to use our HSM for crypto operations
     */
    ret = wolfSSL_CTX_SetDevId(ctx, WH_DEV_ID);
    if (ret != WOLFSSL_SUCCESS) {
        printf("ERROR: wolfSSL_CTX_SetDevId failed\n");
        goto cleanup;
    }
    printf("  Configured HSM device ID for crypto operations\n");

    /*
     * Register PK callbacks for HSM operations
     * These callbacks intercept (D)TLS-level ECC operations (signing during
     * CertificateVerify) and delegate them to the
     * HSM. It allows to use the id of a private key stored on the HSM without
     * ever exposing the private key itself.
     */
    wolfSSL_CTX_SetEccSignCb(ctx, myEccSignCb);
    wolfSSL_CTX_SetEccSignCtx(ctx, &hsmCtx);
    printf("  Registered ECC PK callbacks for HSM operations\n");

    /*
     * Load CA certificate for verifying DTLS server
     */
    ret = wolfSSL_CTX_load_verify_buffer(ctx, ca_ecc_cert_der_256,
                                         sizeof_ca_ecc_cert_der_256,
                                         WOLFSSL_FILETYPE_ASN1);
    if (ret != WOLFSSL_SUCCESS) {
        printf("ERROR: Failed to load CA certificate buffer\n");
        goto cleanup;
    }
    printf("  Loaded CA certificate from buffer\n");

    /*
     * Load client certificate (public part only)
     * The private key is NOT loaded here - it stays on the HSM
     */
    ret = wolfSSL_CTX_use_certificate_buffer(ctx, cliecc_cert_der_256,
                                             sizeof_cliecc_cert_der_256,
                                             WOLFSSL_FILETYPE_ASN1);
    if (ret != WOLFSSL_SUCCESS) {
        printf("ERROR: Failed to load client certificate buffer\n");
        goto cleanup;
    }
    printf("  Loaded client certificate from buffer\n");

    /*
     * Create UDP socket for DTLS
     */
    printf("\n[3] Creating UDP socket...\n");
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("ERROR: Cannot create socket\n");
        goto cleanup;
    }

    /* Set up DTLS server address */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port   = htons(DTLS_SERVER_PORT);
    if (inet_pton(AF_INET, dtlsServerIp, &servAddr.sin_addr) != 1) {
        printf("ERROR: Invalid IP address: %s\n", dtlsServerIp);
        goto cleanup;
    }
    printf("  Created UDP socket\n");

    /*
     * Create DTLS session
     */
    printf("\n[4] Creating DTLS session...\n");
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("ERROR: wolfSSL_new failed\n");
        goto cleanup;
    }

    /* Configure DTLS peer and socket */
    ret = wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));
    if (ret != WOLFSSL_SUCCESS) {
        printf("ERROR: wolfSSL_dtls_set_peer failed\n");
        goto cleanup;
    }

    ret = wolfSSL_set_fd(ssl, sockfd);
    if (ret != WOLFSSL_SUCCESS) {
        printf("ERROR: wolfSSL_set_fd failed\n");
        goto cleanup;
    }
    printf("  DTLS session configured\n");

    /*
     * Step 12: Perform DTLS handshake
     */
    printf("\n[5] Performing DTLS handshake...\n");
    ret = wolfSSL_connect(ssl);
    if (ret != WOLFSSL_SUCCESS) {
        err = wolfSSL_get_error(ssl, ret);
        printf("ERROR: wolfSSL_connect failed: %d (%s)\n",
               err, wolfSSL_ERR_reason_error_string(err));
        goto cleanup;
    }
    printf("  DTLS handshake successful!\n");
    printf("  Cipher: %s\n", wolfSSL_get_cipher(ssl));

    /*
     * Application data exchange
     */
    printf("\n[6] Ready for application data\n");
    printf("Enter message to send (or empty line to quit): ");
    fflush(stdout);

    if (fgets(sendLine, sizeof(sendLine), stdin) != NULL) {
        size_t len = strlen(sendLine);

        /* Remove trailing newline if present */
        if (len > 0 && sendLine[len - 1] == '\n') {
            sendLine[len - 1] = '\0';
            len--;
        }

        if (len > 0) {
            /* Send message to server */
            ret = wolfSSL_write(ssl, sendLine, (int)len);
            if (ret != (int)len) {
                printf("ERROR: wolfSSL_write failed\n");
                goto cleanup;
            }
            printf("  Sent: %s\n", sendLine);

            /* Receive response from server */
            memset(recvLine, 0, sizeof(recvLine));
            n = wolfSSL_read(ssl, recvLine, sizeof(recvLine) - 1);
            if (n < 0) {
                err = wolfSSL_get_error(ssl, n);
                if (err != WOLFSSL_ERROR_WANT_READ) {
                    printf("ERROR: wolfSSL_read failed: %d\n", err);
                }
            }
            else {
                recvLine[n] = '\0';
                printf("  Received: %s\n", recvLine);
            }
        }
    }

    /*
     * Clean shutdown - we made it here so operation was successful
     */
    printf("\n[7] Closing connection...\n");
    wolfSSL_shutdown(ssl);
    printf("  DTLS connection closed\n");

    ret = WOLFSSL_SUCCESS;

cleanup:
    wolfSSL_free(ssl);
    if (sockfd >= 0) {
        close(sockfd);
    }
    wolfSSL_CTX_free(ctx);

    wolfSSL_Cleanup();

cleanup_whclient:
    wh_Client_CommClose(whClient);
    wh_Client_Cleanup(whClient);
    printf("  wolfHSM client disconnected\n");

    printf("\nDone.\n");
    return (ret == WOLFSSL_SUCCESS) ? 0 : 1;
}
