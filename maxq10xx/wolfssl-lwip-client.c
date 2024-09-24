/* wolfssl-lwip-client.c
 *
 * Based on client-tls-pkcallback.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* This example shows how to write a simple TLS client that uses the features
 * of the Analog Devices MAXQ1065 and 1080 USS. Note that this is not a
 * stand-alone application. This is part of an example that can be found in the
 * SDK supplied by Analog Devices. */

#include "lwip/opt.h"

#if !NO_SYS /* don't build if not configured to run an OS in lwipopts.h */

/***** Includes *****/
#include <stdio.h>

/* wolfSSL */
#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include "lwip_crypto_libs_app.h"
#include "crypto_keys.h"

#define TLS12 1
#define TLS13 2

#define ECC 1
#define RSA 2
#define PSK 3

#if defined(TEST_TLS_1_3)
static int tls_version = TLS13;
#elif defined(TEST_TLS_1_2)
static int tls_version = TLS12;
#else
#error TEST_TLS_1_3 or TEST_TLS_1_2 MUST be defined in project.mk to test TLS
#endif

#if defined(TEST_PSK)
static int alg = PSK;
#elif defined(TEST_RSA)
static int alg = RSA;
#else
static int alg = ECC;
#endif

static char *ciphersuite = NULL;
static const char* kIdentityStr = "Client_identity";

static unsigned int my_psk_client_cb(struct WOLFSSL* ssl, const char* hint,
                                char* identity, unsigned int id_max_len,
                                uint8_t* key, unsigned int key_max_len)
{
    (void)ssl;
    (void)hint;
    (void)key;
    (void)key_max_len;

    /* see internal.h MAX_PSK_ID_LEN for PSK identity limit */
    XSTRNCPY(identity, kIdentityStr, id_max_len);

    /* For TLS 1.2, we indicate that MAXQ has the PSK. */
    return USE_HW_PSK;
}

#ifdef WOLFSSL_TLS13
static unsigned int my_psk_client_cs_cb(struct WOLFSSL* ssl, const char* hint,
                                char* identity, unsigned int id_max_len,
                                unsigned char* key, unsigned int key_max_len,
                                const char* ciphersuite)
{
    (void)ssl;
    (void)hint;
    (void)key;
    (void)key_max_len;

#ifdef WOLFSSL_PSK_MULTI_ID_PER_CS
    /* Multiple calls for each cipher suite. First identity byte indicates the
     * number of identites seen so far for cipher suite. */
    if (identity[0] != 0) {
        return 0;
    }
#endif  /* WOLFSSL_PSK_MULTI_ID_PER_CS */

    /* see internal.h MAX_PSK_ID_LEN for PSK identity limit */
    XSTRNCPY(identity, kIdentityStr, id_max_len);
    XSTRNCAT(identity, ciphersuite + XSTRLEN(ciphersuite) - 6, id_max_len);

    /* For TLS 1.3, we just return an unmodified key. */
    return 32;
}
#endif /* WOLFSSL_TLS13 */

void mxc_wolfssl_create(int sockfd, WOLFSSL **ssl)
{
    int ret;
    int exit_clean = 1;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;

    /*---------------------------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------------------------*/
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        MXC_ERR_MSG("ERROR: Failed to initialize the library\n");
        goto exit;
    }

    /* Create and initialize WOLFSSL_CTX */
#ifdef WOLFSSL_TLS13
    if (tls_version == TLS13) {
        MXC_DEBUG_MSG_GRN("TLS v1.3\n");
        ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
    } else
#endif /* WOLFSSL_TLS13 */
    {
        MXC_DEBUG_MSG_GRN("TLS v1.2\n");
        ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    }

    if (ctx == NULL) {
        MXC_ERR_MSG("ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* At this point you would normally register a CA certificate, however, it
     * resides in MAXQ10xx and has already been registered. */
#ifndef NO_FILESYSTEM
    /* Load the dummy private key; actually a public key. The actual private
     * key resides in MAXQ 10xx. */
    if (key_file != NULL) {
        if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, key_file,
                            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
            MXC_ERR_MSG("ERROR: failed to load %s, please check the "
                    "file.\n", key_file);
            goto exit;
        }
    }
#else

#if defined (TEST_RSA)
    ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx, rsa_key_der_2048,
            sizeof(rsa_key_der_2048), WOLFSSL_FILETYPE_ASN1);
#elif defined (HAVE_ECC)
    ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx, ecc_key_der_256,
            sizeof(ecc_key_der_256), WOLFSSL_FILETYPE_ASN1);
#else
    ret = wolfSSL_CTX_use_PrivateKey_buffer(srv_ctx, server_key_der_2048,
            sizeof_server_key_der_2048, WOLFSSL_FILETYPE_ASN1);
#endif

    if (ret != WOLFSSL_SUCCESS) {
        MXC_DEBUG_MSG("error loading private key\n");
        goto exit;
    }
#endif
    /* If specified, set the ciphersuite. */
    if (ciphersuite != NULL) {
        if (wolfSSL_CTX_set_cipher_list(ctx, ciphersuite) != WOLFSSL_SUCCESS) {
            MXC_ERR_MSG("Invalid cipher suite.\n");
            goto exit;
        }
    }
#ifndef NO_PSK
    if (alg == PSK) {
        wolfSSL_CTX_set_psk_client_callback(ctx, my_psk_client_cb);
#ifdef WOLFSSL_TLS13
        if (tls_version == TLS13) {
            wolfSSL_CTX_set_psk_client_cs_callback(ctx, my_psk_client_cs_cb);
        }
#endif /* WOLFSSL_TLS13 */

        if (ciphersuite != NULL) {
            wolfSSL_CTX_set_psk_callback_ctx(ctx, (void*)ciphersuite);
        }
    }
#endif
    /* Validate peer certificate */
    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, NULL);

    /* Create a WOLFSSL object */
    if ((*ssl = wolfSSL_new(ctx)) == NULL) {
        MXC_ERR_MSG("ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto exit;
    }

    exit_clean = 0;
exit:
    /* Cleanup and return */
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);
    if (exit_clean)
        mxc_wolfssl_close(sockfd, *ssl);
}

void mxc_wolfssl_connect(int sockfd, WOLFSSL *ssl)
{
    int ret, err;
    int exit_clean = 0;

#ifdef WOLFSSL_TLS13
    if (alg == RSA) {
        if (wolfSSL_UseKeyShare(ssl, WOLFSSL_FFDHE_2048) != WOLFSSL_SUCCESS) {
            MXC_ERR_MSG("ERROR: failed to create WOLFSSL object\n");
            ret = -1;
            exit_clean = 1;
            goto exit;
        }
    }
#endif /* WOLFSSL_TLS13 */
    //MXC_DEBUG_MSG("mxc_wolfssl_connect->wolfSSL_set_fd\n");
    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        MXC_ERR_MSG("ERROR: Failed to set the file descriptor\n");
        exit_clean = 1;
        goto exit;
    }

    wolfSSL_SSLSetIORecv(ssl, mxc_wolfssl_receive);
    wolfSSL_SSLSetIOSend(ssl, mxc_wolfssl_send);

    //MXC_DEBUG_MSG("mxc_wolfssl_connect->wolfSSL_connect\n");
    /* Connect to wolfSSL on the server side */
    do {
        ret = wolfSSL_connect(ssl);
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WC_PENDING_E);
    if (ret != WOLFSSL_SUCCESS) {
        MXC_ERR_MSG("ERROR: failed to connect to wolfSSL\n");
        exit_clean = 1;
        goto exit;
    }

    MXC_DEBUG_MSG_GRN("\nmxc_wolfssl_connect-> Done\n\n");

exit:
    if (exit_clean)
        mxc_wolfssl_close(sockfd, ssl);
}

void mxc_wolfssl_write(WOLFSSL* ssl, const void* data, int sz)
{
    int ret;

    MXC_DEBUG_MSG(". mxc_wolfssl_write...\n");

    /* Send the message to the server */
    if ((ret = wolfSSL_write(ssl, data, sz)) != sz) {
        MXC_ERR_MSG("ERROR: failed to write entire message\n");
        MXC_ERR_MSG("%d bytes of %d bytes were sent", ret, sz);
    }
}

int mxc_wolfssl_read(WOLFSSL* ssl)
{
    int ret;
    char buff[SOCKET_BUF_SIZE];

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        MXC_ERR_MSG("ERROR: failed to read\n");
    }

    /* Print to stdout any data the server sends */
    MXC_DEBUG_MSG("Server: %s\n", buff);
    return ret;
}

void mxc_wolfssl_close(int sockfd, WOLFSSL* ssl)
{
    unsigned char lBuff[SOCKET_BUF_SIZE];
    /* Cleanup and return */
    if (ssl != NULL)
        wolfSSL_free(ssl);

    wolfSSL_Cleanup();

    if (sockfd >= 0)
        lwip_close(sockfd);
    while(lwip_read(sockfd, lBuff, SOCKET_BUF_SIZE) > 0);
}

#endif /* !NO_SYS */
