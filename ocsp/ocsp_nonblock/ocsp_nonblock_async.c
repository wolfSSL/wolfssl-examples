/* ocsp_nonblock_async.c
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
 **/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <netdb.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include <wolfssl/error-ssl.h>

#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/ecc.h>

#define SERVER_NAME "www.youtube.com"

#define SERVER_PORT 443
#define ALPN_PROTOS "http/1.1"

#if defined(WOLFSSL_ASYNC_CRYPT) && defined(HAVE_SNI) && defined(HAVE_ALPN) \
    && defined(WOLFSSL_NONBLOCK_OCSP) && defined(HAVE_CERTIFICATE_STATUS_REQUEST) \
    && defined(HAVE_CERTIFICATE_STATUS_REQUEST_V2)

static const char* sys_certs_file = "/etc/ssl/certs/ca-certificates.crt";

#if defined(WOLFSSL_ASYNC_CRYPT)
static int wait_async(WOLFSSL_CTX* ctx, WOLFSSL* ssl)
{
    WOLF_EVENT* wolfEvent;
    int n = 0;
    do {
        if (wolfSSL_CTX_AsyncPoll(ctx, &wolfEvent, 1, WOLF_POLL_FLAG_CHECK_HW, &n) != 0) {
            fprintf(stderr, "error calling wolfSSL_CTX_AsyncPoll()\n");
            return -1;
        }
    } while (n == 0);
    assert(n == 1);
    assert(ssl == (WOLFSSL*)wolfEvent->context);
    return 0;
}
#endif

static int verify_cb(int preverify_ok, WOLFSSL_X509_STORE_CTX* store)
{
    printf("verify_cb()\n");
    printf("  preverify_ok = %d\n", preverify_ok);
    if (preverify_ok == 0) {
        printf("  VERIFY FAILED\n");
        printf("  store->error_depth: %d\n", store->error_depth);
        printf("  store->error: %d\n", store->error);
    }

    return preverify_ok;
}

static int ocsp_cb(void* ctx, const char* url, int urlSz, unsigned char* request, int requestSz, unsigned char** response)
{
    printf("ocsp_cb(): %s\n", url);

    /* Simulate a non-blocking responder: return "want read" once (using a
     * marker file to remember the call), then perform the lookup on the retry. */
    if (access("ocsp.req", F_OK) != 0) {
        FILE* mark = fopen("ocsp.req", "wb");
        if (mark == NULL)
            return -1;
        if (fclose(mark) != 0)
            return -1;
        printf("  simulate 'want read'\n");
        return WOLFSSL_CBIO_ERR_WANT_READ;
    }
    if (remove("ocsp.req") != 0)
        return -1;

    /* Use wolfSSL's built-in OCSP HTTP lookup, which parses the URL and
     * fetches the response over a socket. */
    return EmbedOcspLookup(ctx, url, urlSz, request, requestSz, response);
}

static void ocsp_free(void* ctx, unsigned char* response)
{
    /* Pair with EmbedOcspLookup's allocator. */
    EmbedOcspRespFree(ctx, response);
}

int test_connect(WOLFSSL_CTX* ctx)
{
    int ret;
    int errCode;
    char errBuff[WOLFSSL_MAX_ERROR_SZ];

    int sockfd = -1;
    struct addrinfo hints, *res = NULL, *ai;
    char portStr[6];

    int result = 0;

    ret = snprintf(portStr, sizeof(portStr), "%d", SERVER_PORT);
    if (ret < 0 || (size_t)ret >= sizeof(portStr)) {
        fprintf(stderr, "failed to format port %d\n", SERVER_PORT);
        result = -1;
        goto exit;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /* Resolve the server by name so the example keeps working as its IP changes. */
    if (getaddrinfo(SERVER_NAME, portStr, &hints, &res) != 0) {
        fprintf(stderr, "failed to resolve %s\n", SERVER_NAME);
        result = -1;
        goto exit;
    }

    for (ai = res; ai != NULL; ai = ai->ai_next) {
        sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sockfd == -1)
            continue;
        if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;
        if (close(sockfd) != 0)
            fprintf(stderr, "warning: failed to close socket\n");
        sockfd = -1;
    }
    freeaddrinfo(res);

    if (sockfd == -1) {
        fprintf(stderr, "failed to connect to %s:%d\n", SERVER_NAME, SERVER_PORT);
        result = -1;
        goto exit;
    }

    // set non-block socket
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        fprintf(stderr, "fcntl(F_GETFL) failed\n");
        result = -1;
        goto exit_closefd;
    }
    if ((flags & O_NONBLOCK) == 0) {
        flags |= O_NONBLOCK;
        if (fcntl(sockfd, F_SETFL, flags) != 0) {
            fprintf(stderr, "fcntl(F_SETFL) failed\n");
            result = -1;
            goto exit_closefd;
        }
    }

    WOLFSSL *ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        fprintf(stderr, "wolfSSL_new() failed\n");
        result = -1;
        goto exit_closefd;
    }

    wolfSSL_set_fd(ssl, sockfd);

    wolfSSL_set_using_nonblock(ssl, 1);

    ret = wolfSSL_SetOCSP_Cb(ssl, ocsp_cb, ocsp_free, NULL);
    if (ret != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_SetOCSP_Cb() failed with code %d\n", ret);
        result = -1;
        goto exit_freessl;
    }

    long opt = wolfSSL_get_options(ssl);
    const long ver_opt_mask = SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2;
    opt = (opt & ~ver_opt_mask) | SSL_OP_NO_SSLv3;
    if (opt != wolfSSL_set_options(ssl, opt)) {
        fprintf(stderr, "Could not configure TLS versions on client stub\n");
        result = -1;
        goto exit_freessl;
    }

    if ((ret = wolfSSL_UseSupportedCurve(ssl, WOLFSSL_ECC_X25519)) != SSL_SUCCESS ||
        (ret = wolfSSL_UseSupportedCurve(ssl, WOLFSSL_ECC_SECP256R1)) != SSL_SUCCESS ||
        (ret = wolfSSL_UseSupportedCurve(ssl, WOLFSSL_ECC_SECP384R1)) != SSL_SUCCESS ||
        (ret = wolfSSL_UseSupportedCurve(ssl, WOLFSSL_ECC_SECP521R1)) != SSL_SUCCESS) {
        fprintf(stderr, "Could not set SSL supported groups on client stub\n");
        result = -1;
        goto exit_freessl;
    }

    ret = wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, SERVER_NAME, strlen(SERVER_NAME));
    if (ret != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_UseSNI() failed with code %d\n", ret);
        result = -1;
        goto exit_freessl;
    }

    ret = wolfSSL_UseALPN(ssl, ALPN_PROTOS, strlen(ALPN_PROTOS), WOLFSSL_ALPN_FAILED_ON_MISMATCH);
    if (ret != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_UseALPN() failed with code %d\n", ret);
        result = -1;
        goto exit_freessl;
    }

    printf("Connecting...\n");

    do {
        ret = wolfSSL_connect(ssl);
        errCode = 0;
        if (ret != SSL_SUCCESS) {
            errCode = wolfSSL_get_error(ssl, ret);

            if (errCode != WOLFSSL_ERROR_WANT_READ) {
                printf("wolfSSL_connect() returned %d (error code %d)\n", ret, errCode);
            }

#if defined(WOLFSSL_ASYNC_CRYPT)
            if (errCode == WC_PENDING_E) {
                if (wait_async(ctx, ssl) != 0) {
                    fprintf(stderr, "wait_async() failed\n");
                    result = -1;
                    goto exit_freessl;
                }
            }
#endif
        }

    } while (ret != SSL_SUCCESS && (errCode == SSL_ERROR_WANT_READ || errCode == OCSP_WANT_READ || errCode == WC_PENDING_E));

    if (ret != SSL_SUCCESS) {
        errCode = wolfSSL_get_error(ssl, ret);
        fprintf(stderr, "wolfSSL_connect() failed with code %d\n", ret);
        fprintf(stderr, "ERROR %d: %s\n", errCode,
                wolfSSL_ERR_error_string((unsigned long) errCode, errBuff));
        if (errCode == FATAL_ERROR) {
            WOLFSSL_ALERT_HISTORY hist;
            wolfSSL_get_alert_history(ssl, &hist);
            int last_alert_code = hist.last_rx.code;
            const char* last_alert_type = wolfSSL_alert_type_string_long(last_alert_code);
            fprintf(stderr, "Last alert received: %d - %s\n", last_alert_code, last_alert_type);
        }
        result = -1;
        goto exit_freessl;
    }

    printf("CONNECTED\n");

    if (wolfSSL_get_error(ssl, ret) != SOCKET_PEER_CLOSED_E) {
        printf("  Closing connection...\n");
        ret = wolfSSL_shutdown(ssl);
        if (ret == SSL_SHUTDOWN_NOT_DONE) {
            ret = wolfSSL_shutdown(ssl);
        }
        if (ret != SSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_shutdown() failed with code %d (error %d)\n", ret, wolfSSL_get_error(ssl, ret));
            result = -1;
            goto exit_freessl;
        }
    }

exit_freessl:
    wolfSSL_free(ssl);

exit_closefd:
    close(sockfd);
    printf("  CLOSED\n");

exit:
    return result;
}
#endif

int main(int argc, char** argv)
{
 #if defined(WOLFSSL_ASYNC_CRYPT) && defined(HAVE_SNI) && defined(HAVE_ALPN) \
    && defined(WOLFSSL_NONBLOCK_OCSP) && defined(HAVE_CERTIFICATE_STATUS_REQUEST) \
    && defined(HAVE_CERTIFICATE_STATUS_REQUEST_V2)

    int err;
    int result = 0;
    WC_RNG rng;
    int asyncCryptDevId = INVALID_DEVID;
    pthread_t current_thread;
    WOLFSSL_METHOD *method = NULL;
    WOLFSSL_CTX *ctx = NULL;

    /* Check presence of sys_certs_file */
    if (access (sys_certs_file, F_OK) == -1 && argc == 1) {
        fprintf(stderr, "Default system cert file /etc/ssl/certs/ca-certificates.crt doesn't exist."
                " Please provide cert file path as show below.\n");
        fprintf(stderr, "./ocsp_nonblock_asynccrypt ../../mycerts/ca.crt\n");
        return -1;
    }
    /* Handle user provided certs file */
    else if (argc == 2) {
        if (access (argv[1], F_OK) == -1) {
            fprintf(stderr, "Provided cert file %s doesn't exist."
                    " Please provide a valid path.\n", argv[1]);
            return -1;
        }
        else {
            sys_certs_file = argv[1];
        }
    }

    wolfSSL_Debugging_ON();

    err = wolfSSL_Init();
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init() failed with code %d\n", err);
        return -1;
    }

    wolfSSL_load_error_strings();

#if defined(WOLFSSL_ASYNC_CRYPT)
    wolfAsync_HardwareStart();
    printf("WolfSSL AsyncCrypt Enabled\n");
#if defined(HAVE_INTEL_QA)
    printf("WolfSSL AsyncCrypt with IntelQA Mode\n");
#elif defined(WOLFSSL_ASYNC_CRYPT_SW)
    printf("WolfSSL AsyncCrypt with Simulation Mode\n");
#else
    #error Unknown HW Acceleration device
#endif

    current_thread = pthread_self();
    if (wolfAsync_DevOpenThread(&asyncCryptDevId, &current_thread) < 0) {
        fprintf(stderr, "Could not initialize AsyncCrypt device\n");
        return -1;
    }

#endif

    err = wc_InitRng(&rng);
    if (err != 0) {
        fprintf(stderr, "wc_InitRng() failed with code %d\n", err);
        result = -1;
        goto exit;
    }

    method = wolfSSLv23_client_method();
    if (method == NULL) {
        fprintf(stderr, "failed to get client method\n");
        result = -1;
        goto exit;
    }

    ctx = wolfSSL_CTX_new(method);
    if (ctx == NULL) {
        fprintf(stderr, "failed to create context object\n");
        result = -1;
        goto exit;
    }

    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_cb);

    err = wolfSSL_CTX_UseOCSPStapling(ctx, WOLFSSL_CSR_OCSP, WOLFSSL_CSR_OCSP_USE_NONCE);
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_UseOCSPStapling() returned %d\n", err);
        result = -1;
        goto exit;
    }

    err = wolfSSL_CTX_UseOCSPStaplingV2(ctx, WOLFSSL_CSR2_OCSP_MULTI, WOLFSSL_CSR2_OCSP_USE_NONCE);
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_UseOCSPStaplingV2() returned %d\n", err);
        result = -1;
        goto exit;
    }

    err = wolfSSL_CTX_EnableOCSP(ctx, WOLFSSL_OCSP_CHECKALL | WOLFSSL_OCSP_NO_NONCE);
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_EnableOCSP() returned %d\n", err);
        result = -1;
        goto exit;
    }

    err = wolfSSL_CTX_UnloadCAs(ctx);
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_UnloadCAs() returned %d\n", err);
        result = -1;
        goto exit;
    }

    err = wolfSSL_CTX_load_verify_locations(ctx, sys_certs_file, NULL);
    if (err != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_load_verify_locations() returned %d\n", err);
        result = -1;
        goto exit;
    }

#if defined(WOLFSSL_ASYNC_CRYPT)
    if (wolfSSL_CTX_UseAsync(ctx, asyncCryptDevId) != SSL_SUCCESS ) {
        fprintf(stderr, "Could not enable AsyncCrypt device on TLS context\n");
        result = -1;
        goto exit;
    }
#endif

    err = test_connect(ctx);
    if (err != 0) {
        fprintf(stderr, "test_connect() failed\n");
        fprintf(stderr, "CONNECT FAILED\n");
    }
    else {
        fprintf(stderr, "CONNECT PASSED\n");
    }

    wolfSSL_CTX_free(ctx);
    ctx = NULL;

exit:

    printf("\nDONE\n\n");


    wc_FreeRng(&rng);

#if defined(WOLFSSL_ASYNC_CRYPT)
    if (asyncCryptDevId != INVALID_DEVID) {
        wolfAsync_DevClose(&asyncCryptDevId);
    }

    wolfAsync_HardwareStop();
#endif

    wolfSSL_Cleanup();

    return result;
#else
    (void)argc;
    (void)argv;

    printf("Please compile wolfSSL with  ./configure --enable-asynccrypt --enable-sni" 
           " --enable-alpn --enable-ocspstapling --enable-ocspstapling2 --enable-opensslextra"
           " --enable-curve25519 CFLAGS=-DWOLFSSL_NONBLOCK_OCSP");
    return -1;
#endif
}
