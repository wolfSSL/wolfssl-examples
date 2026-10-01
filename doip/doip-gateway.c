/* doip-gateway.c
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

/* The vehicle side of DoIP over TLS: a DoIP entity that accepts a TLS 1.3
 * connection on TCP_DATA_TLS, activates routing, and answers a UDS request.
 *
 * The security-relevant part is not the framing, it is who is allowed to send
 * diagnostics at all. They can unlock an ECU, reflash it and read out personal
 * data, so this entity requires a client certificate.
 *
 * Two gates, and the example shows both:
 *
 *   the handshake      no client certificate, no connection at all
 *   routing activation a certificate is not the same as authorisation for a
 *                      given activation type, so the entity can still answer
 *                      DOIP_RA_MISSING_AUTH. Run with --deny-auth to see it.
 *
 * Usage: ./doip-gateway [port] [--deny-auth]
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

#include "doip.h"

#define CERT_FILE "../certs/server-ecc.pem"
#define KEY_FILE  "../certs/ecc-key.pem"
#define CA_FILE   "../certs/client-ecc-cert.pem"

/* The VIN this ECU reports for UDS ReadDataByIdentifier of DID 0xF190. */
#define VIN_STRING "WOLFSSL0000000001"

static volatile sig_atomic_t running = 1;

static void on_signal(int sig)
{
    (void)sig;
    running = 0;
}

/* Close the connection without destroying the last thing we said on it.
 *
 * A tester refused at the handshake is told so by a fatal alert, and under TLS
 * 1.3 it has already finished its own handshake by then -- so it is typically
 * sending a routing activation request while that alert is on the way. Writing
 * to a connection this process has closed earns a TCP RST, and a RST makes the
 * peer's stack discard what it has received but not yet read, the alert
 * included. The tester then cannot tell a refusal from a broken link. It is a
 * race, so it appears on a loaded CI runner rather than on a developer's
 * loopback.
 *
 * Reading until the peer closes keeps the socket alive long enough to absorb
 * that request and for the alert to be read, and leaves close() sending a FIN.
 * The timeout bounds a peer that neither reads nor closes. */
static void drain_and_close(int fd)
{
    struct timeval tv;
    unsigned char sink[256];

    tv.tv_sec = 2;
    tv.tv_usec = 0;
    (void)setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const void*)&tv,
            (socklen_t)sizeof(tv));
    while (recv(fd, sink, sizeof(sink), 0) > 0) {
        /* Discarded: there is no session left to carry it, and in the served
         * case this is the tester's close_notify. */
    }
    close(fd);
}

/* Reads exactly len bytes. Returns 0 on success, -1 on error or close. */
static int read_exact(WOLFSSL* ssl, unsigned char* buf, size_t len)
{
    size_t got = 0;

    while (got < len) {
        int n = wolfSSL_read(ssl, buf + got, (int)(len - got));

        if (n <= 0) {
            return -1;
        }
        got += (size_t)n;
    }
    return 0;
}

/* Sends a DoIP message. Returns 0 on success. */
static int send_message(WOLFSSL* ssl, unsigned short payloadType,
        const unsigned char* payload, unsigned int payloadLen)
{
    unsigned char out[DOIP_HEADER_LEN + DOIP_MAX_PAYLOAD];

    if (payloadLen > DOIP_MAX_PAYLOAD) {
        return -1;
    }
    doip_write_header(out, payloadType, payloadLen);
    if (payloadLen > 0) {
        memcpy(out + DOIP_HEADER_LEN, payload, payloadLen);
    }

    if (wolfSSL_write(ssl, out, (int)(DOIP_HEADER_LEN + payloadLen)) !=
            (int)(DOIP_HEADER_LEN + payloadLen)) {
        return -1;
    }
    printf("  -> %-32s (%u byte payload)\n", doip_payload_name(payloadType),
            payloadLen);
    return 0;
}

/* Routing activation response: tester address, entity address, code, then
 * four reserved bytes. */
static int send_activation_response(WOLFSSL* ssl, unsigned short testerAddr,
        unsigned char code)
{
    unsigned char p[9];

    p[0] = (unsigned char)((testerAddr >> 8) & 0xFF);
    p[1] = (unsigned char)(testerAddr & 0xFF);
    p[2] = (unsigned char)((DOIP_GATEWAY_ADDRESS >> 8) & 0xFF);
    p[3] = (unsigned char)(DOIP_GATEWAY_ADDRESS & 0xFF);
    p[4] = code;
    memset(p + 5, 0, 4); /* reserved by ISO */

    printf("  activation %s: %s\n",
            (code == DOIP_RA_SUCCESS) ? "granted" : "REFUSED",
            doip_ra_reason(code));
    return send_message(ssl, DOIP_PT_ROUTING_ACTIVATION_RES, p, sizeof(p));
}

/* A diagnostic message ack or nack (0x8002/0x8003) carries the SOURCE address
 * of the entity sending it -- this gateway -- and then the TARGET address it
 * is replying to, which is the tester. That is the same order as the UDS
 * response below, and the reverse of the request's own SA/TA pair, so the
 * parameters are named for what goes on the wire rather than for the request
 * they came from. */
static int send_diag_ack(WOLFSSL* ssl, unsigned short source,
        unsigned short target, unsigned char code, int positive)
{
    unsigned char p[5];

    p[0] = (unsigned char)((source >> 8) & 0xFF);
    p[1] = (unsigned char)(source & 0xFF);
    p[2] = (unsigned char)((target >> 8) & 0xFF);
    p[3] = (unsigned char)(target & 0xFF);
    p[4] = code;

    return send_message(ssl, positive ? DOIP_PT_DIAG_MESSAGE_ACK :
            DOIP_PT_DIAG_MESSAGE_NACK, p, sizeof(p));
}

/* Answers one UDS request. Returns the response length written to 'res'.
 *
 * Only ReadDataByIdentifier (0x22) for DID 0xF190 (VIN) is implemented; every
 * other service gets the standard negative response. That is enough to show
 * the DoIP layer carrying a real payload without becoming a UDS stack. */
static int handle_uds(const unsigned char* req, unsigned int reqLen,
        unsigned char* res)
{
    if (reqLen >= 3 && req[0] == 0x22 && req[1] == 0xF1 && req[2] == 0x90) {
        size_t vinLen = strlen(VIN_STRING);

        res[0] = 0x62;  /* positive response to 0x22 */
        res[1] = 0xF1;
        res[2] = 0x90;
        memcpy(res + 3, VIN_STRING, vinLen);
        printf("  UDS 0x22 F190 (read VIN) -> %s\n", VIN_STRING);
        return (int)(3 + vinLen);
    }

    /* 0x7F <service> 0x11 serviceNotSupported */
    res[0] = 0x7F;
    res[1] = (reqLen > 0) ? req[0] : 0x00;
    res[2] = 0x11;
    printf("  UDS service 0x%02X not supported\n", res[1]);
    return 3;
}

/* Drives one accepted connection to completion. */
static void serve(WOLFSSL* ssl, int authenticated)
{
    unsigned char header[DOIP_HEADER_LEN];
    unsigned char payload[DOIP_MAX_PAYLOAD];
    int activated = 0;
    unsigned short testerAddr = 0;

    while (running) {
        /* Initialized even though doip_parse_header() writes them on success:
         * a header it rejects leaves them alone, and nothing should be able to
         * reach a read with an indeterminate length. */
        unsigned short type = 0;
        unsigned int len = 0;
        unsigned char nack = 0;

        if (read_exact(ssl, header, DOIP_HEADER_LEN) != 0) {
            printf("  peer closed the connection\n");
            return;
        }

        if (doip_parse_header(header, &type, &len, &nack) != 0) {
            printf("  <- malformed header (nack 0x%02X)\n", nack);
            (void)send_message(ssl, DOIP_PT_GENERIC_NACK, &nack, 1);
            return; /* the standard closes the socket after this */
        }

        if (len > 0 && read_exact(ssl, payload, len) != 0) {
            printf("  truncated payload\n");
            return;
        }

        printf("  <- %-32s (%u byte payload)\n", doip_payload_name(type), len);

        switch (type) {

        case DOIP_PT_ROUTING_ACTIVATION_REQ:
            /* source address (2) | activation type (1) | reserved (4) */
            if (len < 7) {
                unsigned char nack = DOIP_NACK_INVALID_LENGTH;
                (void)send_message(ssl, DOIP_PT_GENERIC_NACK, &nack, 1);
                return;
            }
            testerAddr = (unsigned short)((payload[0] << 8) | payload[1]);
            printf("  tester address 0x%04X, activation type 0x%02X\n",
                    testerAddr, payload[2]);

            if (!authenticated) {
                /* A trusted certificate is not the same as authorisation for
                 * this activation type. Refusing here, after a successful
                 * handshake, is the second gate. */
                (void)send_activation_response(ssl, testerAddr,
                        DOIP_RA_MISSING_AUTH);
                return;
            }
            if (payload[2] != DOIP_ACTIVATION_DEFAULT &&
                    payload[2] != DOIP_ACTIVATION_WWH_OBD) {
                (void)send_activation_response(ssl, testerAddr,
                        DOIP_RA_UNSUPPORTED_TYPE);
                return;
            }
            if (send_activation_response(ssl, testerAddr,
                        DOIP_RA_SUCCESS) != 0) {
                return;
            }
            activated = 1;
            break;

        case DOIP_PT_DIAG_MESSAGE: {
            unsigned short sa, ta;
            unsigned char res[DOIP_MAX_PAYLOAD];
            int resLen;

            if (len < 4) {
                unsigned char nack = DOIP_NACK_INVALID_LENGTH;
                (void)send_message(ssl, DOIP_PT_GENERIC_NACK, &nack, 1);
                return;
            }
            sa = (unsigned short)((payload[0] << 8) | payload[1]);
            ta = (unsigned short)((payload[2] << 8) | payload[3]);

            if (!activated) {
                /* Diagnostics before routing activation are not answered. */
                printf("  diagnostics before activation, refusing\n");
                (void)send_diag_ack(ssl, DOIP_GATEWAY_ADDRESS, sa,
                        DOIP_DIAG_NACK_INVALID_SA, 0);
                return;
            }
            if (sa != testerAddr) {
                (void)send_diag_ack(ssl, DOIP_GATEWAY_ADDRESS, sa,
                        DOIP_DIAG_NACK_INVALID_SA, 0);
                break;
            }
            if (ta != DOIP_GATEWAY_ADDRESS) {
                (void)send_diag_ack(ssl, DOIP_GATEWAY_ADDRESS, sa,
                        DOIP_DIAG_NACK_UNKNOWN_TA, 0);
                break;
            }

            /* Ack first, then the UDS response as its own message: two
             * separate DoIP messages, which is what a tester expects. */
            if (send_diag_ack(ssl, DOIP_GATEWAY_ADDRESS, sa,
                        DOIP_DIAG_ACK_OK, 1) != 0) {
                return;
            }

            resLen = handle_uds(payload + 4, len - 4, res + 4);
            res[0] = (unsigned char)((DOIP_GATEWAY_ADDRESS >> 8) & 0xFF);
            res[1] = (unsigned char)(DOIP_GATEWAY_ADDRESS & 0xFF);
            res[2] = (unsigned char)((sa >> 8) & 0xFF);
            res[3] = (unsigned char)(sa & 0xFF);
            if (send_message(ssl, DOIP_PT_DIAG_MESSAGE, res,
                        (unsigned int)(4 + resLen)) != 0) {
                return;
            }
            break;
        }

        case DOIP_PT_ALIVE_CHECK_REQ: {
            /* Note the direction. In ISO 13400-2 the DoIP ENTITY -- this
             * gateway -- sends the alive check request, and the tester
             * answers, which is how the entity decides whether to reclaim a
             * socket whose tester has gone away. A request arriving here is
             * therefore the reverse of the specified flow.
             *
             * Answered anyway, with the registered tester address this socket
             * belongs to, because a tester probing liveness is harmless and
             * refusing it would teach less than this comment does. A gateway
             * that needs the real thing sends the request itself on its
             * inactivity timer, which this example has no event loop for. */
            unsigned char p[2];

            p[0] = (unsigned char)((testerAddr >> 8) & 0xFF);
            p[1] = (unsigned char)(testerAddr & 0xFF);
            (void)send_message(ssl, DOIP_PT_ALIVE_CHECK_RES, p, sizeof(p));
            break;
        }

        default: {
            unsigned char nack = DOIP_NACK_UNKNOWN_TYPE;

            (void)send_message(ssl, DOIP_PT_GENERIC_NACK, &nack, 1);
            break;
        }
        }
    }
}

int main(int argc, char** argv)
{
    WOLFSSL_CTX* ctx = NULL;
    int sock = -1;
    struct sockaddr_in addr;
    int on = 1;
    int port = DOIP_PORT_TCP_DATA_TLS;
    int denyAuth = 0;
    int i;
    int ret = 1;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--deny-auth") == 0) {
            denyAuth = 1;
        }
        else {
            port = atoi(argv[i]);
        }
    }

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

    /* TLS 1.3 only. A diagnostic interface has no reason to offer anything
     * older: both ends are in the same vehicle programme. */
    ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
    if (ctx == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new failed\n");
        goto cleanup;
    }

    if (wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, WOLFSSL_FILETYPE_PEM)
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not load %s\n", CERT_FILE);
        goto cleanup;
    }
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, WOLFSSL_FILETYPE_PEM)
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not load %s\n", KEY_FILE);
        goto cleanup;
    }

    /* Require a client certificate. Routing activation is refused without
     * one, so this is what gates diagnostic access. */
    if (wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL)
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not load %s\n", CA_FILE);
        goto cleanup;
    }
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

    printf("DoIP gateway 0x%04X listening on 127.0.0.1:%d (TLS 1.3)\n",
            DOIP_GATEWAY_ADDRESS, port);
    printf("client certificate required by the handshake\n");
    if (denyAuth) {
        printf("--deny-auth: routing activation will be refused with "
               "DOIP_RA_MISSING_AUTH\n");
    }

    while (running) {
        WOLFSSL* ssl;
        int conn = accept(sock, NULL, NULL);

        if (conn < 0) {
            if (running) {
                perror("accept");
            }
            break;
        }

        ssl = wolfSSL_new(ctx);
        if (ssl == NULL) {
            close(conn);
            continue;
        }
        wolfSSL_set_fd(ssl, conn);

        printf("\nconnection\n");
        if (wolfSSL_accept(ssl) != WOLFSSL_SUCCESS) {
            int err = wolfSSL_get_error(ssl, 0);
            char buf[80];

            /* A tester with no certificate never gets to send a DoIP byte.
             * The handshake is the first gate; routing activation is the
             * second, for the case where a certificate is present but the
             * policy still says no. */
            fprintf(stderr, "  handshake failed: %s\n",
                    wolfSSL_ERR_error_string((unsigned long)err, buf));
        }
        else {
            printf("  TLS %s, %s\n", wolfSSL_get_version(ssl),
                    wolfSSL_get_cipher(ssl));
            /* WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT means a completed handshake
             * already proves the tester presented a certificate this entity
             * trusts -- no need to inspect it, which would pull in
             * KEEP_PEER_CERT or the OpenSSL compatibility layer. */
            printf("  tester authenticated by the handshake\n");
            serve(ssl, !denyAuth);
        }

        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
        drain_and_close(conn);

        /* One connection is enough for the example to be checkable. */
        break;
    }

    ret = 0;
    printf("\ndoip-gateway: done\n");

cleanup:
    if (sock >= 0) {
        close(sock);
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
    }
    wolfSSL_Cleanup();
    return ret;
}
