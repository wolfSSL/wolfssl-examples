/* client-tls-cryptocb.c
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

#ifndef COMMON_H
#define COMMON_H

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/port/aria/aria-cryptocb.h>

#define DEFAULT_PORT 11111

#define CA_FILE "../certs/ca-ecc-cert.pem"

#define CLIENT_ECC_FILE    "../certs/intermediate/client-chain-ecc.pem"
#define SERVER_ECC_FILE    "../certs/intermediate/server-chain-ecc.pem"
#define CLIENT_KEY_FILE    "../certs/ecc-client-key.pem"
#define SERVER_KEY_FILE    "../certs/ecc-key.pem"

#define CIPHER_LIST "ECDHE-ECDSA-ARIA128-GCM-SHA256:ECDHE-ECDSA-ARIA256-GCM-SHA384"
//#define CIPHER_LIST "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384"

#endif /* COMMON_H */
