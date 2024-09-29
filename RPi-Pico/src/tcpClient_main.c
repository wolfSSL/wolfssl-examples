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

#include "wolf/wifi.h"
#include "wolf/tcp.h"

#define TCP_PORT 11111

void tcpClient_test(void *arg)
{
    (void)arg;
    int i;
    int ret;
    #define BUFF_SIZE 2048
    char buffer[BUFF_SIZE];
    #define SIZE_OF_CLIENT_HELLO 16
    char msg[SIZE_OF_CLIENT_HELLO] = "Client Hello";

    int sock;
    struct sockaddr_in servAddr;

    cyw43_arch_init();

    printf("Connecting to Wi-Fi...\n");
    if (wolf_wifiConnect(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 5000)) {
        printf("failed to connect.\n");
        return;
    }
    else {
        printf("Wifi connected.\n");
    }

    lwip_init();
    tcp_initThread();

    printf("Starting TCP client\n");

    for(i=0; i< 3; i++) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            printf("ERROR:socke()\n");
            return;
        }

        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;       /* using IPv4      */
        servAddr.sin_port = htons(TCP_PORT); /* on DEFAULT_PORT */

        printf("Connecting to the server(%s)\n", TCP_SERVER);
        if (inet_pton(AF_INET, TCP_SERVER, &servAddr.sin_addr) != 1) {
            fprintf(stderr, "ERROR: invalid address\n");
            goto exit;
        }

        if ((ret = connect(sock,(struct sockaddr*) &servAddr, 
                                sizeof(servAddr))) != EXIT_SUCCESS) {
            printf("ERROR:connect(%d)\n", ret);
            goto exit;
        }

        printf("Writing to server: %s\n", msg);
        ret = write(sock, msg, sizeof(msg));
        if (ret < 0) {
            printf("Failed to write data. err=%d\n", ret);
            goto exit;
        }
        
        ret = read(sock, buffer, BUFF_SIZE);
        if (ret < 0) {
            printf("Failed to read data. err=%d\n", ret);
            goto exit;
        }
        buffer[ret] = '\0';
        printf("Received message[%d]: %s\n", i, buffer);
        close(sock);
    }

    printf("End of TCP client\n");

    cyw43_arch_deinit();

    printf("Wifi disconnected\n");

exit:
    close(sock);
}

void main(void)
{
    TaskHandle_t task_tcpClient;
#define STACK_SIZE (1024 * 16)

    int i;

    stdio_init_all();
    for(i=0; i<10; i++) {
        printf("Starting in %dSec.\n", 10 - i);
        sleep_ms(1000);
    }

    printf("Creating tlsClient task, stack = %d\n", STACK_SIZE);
    xTaskCreate(tcpClient_test, "WifiMainThread", STACK_SIZE, NULL, 
                                CYW43_TASK_PRIORITY+1, &task_tcpClient);
    vTaskStartScheduler();

}
