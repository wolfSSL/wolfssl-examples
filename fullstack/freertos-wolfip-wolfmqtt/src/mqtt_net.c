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

#include <stdio.h>
#include <string.h>
#include "mqtt_net.h"
#include "wolfip/socket.h"
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

/* TLS Configuration */
#define MQTT_TLS_TIMEOUT_MS 10000
#define MQTT_TLS_VERIFY_PEER 1

/* Global TLS context */
static WOLFSSL_CTX* mqtt_ctx;
static WOLFSSL* mqtt_ssl;

/* Network callbacks */
static int mqtt_net_connect_cb(void* context, const char* host, word16 port,
    int timeout_ms)
{
    int rc;
    struct wolfIP_sockaddr_in addr;
    MqttNet* net = (MqttNet*)context;

    /* Create socket */
    net->sockfd = wolfIP_sock_socket(net->ipstack, WOLFIP_AF_INET,
                                   WOLFIP_SOCK_STREAM, 0);
    if (net->sockfd < 0) {
        printf("Failed to create socket\n");
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* Setup address */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = WOLFIP_AF_INET;
    addr.sin_port = wolfIP_htons(port);
    addr.sin_addr.s_addr = wolfIP_inet_addr(host);
    
    printf("Connecting to %s:%d\n", host, port);
    printf("IP address: %08x\n", addr.sin_addr.s_addr);
    printf("Attempting to connect to %s:%d (IP: %08x)\n", host, port, addr.sin_addr.s_addr);
    
    /* Connect socket */
    rc = wolfIP_sock_connect(net->ipstack, net->sockfd,
                           (struct wolfIP_sockaddr*)&addr, sizeof(addr));
    if (rc < 0) {
        printf("Failed to connect to %s:%d\n", host, port);
        wolfIP_sock_close(net->ipstack, net->sockfd);
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* Setup TLS if enabled */
    #ifdef ENABLE_MQTT_TLS
    if (mqtt_ctx == NULL) {
        /* Initialize wolfSSL */
        wolfSSL_Init();
        
        /* Create and initialize WOLFSSL_CTX */
        mqtt_ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
        if (mqtt_ctx == NULL) {
            printf("Failed to create WOLFSSL_CTX\n");
            wolfIP_sock_close(net->ipstack, net->sockfd);
            return MQTT_CODE_ERROR_NETWORK;
        }

        /* Load CA certificate */
        printf("Loading CA certificate from: %s\n", MQTT_TLS_CA_CERT);
        if (wolfSSL_CTX_load_verify_locations(mqtt_ctx, MQTT_TLS_CA_CERT, NULL)
            != WOLFSSL_SUCCESS) {
            printf("Failed to load CA certificate\n");
            wolfSSL_CTX_free(mqtt_ctx);
            mqtt_ctx = NULL;
            wolfIP_sock_close(net->ipstack, net->sockfd);
            return MQTT_CODE_ERROR_NETWORK;
        }

        /* Load client certificate and key */
        printf("Loading client certificate from: %s\n", MQTT_TLS_CLIENT_CERT);
        if (wolfSSL_CTX_use_certificate_file(mqtt_ctx, MQTT_TLS_CLIENT_CERT,
                                           WOLFSSL_FILETYPE_PEM)
            != WOLFSSL_SUCCESS) {
            printf("Failed to load client certificate\n");
            wolfSSL_CTX_free(mqtt_ctx);
            mqtt_ctx = NULL;
            wolfIP_sock_close(net->ipstack, net->sockfd);
            return MQTT_CODE_ERROR_NETWORK;
        }

        printf("Loading client key from: %s\n", MQTT_TLS_CLIENT_KEY);
        if (wolfSSL_CTX_use_PrivateKey_file(mqtt_ctx, MQTT_TLS_CLIENT_KEY,
                                          WOLFSSL_FILETYPE_PEM)
            != WOLFSSL_SUCCESS) {
            printf("Failed to load client key\n");
            wolfSSL_CTX_free(mqtt_ctx);
            mqtt_ctx = NULL;
            wolfIP_sock_close(net->ipstack, net->sockfd);
            return MQTT_CODE_ERROR_NETWORK;
        }
    }

    /* Create new WOLFSSL object */
    mqtt_ssl = wolfSSL_new(mqtt_ctx);
    if (mqtt_ssl == NULL) {
        printf("Failed to create WOLFSSL object\n");
        wolfIP_sock_close(net->ipstack, net->sockfd);
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* Associate the socket with WOLFSSL */
    wolfSSL_set_fd(mqtt_ssl, net->sockfd);

    /* Connect TLS */
    rc = wolfSSL_connect(mqtt_ssl);
    if (rc != WOLFSSL_SUCCESS) {
        printf("TLS connect failed, error: %d\n",
               wolfSSL_get_error(mqtt_ssl, rc));
        wolfSSL_free(mqtt_ssl);
        mqtt_ssl = NULL;
        wolfIP_sock_close(net->ipstack, net->sockfd);
        return MQTT_CODE_ERROR_NETWORK;
    }

    printf("TLS connection established\n");
    #endif

    return MQTT_CODE_SUCCESS;
}

static int mqtt_net_read_cb(void* context, byte* buf, int buf_len,
    int timeout_ms)
{
    int rc;
    MqttNet* net = (MqttNet*)context;

#ifdef ENABLE_MQTT_TLS
    if (mqtt_ssl) {
        rc = wolfSSL_read(mqtt_ssl, buf, buf_len);
    }
    else {
#endif
        rc = wolfIP_sock_recv(net->ipstack, net->sockfd, buf, buf_len, 0);
#ifdef ENABLE_MQTT_TLS
    }
#endif

    if (rc < 0) {
        return MQTT_CODE_ERROR_NETWORK;
    }

    return rc;
}

static int mqtt_net_write_cb(void* context, const byte* buf, int buf_len,
    int timeout_ms)
{
    int rc;
    MqttNet* net = (MqttNet*)context;

#ifdef ENABLE_MQTT_TLS
    if (mqtt_ssl) {
        rc = wolfSSL_write(mqtt_ssl, buf, buf_len);
    }
    else {
#endif
        rc = wolfIP_sock_send(net->ipstack, net->sockfd, buf, buf_len, 0);
#ifdef ENABLE_MQTT_TLS
    }
#endif

    if (rc < 0) {
        return MQTT_CODE_ERROR_NETWORK;
    }

    return rc;
}

static int mqtt_net_disconnect_cb(void* context)
{
    MqttNet* net = (MqttNet*)context;

    #ifdef ENABLE_MQTT_TLS
    if (mqtt_ssl) {
        wolfSSL_free(mqtt_ssl);
        mqtt_ssl = NULL;
    }
    #endif

    if (net->sockfd >= 0) {
        wolfIP_sock_close(net->ipstack, net->sockfd);
        net->sockfd = -1;
    }

    return MQTT_CODE_SUCCESS;
}

int MqttNet_Init(MqttNet* net, struct wolfIP* ipstack)
{
    if (net == NULL || ipstack == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    memset(net, 0, sizeof(MqttNet));

    net->ipstack = ipstack;
    net->sockfd = -1;
    net->connect = mqtt_net_connect_cb;
    net->read = mqtt_net_read_cb;
    net->write = mqtt_net_write_cb;
    net->disconnect = mqtt_net_disconnect_cb;

    return MQTT_CODE_SUCCESS;
}

int MqttNet_DeInit(MqttNet* net)
{
    if (net) {
        mqtt_net_disconnect_cb(net);
    }

    #ifdef ENABLE_MQTT_TLS
    if (mqtt_ctx) {
        wolfSSL_CTX_free(mqtt_ctx);
        mqtt_ctx = NULL;
        wolfSSL_Cleanup();
    }
    #endif

    return MQTT_CODE_SUCCESS;
}
