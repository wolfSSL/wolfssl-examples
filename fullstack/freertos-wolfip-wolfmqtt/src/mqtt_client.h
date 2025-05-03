/* mqtt_client.h
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

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wolfmqtt/mqtt_client.h>
#include "mqtt_net.h"
#include "../include/mqtt_config.h"

/* MQTT broker settings */
#define MQTT_HOST "10.10.0.1"
#define MQTT_PORT 8883
#define MQTT_CLIENT_ID "wolfMQTT-client"
#define MQTT_TOPIC "wolfMQTT/example"
#define MQTT_QOS 1
#define MQTT_KEEP_ALIVE_SEC 60
#define MQTT_DEFAULT_CMD_TIMEOUT_MS 30000

/* MQTT Client context */
typedef struct MqttClientContext {
    MqttClient client;
    MqttNet net;
    byte *tx_buf;
    byte *rx_buf;
    int socket_fd;
} MqttClientContext;

/* Function prototypes */
int mqtt_client_init(void);
int mqtt_client_cleanup(void);

/* External access to MQTT client context */
MqttClientContext* mqtt_client_get_context(void);

#endif /* MQTT_CLIENT_H */
