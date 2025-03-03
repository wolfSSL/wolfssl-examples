/* https_server.h
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

#ifndef HTTPS_SERVER_H
#define HTTPS_SERVER_H

#include "FreeRTOS.h"
#include "task.h"
#include "wolfip.h"
#include "httpd.h"
#include <wolfssl/ssl.h>

/* HTTPS server configuration */
#define HTTPS_PORT 443
#define HTTPS_TASK_STACK_SIZE (16 * 1024)
#define HTTPS_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

/* Certificate paths */
#define CERT_FILE "../../../../wolfssl/certs/server-cert.pem"
#define KEY_FILE  "../../../../wolfssl/certs/server-key.pem"
#define CA_FILE   "../../../../wolfssl/certs/ca-cert.pem"

/* Initialize HTTPS server with wolfSSL and wolfIP */
int https_server_init(struct wolfIP *ipstack);

/* Start HTTPS server task */
int https_server_start(void);

#endif /* HTTPS_SERVER_H */
