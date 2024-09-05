/* wifi.c
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

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "wolf/common.h"

int wolf_wifiConnect(const char *ssid, const char *pw, uint32_t auth, uint32_t timeout)
{
    int retry;
    #define MAX_RETRY 5
    printf("Initializing Wi-Fi... %s, %s, %d\n", ssid, pw, auth);
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wifi\n");
    for(retry = 0; retry < MAX_RETRY; retry++) {
        if (cyw43_arch_wifi_connect_timeout_ms(ssid, pw, auth, timeout)) {
            fprintf(stderr, "failed to connect. Retrying\n");
        } else {
            return WOLF_SUCCESS;
        }
        sleep_ms(1000);
    }
    return WOLF_FAIL;
}

int wolf_wifiDisconnect(void)
{
    cyw43_arch_deinit();
    printf("Wifi disconnected\n");

    return 0;
}

