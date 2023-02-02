/* wolfssl_client.c 
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* Simple wolfSSL TLS client example demonstrating integration with SE050.
 *
 * This example uses SE050 hardware cryptography for cryptography operations
 * needed by wolfSSL for SSL/TLS (where supported), but does not store the
 * client certificate nor private key inside the SE050 module.
 *
 * For a more advanced example that demonstrates storing the client RSA or
 * ECC private key inside the SE050 and using a key ID to conduct crypto
 * operations using that key inside the module, please see the demo at:
 *
 * wolfssl_client_cert_key/wolfssl_client_cert_key.c
 *
 * This example has been set up to load a RSA client certificate and private
 * key, and by default configured to connect to the wolfSSL proper example
 * server. The wolfSSL example server ships with wolfSSL and by default is
 * compiled as part of the wolfSSL package. Example server should be started
 * with:
 *
 * $ ./examples/server/server
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/certs_test.h>
#include <wolfssl/wolfcrypt/port/nxp/se050_port.h>

#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <nxLog_App.h>

#define SERVER_IP "127.0.0.1"
#define DEFAULT_PORT 11111

#ifdef __cplusplus
}
#endif

#if defined(SIMW_DEMO_ENABLE__DEMO_WOLFSSL_CLIENT)

static ex_sss_boot_ctx_t gex_sss_boot_ctx;

static const char httpGetMsg[] = "GET /index.html HTTP/1.0\r\n\r\n";

#define EX_SSS_BOOT_PCONTEXT (&gex_sss_boot_ctx)
#define EX_SSS_BOOT_DO_ERASE 1
#define EX_SSS_BOOT_EXPOSE_ARGC_ARGV 0

#include <ex_sss_main_inc.h>

sss_status_t ex_sss_entry(ex_sss_boot_ctx_t *pCtx)
{
    sss_status_t status = kStatus_SSS_Success;
    sss_session_t *pSession = (sss_session_t*)&pCtx->session;
    sss_key_store_t *pKeyStore = (sss_key_store_t*)&pCtx->ks;

    int sockfd;
    int ret = WOLFSSL_SUCCESS;
    struct sockaddr_in servAddr;
    char buff[256];
    size_t len;
    word32 keyId = 0;
    char reply[256];

    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;

    LOG_I("wolfSSL example client\n");

    LOG_I("Running wc_se050_set_config()");
    if (wc_se050_set_config(pSession, NULL, pKeyStore) != 0) {
        LOG_E("wc_se050_set_config failed");
        ret = WOLFSSL_FAILURE;
    }
    else {
        LOG_I("SE050 config successfully set in wolfSSL");
    }

    /* Initialize wolfSSL library, enable debug logs if compiled in */
    wolfSSL_Init();
    wolfSSL_Debugging_ON();

    /* Create and set up socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        LOG_E("Failed to create socket");
        ret = WOLFSSL_FAILURE;
    }

    if (ret == WOLFSSL_SUCCESS) {
        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port   = htons(DEFAULT_PORT);

        if (inet_pton(AF_INET, SERVER_IP, &servAddr.sin_addr) != 1) {
            LOG_E("Invalid address, inet_pton failed");
            ret = WOLFSSL_FAILURE;
        }
    }

    /* Connect to server */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Created and configured socket");
        if (connect(sockfd, (struct sockaddr*) &servAddr,
                    sizeof(servAddr)) == -1) {
            LOG_E("failed to connect");
            ret = WOLFSSL_FAILURE;
        }
    }

    /* Make new SSL context */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Socket connected");
        ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
        if (ctx == NULL) {
            LOG_E("wolfSSL_CTX_new failed");
            ret = WOLFSSL_FAILURE;
        }
    }

    /* Load trusted CA cert to verify server, from buffer in certs_test.h */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Created WOLFSSL_CTX");
        ret = wolfSSL_CTX_load_verify_buffer(ctx, ca_cert_der_2048,
                sizeof_ca_cert_der_2048, SSL_FILETYPE_ASN1);
        if (ret != WOLFSSL_SUCCESS) {
            LOG_E("wolfSSL_CTX_load_verify_buffer failed");
        }
    }

    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Created WOLFSSL_CTX");
        ret = wolfSSL_CTX_load_verify_buffer(ctx, ca_ecc_cert_der_256,
                sizeof_ca_ecc_cert_der_256, SSL_FILETYPE_ASN1);
        if (ret != WOLFSSL_SUCCESS) {
            LOG_E("wolfSSL_CTX_load_verify_buffer failed");
        }
    }

    /* Load client cert into CTX from buffer in certs_test.h */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Loaded CA certs into CTX");
        ret = wolfSSL_CTX_use_certificate_buffer(ctx, client_cert_der_2048,
                sizeof_client_cert_der_2048, SSL_FILETYPE_ASN1);
        if (ret != WOLFSSL_SUCCESS) {
            LOG_E("wolfSSL_CTX_use_certificate_buffer failed");
        }
    }

    /* Load private key into CTX from buffer in certs_test.h */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Loaded client certificate into CTX");
        ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx, client_key_der_2048,
            sizeof_client_key_der_2048, SSL_FILETYPE_ASN1);
        if (ret != WOLFSSL_SUCCESS) {
            LOG_E("wolfSSL_CTX_use_PrivateKey_buffer failed");
        }
    }

    /* Create a WOLFSSL session */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Loaded client private key into CTX");
        ssl = wolfSSL_new(ctx);
        if (ssl == NULL) {
            LOG_E("wolfSSL_new failed");
            ret = WOLFSSL_FAILURE;
        }
    }

    /* Pass socket descriptor to wolfSSL session */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Created new WOLFSSL");
        ret = wolfSSL_set_fd(ssl, sockfd);
        if (ret != WOLFSSL_SUCCESS) {
            LOG_E("wolfSSL_set_fd failed");
        }
    }

    /* Do SSL/TLS handshake with peer */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Set wolfSSL fd");
        ret = wolfSSL_connect(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            LOG_E("wolfSSL_connect failed, err = %d", wolfSSL_get_error(ssl, ret));
        }
    }

    /* Send simple HTTP GET to server */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Sending message to server: %s\n", httpGetMsg);
        ret = wolfSSL_write(ssl, httpGetMsg, XSTRLEN(httpGetMsg));
        if (ret <= 0) {
            LOG_E("wolfSSL_write failed");
            ret = WOLFSSL_FAILURE;
        }
        else {
            LOG_I("wolfSSL_write sent %d bytes\n", ret);
            ret = WOLFSSL_SUCCESS;
        }
    }

    /* Read server response */
    if (ret == WOLFSSL_SUCCESS) {
        XMEMSET(reply, 0, sizeof(reply));
        ret = wolfSSL_read(ssl, reply, sizeof(reply)-1);
        if (ret <= 0) {
            LOG_E("wolfSSL_read failed");
            ret = WOLFSSL_FAILURE;
        }
        else {
            LOG_I("Server response: %s\n", reply);
            ret = WOLFSSL_SUCCESS;
        }
    }

    /* Bidirectional shutdown */
    if (ret == WOLFSSL_SUCCESS) {
        while (wolfSSL_shutdown(ssl) == SSL_SHUTDOWN_NOT_DONE) {
            LOG_I("TLS shutdown not complete");
        }
        LOG_I("TLS shutdown complete");
    }

    wolfSSL_free(ssl);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    close(sockfd);

    if (ret == WOLFSSL_FAILURE) {
        status = kStatus_SSS_Fail;
    }

    LOG_I("Done with sample app");
    return status;
}

#endif /* SIMW_DEMO_ENABLE__DEMO_WOLFSSL_CLIENT */
