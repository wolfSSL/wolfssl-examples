/* tcpClient_main.c
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
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

#define TCP_PORT 11111
#define WIFI_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)

void tlsClient_test(void *arg)
{
    (void)arg;
    int i;
    int ret;
    #define BUFF_SIZE 2048
    static char buffer[BUFF_SIZE];
    char msg[] = "Hello Server";

    int sock;
    struct sockaddr_in servAddr;

    WOLFSSL_CTX *ctx    = NULL;
    WOLFSSL     *ssl    = NULL;

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

    printf("\nStarting tlsClient_test\n");

    if(time_init() < 0) {
        printf("ERROR:time_init()\n");
        return;
    }

    if ((ctx = wolfSSL_CTX_new((wolfTLSv1_2_client_method()))) == NULL) {
        printf("ERROR:wolfSSL_CTX_new()\n");
        return;
    }

    printf("wolfSSL_CTX_new\n");
    /* Register callbacks */
    wolfSSL_CTX_SetIORecv(ctx, my_IORecv);
    wolfSSL_CTX_SetIOSend(ctx, my_IOSend);

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_buffer(ctx, ca_cert_der_2048,
            sizeof_ca_cert_der_2048, SSL_FILETYPE_ASN1)) != WOLFSSL_SUCCESS) {
        printf("ERROR: failed to load CA cert. %d\n", ret);
        goto exit;
    }

    printf("wolfSSL_CTX_load_verify_buffer\n");

    for(i=0; i<10; i++) {

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            printf("ERROR:wolf_TCPsocke()\n");
            return;
        }

        printf("Retured from socket\n");

        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;       /* using IPv4      */
        servAddr.sin_port = htons(TCP_PORT); /* on DEFAULT_PORT */

        printf("Connecting to the server(%s)\n", TCP_SERVER);
        if (inet_pton(AF_INET, TCP_SERVER, &servAddr.sin_addr) != 1) {
            fprintf(stderr, "ERROR: invalid address\n");
            goto exit;
        }

        if ((ret = connect(sock, (struct sockaddr *)&servAddr, 
                                sizeof(servAddr))) != EXIT_SUCCESS) {
            printf("ERROR:connect(%d)\n", ret);
            goto exit;
        }

        printf("TCP connected\n");

        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1; 
            goto exit;
        }

        if ((ret = wolfSSL_set_fd(ssl, sock)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
            goto exit;
        }

        printf("TLS Connecting\n");
        if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to connect to wolfSSL(%d)\n",
                wolfSSL_get_error(ssl, ret));
            goto exit;
        }

        printf("Writing to server: %s\n", msg);
        ret = wolfSSL_write(ssl, msg, strlen(msg));
        if (ret < 0) {
            printf("Failed to write data. err=%d\n", ret);
            goto exit;
        }

        ret = wolfSSL_read(ssl, buffer, BUFF_SIZE);
        if (ret < 0) {
            printf("Failed to read data. err=%d\n", ret);
            goto exit;
        }
        printf("Message: %s\n", buffer);

        wolfSSL_free(ssl);
        close(sock);
    }
    printf("End of TLS Client\n");

exit:
    if (ssl)
        wolfSSL_free(ssl);
    if (sock >= 0)
        close(sock);
    if (ctx)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    while(1)
        vTaskDelay(1000);
}

void main(void)
{
    TaskHandle_t task_tlsClient;
#define STACK_SIZE (1024 * 16)

    int i;

    stdio_init_all();
    for (i = 0; i < 10; i++)
    {
        printf("Starting in %dSec.\n", 10 - i);
        sleep_ms(1000);
    }

    printf("Creating tlsClient task, stack = %d\n", STACK_SIZE);
    xTaskCreate(tlsClient_test, "TLS_MainThread", STACK_SIZE, NULL, 
                                CYW43_TASK_PRIORITY + 1, &task_tlsClient);
    vTaskStartScheduler();
}
