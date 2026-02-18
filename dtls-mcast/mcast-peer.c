/* mcast-peer.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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
 *
 *=============================================================================
 *
 * Example of DTLS 1.2 multicast with PSK using wolfSSL.
 * Three peers can communicate with each other over multicast.
 *
 * Usage: ./mcast-peer <node_id>
 *   where node_id is 0, 1, or 2
 *
 * Requires wolfSSL built with: ./configure --enable-dtls --enable-mcast
 */

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>

/* Multicast group settings */
#define MCAST_GROUP     "239.255.0.1"
#define MCAST_PORT      12345
#define MSGLEN          256

/* Number of peers in the multicast group */
#define NUM_PEERS       3

/* Pre-shared secret components (must be same for all peers) */
#define PMS_SIZE        48
#define RANDOM_SIZE     32

/* Epoch for the multicast session */
#define MCAST_EPOCH     1

#if defined(WOLFSSL_DTLS) && defined(WOLFSSL_MULTICAST)

/* Global flag for clean shutdown */
static volatile sig_atomic_t int running = 1;

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
}

/*
 * Setup a multicast UDP socket for sending to the group
 */
static int setup_tx_socket(struct sockaddr_in* txAddr)
{
    int sd;
    unsigned char ttl = 1;
    int on = 1;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        perror("socket (tx)");
        return -1;
    }

    /* Set TTL for multicast */
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("setsockopt IP_MULTICAST_TTL");
        close(sd);
        return -1;
    }

    /* Enable loopback so other processes on same host can receive */
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &on, sizeof(on)) < 0) {
        perror("setsockopt IP_MULTICAST_LOOP");
        close(sd);
        return -1;
    }

    /* Setup destination address */
    memset(txAddr, 0, sizeof(*txAddr));
    txAddr->sin_family = AF_INET;
    txAddr->sin_addr.s_addr = inet_addr(MCAST_GROUP);
    txAddr->sin_port = htons(MCAST_PORT);

    /* Connect to multicast address for sending */
    if (connect(sd, (struct sockaddr*)txAddr, sizeof(*txAddr)) < 0) {
        perror("connect (tx)");
        close(sd);
        return -1;
    }

    return sd;
}

/*
 * Setup a multicast UDP socket for receiving from the group
 */
static int setup_rx_socket(void)
{
    int sd;
    int on = 1;
    struct sockaddr_in addr;
    struct ip_mreq mreq;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        perror("socket (rx)");
        return -1;
    }

    /* Allow multiple sockets to use the same port */
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(sd);
        return -1;
    }

#ifdef SO_REUSEPORT
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
        perror("setsockopt SO_REUSEPORT");
        close(sd);
        return -1;
    }
#endif

    /* Bind to multicast port */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MCAST_PORT);

    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind (rx)");
        close(sd);
        return -1;
    }

    /* Join the multicast group */
    mreq.imr_multiaddr.s_addr = inet_addr(MCAST_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt IP_ADD_MEMBERSHIP");
        close(sd);
        return -1;
    }

    /* Set non-blocking for receive */
    {
        int flags = fcntl(sd, F_GETFL, 0);
        if (flags < 0 || fcntl(sd, F_SETFL, flags | O_NONBLOCK) < 0) {
            perror("fcntl");
            close(sd);
            return -1;
        }
    }

    return sd;
}

int main(int argc, char** argv)
{
    int ret;
    int myId;
    int txSd = -1;
    int rxSd = -1;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* sslTx = NULL;
    WOLFSSL* sslRx = NULL;
    struct sockaddr_in txAddr;
    int i;

    /* Pre-shared secret components - same for all peers */
    unsigned char pms[PMS_SIZE];
    unsigned char clientRandom[RANDOM_SIZE];
    unsigned char serverRandom[RANDOM_SIZE];
    /* WDM-NULL-SHA256 cipher suite bytes */
    unsigned char suite[2] = { 0x00, 0xFE };

    time_t lastSend = 0;
    int msgCount = 0;

    /* Parse arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <node_id>\n", argv[0]);
        fprintf(stderr, "  node_id: 0, 1, or 2\n");
        return 1;
    }

    myId = atoi(argv[1]);
    if (myId < 0 || myId >= NUM_PEERS) {
        fprintf(stderr, "Error: node_id must be 0, 1, or 2\n");
        return 1;
    }

    /* Setup signal handler */
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("=== DTLS Multicast Peer - Node %d ===\n", myId);

    /* Enable debug output if built with --enable-debug */
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    /* Initialize the pre-shared secrets (same for all peers) */
    memset(pms, 0x23, sizeof(pms));
    memset(clientRandom, 0xA5, sizeof(clientRandom));
    memset(serverRandom, 0x5A, sizeof(serverRandom));

    /* Initialize wolfSSL */
    ret = wolfSSL_Init();
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error: wolfSSL_Init failed: %d\n", ret);
        return 1;
    }

    /* Create DTLS 1.2 context */
    ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method());
    if (ctx == NULL) {
        fprintf(stderr, "Error: wolfSSL_CTX_new failed\n");
        goto cleanup;
    }

    /* Set multicast member ID */
    ret = wolfSSL_CTX_mcast_set_member_id(ctx, (unsigned short)myId);
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error: wolfSSL_CTX_mcast_set_member_id failed: %d\n", ret);
        goto cleanup;
    }

    /* Set multicast cipher suite (NULL cipher with SHA256 for integrity) */
    ret = wolfSSL_CTX_set_cipher_list(ctx, "WDM-NULL-SHA256");
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error: wolfSSL_CTX_set_cipher_list failed: %d\n", ret);
        fprintf(stderr, "Make sure wolfSSL is built with --enable-mcast\n");
        goto cleanup;
    }

    /* Setup sockets */
    txSd = setup_tx_socket(&txAddr);
    if (txSd < 0) {
        goto cleanup;
    }

    rxSd = setup_rx_socket();
    if (rxSd < 0) {
        goto cleanup;
    }

    printf("Node %d: Sockets ready, joined multicast group %s:%d\n",
           myId, MCAST_GROUP, MCAST_PORT);

    /* Create SSL object for transmitting */
    sslTx = wolfSSL_new(ctx);
    if (sslTx == NULL) {
        fprintf(stderr, "Error: wolfSSL_new (tx) failed\n");
        goto cleanup;
    }

    /* Create SSL object for receiving */
    sslRx = wolfSSL_new(ctx);
    if (sslRx == NULL) {
        fprintf(stderr, "Error: wolfSSL_new (rx) failed\n");
        goto cleanup;
    }

    /* Set socket file descriptors */
    wolfSSL_set_fd(sslTx, txSd);
    wolfSSL_set_fd(sslRx, rxSd);

    /* Set using nonblock for receive */
    wolfSSL_dtls_set_using_nonblock(sslRx, 1);

    /* Add peers for receive tracking BEFORE setting secret.
     * IMPORTANT: wolfSSL_mcast_peer_add() zeros the peer entry, including
     * nextEpoch. wolfSSL_set_secret() sets nextEpoch for all entries.
     * So peers must be added first, then secret set afterward. */
    for (i = 0; i < NUM_PEERS; i++) {
        if (i != myId) {
            ret = wolfSSL_mcast_peer_add(sslRx, (unsigned short)i, 0);
            if (ret != WOLFSSL_SUCCESS) {
                fprintf(stderr, "Error: wolfSSL_mcast_peer_add(%d) failed: %d\n",
                        i, ret);
                goto cleanup;
            }
            printf("Node %d: Added peer %d to receive tracking\n", myId, i);
        }
    }

    /* Set the pre-shared secret for both SSL objects.
     * This must be called AFTER adding peers so their nextEpoch gets set. */
    ret = wolfSSL_set_secret(sslTx, MCAST_EPOCH, pms, sizeof(pms),
                             clientRandom, serverRandom, suite);
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error: wolfSSL_set_secret (tx) failed: %d\n", ret);
        goto cleanup;
    }

    ret = wolfSSL_set_secret(sslRx, MCAST_EPOCH, pms, sizeof(pms),
                             clientRandom, serverRandom, suite);
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error: wolfSSL_set_secret (rx) failed: %d\n", ret);
        goto cleanup;
    }

    /* Flush any stale packets from previous runs */
    {
        char discard[MSGLEN];
        struct timeval tv_flush = {0, 0};
        fd_set fds;
        while (1) {
            FD_ZERO(&fds);
            FD_SET(rxSd, &fds);
            if (select(rxSd + 1, &fds, NULL, NULL, &tv_flush) <= 0)
                break;
            if (recv(rxSd, discard, sizeof(discard), 0) <= 0)
                break;
        }
    }

    printf("Node %d: Ready. Press Ctrl+C to exit.\n", myId);
    printf("Node %d: Sending messages every 3 seconds...\n\n", myId);

    /* Main loop */
    while (running) {
        fd_set readfds;
        struct timeval tv;
        time_t now;

        FD_ZERO(&readfds);
        FD_SET(rxSd, &readfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(rxSd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        /* Check for incoming messages */
        if (ret > 0 && FD_ISSET(rxSd, &readfds)) {
            unsigned char buf[MSGLEN];
            unsigned short peerId = 0;
            int recvLen;

            recvLen = wolfSSL_mcast_read(sslRx, &peerId, buf, sizeof(buf) - 1);
            if (recvLen > 0) {
                /* Ignore our own messages (loopback) */
                if (peerId != (unsigned short)myId) {
                    buf[recvLen] = '\0';
                    printf("Node %d: Received from peer %d: \"%s\"\n",
                           myId, peerId, buf);
                }
            }
            else if (recvLen < 0) {
                int err = wolfSSL_get_error(sslRx, recvLen);
                if (err != WOLFSSL_ERROR_WANT_READ) {
                    /* Ignore decryption errors from our own packets */
                    if (err != DECRYPT_ERROR && err != VERIFY_MAC_ERROR) {
                        char errStr[80];
                        wolfSSL_ERR_error_string(err, errStr);
                        fprintf(stderr, "Node %d: Read error: %s (%d)\n",
                                myId, errStr, err);
                    }
                }
            }
        }

        /* Send a message every 3 seconds */
        now = time(NULL);
        if (now - lastSend >= 3) {
            char msg[MSGLEN];
            int writeLen;

            snprintf(msg, sizeof(msg), "Hello from node %d, message #%d",
                     myId, ++msgCount);

            writeLen = wolfSSL_write(sslTx, msg, (int)strlen(msg));
            if (writeLen > 0) {
                printf("Node %d: Sent: \"%s\"\n", myId, msg);
            }
            else {
                int err = wolfSSL_get_error(sslTx, writeLen);
                char errStr[80];
                wolfSSL_ERR_error_string(err, errStr);
                fprintf(stderr, "Node %d: Write error: %s (%d)\n",
                        myId, errStr, err);
            }

            lastSend = now;
        }
    }

    printf("\nNode %d: Shutting down...\n", myId);

cleanup:
    if (sslTx != NULL) {
        wolfSSL_free(sslTx);
    }
    if (sslRx != NULL) {
        wolfSSL_free(sslRx);
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
    }
    if (rxSd >= 0) {
        /* Leave multicast group */
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(MCAST_GROUP);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        setsockopt(rxSd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        close(rxSd);
    }
    if (txSd >= 0) {
        close(txSd);
    }
    wolfSSL_Cleanup();

    printf("Node %d: Goodbye!\n", myId);
    return 0;
}
#else
int main()
{
    fprintf(stderr, "Please configure the wolfssl library with --enable-dtls --enable-mcast.\n");
    return EXIT_FAILURE;
}
#endif /* WOLFSSL_DTLS && WOLFSSL_MULTICAST */
