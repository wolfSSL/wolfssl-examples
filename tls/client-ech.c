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

/* ECH client: Attempt to connect to a cloudflare server with ECH.
 *
 * The ECH configs needed for this can be retrieved via DNS, i.e.,
 * $ dig +tls @1.1.1.1 HTTPS crypto.cloudflare.com
 */

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/test.h>
#include <wolfssl/wolfcrypt/coding.h>

#define APPROX_B64_LEN(len) (((len) + 2) / 3 * 4 + 1)

#define SERV_PORT         443
#define RDBUFF_LEN        512
#define RETRY_CONFIGS_LEN 512
#define CERT_FILE         "../certs/ech-client-cert.pem"

#ifdef HAVE_ECH

/* Log error, decoding err as a wolfSSL error code when possible */
static void log_error(const char* function, int err)
{
    if (err != 0) {
        char errStr[WOLFSSL_MAX_ERROR_SZ];
        fprintf(stderr, "ERROR: %s returned %d (%s)\n", function, err,
            wolfSSL_ERR_error_string(err, errStr));
    }
    else {
        /* log WOLFSSL_FAILURE */
        fprintf(stderr, "ERROR: %s failed\n", function);
    }
}

#define LOG_ERROR(function, ret, label)     \
    do {                                    \
        log_error(function, (ret));         \
        (ret) = -1;                         \
        goto label;                         \
    } while (0)

int main(int argc, char** argv)
{
    int ret = 0;
    byte rd_buf[RDBUFF_LEN];
    int sockfd = SOCKET_INVALID;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    struct  sockaddr_in servAddr;
    const char message[] =
        "GET /cdn-cgi/trace HTTP/1.1\r\n"
        "Host: crypto.cloudflare.com\r\n"
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:105.0) Gecko/20100101 Firefox/105.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Referer: https://www.google.com/\r\n"
        "DNT: 1\r\n"
        "Connection: close\r\n"
        "Upgrade-Insecure-Requests: 1\r\n"
        "Sec-Fetch-Dest: document\r\n"
        "Sec-Fetch-Mode: navigate\r\n"
        "Sec-Fetch-Site: cross-site\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "\r\n";
    const char publicIP[] = "104.18.4.139";
    const char privateSNI[] = "crypto.cloudflare.com";
    byte retryConfigs[RETRY_CONFIGS_LEN];
    word32 retryConfigsLen = sizeof(retryConfigs);
    char retryConfigsBase64[APPROX_B64_LEN(RETRY_CONFIGS_LEN)];
    word32 retryConfigsBase64Len = sizeof(retryConfigsBase64);

    /* The wolfSSL ECH client will connect to this server under the SNI given in
     * the echConfig (which is likely cloudflare-ech.com), which will either
     * serve or route the connection to crypto.cloudflare.com where it makes the
     * HTTP request. */

    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: %s <base64 ech config>\n", argv[0]);
        return -1;
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto end;
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
        goto socket_cleanup;
    }

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_Init", ret, socket_cleanup);
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method())) == NULL) {
        ret = WOLFSSL_FAILURE;
        LOG_ERROR("wolfSSL_CTX_new", ret, wolf_cleanup);
    }

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CERT_FILE, NULL))
            != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_load_verify_locations(" CERT_FILE ")", ret,
            ctx_cleanup);
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        ret = WOLFSSL_FAILURE;
        LOG_ERROR("wolfSSL_new", ret, ctx_cleanup);
    }

    /* ECH configs can be retrieved from DNS
     * i.e. `$ dig +tls @1.1.1.1 HTTPS crypto.cloudflare.com` */

    /* Set the ECH configs. This will setup the client to perform an ECH
     * connection with the server.
     * If these are not set then a GREASE ECH extension will be sent. When
     * sending a GREASE ECH the connection will NOT hide any information - it is
     * basically a normal TLSv13 connection. */
    if ((ret = wolfSSL_SetEchConfigsBase64(ssl, argv[1], strlen(argv[1]))) !=
            WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_SetEchConfigsBase64", ret, cleanup);
    }

    /* Now that ECH configs are set the private SNI will be encrypted */
    if ((ret = wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, privateSNI,
            strlen(privateSNI))) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_UseSNI", ret, cleanup);
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_set_fd", ret, cleanup);
    }

    /* Connect to Cloudflare */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        ret = wolfSSL_get_error(ssl, ret);

        if (wolfSSL_GetEchRetryConfigs(ssl, retryConfigs, &retryConfigsLen) ==
                WOLFSSL_SUCCESS) {
            fprintf(stderr, "\n");
            if (Base64_Encode_NoNl(retryConfigs, retryConfigsLen,
                    (byte*)retryConfigsBase64, &retryConfigsBase64Len) != 0) {
                fprintf(stderr, "ERROR: failed to encode retry configs "
                                "in Base64\n");
            }
            else {
                printf("Received retry configs: %s\n", retryConfigsBase64);
            }
        }
        LOG_ERROR("wolfSSL_connect", ret, cleanup);
    }

    /* Write the message that will ask the server for information on the
     * connection */
    if ((ret = wolfSSL_write(ssl, message, strlen(message))) !=
            (int)strlen(message)) {
        fprintf(stderr, "%d bytes of %d bytes were sent\n",
                ret, (int)strlen(message));
        ret = wolfSSL_get_error(ssl, ret);
        LOG_ERROR("wolfSSL_write", ret, cleanup);
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

    /* a clean close returns 0; anything negative is a real read error */
    if (ret < 0) {
        ret = wolfSSL_get_error(ssl, ret);
        LOG_ERROR("wolfSSL_read", ret, cleanup);
    }

    while (wolfSSL_shutdown(ssl) == WOLFSSL_SHUTDOWN_NOT_DONE) {
        ; /* do nothing */
    }

    ret = 0;

cleanup:
    /* Cleanup and return */
    if (wolfSSL_GetEchStatus(ssl) == WOLFSSL_ECH_STATUS_ACCEPTED) {
        if (ret == 0)
            fprintf(stdout, "\nClient: ECH successful\n");
        else
            fprintf(stdout, "\nClient: ECH successful, "
                               "but connection failed\n");
    }
    else {
        fprintf(stdout, "\nClient: ECH failed\n");
    }

    wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
ctx_cleanup:
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
wolf_cleanup:
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
socket_cleanup:
    close(sockfd);          /* Close the connection to the server       */
end:
    return ret;
}
#else
int main(void)
{
    fprintf(stderr, "Please build wolfssl with ./configure --enable-ech\n");
    return 1;
}
#endif
