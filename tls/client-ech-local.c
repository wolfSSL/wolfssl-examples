/* client-ech-local.c
 *
 * Copyright (C) 2023 wolfSSL Inc.
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

/* Local ECH client: Attempt to connect to a local ECH-enabled server. An ECH
 * config must be provided as well as a target SNI.
 *
 * The associated ECH configs can be acquired from a locally run ECH server.
 * Each of the example servers will print the possible target SNIs.
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

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/test.h>
#include <wolfssl/wolfcrypt/coding.h>

#define APPROX_B64_LEN(len) (((len) + 2) / 3 * 4 + 1)

#define DEFAULT_PORT      11111
#define BUFF_LEN          256
#define RETRY_CONFIGS_LEN 256
#define CERT_FILE         "../certs/ca-cert.pem"

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
    int                sockfd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    char               buff[BUFF_LEN];
    size_t             len;
    int                ret;
    byte               retryConfigs[RETRY_CONFIGS_LEN];
    word32             retryConfigsLen = sizeof(retryConfigs);
    char               retryConfigsBase64[APPROX_B64_LEN(RETRY_CONFIGS_LEN)];
    word32             retryConfigsBase64Len = sizeof(retryConfigsBase64);

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    /* Check for proper calling convention */
    if (argc != 3) {
        printf("usage: %s <base64 ech config> <private SNI>\n", argv[0]);
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
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */

    /* Connect over localhost */
    if (inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        ret = -1;
        goto socket_cleanup;
    }

    /* Connect to the server */
    if ((ret = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)))
            == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto socket_cleanup;
    }

    /*---------------------------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------------------------*/

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

    /* Set ECH configs to those provided on the command line */
    if ((ret = wolfSSL_SetEchConfigsBase64(ssl, argv[1], strlen(argv[1]))) !=
            WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_SetEchConfigsBase64", ret, cleanup);
    }

    /* Use the private SNI provided on the command line */
    if ((ret = wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, argv[2],
            strlen(argv[2]))) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_UseSNI", ret, cleanup);
    }

    /* Make sure the certificates are verified */
    wolfSSL_set_verify(ssl, WOLFSSL_VERIFY_PEER, NULL);
    /* and that the domain is checked */
    if ((ret = wolfSSL_check_domain_name(ssl, argv[2])) !=
            WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_check_domain_name", ret, cleanup);
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_set_fd", ret, cleanup);
    }

    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        ret = wolfSSL_get_error(ssl, ret);
        if (wolfSSL_GetEchRetryConfigs(ssl, retryConfigs, &retryConfigsLen) ==
                WOLFSSL_SUCCESS) {
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

    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto cleanup;
    }
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    if ((ret = wolfSSL_write(ssl, buff, len)) != (int)len) {
        fprintf(stderr, "%d bytes of %d bytes were sent\n", ret, (int)len);
        ret = wolfSSL_get_error(ssl, ret);
        LOG_ERROR("wolfSSL_write", ret, cleanup);
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) < 0) {
        ret = wolfSSL_get_error(ssl, ret);
        LOG_ERROR("wolfSSL_read", ret, cleanup);
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);

    /* Bidirectional shutdown */
    while (wolfSSL_shutdown(ssl) == SSL_SHUTDOWN_NOT_DONE) {
        printf("Shutdown not complete\n");
    }

    printf("Shutdown complete\n");

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
    return ret;             /* Return reporting a success               */
}
#else
int main(void)
{
    printf("Please build wolfssl with ./configure --enable-ech\n");
    return 1;
}
#endif
