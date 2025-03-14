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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "mqtt_net.h"
#include "wolfip_compat.h"
#include "wolfip_freertos.h"
#include "../include/mqtt_config.h"

/* Global variables */
static WOLFSSL_CTX* mqtt_ctx = NULL;
static WOLFSSL* mqtt_ssl = NULL;

/* Network connect callback */
static int mqtt_net_connect_cb(void *context, const char *host, word16 port, int timeout_ms)
{
    struct wolfIP *ipstack;
    struct sockaddr_in addr;
    int sockfd;
    int rc;

    /* Get wolfIP stack instance */
    ipstack = wolfip_get_stack();
    if (!ipstack) {
        printf("Failed to get wolfIP stack instance\n");
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* Create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Failed to create socket\n");
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* Set up address */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    /* Convert host string to IP address */
    addr.sin_addr.s_addr = inet_addr(host);
    
    printf("Connecting to %s:%d\n", host, port);
    printf("IP address: %08x\n", addr.sin_addr.s_addr);
    
    /* Connect socket */
    rc = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc < 0) {
        printf("Failed to connect to %s:%d, error %d\n", host, port, rc);
        close(sockfd);
        return MQTT_CODE_ERROR_NETWORK;
    }

#ifdef ENABLE_MQTT_TLS
    /* Initialize TLS */
    if (mqtt_ctx == NULL) {
        /* Initialize wolfSSL */
        mqtt_ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
        if (mqtt_ctx == NULL) {
            printf("Failed to create WOLFSSL_CTX\n");
            close(sockfd);
            return MQTT_CODE_ERROR_NETWORK;
        }

        /* Load CA certificate */
        printf("Loading CA certificate from: %s\n", MQTT_TLS_CA_CERT);
        if (wolfSSL_CTX_load_verify_locations(mqtt_ctx, MQTT_TLS_CA_CERT, NULL)
            != WOLFSSL_SUCCESS) {
            printf("Failed to load CA certificate\n");
            wolfSSL_CTX_free(mqtt_ctx);
            mqtt_ctx = NULL;
            close(sockfd);
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
            close(sockfd);
            return MQTT_CODE_ERROR_NETWORK;
        }

        printf("Loading client key from: %s\n", MQTT_TLS_CLIENT_KEY);
        if (wolfSSL_CTX_use_PrivateKey_file(mqtt_ctx, MQTT_TLS_CLIENT_KEY,
                                          WOLFSSL_FILETYPE_PEM)
            != WOLFSSL_SUCCESS) {
            printf("Failed to load client key\n");
            wolfSSL_CTX_free(mqtt_ctx);
            mqtt_ctx = NULL;
            close(sockfd);
            return MQTT_CODE_ERROR_NETWORK;
        }
    }

    /* Create wolfSSL object */
    mqtt_ssl = wolfSSL_new(mqtt_ctx);
    if (mqtt_ssl == NULL) {
        printf("Failed to create WOLFSSL object\n");
        close(sockfd);
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* Set up wolfSSL I/O */
    wolfSSL_set_fd(mqtt_ssl, sockfd);

    /* Perform TLS handshake */
    rc = wolfSSL_connect(mqtt_ssl);
    if (rc != WOLFSSL_SUCCESS) {
        printf("TLS handshake failed, error %d\n", wolfSSL_get_error(mqtt_ssl, rc));
        wolfSSL_free(mqtt_ssl);
        mqtt_ssl = NULL;
        close(sockfd);
        return MQTT_CODE_ERROR_NETWORK;
    }

    printf("TLS handshake successful\n");
#endif

    /* Store socket descriptor in context */
    *(int*)context = sockfd;

    return MQTT_CODE_SUCCESS;
}

/* Network read callback */
static int mqtt_net_read_cb(void *context, byte *buf, int buf_len, int timeout_ms)
{
    int sockfd = *(int*)context;
    int rc;

#ifdef ENABLE_MQTT_TLS
    if (mqtt_ssl) {
        rc = wolfSSL_read(mqtt_ssl, buf, buf_len);
    }
    else {
#else
    {
#endif
        rc = recv(sockfd, buf, buf_len, 0);
    }

    if (rc < 0) {
        return MQTT_CODE_ERROR_NETWORK;
    }

    return rc;
}

/* Network write callback */
static int mqtt_net_write_cb(void *context, const byte *buf, int buf_len, int timeout_ms)
{
    int sockfd = *(int*)context;
    int rc;

#ifdef ENABLE_MQTT_TLS
    if (mqtt_ssl) {
        rc = wolfSSL_write(mqtt_ssl, buf, buf_len);
    }
    else {
#else
    {
#endif
        rc = send(sockfd, buf, buf_len, 0);
    }

    if (rc < 0) {
        return MQTT_CODE_ERROR_NETWORK;
    }

    return rc;
}

/* Network disconnect callback */
static int mqtt_net_disconnect_cb(void *context)
{
    int sockfd = *(int*)context;

#ifdef ENABLE_MQTT_TLS
    /* Clean up TLS */
    if (mqtt_ssl) {
        wolfSSL_shutdown(mqtt_ssl);
        wolfSSL_free(mqtt_ssl);
        mqtt_ssl = NULL;
    }
#endif

    /* Close socket */
    if (sockfd >= 0) {
        close(sockfd);
        *(int*)context = -1;
    }

    return MQTT_CODE_SUCCESS;
}

/* Initialize MQTT network */
int mqtt_net_init(MqttNet *net)
{
    if (net == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    /* Set up network callbacks */
    net->connect = mqtt_net_connect_cb;
    net->read = mqtt_net_read_cb;
    net->write = mqtt_net_write_cb;
    net->disconnect = mqtt_net_disconnect_cb;

    /* Socket is initialized in connect callback */
    net->context = (void*)malloc(sizeof(int));
    if (net->context) {
        *(int*)net->context = -1;
    }

    return MQTT_CODE_SUCCESS;
}
