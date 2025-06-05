/* server-dtls-demux.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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
 * Example of complete DTLS server using a single socket with de-multiplexing,
 * timeout support, and using `poll`. This example his no external dependencies
 * on any event libraries.
 */

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#include "dtls-common.h"

/* We need a constant CID size because the CID field in the record header doesn't have a length field */
#define CID_SIZE 8

static int intCalled = 0;

/**
 * \brief Signal handler for teardown.
 *
 * \param signum Signal number.
 */
static void teardown(int signum)
{
    intCalled = 1;
}

#define APP_DATA_WAIT 30 /* How long we will wait for application data after completing the handshake */
#define QUICK_DIV      4 /* Our quick timeout divider. Used only for DTLS 1.3. */
#define MAX_HS_TIME   10 /* Maximum time we allow a connection to be in the handshake phase. This is
                          * important to check because we want to limit the ability for malicious clients
                          * to stall and use up server resources. */

/**
 * \struct ConnList
 * \brief Structure to hold connection information.
 */
struct ConnList {
    WOLFSSL* ssl; /**< WOLFSSL object for the connection */
    time_t t_started; /**< Time when the connection started */
    int id; /**< ID number of the connection */
    struct ConnList* next; /**< Pointer to the next connection in the list */
};

/**
 * \struct DtlsTimeout
 * \brief Structure to hold timeout information.
 */
struct DtlsTimeout {
    struct timespec ts; /**< Time when the timeout should occur */
    struct ConnList* conn; /**< Pointer to the connection associated with the timeout */
    struct DtlsTimeout* next; /**< Pointer to the next timeout in the list */
};

/**
 * \brief Create a new WOLFSSL_CTX object.
 *
 * \return Pointer to the new WOLFSSL_CTX object, or NULL on error.
 */
WOLFSSL_CTX* newCTX(void);

/**
 * \brief Create a new WOLFSSL object.
 *
 * \param ctx Pointer to the WOLFSSL_CTX object.
 * \param fd File descriptor for the socket.
 * \param rng Pointer to the random number generator.
 * \param connList Pointer to the list of connections.
 *
 * \return Pointer to the new WOLFSSL object, or NULL on error.
 */
WOLFSSL* newSSL(WOLFSSL_CTX* ctx, int fd, WC_RNG* rng, struct ConnList* connList);

/**
 * \brief Create a new socket.
 *
 * \return File descriptor for the new socket, or INVALID_SOCKET on error.
 */
int newFD(void);

/**
 * \brief Create a new connection and add it to the connection list.
 *
 * \param ssl Pointer to the WOLFSSL object.
 * \param connList Pointer to the list of connections.
 *
 * \return Pointer to the new connection, or NULL on error.
 */
struct ConnList* newConn(WOLFSSL* ssl, struct ConnList** connList);

/**
 * \brief Free a connection and remove it from the connection list.
 *
 * \param connList Pointer to the list of connections.
 * \param conn Pointer to the connection to be freed.
 * \param tList Pointer to the list of timeouts.
 */
void freeConn(struct ConnList** connList, struct ConnList* conn, struct DtlsTimeout** tList);

/**
 * \brief Find a connection in the connection list based on the connection ID or peer address
 *
 * \param connList Pointer to the list of connections.
 * \param msg Pointer to the message.
 * \param sz Size of the message.
 * \param peerAddr Pointer to the peer address.
 * \param peerAddrLen Length of the peer address.
 *
 * \return Pointer to the matching connection, or NULL if not found.
 */
struct ConnList* findConn(struct ConnList* connList, byte* msg, ssize_t sz, struct sockaddr* peerAddr, socklen_t peerAddrLen);

/**
 * \brief Handle an existing connection.
 *
 * \param conn Pointer to the connection.
 * \param msg Pointer to the message.
 * \param msgSz Size of the message.
 * \param peerAddr Pointer to the peer address.
 * \param peerAddrLen Length of the peer address.
 *
 * \return 1 on success, 0 on error.
 */
int dispatchExistingConnection(struct ConnList* conn, byte* msg, ssize_t msgSz, struct sockaddr* peerAddr, socklen_t peerAddrLen);

/**
 * \brief Handle a new connection.
 *
 * \param ssl Pointer to the WOLFSSL object.
 * \param msg Pointer to the message.
 * \param msgSz Size of the message.
 * \param peerAddr Pointer to the peer address.
 * \param peerAddrLen Length of the peer address.
 *
 * \return WOLFSSL_SUCCESS on success, WOLFSSL_FATAL_ERROR on error.
 */
int dispatchNewConnection(WOLFSSL* ssl, byte* msg, ssize_t msgSz, struct sockaddr* peerAddr, socklen_t peerAddrLen);

/**
 * \brief Return the next timeout in milliseconds.
 *
 * \param t Pointer to the list of timeouts.
 *
 * \return Next timeout in milliseconds, or -1 if no timeout set.
 */
int getNextTimeout(struct DtlsTimeout* t);

/**
 * \brief Register the next timeout for a connection.
 *
 * \param out Pointer to the list of timeouts.
 * \param conn Pointer to the connection.
 *
 * \return 1 on success, 0 on error.
 */
int registerTimeout(struct DtlsTimeout** out, struct ConnList* conn);

/**
 * \brief Free any timeouts associated with a connection.
 *
 * \param out Pointer to the list of timeouts.
 * \param conn Pointer to the connection.
 */
void freeTimeouts(struct DtlsTimeout** out, struct ConnList* conn);

/**
 * \brief Handle a timeout that occurred for a connection.
 *
 * \param conn Pointer to the connection.
 *
 * \return WOLFSSL_SUCCESS on success, -1 on error.
 */
int handleTimeout(struct ConnList* conn);

/**
 * \brief Handle application data received from a peer.
 *
 * \param ssl Pointer to the WOLFSSL object.
 * \param appData Pointer to the application data.
 * \param appDataSz Size of the application data.
 * \param id ID number of the connection.
 *
 * \return Number of bytes written, or a negative value on error.
 */
static int handleApplicationData(WOLFSSL* ssl, byte* appData, int appDataSz, int id)
{
    /* Process app data from peer. For this example just echo it */
    const void* peer = NULL;
    unsigned int peerSz = 0;
    char* peerName = NULL;
    uint16_t peerPort = 0;
    if (wolfSSL_dtls_get0_peer(ssl, &peer, &peerSz) == WOLFSSL_SUCCESS) {
        peerName = inet_ntoa(((struct sockaddr_in *)peer)->sin_addr);
        peerPort = ntohs(((struct sockaddr_in *)peer)->sin_port);
    }
    printf("(#%d) from %s:%d wrote: %.*s\n", id, peerName, peerPort, appDataSz, appData);
    return wolfSSL_write(ssl, appData, appDataSz);
}

/**
 * \brief Main function for the DTLS server.
 *
 * \return 0 on success, non-zero on error.
 */
int main(void)
{
    int exitVal = 1;
    WOLFSSL_CTX*  ctx = NULL;
    /* List of active or handshaking connections */
    struct ConnList* connList = NULL;
    /* An ordered list of timeouts */
    struct DtlsTimeout* timeouts = NULL;
    /* The stateless listening WOLFSSL object */
    WOLFSSL* listenSSL = NULL;
    int ret = 0;
    /* Our one socket that we read from and send to. We do the demultiplexing ourselves. */
    struct pollfd listenfd;
    WC_RNG* rng = NULL;

    signal(SIGINT, teardown);
    memset(&listenfd, 0, sizeof(listenfd));
    listenfd.fd = INVALID_SOCKET;
    listenfd.events = POLLIN;

    /* Uncomment if you want debugging. */
    // wolfSSL_Debugging_ON();

    if ((rng = wc_rng_new(NULL, 0, NULL)) == NULL) {
        fprintf(stderr, "wc_rng_new error.\n");
        goto cleanup;
    }

    /* Initialize wolfSSL */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init error.\n");
        goto cleanup;
    }

    if ((listenfd.fd = newFD()) == INVALID_SOCKET) {
        fprintf(stderr, "newFD error.\n");
        goto cleanup;
    }

    if ((ctx = newCTX()) == NULL) {
        fprintf(stderr, "newCTX error.\n");
        goto cleanup;
    }

    if ((listenSSL = newSSL(ctx, listenfd.fd, rng, connList)) == NULL) {
        fprintf(stderr, "newSSL error.\n");
        goto cleanup;
    }

    /* main loop */
    while (!intCalled) {
        ret = poll(&listenfd, 1, getNextTimeout(timeouts));
        if (ret < 0) {
            perror("poll");
            goto cleanup;
        }

        if (ret == 0) {
            /* got timeout */
            if (timeouts == NULL)
                goto cleanup;
            /* timeouts is ordered so the first element is the one we need to trigger */
            if (handleTimeout(timeouts->conn) == WOLFSSL_SUCCESS) {
                /* register new timeout */
                if (!registerTimeout(&timeouts, timeouts->conn))
                    goto cleanup;
            }
            else {
                /* error occurred, clean up the connection */
                freeConn(&connList, timeouts->conn, &timeouts);
            }
        }
        else {
            /* data to read */
            byte readBuf[2000];
            ssize_t sz = 0;
            /* peer's address */
            struct sockaddr peerAddr;
            socklen_t peerAddrLen = sizeof(peerAddr);
            struct ConnList *conn = NULL;

            sz = recvfrom(listenfd.fd, readBuf, sizeof(readBuf), 0, &peerAddr, &peerAddrLen);
            if (sz <= 0)
                goto cleanup;

            /* find ssl object */
            conn = findConn(connList, readBuf, sz, &peerAddr, peerAddrLen);
            if (conn != NULL) {
                /* found an existing connection */
                if (!dispatchExistingConnection(conn, readBuf, sz, &peerAddr, peerAddrLen)) {
                    /* cleanup on error */
                    freeConn(&connList, conn, &timeouts);
                    conn = NULL;
                }
            }
            else {
                ret = dispatchNewConnection(listenSSL, readBuf, sz, &peerAddr, peerAddrLen);
                if (ret == WOLFSSL_SUCCESS) {
                    /* Setup new listening object */
                    if ((conn = newConn(listenSSL, &connList)) == NULL) {
                        fprintf(stderr, "newConn error.\n");
                        goto cleanup;
                    }
                    if ((listenSSL = newSSL(ctx, listenfd.fd, rng, connList)) == NULL) {
                        fprintf(stderr, "newSSL error.\n");
                        goto cleanup;
                    }
                }
                else if (ret == WOLFSSL_FATAL_ERROR) {
                    /* clean up the connection */
                    wolfSSL_free(listenSSL);
                    if ((listenSSL = newSSL(ctx, listenfd.fd, rng, connList)) == NULL) {
                        fprintf(stderr, "newSSL error.\n");
                        goto cleanup;
                    }
                }
            }
            /* register timeout */
            if (conn != NULL && !registerTimeout(&timeouts, conn))
                goto cleanup;
        }
    }

    exitVal = 0;
cleanup:
    while (timeouts != NULL) {
        struct DtlsTimeout* t = timeouts;
        timeouts = timeouts->next;
        free(t);
    }
    while (connList != NULL) {
        struct ConnList* c = connList;
        connList = connList->next;
        wolfSSL_free(c->ssl);
        free(c);
    }
    wc_rng_free(rng);
    wolfSSL_CTX_free(ctx);
    wolfSSL_free(listenSSL);
    if (listenfd.fd != INVALID_SOCKET)
        close(listenfd.fd);
    return intCalled ? 0 : exitVal;
}

WOLFSSL_CTX* newCTX(void)
{
    WOLFSSL_CTX* ctx = NULL;
    /* Use highest available DTLS version with possible downgrade */
    if ((ctx = wolfSSL_CTX_new(wolfDTLS_server_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return NULL;
    }
    /* Load CA certificates */
    if (wolfSSL_CTX_load_verify_locations(ctx, caCertLoc, 0) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", caCertLoc);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }
    /* Load server certificates */
    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", servCertLoc);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }
    /* Load server Keys */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", servKeyLoc);
        wolfSSL_CTX_free(ctx);
        return NULL;
    }
    /* Set DTLS 1.2 as the minimum protocol version */
    if (wolfSSL_CTX_SetMinVersion(ctx, WOLFSSL_DTLSV1_2) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error in wolfSSL_CTX_SetMinVersion \n");
        wolfSSL_CTX_free(ctx);
        return NULL;
    }
    return ctx;
}

WOLFSSL* newSSL(WOLFSSL_CTX* ctx, int fd, WC_RNG* rng, struct ConnList* connList)
{
    WOLFSSL* ssl = NULL;
    /* Applications should update this secret periodically */
    char *secret = "My secret";
    byte newCid[CID_SIZE];

    /* Create the WOLFSSL Object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "wolfSSL_new error.\n");
        return NULL;
    }
    /* Set the secret for cookie creation */
#if defined(WOLFSSL_SEND_HRR_COOKIE)
    if (wolfSSL_send_hrr_cookie(ssl, (byte*)secret, strlen(secret)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_send_hrr_cookie error.\n");
        wolfSSL_free(ssl);
        return NULL;
    }
#endif
    if (wolfSSL_DTLS_SetCookieSecret(ssl, (byte*)secret, strlen(secret)) != 0) {
        fprintf(stderr, "wolfSSL_DTLS_SetCookieSecret error.\n");
        wolfSSL_free(ssl);
        return NULL;
    }
    /* Set the maximum timeout to 9 so that we don't hold onto resources for too long */
    if (wolfSSL_dtls_set_timeout_max(ssl, 9) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_DTLS_SetCookieSecret error.\n");
        wolfSSL_free(ssl);
        return NULL;
    }
    if (wolfSSL_set_write_fd(ssl, fd) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_set_write_fd error.\n");
        wolfSSL_free(ssl);
        return NULL;
    }

#ifdef WOLFSSL_DTLS_CID
    while (1) {
        struct ConnList* conn = NULL;
        int found = 0;
        /* Generate CID */
        if (wc_RNG_GenerateBlock(rng, newCid, sizeof(newCid)) != 0) {
            fprintf(stderr, "wc_RNG_GenerateBlock error.\n");
            wolfSSL_free(ssl);
            return NULL;
        }
        /* Check that the CID is not in use */
        for (conn = connList; conn != NULL; conn = conn->next) {
            byte* cid = NULL;
            if (wolfSSL_dtls_cid_get0_rx(ssl, &cid) == WOLFSSL_SUCCESS &&
                    memcmp(newCid, cid, CID_SIZE) == 0) {
                found = 1;
                break;
            }
        }
        if (found)
            continue;
        break;
    }
    if (wolfSSL_dtls_cid_use(ssl) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_dtls_cid_use error.\n");
        wolfSSL_free(ssl);
        return NULL;
    }
    if (wolfSSL_dtls_cid_set(ssl, newCid, sizeof(newCid)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_dtls_cid_set error.\n");
        wolfSSL_free(ssl);
        return NULL;
    }
#endif
    wolfSSL_SSLDisableRead(ssl);

    return ssl;
}

int newFD(void)
{
    int fd;
    struct sockaddr_in servAddr;        /* our server's address */

    /* Create a UDP/IP socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
        perror("socket()");
        return INVALID_SOCKET;
    }
    memset((char *)&servAddr, 0, sizeof(servAddr));
    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(SERV_PORT);

    /* We don't set non-blocking because we always poll before reading and we
     * disable reading in the WOLFSSL objects. */

    /* Bind Socket */
    if (bind(fd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind()");
        close(fd);
        return INVALID_SOCKET;
    }
    return fd;
}

struct ConnList* newConn(WOLFSSL* ssl, struct ConnList** connList)
{
    struct ConnList* conn = (struct ConnList*)malloc(sizeof(struct ConnList));
    static int id = 0;
    if (conn == NULL)
        return NULL;
    conn->ssl = ssl;
    conn->t_started = time(NULL);
    conn->next = *connList;
    conn->id = id++;
    *connList = conn;
    return conn;
}

void freeConn(struct ConnList** connList, struct ConnList* conn, struct DtlsTimeout** tList)
{
    struct ConnList* it = *connList; /* iterator */
    struct ConnList** prev = connList;

    freeTimeouts(tList, conn);

    /* Find conn in connList */
    while (it != conn) {
        prev = &it->next;
        it = it->next;
    }

    wolfSSL_free(conn->ssl);

    if (it == conn)
        *prev = conn->next;
    else
        fprintf(stderr, "conn not found in connList\n");

    free(conn);
}

struct ConnList* findConn(struct ConnList* connList, byte* msg, ssize_t sz, struct sockaddr* peerAddr, socklen_t peerAddrLen)
{
    const unsigned char* msgCid = NULL;
    msgCid = wolfSSL_dtls_cid_parse(msg, sz, CID_SIZE);
    for (; connList != NULL; connList = connList->next) {
        const void* peer = NULL;
        unsigned int peerSz = 0;
        if (msgCid != NULL) {
            /* try to match on msgCid */
            unsigned char* connCid = NULL;
            (void)wolfSSL_dtls_cid_get0_rx(connList->ssl, &connCid);
            if (connCid != NULL && memcmp(connCid, msgCid, CID_SIZE) == 0) {
                /* found matching connection */
                break;
            }
        }
        if (wolfSSL_dtls_get0_peer(connList->ssl, &peer, &peerSz) != WOLFSSL_SUCCESS)
            continue;
        if (peerAddrLen == peerSz && memcmp(peer, peerAddr, peerSz) == 0) {
            /* found matching connection */
            break;
        }
    }
    return connList;
}

int dispatchExistingConnection(struct ConnList* conn, byte* msg, ssize_t msgSz, struct sockaddr* peerAddr,
                               socklen_t peerAddrLen)
{
    int ret;
    if (wolfSSL_inject(conn->ssl, msg, msgSz) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_inject error.\n");
        return 0;
    }
    /* set the peer for sending */
    if (wolfSSL_dtls_set_pending_peer(conn->ssl, peerAddr, peerAddrLen) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_dtls_set_peer error.\n");
        return 0;
    }
    if (!wolfSSL_is_init_finished(conn->ssl)) {
        ret = wolfSSL_accept(conn->ssl);
        if (ret != WOLFSSL_SUCCESS &&
                wolfSSL_get_error(conn->ssl, ret) != WOLFSSL_ERROR_WANT_READ)
            return 0;
    }
    else {
        /* re-use msg buffer since output will always be smaller than input */
        int readSz = ret = wolfSSL_read(conn->ssl, msg, msgSz);
        if (ret > 0) {
            ret = handleApplicationData(conn->ssl, msg, readSz, conn->id);
            if (ret <= 0)
                return 0;
        }
        else if (wolfSSL_get_error(conn->ssl, ret) != WOLFSSL_ERROR_WANT_READ)
            return 0;
    }
    return 1;
}

int dispatchNewConnection(WOLFSSL* ssl, byte* msg, ssize_t msgSz, struct sockaddr* peerAddr, socklen_t peerAddrLen)
{
    /* connection not found, continuing with new connection */
    if (wolfSSL_inject(ssl, msg, msgSz) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_inject error.\n");
        return WOLFSSL_FATAL_ERROR;
    }
    /* set the peer for cookie calculation and sending */
    if (wolfSSL_dtls_set_peer(ssl, peerAddr, peerAddrLen) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_dtls_set_peer error.\n");
        return WOLFSSL_FATAL_ERROR;
    }
    return wolfDTLS_accept_stateless(ssl);
}

int handleTimeout(struct ConnList* conn)
{
    int ret;
    if (!wolfSSL_is_init_finished(conn->ssl)) {
        if (time(NULL) - conn->t_started > MAX_HS_TIME) {
            /* Handshake is taking too long. Kill it and let peer try again. */
            /* Try sending an alert. This is just a courtesy to the peer. We don't care if it succeeds. */
            (void)wolfSSL_SendUserCanceled(conn->ssl);
            return -1;
        }
        ret = wolfSSL_dtls_got_timeout(conn->ssl);
    }
    else {
        /* app data timed out. terminate as stale connection */
        /* Too long waiting for peer data. Shutdown the connection.
         * Don't wait for a response from the peer. */
        fprintf(stderr, "Closing connection after timeout\n");
        (void)wolfSSL_shutdown(conn->ssl);
        ret = -1;
    }
    return ret;
}

int getNextTimeout(struct DtlsTimeout* t)
{
    struct timespec ts;
    int ms;

    if (t == NULL)
        return -1;

    /* use clock_gettime to get ms resolution */
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        return 0;
    }

    if (ts.tv_sec > t->ts.tv_sec ||
            (ts.tv_sec == t->ts.tv_sec && ts.tv_nsec > t->ts.tv_nsec)) {
        /* timeout already should have occurred */
        return 0;
    }

    ms = (t->ts.tv_sec - ts.tv_sec) * 1000;
    if (ts.tv_nsec <= t->ts.tv_nsec) {
        ms += (t->ts.tv_nsec - ts.tv_nsec) / 1000000;
    }
    else {
        ms += (t->ts.tv_nsec + 1000000000 - ts.tv_nsec) / 1000000;
        ms -= 1000;
    }
    return ms;
}

static void addTimeSpec(struct timespec* ts, WOLFSSL* ssl)
{
    long ns;
    long rem; /* remainder */

    if (!wolfSSL_is_init_finished(ssl)) {
        ns = wolfSSL_dtls_get_current_timeout(ssl) * 1000000000;
#ifdef WOLFSSL_DTLS13
        if (wolfSSL_version(ssl) == DTLS1_3_VERSION && wolfSSL_dtls13_use_quick_timeout(ssl))
            ns /= QUICK_DIV;
#endif
    }
    else {
        /* waiting on app data */
        ns = (long)APP_DATA_WAIT * 1000000000;
    }
    rem         = ns % 1000000000;
    ts->tv_sec += ns / 1000000000;
    if (ts->tv_nsec + rem > 999999999) {
        /* does not fit in tv_nsec member */
        ts->tv_sec++;
        ts->tv_nsec = ts->tv_nsec + rem - 999999999;
    }
    else {
        ts->tv_nsec += rem;
    }
}

int registerTimeout(struct DtlsTimeout** out, struct ConnList* conn)
{
    struct DtlsTimeout* t;
    struct DtlsTimeout* it; /* iterator */
    struct DtlsTimeout** prev;
    struct timespec ts;

    /* clear existing timeouts */
    freeTimeouts(out, conn);

    if (wolfSSL_dtls_get_current_timeout(conn->ssl) == 0)
        return 0;

    /* use clock_gettime to get ms resolution */
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        return 0;
    }
    addTimeSpec(&ts, conn->ssl);

    t = (struct DtlsTimeout*)malloc(sizeof(struct DtlsTimeout));
    if (t == NULL)
        return 0;

    /* time when timeout should occur */
    t->ts = ts;
    t->conn = conn;
    t->next = NULL;

    if (*out == NULL) {
        /* Empty list */
        *out = t;
        return 1;
    }
    /* Find the correct spot for the timeout */
    for (prev = out, it = *out; it != NULL; prev = &it->next, it = it->next) {
        if (t->ts.tv_sec < it->ts.tv_sec)
            break;
        else if (t->ts.tv_sec == it->ts.tv_sec && t->ts.tv_nsec < it->ts.tv_nsec)
            break;
    }
    /* Place it in the correct order in the list */
    *prev = t;
    t->next = it;
    return 1;
}

void freeTimeouts(struct DtlsTimeout** out, struct ConnList* conn)
{
    struct DtlsTimeout* it = *out; /* iterator */
    struct DtlsTimeout** prev = out;

    while (it != NULL) {
        if (it->conn == conn) {
            *prev = it->next;
            free(it);
            it = *prev;
        }
        else {
            prev = &it->next;
            it = it->next;
        }
    }
}
