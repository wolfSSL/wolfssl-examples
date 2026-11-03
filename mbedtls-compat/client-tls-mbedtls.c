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
 * compatibility layer. tls_get() is unchanged from the Mbed TLS migration
 * guide. */

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

/* Mbed TLS, provided by wolfPSA */
#include <psa/crypto.h>
#include <mbedtls/ssl.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/x509_crt.h>

#define DEFAULT_PORT 11111

#define CA_FILE   "../certs/ca-cert.pem"
/* DNS name in the subjectAltName of ../certs/server-cert.pem */
#define HOST_NAME "example.com"

int tls_get(int fd, const char* host, const char* caFile)
{
    int ret;
    mbedtls_net_context net;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt ca;
    unsigned char buf[256];
    const char req[] = "GET / HTTP/1.0\r\n\r\n";

    mbedtls_net_init(&net);
    net.fd = fd;
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&ca);

    ret = (int)psa_crypto_init(); /* 4.x: before any parsing */
    if (ret != PSA_SUCCESS)
        goto out;
    if ((ret = mbedtls_x509_crt_parse_file(&ca, caFile)) != 0)
        goto out;
    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
        goto out;
    mbedtls_ssl_conf_ca_chain(&conf, &ca, NULL);
    /* 3.6 only: mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &drbg); */

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0 ||
        (ret = mbedtls_ssl_set_hostname(&ssl, host)) != 0)
        goto out;
    mbedtls_ssl_set_bio(&ssl, &net, mbedtls_net_send, mbedtls_net_recv, NULL);

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            goto out;
    }
    ret = mbedtls_ssl_write(&ssl, (const unsigned char*)req, sizeof(req) - 1);
    if (ret <= 0)
        goto out;
    do {
        ret = mbedtls_ssl_read(&ssl, buf, sizeof(buf) - 1);
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
             ret == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET);
    if (ret > 0)
        ret = 0;
    mbedtls_ssl_close_notify(&ssl);

out:
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&ca);
    return ret;
}

int main(int argc, char** argv)
{
    int                sockfd;
    struct sockaddr_in servAddr;
    const char*        caFile = CA_FILE;
    const char*        host = HOST_NAME;
    int                nonBlock = 0;
    int                opt;
    int                ret;

    while ((opt = getopt(argc, argv, "A:S:N")) != -1) {
        if (opt == 'A')
            caFile = optarg;
        else if (opt == 'S')
            host = optarg;
        else if (opt == 'N')
            nonBlock = 1;
        else
            break;
    }
    if (opt != -1 || optind != argc - 1) {
        printf("usage: %s [-A <CA file>] [-S <server name>] [-N] "
               "<IPv4 address>\n", argv[0]);
        return 1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port   = htons(DEFAULT_PORT);
    if (inet_pton(AF_INET, argv[optind], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        return 1;
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return 1;
    }
    if (connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        close(sockfd);
        return 1;
    }
    /* tls_get() retries on MBEDTLS_ERR_SSL_WANT_READ/WANT_WRITE */
    if (nonBlock &&
            fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK) != 0) {
        fprintf(stderr, "ERROR: failed to set non-blocking\n");
        close(sockfd);
        return 1;
    }

    ret = tls_get(sockfd, host, caFile);
    printf("tls_get() returned %d\n", ret);

    close(sockfd);
    return (ret == 0) ? 0 : 1;
}
