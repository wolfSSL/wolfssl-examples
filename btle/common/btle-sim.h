/* btle-sim.h
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
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


#define BTLE_MSG_MAX_SIZE   1024
#define BTLE_BLOCK_SIZE     16

#define _GNU_SOURCE
#include <string.h> /* for strnstr */

#define EXIT_STRING "exit"

typedef enum {
    BTLE_PKT_TYPE_NULL,
    BTLE_PKT_TYPE_KEY,
    BTLE_PKT_TYPE_SALT,
    BTLE_PKT_TYPE_MSG,
    BTLE_PKT_TYPE_TLS,
    BTLE_PKT_TYPE_MAX,
} BtlePacket_t;

typedef enum {
    BTLE_ROLE_CLIENT,
    BTLE_ROLE_SERVER,
} BtleRole_t;


int  btle_open(void** dev, int role);
int  btle_send(const unsigned char* buf, int len, int type, void* context);
int  btle_recv(unsigned char* buf, int len, int* type, void* context);
int  btle_recv_ex(unsigned char* buf, int len, int* type, void* context, int non_block);
void btle_close(void* context);
int  btle_msg_pad(unsigned char* buf, int* len, void* context);
