/*
 * Copyright (C) 2021 wolfSSL Inc.
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
 *
 * This file is subject to the terms and conditions of the GNU 
 * General Public License v2. See the file LICENSE in the top level
 * directory for more details.
 */


/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example wolfSSL application using Posix sockets 
 *
 * @author      Daniele Lacamera <daniele@wolfssl.com>, Kaleb J. Himes <kaleb@wolfssl.com>
 *
 * @}
 */

#include <stdio.h>

#include "lwip.h"
#include "lwip/opt.h"
#include "lwip/netif.h"
#include "net/ipv6/addr.h"
#include "shell.h"
#include "msg.h"

#include "net/sock/tcp.h"
#include "xtimer.h"
#include "lwip/sockets.h"


static int ifconfig(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    for (struct netif *iface = netif_list; iface != NULL; iface = iface->next) {
        printf("%s_%02u: ", iface->name, iface->num);
#ifdef MODULE_LWIP_IPV6
        char addrstr[IPV6_ADDR_MAX_STR_LEN];
        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            if (!ipv6_addr_is_unspecified((ipv6_addr_t *)&iface->ip6_addr[i])) {
                printf(" inet6 %s\n", ipv6_addr_to_str(addrstr, (ipv6_addr_t *)&iface->ip6_addr[i],
                                                       sizeof(addrstr)));
            }
        }
#endif
        puts("");
    }
    return 0;
}


#ifndef MODULE_POSIX_SOCKETS
#   error RIOT-OS lacks support for posix sockets, and this TLS app is configured to use them. Please ensure that MODULE_POSIX_SOCKETS is enabled in your configuration.
#endif


#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int tls_client(int argc, char **argv);
extern int tls_server(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "tlsc", "Start a TLS client", tls_client },
    { "tlss", "Start and stop a TLS server", tls_server },
    { "ifconfig", "Shows assigned IP addresses", ifconfig },
    { NULL, NULL, NULL }
};

static void add_site_local_address(struct netif *iface)
{
    (void)iface;
#ifdef MODULE_LWIP_IPV6
    #define SITE_LOCAL_PREFIX 0xBBAAC0FE
    ip6_addr_t sl_addr;
    memcpy(&sl_addr, &iface->ip6_addr[0], sizeof(ip6_addr_t));
    sl_addr.addr[0] = SITE_LOCAL_PREFIX;
    netif_add_ip6_address(iface, &sl_addr, NULL);
#endif
}

static void manual_eth_address(struct netif *iface)
{
    ip4_addr_t ipaddr, netmask, gw;
    IP4_ADDR(&ipaddr, 192, 168, 7, 2);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 7, 1);
    netif_set_addr(iface, &ipaddr, &netmask, &gw);
}

int main(void)
{
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT TLS testing implementation");

    /* Initialize TCP/IP stack */
    xtimer_init();
    lwip_bootstrap();

    for (struct netif *iface = netif_list; iface != NULL; iface = iface->next) {
        if (strncmp(iface->name, "lo", 2) != 0)
            /* Add site-local address */
            add_site_local_address(iface);
        if (strncmp(iface->name, "ET", 2) == 0)
            manual_eth_address(iface);
            /* IPv4: eth address */
    }

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
