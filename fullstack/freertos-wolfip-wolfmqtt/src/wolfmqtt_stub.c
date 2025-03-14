/* wolfmqtt_stub.c
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
#include "wolfmqtt_stub.h"

/* Initialize MQTT client */
int MqttClient_Init(MqttClient *client, MqttNet *net, MqttMsgCb msg_cb,
                   byte *tx_buf, int tx_buf_len, byte *rx_buf, int rx_buf_len,
                   int cmd_timeout_ms)
{
    printf("MqttClient_Init stub called\n");
    (void)client;
    (void)net;
    (void)msg_cb;
    (void)tx_buf;
    (void)tx_buf_len;
    (void)rx_buf;
    (void)rx_buf_len;
    (void)cmd_timeout_ms;
    return MQTT_CODE_SUCCESS;
}

/* Connect to MQTT broker */
int MqttClient_Connect(MqttClient *client, MqttConnect *connect)
{
    printf("MqttClient_Connect stub called\n");
    printf("  Client ID: %s\n", connect->client_id);
    printf("  Keep Alive: %d seconds\n", connect->keep_alive_sec);
    printf("  Clean Session: %d\n", connect->clean_session);
    (void)client;
    return MQTT_CODE_SUCCESS;
}

/* Subscribe to MQTT topic */
int MqttClient_Subscribe(MqttClient *client, MqttSubscribe *subscribe)
{
    printf("MqttClient_Subscribe stub called\n");
    printf("  Packet ID: %d\n", subscribe->packet_id);
    printf("  Topic Count: %d\n", subscribe->topic_count);
    for (int i = 0; i < subscribe->topic_count; i++) {
        printf("  Topic %d: %s (QoS %d)\n", i, 
               subscribe->topics[i].topic_filter,
               subscribe->topics[i].qos);
    }
    (void)client;
    return MQTT_CODE_SUCCESS;
}

/* Publish MQTT message */
int MqttClient_Publish(MqttClient *client, MqttPublish *publish)
{
    printf("MqttClient_Publish stub called\n");
    printf("  Packet ID: %d\n", publish->packet_id);
    printf("  Topic: %s\n", publish->topic_name);
    printf("  QoS: %d\n", publish->qos);
    printf("  Retain: %d\n", publish->retain);
    printf("  Duplicate: %d\n", publish->duplicate);
    printf("  Message: %.*s\n", (int)publish->total_len, publish->buffer);
    (void)client;
    return MQTT_CODE_SUCCESS;
}

/* Wait for MQTT message */
int MqttClient_WaitMessage(MqttClient *client, int timeout_ms)
{
    (void)client;
    (void)timeout_ms;
    /* Simulate timeout */
    return MQTT_CODE_ERROR_TIMEOUT;
}

/* Disconnect from MQTT broker */
int MqttClient_Disconnect(MqttClient *client)
{
    printf("MqttClient_Disconnect stub called\n");
    (void)client;
    return MQTT_CODE_SUCCESS;
}
