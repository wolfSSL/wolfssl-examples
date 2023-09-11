/* main.c
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* ESP specific */
#include <nvs_flash.h>
#include <esp_log.h>
#include "esp_event.h"

/* wolfSSL  */
#include <wolfssl/wolfcrypt/port/Espressif/esp32-crypt.h>

/* project */
#include "main.h"
#include "time_helper.h"
#include "server-dtls13.h"
static const char* const TAG = "main task";

#define USE_WIFI_EXAMPLE
#ifdef USE_WIFI_EXAMPLE
#include "esp_netif.h"
    #include "protocol_examples_common.h" /* see project CMakeLists.txt */
#else
    #include "wifi_connect.h"
#endif
void app_main(void)
{
    esp_err_t ret;
    ESP_LOGI(TAG, "-------------- wolfSSL DTLS 1.3 Server Example ---------");
    ESP_LOGI(TAG, "--------------------------------------------------------");
    ESP_LOGI(TAG, "--------------------------------------------------------");
    ESP_LOGI(TAG, "---------------------- BEGIN MAIN ----------------------");
    ESP_LOGI(TAG, "--------------------------------------------------------");
    ESP_LOGI(TAG, "--------------------------------------------------------");
#ifdef HAVE_VERSION_EXTENDED_INFO
    esp_ShowExtendedSystemInfo();
#endif

    /* see project CMakeLists.txt for detection of sample code in ESP-IDF */
#ifdef FOUND_PROTOCOL_EXAMPLES_DIR
    ESP_LOGI(TAG, "FOUND_PROTOCOL_EXAMPLES_DIR is active, using example code.");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ret = set_time(); /* need to setup NTP before WiFi */
    ESP_ERROR_CHECK(example_connect());
    ret = set_time_wait_for_ntp();
#else
    ESP_ERROR_CHECK(nvs_flash_init());

    /* Initialize NVS */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize WiFi */
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    ret = wifi_init_sta();
    while (ret != 0) {
        ESP_LOGI(TAG, "Waiting...");
        vTaskDelay(60000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Trying WiFi again...");
        ret = wifi_init_sta();
    }
#endif

    /* set time for cert validation */
    if (ret < -1) {
        /* a value of -1 means there was no NTP server, so no need to wait */
        ESP_LOGI(TAG, "Waiting 10 seconds for NTP to complete." );
        vTaskDelay(10000 / portTICK_PERIOD_MS); /* brute-force solution */
    }

    ESP_LOGI(TAG, "CONFIG_ESP_MAIN_TASK_STACK_SIZE = %d bytes (%d words)",
                   CONFIG_ESP_MAIN_TASK_STACK_SIZE,
                   (int)(CONFIG_ESP_MAIN_TASK_STACK_SIZE / sizeof(void*)));

    /* HWM is maximum amount of stack space that has been unused, in words. */
    ESP_LOGI(TAG, "Initial Stack Used (before wolfSSL Server): %d bytes",
                   CONFIG_ESP_MAIN_TASK_STACK_SIZE
                   - (uxTaskGetStackHighWaterMark(NULL) / 4)
            );
    ESP_LOGI(TAG, "Starting TLS Server...\n");

#if defined(SINGLE_THREADED)
    /* just call the task */
    dtls13_smp_server_task((void*)NULL);
#else
    /* start a thread with the task */
    dtls13_smp_server_init((int)NULL); /* NULL uses the DEFAULT_PORT value */
#endif

#ifdef INCLUDE_uxTaskGetStackHighWaterMark
        ESP_LOGI(TAG, "Stack HWM: %d", uxTaskGetStackHighWaterMark(NULL));

        ESP_LOGI(TAG, "Stack used: %d", CONFIG_ESP_MAIN_TASK_STACK_SIZE
                                        - (uxTaskGetStackHighWaterMark(NULL)));
#endif

    /* done */
    while (1) {
        ESP_LOGV(TAG, "\n\nLoop...\n\n");

#if defined(SINGLE_THREADED)
        ESP_LOGV(TAG, "\n\nDone!\n\n");
        while (1);
#else
        ESP_LOGI(TAG, "\n\nvTaskDelete main...\n\n");
        vTaskDelay(1000);
        vTaskDelete(NULL);

        /* if successful vTaskDelete, we should never get here: */
        ESP_LOGI(TAG, "\n\nvTaskDelete Complete, but failed?...\n\n");

        vTaskDelay(60000);
#endif
    } /* done whle */

} /* app_main */
