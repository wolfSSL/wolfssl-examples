/* server-tcp.c
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

#include "wolf/wifi.h"
#include "wolf/tcp.h"

#define DEFAULT_PORT 11111

void tcpServer_test(void *arg)
{
    (void)arg;
    int ret;
    int sockfd;
    int connd;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t size = sizeof(clientAddr);
    char buff[256];
    size_t len;
    int shutdown = 0;
    const char *reply = "I hear ya fa shizzle!\n";

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

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto end;
    }
    printf("sockfd = %d\n", sockfd);
    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;           /* using IPv4      */
    servAddr.sin_port = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;   /* from anywhere   */
    printf("binding\n");
    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto servsocket_cleanup;
    }
    printf("listening\n");
    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) < 0)
    {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto servsocket_cleanup;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown)
    {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr *)&clientAddr, &size)) < 0)
        {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1;
            goto servsocket_cleanup;
        }

        printf("Client connected successfully\n");

        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if ((ret = read(connd, buff, sizeof(buff) - 1)) < 0)
        {
            fprintf(stderr, "ERROR: failed to read\n");
            goto clientsocket_cleanup;
        }

        /* Print to stdout any data the client sends */
        printf("Client: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0)
        {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }

        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        /* Reply back to the client */
        if ((ret = write(connd, buff, len)) != len)
        {
            fprintf(stderr, "ERROR: failed to write\n");
            goto clientsocket_cleanup;
        }

        /* Cleanup after this connection */
        close(connd); /* Close the connection to the client   */
    }

    printf("Shutdown complete\n");

    /* Cleanup and return */
clientsocket_cleanup:
    close(connd); /* Close the connection to the client   */
servsocket_cleanup:
    close(sockfd); /* Close the socket listening for clients     */
end:
    return; /* Return reporting a success               */

}

void main(void)
{
    TaskHandle_t task_tcpServer;
#define STACK_SIZE (1024 * 16)

    int i;

    stdio_init_all();
    for (i = 0; i < 10; i++)
    {
        printf("Starting in %dSec.\n", 10 - i);
        sleep_ms(1000);
    }

    printf("Creating tcpServer task, stack = %d\n", STACK_SIZE);
    xTaskCreate(tcpServer_test, "WifiMainThread", STACK_SIZE, NULL,
                CYW43_TASK_PRIORITY + 1, &task_tcpServer);
    vTaskStartScheduler();
}