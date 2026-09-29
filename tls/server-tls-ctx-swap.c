/* server-tls-ctx-swap.c
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

/* Multi-CTX TLS server: a default WOLFSSL_CTX fronts multiple tenants, with a
 * separate WOLFSSL_CTX (cert + key) prepared for each tenant. A servername
 * callback dispatches on the SNI and swaps the live connection onto that
 * tenant's CTX. Clients that send no SNI, or an SNI matching no
 * tenant, stay on the default CTX.
 *
 * Unlike `server-tls-sni-callback.c` (which installs the tenant cert/key
 * directly on the per-connection WOLFSSL object) this swaps the whole CTX
 * with wolfSSL_set_SSL_CTX midway through the handshake. This API requires
 * wolfSSL built with --enable-opensslextra (or --enable-all).
 *
 * Accepts both TLS 1.2 and TLS 1.3 connections.
 *
 * Test with a client that sends one of the tenant names as its SNI, e.g.
 * wolfSSL's example client: ./examples/client/client -S tenant-a.example
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

#define DEFAULT_PORT      11111
#define BUFF_LEN          256
#define DEFAULT_CERT_FILE "../certs/server-cert.pem"
#define DEFAULT_KEY_FILE  "../certs/server-key.pem"

#define TENANT_A_NAME       "tenant-a.example"
#define TENANT_A_CERT_FILE  "../certs/tenant-a-cert.pem"
#define TENANT_A_KEY_FILE   "../certs/server-key.pem"
#define TENANT_B_NAME       "tenant-b.example"
#define TENANT_B_CERT_FILE  "../certs/tenant-b-cert.pem"
#define TENANT_B_KEY_FILE   "../certs/server-key.pem"

#if defined(HAVE_SNI) && (defined(OPENSSL_ALL) || defined(OPENSSL_EXTRA))

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

/* The SNI, cert, key, and per-tenant context */
typedef struct tls_tenant {
    const char*  name;
    const char*  certFile;
    const char*  keyFile;
    WOLFSSL_CTX* ctx;
} tls_tenant;

/* This is used to print which tenant was selected */
static const tls_tenant* selectedTenant = NULL;

/* SNI dispatch callback, invoked mid-handshake with the SNI sent by the
 * client.
 *
 * On a tenant match the connection is swapped onto that tenant's CTX with
 * wolfSSL_set_SSL_CTX, so the handshake uses its cert/key in place of the
 * default CTX's.
 *
 * returns:
 *   0            -> tenant match (ctx selected)
 *   noack_return -> no SNI or unknown name: stay on the default CTX, without
 *                   acking the SNI
 *   fatal_return -> failed to swap onto a matched tenant's CTX + aborts */
static int sni_select_tenant(WOLFSSL* ssl, int* ad, void* arg)
{
    const tls_tenant* tenants = (const tls_tenant*)arg;
    void*             name = NULL;
    word16            nameLen;

    if (tenants == NULL)
        return noack_return;

    /* Access the received SNI */
    nameLen = wolfSSL_SNI_GetRequest(ssl, WOLFSSL_SNI_HOST_NAME, &name);

    /* no SNI -> default CTX already in use */
    if (nameLen == 0 || name == NULL)
        return noack_return;

    fprintf(stdout, "Got client SNI: %.*s\n", (int)nameLen, (const char*)name);

    /* match one of the configured tenants and swap to its ctx */
    for (; tenants->name != NULL; tenants++) {
        if (strlen((const char*)tenants->name) == nameLen &&
                strncmp((const char*)name, tenants->name, nameLen) == 0) {
            if (wolfSSL_set_SSL_CTX(ssl, tenants->ctx) == NULL) {
                if (ad)
                    *ad = internal_error;
                return fatal_return;
            }
            selectedTenant = tenants;
            return 0;
        }
    }

    /* unknown name -> fall back to the default host, without acking the SNI.
     * Strict-SNI deployments could instead abort with:
     *     if (ad)
     *         *ad = unrecognized_name;
     *     return fatal_return; */
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
    const char*        reply = "I hear ya fa shizzle!\n";

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    tls_tenant tenants[] = {
        { TENANT_A_NAME, TENANT_A_CERT_FILE, TENANT_A_KEY_FILE, NULL },
        { TENANT_B_NAME, TENANT_B_CERT_FILE, TENANT_B_KEY_FILE, NULL },
        { NULL, NULL, NULL, NULL }
    };
    tls_tenant* tenants_p;

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

    /* Create and initialize WOLFSSL_CTX: accept the highest TLS version the
     * client supports, but no lower than TLS 1.2 */
    if ((ctx = wolfSSL_CTX_new(wolfSSLv23_server_method())) == NULL) {
        ret = WOLFSSL_FAILURE;
        LOG_ERROR("wolfSSL_CTX_new", ret, exit);
    }

    if ((ret = wolfSSL_CTX_SetMinVersion(ctx, WOLFSSL_TLSV1_2))
            != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_SetMinVersion", ret, exit);
    }

    tenants_p = tenants;
    fprintf(stdout, "Tenants:\n");
    for (; tenants_p->name != NULL; tenants_p++) {
        fprintf(stdout, "  %s\n", tenants_p->name);
    }
    fprintf(stdout, "\n");

    /* Load a default cert/key on the CTX, used when the client sends no SNI
     * or an SNI matching no tenant */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, DEFAULT_CERT_FILE,
            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_use_certificate_file(" DEFAULT_CERT_FILE ")",
            ret, exit);
    }

    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, DEFAULT_KEY_FILE,
            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        LOG_ERROR("wolfSSL_CTX_use_PrivateKey_file(" DEFAULT_KEY_FILE ")", ret,
            exit);
    }

    /* Build a CTX per tenant, each with its own cert and key. The servername
     * callback swaps the live connection onto one of these (via
     * wolfSSL_set_SSL_CTX) once the SNI is known. */
    for (tenants_p = tenants; tenants_p->name != NULL; tenants_p++) {
        if ((tenants_p->ctx = wolfSSL_CTX_new(wolfSSLv23_server_method()))
                == NULL) {
            fprintf(stderr, "ERROR: failed to create CTX for tenant %s\n",
                tenants_p->name);
            ret = -1;
            goto exit;
        }

        if (wolfSSL_CTX_use_certificate_file(tenants_p->ctx,
                tenants_p->certFile, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load cert for tenant %s\n",
                tenants_p->name);
            ret = -1;
            goto exit;
        }

        if (wolfSSL_CTX_use_PrivateKey_file(tenants_p->ctx,
                tenants_p->keyFile, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load key for tenant %s\n",
                tenants_p->name);
            ret = -1;
            goto exit;
        }
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

        /* Reset the marked selection */
        selectedTenant = NULL;

        /* Attempt to receive a client: the SNI callback fires mid-handshake
         * while parsing the ClientHello */
        ret = wolfSSL_accept(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            ret = wolfSSL_get_error(ssl, ret);
            LOG_ERROR("wolfSSL_accept", ret, restart_server);
        }

        fprintf(stdout, "Client connected successfully\n");

        /* The client may abort after the handshake (e.g. if it rejects our
         * certificate), so handle the potential IO errors here */
        memset(buff, 0, sizeof(buff));
        ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
        if (ret > 0) {
            fprintf(stdout, "Client: %s", buff);

            if (strncmp(buff, "shutdown", 8) == 0) {
                fprintf(stdout, "Shutdown command issued!\n");
                shutdown = 1;
            }

            /* print the selected tenant and then the staple reply */
            memset(buff, 0, sizeof(buff));
            if (selectedTenant != NULL)
                snprintf(buff, sizeof(buff), "%s: %s", selectedTenant->name,
                    reply);
            else
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
    for (tenants_p = tenants; tenants_p->name != NULL; tenants_p++) {
        if (tenants_p->ctx)
            wolfSSL_CTX_free(tenants_p->ctx);
    }
    if (ctx)
        wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
bad_init:

    return ret;
}
#else
int main(void)
{
    fprintf(stdout, "Please build wolfssl with ./configure --enable-sni "
        "--enable-opensslextra\n");
    return 1;
}
#endif
