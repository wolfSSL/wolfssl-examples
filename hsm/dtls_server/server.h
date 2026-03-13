/*
 * Copyright (C) 2024 wolfSSL Inc.
 *
 * This file is part of wolfHSM.
 *
 * wolfHSM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfHSM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with wolfHSM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * server.h
 *
 * Header for the TLS/DTLS server demo that uses wolfHSM for crypto operations.
 * This API is protocol-agnostic and can be used for both TLS and DTLS.
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <stdint.h>
#include <stddef.h>
#include <netinet/in.h>

#include "wolfhsm/wh_client.h"

/* Forward declaration for opaque server structure */
typedef struct SERVER_CONTEXT SERVER_CONTEXT;

/* Server configuration */
typedef struct ServerConfig {
    const char* caCertFile;     /* Path to CA certificate file (PEM or DER) */
    const char* serverCertFile; /* Path to server certificate file (optional) */
    const char* serverKeyFile;  /* Path to server private key file (optional) */
    int         port;           /* Port to listen on (default: 11111) */
} ServerConfig;

/* Server management functions */
SERVER_CONTEXT* Server_Get(void);
int Server_Init(SERVER_CONTEXT* ctx, whClientContext* client,
                const ServerConfig* config);
int Server_Accept(SERVER_CONTEXT* ctx);
void Server_Cleanup(SERVER_CONTEXT* ctx);
int Server_Close(SERVER_CONTEXT* ctx);

/* Data transfer functions */
int Server_Read(SERVER_CONTEXT* ctx, unsigned char* data, size_t length);
int Server_Write(SERVER_CONTEXT* ctx, unsigned char* data, size_t length);

#endif /* SERVER_H_ */
