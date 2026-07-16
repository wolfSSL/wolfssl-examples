/* custom_ca.h
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

#ifndef CUSTOM_CA_H
#define CUSTOM_CA_H

/* Trust anchor for WOLFSSL_MN_USE_CUSTOM_CA=1.
 *
 * CA is pinned at compile time and must be manually updated if the target's chain changes.
 * Hostname verification is enforced using DEFAULT_HOSTNAME in client-tls-mn.c.
 *
 * To configure:
 * 1. Get root CA: openssl s_client -connect <host>:443 -servername <host> -showcerts </dev/null
 * 2. Convert to DER: openssl x509 -in root.pem -outform der | xxd -i
 * 3. Paste bytes below and update DEFAULT_IPADDR/DEFAULT_PORT/DEFAULT_HOSTNAME in client-tls-mn.c.
 */

#define CUSTOM_CA_PLACEHOLDER

#ifdef CUSTOM_CA_PLACEHOLDER
#error "mynewt/custom_ca.h: paste your target's root CA DER bytes into " \
       "custom_ca_der[] and remove this #error before building with " \
       "WOLFSSL_MN_USE_CUSTOM_CA=1 (see the instructions above)."
#endif

static const unsigned char custom_ca_der[] =
{
    /* TODO: paste your target's root CA DER bytes here */
    0
};

static const int sizeof_custom_ca_der = sizeof(custom_ca_der);

#endif /* CUSTOM_CA_H */
