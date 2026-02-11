/* client-ech.c
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
 */

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/test.h>

#define SERV_PORT 443
#define CERT_FILE "../certs/ech-client-cert.pem"
#define RDBUFF_LEN 512
#define ECHBUFF_LEN 256

#ifdef HAVE_ECH
int main(void)
{
    int ret = 0;
    byte rd_buf[RDBUFF_LEN];
    int sockfd = -1;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    struct  sockaddr_in servAddr;
    const char message[] =
        "GET /cdn-cgi/trace/ HTTP/1.1\r\n"
        "Host: crypto.cloudflare.com\r\n"
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:105.0) Gecko/20100101 Firefox/105.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Referer: https://www.google.com/\r\n"
        "DNT: 1\r\n"
        "Connection: keep-alive\r\n"
        "Upgrade-Insecure-Requests: 1\r\n"
        "Sec-Fetch-Dest: document\r\n"
        "Sec-Fetch-Mode: navigate\r\n"
        "Sec-Fetch-Site: cross-site\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "\r\n";
    const char ip_string[] = "104.18.11.118";
    const char SNI[] = "crypto.cloudflare.com";
    uint8_t ech_configs[ECHBUFF_LEN];
    uint32_t ech_configs_len = ECHBUFF_LEN;

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto cleanup;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto cleanup;
    }

    /* this first tls connection is only used to get the retry configs */
    /* these configs can also be retrieved from DNS */

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto cleanup;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(SERV_PORT);    /* on DEFAULT_PORT */

    /* set the ip string to the cloudflare server */
    servAddr.sin_addr.s_addr = inet_addr( ip_string );

    /* Connect to the server */
    if ((ret = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)))
         == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto cleanup;
    }

    /*---------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------*/

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CERT_FILE, NULL))
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto ctx_cleanup;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto ctx_cleanup;
    }

    if (wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, SNI, strlen(SNI)) !=
            WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to wolfSSL_UseSNI\n");
        ret = -1;
        goto ssl_cleanup;
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto ssl_cleanup;
    }

    /* Connect to Cloudflare */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to Cloudflare\n");
        fprintf(stderr, "%d %d\n", ret, wolfSSL_get_error(ssl, ret));
        goto ssl_cleanup;
    }

    /* retrieve the retry configs sent by the server */
    if ((ret = wolfSSL_GetEchConfigs(ssl, ech_configs, &ech_configs_len)) !=
            WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_GetEchConfigs error %d\n", ret);
        goto ssl_cleanup;
    }

    while (wolfSSL_shutdown(ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
        fprintf(stderr, "Shutdown not complete\n");
    }
    fprintf(stderr, "Shutdown complete\n");

    /* frees all data before client termination */
    wolfSSL_free(ssl);
    close(sockfd);

    ssl = NULL;
    sockfd = -1;

    /* now we create a new connection that will send the real ech */

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto ctx_cleanup;
    }
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(SERV_PORT);    /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = inet_addr( ip_string );

    if ((ret = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)))
         == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto ctx_cleanup;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto ctx_cleanup;
    }

    /* set the ech configs taken from dns */
    if ((ret = wolfSSL_SetEchConfigs(ssl, ech_configs, ech_configs_len)) !=
            WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_SetEchConfigs error %d\n", ret);
        goto ssl_cleanup;
    }

    if (wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, SNI, strlen(SNI)) !=
            WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to wolfSSL_UseSNI\n");
        ret = -1;
        goto ssl_cleanup;
    }

    /* Connect wolfssl to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr,  "\t\t\twolfSSL_set_fd error %d\n", ret);
        goto ssl_cleanup;
    }

    /* Connect to Cloudflare */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to Cloudflare\n");
        fprintf(stderr, "%d %d\n", ret, wolfSSL_get_error(ssl, ret));
        goto ssl_cleanup;
    }

    wolfSSL_write(ssl, message, strlen(message));
    if ((ret = wolfSSL_write(ssl, message, strlen(message))) !=
            strlen(message)) {
        fprintf(stderr, "ERROR: failed to write entire message\n");
        fprintf(stderr, "%d bytes of %d bytes were sent",
                ret, (int)strlen(message));
        goto ssl_cleanup;
    }

    do
    {
        ret = wolfSSL_read(ssl, rd_buf, RDBUFF_LEN);

        if (ret <= 0)
            break;

        fprintf(stdout, "%.*s", ret, rd_buf);
    /* read until the chunk size is 0 */
    } while (rd_buf[0] != '0');

    while (wolfSSL_shutdown(ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
        fprintf(stderr, "Shutdown not complete\n");
    }
    fprintf(stderr, "Shutdown complete\n");

    ret = 0;

ssl_cleanup:
    wolfSSL_free(ssl);
    ssl = NULL;
ctx_cleanup:
    wolfSSL_CTX_free(ctx);
cleanup:
    wolfSSL_Cleanup();
    close(sockfd);
    sockfd = -1;

    return ret;
}
#else
int main(void)
{
    fprintf(stderr, "Please build wolfssl with ./configure --enable-ech\n");
    return 1;
}
#endif
