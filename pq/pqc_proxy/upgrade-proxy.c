/* upgrade-proxy.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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

/* upgrade-proxy: a TLS terminating crypto-proxy that *upgrades* legacy clients.
 *
 *   legacy-client =(legacy TLS 1.2)=> upgrade-proxy =(PQC TLS 1.3)=> pq-server
 *
 * This is the mirror image of pq-proxy. Here the proxy is:
 *   - a TLS *server* on the frontend, accepting a classical TLS 1.2 handshake
 *     from a legacy client that cannot speak PQC;
 *   - a TLS *client* on the backend, opening a quantum-safe (hybrid ML-KEM)
 *     TLS 1.3 connection to a modern origin server.
 *
 * It lets an un-upgradeable client still reach a service over a quantum-safe
 * link, with the post-quantum key exchange protecting the proxy-to-server leg.
 * Connections are handled one at a time to keep the example readable.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include "proxy-common.h"

#if defined(WOLFSSL_TLS13) && defined(WOLFSSL_HAVE_MLKEM)

static int mListenfd = SOCKET_INVALID;
static int mShutdown = 0;

static void sig_handler(const int sig)
{
    (void)sig;
    mShutdown = 1;
    if (mListenfd != SOCKET_INVALID) {
        close(mListenfd);
        mListenfd = SOCKET_INVALID;
    }
}

/* Open a TCP connection to the PQC origin server. */
static int connect_backend(const char* host, int port)
{
    int sockfd;
    struct sockaddr_in addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/* Pump all currently available plaintext from src to dst.
 * Returns 1 on success, 0 if the source closed, -1 on error. */
static int pump(WOLFSSL* src, WOLFSSL* dst)
{
    char buff[PROXY_BUFFER_SZ];
    int  n;

    do {
        n = wolfSSL_read(src, buff, sizeof(buff));
        if (n <= 0) {
            int err = wolfSSL_get_error(src, n);
            if (err == WOLFSSL_ERROR_WANT_READ ||
                    err == WOLFSSL_ERROR_WANT_WRITE)
                return 1;
            return (n == 0 || err == WOLFSSL_ERROR_ZERO_RETURN) ? 0 : -1;
        }

        if (wolfSSL_write(dst, buff, n) != n)
            return -1;

        /* wolfSSL may already hold further decrypted records that the socket
         * select() would not report; drain while data remains buffered. */
    } while (wolfSSL_pending(src) > 0);

    return 1;
}

/* Shuttle application data between the frontend and backend TLS sessions until
 * either side closes the connection. */
static void relay(WOLFSSL* front, int frontFd, WOLFSSL* back, int backFd)
{
    int maxFd = (frontFd > backFd ? frontFd : backFd) + 1;

    while (!mShutdown) {
        fd_set readfds;
        int rc;

        FD_ZERO(&readfds);
        FD_SET(frontFd, &readfds);
        FD_SET(backFd, &readfds);

        rc = select(maxFd, &readfds, NULL, NULL, NULL);
        if (rc < 0)
            break;

        if (FD_ISSET(frontFd, &readfds) || wolfSSL_pending(front) > 0) {
            if (pump(front, back) <= 0)
                break;
        }
        if (FD_ISSET(backFd, &readfds) || wolfSSL_pending(back) > 0) {
            if (pump(back, front) <= 0)
                break;
        }
    }
}

static void print_leg(const char* label, WOLFSSL* ssl)
{
    const char* group = wolfSSL_get_curve_name(ssl);
    printf("  %-9s %s  %s  kex=%s\n", label,
        wolfSSL_get_version(ssl), wolfSSL_get_cipher(ssl),
        group ? group : "(classical)");
}

/* Accept legacy clients on the frontend and bridge each one to a fresh
 * quantum-safe backend connection until a shutdown is requested. */
static int accept_loop(WOLFSSL_CTX* frontCtx, WOLFSSL_CTX* backCtx,
    const char* backHost, int backPort)
{
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);

    while (!mShutdown) {
        int      frontFd = SOCKET_INVALID;
        int      backFd  = SOCKET_INVALID;
        WOLFSSL* front   = NULL;
        WOLFSSL* back    = NULL;

        printf("\nWaiting for a client...\n");
        frontFd = accept(mListenfd, (struct sockaddr*)&clientAddr, &size);
        if (frontFd == -1) {
            if (mShutdown)
                break;
            fprintf(stderr, "ERROR: failed to accept\n");
            continue;
        }

        /* Terminate the legacy client connection. */
        if ((front = wolfSSL_new(frontCtx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create frontend WOLFSSL\n");
            goto conn_cleanup;
        }
        wolfSSL_set_fd(front, frontFd);
        if (wolfSSL_accept(front) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: frontend handshake failed: %d\n",
                wolfSSL_get_error(front, 0));
            goto conn_cleanup;
        }

        /* Open the quantum-safe connection to the origin. */
        backFd = connect_backend(backHost, backPort);
        if (backFd < 0) {
            fprintf(stderr, "ERROR: failed to reach backend %s:%d\n",
                backHost, backPort);
            goto conn_cleanup;
        }
        if ((back = wolfSSL_new(backCtx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create backend WOLFSSL\n");
            goto conn_cleanup;
        }
        wolfSSL_set_fd(back, backFd);
        /* Pre-generate the hybrid key share so the very first ClientHello to
         * the origin already carries the post-quantum group. */
        if (wolfSSL_UseKeyShare(back, PQC_GROUP) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to set backend key share\n");
            goto conn_cleanup;
        }
        if (wolfSSL_connect(back) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: backend handshake failed: %d\n",
                wolfSSL_get_error(back, 0));
            goto conn_cleanup;
        }

        printf("Bridging connection:\n");
        print_leg("frontend", front);
        print_leg("backend",  back);

        relay(front, frontFd, back, backFd);
        printf("Connection closed\n");

conn_cleanup:
        if (front) {
            wolfSSL_shutdown(front);
            wolfSSL_free(front);
        }
        if (back) {
            wolfSSL_shutdown(back);
            wolfSSL_free(back);
        }
        if (frontFd != SOCKET_INVALID)
            close(frontFd);
        if (backFd != SOCKET_INVALID)
            close(backFd);
    }

    return 0;
}

int main(int argc, char** argv)
{
    int ret = 0;
    int frontPort = UPGRADE_PORT;
    const char* backHost = PQSERVER_HOST;
    int backPort = PQSERVER_PORT;
    int on = 1;

    struct sockaddr_in servAddr;

    /* The backend group is restricted to a single post-quantum (hybrid) group
     * so that the proxy-to-server leg is always quantum-safe. */
    int backGroups[] = { PQC_GROUP };

    WOLFSSL_CTX* frontCtx = NULL;   /* frontend: legacy TLS 1.2 server */
    WOLFSSL_CTX* backCtx  = NULL;   /* backend:  PQC TLS 1.3 client */

    if (argc >= 2) frontPort = atoi(argv[1]);
    if (argc >= 3) backHost  = argv[2];
    if (argc >= 4) backPort  = atoi(argv[3]);
    if (argc > 4) {
        printf("usage: %s [<frontend port> [<backend host> "
            "[<backend port>]]]\n", argv[0]);
        printf("Defaults: %d %s %d\n", UPGRADE_PORT, PQSERVER_HOST,
            PQSERVER_PORT);
        return 0;
    }

    signal(SIGINT, sig_handler);

    wolfSSL_Init();

    /* ---- Frontend context: legacy classical TLS 1.2 server --------------- */
    if ((frontCtx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create frontend WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }
    if ((ret = wolfSSL_CTX_set_cipher_list(frontCtx, LEGACY_CIPHERS))
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set frontend cipher list\n");
        goto exit;
    }
    if ((ret = wolfSSL_CTX_use_certificate_file(frontCtx, LEGACY_CERT,
            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", LEGACY_CERT);
        goto exit;
    }
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(frontCtx, LEGACY_KEY,
            WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", LEGACY_KEY);
        goto exit;
    }

    /* ---- Backend context: post-quantum / hybrid TLS 1.3 client ----------- */
    if ((backCtx = wolfSSL_CTX_new(wolfTLSv1_3_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create backend WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }
    if ((ret = wolfSSL_CTX_load_verify_locations(backCtx, PQC_CA, NULL))
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s\n", PQC_CA);
        goto exit;
    }
    /* Only negotiate the hybrid PQC group with the origin. */
    if ((ret = wolfSSL_CTX_set_groups(backCtx, backGroups,
            sizeof(backGroups) / sizeof(backGroups[0]))) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set backend PQC group\n");
        goto exit;
    }

    /* ---- Listen on the frontend port ------------------------------------- */
    if ((mListenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }
    setsockopt(mListenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_port        = htons(frontPort);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(mListenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto exit;
    }
    if (listen(mListenfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }

    printf("PQC upgrade-proxy ready\n");
    printf("  frontend : 0.0.0.0:%d  TLS 1.2  %s\n",
        frontPort, LEGACY_CIPHERS);
    printf("  backend  : %s:%d  TLS 1.3  kex=%s  auth=%s\n",
        backHost, backPort, PQC_GROUP_NAME, PQC_AUTH);

    ret = accept_loop(frontCtx, backCtx, backHost, backPort);

    printf("Shutdown complete\n");

exit:
    if (mListenfd != SOCKET_INVALID)
        close(mListenfd);
    if (frontCtx)
        wolfSSL_CTX_free(frontCtx);
    if (backCtx)
        wolfSSL_CTX_free(backCtx);
    wolfSSL_Cleanup();

    return ret;
}

#else

int main(void)
{
    printf("This example requires TLS 1.3 and ML-KEM (Kyber).\n");
    printf("Configure wolfSSL like this:\n");
    printf("    ./configure --enable-mlkem --enable-tls13\n");
    printf("Add --enable-mldsa for post-quantum authentication too.\n");
    return 0;
}

#endif /* WOLFSSL_TLS13 && WOLFSSL_HAVE_MLKEM */
