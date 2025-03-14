/* wolfip_compat.h
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

#ifndef WOLFIP_COMPAT_H
#define WOLFIP_COMPAT_H

#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* Define wolfIP structure */
struct wolfIP {
    int dummy;  /* Placeholder to make the structure non-empty */
};

/* Define wolfIP_ipconfig structure */
struct wolfIP_ipconfig {
    uint32_t addr;
    uint32_t netmask;
    uint32_t gateway;
};

/* Define wolfIP_sockaddr structure */
struct wolfIP_sockaddr {
    union {
        struct sockaddr sa;
        struct sockaddr_in sin;
    } addr;
};

/* Define constants for socket API */
#define WOLFIP_AF_INET AF_INET
#define WOLFIP_SOCK_STREAM SOCK_STREAM
#define WOLFIP_SOCK_DGRAM SOCK_DGRAM

/* Define struct for wolfIP_eth_frame to avoid compilation errors */
struct wolfIP_eth_frame {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
};

/* Define struct for wolfIP_ip_packet to avoid compilation errors */
struct wolfIP_ip_packet {
    struct wolfIP_eth_frame eth;
    uint8_t ver_ihl, tos;
    uint16_t len, id, flags_fo;
    uint8_t ttl, proto;
    uint16_t csum;
    uint32_t src, dst;
    uint8_t data[0];
};

/* Define empty function for eth_output_add_header to avoid compilation errors */
static inline int eth_output_add_header(void *s, const uint8_t *dst, 
                                       struct wolfIP_eth_frame *eth, uint16_t type)
{
    (void)s;
    (void)dst;
    (void)eth;
    (void)type;
    return 0;
}

#endif /* WOLFIP_COMPAT_H */
