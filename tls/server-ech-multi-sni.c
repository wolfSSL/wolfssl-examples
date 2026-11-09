/* server-ech-multi-sni.c
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

/* Multi-SNI ECH server: a single WOLFSSL_CTX fronts multiple tenants based on
 * the inner/private SNI. A servername callback dispatches on the inner SNI and
 * installs that tenant's certificate and private key on the per-connection
 * WOLFSSL object.
 *
 * Pair with client-ech-local and use one of the tenant names as the
 * inner/private SNI.
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
#define PUBLIC_NAME    "public.com"
#define ECH_CERT_FILE  "../certs/ech-public-cert.pem"
#define ECH_KEY_FILE   "../certs/ech-public-key.pem"

#define TENANT_A_NAME       "tenant-a.example"
#define TENANT_A_CERT_FILE  "../certs/tenant-a-cert.pem"
#define TENANT_A_KEY_FILE   "../certs/server-key.pem"
#define TENANT_B_NAME       "tenant-b.example"
#define TENANT_B_CERT_FILE  "../certs/tenant-b-cert.pem"
#define TENANT_B_KEY_FILE   "../certs/server-key.pem"

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

/* The SNI, cert, and key for a tenant */
typedef struct ech_tenant {
    const char* name;
    const char* certFile;
    const char* keyFile;
} ech_tenant;

/* SNI dispatch callback. This is invoked for the SNI chosen by the server:
 *   ECH rejected / GREASE / no ECH    -> outer (cleartext) SNI
 *   ECH accepted                      -> inner SNI
 *
 * The selected cert/key are installed on the WOLFSSL object so the handshake
 * uses them in place of the CTX defaults.
 *
 * returns:
 *   0            -> name match (cert/key selected)
 *   noack_return -> no SNI or unknown name: keep the default cert/key from the
 *                   CTX, without acking the SNI
 *   fatal_return -> matched tenant's cert/key failed to load + aborts */
static int sni_select_tenant(WOLFSSL* ssl, int* ad, void* arg)
{
    const ech_tenant* tenants = (const ech_tenant*)arg;
    void*             name = NULL;
    word16            nameLen;

    if (tenants == NULL)
        return noack_return;

    /* Access the received SNI */
    nameLen = wolfSSL_SNI_GetRequest(ssl, WOLFSSL_SNI_HOST_NAME, &name);

    /* no SNI provided, keep the default cert and key installed */
    if (nameLen == 0 || name == NULL)
        return noack_return;

    fprintf(stdout, "Got client SNI: %.*s\n", (int)nameLen, (const char*)name);

    /* public name -> default cert and key already installed on the CTX */
    if (strlen(PUBLIC_NAME) == nameLen &&
            strncmp((const char*)name, PUBLIC_NAME, nameLen) == 0) {
        return 0;
    }

    /* otherwise match one of the configured tenants and install its cert/key */
    for (; tenants->name != NULL; tenants++) {
        if (strlen((const char*)tenants->name) == nameLen &&
                strncmp((const char*)name, tenants->name, nameLen) == 0) {
            if (wolfSSL_use_certificate_file(ssl, tenants->certFile,
                    WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS ||
                wolfSSL_use_PrivateKey_file(ssl, tenants->keyFile,
                    WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
                if (ad)
                    *ad = internal_error;
                return fatal_return;
            }
            return 0;
        }
    }

    /* unknown name -> fall back to the default host, without acking the SNI */
    return noack_return;
}

int main(void)
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
    byte               echConfig[ECH_CONFIG_LEN];
    word32             echConfigLen = sizeof(echConfig);
    char               echConfigBase64[APPROX_B64_LEN(ECH_CONFIG_LEN)];
    word32             echConfigBase64Len = sizeof(echConfigBase64);

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    const ech_tenant tenants[] = {
        { TENANT_A_NAME, TENANT_A_CERT_FILE, TENANT_A_KEY_FILE },
        { TENANT_B_NAME, TENANT_B_CERT_FILE, TENANT_B_KEY_FILE },
        { NULL, NULL, NULL }
    };
    const ech_tenant* tenants_p;

#ifdef HAVE_SIGNAL
    /* A client that resets the connection can make a later wolfSSL_write()
     * raise SIGPIPE; ignore it so a client cannot kill the server. */
    signal(SIGPIPE, SIG_IGN);
#endif

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_Init", ret, bad_init);
    }

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

    /* Generate the ECH config */
    if ((ret = wolfSSL_CTX_GenerateEchConfig(ctx, PUBLIC_NAME, 0, 0, 0))
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

    tenants_p = tenants;
    fprintf(stdout, "Tenants:\n");
    for (; tenants_p->name != NULL; tenants_p++) {
        fprintf(stdout, "  %s\n", tenants_p->name);
    }
    fprintf(stdout, "\n");

    /* Load a default cert/key on the CTX for the publicName / fallback path.
     * Clients with ECH-rejected handshakes use these. */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, ECH_CERT_FILE,
            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_use_certificate_file(" ECH_CERT_FILE ")", ret,
            exit);
    }

    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, ECH_KEY_FILE,
            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_use_PrivateKey_file(" ECH_KEY_FILE ")", ret,
            exit);
    }

    /* Register the dispatch callback */
    wolfSSL_CTX_set_servername_callback(ctx, sni_select_tenant);
    wolfSSL_CTX_set_servername_arg(ctx, (void*)tenants);

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

    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }

    while (!shutdown) {
        fprintf(stdout, "\nWaiting for a connection...\n");

        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
                == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1;
            goto exit;
        }

        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            ret = WOLFSSL_FAILURE;
            LOG_ERROR("wolfSSL_new", ret, exit);
        }

        wolfSSL_set_fd(ssl, connd);

        /* Attempt to receive a client: the SNI callback fires mid-handshake
         * while parsing the inner ClientHello */
        ret = wolfSSL_accept(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            ret = wolfSSL_get_error(ssl, ret);
            LOG_ERROR("wolfSSL_accept", ret, restart_server);
        }

        fprintf(stdout, "Client connected successfully\n");

        status = wolfSSL_GetEchStatus(ssl);
        if (status == WOLFSSL_ECH_STATUS_ACCEPTED) {
            fprintf(stdout, "Client connected with ECH\n");
        }
        else if (status == WOLFSSL_ECH_STATUS_NOT_OFFERED) {
            fprintf(stdout, "Client did not send ECH\n");
        }
        else if (status == WOLFSSL_ECH_STATUS_REJECTED) {
            fprintf(stdout, "Client ECH failed, sent retry configs\n");
        }

        /* When ECH is rejected the client may abort before sending anything, so
         * handle the potential IO errors here */
        memset(buff, 0, sizeof(buff));
        ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
        if (ret > 0) {
            fprintf(stdout, "Client: %s", buff);

            if (strncmp(buff, "shutdown", 8) == 0) {
                fprintf(stdout, "Shutdown command issued!\n");
                shutdown = 1;
            }

            memset(buff, 0, sizeof(buff));
            memcpy(buff, reply, strlen(reply));
            len = strnlen(buff, sizeof(buff));

            /* Treat a bad write as non-fatal too */
            if (wolfSSL_write(ssl, buff, len) != (int)len)
                fprintf(stderr, "ERROR: failed to write\n");
        }
        else {
            int err = wolfSSL_get_error(ssl, ret);
            if (err == WOLFSSL_ERROR_ZERO_RETURN)
                fprintf(stdout, "Client closed the connection\n");
            else
                fprintf(stderr, "wolfSSL_read error = %d\n", err);
        }

        wolfSSL_shutdown(ssl);
        fprintf(stdout, "Shutdown complete\n");

restart_server:
        wolfSSL_free(ssl);
        ssl = NULL;
        close(connd);
        connd = SOCKET_INVALID;
    }

    ret = 0;

exit:
    if (ssl)
        wolfSSL_free(ssl);
    if (connd != SOCKET_INVALID)
        close(connd);
    if (sockfd != SOCKET_INVALID)
        close(sockfd);
    if (ctx)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
bad_init:

    return ret;
}
#else
int main(void)
{
    fprintf(stdout, "Please build wolfssl with ./configure --enable-ech\n");
    return 1;
}
#endif
