/* client-tls-mbedtls.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

/* Mbed TLS 4.x client built on wolfSSL through wolfPSA's Mbed TLS
 * compatibility layer. tls_get() is the client from the Mbed TLS migration
 * guide, plus retrying partial writes, poll() before non-blocking retries, an
 * error when the server closes without a reply, and printing the protocol,
 * cipher suite and certificate verify result. */

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* socket includes */
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

/* Mbed TLS, provided by wolfPSA */
#include <psa/crypto.h>
#include <mbedtls/ssl.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/x509_crt.h>

#define DEFAULT_PORT "11111"

#define CA_FILE   "../certs/ca-cert.pem"

/* Non-blocking I/O: wait for the socket, then call again with the same
 * arguments */
static int tls_want(int fd, int ret)
{
    struct pollfd pfd;

    if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
        ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        return 0;
    }
    pfd.fd = fd;
    pfd.events = (ret == MBEDTLS_ERR_SSL_WANT_READ) ? POLLIN : POLLOUT;
    pfd.revents = 0;
    return poll(&pfd, 1, -1) > 0;
}

int tls_get(int fd, const char* host, const char* caFile)
{
    int ret;
    mbedtls_net_context net;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt ca;
    unsigned char buf[256];
    char info[512];
    const char req[] = "GET / HTTP/1.0\r\n\r\n";
    size_t sent = 0;

    mbedtls_net_init(&net);
    net.fd = fd;
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&ca);

    ret = (int)psa_crypto_init(); /* 4.x: before any parsing */
    if (ret != PSA_SUCCESS) {
        goto out;
    }
    if ((ret = mbedtls_x509_crt_parse_file(&ca, caFile)) != 0) {
        goto out;
    }
    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        goto out;
    }
    mbedtls_ssl_conf_ca_chain(&conf, &ca, NULL);
    /* 3.6 only: mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &drbg); */

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0 ||
        (ret = mbedtls_ssl_set_hostname(&ssl, host)) != 0) {
        goto out;
    }
    mbedtls_ssl_set_bio(&ssl, &net, mbedtls_net_send, mbedtls_net_recv, NULL);

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (!tls_want(fd, ret)) {
            goto out;
        }
    }
    printf("Handshake done, %s, %s\n", mbedtls_ssl_get_version(&ssl),
           mbedtls_ssl_get_ciphersuite(&ssl));

    while (sent < sizeof(req) - 1) {
        ret = mbedtls_ssl_write(&ssl, (const unsigned char*)req + sent,
                                sizeof(req) - 1 - sent);
        if (ret > 0) {
            sent += (size_t)ret;
        }
        else if (!tls_want(fd, ret)) {
            goto out;
        }
    }
    do {
        ret = mbedtls_ssl_read(&ssl, buf, sizeof(buf) - 1);
    } while (tls_want(fd, ret) ||
             ret == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET);
    if (ret > 0) {
        ret = 0;
    }
    else if (ret == 0) {
        ret = MBEDTLS_ERR_SSL_CONN_EOF; /* closed before any data */
    }
    mbedtls_ssl_close_notify(&ssl);

out:
    if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
        mbedtls_x509_crt_verify_info(info, sizeof(info), "  ! ",
                                     mbedtls_ssl_get_verify_result(&ssl));
        printf("%s", info);
    }
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&ca);
    return ret;
}

int main(int argc, char** argv)
{
    mbedtls_net_context net;
    const char*         caFile = CA_FILE;
    const char*         name = NULL;
    const char*         port = DEFAULT_PORT;
    int                 nonBlock = 0;
    int                 flags;
    int                 opt;
    int                 ret;

    while ((opt = getopt(argc, argv, "A:S:p:N")) != -1) {
        if (opt == 'A') {
            caFile = optarg;
        }
        else if (opt == 'S') {
            name = optarg;
        }
        else if (opt == 'p') {
            port = optarg;
        }
        else if (opt == 'N') {
            nonBlock = 1;
        }
        else {
            break;
        }
    }
    if (opt != -1 || optind != argc - 1) {
        printf("usage: %s [-A <CA file>] [-S <server name>] [-p <port>] [-N] "
               "<host>\n", argv[0]);
        return 1;
    }
    if (name == NULL) {
        name = argv[optind];
    }

    mbedtls_net_init(&net);
    if (mbedtls_net_connect(&net, argv[optind], port,
            MBEDTLS_NET_PROTO_TCP) != 0) {
        fprintf(stderr, "ERROR: failed to connect\n");
        return 1;
    }
    /* tls_get() retries on MBEDTLS_ERR_SSL_WANT_READ/WANT_WRITE */
    if (nonBlock) {
        flags = fcntl(net.fd, F_GETFL);
        if (flags == -1 ||
                fcntl(net.fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            fprintf(stderr, "ERROR: failed to set non-blocking\n");
            mbedtls_net_free(&net);
            return 1;
        }
    }

    ret = tls_get(net.fd, name, caFile);
    printf("tls_get() returned %d\n", ret);

    mbedtls_net_free(&net);
    return (ret == 0) ? 0 : 1;
}
