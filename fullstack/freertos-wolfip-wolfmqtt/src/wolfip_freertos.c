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
#include "../../../wolfip/wolfip.h"

/* External functions from linux_tap.c */
extern int tap_init(struct ll *ll, const char *ifname, uint32_t host_ip);

static struct wolfIP *g_ipstack = NULL;

int wolfip_init(void)
{
    struct wolfIP *ipstack;
    ip4 ip, netmask, gateway;
    struct ll *dev;
    int ret;

    /* Initialize wolfIP */
    wolfIP_init_static(&ipstack);
    if (!ipstack) {
        printf("Failed to initialize wolfIP\n");
        return -1;
    }

    /* Configure IP settings */
    ip = htonl(0x0A0A000A);      /* 10.10.0.10 */
    netmask = htonl(0xFFFFFF00); /* 255.255.255.0 */
    gateway = htonl(0x0A0A0001); /* 10.10.0.1 */

    /* Get device interface */
    dev = wolfIP_getdev(ipstack);
    if (!dev) {
        printf("Failed to get device interface\n");
        return -1;
    }

    /* Initialize TAP device */
    ret = tap_init(dev, "wtap0", ip);
    if (ret < 0) {
        printf("Failed to initialize TAP device\n");
        return -1;
    }

    /* Set IP configuration */
    wolfIP_ipconfig_set(ipstack, ip, netmask, gateway);

    /* Print IP configuration */
    wolfIP_ipconfig_get(ipstack, &ip, &netmask, &gateway);
    printf("IP Configuration:\n");
    printf("IP: %d.%d.%d.%d\n",
           (ntohl(ip) >> 24) & 0xFF,
           (ntohl(ip) >> 16) & 0xFF,
           (ntohl(ip) >> 8) & 0xFF,
           ntohl(ip) & 0xFF);
    printf("Netmask: %d.%d.%d.%d\n",
           (ntohl(netmask) >> 24) & 0xFF,
           (ntohl(netmask) >> 16) & 0xFF,
           (ntohl(netmask) >> 8) & 0xFF,
           ntohl(netmask) & 0xFF);
    printf("Gateway: %d.%d.%d.%d\n",
           (ntohl(gateway) >> 24) & 0xFF,
           (ntohl(gateway) >> 16) & 0xFF,
           (ntohl(gateway) >> 8) & 0xFF,
           ntohl(gateway) & 0xFF);

    g_ipstack = ipstack;
    return 0;
}

struct wolfIP* wolfip_get_stack(void)
{
    return g_ipstack;
}
