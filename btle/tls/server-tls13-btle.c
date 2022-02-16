/* server-tls13-btle.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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
 *
 *=============================================================================
 *
 * Example TLS v1.3 server over BTLE
 */

#include "btle-sim.h"

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/wc_port.h>

#define CERT_FILE "../../certs/server-cert.pem"
#define KEY_FILE  "../../certs/server-key.pem"
#define CA_FILE   "../../certs/client-cert.pem"


typedef struct CbCtx {
    void* devCtx;
} CbCtx_t;

static int btleIORx(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int recvd;
    CbCtx_t* cbCtx = (CbCtx_t*)ctx;
#ifdef DEBUG_BTLE_IO
    printf("BTLE Read: In %d\n", sz);
#endif

    recvd = btle_recv((unsigned char*)buf, sz, NULL, cbCtx->devCtx);
    if (recvd == 0) {
        return WOLFSSL_CBIO_ERR_WANT_READ;
    }

#ifdef DEBUG_BTLE_IO
    printf("BTLE Read: Out %d\n", recvd);
#endif
    return recvd;
}

static int btleIOTx(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int sent;
    CbCtx_t* cbCtx = (CbCtx_t*)ctx;
#ifdef DEBUG_BTLE_IO
    printf("BTLE Write: In %d\n", sz);
#endif

    sent = btle_send((unsigned char*)buf, sz, BTLE_PKT_TYPE_TLS, cbCtx->devCtx);
    if (sent == 0) {
        return WOLFSSL_CBIO_ERR_WANT_WRITE;
    }

#ifdef DEBUG_BTLE_IO
    printf("BTLE Write: Out %d\n", sent);
#endif
    return sent;
}


int main(int argc, char** argv)
{
    int ret = -1, err;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    CbCtx_t cBctx;
    byte echoBuffer[100];

    memset(&cBctx, 0, sizeof(cBctx));

#if 0
    wolfSSL_Debugging_ON();
#endif

    wolfSSL_Init();

    /* open BTLE */
    printf("Waiting for client\n");
    ret = btle_open(&cBctx.devCtx, BTLE_ROLE_SERVER);
    if (ret != 0) {
        printf("btle_open failed %d! errno %d\n", ret, errno);
        goto done;
    }

    ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
    if (ctx == NULL) {
        printf("Error creating WOLFSSL_CTX\n");
        goto done;
    }

    /* Register wolfSSL send/recv callbacks */
    wolfSSL_CTX_SetIOSend(ctx, btleIOTx);
    wolfSSL_CTX_SetIORecv(ctx, btleIORx);

    /* For testing disable peer cert verification */
    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    /* Set server key and certificate (required) */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n", CERT_FILE);
        goto done;
    }

    /* Load server key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n", KEY_FILE);
        goto done;
    }

    /* Mutual Authentication */
#if 0
    /* Load client certificate as "trusted" into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL))
         != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CA_FILE);
        goto done;
    }
#endif

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("Error creating WOLFSSL\n");
        goto done;
    }

    /* Register wolfSSL read/write callback contexts */
    wolfSSL_SetIOReadCtx(ssl, &cBctx);
    wolfSSL_SetIOWriteCtx(ssl, &cBctx);

    printf("TLS accepting\n");
    do {
        ret = wolfSSL_accept(ssl);
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);
    if (ret != WOLFSSL_SUCCESS) {
        printf("TLS accept error %d\n", err);
        goto done;
    }
    printf("TLS Accept handshake done\n");

    while (1) {
        /* Waiting for data to echo */
        memset(echoBuffer, 0, sizeof(echoBuffer));
        do {
            ret = wolfSSL_read(ssl, echoBuffer, sizeof(echoBuffer)-1);
            err = wolfSSL_get_error(ssl, ret);
        } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);
        printf("Read (%d): %s\n", ret, echoBuffer);

        do {
            ret = wolfSSL_write(ssl, echoBuffer, XSTRLEN((char*)echoBuffer));
            err = wolfSSL_get_error(ssl, ret);
        } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);
        printf("Sent (%d): %s\n", ret, echoBuffer);

        /* check for exit flag */
        if (strncmp((char*)echoBuffer, EXIT_STRING, strlen(EXIT_STRING)) == 0) {
            printf("Exit, closing connection\n");
            break;
        }
    }

    ret = 0; /* Success */

done:
    if (ssl) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (ctx) {
        wolfSSL_CTX_free(ctx);
    }

    if (cBctx.devCtx != NULL) {
        btle_close(cBctx.devCtx);
    }

    wolfSSL_Cleanup();

    return ret;
}
