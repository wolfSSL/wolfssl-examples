/* main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <FreeRTOS.h>
#include <task.h>
#include "mqtt_client.h"
#include "wolfip_freertos.h"

/* FreeRTOS task priorities */
#define MQTT_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define UDP_TASK_PRIORITY  (tskIDLE_PRIORITY + 1)

/* UDP echo server port */
#define UDP_PORT 7

/* UDP echo server task */
static void udp_echo_task(void* pvParameters)
{
    struct wolfIP *ipstack = wolfip_get_stack();
    int sockfd;
    struct wolfIP_sockaddr_in addr;
    char buffer[1024];
    socklen_t addrlen = sizeof(addr);

    /* Create UDP socket */
    sockfd = wolfIP_sock_socket(ipstack, AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("Failed to create UDP socket\n");
        return;
    }

    /* Bind socket */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (wolfIP_sock_bind(ipstack, sockfd, (struct wolfIP_sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Failed to bind UDP socket\n");
        wolfIP_sock_close(ipstack, sockfd);
        return;
    }

    printf("UDP echo server started on port %d\n", UDP_PORT);

    while (1) {
        int len = wolfIP_sock_recvfrom(ipstack, sockfd, buffer, sizeof(buffer), 0,
                                     (struct wolfIP_sockaddr*)&addr, &addrlen);
        if (len > 0) {
            wolfIP_sock_sendto(ipstack, sockfd, buffer, len, 0,
                             (struct wolfIP_sockaddr*)&addr, addrlen);
        }
    }
}

int main(void)
{
    int ret;

    printf("Starting FreeRTOS with wolfIP...\n");

    /* Initialize wolfIP */
    ret = wolfip_init();
    if (ret < 0) {
        printf("Failed to initialize wolfIP\n");
        return -1;
    }

    /* Start UDP echo server */
    printf("Starting UDP echo server...\n");
    xTaskCreate(udp_echo_task, "UDP_Echo", configMINIMAL_STACK_SIZE * 4,
                NULL, UDP_TASK_PRIORITY, NULL);

    /* Start MQTT client */
    printf("Starting MQTT client...\n");
    ret = mqtt_client_init(wolfip_get_stack());
    if (ret < 0) {
        printf("Failed to initialize MQTT client\n");
        return -1;
    }

    ret = mqtt_client_start();
    if (ret < 0) {
        printf("Failed to start MQTT client\n");
        return -1;
    }

    /* Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
    return 0;
}
