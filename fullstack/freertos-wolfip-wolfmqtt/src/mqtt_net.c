/* mqtt_net.c
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

#include "mqtt_net.h"
#include <wolfip.h>
#include <wolfssl/ssl.h>

/* Static TLS context */
static WOLFSSL_CTX* mqtt_tls_ctx;
static WOLFSSL* mqtt_tls;

/* Static packet ID counter */
static word16 packet_id = 0;

/* Initialize TLS */
static int mqtt_tls_init(void) {
    wolfSSL_Init();
    mqtt_tls_ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
    if (!mqtt_tls_ctx) {
        printf("Failed to create WOLFSSL_CTX\n");
        return -1;
    }

    if (wolfSSL_CTX_load_verify_locations(mqtt_tls_ctx, MQTT_TLS_CA_CERT, NULL) != SSL_SUCCESS) {
        printf("Failed to load CA certificate\n");
        return -1;
    }
    if (wolfSSL_CTX_use_certificate_file(mqtt_tls_ctx, MQTT_TLS_CLIENT_CERT, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Failed to load client certificate\n");
        return -1;
    }
    if (wolfSSL_CTX_use_PrivateKey_file(mqtt_tls_ctx, MQTT_TLS_CLIENT_KEY, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Failed to load client key\n");
        return -1;
    }

    printf("TLS initialization successful\n");
    return 0;
}

/* Get next packet ID */
word16 mqtt_get_packetid(void) {
    return ++packet_id;
}

/* Network Callbacks */
static int mqtt_net_connect(void *context, const char* host, word16 port,
    int timeout_ms)
{
    MqttNetContext *net = (MqttNetContext*)context;
    int rc;

    /* Create TCP socket */
    printf("Creating TCP socket...\n");
    net->sockfd = wolfIP_sock_socket(net->ipstack, WOLFIP_AF_INET, WOLFIP_SOCK_STREAM, 0);
    if (net->sockfd < 0) {
        printf("Failed to create socket\n");
        return MQTT_CODE_ERROR_NETWORK;
    }
    printf("Socket created successfully\n");

    /* Connect to host */
    struct wolfIP_sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = WOLFIP_AF_INET;
    addr.sin_port = port;
    printf("Connecting to %s:%d\n", host, port);
    addr.sin_addr.s_addr = atoip4(host);
    printf("IP address: %08x\n", addr.sin_addr.s_addr);
    printf("Attempting to connect to %s:%d (IP: %08x)\n", host, port, addr.sin_addr.s_addr);
    rc = wolfIP_sock_connect(net->ipstack, net->sockfd, (struct wolfIP_sockaddr*)&addr, sizeof(addr));
    if (rc != 0) {
        printf("Socket connect failed: %d\n", rc);
        wolfIP_sock_close(net->ipstack, net->sockfd);
        return MQTT_CODE_ERROR_NETWORK;
    }
    printf("Socket connected successfully\n");

    /* Initialize TLS */
    net->ssl = wolfSSL_new(mqtt_tls_ctx);
    if (!net->ssl) {
        printf("Failed to create WOLFSSL object\n");
        wolfIP_sock_close(net->ipstack, net->sockfd);
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* Set the socket file descriptor */
    wolfSSL_set_fd(net->ssl, net->sockfd);

    /* Connect TLS */
    printf("Starting TLS handshake...\n");
    if (wolfSSL_connect(net->ssl) != SSL_SUCCESS) {
        printf("TLS handshake failed\n");
        wolfSSL_free(net->ssl);
        net->ssl = NULL;
        wolfIP_sock_close(net->ipstack, net->sockfd);
        return MQTT_CODE_ERROR_NETWORK;
    }
    printf("TLS connection established\n");

    return MQTT_CODE_SUCCESS;
}

static int mqtt_net_read(void *context, byte* buf, int buf_len,
    int timeout_ms)
{
    MqttNetContext *net = (MqttNetContext*)context;
    int rc;

    if (context == NULL || buf == NULL || buf_len <= 0) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    if (net->ssl) {
        rc = wolfSSL_read(net->ssl, buf, buf_len);
    } else {
        rc = wolfIP_sock_recv(net->ipstack, net->sockfd, buf, buf_len, 0);
    }
    if (rc < 0) {
        return MQTT_CODE_ERROR_NETWORK;
    }

    return rc;
}

static int mqtt_net_write(void *context, const byte* buf, int buf_len,
    int timeout_ms)
{
    MqttNetContext *net = (MqttNetContext*)context;
    int rc;

    if (context == NULL || buf == NULL || buf_len <= 0) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    if (net->ssl) {
        rc = wolfSSL_write(net->ssl, buf, buf_len);
    } else {
        rc = wolfIP_sock_send(net->ipstack, net->sockfd, buf, buf_len, 0);
    }
    if (rc < 0) {
        return MQTT_CODE_ERROR_NETWORK;
    }

    return rc;
}

static int mqtt_net_disconnect(void *context)
{
    MqttNetContext *net = (MqttNetContext*)context;

    if (net) {
        if (net->ssl) {
            wolfSSL_free(net->ssl);
            net->ssl = NULL;
        }
        wolfIP_sock_close(net->ipstack, net->sockfd);
        net->sockfd = -1;
    }

    return MQTT_CODE_SUCCESS;
}

int MqttNet_Init(MqttNet* net, struct wolfIP* ipstack)
{
    int rc;
    
    if (net == NULL || ipstack == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    /* Initialize TLS */
    rc = mqtt_tls_init();
    if (rc != 0) {
        printf("TLS initialization failed\n");
        return MQTT_CODE_ERROR_NETWORK;
    }

    XMEMSET(net, 0, sizeof(MqttNet));

    net->connect = mqtt_net_connect;
    net->read = mqtt_net_read;
    net->write = mqtt_net_write;
    net->disconnect = mqtt_net_disconnect;

    /* Setup network context */
    net->context = (MqttNetContext*)WOLFMQTT_MALLOC(sizeof(MqttNetContext));
    if (net->context == NULL) {
        wolfSSL_CTX_free(mqtt_tls_ctx);
        return MQTT_CODE_ERROR_MEMORY;
    }

    ((MqttNetContext*)net->context)->ipstack = ipstack;
    ((MqttNetContext*)net->context)->sockfd = -1;
    ((MqttNetContext*)net->context)->ssl = NULL;

    printf("MQTT network initialized successfully\n");
    return MQTT_CODE_SUCCESS;
}

int MqttNet_DeInit(MqttNet* net)
{
    if (net) {
        if (net->context) {
            mqtt_net_disconnect(net->context);
            WOLFMQTT_FREE(net->context);
        }
        if (mqtt_tls_ctx) {
            wolfSSL_CTX_free(mqtt_tls_ctx);
            mqtt_tls_ctx = NULL;
        }
        wolfSSL_Cleanup();
        XMEMSET(net, 0, sizeof(MqttNet));
    }
    return MQTT_CODE_SUCCESS;
}
