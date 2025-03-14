#include "mqtt_config.h"
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

#include "FreeRTOS.h"
#include "task.h"
#include "wolfip.h"
#include "mqtt_net.h"
#include <wolfmqtt/mqtt_client.h>

/* Get next packet ID */
word16 mqtt_get_packetid(void);

/* MQTT Configuration */
#define MQTT_HOST "10.10.0.1"
#define MQTT_PORT 8883
#define MQTT_KEEP_ALIVE_SEC 60
#define MQTT_CLIENT_ID "FreeRTOS_Client"
#define MQTT_TEST_TOPIC "test/topic"

/* Task Configuration */
#define MQTT_TASK_STACK_SIZE (16 * 1024)
#define MQTT_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

/* Initialize MQTT client with wolfIP */
int mqtt_client_init(struct wolfIP *ipstack);

/* Start MQTT client task */
int mqtt_client_start(void);

#endif /* MQTT_CLIENT_H */
