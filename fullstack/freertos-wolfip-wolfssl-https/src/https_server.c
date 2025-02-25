/* https_server.c
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

#include "https_server.h"
#include "httpd.h"
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

static WOLFSSL_CTX *g_ssl_ctx = NULL;
static struct httpd g_httpd;

/* Root page handler */
static int handle_root(struct httpd *httpd, struct http_client *hc, struct http_request *req) {
    const char *response = "<html><body><h1>wolfSSL HTTPS Demo</h1>"
                          "<p>TLS 1.3 + FreeRTOS + wolfIP</p></body></html>";
    http_send_response_headers(hc, HTTP_STATUS_OK, "OK", "text/html", strlen(response));
    http_send_response_body(hc, response, strlen(response));
    return 0;
}

int https_server_init(struct wolfIP *ipstack) {
    int ret;

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        printf("Failed to initialize wolfSSL\n");
        return -1;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((g_ssl_ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method())) == NULL) {
        printf("Failed to create WOLFSSL_CTX\n");
        return -1;
    }

    /* Load server certificates */
    if ((ret = wolfSSL_CTX_use_certificate_file(g_ssl_ctx, CERT_FILE, 
        WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        printf("Failed to load %s\n", CERT_FILE);
        return -1;
    }

    /* Load server key */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(g_ssl_ctx, KEY_FILE,
        WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        printf("Failed to load %s\n", KEY_FILE);
        return -1;
    }

    /* Initialize HTTP server with SSL context */
    if (httpd_init(&g_httpd, ipstack, HTTPS_PORT, g_ssl_ctx) != 0) {
        printf("Failed to initialize HTTPS server\n");
        return -1;
    }

    /* Register handlers */
    if (httpd_register_handler(&g_httpd, "/", handle_root) != 0) {
        printf("Failed to register root handler\n");
        return -1;
    }

    printf("HTTPS server initialized on port %d\n", HTTPS_PORT);
    return 0;
}

static void https_server_task(void* pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(100);
    
    printf("HTTPS server task started\n");
    
    /* Task main loop - wolfIP handles connections in callbacks */
    for(;;) {
        vTaskDelay(xDelay);
    }
}

int https_server_start(void) {
    BaseType_t ret;
    
    ret = xTaskCreate(https_server_task,
                     "HTTPS_Server",
                     HTTPS_TASK_STACK_SIZE,
                     NULL,
                     HTTPS_TASK_PRIORITY,
                     NULL);
                     
    return (ret == pdPASS) ? 0 : -1;
}
