/* example-lwip-native-echoclient.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/memp.h"
#include <stdio.h>
#include <string.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

#define USE_CERT_BUFFERS_2048
#define USE_CERT_BUFFERS_256
#include <wolfssl/certs_test.h>
#include <wolfcrypt/test/test.h>

static err_t tcp_echoclient_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static void shutdown(void);
static int TLS_connect(void);
static int TLS_setup(void);

struct tcp_pcb *tls_pcb   = NULL;
struct tcp_pcb *debug_pcb = NULL;

#ifdef DEBUG_PORT
    static int debugConnected = 0;
    static int testWolfcrypt = 0; /* set to 1 to not run wolfCrypt tests */
#else
    /* default to "connected" for debugging when not used */
    static int debugConnected = 1;
    static int testWolfcrypt = 1;
#endif
static int tlsConnected = 0;
static int tcpConnected = 0;
static int tlsWaitingForReply = 0;

WOLFSSL_CTX *ctx;
WOLFSSL *ssl;

struct debugging *logging = NULL;
struct debugging {
  struct tcp_pcb *pcb;
  struct pbuf *p_tx;
};

#ifdef DEBUG_PORT
static err_t debug_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    /* only sending out debug logs, drop any incoming msgs */
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static err_t debug_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    return ERR_OK;
}

void loggingCb(const int logLevel, const char *const logMessage)
{
    if (logging != NULL) {
        tcp_write(debug_pcb, logMessage, strlen(logMessage), TCP_WRITE_FLAG_COPY);
        tcp_write(debug_pcb, "\n\r", sizeof("\n\r"), TCP_WRITE_FLAG_COPY);
        tcp_output(debug_pcb);
    }
}

/* runs the wolfcrypt test on startup */
static void run_test_wolfcrypt(void)
{
    if (debugConnected == 1 && testWolfcrypt == 0) {
        if (wolfcrypt_test(NULL) == 0) {
            loggingCb(0, "wolfcrypt tests passed");
        }
        else {
            loggingCb(0, "wolfcrypt tests failed");
        }
        testWolfcrypt = 1;
    }
}

static err_t start_debug(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err == ERR_OK) {
        logging = (struct debugging *)mem_malloc(sizeof(struct debugging));
        logging->pcb = tpcb;
        tcp_arg(tpcb, logging);
        tcp_recv(debug_pcb, debug_recv);
        tcp_sent(tpcb, debug_sent);
        loggingCb(0, "wolfSSL debug connection");
        if (testWolfcrypt == 0) {
            loggingCb(0, "Running wolfCrypt tests, this could take some time...");
        }
        debugConnected = 1;
    }
    return ERR_OK;
}

#else
void loggingCb(const int logLevel, const char *const logMessage)
{
    (void)logLevel;
    (void)logMessage;
}

static void run_test_wolfcrypt(void)
{
    /* do nothing in this case */
}
#endif /* DEBUG_PORT */


void tls_echoclient_connect(void)
{
    ip_addr_t DestIPaddr;

#ifdef DEBUG_PORT
    if (debugConnected == 0) {
        debug_pcb = tcp_new();
        IP4_ADDR( &DestIPaddr, DEST_IP_ADDR0, DEST_IP_ADDR1, DEST_IP_ADDR2, DEST_IP_ADDR3 );
        tcp_connect(debug_pcb, &DestIPaddr, DEBUG_PORT, start_debug);
    }
#endif

    if (debugConnected == 1 && testWolfcrypt == 1) {
        if (tcpConnected == 0) {
            tls_pcb = tcp_new();
            if (tls_pcb != NULL) {
                IP4_ADDR( &DestIPaddr, DEST_IP_ADDR0, DEST_IP_ADDR1, DEST_IP_ADDR2, DEST_IP_ADDR3 );
                tcp_connect(tls_pcb,&DestIPaddr,DEST_PORT,tcp_echoclient_connected);
            }
        }

        if (tcpConnected == 1 && tlsConnected == 0) {
            TLS_connect();
        }

        if (tlsConnected == 1) {
            int ret;
            char reply[256];
            int err;

            if (tlsWaitingForReply == 0) {
                ret = wolfSSL_write(ssl, "TLS **TEST 1** ", sizeof("TLS **TEST 1** "));
                if (ret <= 0) {
                    loggingCb(0, "error when writing TLS message");
                    shutdown();
                }
                else {
                    tlsWaitingForReply = 1;
                }
            }

            /* read a reply from the server */
            if (tlsWaitingForReply == 1) {
                memset(reply, 0, sizeof(reply));
                ret = wolfSSL_read(ssl, reply, sizeof(reply));
                if (ret <= 0) {
                    err = wolfSSL_get_error(ssl, 0);
                    if (err != SSL_ERROR_WANT_READ &&
                        err != WOLFSSL_ERROR_WANT_WRITE) {
                        loggingCb(0, "error when reading TLS message");
                        shutdown();
                    }
                }
                if (ret > 0) {
                    loggingCb(0, reply);
                    shutdown(); /* received reply, done with connection */
                }
            }
        }
    }
    else {
        run_test_wolfcrypt();
    }
}


static err_t tcp_echoclient_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err == ERR_OK) {
        tcpConnected = 1;
        /* a tcp_poll could be added here for connection timeout checking */
        return TLS_setup();
    }
    return ERR_OK;
}


/* setup all the TLS structures and load CA */
static int TLS_setup(void)
{
    int ret;

#ifdef DEBUG_WOLFSSL
    /* redirect wolfssl debug print outs */
    wolfSSL_SetLoggingCb(loggingCb);
    wolfSSL_Debugging_ON();
#endif
    wolfSSL_Init(); /* called once on program startup */
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    if (ctx == NULL) {
        loggingCb(0, "ctx was null");
        shutdown();
        return ERR_MEM;
    }

    // This is an example of loading an RSA CA certificate instead of ECC
    //ret = wolfSSL_CTX_load_verify_buffer(ctx, ca_cert_der_2048, sizeof(ca_cert_der_2048), WOLFSSL_FILETYPE_ASN1);
    //

    ret = wolfSSL_CTX_load_verify_buffer(ctx, ca_ecc_cert_der_256, sizeof(ca_ecc_cert_der_256), WOLFSSL_FILETYPE_ASN1);
    if (ret != WOLFSSL_SUCCESS) {
        loggingCb(0, "error loading in verify buffer");
        return ERR_MEM;
    }

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        loggingCb(0, "ssl was null");
        return ERR_MEM;
    }
    wolfSSL_SetIO_LwIP(ssl, tls_pcb, NULL, NULL, NULL);
    return ERR_OK;
}

static int TLS_connect(void)
{
    if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
        /* check if hitting a want read/write case and should call again */
        int err = wolfSSL_get_error(ssl, 0);
        if (err != SSL_ERROR_WANT_READ && err != WOLFSSL_ERROR_WANT_WRITE) {
            loggingCb(0, "connect error, shutting down");
            loggingCb(0, wolfSSL_ERR_reason_error_string(err));
            shutdown();
            return ERR_CONN;
        }
        loggingCb(0, "found want read/write");
    }
    else {
        loggingCb(0, "setting tlsConnected to 1");
        tlsConnected = 1;
    }
    return ERR_OK;
}

/* close all connections and free TLS session */
static void shutdown(void)
{
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }

    if (ctx != NULL) {
            wolfSSL_CTX_free(ctx);
    }

    tlsConnected = 0;
    tlsWaitingForReply = 0;
    wolfSSL_Cleanup();

    #if 0 /* option to close down debug channel */
    if (debugConnected == 1) {
        tcp_output(debug_pcb);
        tcp_close(debug_pcb);
        debug_pcb = NULL;
        debugConnected = 0;
    }
    #endif

    if (tcpConnected == 1) {
        tcp_output(tls_pcb);
        tcp_close(tls_pcb);
        tls_pcb = NULL;
        tcpConnected = 0;
    }
}

