/* example-lwip-native-echoclient.c
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
 */


#include "main.h"
#include "tlsecho.h"
#include "FreeRTOSConfig.h"
#include "cmsis_os.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/memp.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>


#ifndef NO_RSA
    #define USE_CERT_BUFFERS_2048
    #define CA_CERTS           ca_cert_der_2048
    #define CA_CERTS_LEN       sizeof_ca_cert_der_2048
    #define CLIENT_CERT        client_cert_der_2048
    #define CLIENT_CERT_LEN    sizeof_client_cert_der_2048
    #define CLIENT_KEY         client_key_der_2048
    #define CLIENT_KEY_LEN     sizeof_client_key_der_2048
#elif defined(HAVE_ECC)
    #define USE_CERT_BUFFERS_256
    #define CA_CERTS           ca_ecc_cert_der_256
    #define CA_CERTS_LEN       sizeof_ca_ecc_cert_der_256
    #define CLIENT_CERT        cliecc_cert_der_256
    #define CLIENT_CERT_LEN    sizeof_cliecc_cert_der_256
    #define CLIENT_KEY         ecc_clikey_der_256
    #define CLIENT_KEY_LEN     sizeof_ecc_clikey_der_256
#else
    #error This examples requires either RSA or ECC to be enabled
#endif
#include <wolfssl/certs_test.h>
#include <wolfcrypt/test/test.h>

#ifndef DEST_PORT
    #define DEST_PORT 11111
#endif
#ifndef DEST_IP_ADDR0
    #define DEST_IP_ADDR0 192
    #define DEST_IP_ADDR1 168
    #define DEST_IP_ADDR2 1
    #define DEST_IP_ADDR3 100
#endif

#ifndef MAX_MSG_SIZE
#define MAX_MSG_SIZE 1024
#endif

#define TLSECHO_THREAD_PRIO  (osPriorityAboveNormal)
#define TEST_MSG "TLS **TEST 1** "

typedef enum connection_state {
	CONNECTION_STATE_NONE,
	CONNECTION_STATE_CONNECTING,
	CONNECTION_STATE_CONNECTED,
	CONNECTION_STATE_TLS_CONNECTING,
	CONNECTION_STATE_TLS_CONNECTED,
	CONNECTION_STATE_TLS_WRITTEN,
	CONNECTION_STATE_SHUTDOWN,
	CONNECTION_STATE_DONE
} connection_state;

static err_t tcp_echoclient_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static void TLS_shutdown(void);
static int TLS_connect(void);
static int TLS_setup(void);

struct tcp_pcb *tls_pcb   = NULL;
struct tcp_pcb *debug_pcb = NULL;

static WOLFSSL_CTX *ctx = NULL;
static WOLFSSL *ssl = NULL;

static connection_state state = CONNECTION_STATE_NONE;

void loggingCb(const int logLevel, const char *const logMessage)
{
    printf("%s\n", logMessage);
    (void)logLevel;
}

static void tls_echoclient_connect(void *arg)
{
    ip_addr_t DestIPaddr;
    int ret;
    char reply[MAX_MSG_SIZE];
    int err;
    uint32_t led_counter = 0;

    while(1)
    {
		switch(state)
		{
			case CONNECTION_STATE_NONE:
				tls_pcb = tcp_new();
				if (tls_pcb != NULL) {
					IP4_ADDR(&DestIPaddr, DEST_IP_ADDR0, DEST_IP_ADDR1, DEST_IP_ADDR2, DEST_IP_ADDR3);
					tcp_connect(tls_pcb, &DestIPaddr, DEST_PORT,
						tcp_echoclient_connected);
					printf("Connecting\n");
				}
				state = CONNECTION_STATE_CONNECTING;
				break;
			case CONNECTION_STATE_CONNECTED:
			case CONNECTION_STATE_TLS_CONNECTING:
				state = CONNECTION_STATE_TLS_CONNECTING;
				TLS_connect();
				break;
			case CONNECTION_STATE_TLS_CONNECTED:
				ret = wolfSSL_write(ssl, TEST_MSG, sizeof(TEST_MSG));
				if (ret <= 0) {
					loggingCb(0, "error when writing TLS message");
					state = CONNECTION_STATE_SHUTDOWN;
				}
				else {
					state = CONNECTION_STATE_TLS_WRITTEN;
				}
				break;
			case CONNECTION_STATE_TLS_WRITTEN:
				memset(reply, 0, sizeof(reply));
				ret = wolfSSL_read(ssl, reply, sizeof(reply));
				if (ret <= 0) {
					err = wolfSSL_get_error(ssl, 0);
					if (err != SSL_ERROR_WANT_READ &&
						err != WOLFSSL_ERROR_WANT_WRITE) {
						loggingCb(0, "error when reading TLS message");
						state = CONNECTION_STATE_SHUTDOWN;
					}
				}
				if (ret > 0) {
					printf("Got message: %s\n", reply);
					/* received reply, done with connection */
					state = CONNECTION_STATE_SHUTDOWN;
				}
				break;
			case CONNECTION_STATE_SHUTDOWN:
				TLS_shutdown();
				state = CONNECTION_STATE_DONE;
				break;
			case CONNECTION_STATE_DONE:
				return;
			default:
				break;
		}
		vTaskDelay(10);
		led_counter++;
		if ((led_counter % 50) == 0)
			BSP_LED_Toggle(LED_BLUE);
    }
}

void
tlsecho_init(void)
{
  sys_thread_new("tlsecho_thread", tls_echoclient_connect, NULL, 128*1024, TLSECHO_THREAD_PRIO);
}

static err_t tcp_echoclient_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err == ERR_OK) {
        printf("Connected\n");
        state = CONNECTION_STATE_CONNECTED;
        /* a tcp_poll could be added here for connection timeout checking */
        return TLS_setup();
    } else {
    	printf("Error connecting %d\n", err);
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
        state = CONNECTION_STATE_SHUTDOWN;
        return ERR_MEM;
    }
    /* Load the CA */
    ret = wolfSSL_CTX_load_verify_buffer(ctx, CA_CERTS, CA_CERTS_LEN,
        WOLFSSL_FILETYPE_ASN1);
    if (ret != WOLFSSL_SUCCESS) {
        loggingCb(0, "error loading in verify buffer");
        return ERR_MEM;
    }

#if 1
    /* Disable peer certificate validation for testing */
    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_NONE, NULL);
#endif

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
            state = CONNECTION_STATE_SHUTDOWN;
            return ERR_CONN;
        }
    }
    else {
        loggingCb(0, "TLS handshake done!");
        state = CONNECTION_STATE_TLS_CONNECTED;
    }
    return ERR_OK;
}

/* close all connections and free TLS session */
static void TLS_shutdown(void)
{
	printf("Connection shutdown\n");
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }

    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
    }

    wolfSSL_Cleanup();

    tcp_output(tls_pcb);
    tcp_close(tls_pcb);
    tls_pcb = NULL;
}
