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
#include <signal.h>
#include <time.h>

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

/* wolfSSL includes */
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

/* wolfIP includes */
#include "wolfip_compat.h"

/* Application includes */
#include "wolfip_freertos.h"
#include "wolfmqtt_stub.h"

/* Task priorities */
#define WOLFIP_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define MQTT_TASK_PRIORITY   (tskIDLE_PRIORITY + 1)

/* Task stack sizes */
#define WOLFIP_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)
#define MQTT_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 4)

/* Task handles */
static TaskHandle_t wolfip_task_handle = NULL;
static TaskHandle_t mqtt_task_handle = NULL;

/* Signal handler for clean exit */
static void signal_handler(int sig)
{
    (void)sig;
    printf("Caught signal, exiting...\n");
    vTaskEndScheduler();
}

/* wolfIP task */
static void wolfip_task(void *pvParameters)
{
    (void)pvParameters;
    struct wolfIP *ipstack = wolfip_get_stack();
    uint64_t now;

    printf("wolfIP task started\n");

    while (1) {
        now = time(NULL) * 1000; /* Get current time in milliseconds */
        wolfIP_poll(ipstack, now);
        vTaskDelay(pdMS_TO_TICKS(10)); /* 10ms delay */
    }
}

/* MQTT client task */
static void mqtt_task(void *pvParameters)
{
    int rc;
    MqttClientContext *mqttCtx;
    
    (void)pvParameters;

    printf("MQTT task started\n");

    /* Wait for wolfIP to initialize */
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Initialize MQTT client */
    rc = mqtt_client_init();
    if (rc != MQTT_CODE_SUCCESS) {
        printf("Failed to initialize MQTT client: %d\n", rc);
        vTaskDelete(NULL);
        return;
    }
    
    /* Get MQTT client context */
    mqttCtx = mqtt_client_get_context();
    
    /* Process messages */
    printf("Waiting for MQTT messages...\n");
    while (1) {
        /* Process messages */
        rc = mqtt_client_process_message();
        if (rc != MQTT_CODE_SUCCESS) {
            printf("MQTT message processing error: %d\n", rc);
            break;
        }
        
        /* Send ping to keep connection alive */
        rc = mqtt_client_ping();
        if (rc != MQTT_CODE_SUCCESS) {
            printf("MQTT ping failed: %d\n", rc);
            break;
        }
        
        /* Sleep to prevent CPU hogging */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    /* Cleanup MQTT client */
    mqtt_client_cleanup();
    
    /* Task complete */
    vTaskDelete(NULL);
}

int main(void)
{
    /* Initialize random seed */
    srand(time(NULL));

    /* Set up signal handler */
    signal(SIGINT, signal_handler);

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Initialize wolfIP */
    if (wolfip_init() != 0) {
        printf("Failed to initialize wolfIP\n");
        return -1;
    }

    /* Create wolfIP task */
    if (xTaskCreate(wolfip_task, "wolfIP", WOLFIP_TASK_STACK_SIZE,
                   NULL, WOLFIP_TASK_PRIORITY, &wolfip_task_handle) != pdPASS) {
        printf("Failed to create wolfIP task\n");
        return -1;
    }

    /* Create MQTT client task */
    if (xTaskCreate(mqtt_task, "MQTT", MQTT_TASK_STACK_SIZE,
                   NULL, MQTT_TASK_PRIORITY, &mqtt_task_handle) != pdPASS) {
        printf("Failed to create MQTT task\n");
        return -1;
    }

    /* Start the scheduler */
    vTaskStartScheduler();

    /* Should never reach here unless there is insufficient RAM */
    printf("Scheduler exited\n");

    /* Cleanup */
    wolfSSL_Cleanup();

    return 0;
}
