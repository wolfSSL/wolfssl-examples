/* time_helper.c
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

/*
# This tag is used to include this file in the ESP Component Registry:
# __ESP_COMPONENT_SOURCE__
*/

#include <string.h>
#include <lwip/apps/sntp.h>
#include <esp_netif_sntp.h>
#include "sdkconfig.h"
#include "esp_log.h"

#include "time_helper.h"

const static char* TAG = "time_helper";

#define TIME_ZONE "PST-8"
/* NELEMS(x) number of elements
 * To determine the number of elements in the array, we can divide the total size of
 * the array by the size of the array element
 * See https://stackoverflow.com/questions/37538/how-do-i-determine-the-size-of-my-array-in-c
 **/
#define NELEMS(x)  ( (int)(sizeof(x) / sizeof((x)[0])) )
#define NTP_SERVER_LIST ( (char*[]) {                        \
                                     "pool.ntp.org",         \
                                     "time.nist.gov",        \
                                     "utcnist.colorado.edu"  \
                                     }                       \
                        )
/* #define NTP_SERVER_COUNT using NELEMS:
 *
 *  (int)(sizeof(NTP_SERVER_LIST) / sizeof(NTP_SERVER_LIST[0]))
 */
#define USE_NTP
#define NTP_SERVER_COUNT NELEMS(NTP_SERVER_LIST)
char* ntpServerList[NTP_SERVER_COUNT] = NTP_SERVER_LIST;

/* our NTP server list is global info */
extern char* ntpServerList[NTP_SERVER_COUNT];

/* the worst-case scenario is a hard-coded date/time */
int set_fixed_default_time()
{
    time_t interim_time;

    /* ideally, we'd like to set time from network,
     * but let's set a default time, just in case */
    struct tm timeinfo = {
        .tm_year = 2023 - 1900,
        .tm_mon = 7,
        .tm_mday = 18,
        .tm_hour = 9,
        .tm_min = 49,
        .tm_sec = 0
    };
    struct timeval now;
    /* set interim static time */
    interim_time = mktime(&timeinfo);
    now = (struct timeval){ .tv_sec = interim_time };
    settimeofday(&now, NULL);

    return 0;
}

/* set_time_from_string
 *
 * returns 0 = success if able to set the time from the provided string
 * error for any other value, typically -1 */
int set_time_from_string(char* time_buffer)
{
    /* expecting github default formatting: 'Thu Aug 31 12:41:45 2023 -0700' */
    const char *format = "%3s %3s %d %d:%d:%d %d %s";
    struct tm this_timeinfo;
    struct timeval now;
    time_t interim_time;
    char day_str[4];
    char month_str[4];
    char offset[6]; /* expecting trailing single quote, not used */
    int day, year, hour, minute, second;
    int quote_offset = 0;
    int ret = 0;

    /* we are expecting the string to be encapsulated in single quotes */
    if (*time_buffer == 0x27) {
        quote_offset = 1;
    }

    ret = sscanf(time_buffer + quote_offset,
                format,
                day_str, month_str,
                &day, &hour, &minute, &second, &year, &offset);

    if (ret == 8) {
        /* we found a match for all componets */

        const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

        for (int i = 0; i < 12; i++) {
            if (strcmp(month_str, months[i]) == 0) {
                this_timeinfo.tm_mon = i;
                break;
            }
        }

        this_timeinfo.tm_mday = day;
        this_timeinfo.tm_hour = hour;
        this_timeinfo.tm_min = minute;
        this_timeinfo.tm_sec = second;
        this_timeinfo.tm_year = year - 1900; /* Number of years since 1900 */

        interim_time = mktime(&this_timeinfo);
        now = (struct timeval){ .tv_sec = interim_time };
        settimeofday(&now, NULL);
        ESP_LOGI(TAG, "Time updated to %s", time_buffer);
        ret = 0; /* success */
    }
    else {
        ret = -1;
        ESP_LOGE(TAG, "Failed to convert \"%s\" to a tm date.", time_buffer);
        ESP_LOGI(TAG, "Trying fixed date that was hard-coded.");
        set_fixed_default_time();
    }
    return ret;
}

/* set time; returns 0 if succecssfully confirmed NTP update */
int set_time(void)
{
    /* we'll also return a result code of zero */
    int res = 0;
    int i = 0; /* counter for time servers */

#ifdef LIBWOLFSSL_VERSION_GIT_HASH_DATE
    /* initialy set a default approximate time from recent git commit */
    ESP_LOGI(TAG, "Found git hash date, attempting to set system date.");
    set_time_from_string(LIBWOLFSSL_VERSION_GIT_HASH_DATE);

//    return 0;

    res = -4;
#else
    /* otherwise set a fixed time that was hard coded */
    set_fixed_default_time();
    restrict = -3;
#endif

#ifndef NTP_SERVER_COUNT
    ESP_LOGW(TAG, "WArning: no sntp server names defined. Setting to empty list");
    #define NTP_SERVER_COUNT 0
    char* ntpServerList[NTP_SERVER_COUNT];
#endif /* not defined: NTP_SERVER_COUNT */

#ifndef TIME_ZONE
    #define TIME_ZONE "PST-8"
#endif /* not defined: TIME_ZONE */

    /* set timezone */
    setenv("TZ", TIME_ZONE, 1);
    tzset();

    if (NTP_SERVER_COUNT) {
        /* next, let's setup NTP time servers
         *
         * see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html#sntp-time-synchronization
         */
        sntp_setoperatingmode(SNTP_OPMODE_POLL);

        ESP_LOGI(TAG, "sntp_setservername:");
        for (i = 0; i < NTP_SERVER_COUNT; i++) {
            const char* thisServer = ntpServerList[i];
            if (strncmp(thisServer, "\x00", 1) == 0) {
                /* just in case we run out of NTP servers */
                break;
            }
            ESP_LOGI(TAG, "%s", thisServer);
            sntp_setservername(i, thisServer);
        }
        sntp_init();
        if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(200000)) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to update system time within 10s timeout using NTP.");
            res = -2;
        }
        ESP_LOGI(TAG, "sntp_init done.");
    }
    else {
        ESP_LOGW(TAG, "No sntp time servers found.");
        res = -1;
    }
    return res;
}

