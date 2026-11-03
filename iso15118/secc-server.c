/* secc-server.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

/* The SECC side of an ISO 15118 TLS session: the charging station.
 *
 * This is the TLS layer of Plug & Charge and nothing above it. The SECC
 * presents a chain that goes SECC leaf -> CPO Sub-CA 2 -> CPO Sub-CA 1, and
 * requires the vehicle to present a contract chain reaching the same V2G Root
 * CA. Mutual authentication is what -20 uses in place of the application-layer
 * signature that -2 relied on.
 *
 * Deliberately NOT here: EXI encoding, XML signatures, SECC Discovery
 * Protocol, the V2G message state machine. wolfSSL has no EXI codec and
 * should not grow one; faking those would teach the wrong shape. Once the
 * session is up this example exchanges a short marker so you can see data
 * flowing, and stops.
 *
 * Usage: ./secc-server [port]
 */

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Leaf first, then the sub-CAs. The V2G root is not sent: it is a locally
 * configured trust anchor at both ends, which is why three certificates go on
 * the wire for a four tier hierarchy. */
#define SECC_CHAIN "certs/secc-chain.pem"
#define SECC_KEY   "certs/secc.key"
#define V2G_ROOT   "certs/v2g-root.pem"

/* No registered port: a real SECC advertises one over SDP, which is out of
 * scope here. */
#define SECC_DEFAULT_PORT 15118

static volatile sig_atomic_t running = 1;

static void on_signal(int sig)
{
    (void)sig;
    running = 0;
}

/* Close the connection without destroying the last thing we said on it.
 *
 * Under TLS 1.3 the vehicle finishes its handshake before this end has looked
 * at its certificate, so a refused EVCC is typically already sending a
 * SessionSetupReq while the SECC is sending the fatal alert that explains the
 * refusal. Writing to a connection this process has closed earns a TCP RST,
 * and a RST makes the peer's stack throw away whatever it has received but not
 * yet read -- the alert included. The vehicle then sees a bare disconnect and
 * cannot tell a refusal from a broken link or a station that silently accepted
 * it. It is a race, so it shows up on a loaded CI runner and not on a
 * developer's loopback.
 *
 * Staying in a read loop until the peer closes keeps the socket alive long
 * enough to absorb that request and for the alert to be read, and leaves
 * close() sending a FIN. The timeout bounds a peer that neither reads nor
 * closes. */
static void drain_and_close(int fd)
{
    struct timeval tv;
    unsigned char sink[256];

    tv.tv_sec = 2;
    tv.tv_usec = 0;
    (void)setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const void*)&tv,
            (socklen_t)sizeof(tv));
    while (recv(fd, sink, sizeof(sink), 0) > 0) {
        /* Discarded: after a refusal there is no session to carry it, and in
         * the success path this is the peer's close_notify. */
    }
    close(fd);
}

int main(int argc, char** argv)
{
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    int sock = -1, conn = -1;
    struct sockaddr_in addr;
    int on = 1;
    int port = (argc > 1) ? atoi(argv[1]) : SECC_DEFAULT_PORT;
    int ret = 1;
    char buf[256];
    int n;

    /* sigaction without SA_RESTART, not signal(): a handler installed with
     * signal() gets SA_RESTART on most platforms, so accept() below would be
     * restarted after SIGINT instead of returning EINTR and the server could
     * not be interrupted while waiting for a connection. */
    {
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = on_signal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        (void)sigaction(SIGINT, &sa, NULL);
    }
    signal(SIGPIPE, SIG_IGN);

    wolfSSL_Init();

    /* TLS 1.3 only. ISO 15118-20 requires it, and offering anything older
     * would let a downgrade pick a suite the standard does not permit. */
    ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
    if (ctx == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new failed\n");
        goto cleanup;
    }

    /* Restrict to the suites ISO 15118-20 names. wolfSSL would otherwise
     * happily negotiate other perfectly good TLS 1.3 suites, which a
     * conformance test would flag. */
    if (wolfSSL_CTX_set_cipher_list(ctx,
                "TLS13-AES128-GCM-SHA256:TLS13-CHACHA20-POLY1305-SHA256")
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not restrict the cipher list\n");
        goto cleanup;
    }

    /* The chain file carries the leaf and both sub-CAs. */
    if (wolfSSL_CTX_use_certificate_chain_file(ctx, SECC_CHAIN)
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not load %s -- run ./generate_v2g_certs.sh\n",
                SECC_CHAIN);
        goto cleanup;
    }
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, SECC_KEY, WOLFSSL_FILETYPE_PEM)
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not load %s\n", SECC_KEY);
        goto cleanup;
    }

    /* One trust anchor for both directions: the V2G root signs the CPO
     * sub-CAs this station uses and the MO sub-CAs behind the vehicle's
     * contract certificate. */
    if (wolfSSL_CTX_load_verify_locations(ctx, V2G_ROOT, NULL)
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not load %s\n", V2G_ROOT);
        goto cleanup;
    }

    /* Mutual authentication. A vehicle with no contract certificate, or one
     * that does not chain to the V2G root, does not get a session. */
    wolfSSL_CTX_set_verify(ctx,
            WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        goto cleanup;
    }
    (void)setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short)port);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        goto cleanup;
    }
    if (listen(sock, 4) < 0) {
        perror("listen");
        goto cleanup;
    }

    printf("SECC listening on 127.0.0.1:%d\n", port);
    printf("  TLS 1.3 only, mutual authentication against the V2G root\n");
    printf("  chain sent: SECC leaf -> CPO Sub-CA 2 -> CPO Sub-CA 1\n");

    conn = accept(sock, NULL, NULL);
    if (conn < 0) {
        if (running) {
            perror("accept");
        }
        goto cleanup;
    }

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        goto cleanup;
    }
    wolfSSL_set_fd(ssl, conn);

    printf("\nEVCC connected\n");
    if (wolfSSL_accept(ssl) != WOLFSSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, 0);
        char ebuf[80];

        fprintf(stderr, "  handshake failed: %s\n",
                wolfSSL_ERR_error_string((unsigned long)err, ebuf));
        printf("\nsecc-server: REFUSED\n");
        ret = 0; /* a refusal is a valid outcome */
        goto cleanup;
    }

    printf("  TLS %s, %s\n", wolfSSL_get_version(ssl),
            wolfSSL_get_cipher(ssl));
    printf("  contract chain verified to the V2G root\n");

    /* Stand-in for the V2G session. A real SECC would now be exchanging
     * EXI-encoded messages; this just proves the channel carries data. */
    /* The marker exchange is the only evidence the channel carried data, so a
     * failed read or write is a failure of the run -- reporting PASS anyway
     * would claim an exchange that did not happen. */
    n = wolfSSL_read(ssl, buf, (int)sizeof(buf) - 1);
    if (n <= 0) {
        int err = wolfSSL_get_error(ssl, n);
        char ebuf[80];

        fprintf(stderr, "  no request from the EVCC: %s\n",
                wolfSSL_ERR_error_string((unsigned long)err, ebuf));
        printf("\nsecc-server: FAIL\n");
        goto cleanup;
    }

    buf[n] = '\0';
    printf("  <- %s\n", buf);

    if (wolfSSL_write(ssl, "SessionSetupRes: accepted",
                (int)strlen("SessionSetupRes: accepted"))
            != (int)strlen("SessionSetupRes: accepted")) {
        fprintf(stderr, "  could not answer the EVCC\n");
        printf("\nsecc-server: FAIL\n");
        goto cleanup;
    }
    printf("  -> SessionSetupRes: accepted\n");

    printf("\nsecc-server: PASS\n");
    ret = 0;

cleanup:
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (conn >= 0) {
        drain_and_close(conn);
    }
    if (sock >= 0) {
        close(sock);
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
    }
    wolfSSL_Cleanup();
    return ret;
}
