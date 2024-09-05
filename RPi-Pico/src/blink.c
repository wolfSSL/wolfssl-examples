/* blink.c
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

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#ifdef PICO_CYW43_ARCH_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#define SLEEP_MS vTaskDelay
#else
#define SLEEP_MS sleep_ms
#endif

void blink(int n, int init)
{
    int i;

    if (init)
    {
        stdio_init_all();
        if (cyw43_arch_init_with_country(CYW43_COUNTRY_JAPAN))
        {
            while (1)
            {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                 SLEEP_MS(25);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                 SLEEP_MS(25);
            }
        }
    }
    for (i = 0; i < n; i++)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
         SLEEP_MS(250);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
         SLEEP_MS(250);
    }
}