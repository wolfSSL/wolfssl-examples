/* client-dtls13.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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
 *
 *=============================================================================
 *
 * Bare-bones example of a DTLS 1.3 server for instructional/learning purposes.
 * This example can only accept one connection at a time.
 *
 * Define USE_DTLS12 to use DTLS 1.2 instead of DTLS 1.3
 */

/*
 * This code was adapted from the wolfSSL/wolfssl-examples/dtls located at:
 *
 * https://github.com/wolfSSL/wolfssl-examples/blob/master/dtls/client-dtls13.c
 */
#include "client-dtls13.h"

#if defined(SINGLE_THREADED)
    #define WOLFSSL_ESP_TASK int
#else
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <freertos/event_groups.h>
    #define WOLFSSL_ESP_TASK void
#endif

/* Espressif socket */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* #include <signal.h> not fully implemented in ESP-IDF */
#include <lwip/netdb.h>
#include <lwip/sockets.h>

/* wolfSSL */
#include <wolfssl/wolfcrypt/settings.h>
#include "user_settings.h" /* always before other wolfssl files */

#include <wolfssl/ssl.h>
#include <errno.h>
#include <wolfssl/certs_test.h>

/* this app */
#include "dtls-common.h"

/* convert macros values to string */
#define STRINGIFY(x) #x

static const char* const TAG = "server-dtls13";

WOLFSSL_CTX*  ctx = NULL;
WOLFSSL*      ssl = NULL;
int           listenfd = INVALID_SOCKET;   /* Initialize our socket */

/* Note: not implemented at this time:
 * static void sig_handler(const int sig); */

/* show stack space for this task */
static int ShowStackInfo(char* msg)
{
    int ret;
    ret = TLS_SMP_CLIENT_TASK_WORDS - (uxTaskGetStackHighWaterMark(NULL));
    ESP_LOGI(TAG, "%s: %d words", msg, ret);
    return ret;
}

#if 0
int main(int argc, char** argv)
#else
WOLFSSL_ESP_TASK dtls13_smp_client_task(void *pvParameters)
#endif
{
    /* Loc short for "location" */
#if defined(SINGLE_THREADED)
    #define TLS_SMP_CLIENT_TASK_RET exitVal
#else
    #define TLS_SMP_CLIENT_TASK_RET
#endif

    /* standard variables used in a dtls client*/
    int             n = 0;
    int             sockfd = INVALID_SOCKET;
    int             err;
    int             ret;
    struct          sockaddr_in servAddr;
    WOLFSSL*        ssl = NULL;
    WOLFSSL_CTX*    ctx = NULL;
    char            sendLine[MAXLINE];
    char            recvLine[MAXLINE - 1];

    ShowStackInfo("dtls13_smp_client_task startup");
    /* Initialize wolfSSL before assigning ctx */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        ESP_LOGE(TAG, "wolfSSL_CTX_new error.\n");
        goto cleanup;
    }

    /* No-op when debugging is not compiled in */
    wolfSSL_Debugging_ON();

    if ( (ctx = wolfSSL_CTX_new(
#ifndef USE_DTLS12
            wolfDTLSv1_3_client_method()
#else
            wolfDTLSv1_2_client_method()
#endif
            )) == NULL) {
        ESP_LOGE(TAG, "wolfSSL_CTX_new error.\n");
        goto cleanup;
    }

    /* Load certificates into ctx variable */
#ifdef NO_FILESYSTEM
    if (wolfSSL_CTX_load_verify_buffer(ctx,
                                       CTX_CA_CERT, CTX_CA_CERT_SIZE,
                                       CTX_CA_CERT_TYPE) != SSL_SUCCESS) {
        ESP_LOGE(TAG, "Error loading CTX_CA_CERT, please check the file.\n");
        goto cleanup;
    }
#else
    if (wolfSSL_CTX_load_verify_locations(ctx, caCertLoc, 0)
	    != SSL_SUCCESS) {
        ESP_LOGE(TAG, "Error loading %s, please check the file.\n", caCertLoc);
        goto cleanup;
    }
#endif

    /* Assign ssl variable */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        ESP_LOGE(TAG, "unable to get ssl object\n");
        goto cleanup;
    }

    /* servAddr setup */
    ESP_LOGI(TAG, "See ./include/client-dtls13.h to update settings.");
    ESP_LOGI(TAG, "Setting server address to %s, port %d.",
                   TLS_SMP_SERVER_ADDRESS, SERV_PORT);
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, TLS_SMP_SERVER_ADDRESS, &servAddr.sin_addr) < 1) {
        ESP_LOGE(TAG, "inet_pton()");
        goto cleanup;
    }

    if (wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr))
            != WOLFSSL_SUCCESS) {
        ESP_LOGE(TAG, "wolfSSL_dtls_set_peer failed\n");
        goto cleanup;
    }

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
       ESP_LOGE(TAG, "socket()");
       goto cleanup;
    }

    /* Set the file descriptor for ssl */
    if (wolfSSL_set_fd(ssl, sockfd) != WOLFSSL_SUCCESS) {
        ESP_LOGE(TAG, "cannot set socket file descriptor\n");
        goto cleanup;
    }

    /* Perform SSL connection */
    ESP_LOGI(TAG, "Connecting to DTLS 1.3 server...");
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        err = wolfSSL_get_error(ssl, 0);
        ESP_LOGE(TAG, "err = %d, %s\n",
                       err, wolfSSL_ERR_reason_error_string(err));
        ESP_LOGE(TAG, "wolfSSL_connect failed\n");
        goto cleanup;
    }

    showConnInfo(ssl);

/*****************************************************************************/
/*                  Code for sending datagram to server                      */
    while (1) {

        ESP_LOGI(TAG, "Sending message");

        strcpy(sendLine, "Hello World.");

        /* Send sendLine to the server */
        if (wolfSSL_write(ssl, sendLine, strlen(sendLine)) != strlen(sendLine)) {
            err = wolfSSL_get_error(ssl, 0);
            ESP_LOGE(TAG, "err = %d, %s\n",
                           err, wolfSSL_ERR_reason_error_string(err));
            ESP_LOGE(TAG, "wolfSSL_write failed\n");
            goto cleanup;
        }

        ESP_LOGI(TAG, "Reading reply");
        /* n is the # of bytes received */
        n = wolfSSL_read(ssl, recvLine, sizeof(recvLine)-1);

        if (n > 0) {
            /* Add a terminating character to the generic server message */
            recvLine[n] = '\0';
            ESP_LOGI(TAG, "%s\n", recvLine);
        }
        else {
            err = wolfSSL_get_error(ssl, 0);
            ESP_LOGE(TAG, "err = %d, %s\n",
                           err, wolfSSL_ERR_reason_error_string(err));
            ESP_LOGE(TAG, "wolfSSL_read failed\n");
            goto cleanup;
        }
    }
/*                End code for sending datagram to server                    */
/*****************************************************************************/

cleanup:
    if (ssl != NULL) {
        /* Attempt a full shutdown */
        ret = wolfSSL_shutdown(ssl);
        if (ret == WOLFSSL_SHUTDOWN_NOT_DONE)
            ret = wolfSSL_shutdown(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            ESP_LOGE(TAG, "err = %d, %s\n",
                           err, wolfSSL_ERR_reason_error_string(err));
            ESP_LOGE(TAG, "wolfSSL_shutdown failed\n");
        }
        wolfSSL_free(ssl);
    }
    if (sockfd != INVALID_SOCKET)
        close(sockfd);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    vTaskDelete(NULL);

    return TLS_SMP_CLIENT_TASK_RET;
}


#ifndef WOLFSSL_ESPIDF
static void sig_handler(const int sig)
{
    (void)sig;
    free_resources();
    wolfSSL_Cleanup();
}
#endif



#if defined(SINGLE_THREADED)
    /* we don't initialize a thread */
#else
/* create task */
int dtls13_smp_client_init(int port)
{
    int ret = 0;
    int thisPort;
    thisPort = port;
    if (thisPort == 0) {
        thisPort = DEFAULT_PORT;
    }


#if ESP_IDF_VERSION_MAJOR >= 4
    TaskHandle_t _handle;
#else
    xTaskHandle _handle;
#endif

    /* http://esp32.info/docs/esp_idf/html/dd/d3c/group__xTaskCreate.html */
    ESP_LOGI(TAG, "Creating dtls13_smp_client_task with stack size = %d words",
                   TLS_SMP_CLIENT_TASK_WORDS);
    ret = xTaskCreate(dtls13_smp_client_task,
                      TLS_SMP_CLIENT_TASK_NAME,
                      TLS_SMP_CLIENT_TASK_WORDS, /* not bytes! */
                      (void*)&thisPort,
                      TLS_SMP_CLIENT_TASK_PRIORITY,
                      &_handle);

    if (ret == pdPASS) {
        ESP_LOGI(TAG, "Success: create thread %s", TLS_SMP_CLIENT_TASK_NAME);
    }
    else {
        ESP_LOGE(TAG, "create thread %s failed", TLS_SMP_CLIENT_TASK_NAME);
    }

    /* vTaskStartScheduler(); // called automatically in ESP-IDF */
    return ret;
}
#endif
