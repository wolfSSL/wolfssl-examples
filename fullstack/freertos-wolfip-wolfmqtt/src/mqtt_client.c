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

#include "mqtt_client.h"
#include <wolfmqtt/mqtt_client.h>
#include <wolfmqtt/mqtt_socket.h>

/* Global instances */
static MqttClient g_mqtt_client;
static MqttNet g_mqtt_net;
static struct wolfIP *g_ipstack;

/* MQTT message callback */
static int mqtt_message_cb(MqttClient *client, MqttMessage *msg,
    byte msg_new, byte msg_done)
{
    byte buf[MQTT_MAX_PACKET_SIZE];
    word32 len;

    (void)client;

    if (msg_new) {
        /* Message received */
        len = msg->buffer_len;
        if (len > sizeof(buf)) {
            len = sizeof(buf);
        }
        memcpy(buf, msg->buffer, len);
        printf("MQTT Message: Topic %s, Data: %s\n", 
               msg->topic_name, buf);
    }

    return MQTT_CODE_SUCCESS;
}

/* MQTT client task */
static void mqtt_client_task(void* pvParameters)
{
    int rc;
    MqttNet net;
    MqttConnect connect;
    const char *test_topic = "test/topic";
    byte buf[MQTT_MAX_PACKET_SIZE];
    
    /* Initialize MQTT client */
    word16 port = MQTT_PORT;
    word32 buf_len = sizeof(buf);
    rc = MqttClient_Init(&g_mqtt_client, &g_mqtt_net, mqtt_message_cb,
        buf, buf_len, buf, buf_len, MQTT_DEFAULT_CMD_TIMEOUT_MS);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("MQTT client init failed: %d\n", rc);
        return;
    }
    
    printf("MQTT client initialized successfully\n");
    
    /* Enable non-blocking mode */
    printf("Connecting to MQTT broker at %s:%d...\n", MQTT_HOST, MQTT_PORT);
    rc = MqttClient_NetConnect(&g_mqtt_client, MQTT_HOST, MQTT_PORT,
        MQTT_DEFAULT_CMD_TIMEOUT_MS, 0, NULL);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("MQTT client connection failed: %d\n", rc);
        return;
    }
    printf("MQTT client connected successfully\n");

    /* Connect to broker */
    memset(&connect, 0, sizeof(connect));
    connect.keep_alive_sec = MQTT_KEEP_ALIVE_SEC;
    connect.clean_session = 1;
    connect.client_id = MQTT_CLIENT_ID;

    printf("Connecting to MQTT broker with client ID: %s\n", MQTT_CLIENT_ID);
    rc = MqttClient_Connect(&g_mqtt_client, &connect);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("MQTT connect failed: %d\n", rc);
        MqttClient_NetDisconnect(&g_mqtt_client);
        return;
    }
    printf("MQTT connection established\n");

    printf("MQTT Connected\n");

    /* Subscribe to test topic */
    MqttSubscribe subscribe;
    MqttTopic topics[1];
    memset(&subscribe, 0, sizeof(MqttSubscribe));
    subscribe.packet_id = mqtt_get_packetid();
    subscribe.topic_count = 1;
    subscribe.topics = topics;
    topics[0].topic_filter = MQTT_TEST_TOPIC;
    topics[0].qos = 0;

    rc = MqttClient_Subscribe(&g_mqtt_client, &subscribe);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("MQTT subscribe failed: %d\n", rc);
        return;
    }

    printf("Subscribed to %s\n", test_topic);

    /* Main loop */
    while (1) {
        /* Publish message every 5 seconds */
        const char *msg = "Hello from FreeRTOS!";
        MqttPublish publish;
        memset(&publish, 0, sizeof(publish));
        publish.retain = 0;
        publish.qos = 0;
        publish.duplicate = 0;
        publish.topic_name = test_topic;
        publish.buffer = (byte*)msg;
        publish.total_len = strlen(msg);

        rc = MqttClient_Publish(&g_mqtt_client, &publish);
        if (rc != MQTT_CODE_SUCCESS) {
            printf("MQTT publish failed: %d\n", rc);
        }

        /* Check for incoming messages */
        rc = MqttClient_WaitMessage(&g_mqtt_client, 5000);
        if (rc == MQTT_CODE_ERROR_TIMEOUT) {
            /* Expected */
        }
        else if (rc != MQTT_CODE_SUCCESS) {
            printf("MQTT wait error: %d\n", rc);
        }
    }
}

int mqtt_client_init(struct wolfIP *ipstack)
{
    int rc;
    printf("Initializing MQTT client with wolfIP...\n");
    g_ipstack = ipstack;
    rc = MqttNet_Init(&g_mqtt_net, ipstack);
    if (rc != MQTT_CODE_SUCCESS) {
        printf("Failed to initialize MQTT network: %d\n", rc);
        return rc;
    }
    printf("MQTT network initialized successfully\n");
    return MQTT_CODE_SUCCESS;
}

int mqtt_client_start(void)
{
    BaseType_t ret;
    
    ret = xTaskCreate(mqtt_client_task,
                     "MQTT_Client",
                     MQTT_TASK_STACK_SIZE,
                     NULL,
                     MQTT_TASK_PRIORITY,
                     NULL);
                     
    return (ret == pdPASS) ? 0 : -1;
}
