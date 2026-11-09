/* server-quic.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

/* The server half of a QUIC connection, told from wolfSSL's point of view.
 *
 * As on the client, wolfSSL does not speak QUIC. It runs the TLS 1.3
 * handshake on behalf of a QUIC stack: the stack hands it the peer's
 * handshake bytes, and wolfSSL answers through four callbacks with bytes to
 * send, keys to install and alerts to raise. Everything else, packets,
 * encryption, loss recovery, is the stack's job, and in this demo that job
 * is left as a placeholder.
 *
 * The server waits for a client's first datagram, which carries the
 * ClientHello, and from then on mirrors the client: hand wolfSSL what
 * arrives, let it answer through the callbacks, repeat until the handshake
 * is done. It then opens each protected message the client sends and
 * answers it, until the client goes quiet or sends "shutdown". Going quiet
 * ends only that connection and the server waits for the next client;
 * "shutdown" stops the server.
 *
 * Pair with client-quic. Requires wolfSSL built with --enable-quic.
 */

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>

#include "shared-quic-constants.h"

#ifdef WOLFSSL_QUIC

#include <wolfssl/quic.h>

#define SERVER_REPLY "I hear ya fa shizzle!"

/* What the callbacks need to reach the network, plus the two secrets the
 * demo keeps. Unlike the client, the server learns its peer's address from
 * the first datagram, so it keeps the length recvfrom() reported too. A real
 * stack would hang its whole connection object here, one per client. */
typedef struct {
    int                fd;
    struct sockaddr_in peer;
    socklen_t          peerLen;
    uint8_t            appReadSecret[QUIC_DEMO_SECRET_MAX];
    uint8_t            appWriteSecret[QUIC_DEMO_SECRET_MAX];
    size_t             appSecretLen;
} DemoConn;

/* Our transport parameters. As on the client, TLS never looks inside them;
 * it just carries them in the quic_transport_parameters extension, in the
 * server's case inside EncryptedExtensions. A real stack encodes its RFC 9000
 * section 18 limits here, and a server would also include the connection
 * IDs it chose. These bytes say max_idle_timeout = 60s, initial_max_data =
 * 10 MiB and max_udp_payload_size = 1200. */
static const uint8_t serverTransportParams[] = {
    0x01, 0x04, 0x80, 0x00, 0xea, 0x60,
    0x04, 0x04, 0x80, 0xa0, 0x00, 0x00,
    0x03, 0x02, 0x44, 0xb0
};

/*---------------------------------------------------------------------------*/
/* The four callbacks wolfSSL drives the QUIC stack through                  */
/*---------------------------------------------------------------------------*/

/* wolfSSL has derived new traffic secrets for a level. A real stack would
 * expand them into packet protection keys right away.
 * Having a key is not the same as using it, though:
 * the read and write levels printed in main() show the server writing at
 * the application level while still reading at the handshake level, until
 * the client's Finished arrives. The demo keeps only the application
 * secrets, which main() needs to open the client's message and answer it. */
static int SetEncryptionSecrets(WOLFSSL* ssl, WOLFSSL_ENCRYPTION_LEVEL level,
                                const uint8_t* readSecret,
                                const uint8_t* writeSecret, size_t secretLen)
{
    DemoConn* conn = (DemoConn*)wolfSSL_get_app_data(ssl);

    if (level == wolfssl_encryption_application) {
        if (secretLen > QUIC_DEMO_SECRET_MAX) {
            return 0;
        }
        if (readSecret != NULL) {
            memcpy(conn->appReadSecret, readSecret, secretLen);
        }
        if (writeSecret != NULL) {
            memcpy(conn->appWriteSecret, writeSecret, secretLen);
        }
        conn->appSecretLen = secretLen;
    }
    printf("[server] keys ready at %s level:%s%s (%u-byte secrets)\n",
           QUIC_DEMO_LEVEL_NAME(level), readSecret ? " read" : "",
           writeSecret ? " write" : "", (unsigned int)secretLen);
    return 1;
}

/* wolfSSL has handshake bytes to send at a level. A real stack would place
 * them in CRYPTO frames inside a packet protected with that level's keys,
 * and keep them until acknowledged. The server's first flight is the largest
 * thing either side sends, since it carries the certificate chain, so it
 * may span several datagrams; until the client's address is validated, RFC
 * 9000 section 8.1 also caps it at three times what the client sent. The
 * demo sends the bytes as they are, tagged with the level, split into
 * datagram-sized chunks. */
static int AddHandshakeData(WOLFSSL* ssl, WOLFSSL_ENCRYPTION_LEVEL level,
                            const uint8_t* data, size_t len)
{
    DemoConn* conn = (DemoConn*)wolfSSL_get_app_data(ssl);
    uint8_t   dgram[1 + QUIC_DEMO_DGRAM_MAX];
    size_t    chunk;

    while (len > 0) {
        chunk = (len > QUIC_DEMO_DGRAM_MAX) ? QUIC_DEMO_DGRAM_MAX : len;
        dgram[0] = (uint8_t)level;
        memcpy(dgram + 1, data, chunk);
        if (sendto(conn->fd, dgram, 1 + chunk, 0,
                   (struct sockaddr*)&conn->peer, conn->peerLen) < 0) {
            return 0;
        }
        printf("[server] sent %u handshake bytes at %s level\n",
               (unsigned int)chunk, QUIC_DEMO_LEVEL_NAME(level));
        data += chunk;
        len  -= chunk;
    }
    return 1;
}

/* A flight of handshake messages is complete. For the server that means
 * ServerHello through Finished have all been handed over, and a stack that
 * batches packets would coalesce and send them now; the demo already sent
 * everything. */
static int FlushFlight(WOLFSSL* ssl)
{
    (void)ssl;
    return 1;
}

/* wolfSSL wants to raise a TLS alert, for instance when the client offers
 * no ALPN protocol we accept. QUIC has no alert records, so a real stack
 * closes the connection with CONNECTION_CLOSE carrying error code
 * 0x0100 + alert (RFC 9001 section 4.8). */
static int SendAlert(WOLFSSL* ssl, WOLFSSL_ENCRYPTION_LEVEL level,
                     uint8_t alert)
{
    (void)ssl;
    printf("[server] TLS alert %u at %s level, a stack would send "
           "CONNECTION_CLOSE 0x%04x\n", (unsigned int)alert,
           QUIC_DEMO_LEVEL_NAME(level), 0x0100 + (unsigned int)alert);
    return 1;
}

/* The callbacks are matched by position, so they must follow the field order
 * of WOLFSSL_QUIC_METHOD in <wolfssl/quic.h>: set_encryption_secrets,
 * add_handshake_data, flush_flight, send_alert. The order follows a
 * handshake: keys for a level, bytes at that level, the end of a flight, and
 * an alert if something goes wrong. */
static const WOLFSSL_QUIC_METHOD quicMethod = {
    /*set_encryption_secrets=*/SetEncryptionSecrets,
    /*add_handshake_data=*/AddHandshakeData,
    /*flush_flight=*/FlushFlight,
    /*send_alert=*/SendAlert
};

/* Lay out the HkdfLabel that wolfSSL_quic_hkdf_expand() takes as its info,
 * per RFC 8446 section 7.1: output length, "tls13 " + label, and an empty
 * context. Both sides must build it byte for byte the same, or their keys
 * won't match. Returns the info length. */
static size_t QuicKdfLabel(uint8_t* info, const char* label, size_t outLen)
{
    size_t labelLen = strlen(label);
    size_t idx = 0;

    info[idx++] = (uint8_t)(outLen >> 8);
    info[idx++] = (uint8_t)outLen;
    info[idx++] = (uint8_t)(6 + labelLen);
    memcpy(info + idx, "tls13 ", 6);
    idx += 6;
    memcpy(info + idx, label, labelLen);
    idx += labelLen;
    info[idx++] = 0;
    return idx;
}

/*---------------------------------------------------------------------------*/

int main(int argc, char** argv)
{
    /* What the handshake settled on for protecting packets. wolfSSL picks
     * the AEAD and the hash; the key and tag lengths follow from them. */
    struct QuicSuite {
        const WOLFSSL_EVP_CIPHER* aead;
        const WOLFSSL_EVP_MD*     md;
        size_t                    keyLen;
        size_t                    tagLen;
    };

    /* One direction of 1-RTT packet protection, expanded from that
     * direction's application secret. The AEAD context holds the key, and
     * each packet's nonce is the IV mixed with its packet number. */
    struct QuicPacketKeys {
        WOLFSSL_EVP_CIPHER_CTX* aead;
        uint8_t                 key[QUIC_DEMO_KEY_MAX];
        uint8_t                 iv[QUIC_DEMO_IV_LEN];
        uint8_t                 nextPn;   /* sending side only */
    };

    int          ret = 0;
    int          sockfd = -1;
    int          shutdownRequested = 0;
    WOLFSSL_CTX* ctx = NULL;

    (void)argc;
    (void)argv;

    /* A UDP socket on the well-known port stands in for the QUIC listener.
     * There is no listen() or accept() in UDP: every client's datagrams
     * arrive on this one socket, and the server tells them apart itself. */
    if (ret == 0) {
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            fprintf(stderr, "ERROR: failed to create the socket\n");
            ret = -1;
        }
    }
    if (ret == 0) {
        struct sockaddr_in servAddr;

        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family      = AF_INET;
        servAddr.sin_port        = htons(QUIC_DEMO_PORT);
        servAddr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr))
                == -1) {
            fprintf(stderr, "ERROR: failed to bind\n");
            ret = -1;
        }
    }

    /*---------------------------------*/
    /* Setting up wolfSSL for QUIC     */
    /*---------------------------------*/
    if (ret == 0) {
        if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to initialize the library\n");
            ret = -1;
        }
    }

    /* QUIC is built on TLS 1.3 and nothing older */
    if (ret == 0) {
        if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method())) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
            ret = -1;
        }
    }
    /* The certificate and key prove who we are, just as in TLS over TCP.
     * The client checks the certificate against its CA file. */
    if (ret == 0) {
        if (wolfSSL_CTX_use_certificate_file(ctx, QUIC_DEMO_CERT_FILE,
                WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load %s\n", QUIC_DEMO_CERT_FILE);
            ret = -1;
        }
    }
    if (ret == 0) {
        if (wolfSSL_CTX_use_PrivateKey_file(ctx, QUIC_DEMO_KEY_FILE,
                WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load %s\n", QUIC_DEMO_KEY_FILE);
            ret = -1;
        }
    }

    /* Installing the callbacks is what turns a TLS context into a QUIC one.
     * Every WOLFSSL made from this context inherits them, so from here on
     * wolfSSL never touches the socket; it talks only to the callbacks. */
    if (ret == 0) {
        if (wolfSSL_CTX_set_quic_method(ctx, &quicMethod) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: wolfSSL_CTX_set_quic_method failed\n");
            ret = -1;
        }
    }

    /*---------------------------------*/
    /* Serving clients                 */
    /*---------------------------------*/
    /* One client at a time, until one says "shutdown". A real server juggles
     * many at once, routing each packet to its own WOLFSSL by the connection
     * ID in the packet's header. A client that fails costs only its own
     * connection: its errors land in connRet, which ends the connection but
     * not the server. Only a broken socket or allocator sets ret too. */
    while (ret == 0 && !shutdownRequested) {
        /* Everything below lives and dies with one connection */
        DemoConn              conn;
        struct QuicSuite      suite;
        struct QuicPacketKeys tx;   /* our replies, from the write secret */
        struct QuicPacketKeys rx;   /* the client's, from the read secret */
        struct timeval        timeout;
        uint8_t               dgram[1 + QUIC_DEMO_DGRAM_MAX];
        int                   n;
        int                   connRet = 0;
        WOLFSSL*              ssl = NULL;

        memset(&conn, 0, sizeof(conn));
        memset(&suite, 0, sizeof(suite));
        memset(&tx, 0, sizeof(tx));
        memset(&rx, 0, sizeof(rx));
        conn.fd = sockfd;

        /* Between clients, wait as long as it takes */
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                   sizeof(timeout));
        printf("[server] waiting for a client on port %d\n", QUIC_DEMO_PORT);

        /* The first datagram tells us who the client is. A real server would
         * check that it is a padded Initial packet, maybe answer with Retry
         * to validate the address, and only then commit state to it. The
         * demo's check is that it arrived at the initial level, which also
         * skips anything a failed client left behind. */
        conn.peerLen = sizeof(conn.peer);
        n = (int)recvfrom(sockfd, dgram, sizeof(dgram), 0,
                          (struct sockaddr*)&conn.peer, &conn.peerLen);
        if (n < 0) {
            fprintf(stderr, "ERROR: failed to receive from the socket\n");
            ret = -1;
            connRet = -1;
        }
        else if (n < 2 || dgram[0] != wolfssl_encryption_initial) {
            printf("[server] ignoring a datagram that starts no handshake\n");
            continue;
        }

        /* Once a client has started, don't wait on it forever */
        if (connRet == 0) {
            timeout.tv_sec = QUIC_DEMO_TIMEOUT_SEC;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                       sizeof(timeout));
        }

        /* Each connection gets a fresh WOLFSSL from the shared context */
        if (connRet == 0) {
            if ((ssl = wolfSSL_new(ctx)) == NULL) {
                fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
                ret = -1;
                connRet = -1;
            }
            else {
                /* Let the callbacks find the network and this client's
                 * address */
                wolfSSL_set_app_data(ssl, &conn);
            }
        }

        /* A QUIC handshake fails without transport parameters on both
         * sides. The client's arrive in its ClientHello; ours go back in
         * EncryptedExtensions. */
        if (connRet == 0) {
            if (wolfSSL_set_quic_transport_params(ssl, serverTransportParams,
                    sizeof(serverTransportParams)) != WOLFSSL_SUCCESS) {
                fprintf(stderr, "ERROR: wolfSSL_set_quic_transport_params "
                        "failed\n");
                connRet = -1;
            }
        }

        /* We set out ALPN protocols. On the server this is the list we accept.
         * A client that offers none in our list is
         * turned away with a no_application_protocol alert,
         * through SendAlert(). */
        if (connRet == 0) {
            if (wolfSSL_UseALPN(ssl, (char*)QUIC_DEMO_ALPN, QUIC_DEMO_ALPN_SZ,
                    WOLFSSL_ALPN_FAILED_ON_MISMATCH) != WOLFSSL_SUCCESS) {
                fprintf(stderr, "ERROR: wolfSSL_UseALPN failed\n");
                connRet = -1;
            }
        }

#ifdef WOLFSSL_EARLY_DATA
        /* Accepting 0-RTT is the server's call. This demo never reads early
         * data, so it says no up front. */
        if (connRet == 0) {
            wolfSSL_set_quic_early_data_enabled(ssl, 0);
        }
#endif

        /*---------------------------------*/
        /* The handshake                   */
        /*---------------------------------*/
        /* The server's loop runs the other way round from the client's: data
         * first, then a step. Each datagram goes to wolfSSL_provide_quic_data()
         * at the level it came in on, and wolfSSL_quic_do_handshake() reacts,
         * going as far as the bytes it has been given allow. The ClientHello
         * draws out the server's whole first flight, ServerHello at the
         * initial level and EncryptedExtensions through Finished at the
         * handshake level, via AddHandshakeData(). wolfSSL then says
         * WANT_READ until the client's Finished arrives, and the step that
         * consumes it is the one that returns success. */
        while (connRet == 0) {
            int rc;
            int err;

            printf("[server] received %d handshake bytes at %s level\n", n - 1,
                   QUIC_DEMO_LEVEL_NAME(dgram[0]));

            /* A real stack would decrypt the packet and reassemble the
             * CRYPTO frames before this point; the demo's datagram is the
             * data. */
            if (wolfSSL_provide_quic_data(ssl,
                    (WOLFSSL_ENCRYPTION_LEVEL)dgram[0], dgram + 1,
                    (size_t)(n - 1)) != WOLFSSL_SUCCESS) {
                fprintf(stderr, "ERROR: wolfSSL_provide_quic_data failed\n");
                connRet = -1;
            }

            if (connRet == 0) {
                rc = wolfSSL_quic_do_handshake(ssl);
                if (rc == WOLFSSL_SUCCESS) {
                    break;
                }
                err = wolfSSL_get_error(ssl, rc);
                if (err != WOLFSSL_ERROR_WANT_READ) {
                    fprintf(stderr, "ERROR: handshake failed: %d (%s)\n", err,
                            wolfSSL_ERR_reason_error_string(
                                (unsigned long)err));
                    connRet = -1;
                }
            }

            /* Still mid-handshake: show where each direction stands, then
             * wait for the client's next datagram. Any address will do here;
             * a real server would route by connection ID instead. */
            if (connRet == 0) {
                printf("[server] reading at %s level, writing at %s level\n",
                       QUIC_DEMO_LEVEL_NAME(wolfSSL_quic_read_level(ssl)),
                       QUIC_DEMO_LEVEL_NAME(wolfSSL_quic_write_level(ssl)));

                n = (int)recvfrom(sockfd, dgram, sizeof(dgram), 0, NULL,
                                  NULL);
                if (n < 2) {
                    fprintf(stderr, "ERROR: no handshake data from the "
                            "client\n");
                    connRet = -1;
                }
            }
        }

        /*---------------------------------*/
        /* What the handshake settled      */
        /*---------------------------------*/
        /* With a session ticket build, the final step above also produced a
         * NewSessionTicket at the application level, already on its way. */
        if (connRet == 0) {
            const uint8_t* peerParams = NULL;
            size_t         peerParamsLen = 0;
            char*          alpn = NULL;
            unsigned short alpnSz = 0;

            printf("[server] handshake complete: %s, %s\n",
                   wolfSSL_get_version(ssl), wolfSSL_get_cipher_name(ssl));

            if (wolfSSL_ALPN_GetProtocol(ssl, &alpn, &alpnSz)
                    == WOLFSSL_SUCCESS) {
                printf("[server] ALPN: %.*s\n", (int)alpnSz, alpn);
            }

            /* The client's transport parameters come back as the raw bytes
             * it sent. A real stack would decode them and size its flow
             * control windows and idle timer from them. */
            wolfSSL_get_peer_quic_transport_params(ssl, &peerParams,
                                                   &peerParamsLen);
            printf("[server] client transport parameters: %u bytes\n",
                   (unsigned int)peerParamsLen);
        }

        /*---------------------------------*/
        /* Application data                */
        /*---------------------------------*/
        /* The handshake's real product is the pair of application secrets
         * handed to SetEncryptionSecrets(), one per direction. Each direction
         * has its own keys. The client protects its packets with keys from
         * its application write secret, which is our read secret, so
         * expanding that the same way (RFC 9001 section 5.1) gives the same
         * key and IV. Our write secret gives the keys for the reply. A real
         * stack would also derive the "quic hp" header protection keys here.
         *
         * First, the ciphers to expand them for. wolfSSL picked the AEAD and
         * hash during the handshake; the key and tag lengths follow. */
        if (connRet == 0) {
            suite.aead   = wolfSSL_quic_get_aead(ssl);
            suite.md     = wolfSSL_quic_get_md(ssl);
            suite.keyLen = (size_t)wolfSSL_EVP_Cipher_key_length(suite.aead);
            suite.tagLen = wolfSSL_quic_get_aead_tag_len(suite.aead);
            if (suite.keyLen > QUIC_DEMO_KEY_MAX) {
                fprintf(stderr, "ERROR: unexpected key length %u\n",
                        (unsigned int)suite.keyLen);
                connRet = -1;
            }
        }
        if (connRet == 0) {
            uint8_t info[32];
            size_t  infoLen;

            infoLen = QuicKdfLabel(info, "quic key", suite.keyLen);
            if (wolfSSL_quic_hkdf_expand(rx.key, suite.keyLen, suite.md,
                    conn.appReadSecret, conn.appSecretLen, info, infoLen)
                    != WOLFSSL_SUCCESS ||
                wolfSSL_quic_hkdf_expand(tx.key, suite.keyLen, suite.md,
                    conn.appWriteSecret, conn.appSecretLen, info, infoLen)
                    != WOLFSSL_SUCCESS) {
                fprintf(stderr, "ERROR: failed to derive the packet keys\n");
                connRet = -1;
            }

            if (connRet == 0) {
                infoLen = QuicKdfLabel(info, "quic iv", QUIC_DEMO_IV_LEN);
                if (wolfSSL_quic_hkdf_expand(rx.iv, QUIC_DEMO_IV_LEN,
                        suite.md, conn.appReadSecret, conn.appSecretLen,
                        info, infoLen) != WOLFSSL_SUCCESS ||
                    wolfSSL_quic_hkdf_expand(tx.iv, QUIC_DEMO_IV_LEN,
                        suite.md, conn.appWriteSecret, conn.appSecretLen,
                        info, infoLen) != WOLFSSL_SUCCESS) {
                    fprintf(stderr, "ERROR: failed to derive the packet "
                            "IVs\n");
                    connRet = -1;
                }
            }
        }
        /* One AEAD context per direction, reused for every packet on this
         * connection: rx decrypts, tx encrypts */
        if (connRet == 0) {
            rx.aead = wolfSSL_quic_crypt_new(suite.aead, rx.key, rx.iv, 0);
            tx.aead = wolfSSL_quic_crypt_new(suite.aead, tx.key, tx.iv, 1);
            if (rx.aead == NULL || tx.aead == NULL) {
                fprintf(stderr, "ERROR: wolfSSL_quic_crypt_new failed\n");
                connRet = -1;
            }
        }

        /* The connection stays open while the client keeps talking. Each
         * message gets a reply, except "shutdown", which ends the connection
         * and the server with it. A client that stays quiet for
         * QUIC_DEMO_TIMEOUT_SEC is taken as gone; a real server would close
         * on the idle timeout both sides announced in their transport
         * parameters. */
        while (connRet == 0) {
            uint8_t nonce[QUIC_DEMO_IV_LEN];
            uint8_t plain[QUIC_DEMO_DGRAM_MAX + 1];
            size_t  plainLen;
            uint8_t packet[QUIC_DEMO_APP_HDR_LEN + sizeof(SERVER_REPLY) +
                        QUIC_DEMO_TAG_MAX];
            size_t  replyLen = strlen(SERVER_REPLY);

            /* A real server would first remove header protection to learn
             * the packet number, and would match the connection ID to this
             * connection. The demo's header is in the clear: a type byte and
             * the packet number. */
            n = (int)recvfrom(sockfd, dgram, sizeof(dgram), 0, NULL, NULL);
            if (n < 0) {
                printf("[server] client went quiet, closing its connection\n");
                break;
            }
            if (n < (int)(QUIC_DEMO_APP_HDR_LEN + suite.tagLen) ||
                    dgram[0] != QUIC_DEMO_APP_PACKET) {
                fprintf(stderr, "ERROR: expected application data from the "
                        "client\n");
                connRet = -1;
            }

            /* Rebuild the nonce the client used, its IV (our rx.iv) XORed
             * with the packet number, and open the packet. The header is
             * passed as associated data, just as the client sealed it, so
             * the tag check fails if a single bit of header or payload was
             * altered. */
            if (connRet == 0) {
                memcpy(nonce, rx.iv, sizeof(nonce));
                nonce[QUIC_DEMO_IV_LEN - 1] ^= dgram[1];
                plainLen = (size_t)n - QUIC_DEMO_APP_HDR_LEN - suite.tagLen;

                if (wolfSSL_quic_aead_decrypt(plain, rx.aead,
                        dgram + QUIC_DEMO_APP_HDR_LEN,
                        (size_t)n - QUIC_DEMO_APP_HDR_LEN, nonce,
                        dgram, QUIC_DEMO_APP_HDR_LEN) != WOLFSSL_SUCCESS) {
                    fprintf(stderr, "ERROR: wolfSSL_quic_aead_decrypt "
                            "failed\n");
                    connRet = -1;
                }
            }
            if (connRet == 0) {
                plain[plainLen] = '\0';
                printf("[server] opened protected packet %u: \"%s\"\n",
                       (unsigned int)dgram[1], (char*)plain);

                if (strcmp((char*)plain, QUIC_DEMO_SHUTDOWN) == 0) {
                    printf("[server] shutdown requested\n");
                    shutdownRequested = 1;
                    break;
                }
            }

            /* The reply is protected the same way with our own key and IV.
             * Packet numbers count separately in each direction, and the two
             * directions never share an IV, so no nonce repeats. A real
             * server would put the reply in a STREAM frame on the client's
             * stream, and send HANDSHAKE_DONE so the client can drop its
             * handshake keys. */
            if (connRet == 0) {
                packet[0] = QUIC_DEMO_APP_PACKET;
                packet[1] = tx.nextPn++;
                memcpy(nonce, tx.iv, sizeof(nonce));
                nonce[QUIC_DEMO_IV_LEN - 1] ^= packet[1];

                if (wolfSSL_quic_aead_encrypt(packet+ QUIC_DEMO_APP_HDR_LEN,
                        tx.aead, (const uint8_t*)SERVER_REPLY, replyLen,
                        nonce, packet, QUIC_DEMO_APP_HDR_LEN)
                        != WOLFSSL_SUCCESS) {
                    fprintf(stderr, "ERROR: wolfSSL_quic_aead_encrypt "
                            "failed\n");
                    connRet = -1;
                }
            }
            if (connRet == 0) {
                n = (int)(QUIC_DEMO_APP_HDR_LEN + replyLen + suite.tagLen);
                if (sendto(sockfd, packet, (size_t)n, 0,
                           (struct sockaddr*)&conn.peer, conn.peerLen) < 0) {
                    fprintf(stderr, "ERROR: failed to send the reply\n");
                    connRet = -1;
                }
                else {
                    printf("[server] sent \"%s\" as protected packet %u "
                           "(%d bytes)\n", SERVER_REPLY,
                           (unsigned int)packet[1], n);
                }
            }
        }

        /* Closing is QUIC's business too: a real server would send
         * CONNECTION_CLOSE, or let the idle timeout end the connection, and
         * linger in the draining state. TLS sends no close_notify in QUIC,
         * so there is no wolfSSL_shutdown() here. */

        /* Whatever happened, this connection's state goes; the context and
         * socket stay for the next client */
        wolfSSL_EVP_CIPHER_CTX_free(rx.aead);
        wolfSSL_EVP_CIPHER_CTX_free(tx.aead);
        wolfSSL_free(ssl);
    }

    /* Success or failure, everything is released here */
    if (ctx)
        wolfSSL_CTX_free(ctx);
    if (sockfd != -1)
        close(sockfd);
    wolfSSL_Cleanup();

    return ret;
}

#else

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    printf("Example requires the QUIC API (build wolfSSL with "
           "--enable-quic)\n");
    return 0;
}

#endif /* WOLFSSL_QUIC */
