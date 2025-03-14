/* mqtt_client.c
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
#include <time.h>
#include "wolfip_freertos.h"
#include "mqtt_client.h"
#include "../include/mqtt_config.h"

/* Buffer sizes */
#define MAX_BUFFER_SIZE 1024
#define PRINT_BUFFER_SIZE 80

/* Global MQTT client context */
static MqttClientContext mqttCtx;

/* MQTT message callback */
static int mqtt_message_cb(MqttClient *client, MqttMessage *message,
    byte msg_new, byte msg_done)
{
    char buf[PRINT_BUFFER_SIZE+1];
    word32 len;

    /* Print incoming message */
    if (msg_new) {
        /* Determine min size to dump */
        len = message->topic_name_len;
        if (len > PRINT_BUFFER_SIZE) {
            len = PRINT_BUFFER_SIZE;
        }
        memcpy(buf, message->topic_name, len);
        buf[len] = '\0'; /* Make sure it's null terminated */

        printf("MQTT Message: Topic %s, Qos %d, Len %u\n",
            buf, message->qos, message->total_len);
    }

    /* Print message payload */
    len = message->buffer_len;
    if (len > PRINT_BUFFER_SIZE) {
        len = PRINT_BUFFER_SIZE;
    }
    memcpy(buf, message->buffer, len);
    buf[len] = '\0'; /* Make sure it's null terminated */
    printf("Payload (%d - %d): %s\n",
        message->buffer_pos, message->buffer_pos + message->buffer_len, buf);

    if (msg_done) {
        printf("MQTT Message: Done\n");
    }

    return MQTT_CODE_SUCCESS;
}

/* Initialize MQTT client */
int mqtt_client_init(void)
{
    int rc;
    MqttConnect connect;
    MqttSubscribe subscribe;
    MqttTopic topics[1];
    MqttPublish publish;
    const char *message = "Hello from wolfMQTT FreeRTOS client!";

    printf("MQTT Client Initialization\n");

    /* Initialize network callbacks */
    rc = mqtt_net_init(&mqttCtx.net);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("Failed to initialize MQTT network: %d\n", rc);
        return rc;
    }

    /* Allocate buffers */
    mqttCtx.tx_buf = (byte*)malloc(MAX_BUFFER_SIZE);
    mqttCtx.rx_buf = (byte*)malloc(MAX_BUFFER_SIZE);
    if (mqttCtx.tx_buf == NULL || mqttCtx.rx_buf == NULL) {
        printf("Failed to allocate buffers\n");
        mqtt_client_cleanup();
        return MQTT_CODE_ERROR_MEMORY;
    }

    /* Initialize MQTT client */
    rc = MqttClient_Init(&mqttCtx.client, &mqttCtx.net, mqtt_message_cb,
        mqttCtx.tx_buf, MAX_BUFFER_SIZE,
        mqttCtx.rx_buf, MAX_BUFFER_SIZE,
        MQTT_DEFAULT_CMD_TIMEOUT_MS);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("Failed to initialize MQTT client: %d\n", rc);
        mqtt_client_cleanup();
        return rc;
    }

    /* Connect to broker */
    printf("Connecting to MQTT broker at %s:%d\n", MQTT_HOST, MQTT_PORT);
    rc = MqttClient_NetConnect(&mqttCtx.client, MQTT_HOST, MQTT_PORT,
        MQTT_DEFAULT_CMD_TIMEOUT_MS, 1, NULL);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("Failed to connect to MQTT broker: %d\n", rc);
        mqtt_client_cleanup();
        return rc;
    }

    /* Build connect packet */
    memset(&connect, 0, sizeof(MqttConnect));
    connect.keep_alive_sec = MQTT_KEEP_ALIVE_SEC;
    connect.clean_session = 1;
    connect.client_id = MQTT_CLIENT_ID;

    /* Send Connect and wait for Connect Ack */
    rc = MqttClient_Connect(&mqttCtx.client, &connect);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("Failed to send MQTT connect: %d\n", rc);
        mqtt_client_cleanup();
        return rc;
    }

    /* Subscribe to topic */
    memset(&subscribe, 0, sizeof(MqttSubscribe));
    subscribe.packet_id = 1;
    subscribe.topic_count = 1;
    subscribe.topics = topics;
    topics[0].topic_filter = MQTT_TOPIC;
    topics[0].qos = MQTT_QOS;

    rc = MqttClient_Subscribe(&mqttCtx.client, &subscribe);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("Failed to subscribe to topic: %d\n", rc);
        mqtt_client_cleanup();
        return rc;
    }

    /* Publish message */
    memset(&publish, 0, sizeof(MqttPublish));
    publish.retain = 0;
    publish.qos = MQTT_QOS;
    publish.duplicate = 0;
    publish.topic_name = MQTT_TOPIC;
    publish.packet_id = 2;
    publish.buffer = (byte*)message;
    publish.total_len = (word16)strlen(message);

    rc = MqttClient_Publish(&mqttCtx.client, &publish);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("Failed to publish message: %d\n", rc);
        mqtt_client_cleanup();
        return rc;
    }

    printf("MQTT client initialized successfully\n");
    return MQTT_CODE_SUCCESS;
}

/* Cleanup MQTT client */
int mqtt_client_cleanup(void)
{
    int rc = MQTT_CODE_SUCCESS;

    /* Disconnect from broker */
    if (mqttCtx.client.flags & MQTT_CLIENT_FLAG_IS_CONNECTED) {
        rc = MqttClient_Disconnect(&mqttCtx.client);
        if (rc != MQTT_CODE_SUCCESS) {
            printf("Failed to disconnect MQTT client: %d\n", rc);
        }
    }

    /* Cleanup network */
    MqttClient_NetDisconnect(&mqttCtx.client);

    /* Free resources */
    if (mqttCtx.tx_buf) {
        free(mqttCtx.tx_buf);
        mqttCtx.tx_buf = NULL;
    }
    if (mqttCtx.rx_buf) {
        free(mqttCtx.rx_buf);
        mqttCtx.rx_buf = NULL;
    }

    return rc;
}

/* Get MQTT client context */
MqttClientContext* mqtt_client_get_context(void)
{
    return &mqttCtx;
}
