/* server-dtls13.h
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
#ifndef CLIENT_TLS
#define CLIENT_TLS

#define DEFAULT_PORT                     11111

#define TLS_SMP_CLIENT_TASK_NAME         "dtls13"
#define TLS_SMP_CLIENT_TASK_WORDS        20240
#define TLS_SMP_CLIENT_TASK_PRIORITY     5

#define TLS_SMP_SERVER_ADDRESS           "192.168.1.125"

#include <wolfssl/wolfcrypt/settings.h>
#include "user_settings.h"

#include <wolfssl/ssl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function to show the ciphers available / in use. */
int ShowCiphers(WOLFSSL* ssl);

/* This is the TLS Server function, possibly in an RTOS thread. */
//WOLFSSL_ESP_TASK udp_server_task(void *pvParameters);

/* init will create an RTOS task, otherwise server is simply function call. */
#if defined(SINGLE_THREADED)
    /* no init neded */
#else
    int dtls13_smp_client_init(int port);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CLIENT_TLS */
