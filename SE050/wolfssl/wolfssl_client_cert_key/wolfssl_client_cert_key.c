/* wolfssl_client_cert_key.c 
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

/* Simple wolfSSL TLS client example demonstrating integration with SE050,
 * where client certificate and private key are stored in the SE050.
 *
 * Credential for this example have been set up to connect to the wolfSSL
 * proper example server. The wolfSSL example server ships with wolfSSL and
 * by default is compiled as part of the wolfSSL package. Example server
 * should be started with:
 *
 * RSA certs/keys (default for wolfSSL example server):
 *
 * $ ./examples/server/server
 *
 * ECC certs/keys:
 *
 * $ ./examples/server/server -c ./certs/server-ecc.pem -k ./certs/ecc-key.pem \
 *     -A ./certs/client-ecc-cert.pem
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/certs_test.h>
#include <wolfssl/wolfcrypt/port/nxp/se050_port.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/asn_public.h>

#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <nxLog_App.h>

/* Server endpoint IP and host, change if needed */
#define SERVER_IP "127.0.0.1"
#define DEFAULT_PORT 11111

/* This needs to be a signed int that isn't -2 */
#define HARDWARE_ID 10

#ifdef __cplusplus
}
#endif

#if defined(SIMW_DEMO_ENABLE__DEMO_WOLFSSL_CLIENT_CERT_KEY)

static ex_sss_boot_ctx_t gex_sss_boot_ctx;

/* Message to be sent to server */
static const char httpGetMsg[] = "GET /index.html HTTP/1.0\r\n\r\n";

#define EX_SSS_BOOT_PCONTEXT (&gex_sss_boot_ctx)
#define EX_SSS_BOOT_DO_ERASE 1
#define EX_SSS_BOOT_EXPOSE_ARGC_ARGV 0

/* SE050 key ID's to store example certs and keys into */
#define KEYID_RSA_PRIVATE  0x00005002 /* stores client_key_der_2048 */
#define KEYID_RSA_CERT     0x00005003 /* stores client_cert_der_2048 */
#define KEYID_ECC_PRIVATE  0x00005005 /* stores ecc_clikey_der_256 */
#define KEYID_ECC_CERT     0x00005006 /* stores cliecc_cert_der_256 */

/* Use RSA or ECC certs/keys with this example. Default is RSA, but set to
 * 0 to use ECC certs/keys (KEYID_ECC_PRIVATE/KEYID_ECC_CERT) instead. */
#define EXAMPLE_USE_RSA 1

/* Define to 1 to extract and load client certificate from SE050 key ID.
 * This example already loads the client certificate from C array / buffer
 * into the SE050 key ID slot KEYID_RSA_CERT or KEYID_ECC_CERT for demo
 * purposes. If this is defined to 0, the demo will not extract back out the
 * certificate from SE050 and instead load directly from the C array again
 * into wolfSSL. */
#define EXAMPLE_READ_CERT_FROM_SE050 1

#include <ex_sss_main_inc.h>

/**
 * Insert example certs/keys into SE050 key IDs. This functionality
 * might be done offline or at provisioning in production code. Provided
 * here for a complete example which includes storing the certs/keys into
 * SE050 module.
 *
 * Example certs/keys used here are distributed with wolfSSL in
 * <wolfssl/certs_test.h>. There are file equivalents to these credentials
 * under the "certs" directory in wolfSSL.
 *
 * Key ID             Credential
 * ------             --------------------------------
 * KEYID_RSA_PRIVATE  Client RSA private key (client_key_der_2048)
 * KEYID_RSA_CERT     Client RSA certificate (client_cert_der_248)
 * KEYID_ECC_PRIVATE  Client ECC private key (ecc_clikey_der_256)
 * KEYID_ECC_CERT     Client ECC certificate (cliecc_cert_der_256)
 *
 * Return 0 on success, negative on error.
 */
static int write_certs_keys_to_se050(void)
{
    int ret = 0;

#if EXAMPLE_USE_RSA == 1
    /* Insert RSA private key */
    ret = wc_se050_rsa_insert_private_key(KEYID_RSA_PRIVATE,
                                          client_key_der_2048,
                                          sizeof_client_key_der_2048);
    if (ret != 0) {
        LOG_I("Error in se050_rsa_insert_private_key, ret = %d\n", ret);
    } else {
        LOG_I("Inserted RSA private key into SE050 key ID: 0x%08x\n",
              KEYID_RSA_PRIVATE);
    }

    /* Insert RSA certificate */
    ret = wc_se050_insert_binary_object(KEYID_RSA_CERT,
                                        client_cert_der_2048,
                                        sizeof_client_cert_der_2048);
    if (ret != 0) {
        LOG_I("Error in wc_se050_insert_binary_object, ret = %d\n", ret);
    } else {
        LOG_I("Inserted RSA certificate into SE050 key ID: 0x%08x\n",
              KEYID_RSA_CERT);
    }

#else /* Load ECC cert/key */

    /* Insert ECC private key */
    ret = wc_se050_ecc_insert_private_key(KEYID_ECC_PRIVATE, ecc_clikey_der_256,
                                          sizeof_ecc_clikey_der_256);
    if (ret != 0) {
        LOG_I("Error in se050_ecc_insert_private_key, ret = %d\n", ret);
    } else {
        LOG_I("Inserted ECC private key into SE050 key ID: 0x%08x\n",
              KEYID_ECC_PRIVATE);
    }

    /* Insert ECC certificate */
    ret = wc_se050_insert_binary_object(KEYID_ECC_CERT, cliecc_cert_der_256,
                                        sizeof_cliecc_cert_der_256);
    if (ret != 0) {
        LOG_I("Error in wc_se050_insert_binary_object, ret = %d\n", ret);
    } else {
        LOG_I("Inserted ECC certificate into SE050 key ID: 0x%08x\n",
              KEYID_ECC_CERT);
    }

#endif /* EXAMPLE_USE_RSA */

    return ret;
}

/**
 * Erase sample certs/keys from SE050. This may not be needed in
 * production code, as credentials may need to remain stored in SE050.
 * Included here for complete example, and to cleanup SE050 use.
 *
 * Returns 0 on success, negative on error.
 */
static int erase_certs_keys_from_se050(void)
{
    int ret = 0;

#if EXAMPLE_USE_RSA == 1
    ret = wc_se050_erase_object(KEYID_RSA_PRIVATE);
    if (ret != 0) {
        LOG_I("Failed to erase RSA private key, ret = %d", ret);
    } else {
        LOG_I("Erased RSA private key, key ID: 0x%08x", KEYID_RSA_PRIVATE);
    }
    ret = wc_se050_erase_object(KEYID_RSA_CERT);
    if (ret != 0) {
        LOG_I("Failed to erase RSA certificate, ret = %d", ret);
    } else {
        LOG_I("Erased RSA certificate, key ID: 0x%08x", KEYID_RSA_CERT);
    }
#else
    ret = wc_se050_erase_object(KEYID_ECC_PRIVATE);
    if (ret != 0) {
        LOG_I("Failed to erase ECC private key, ret = %d", ret);
    } else {
        LOG_I("Erased ECC private key, key ID: 0x%08x", KEYID_ECC_PRIVATE);
    }
    ret = wc_se050_erase_object(KEYID_ECC_CERT);
    if (ret != 0) {
        LOG_I("Failed to erase ECC certificate, ret = %d", ret);
    } else {
        LOG_I("Erased ECC certificate, key ID: 0x%08x", KEYID_ECC_CERT);
    }
#endif /* EXAMPLE_USE_RSA */

    return ret;
}

/**
 * Main entry point for SE050 middleware demos.
 *
 * pCtx - initialized pointer to ex_sss_boot_ctx_t
 *
 * Returns kStatus_SSS_Success on success, kStatus_SSS_Fail on failure
 */
sss_status_t ex_sss_entry(ex_sss_boot_ctx_t *pCtx)
{
    sss_status_t status = kStatus_SSS_Success;
    sss_session_t *pSession = (sss_session_t*)&pCtx->session;
    sss_key_store_t *pKeyStore = (sss_key_store_t*)&pCtx->ks;

    int sockfd;
    int ret = WOLFSSL_SUCCESS;
    int credentialsStored = 0;
    int privateKeyId = 0;
    int cliCertKeyId = 0;
    struct sockaddr_in servAddr;
    char buff[256];
    size_t len;
    char reply[256];

#if EXAMPLE_READ_CERT_FROM_SE050 == 1
    unsigned char* tmpCert = NULL;
    unsigned int tmpCertSz = 0;
#endif

    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;

    LOG_I("------------------------------------------------------------------");
    LOG_I("wolfSSL example client");
#if EXAMPLE_USE_RSA == 1
    LOG_I("Using RSA certs/keys inside SE050");
#else
    LOG_I("Using ECC certs/keys inside SE050");
#endif
    LOG_I("------------------------------------------------------------------");

    /* Set private key ID based on demo config */
#if EXAMPLE_USE_RSA == 1
        privateKeyId = KEYID_RSA_PRIVATE;
        cliCertKeyId = KEYID_RSA_CERT;
#else
        privateKeyId = KEYID_ECC_PRIVATE;
        cliCertKeyId = KEYID_ECC_CERT;
#endif

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

    /* Write example certs/keys to SE050 */
    ret = write_certs_keys_to_se050();
    if (ret != 0) {
        LOG_E("Failed to write certs/keys to SE050");
        ret = WOLFSSL_FAILURE;
    }
    else {
        credentialsStored = 1;
        ret = WOLFSSL_SUCCESS;
    }

    /* Create and set up socket */
    if (ret == WOLFSSL_SUCCESS) {
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            LOG_E("Failed to create socket");
            ret = WOLFSSL_FAILURE;
        }
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

    /* Load trusted RSA or ECC CA cert to verify server, certs_test.h */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Created WOLFSSL_CTX");
#if EXAMPLE_USE_RSA == 1
        ret = wolfSSL_CTX_load_verify_buffer(ctx, ca_cert_der_2048,
                sizeof_ca_cert_der_2048, SSL_FILETYPE_ASN1);
#else /* use ECC cert */
        ret = wolfSSL_CTX_load_verify_buffer(ctx, ca_ecc_cert_der_256,
                sizeof_ca_ecc_cert_der_256, SSL_FILETYPE_ASN1);
#endif
        if (ret != WOLFSSL_SUCCESS) {
            LOG_E("wolfSSL_CTX_load_verify_buffer failed");
        }
    }

    /* Loading a Client/Server Certificate:
     *
     * This can be done in several different ways, and will be dependent on
     * end user desired usage. The client/server certificate can be stored
     * inside the SE050, but will need to be extracted back out in order to be
     * loaded into wolfSSL. Alternatively, the certificate can be stored/kept
     * in a buffer/array on the device (not SE050) and loaded from there.
     *
     * This example demonstrates both scenarios. When
     * EXAMPLE_READ_CERT_FROM_SE050 is defined to 1, the demo will pull the
     * client certificate out of the SE050 based on key ID where it has been
     * loaded, then load that into wolfSSL. If not defined, the demo will 
     * directly load an example client certificate from a C array in
     * <wolfssl/certs_test.h>, which matches the prviate key loaded into the
     * SE050. */
#if EXAMPLE_READ_CERT_FROM_SE050 == 1
    /* Read client cert from SE050 and load into WOLFSSL_CTX */
    if (ret == WOLFSSL_SUCCESS) {
        /* Get object size at SE050 key ID, to allocate memory */
        ret = wc_se050_get_binary_object(cliCertKeyId, tmpCert, &tmpCertSz);
        if (ret != LENGTH_ONLY_E) {
            LOG_E("Failed to get size of certificate object in SE050");
            ret = WOLFSSL_FAILURE;
        }
        else {
            ret = WOLFSSL_SUCCESS;
        }
    }

    if (ret == WOLFSSL_SUCCESS) {
        /* Allocate memory to read client cert */
        tmpCert = (byte*)XMALLOC(tmpCertSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (tmpCert == NULL) {
            LOG_E("Failed to allocate memory for client certificate");
            ret = WOLFSSL_FAILURE;
        }
    }

    if (ret == WOLFSSL_SUCCESS) {
        /* Read client cert into tmp buffer */
        XMEMSET(tmpCert, 0, tmpCertSz);

        ret = wc_se050_get_binary_object(cliCertKeyId, tmpCert, &tmpCertSz);
        if (ret != 0) {
            LOG_E("Failed to read client cert from SE050");
            ret = WOLFSSL_FAILURE;
        }
        else {
            ret = WOLFSSL_SUCCESS;
        }
    }

    if (ret == WOLFSSL_SUCCESS) {
        ret = wolfSSL_CTX_use_certificate_buffer(ctx, tmpCert, tmpCertSz,
                SSL_FILETYPE_ASN1);
        if (ret != WOLFSSL_SUCCESS) {
            LOG_E("wolfSSL_CTX_use_certificate_buffer failed");
        }
    }
#else
    /* Load client cert into CTX from buffer in certs_test.h */
    if (ret == WOLFSSL_SUCCESS) {
        LOG_I("Loaded CA certs into CTX");
    #if EXAMPLE_USE_RSA == 1
        ret = wolfSSL_CTX_use_certificate_buffer(ctx, client_cert_der_2048,
                sizeof_client_cert_der_2048, SSL_FILETYPE_ASN1);
    #else
        ret = wolfSSL_CTX_use_certificate_buffer(ctx, cliecc_cert_der_256,
                sizeof_cliecc_cert_der_256, SSL_FILETYPE_ASN1);
    #endif
        if (ret != WOLFSSL_SUCCESS) {
            LOG_E("wolfSSL_CTX_use_certificate_buffer failed");
        }
    }
#endif /* EXAMPLE_READ_CERT_FROM_SE050 */

    /* Set SE050 key slot to be used as client private key */
    if (ret == WOLFSSL_SUCCESS) {
        ret = wolfSSL_CTX_use_PrivateKey_Id(ctx,
                (const unsigned char*)&privateKeyId,
                sizeof(int), HARDWARE_ID);
        if (ret != WOLFSSL_SUCCESS) {
            LOG_E("wolfSSL_CTX_use_PrivateKey_Id failed");
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
            LOG_E("wolfSSL_connect failed, err = %d",
                  wolfSSL_get_error(ssl, ret));
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
            LOG_I("wolfSSL_write sent %d bytes", ret);
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

    /* Bidirectional SSL/TLS shutdown, close_notify alerts */
    if (ret == WOLFSSL_SUCCESS) {
        while (wolfSSL_shutdown(ssl) == SSL_SHUTDOWN_NOT_DONE) {
            LOG_I("TLS shutdown not complete");
        }
        LOG_I("TLS shutdown complete");
    }

    /* Free resources, close socket */
    wolfSSL_free(ssl);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    close(sockfd);

#if EXAMPLE_READ_CERT_FROM_SE050 == 1
    if (tmpCert != NULL) {
        XFREE(tmpCert, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        tmpCert = NULL;
        tmpCertSz = 0;
    }
#endif

    /* Erase example certs/keys from SE050 if stored */
    if (credentialsStored) {
        ret = erase_certs_keys_from_se050();
        if (ret != 0) {
            LOG_E("Error erasing certs/keys from SE050");
        } else {
            ret = WOLFSSL_SUCCESS;
        }
    }

    if (ret == WOLFSSL_FAILURE) {
        status = kStatus_SSS_Fail;
    }

    LOG_I("Done with sample app");
    return status;
}

#endif /* SIMW_DEMO_ENABLE__DEMO_WOLFSSL_CLIENT */

