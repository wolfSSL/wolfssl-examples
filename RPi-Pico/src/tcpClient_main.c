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

#include "wolf/common.h"
#include "wolf/tcp.h"
#include "wolf/wifi.h"
#include "wolf/blink.h"
#include "lwip/tcp.h"

#define TCP_PORT 1111

static void dump_bytes(const uint8_t *p, uint32_t len)
{
    for (; len; len--, p++) {
        if (((unsigned long)p & 0x07) == 0) {
            printf("\n");
        }
        printf("%02x ", *p);
    }
    printf("\n");
}

void tcpClient_test(void)
{
    int i;
    int err;
    #define BUFF_SIZE 2048
    char buffer[BUFF_SIZE];
    #define SIZE_OF_CLIENT_HELLO 815
    char msg[SIZE_OF_CLIENT_HELLO] = "\026\003\003\003\052\001\000\003\046 Fake Client Hello";

    WOLF_SOCKET_T *sock = wolf_TCPsocket();
    if (!sock)
    {
        printf("ERROR:wolf_TCPsocke()\n");
        return;
    }
    if (!wolf_TCPconnect(sock, TEST_TCP_SERVER_IP, TCP_PORT)) {
        printf("ERROR:wolf_TCPconnect()\n");
        goto exit;
    }

    DEBUG_printf("Writing to server: %s\n", msg);
    err = wolf_TCPwrite(sock, msg, sizeof(msg));
    if (err < 0) {
        DEBUG_printf("Failed to write data. err=%d\n", err);
        goto exit;
    }
    DEBUG_printf("Writen data %d bytes\n", err);   
    
    err = wolf_TCPread(sock, buffer, BUFF_SIZE);
    if (err < 0) {
        DEBUG_printf("Failed to read data. err=%d\n", err);
        goto exit;
    }
    DEBUG_printf("Read data %d bytes\n", err);
    dump_bytes(buffer, err);

exit:
    free(sock);
}

void main(void)
{
    blink(20, 1);
    printf("Started main_task\n");

    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    printf("WIFI_SSID=%s, WIFI_PASSWORD=%s\n", WIFI_SSID, WIFI_PASSWORD);
    if(wolf_wifiConnect(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return;
    } else {
        printf("Connected.\n");
    }
    cyw43_arch_lwip_begin();
    printf("\nReady, running iperf client\n");

    tcpClient_test();

    cyw43_arch_lwip_end();
    cyw43_arch_deinit();

    printf("End of iperf client\n");
}

void lwip_example_app_platform_assert(const char *msg, int line, const char *file)
{
    printf("Assertion \"%s\" failed at line %d in %s\n", msg, line, file);
    fflush(NULL);
}

