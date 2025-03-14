/* wolfip_freertos.c
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
#include <arpa/inet.h>
#include "wolfip_freertos.h"
#include "../../../../../../wolfip/wolfip.h"

/* External functions */
extern int tap_init(const char *ifname, const char *ipaddr, const char *netmask);

/* Static wolfIP instance pointer */
static struct wolfIP *g_ipstack;

int wolfip_init(void)
{
    int ret;
    uint32_t ip, mask, gw;

    /* Initialize wolfIP */
    wolfIP_init_static(&g_ipstack);

    /* Initialize TAP device */
    ret = tap_init("tap0", "10.10.0.10", "255.255.255.0");
    if (ret < 0) {
        printf("Failed to initialize TAP device\n");
        return -1;
    }

    /* Configure IP settings */
    ip = inet_addr("10.10.0.10");
    mask = inet_addr("255.255.255.0");
    gw = inet_addr("10.10.0.1");

    /* Set IP configuration */
    wolfIP_ipconfig_set(g_ipstack, ip, mask, gw);

    /* Print IP configuration */
    printf("IP Configuration:\n");
    printf("IP: 10.10.0.10\n");
    printf("Netmask: 255.255.255.0\n");
    printf("Gateway: 10.10.0.1\n");

    return 0;
}

struct wolfIP* wolfip_get_stack(void)
{
    return g_ipstack;
}
