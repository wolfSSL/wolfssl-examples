/*
 * dtls-common.h
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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
 *
 * -----------------------------------------------------------------------------
 *
 * Define USE_DTLS12 to use DTLS 1.2 instead of DTLS 1.3
 *
 */

#ifndef DTLS_COMMON_H_
#define DTLS_COMMON_H_

#define INVALID_SOCKET -1
#define MAXLINE   4096
#define SERV_PORT 11111
#define LOOP_LIMIT 5
#define SFD_TIMEOUT 1

/* Loc short for "location" */
const char caCertLoc[] = "../certs/ca-cert.pem";
const char servCertLoc[] = "../certs/server-cert.pem";
const char servKeyLoc[] = "../certs/server-key.pem";

static inline void showConnInfo(WOLFSSL* ssl) {
    printf("New connection established using %s %s\n",
            wolfSSL_get_version(ssl), wolfSSL_get_cipher(ssl));
}


#endif /* DTLS_COMMON_H_ */
