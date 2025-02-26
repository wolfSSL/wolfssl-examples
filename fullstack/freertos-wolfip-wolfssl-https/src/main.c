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
#include "FreeRTOS.h"
#include "task.h"
#include "wolfip_freertos.h"
#include "https_server.h"

static void testTask(void* pvParameters) {
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    int ret;
    
    printf("Initializing wolfIP...\n");
    ret = wolfIP_FreeRTOS_Init();
    if (ret != 0) {
        printf("Failed to initialize wolfIP\n");
        return;
    }
    
    printf("Starting wolfIP network task...\n");
    ret = wolfIP_FreeRTOS_Start();
    if (ret != 0) {
        printf("Failed to start wolfIP network task\n");
        return;
    }

    printf("Starting UDP echo server...\n");
    ret = wolfIP_Start_UDP_Echo();
    if (ret != 0) {
        printf("Failed to start UDP echo server\n");
        return;
    }

    printf("Starting HTTPS server...\n");
    ret = https_server_init(g_wolfip);
    if (ret != 0) {
        printf("Failed to initialize HTTPS server\n");
        return;
    }
    ret = https_server_start();
    if (ret != 0) {
        printf("Failed to start HTTPS server\n");
        return;
    }
    
    printf("Network stack, UDP echo server, and HTTPS server running...\n");
    for(;;) {
        vTaskDelay(xDelay);
    }
}

int main(void) {
    printf("Starting FreeRTOS with wolfIP...\n");
    
    /* Create the test task */
    xTaskCreate(testTask, "TestTask", configMINIMAL_STACK_SIZE, 
                NULL, tskIDLE_PRIORITY + 1, NULL);
    
    /* Start the scheduler */
    vTaskStartScheduler();
    
    /* Should never reach here */
    return 0;
}
