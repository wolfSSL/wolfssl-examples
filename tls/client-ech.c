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
    const char publicIP[] = "104.18.11.118";
    const char publicSNI[] = "cloudflare-ech.com";
    const char privateSNI[] = "crypto.cloudflare.com";
    byte echConfigs[ECHBUFF_LEN];
    word32 echConfigsLen = ECHBUFF_LEN;

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

    /*---------------------------------------------------*/
    /* Start of wolfSSL GREASE connection                */
    /*---------------------------------------------------*/

    /* this first tls connection is only used to get the retry configs - these
     * configs can also be retrieved from DNS
     * i.e. `$ dig cloudflare-ech.com HTTPS` */


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

    /* set the ip string to the cloudflare ECH server */
    servAddr.sin_addr.s_addr = inet_addr( publicIP );

    /* Connect to the server */
    if ((ret = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)))
         == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto cleanup;
    }

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

    /* Set SNI to the ECH server
     * Take care not to set this to the private SNI because that would leak
     * information about it. The private SNI will only be encrypted once the ECH
     * configs are set. */
    if ((ret = wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, publicSNI,
            strlen(publicSNI))) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set public SNI %d\n", ret);
        ret = -1;
        goto ssl_cleanup;
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor %d\n", ret);
        goto ssl_cleanup;
    }

    /* Connect to Cloudflare ECH server */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to Cloudflare ECH server\n");
        fprintf(stderr, "%d %d\n", ret, wolfSSL_get_error(ssl, ret));
        goto ssl_cleanup;
    }

    /* If the GREASE was successful then retry configs sent by the server
     * should be available. Store these in echConfigs for encrypting the
     * upcoming ECH connection. */
    if ((ret = wolfSSL_GetEchConfigs(ssl, echConfigs, &echConfigsLen)) !=
            WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: unable to get GREASE configs %d\n", ret);
        goto ssl_cleanup;
    }

    while (wolfSSL_shutdown(ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
        ; /* do nothing */
    }

    /* frees all data before client termination */
    wolfSSL_free(ssl);
    close(sockfd);

    ssl = NULL;
    sockfd = -1;

    /*---------------------------------------------------*/
    /* Start of wolfSSL ECH connection                   */
    /*---------------------------------------------------*/

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto ctx_cleanup;
    }
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(SERV_PORT);    /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = inet_addr( publicIP );

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

    /* set the ECH configs with configs taken from the GREASE connection */
    if ((ret = wolfSSL_SetEchConfigs(ssl, echConfigs, echConfigsLen)) !=
            WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set GREASE configs %d\n", ret);
        goto ssl_cleanup;
    }

    /* Now that ECH configs are set the private SNI will be encrypted, therefore
     * it is now fine (and correct) to set the private SNI here */
    if ((ret = wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, privateSNI,
            strlen(privateSNI))) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set private SNI %d\n", ret);
        ret = -1;
        goto ssl_cleanup;
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor %d\n", ret);
        goto ssl_cleanup;
    }

    /* Connect to Cloudflare */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to Cloudflare\n");
        fprintf(stderr, "%d %d\n", ret, wolfSSL_get_error(ssl, ret));
        goto ssl_cleanup;
    }

    /* Write the message that will ask the server for information on the
     * connection */
    wolfSSL_write(ssl, message, strlen(message));
    if ((ret = wolfSSL_write(ssl, message, strlen(message))) !=
            strlen(message)) {
        fprintf(stderr, "ERROR: failed to write entire message\n");
        fprintf(stderr, "%d bytes of %d bytes were sent",
                ret, (int)strlen(message));
        goto ssl_cleanup;
    }

    /* Retrieve the server's response:
     * If ECH is being correctly used then 'sni=encrypted' should show up */
    do
    {
        ret = wolfSSL_read(ssl, rd_buf, RDBUFF_LEN);

        if (ret <= 0)
            break;

        fprintf(stdout, "%.*s", ret, rd_buf);
    /* read until the chunk size is 0 */
    } while (rd_buf[0] != '0');

    while (wolfSSL_shutdown(ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
        ; /* do nothing */
    }

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
