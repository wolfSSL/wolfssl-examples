/* server-dtls13.c
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
#include "server-dtls13.h"

#if defined(SINGLE_THREADED)
    #define WOLFSSL_ESP_TASK int
#else
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <freertos/event_groups.h>
    #define WOLFSSL_ESP_TASK static void
#endif

/* Espressif socket */
#include <esp_log.h>
#include <esp_netif.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
/* #include <signal.h> not fully implemented in ESP-IDF */

/* wolfSSL */
#include <wolfssl/wolfcrypt/settings.h>
#include "user_settings.h" /* include before other wolfssl files */

#include <wolfssl/ssl.h>
#include <errno.h>

#if defined(WOLFSSL_SM2) || defined(WOLFSSL_SM3) || defined(WOLFSSL_SM4)
    #include <wolfssl/certs_test_sm.h>
#else
    #include <wolfssl/certs_test.h>
#endif
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

static void free_resources(void);

/* show stack space for this task */
static int ShowStackInfo(char* msg)
{
    int ret = 0;
#ifdef INCLUDE_uxTaskGetStackHighWaterMark
    ret = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "%s used: %d of %d words. %d free.", msg,
                   TLS_SMP_SERVER_TASK_WORDS - ret,
                   TLS_SMP_SERVER_TASK_WORDS,
                   ret);
#else
    ESP_LOGW(TAG, "Warning: uxTaskGetStackHighWaterMark() "
                  "not available");
#endif
    return ret;
}

#ifdef DTSL13_SERVER_IS_MAIN
int main(int argc, char** argv)
#else
WOLFSSL_ESP_TASK dtls13_smp_server_task(void *pvParameters)
#endif
{
    /* Loc short for "location" */
#if defined(SINGLE_THREADED)
    #define TLS_SMP_SERVER_TASK_RET exitVal
#else
    #define TLS_SMP_SERVER_TASK_RET
#endif
    char          buff[MAXLINE];   /* the incoming message */
    char          ack[] = "I hear you fashizzle!\n";
    struct sockaddr_in servAddr = { 0 };  /* our server's address */
    struct sockaddr_in cliaddr  = { 0 };  /* the client's address */
    socklen_t     cliLen;
    int           err;
    int           recvLen = 0;    /* length of message */
    int           ip_protocol = 0;
    int           ret;

    ESP_LOGI(TAG, "Init Stack: %d words", TLS_SMP_SERVER_TASK_WORDS);
    ShowStackInfo("Begin Stack");

    /* Initialize wolfSSL before assigning ctx */
    ret = wolfSSL_Init();
    if (ret == WOLFSSL_SUCCESS) {
        ESP_LOGI(TAG, "wolfSSL_Init success.");
    }
    else {
        ESP_LOGE(TAG, "wolfSSL_Init error %d.\n", ret);
        return TLS_SMP_SERVER_TASK_RET;
    }

    /* Create new conext ctx & show some diagnostics */
    if (ret == WOLFSSL_SUCCESS) {
        ShowStackInfo("Init Stack");

        /* No-op when debugging is not compiled in */
        wolfSSL_Debugging_ON();

        /* Set ctx to DTLS 1.3 unless DTLS1.2 explicitly enabled */
    #ifndef USE_DTLS12
        ESP_LOGI(TAG, "wolfSSL_CTX_new(wolfDTLSv1_3_server_method())");
        ctx = wolfSSL_CTX_new(wolfDTLSv1_3_server_method());
    #else
        ESP_LOGI(TAG, "wolfSSL_CTX_new(wolfDTLSv1_2_server_method())");
        ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method());
    #endif
        if(ctx == NULL) {
            ESP_LOGE(TAG, "wolfSSL_CTX_new error.\n");
            ret = WOLFSSL_FATAL_ERROR;
        }

        (void)ctx;
        ShowStackInfo("Init ctx Stack");
    } /* new ctx */

#if defined(WOLFSSL_SM2) || defined(WOLFSSL_SM3) || defined(WOLFSSL_SM4)
    #define WOLFSSL_ESP32_CIPHER_SUITE "TLS13-SM4-GCM-SM3:" \
                                       "TLS13-SM4-CCM-SM3:"
    ret = wolfSSL_CTX_set_cipher_list(ctx, WOLFSSL_ESP32_CIPHER_SUITE);
    if (ret == WOLFSSL_SUCCESS) {
        printf("Set cipher list: %s\n", WOLFSSL_ESP32_CIPHER_SUITE);
    }
    else {
       printf("ERROR: failed to set cipher list: %s\n", WOLFSSL_ESP32_CIPHER_SUITE);
    }
#endif

#ifdef NO_FILESYSTEM
    /* Load CA certificates */
    if (ret == WOLFSSL_SUCCESS) {
        /* caCertLoc[] = "../certs/ca-cert.pem"; */
        ret = wolfSSL_CTX_load_verify_buffer(ctx,
                       CTX_CA_CERT,
                       CTX_CA_CERT_SIZE,
                       CTX_SERVER_CERT_TYPE);
    }

    /* if successful, Load server certificates */
    if (ret == WOLFSSL_SUCCESS) {
        ESP_LOGI(TAG, "Success: wolfSSL_CTX_load_verify_buffer loaded %s",
                       STRINGIFY(CTX_CA_CERT));

        /* servCertLoc[] = "../certs/server-cert.pem"; */
        ret = wolfSSL_CTX_use_certificate_buffer(ctx,
                       CTX_SERVER_CERT,
                       CTX_SERVER_CERT_SIZE,
                       CTX_SERVER_CERT_TYPE);
    }
    else {
        ESP_LOGE(TAG, "Failed CA wolfSSL_CTX_load_verify_buffer "
                      "loading CA %s", STRINGIFY(CTX_SERVER_CERT));
    }

    /* if successful Load server Keys */
    if (ret == WOLFSSL_SUCCESS) {
        ESP_LOGI(TAG, "Success: "
                      "wolfSSL_CTX_use_certificate_chain_buffer_format "
                      "loaded cert chain %s", STRINGIFY(CTX_SERVER_CERT));
        /* servKeyLoc[] = "../certs/server-key.pem"; */
        ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx,
                      CTX_SERVER_KEY,
                      CTX_SERVER_KEY_SIZE,
                      CTX_SERVER_KEY_TYPE);
    }
    else {
        ESP_LOGE(TAG, "Failed wolfSSL_CTX_use_certificate_chain_buffer_format "
                      "loading private key %s", STRINGIFY(CTX_SERVER_KEY));
    }


    if (ret == WOLFSSL_SUCCESS) {
        ESP_LOGI(TAG, "Success: "
                      "wolfSSL_CTX_use_PrivateKey_buffer "
                      "loaded private key %s", STRINGIFY(CTX_SERVER_KEY));
    }
    else {
        ESP_LOGE(TAG, "Failed to load private key: %s",
                      STRINGIFY(CTX_SERVER_KEY));
    }
#else
    /* Load CA certificates */
    if (wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0) !=
            SSL_SUCCESS) {
        ESP_LOGE(TAG, "Error loading %s, please check the file.\n", caCertLoc);
        goto cleanup;
    }
    /* Load server certificates */
    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) !=
                                                                 SSL_SUCCESS) {
        ESP_LOGE(TAG, "Error loading %s, please check the file.\n", servCertLoc);
        goto cleanup;
    }
    /* Load server Keys */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc,
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        ESP_LOGE(TAG, "Error loading %s, please check the file.\n", servKeyLoc);
        goto cleanup;
    }
#endif

    /* initialize network vars */
    if (ret == WOLFSSL_SUCCESS) {
        memset((char *)&servAddr, 0, sizeof(servAddr));
        /* host-to-network-long conversion (htonl) */
        /* host-to-network-short conversion (htons) */
        servAddr.sin_family      = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servAddr.sin_port        = htons(SERV_PORT);
        ip_protocol = IPPROTO_IP;

        /* Create a UDP/IP socket */
        listenfd = socket(AF_INET, SOCK_DGRAM, ip_protocol);
        if (listenfd < 0) {
            ESP_LOGE(TAG, "socket() failed: %d", listenfd);
            ret = WOLFSSL_FATAL_ERROR;
        }
        else {
            ESP_LOGI(TAG, "Socket allocated.");
        }
    } /* init network vars */

#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
    int enable = 1;
    lwip_setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable));
#endif

#ifdef USE_SOCKET_TIMEOUT
    /* init socket options */
    if (ret == WOLFSSL_SUCCESS) {
        ESP_LOGI(TAG, "setsockopt timeout ");
        struct timeval timeout;
        timeout.tv_sec = 1000; // 10 seconds
        timeout.tv_usec = 0;

        ESP_LOGI(TAG, "setsockopt timeout %d seconds", (int)timeout.tv_sec);

        if (setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
             ESP_LOGE(TAG, "setsockopt for receive timeout");
             ret = WOLFSSL_FATAL_ERROR;
        }
        if (setsockopt(listenfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
            ESP_LOGE(TAG, "setsockopt for send timeout");
            ret = WOLFSSL_FATAL_ERROR;
        }
    } /* soctet options */
#endif

    /* Bind Socket */
    if (ret == WOLFSSL_SUCCESS) {
        if (bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
            ESP_LOGE(TAG, "bind()");
            ret = WOLFSSL_FATAL_ERROR;
        }
    } /* bind socket */

#ifndef WOLFSSL_ESPIDF
    /* Signals and signal handling are not implemented in esp-idf.
     * Calling raise() will abort the program.
     * see https://esp32.com/viewtopic.php?t=29988&p=103871 */
    signal(SIGINT, sig_handler);
#endif

    ESP_LOGI(TAG, "While Stack used: %d words", TLS_SMP_SERVER_TASK_WORDS
                                        - (uxTaskGetStackHighWaterMark(NULL)));
    ShowStackInfo("While Stack");
    while (1) {
        ESP_LOGI(TAG, "\n\nAwaiting client connection on port %d\n", SERV_PORT);

        cliLen = sizeof(cliaddr);
        ret = (int)recvfrom(listenfd,
                            (char *)&buff, sizeof(buff),
                            MSG_PEEK,
                            (struct sockaddr*)&cliaddr, &cliLen
                           );

        if (ret < 0) {
            ESP_LOGE(TAG, "ERROR during recvfrom()");
            goto cleanup;
        }
        else if (ret == 0) {
            ESP_LOGE(TAG, "recvfrom zero return\n");
            goto cleanup;
        }

        /* Create the WOLFSSL Object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            ESP_LOGE(TAG, "wolfSSL_new error.\n");
            goto cleanup;
        }

        if (wolfSSL_dtls_set_peer(ssl, &cliaddr, cliLen) != WOLFSSL_SUCCESS) {
            ESP_LOGE(TAG, "wolfSSL_dtls_set_peer error.\n");
            goto cleanup;
        }

    #if defined(WOLFSSL_SM2)
        /* SM TLS1.3 Cipher needs to have key share explicitly set. */
        ESP_LOGI(TAG, "Setting WOLFSSL_ECC_SM2P256V1");
        ret = wolfSSL_UseKeyShare(ssl, WOLFSSL_ECC_SM2P256V1);
        if (ret == WOLFSSL_SUCCESS) {
            ESP_LOGI(TAG, "Successfully set WOLFSSL_ECC_SM2P256V1");
        }
        else {
            ESP_LOGE(TAG, "FAILED to set WOLFSSL_ECC_SM2P256V1");
        }
    #endif

        ESP_LOGI(TAG, "Listen...");
        if (wolfSSL_set_fd(ssl, listenfd) != WOLFSSL_SUCCESS) {
            ESP_LOGE(TAG, "wolfSSL_set_fd error.\n");
            break;
        }

        ESP_LOGI(TAG, "Accept..");
        if (wolfSSL_accept(ssl) != SSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            ESP_LOGE(TAG, "error = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
            ESP_LOGE(TAG, "SSL_accept failed.\n");
            goto cleanup;
        }
        showConnInfo(ssl);
        while (1) {
            if ((recvLen = wolfSSL_read(ssl, buff, sizeof(buff)-1)) > 0) {
                ESP_LOGI(TAG, "wolfSSL_read heard %d bytes:\n\n"
                              "%s", recvLen, buff);

                buff[recvLen] = '\0';
            }
            else if (recvLen <= 0) {
                err = wolfSSL_get_error(ssl, 0);
                if (err == WOLFSSL_ERROR_ZERO_RETURN) { /* Received shutdown */
                    break;
                }
                ESP_LOGE(TAG, "error = %d, %s\n",
                               err, wolfSSL_ERR_reason_error_string(err));
                ESP_LOGE(TAG, "SSL_read failed.\n");
                goto cleanup;
            }
            ESP_LOGI(TAG, "Sending reply (check client for this text): %s",
                           ack);
            if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
                err = wolfSSL_get_error(ssl, 0);
                ESP_LOGE(TAG, "error = %d, %s\n",
                               err, wolfSSL_ERR_reason_error_string(err));
                ESP_LOGE(TAG, "wolfSSL_write failed.\n");
                goto cleanup;
            }
            ESP_LOGI(TAG, "Sending complete. Waiting for next message...");
        }

        ESP_LOGI(TAG, "reply sent \"%s\"\n", ack);

        /* Attempt a full shutdown */
        ret = wolfSSL_shutdown(ssl);
        if (ret == WOLFSSL_SHUTDOWN_NOT_DONE) {
            ESP_LOGW(TAG, "WARNING: wolfSSL_shutdown not done the first "
                          "time. Trying again...");
            ret = wolfSSL_shutdown(ssl);
        }
        if (ret != WOLFSSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            ESP_LOGE(TAG, "err = %d, %s\n",
                           err, wolfSSL_ERR_reason_error_string(err));
            ESP_LOGE(TAG, "wolfSSL_shutdown failed\n");
        }
        wolfSSL_free(ssl);
        ssl = NULL;

        ESP_LOGI(TAG, "Awaiting new connection\n");
    }

cleanup:
    free_resources();
    wolfSSL_Cleanup();

#if defined(SINGLE_THREADED)
        ESP_LOGV(TAG, "\n\nDone!\n\n");
        return TLS_SMP_SERVER_TASK_RET;
#else
        ESP_LOGI(TAG, "\n\nvTaskDelete dtls13_smp_server_task...\n\n");
        vTaskDelay(1000);
        vTaskDelete(NULL);

        /* if successful vTaskDelete, we should never get here: */
        ESP_LOGI(TAG, "\n\nvTaskDelete Complete, but failed?...\n\n");

        vTaskDelay(60000);
#endif

    return TLS_SMP_SERVER_TASK_RET;
}

#ifndef WOLFSSL_ESPIDF
static void sig_handler(const int sig)
{
    (void)sig;
    free_resources();
    wolfSSL_Cleanup();
}
#endif

static void free_resources(void)
{
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
        ssl = NULL;
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
        ctx = NULL;
    }
    if (listenfd != INVALID_SOCKET) {
        close(listenfd);
        listenfd = INVALID_SOCKET;
    }
}

#if defined(SINGLE_THREADED)
    /* we don't initialize a thread */
#else
/* create task */
int dtls13_smp_server_init(int port)
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

    /* see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_idf.html */
    ESP_LOGI(TAG, "Creating dtls13_smp_server_task with stack size = %d words",
                   TLS_SMP_SERVER_TASK_WORDS);
    ret = xTaskCreate(dtls13_smp_server_task,
                      TLS_SMP_SERVER_TASK_NAME,
                      TLS_SMP_SERVER_TASK_WORDS, /* not bytes! */
                      (void*)&thisPort,
                      TLS_SMP_SERVER_TASK_PRIORITY,
                      &_handle);

    if (ret == pdPASS) {
        ESP_LOGI(TAG, "Success: create thread %s", TLS_SMP_SERVER_TASK_NAME);
    }
    else {
        ESP_LOGE(TAG, "create thread %s failed", TLS_SMP_SERVER_TASK_NAME);
    }

    /* vTaskStartScheduler(); note needed; called automatically in ESP-IDF */
    return ret;
}
#endif
