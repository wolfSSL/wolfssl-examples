/* server-tls.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/init.h"
#include "lwip/sockets.h"

#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/ssl.h"

#include "wolf/wifi.h"
#include "wolf/tcp.h"
#include "wolf/tls.h"
#include "wolf/time.h"

#define USE_CERT_BUFFERS_256
#define USE_CERT_BUFFERS_2048
#include <wolfssl/certs_test.h>

#define DEFAULT_PORT 11111

void tlsServer_test(void *arg)
{
    (void) arg;
    int sockfd = SOCKET_INVALID;
    int connd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t size = sizeof(clientAddr);
    char buff[256];
    size_t len;
    int shutdown = 0;
    int ret;
    const char *reply = "I hear ya fa shizzle!\n";

    /* declare wolfSSL objects */
    WOLFSSL_CTX *ctx = NULL;
    WOLFSSL *ssl = NULL;
    WOLFSSL_CIPHER *cipher;

    cyw43_arch_init();

    printf("Connecting to Wi-Fi...\n");
    if (wolf_wifiConnect(WIFI_SSID, WIFI_PASSWORD, 
                        CYW43_AUTH_WPA2_AES_PSK, 5000)) {
        printf("failed to connect.\n");
        return;
    }
    else {
        printf("Wifi connected.\n");
    }

    lwip_init();
    tcp_initThread();

    /* Initialize wolfSSL */
    wolfSSL_Init();
    wolfSSL_Debugging_ON();
    if(time_init() < 0) {
        printf("ERROR:time_init()\n");
        return;
    }

    printf("\nStarting tlsClient_test\n");
#if 0
    wolfSSL_Debugging_ON();
#endif

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }

    /* Create and initialize WOLFSSL_CTX */
#ifdef USE_TLSV13
    ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
#else
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
#endif
    if (ctx == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* Register callbacks */
    wolfSSL_CTX_SetIORecv(ctx, my_IORecv);
    wolfSSL_CTX_SetIOSend(ctx, my_IOSend);

    /* Load server certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_buffer(ctx, server_cert_der_2048,
            sizeof_server_cert_der_2048, SSL_FILETYPE_ASN1)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to loadserver_cert_der_2048.\n");
        goto exit;
    }

    /* Load server key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx, server_key_der_2048,
                sizeof_server_key_der_2048, SSL_FILETYPE_ASN1)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load server_key_der_2048\n");
        goto exit;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;           /* using IPv4      */
    servAddr.sin_port = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;   /* from anywhere   */

    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto exit;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr *)&clientAddr, &size)) == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1;
            goto exit;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1;
            goto exit;
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);

        /* Establish TLS connection */
        ret = wolfSSL_accept(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_accept error = %d\n",
                    wolfSSL_get_error(ssl, ret));
            goto exit;
        }

        printf("Client connected successfully\n");

        cipher = wolfSSL_get_current_cipher(ssl);
        printf("SSL cipher suite is %s\n", wolfSSL_CIPHER_get_name(cipher));

        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if ((ret = wolfSSL_read(ssl, buff, sizeof(buff) - 1)) == -1) {
            fprintf(stderr, "ERROR: failed to read\n");
            goto exit;
        }

        /* Print to stdout any data the client sends */
        printf("Client: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }

        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        /* Reply back to the client */
        if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            goto exit;
        }

        /* Notify the client that the connection is ending */
        wolfSSL_shutdown(ssl);
        printf("Shutdown complete\n");

        /* Cleanup after this connection */
        wolfSSL_free(ssl); /* Free the wolfSSL object              */
        ssl = NULL;
        close(connd); /* Close the connection to the client   */
    }

    ret = 0;

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl); /* Free the wolfSSL object              */
    if (connd != SOCKET_INVALID)
        close(connd); /* Close the connection to the client   */
    if (sockfd != SOCKET_INVALID)
        close(sockfd); /* Close the socket listening for clients   */
    if (ctx)
        wolfSSL_CTX_free(ctx); /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();         /* Cleanup the wolfSSL environment          */

    return; /* Return reporting a success               */
}

void main(void)
{
    TaskHandle_t task_tlsServer;
#define STACK_SIZE (1024 * 16)

    int i;

    stdio_init_all();
    for (i = 0; i < 10; i++)
    {
        printf("Starting in %dSec.\n", 10 - i);
        sleep_ms(1000);
    }

    printf("Creating tlsServer task, stack = %d\n", STACK_SIZE);
    xTaskCreate(tlsServer_test, "TLS_MainThread", STACK_SIZE, NULL,
                CYW43_TASK_PRIORITY + 1, &task_tlsServer);
    vTaskStartScheduler();
}