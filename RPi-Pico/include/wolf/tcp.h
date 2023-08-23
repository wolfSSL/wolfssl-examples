/* tcp.c
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
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/ip_addr.h"

#include "wolf/blink.h"
#include "wolf/common.h"

#define DEBUG_printf printf
#define BUF_SIZE (4096*2)

typedef struct {
    struct tcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer[BUF_SIZE];
    int buffer_len;
    int sent_len;
    bool complete;
    int run_count;
    bool connected;
} WOLF_SOCKET_T;

WOLF_SOCKET_T *wolf_TCPsocket(void);
static err_t wolf_TCPfree(WOLF_SOCKET_T *);
bool wolf_TCPconnect(WOLF_SOCKET_T *, const char*, uint32_t);
int wolf_TCPread (WOLF_SOCKET_T *,       unsigned char *, long unsigned int);
int wolf_TCPwrite(WOLF_SOCKET_T *, const unsigned char *, long unsigned int);