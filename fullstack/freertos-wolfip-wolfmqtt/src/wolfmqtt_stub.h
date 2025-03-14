/* wolfmqtt_stub.h
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

#ifndef WOLFMQTT_STUB_H
#define WOLFMQTT_STUB_H

#include <stdint.h>

/* Basic types */
typedef unsigned char byte;
typedef uint16_t word16;
typedef uint32_t word32;

/* Return codes */
#define MQTT_CODE_SUCCESS 0
#define MQTT_CODE_ERROR_TIMEOUT -1
#define MQTT_CODE_ERROR_NETWORK -2

/* MQTT Client */
typedef struct _MqttClient {
    void *ctx;
} MqttClient;

/* MQTT Message */
typedef struct _MqttMessage {
    const char *topic_name;
    uint16_t topic_name_len;
    byte qos;
    uint32_t total_len;
    byte *buffer;
    uint32_t buffer_len;
} MqttMessage;

/* MQTT Connect */
typedef struct _MqttConnect {
    const char *client_id;
    uint16_t keep_alive_sec;
    byte clean_session;
} MqttConnect;

/* MQTT Topic */
typedef struct _MqttTopic {
    const char *topic_filter;
    byte qos;
} MqttTopic;

/* MQTT Subscribe */
typedef struct _MqttSubscribe {
    uint16_t packet_id;
    uint16_t topic_count;
    MqttTopic *topics;
} MqttSubscribe;

/* MQTT Publish */
typedef struct _MqttPublish {
    uint16_t packet_id;
    const char *topic_name;
    byte *buffer;
    uint32_t total_len;
    byte qos;
    byte retain;
    byte duplicate;
} MqttPublish;

/* MQTT Network Callbacks */
typedef int (*MqttNetConnectCb)(void *context, const char *host, word16 port, int timeout_ms);
typedef int (*MqttNetReadCb)(void *context, byte *buf, int buf_len, int timeout_ms);
typedef int (*MqttNetWriteCb)(void *context, const byte *buf, int buf_len, int timeout_ms);
typedef int (*MqttNetDisconnectCb)(void *context);

/* MQTT Network */
typedef struct _MqttNet {
    void *context;
    MqttNetConnectCb connect;
    MqttNetReadCb read;
    MqttNetWriteCb write;
    MqttNetDisconnectCb disconnect;
} MqttNet;

/* MQTT Message Callback */
typedef int (*MqttMsgCb)(MqttClient *client, MqttMessage *msg, byte msg_new, byte msg_done);

/* Function prototypes */
int MqttClient_Init(MqttClient *client, MqttNet *net, MqttMsgCb msg_cb,
                   byte *tx_buf, int tx_buf_len, byte *rx_buf, int rx_buf_len,
                   int cmd_timeout_ms);
int MqttClient_Connect(MqttClient *client, MqttConnect *connect);
int MqttClient_Subscribe(MqttClient *client, MqttSubscribe *subscribe);
int MqttClient_Publish(MqttClient *client, MqttPublish *publish);
int MqttClient_WaitMessage(MqttClient *client, int timeout_ms);
int MqttClient_Disconnect(MqttClient *client);

#endif /* WOLFMQTT_STUB_H */
