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

/* Enable TLS for MQTT */
#define ENABLE_MQTT_TLS

/* MQTT TLS certificate paths */
#define MQTT_TLS_CA_CERT "../../../wolfssl/certs/ca-cert.pem"
#define MQTT_TLS_CLIENT_CERT "../../../wolfssl/certs/client-cert.pem"
#define MQTT_TLS_CLIENT_KEY "../../../wolfssl/certs/client-key.pem"

/* MQTT client configuration */
#define MQTT_HOST "10.10.0.1"
#define MQTT_PORT 8883
#define MQTT_TOPIC "test/topic"
#define MQTT_QOS 0
#define MQTT_CLIENT_ID "wolfMQTT-client"

#endif /* MQTT_CONFIG_H */
