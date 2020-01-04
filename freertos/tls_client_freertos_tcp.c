/* tls_client_freertos_tcp.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
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

/* standard library */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* FreeRTOS and FreeRTOS TCP includes */
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_DNS.h"
#include "FreeRTOS_Sockets.h"

/* wolfSSL */
#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>


#define DEFAULT_PORT        11111
#define DEFAULT_TIMEOUT_MS  5000
#define CERT_FILE           "../certs/ca-cert.pem"

static int NetConnect(int* fd, const char* host, word16 port,
    int timeout_ms)
{
    int rc = -1;
    uint32_t hostIp = 0;
    struct freertos_sockaddr addr;

    hostIp = FreeRTOS_gethostbyname_a(host, NULL, 0, 0);
    if (hostIp == 0)
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = FREERTOS_AF_INET;
    addr.sin_port = FreeRTOS_htons(port);
    addr.sin_addr = hostIp;

    /* Create socket */
    *fd = FreeRTOS_socket(addr.sin_family, FREERTOS_SOCK_STREAM,
                          FREERTOS_IPPROTO_TCP);
    if (*fd == FREERTOS_INVALID_SOCKET)
        return -1;

    /* Set timeouts for socket */
    timeout_ms = pdMS_TO_TICKS(timeout_ms);
    FreeRTOS_setsockopt(*fd, 0, FREERTOS_SO_SNDTIMEO,
        (void*)&timeout_ms, sizeof(timeout_ms));
    FreeRTOS_setsockopt(*fd, 0, FREERTOS_SO_RCVTIMEO,
        (void*)&timeout_ms, sizeof(timeout_ms));

    /* Start connect */
    rc = FreeRTOS_connect(*fd, &addr, sizeof(addr));

    return rc;
}

static int NetDisconnect(int* fd)
{
    if (*fd != -1) {
        FreeRTOS_closesocket(*fd);
        *fd = -1;
    }

    return 0;
}


int main(int argc, char** argv)
{
    int    rc;
    int    sockfd;
    char   buff[256];
    size_t len;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;

    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        return 0;
    }

    /* Initialize wolfSSL */
    wolfSSL_Init();

    rc = NetConnect(&sockfd, argv[1], DEFAULT_PORT, DEFAULT_TIMEOUT_MS);
    if (rc != 0) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return rc;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    /* Load client certificates into WOLFSSL_CTX */
    if (wolfSSL_CTX_load_verify_locations(ctx, CERT_FILE, NULL)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        return -1;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        return -1;
    }

    /* Attach wolfSSL to the socket */
    wolfSSL_set_fd(ssl, sockfd);

    /* Connect to wolfSSL on the server side */
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        return -1;
    }

    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    fgets(buff, sizeof(buff), stdin);
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    if (wolfSSL_write(ssl, buff, len) != len) {
        fprintf(stderr, "ERROR: failed to write\n");
        return -1;
    }


    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(ssl, buff, sizeof(buff)-1) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        return -1;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);


    /* Cleanup and return */
    wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
    NetDisconnect(&sockfd); /* Close the connection to the server       */
    return 0;               /* Return reporting a success               */
}
