/* certs.h
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


/* Use the certificate buffers instead of using filesystem. */
#ifndef NO_RSA
    #define USE_CERT_BUFFERS_2048
    #define SERVER_CERT        server_cert_der_2048
    #define SERVER_CERT_LEN    sizeof_server_cert_der_2048
    #define SERVER_KEY         server_key_der_2048
    #define SERVER_KEY_LEN     sizeof_server_key_der_2048
    #define CA_CERTS           ca_cert_der_2048
    #define CA_CERTS_LEN       sizeof_ca_cert_der_2048
    #define CLIENT_CERT        client_cert_der_2048
    #define CLIENT_CERT_LEN    sizeof_client_cert_der_2048
    #define CLIENT_KEY         client_key_der_2048
    #define CLIENT_KEY_LEN     sizeof_client_key_der_2048
#elif defined(HAVE_ECC)
    #define USE_CERT_BUFFERS_256
    #define SERVER_CERT        serv_ecc_der_256
    #define SERVER_CERT_LEN    sizeof_serv_ecc_der_256
    #define SERVER_KEY         ecc_key_der_256
    #define SERVER_KEY_LEN     sizeof_ecc_key_der_256
    #define CA_CERTS           ca_ecc_cert_der_256
    #define CA_CERTS_LEN       sizeof_ca_ecc_cert_der_256
    #define CLIENT_CERT        cliecc_cert_der_256
    #define CLIENT_CERT_LEN    sizeof_cliecc_cert_der_256
    #define CLIENT_KEY         ecc_clikey_der_256
    #define CLIENT_KEY_LEN     sizeof_ecc_clikey_der_256
#elif defined(HAVE_ED25519)
    #define SERVER_CERT        server_ed25519_cert
    #define SERVER_CERT_LEN    sizeof_server_ed25519_cert
    #define SERVER_KEY         server_ed25519_key
    #define SERVER_KEY_LEN     sizeof_server_ed25519_key
    #define CA_CERTS           ca_ed25519_cert
    #define CA_CERTS_LEN       sizeof_ca_ed25519_cert
    #define CLIENT_CERT        client_ed25519_cert
    #define CLIENT_CERT_LEN    sizeof_client_ed25519_cert
    #define CLIENT_KEY         client_ed25519_key
    #define CLIENT_KEY_LEN     sizeof_client_ed25519_key
#endif

#include <wolfssl/certs_test.h>

