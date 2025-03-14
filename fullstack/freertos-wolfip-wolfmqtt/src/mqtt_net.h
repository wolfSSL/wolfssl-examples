/* mqtt_net.h
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

#ifndef MQTT_NET_H
#define MQTT_NET_H

#include <wolfmqtt/mqtt_client.h>
#include <wolfmqtt/mqtt_socket.h>
#include <wolfmqtt/mqtt_types.h>
#include <wolfssl/ssl.h>
#include "../../../wolfip/wolfip.h"

/* Custom MqttNet context */
typedef struct _MqttNet {
    struct wolfIP* ipstack;
    int sockfd;
    MqttNetConnectCb connect;
    MqttNetReadCb read;
    MqttNetWriteCb write;
    MqttNetDisconnectCb disconnect;
} MqttNet;

/* Network functions */
int MqttNet_Init(MqttNet* net, struct wolfIP* ipstack);
int MqttNet_DeInit(MqttNet* net);

#endif /* MQTT_NET_H */
