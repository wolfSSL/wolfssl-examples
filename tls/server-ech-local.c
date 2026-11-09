/* server-ech-local.c
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

/* ECH server: a single WOLFSSL_CTX with just a single private SNI. This is not
 * an effective use of ECH, instead it is meant to showcase a minimal setup.
 *
 * This example should give a good understanding of the basic workflow needed
 * for ECH before bringing in the systems needed for managing multiple SNIs.
 *
 * Pair with client-ech-local to connect.
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

#define HAVE_SIGNAL
#ifdef HAVE_SIGNAL
#include <signal.h>
#endif

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/test.h>
#include <wolfssl/wolfcrypt/coding.h>

#define APPROX_B64_LEN(len) (((len) + 2) / 3 * 4 + 1)

#define DEFAULT_PORT   11111
#define BUFF_LEN       256
#define ECH_CONFIG_LEN 256
#define CERT_FILE      "../certs/server-cert.pem"
#define KEY_FILE       "../certs/server-key.pem"

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

int main()
{
    int                sockfd = SOCKET_INVALID;
    int                connd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[BUFF_LEN];
    size_t             len;
    int                shutdown = 0;
    int                ret;
    int                status;
    const char*        reply = "I hear ya fa shizzle!\n";
    const char*        publicName = "public.com";
    const char*        privateName = "example.com";
    int                privateNameLen = strlen(privateName);
    byte               echConfig[ECH_CONFIG_LEN];
    word32             echConfigLen = sizeof(echConfig);
    char               echConfigBase64[APPROX_B64_LEN(ECH_CONFIG_LEN)];
    word32             echConfigBase64Len = sizeof(echConfigBase64);

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

#ifdef HAVE_SIGNAL
    /* A client that resets the connection can make a later wolfSSL_write()
     * raise SIGPIPE; ignore it so a misbehaving client cannot kill the
     * server. */
    signal(SIGPIPE, SIG_IGN);
#endif

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_Init", ret, bad_init);
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method())) == NULL) {
        ret = WOLFSSL_FAILURE;
        LOG_ERROR("wolfSSL_CTX_new", ret, exit);
    }

    /* Generate ech config */
    if ((ret = wolfSSL_CTX_GenerateEchConfig(ctx, publicName, 0, 0, 0))
            != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_GenerateEchConfig", ret, exit);
    }

    if ((ret = wolfSSL_CTX_GetEchConfigs(ctx, echConfig, &echConfigLen))
            != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_GetEchConfigs", ret, exit);
    }

    if ((ret = Base64_Encode_NoNl(echConfig, echConfigLen,
            (byte*)echConfigBase64, &echConfigBase64Len)) != 0) {
        LOG_ERROR("Base64_Encode_NoNl", ret, exit);
    }

    fprintf(stdout, "ECH config: %s\n", echConfigBase64);

    fprintf(stdout, "Private name: %s\n\n", privateName);

    if ((ret = wolfSSL_CTX_UseSNI(ctx, WOLFSSL_SNI_HOST_NAME, privateName,
            privateNameLen)) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_UseSNI", ret, exit);
    }

    /* For this example to truly work the certificate installed would need to
     * have both the publicName and privateName in its certificate. This
     * example is only here to show a very simple setup of ECH so this has not
     * been done.
     *
     * The consequence of this is that checking the domain name when ECH is
     * rejected will always return a failure since the publicName will
     * not match. */

    /* Load server certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE,
            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_use_certificate_file(" CERT_FILE ")", ret,
            exit);
    }
    /* Load server key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE,
            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_use_PrivateKey_file(" KEY_FILE ")", ret, exit);
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */

    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto exit;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1;
            goto exit;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            ret = WOLFSSL_FAILURE;
            LOG_ERROR("wolfSSL_new", ret, exit);
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);

        /* Establish TLS connection */
        ret = wolfSSL_accept(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            ret = wolfSSL_get_error(ssl, ret);
            LOG_ERROR("wolfSSL_accept", ret, restart_server);
        }

        printf("Client connected successfully\n");

        status = wolfSSL_GetEchStatus(ssl);
        if (status == WOLFSSL_ECH_STATUS_ACCEPTED) {
            printf("Client connected with ECH\n");
        }
        else if (status == WOLFSSL_ECH_STATUS_NOT_OFFERED) {
            printf("Client did not send ECH\n");
        }
        else if (status == WOLFSSL_ECH_STATUS_REJECTED) {
            printf("Client ECH failed, sent retry configs\n");
        }

        /* When ECH is rejected the client may abort before sending anything, so
         * handle the potential IO errors here */
        memset(buff, 0, sizeof(buff));
        ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
        if (ret > 0) {
            /* Print to stdout any data the client sends */
            printf("Client: %s", buff);

            /* Check for server shutdown command */
            if (strncmp(buff, "shutdown", 8) == 0) {
                printf("Shutdown command issued!\n");
                shutdown = 1;
            }

            /* Write our reply into buff */
            memset(buff, 0, sizeof(buff));
            memcpy(buff, reply, strlen(reply));
            len = strnlen(buff, sizeof(buff));

            /* Reply back to the client; treat a bad write as non-fatal */
            if (wolfSSL_write(ssl, buff, len) != (int)len)
                fprintf(stderr, "ERROR: failed to write\n");
        }
        else {
            int err = wolfSSL_get_error(ssl, ret);
            if (err == WOLFSSL_ERROR_ZERO_RETURN)
                printf("Client closed the connection\n");
            else
                fprintf(stderr, "wolfSSL_read error = %d\n", err);
        }

        /* Notify the client that the connection is ending */
        wolfSSL_shutdown(ssl);
        printf("Shutdown complete\n");

        /* Cleanup after this connection */
restart_server:
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        ssl = NULL;
        close(connd);           /* Close the connection to the client   */
        connd = SOCKET_INVALID;
    }

    ret = 0;

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
    if (connd != SOCKET_INVALID)
        close(connd);           /* Close the connection to the client       */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket listening for clients   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */
bad_init:

    return ret;                 /* Return reporting a success               */
}
#else
int main(void)
{
    printf("Please build wolfssl with ./configure --enable-ech\n");
    return 1;
}
#endif
