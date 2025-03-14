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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wolfssl/ssl.h>
#include "../include/mqtt_config.h"

/* Buffer sizes */
#define MAX_BUFFER_SIZE 1024
#define PRINT_BUFFER_SIZE 80

/* MQTT return codes */
#define MQTT_CODE_SUCCESS 0
#define MQTT_CODE_ERROR_TIMEOUT -1
#define MQTT_CODE_ERROR_NETWORK -2
#define MQTT_CODE_ERROR_MEMORY -3

/* MQTT client context */
typedef struct MqttClientContext {
    unsigned char *tx_buf;
    unsigned char *rx_buf;
    int socket_fd;
    WOLFSSL_CTX *ssl_ctx;
    WOLFSSL *ssl;
} MqttClientContext;

/* Function prototypes */
int mqtt_client_init(void);
int mqtt_client_cleanup(void);
MqttClientContext* mqtt_client_get_context(void);
int mqtt_client_process_message(void);
int mqtt_client_ping(void);

#endif /* WOLFMQTT_STUB_H */
