/* tcpClient_main.c
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

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/ssl.h"

#include "wolf/common.h"
#include "wolf/tcp.h"
#include "wolf/wifi.h"
#include "wolf/blink.h"
#include "lwip/tcp.h"

#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/ssl.h"

#define USE_CERT_BUFFERS_256
#define USE_CERT_BUFFERS_2048
#include <wolfssl/certs_test.h>

#define TCP_PORT 1111

int wolf_cb_TCPwrite(WOLFSSL *ssl, const unsigned char *buff, long unsigned int len, void *ctx)
{
    (void)ssl;
    unsigned long ret;
    SOCKET_T sock = (SOCKET_T)ctx;
    ret = send(sock, buff, len);
    return ret;
}

int wolf_cb_TCPread(WOLFSSL *ssl, unsigned char *buff, long unsigned int len, void *ctx)
{
    (void)ssl;
    SOCKET_T sock = (SOCKET_T)ctx;
    int ret;

    ret = recv(sock, buff, len);
    return ret;
}

void tlsClient_test(void)
{
    int i;
    int ret;
    #define BUFF_SIZE 2048
    static char buffer[BUFF_SIZE];
    char msg[] = "Hello Server";

    SOCKET_T sock;
    struct sockaddr_in servAddr;

    WOLFSSL_CTX *ctx    = NULL;
    WOLFSSL     *ssl    = NULL;

    /* Initialize wolfSSL */
    wolfSSL_Init();
    wolfSSL_Debugging_ON();

    if ((ctx = wolfSSL_CTX_new((wolfTLSv1_2_client_method()))) == NULL) {
        printf("ERROR:wolfSSL_CTX_new()\n");
        return;
    }
    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_buffer(ctx, ca_cert_der_2048,
            sizeof_ca_cert_der_2048, SSL_FILETYPE_ASN1)) != WOLFSSL_SUCCESS) {
        printf("ERROR: failed to load CA cert. %d\n", ret);
        goto exit;
    }

    wolfSSL_SetIORecv(ctx, (CallbackIORecv)wolf_cb_TCPread);
    wolfSSL_SetIOSend(ctx, (CallbackIOSend)wolf_cb_TCPwrite);

    sock = socket();
    if (!sock)
    {
        printf("ERROR:wolf_TCPsocke()\n");
        return;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;           /* using IPv4      */
    servAddr.sin_port = htons(TCP_PORT); /* on DEFAULT_PORT */

    if (inet_pton(AF_INET, TEST_TCP_SERVER_IP, &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        goto exit;
    }

    if (connect(sock,(struct sockaddr*) &servAddr, sizeof(servAddr)) != WOLF_SUCCESS) {
        printf("ERROR:wolf_TCPconnect()\n");
        goto exit;
    }


    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1; 
        goto exit;
    }

    wolfSSL_SetIOReadCtx(ssl, sock);
    wolfSSL_SetIOWriteCtx(ssl, sock);

    printf("TLS Connecting\n");
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL(%d)\n",
            wolfSSL_get_error(ssl, ret));
        goto exit;
    }

    printf("Writing to server: %s\n", msg);
    ret = wolfSSL_write(ssl, msg, strlen(msg));
    if (ret < 0) {
        DEBUG_printf("Failed to write data. err=%d\n", ret);
        goto exit;
    }

    ret = wolfSSL_read(ssl, buffer, BUFF_SIZE);
    if (ret < 0) {
        DEBUG_printf("Failed to read data. err=%d\n", ret);
        goto exit;
    }
    printf("Message: %s\n", buffer);


exit:
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (sock)
        free(sock);              /* Close the connection to the server   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */

}

void main(void)
{
    blink(20, 1);

    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    printf("WIFI_SSID=%s, WIFI_PASSWORD=%s\n", WIFI_SSID, WIFI_PASSWORD);
    if (wolf_wifiConnect(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return;
    } else {
        printf("Wifi connected.\n");
    }
    cyw43_arch_lwip_begin();

    printf("Starting TLS client\n");
    tlsClient_test();
    printf("End of TLS client\n");

    cyw43_arch_lwip_end();
    cyw43_arch_deinit();

    printf("Wifi disconnected\n");
}

void lwip_example_app_platform_assert(const char *msg, int line, const char *file)
{
    printf("Assertion \"%s\" failed at line %d in %s\n", msg, line, file);
    fflush(NULL);
}

#include <time.h>
time_t myTime(time_t *t)
{
    *t = (((2023 - 1970) * 365 + (8 * 30)) * 24 * 60 * 60);
    return *t;
}