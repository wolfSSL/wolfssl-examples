/* tcp.c
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/ip4_addr.h"

#include "lwip/init.h"
#include "lwip/sockets.h"

#include "FreeRTOS.h"
#include "task.h"

static int tcpip_initialized = 0;

static void my_tcpip_init_done(void *arg)
{
    tcpip_initialized = 1;
    printf("\ntcpip_thread initialized!\n");
}

void print_netinfo(void)
{
    struct netif *netif = netif_list;
    if (netif != NULL) {
        ip4_addr_t ip      = netif->ip_addr;
        ip4_addr_t netmask = netif->netmask;
        ip4_addr_t gw      = netif->gw;

        printf("IP Addr: %s\n", ip4addr_ntoa(&ip));
        printf("Netmask: %s\n", ip4addr_ntoa(&netmask));
        printf("Gateway: %s\n", ip4addr_ntoa(&gw));
    }
    else {
        printf("Network interface not found.\n");
    }
}

void tcp_initThread(void)
{
    static struct netif netif;
    static ip4_addr_t ipaddr, netmask, gw;

    tcpip_init(my_tcpip_init_done, NULL);

    while (tcpip_initialized == 0) {
        printf(".");
        vTaskDelay(1000); /* need to wait for initializing TCPIP */
    }

    IP4_ADDR(&ipaddr, 192, 168, 10, 79);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 10, 1);
    
    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, NULL, tcpip_input);
    netif_set_default(&netif);
    netif_set_up(&netif);

    print_netinfo();
}
