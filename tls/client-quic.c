/* client-quic.c
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

/* The client half of a QUIC connection, told from wolfSSL's point of view.
 *
 * wolfSSL does not speak QUIC. It runs the TLS 1.3 handshake on behalf of a
 * QUIC stack: the stack hands it the peer's handshake bytes, and wolfSSL
 * answers through four callbacks with bytes to send, keys to install and
 * alerts to raise. Everything else, packets, encryption, loss recovery, is
 * the stack's job, and in this demo that job is left as a placeholder.
 *
 * Once the handshake is done the client sends one protected message, given
 * on the command line, and waits for the server's protected reply. It then
 * sends "shutdown" as a second packet, which stops the server, and exits.
 *
 * Pair with server-quic. Requires wolfSSL built with --enable-quic.
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

#define CLIENT_MESSAGE "hello over QUIC"

/* What the callbacks need to reach the network, plus the two secrets the
 * demo keeps. A real stack would hang its whole connection object here. */
typedef struct {
    int                fd;
    struct sockaddr_in peer;
    uint8_t            appReadSecret[QUIC_DEMO_SECRET_MAX];
    uint8_t            appWriteSecret[QUIC_DEMO_SECRET_MAX];
    size_t             appSecretLen;
} DemoConn;

/* Our transport parameters. TLS never looks inside them; it just carries
 * them in the quic_transport_parameters extension. A real stack encodes its
 * RFC 9000 section 18 limits here. These bytes say max_idle_timeout = 30s
 * and initial_max_data = 1 MiB. */
static const uint8_t clientTransportParams[] = {
    0x01, 0x04, 0x80, 0x00, 0x75, 0x30,
    0x04, 0x04, 0x80, 0x10, 0x00, 0x00
};

/*---------------------------------------------------------------------------*/
/* The four callbacks wolfSSL drives the QUIC stack through                  */
/*---------------------------------------------------------------------------*/

/* wolfSSL has derived new traffic secrets for a level. A real stack would
 * expand them into packet protection keys right away and start encrypting
 * packets at that level. The demo keeps only the application secrets,
 * which main() turns into keys after the handshake. */
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
    printf("[client] keys ready at %s level:%s%s (%u-byte secrets)\n",
           QUIC_DEMO_LEVEL_NAME(level), readSecret ? " read" : "",
           writeSecret ? " write" : "", (unsigned int)secretLen);
    return 1;
}

/* wolfSSL has handshake bytes to send at a level. A real stack would place
 * them in CRYPTO frames inside a packet protected with that level's keys,
 * and keep them until acknowledged. The demo sends them as they are, tagged
 * with the level. */
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
                   (struct sockaddr*)&conn->peer, sizeof(conn->peer)) < 0) {
            return 0;
        }
        printf("[client] sent %u handshake bytes at %s level\n",
               (unsigned int)chunk, QUIC_DEMO_LEVEL_NAME(level));
        data += chunk;
        len  -= chunk;
    }
    return 1;
}

/* A flight of handshake messages is complete. A stack that batches packets
 * would send them now; the demo already sent everything. */
static int FlushFlight(WOLFSSL* ssl)
{
    (void)ssl;
    return 1;
}

/* wolfSSL wants to raise a TLS alert. QUIC has no alert records, so a real
 * stack closes the connection with CONNECTION_CLOSE carrying error code
 * 0x0100 + alert (RFC 9001 section 4.8). */
static int SendAlert(WOLFSSL* ssl, WOLFSSL_ENCRYPTION_LEVEL level,
                     uint8_t alert)
{
    (void)ssl;
    printf("[client] TLS alert %u at %s level, a stack would send "
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
 * context. Returns the info length. */
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

    int                   ret = 0;
    DemoConn              conn = {0};
    const char*           msg = CLIENT_MESSAGE;
    struct QuicSuite      suite = {0};
    struct QuicPacketKeys tx = {0};   /* our packets, from the write secret */
    struct QuicPacketKeys rx = {0};   /* the server's, from the read secret */

    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    if (argc != 2 && argc != 3) {
        printf("usage: %s <IPv4 address> [message]\n", argv[0]);
        return 0;
    }
    if (argc == 3) {
        msg = argv[2];
    }

    if (strlen(msg) > QUIC_DEMO_DGRAM_MAX - QUIC_DEMO_APP_HDR_LEN -
                      QUIC_DEMO_TAG_MAX) {
        fprintf(stderr, "ERROR: message too long for one packet\n");
        ret = -1;
    }

    /* A plain UDP socket stands in for the QUIC stack's network path */
    if (ret == 0) {
        if ((conn.fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            fprintf(stderr, "ERROR: failed to create the socket\n");
            ret = -1;
        }
    }
    if (ret == 0) {
        conn.peer.sin_family = AF_INET;
        conn.peer.sin_port   = htons(QUIC_DEMO_PORT);
        if (inet_pton(AF_INET, argv[1], &conn.peer.sin_addr) != 1) {
            fprintf(stderr, "ERROR: invalid address\n");
            ret = -1;
        }
    }
    if (ret == 0) {
        /* Don't wait on a silent server forever */
        struct timeval timeout;

        timeout.tv_sec  = QUIC_DEMO_TIMEOUT_SEC;
        timeout.tv_usec = 0;
        setsockopt(conn.fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                   sizeof(timeout));
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
        if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method())) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
            ret = -1;
        }
    }
    if (ret == 0) {
        if (wolfSSL_CTX_load_verify_locations(ctx, QUIC_DEMO_CA_FILE, NULL)
                != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load %s\n", QUIC_DEMO_CA_FILE);
            ret = -1;
        }
    }

    /* Installing the callbacks is what turns a TLS context into a QUIC one.
     * From here on wolfSSL never touches a socket; it talks only to them. */
    if (ret == 0) {
        if (wolfSSL_CTX_set_quic_method(ctx, &quicMethod) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: wolfSSL_CTX_set_quic_method failed\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1;
        }
    }
    if (ret == 0) {
        printf("[client] wolfSSL_is_quic: %d\n", wolfSSL_is_quic(ssl));

        /* Let the callbacks find the network */
        wolfSSL_set_app_data(ssl, &conn);
    }

    /* A QUIC handshake fails without transport parameters on both sides */
    if (ret == 0) {
        if (wolfSSL_set_quic_transport_params(ssl, clientTransportParams,
                sizeof(clientTransportParams)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: wolfSSL_set_quic_transport_params "
                    "failed\n");
            ret = -1;
        }
    }

    /* ...and without an agreed application protocol */
    if (ret == 0) {
        if (wolfSSL_UseALPN(ssl, (char*)QUIC_DEMO_ALPN, QUIC_DEMO_ALPN_SZ,
                WOLFSSL_ALPN_FAILED_ON_MISMATCH) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: wolfSSL_UseALPN failed\n");
            ret = -1;
        }
    }

#ifdef HAVE_SESSION_TICKET
    /* Ask for a ticket so we can watch post-handshake data arrive later */
    if (ret == 0) {
        wolfSSL_UseSessionTicket(ssl);
    }
#endif

    /*---------------------------------*/
    /* The handshake                   */
    /*---------------------------------*/
    /* Each turn of this loop is one step. wolfSSL_quic_do_handshake() goes
     * as far as the bytes it has been given allow, emitting its own messages
     * through AddHandshakeData() along the way. The first call produces the
     * ClientHello. When it needs more from the server it says WANT_READ, and
     * we hand it the next datagram with wolfSSL_provide_quic_data(), tagged
     * with the level it arrived at. */
    while (ret == 0) {
        uint8_t dgram[1 + QUIC_DEMO_DGRAM_MAX];
        int     n = 0;
        int     rc;
        int     err;

        rc = wolfSSL_quic_do_handshake(ssl);
        if (rc == WOLFSSL_SUCCESS) {
            break;
        }
        err = wolfSSL_get_error(ssl, rc);
        if (err != WOLFSSL_ERROR_WANT_READ) {
            fprintf(stderr, "ERROR: handshake failed: %d (%s)\n", err,
                    wolfSSL_ERR_reason_error_string((unsigned long)err));
            ret = -1;
        }

        /* A real stack would decrypt the packet with the level's keys, pull
         * out the CRYPTO frames, put them in order and acknowledge them. The
         * demo's datagram already is the CRYPTO data. */
        if (ret == 0) {
            n = (int)recvfrom(conn.fd, dgram, sizeof(dgram), 0, NULL, NULL);
            if (n < 2) {
                fprintf(stderr, "ERROR: no handshake data from the server\n");
                ret = -1;
            }
        }
        if (ret == 0) {
            printf("[client] received %d handshake bytes at %s level\n",
                   n - 1, QUIC_DEMO_LEVEL_NAME(dgram[0]));

            if (wolfSSL_provide_quic_data(ssl,
                    (WOLFSSL_ENCRYPTION_LEVEL)dgram[0], dgram + 1,
                    (size_t)(n - 1)) != WOLFSSL_SUCCESS) {
                fprintf(stderr, "ERROR: wolfSSL_provide_quic_data failed\n");
                ret = -1;
            }
        }
    }

    /*---------------------------------*/
    /* What the handshake settled      */
    /*---------------------------------*/
    if (ret == 0) {
        const uint8_t* peerParams = NULL;
        size_t         peerParamsLen = 0;
        char*          alpn = NULL;
        unsigned short alpnSz = 0;

        printf("[client] handshake complete: %s, %s\n",
               wolfSSL_get_version(ssl), wolfSSL_get_cipher_name(ssl));
        printf("[client] reading at %s level, writing at %s level\n",
               QUIC_DEMO_LEVEL_NAME(wolfSSL_quic_read_level(ssl)),
               QUIC_DEMO_LEVEL_NAME(wolfSSL_quic_write_level(ssl)));

        if (wolfSSL_ALPN_GetProtocol(ssl, &alpn, &alpnSz) == WOLFSSL_SUCCESS) {
            printf("[client] ALPN: %.*s\n", (int)alpnSz, alpn);
        }

        /* The server's transport parameters come back as the raw bytes it
         * sent. A real stack would decode them and size its flow control
         * windows. */
        wolfSSL_get_peer_quic_transport_params(ssl, &peerParams,
                                               &peerParamsLen);
        printf("[client] server transport parameters: %u bytes\n",
               (unsigned int)peerParamsLen);

        /* The ciphers the stack must protect packets with. wolfSSL picked
         * them during the handshake; the stack pairs them with the secrets
         * it was handed in SetEncryptionSecrets(). */
        suite.aead   = wolfSSL_quic_get_aead(ssl);
        suite.md     = wolfSSL_quic_get_md(ssl);
        suite.keyLen = (size_t)wolfSSL_EVP_Cipher_key_length(suite.aead);
        suite.tagLen = wolfSSL_quic_get_aead_tag_len(suite.aead);
        printf("[client] packet AEAD: %s with %u-byte tag, header protection "
               "%s, hash %s\n",
               wolfSSL_quic_aead_is_gcm(suite.aead)      ? "AES-GCM" :
               wolfSSL_quic_aead_is_ccm(suite.aead)      ? "AES-CCM" :
               wolfSSL_quic_aead_is_chacha20(suite.aead) ? "ChaCha20-Poly1305" :
                                                           "?",
               (unsigned int)suite.tagLen,
               wolfSSL_quic_get_hp(ssl) != NULL ? "available" : "missing",
               suite.md != NULL ? "available" : "missing");
    }

    /*---------------------------------*/
    /* Application data                */
    /*---------------------------------*/
    /* The handshake's real product is the pair of application secrets handed
     * to SetEncryptionSecrets(), one per direction. RFC 9001 section 5.1
     * expands each into a packet key and IV with the hash wolfSSL
     * negotiated: the write secret for what we send, the read secret for
     * what the server sends back. A real stack would also derive the
     * "quic hp" header protection keys here. */
    if (ret == 0 && suite.keyLen > QUIC_DEMO_KEY_MAX) {
        fprintf(stderr, "ERROR: unexpected key length %u\n",
                (unsigned int)suite.keyLen);
        ret = -1;
    }
    if (ret == 0) {
        uint8_t info[32];
        size_t  infoLen;

        infoLen = QuicKdfLabel(info, "quic key", suite.keyLen);
        if (wolfSSL_quic_hkdf_expand(tx.key, suite.keyLen, suite.md,
                conn.appWriteSecret, conn.appSecretLen, info, infoLen)
                != WOLFSSL_SUCCESS ||
            wolfSSL_quic_hkdf_expand(rx.key, suite.keyLen, suite.md,
                conn.appReadSecret, conn.appSecretLen, info, infoLen)
                != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to derive the packet keys\n");
            ret = -1;
        }

        if (ret == 0) {
            infoLen = QuicKdfLabel(info, "quic iv", QUIC_DEMO_IV_LEN);
            if (wolfSSL_quic_hkdf_expand(tx.iv, QUIC_DEMO_IV_LEN, suite.md,
                    conn.appWriteSecret, conn.appSecretLen, info, infoLen)
                    != WOLFSSL_SUCCESS ||
                wolfSSL_quic_hkdf_expand(rx.iv, QUIC_DEMO_IV_LEN, suite.md,
                    conn.appReadSecret, conn.appSecretLen, info, infoLen)
                    != WOLFSSL_SUCCESS) {
                fprintf(stderr, "ERROR: failed to derive the packet IVs\n");
                ret = -1;
            }
        }
    }

    /* One AEAD context per direction, reused for every packet */
    if (ret == 0) {
        tx.aead = wolfSSL_quic_crypt_new(suite.aead, tx.key, tx.iv, 1);
        rx.aead = wolfSSL_quic_crypt_new(suite.aead, rx.key, rx.iv, 0);
        if (tx.aead == NULL || rx.aead == NULL) {
            fprintf(stderr, "ERROR: wolfSSL_quic_crypt_new failed\n");
            ret = -1;
        }
    }

    /* A real 1-RTT packet has a short header with the connection ID and
     * packet number, and its payload is a STREAM frame. The demo's header
     * is a type byte and packet number 0, and the payload is the message.
     * The header is authenticated as associated data, and the nonce is the
     * IV XORed with the packet number, so no two packets share one. */
    if (ret == 0) {
        uint8_t packet[QUIC_DEMO_DGRAM_MAX];
        uint8_t nonce[QUIC_DEMO_IV_LEN];
        size_t  msgLen = strlen(msg);
        int     n;

        packet[0] = QUIC_DEMO_APP_PACKET;
        packet[1] = tx.nextPn++;
        memcpy(nonce, tx.iv, sizeof(nonce));
        nonce[QUIC_DEMO_IV_LEN - 1] ^= packet[1];

        if (wolfSSL_quic_aead_encrypt(packet + QUIC_DEMO_APP_HDR_LEN, tx.aead,
                (const uint8_t*)msg, msgLen, nonce,
                packet, QUIC_DEMO_APP_HDR_LEN) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: wolfSSL_quic_aead_encrypt failed\n");
            ret = -1;
        }

        /* Header protection would now mask the packet number with the hp
         * key, and the packet would wait in the stack's queue until
         * acknowledged. */
        if (ret == 0) {
            n = (int)(QUIC_DEMO_APP_HDR_LEN + msgLen + suite.tagLen);
            if (sendto(conn.fd, packet, (size_t)n, 0,
                       (struct sockaddr*)&conn.peer, sizeof(conn.peer)) < 0) {
                fprintf(stderr, "ERROR: failed to send application data\n");
                ret = -1;
            }
            else {
                printf("[client] sent \"%s\" as protected packet %u "
                       "(%d bytes)\n", msg, (unsigned int)packet[1], n);
            }
        }
    }

    /*---------------------------------*/
    /* Waiting for the reply           */
    /*---------------------------------*/
    /* Two kinds of datagram can arrive now. TLS is not quite done: with
     * session tickets built in, the server follows up with a NewSessionTicket
     * as CRYPTO data at the application level. It is fed in like any other
     * handshake bytes, but processed with wolfSSL_process_quic_post_handshake()
     * now that the handshake itself is over. The other kind is the server's
     * protected reply, which ends the wait. A real stack would tell them
     * apart by the frames inside one packet; the demo uses the first byte. */
    if (ret == 0) {
        uint8_t dgram[1 + QUIC_DEMO_DGRAM_MAX];
        uint8_t nonce[QUIC_DEMO_IV_LEN];
        uint8_t plain[QUIC_DEMO_DGRAM_MAX + 1];
        size_t  plainLen;
        int     n = 0;

        while (ret == 0) {
            n = (int)recvfrom(conn.fd, dgram, sizeof(dgram), 0, NULL, NULL);
            if (n < 2) {
                fprintf(stderr, "ERROR: no reply from the server\n");
                ret = -1;
            }
            else if (dgram[0] == QUIC_DEMO_APP_PACKET) {
                break;
            }
            else {
                printf("[client] received %d post-handshake bytes at %s "
                       "level\n", n - 1, QUIC_DEMO_LEVEL_NAME(dgram[0]));
                if (wolfSSL_provide_quic_data(ssl,
                        (WOLFSSL_ENCRYPTION_LEVEL)dgram[0], dgram + 1,
                        (size_t)(n - 1)) != WOLFSSL_SUCCESS ||
                    wolfSSL_process_quic_post_handshake(ssl)
                        != WOLFSSL_SUCCESS) {
                    fprintf(stderr, "ERROR: post-handshake processing "
                            "failed\n");
                    ret = -1;
                }
                else {
                    printf("[client] Processed post-handshake data\n");
                }
            }
        }

        /* The reply is opened just as the server opened our message, with
         * the keys from our read secret, which is the server's write
         * secret */
        if (ret == 0 && n < (int)(QUIC_DEMO_APP_HDR_LEN + suite.tagLen)) {
            fprintf(stderr, "ERROR: reply too short\n");
            ret = -1;
        }
        if (ret == 0) {
            memcpy(nonce, rx.iv, sizeof(nonce));
            nonce[QUIC_DEMO_IV_LEN - 1] ^= dgram[1];
            plainLen = (size_t)n - QUIC_DEMO_APP_HDR_LEN - suite.tagLen;

            if (wolfSSL_quic_aead_decrypt(plain, rx.aead,
                    dgram + QUIC_DEMO_APP_HDR_LEN,
                    (size_t)n - QUIC_DEMO_APP_HDR_LEN, nonce,
                    dgram, QUIC_DEMO_APP_HDR_LEN) != WOLFSSL_SUCCESS) {
                fprintf(stderr, "ERROR: wolfSSL_quic_aead_decrypt failed\n");
                ret = -1;
            }
            else {
                plain[plainLen] = '\0';
                printf("[client] opened protected packet %u: \"%s\"\n",
                       (unsigned int)dgram[1], (char*)plain);
            }
        }
    }

    /*---------------------------------*/
    /* Shutting the server down        */
    /*---------------------------------*/
    /* One more protected packet on the same connection. Its packet number
     * is 1, so its nonce differs from packet 0's even though the key and IV
     * are the same; reusing a nonce under one key would break the AEAD. A
     * real stack never resets the count, and moves to fresh keys with a key
     * update long before it could run out. */
    if (ret == 0) {
        uint8_t packet[QUIC_DEMO_APP_HDR_LEN + sizeof(QUIC_DEMO_SHUTDOWN) +
                    QUIC_DEMO_TAG_MAX];
        uint8_t nonce[QUIC_DEMO_IV_LEN];
        size_t  msgLen = strlen(QUIC_DEMO_SHUTDOWN);
        int     n;

        packet[0] = QUIC_DEMO_APP_PACKET;
        packet[1] = tx.nextPn++;
        memcpy(nonce, tx.iv, sizeof(nonce));
        nonce[QUIC_DEMO_IV_LEN - 1] ^= packet[1];

        if (wolfSSL_quic_aead_encrypt(packet + QUIC_DEMO_APP_HDR_LEN, tx.aead,
                (const uint8_t*)QUIC_DEMO_SHUTDOWN, msgLen, nonce,
                packet, QUIC_DEMO_APP_HDR_LEN) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: wolfSSL_quic_aead_encrypt failed\n");
            ret = -1;
        }
        if (ret == 0) {
            n = (int)(QUIC_DEMO_APP_HDR_LEN + msgLen + suite.tagLen);
            if (sendto(conn.fd, packet, (size_t)n, 0,
                       (struct sockaddr*)&conn.peer, sizeof(conn.peer)) < 0) {
                fprintf(stderr, "ERROR: failed to send the shutdown\n");
                ret = -1;
            }
            else {
                printf("[client] sent \"%s\" as protected packet %u "
                       "(%d bytes)\n", QUIC_DEMO_SHUTDOWN,
                       (unsigned int)packet[1], n);
            }
        }
    }

    /* Closing is QUIC's business too: a real stack would send
     * CONNECTION_CLOSE and linger in the draining state. TLS sends no
     * close_notify in QUIC, so there is no wolfSSL_shutdown() here. */

    /* Success or failure, everything is released here */
    if (tx.aead)
        wolfSSL_EVP_CIPHER_CTX_free(tx.aead);
    if (rx.aead)
        wolfSSL_EVP_CIPHER_CTX_free(rx.aead);
    if (ssl)
        wolfSSL_free(ssl);
    if (ctx)
        wolfSSL_CTX_free(ctx);
    if (conn.fd != -1)
        close(conn.fd);
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
