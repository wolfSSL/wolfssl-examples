/* common.h
 *
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef  __CANCOMMON_H__
#define __CANCOMMON_H__

#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define CAN_MSG_LEN 8
#define ERR_MSG_LEN 80

#define SERVER_ID 0x7
#define CLIENT_ID 0x8

enum service_type {
    SERVICE_TYPE_CLIENT,
    SERVICE_TYPE_SERVER
};

int can_receive(struct isotp_can_data *data, void *arg, int timeout);
int can_send(struct isotp_can_data *data, void *arg);
int can_connect(const char *address, uint16_t filter);
void can_close(void);

void close_ssl(WOLFSSL_CTX *ctx, WOLFSSL *ssl);
int setup_connection(const char *interface, int local_id, int remote_id);
int setup_ssl(enum service_type type, WOLFSSL_CTX **new_ctx,
        WOLFSSL_METHOD **new_method, WOLFSSL **new_ssl);
#endif /* __CANCOMMON_H__ */
