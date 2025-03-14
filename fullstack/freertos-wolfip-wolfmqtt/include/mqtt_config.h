/* mqtt_config.h
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

#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

/* MQTT Configuration */
#define MQTT_MAX_PACKET_SIZE 1024
#define MQTT_DEFAULT_CMD_TIMEOUT_MS 30000
#define MQTT_KEEP_ALIVE_SEC 60
#define MQTT_CLIENT_ID "FreeRTOS_Client"
#define MQTT_TEST_TOPIC "test/topic"
#define MQTT_HOST "10.10.0.1"
#define MQTT_PORT 8883

/* Debug Configuration */
#ifndef WOLFMQTT_DEBUG
#define WOLFMQTT_DEBUG
#endif

#endif /* MQTT_CONFIG_H */
