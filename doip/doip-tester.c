/* doip-tester.c
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

/* The tester side of DoIP over TLS: connect with TLS 1.3, activate routing,
 * then read the VIN with a UDS request carried in a diagnostic message.
 *
 * Run with --no-cert to present no client certificate. Note what happens:
 * under TLS 1.3 wolfSSL_connect() still SUCCEEDS, because the client
 * completes its half of the handshake before the server has validated the
 * client certificate. The refusal arrives as an alert on the first read, so
 * the tester learns of it when it waits for the routing activation response.
 * A TLS 1.2 tester would have failed in wolfSSL_connect() instead -- a
 * difference worth knowing when moving one to 1.3.
 *
 * Run the gateway with --deny-auth for the other refusal: a tester that is
 * authenticated but not authorised, refused at routing activation with
 * DOIP_RA_MISSING_AUTH.
 *
 * Usage: ./doip-tester [port] [--no-cert]
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

#include "doip.h"

#define CERT_FILE "../certs/client-ecc-cert.pem"
#define KEY_FILE  "../certs/ecc-client-key.pem"
#define CA_FILE   "../certs/ca-ecc-cert.pem"

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

/* Names for the handful of alerts a refusal actually arrives as. wolfSSL has
 * wolfSSL_alert_type_string_long(), but only under OPENSSL_EXTRA, and these
 * examples build against a default library. */
static const char* alert_name(int code)
{
    switch (code) {
        case close_notify:          return "close notify";
        case handshake_failure:     return "handshake failure";
        case bad_certificate:       return "bad certificate";
        case unsupported_certificate: return "unsupported certificate";
        case certificate_revoked:   return "certificate revoked";
        case certificate_expired:   return "certificate expired";
        case certificate_unknown:   return "certificate unknown";
        case unknown_ca:            return "unknown CA";
        case access_denied:         return "access denied";
        case decode_error:          return "decode error";
        case protocol_version:      return "protocol version";
        case insufficient_security: return "insufficient security";
        case certificate_required:  return "certificate required";
        default:                    return "alert";
    }
}

/* Did the PEER refuse us, or did we fail on our own?
 *
 * The difference decides whether a run is a valid outcome or a bug: a peer
 * that rejects an unauthenticated client sends a fatal alert, whereas an
 * expired or untrusted certificate on our side, a name mismatch or a dropped
 * connection is our own failure. Reporting both as a refusal, which these
 * examples used to do, made the refusal checks unable to fail -- the 60 day
 * leaf in certs/ expiring would have read as a successful refusal.
 *
 * Returns 1 and sets *name when a fatal alert arrived from the peer. */
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
     * would pass without the gateway having enforced anything. */
    if (h.last_rx.code == invalid_alert || h.last_rx.level != alert_fatal) {
        return 0;
    }
    *name = alert_name(h.last_rx.code);
    return 1;
}

/* Checks doip_parse_header()'s contract without needing a gateway.
 *
 * This is here because the end-to-end malformed-header case cannot tell the
 * old return convention from the new one: DOIP_NACK_INCORRECT_PATTERN is
 * 0x00, and a gateway that read that as success ended up with payload type 0,
 * which is itself the generic NACK type -- so it answered with a NACK anyway,
 * by luck. These assertions fail on the old convention.
 *
 * Returns 0 if every case passes. */
static int parse_selftest(void)
{
    unsigned char hdr[DOIP_HEADER_LEN];
    unsigned short type;
    unsigned int len;
    unsigned char nack;
    int fails = 0;

    /* a good header round-trips */
    doip_write_header(hdr, DOIP_PT_DIAG_MESSAGE, 42);
    type = 0xFFFF; len = 0xFFFFFFFFu; nack = 0xFF;
    if (doip_parse_header(hdr, &type, &len, &nack) != 0 ||
            type != DOIP_PT_DIAG_MESSAGE || len != 42) {
        printf("  selftest: a good header was rejected\n");
        fails++;
    }

    /* a wrong inverse version byte is rejected, not reported as success, and
     * the out-parameters are left alone */
    doip_write_header(hdr, DOIP_PT_DIAG_MESSAGE, 42);
    hdr[1] = hdr[0];
    type = 0xFFFF; len = 0xFFFFFFFFu; nack = 0xFF;
    if (doip_parse_header(hdr, &type, &len, &nack) == 0) {
        printf("  selftest: a bad inverse version byte was accepted\n");
        fails++;
    }
    else if (nack != DOIP_NACK_INCORRECT_PATTERN) {
        printf("  selftest: wrong nack for a bad pattern (0x%02X)\n", nack);
        fails++;
    }
    else if (type != 0xFFFF || len != 0xFFFFFFFFu) {
        printf("  selftest: a rejected header wrote the out-parameters\n");
        fails++;
    }

    /* a well-formed pattern for a different revision is rejected: the inverse
     * byte matches, but the version is not the one this build speaks */
    doip_write_header(hdr, DOIP_PT_DIAG_MESSAGE, 42);
    hdr[0] = (unsigned char)(DOIP_PROTOCOL_VERSION + 1);
    hdr[1] = (unsigned char)(~hdr[0] & 0xFF);
    nack = 0xFF;
    if (doip_parse_header(hdr, &type, &len, &nack) == 0) {
        printf("  selftest: a foreign protocol version was accepted\n");
        fails++;
    }
    else if (nack != DOIP_NACK_INCORRECT_PATTERN) {
        printf("  selftest: wrong nack for a foreign version (0x%02X)\n",
                nack);
        fails++;
    }

    /* an oversized payload length is rejected with its own code */
    doip_write_header(hdr, DOIP_PT_DIAG_MESSAGE, DOIP_MAX_PAYLOAD + 1);
    nack = 0xFF;
    if (doip_parse_header(hdr, &type, &len, &nack) == 0) {
        printf("  selftest: an oversized length was accepted\n");
        fails++;
    }
    else if (nack != DOIP_NACK_MESSAGE_TOO_LARGE) {
        printf("  selftest: wrong nack for an oversized length (0x%02X)\n",
                nack);
        fails++;
    }

    if (fails == 0) {
        printf("  header parser contract holds\n");
    }
    return (fails == 0) ? 0 : -1;
}

/* Reads one DoIP message. Returns 0 on success. */
static int recv_message(WOLFSSL* ssl, unsigned short* type,
        unsigned char* payload, unsigned int* len)
{
    unsigned char header[DOIP_HEADER_LEN];
    unsigned char nack = 0;

    if (read_exact(ssl, header, DOIP_HEADER_LEN) != 0) {
        int err = wolfSSL_get_error(ssl, 0);
        char buf[80];

        /* In TLS 1.3 the client finishes its handshake before the server has
         * validated the client certificate, so a rejected tester sees
         * wolfSSL_connect() succeed and only learns of the refusal here, as
         * an alert on the first read. Under TLS 1.2 it would have failed in
         * wolfSSL_connect(). Worth knowing when porting a 1.2 tester. */
        fprintf(stderr, "  no reply: %s\n",
                wolfSSL_ERR_error_string((unsigned long)err, buf));
        return -1;
    }
    if (doip_parse_header(header, type, len, &nack) != 0) {
        fprintf(stderr, "  malformed header from the gateway (nack 0x%02X)\n",
                nack);
        return -1;
    }
    if (*len > 0 && read_exact(ssl, payload, *len) != 0) {
        return -1;
    }
    printf("  <- %-32s (%u byte payload)\n", doip_payload_name(*type), *len);
    return 0;
}

int main(int argc, char** argv)
{
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    int sock = -1;
    struct sockaddr_in addr;
    int port = DOIP_PORT_TCP_DATA_TLS;
    int useCert = 1;
    int malformed = 0;
    int i, ret = 1;
    unsigned char payload[DOIP_MAX_PAYLOAD];
    unsigned char req[16];
    unsigned short type = 0;
    unsigned int len = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-cert") == 0) {
            useCert = 0;
        }
        else if (strcmp(argv[i], "--malformed") == 0) {
            malformed = 1;
        }
        else if (strcmp(argv[i], "--selftest") == 0) {
            printf("DoIP header parser selftest\n");
            if (parse_selftest() != 0) {
                printf("\ndoip-tester: FAIL\n");
                return 1;
            }
            printf("\ndoip-tester: PASS (parser selftest)\n");
            return 0;
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

    if (wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL)
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not load %s\n", CA_FILE);
        goto cleanup;
    }

    if (useCert) {
        if (wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE,
                    WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "could not load %s\n", CERT_FILE);
            goto cleanup;
        }
        if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE,
                    WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "could not load %s\n", KEY_FILE);
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

    /* A trusted chain is not an identity. Without this, any certificate the
     * loaded CA signed can answer as the gateway -- including another ECU's.
     * The bundled server-ecc.pem is issued to www.wolfssl.com; a real tester
     * checks the name or SAN its OEM puts in the gateway certificate. */
    if (wolfSSL_check_domain_name(ssl, "www.wolfssl.com") != WOLFSSL_SUCCESS) {
        fprintf(stderr, "could not set the expected gateway name\n");
        goto cleanup;
    }

    printf("DoIP tester 0x%04X -> 127.0.0.1:%d%s\n", DOIP_TESTER_ADDRESS, port,
            useCert ? "" : "  (no client certificate)");

    if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, 0);
        char buf[80];
        const char* alert = NULL;

        fprintf(stderr, "handshake failed: %s\n",
                wolfSSL_ERR_error_string((unsigned long)err, buf));
        if (peer_refused(ssl, &alert)) {
            /* The gateway rejected us and said so: a correct outcome for this
             * example rather than a bug. */
            printf("\ndoip-tester: REFUSED at the handshake (%s)\n", alert);
            ret = 0;
        }
        else {
            /* Our own failure -- an untrusted or expired ../certs/ CA, or the
             * transport. Not a refusal, and not a pass. */
            printf("\ndoip-tester: FAIL (local handshake error, no alert "
                   "from the gateway)\n");
        }
        goto cleanup;
    }
    printf("  TLS %s, %s\n", wolfSSL_get_version(ssl),
            wolfSSL_get_cipher(ssl));

    /* --malformed: a header whose inverse version byte does not match, which
     * is what a stream that is not DoIP, or one that has lost sync, looks
     * like. The gateway must answer with a generic NACK and close.
     *
     * This is a regression test for a return convention that used to make
     * that header indistinguishable from a good one: the NACK code for
     * "incorrect pattern format" is 0x00, which was also the success value,
     * so the gateway went on to read a payload length it had never been
     * given. */
    if (malformed) {
        unsigned char bad[DOIP_HEADER_LEN];

        doip_write_header(bad, DOIP_PT_ROUTING_ACTIVATION_REQ, 0);
        bad[1] = bad[0]; /* should be the inverse */

        printf("  -> deliberately malformed header\n");
        if (wolfSSL_write(ssl, bad, DOIP_HEADER_LEN) != DOIP_HEADER_LEN) {
            fprintf(stderr, "  could not send the malformed header\n");
            goto cleanup;
        }

        if (recv_message(ssl, &type, payload, &len) != 0) {
            /* The gateway closing without a NACK would also be safe, but the
             * standard asks for the NACK, so insist on it. */
            printf("\ndoip-tester: FAIL (no reply to a malformed header)\n");
            goto cleanup;
        }
        if (type != DOIP_PT_GENERIC_NACK || len != 1 ||
                payload[0] != DOIP_NACK_INCORRECT_PATTERN) {
            /* The code matters, not just that something was refused: this
             * header fails the synchronisation pattern, which ISO 13400-2
             * assigns 0x00, and a miswired code would otherwise pass. */
            printf("\ndoip-tester: FAIL (expected an incorrect-pattern"
                   " NACK)\n");
            goto cleanup;
        }
        printf("  gateway rejected it with nack 0x%02X\n", payload[0]);
        printf("\ndoip-tester: PASS (malformed header rejected)\n");
        ret = 0;
        goto cleanup;
    }

    /* Routing activation: source address, activation type, 4 reserved. */
    req[0] = (unsigned char)((DOIP_TESTER_ADDRESS >> 8) & 0xFF);
    req[1] = (unsigned char)(DOIP_TESTER_ADDRESS & 0xFF);
    req[2] = DOIP_ACTIVATION_DEFAULT;
    memset(req + 3, 0, 4);
    if (send_message(ssl, DOIP_PT_ROUTING_ACTIVATION_REQ, req, 7) != 0) {
        goto cleanup;
    }

    if (recv_message(ssl, &type, payload, &len) != 0) {
        const char* alert = NULL;

        /* Under TLS 1.3 the client finishes its handshake before the server
         * has validated the client certificate, so this is where an untrusted
         * tester learns it was refused -- but only if an alert actually
         * arrived. A gateway that crashed or a reset connection is a failure.
         */
        if (peer_refused(ssl, &alert)) {
            printf("\ndoip-tester: REFUSED by the peer after the handshake"
                   " (%s)\n", alert);
            ret = 0;
        }
        else {
            printf("\ndoip-tester: FAIL (no reply and no alert from the"
                   " gateway)\n");
        }
        goto cleanup;
    }
    /* ISO 13400-2 fixes the routing activation response payload at 9 bytes:
     * the tester address, this entity's address, the response code and four
     * reserved bytes. Nine, not five: a truncated response carrying only the
     * code would otherwise be enough to activate routing and move on to
     * diagnostics. */
    if (type != DOIP_PT_ROUTING_ACTIVATION_RES || len < 9) {
        fprintf(stderr, "  unexpected reply to routing activation\n");
        goto cleanup;
    }
    printf("  activation response code 0x%02X: %s\n", payload[4],
            doip_ra_reason(payload[4]));

    if (payload[4] != DOIP_RA_SUCCESS) {
        printf("\ndoip-tester: REFUSED (%s)\n", doip_ra_reason(payload[4]));
        ret = 0; /* a refusal is a valid outcome, not a failure */
        goto cleanup;
    }

    /* Diagnostic message: source, target, then the UDS request.
     * 0x22 F1 90 is ReadDataByIdentifier for the VIN. */
    req[0] = (unsigned char)((DOIP_TESTER_ADDRESS >> 8) & 0xFF);
    req[1] = (unsigned char)(DOIP_TESTER_ADDRESS & 0xFF);
    req[2] = (unsigned char)((DOIP_GATEWAY_ADDRESS >> 8) & 0xFF);
    req[3] = (unsigned char)(DOIP_GATEWAY_ADDRESS & 0xFF);
    req[4] = 0x22;
    req[5] = 0xF1;
    req[6] = 0x90;
    if (send_message(ssl, DOIP_PT_DIAG_MESSAGE, req, 7) != 0) {
        goto cleanup;
    }

    /* First the ack, then the response as a separate message. */
    if (recv_message(ssl, &type, payload, &len) != 0) {
        goto cleanup;
    }
    if (type == DOIP_PT_DIAG_MESSAGE_NACK) {
        printf("  diagnostic message refused, code 0x%02X\n",
                (len >= 5) ? payload[4] : 0);
        goto cleanup;
    }
    if (type != DOIP_PT_DIAG_MESSAGE_ACK || len < 5 ||
            payload[4] != DOIP_DIAG_ACK_OK) {
        /* Length and code both checked: a positive ack carries SA, TA and the
         * ack code, so accepting any 0x8002 would let a malformed or
         * non-success ack through and the run could still end in PASS on the
         * strength of whatever followed. */
        fprintf(stderr, "  expected a successful diagnostic ack\n");
        goto cleanup;
    }

    if (recv_message(ssl, &type, payload, &len) != 0) {
        goto cleanup;
    }
    if (type != DOIP_PT_DIAG_MESSAGE || len < 7) {
        fprintf(stderr, "  expected a diagnostic response\n");
        goto cleanup;
    }

    /* SID 0x62, DID F1 90, then exactly 17 VIN bytes. Without the length
     * check a response carrying no VIN at all prints an empty one and still
     * reports PASS. */
    if (len == 7U + 17U && payload[4] == 0x62 && payload[5] == 0xF1 &&
            payload[6] == 0x90) {
        printf("  VIN: %.*s\n", (int)(len - 7), (const char*)(payload + 7));
        printf("\ndoip-tester: PASS\n");
        ret = 0;
    }
    else if (payload[4] == 0x7F) {
        printf("  UDS negative response, NRC 0x%02X\n",
                (len >= 7) ? payload[6] : 0);
    }
    else {
        fprintf(stderr, "  unrecognised UDS response\n");
    }

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
