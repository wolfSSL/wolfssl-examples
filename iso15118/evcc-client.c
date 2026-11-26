/* evcc-client.c
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

/* The EVCC side of an ISO 15118 TLS session: the vehicle.
 *
 * Presents the contract certificate chain -- contract leaf -> MO Sub-CA 2 ->
 * MO Sub-CA 1 -- and verifies the station's chain against the same V2G Root
 * CA. Under -20 that mutual authentication is what proves the charging
 * contract, replacing the application-layer signature -2 used.
 *
 * Run with --no-contract to connect without one, which the SECC refuses.
 *
 * Usage: ./evcc-client [port] [--no-contract]
 */

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CONTRACT_CHAIN "certs/contract-chain.pem"
#define CONTRACT_KEY   "certs/contract.key"
#define V2G_ROOT       "certs/v2g-root.pem"

#define SECC_DEFAULT_PORT 15118

/* Names for the handful of alerts a refusal actually arrives as. wolfSSL has
 * wolfSSL_alert_type_string_long(), but only under OPENSSL_EXTRA, and this
 * example builds against a default library. */
static const char* alert_name(int code)
{
    switch (code) {
        case close_notify:          return "close notify";
        case handshake_failure:     return "handshake failure";
        case bad_certificate:       return "bad certificate";
        case certificate_expired:   return "certificate expired";
        case certificate_unknown:   return "certificate unknown";
        case unknown_ca:            return "unknown CA";
        case access_denied:         return "access denied";
        case protocol_version:      return "protocol version";
        case insufficient_security: return "insufficient security";
        case certificate_required:  return "certificate required";
        default:                    return "alert";
    }
}

/* Did the SECC refuse us, or did we fail on our own?
 *
 * A station that requires a contract certificate sends a fatal alert. An
 * expired SECC leaf in certs/ (they are issued for 60 days), an untrusted
 * chain or a name mismatch is the vehicle rejecting the station -- a security
 * failure, not a refusal. Reporting both the same way, as this example used
 * to, made the refusal check unable to fail.
 *
 * Returns 1 and sets *name when a fatal alert arrived from the SECC. */
static int peer_refused(WOLFSSL* ssl, const char** name)
{
    WOLFSSL_ALERT_HISTORY h;

    XMEMSET(&h, 0, sizeof(h));
    if (wolfSSL_get_alert_history(ssl, &h) != WOLFSSL_SUCCESS) {
        return 0;
    }
    /* Fatal only. close_notify is an alert as well, at warning level, so a
     * peer that completes the handshake and then simply closes would
     * otherwise be reported as a refusal and exit 0 -- and the refusal check
     * would pass without the SECC having enforced anything. */
    if (h.last_rx.code == invalid_alert || h.last_rx.level != alert_fatal) {
        return 0;
    }
    *name = alert_name(h.last_rx.code);
    return 1;
}

int main(int argc, char** argv)
{
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    int sock = -1;
    struct sockaddr_in addr;
    int port = SECC_DEFAULT_PORT;
    int useContract = 1;
    int i, ret = 1;
    char buf[256];
    int n;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-contract") == 0) {
            useContract = 0;
        }
        else {
            port = atoi(argv[i]);
        }
    }

    signal(SIGPIPE, SIG_IGN);
    wolfSSL_Init();

    ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
    if (ctx == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new failed\n");
        goto cleanup;
    }

    /* The same two suites the SECC allows. Restricting on both ends is what
     * makes a conformance run reproducible. */
    if (wolfSSL_CTX_set_cipher_list(ctx,
                "TLS13-AES128-GCM-SHA256:TLS13-CHACHA20-POLY1305-SHA256")
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not restrict the cipher list\n");
        goto cleanup;
    }

    /* One trust anchor verifies the station's CPO chain. */
    if (wolfSSL_CTX_load_verify_locations(ctx, V2G_ROOT, NULL)
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not load %s -- run ./generate_v2g_certs.sh\n",
                V2G_ROOT);
        goto cleanup;
    }
    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, NULL);

    if (useContract) {
        if (wolfSSL_CTX_use_certificate_chain_file(ctx, CONTRACT_CHAIN)
                != WOLFSSL_SUCCESS) {
            fprintf(stderr, "could not load %s\n", CONTRACT_CHAIN);
            goto cleanup;
        }
        if (wolfSSL_CTX_use_PrivateKey_file(ctx, CONTRACT_KEY,
                    WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "could not load %s\n", CONTRACT_KEY);
            goto cleanup;
        }
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        goto cleanup;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short)port);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        goto cleanup;
    }

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        goto cleanup;
    }
    wolfSSL_set_fd(ssl, sock);

    /* Check the station's identity, not just that its chain is valid. The
     * SECC leaf carries secc.charge.example as a SAN. */
    if (wolfSSL_check_domain_name(ssl, "secc.charge.example")
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not set the expected SECC name\n");
        goto cleanup;
    }

    printf("EVCC -> 127.0.0.1:%d%s\n", port,
            useContract ? "" : "  (no contract certificate)");
    if (useContract) {
        printf("  chain sent: contract leaf -> MO Sub-CA 2 -> "
               "MO Sub-CA 1\n");
    }

    if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, 0);
        char ebuf[80];
        const char* alert = NULL;

        fprintf(stderr, "  handshake failed: %s\n",
                wolfSSL_ERR_error_string((unsigned long)err, ebuf));
        if (peer_refused(ssl, &alert)) {
            printf("\nevcc-client: REFUSED by the SECC (%s)\n", alert);
            ret = 0;
        }
        else {
            printf("\nevcc-client: FAIL (the vehicle rejected the station,"
                   " or the transport failed)\n");
        }
        goto cleanup;
    }

    printf("  TLS %s, %s\n", wolfSSL_get_version(ssl),
            wolfSSL_get_cipher(ssl));
    printf("  SECC chain verified to the V2G root, name matched\n");

    /* Stand-in for the V2G session. */
    (void)wolfSSL_write(ssl, "SessionSetupReq: DE-MO-C0123456789-3",
            (int)strlen("SessionSetupReq: DE-MO-C0123456789-3"));
    printf("  -> SessionSetupReq: DE-MO-C0123456789-3\n");

    n = wolfSSL_read(ssl, buf, (int)sizeof(buf) - 1);
    if (n <= 0) {
        int err = wolfSSL_get_error(ssl, 0);
        char ebuf[80];

        /* Under TLS 1.3 the client finishes its handshake before the server
         * has validated the client certificate, so a vehicle with no contract
         * certificate learns of the refusal here rather than in
         * wolfSSL_connect() -- but only when an alert actually arrived. */
        const char* alert = NULL;

        fprintf(stderr, "  no reply: %s\n",
                wolfSSL_ERR_error_string((unsigned long)err, ebuf));
        if (peer_refused(ssl, &alert)) {
            printf("\nevcc-client: REFUSED by the SECC after the handshake"
                   " (%s)\n", alert);
            ret = 0;
        }
        else {
            /* Either the station answered nothing at all, or it sent the
             * alert and then reset the connection, which makes this end
             * discard the alert before it is read. Both are failures here:
             * without the alert there is no evidence the SECC enforced
             * anything. See drain_and_close() in secc-server.c. */
            printf("\nevcc-client: FAIL (no reply and no alert from the"
                   " SECC)\n");
        }
        goto cleanup;
    }
    buf[n] = '\0';
    printf("  <- %s\n", buf);

    printf("\nevcc-client: PASS\n");
    ret = 0;

cleanup:
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
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
