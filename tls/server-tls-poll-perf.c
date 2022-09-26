/* server-tls-poll-perf.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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
 *=============================================================================
 *
 * This is an example of a TCP Server that uses non-blocking input and output to
 * handle a large number of connections. Reports performance figures.
*/

#include <poll.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include <wolfssl/test.h>


/* Default port to listen on. */
#define DEFAULT_PORT     11111
/* The number of concurrent connections to support. */
#define SSL_NUM_CONN     15
/* The number of bytes to read from client. */
#define NUM_READ_BYTES   16384
/* The number of bytes to write to client. */
#define NUM_WRITE_BYTES  16384
/* The maximum number of bytes to send in a run. */
#define MAX_BYTES        -1
/* The maximum number of connections to perform in this run. */
#define MAX_CONNECTIONS  100
/* The maximum length of the queue of pending connections. */
#define NUM_CLIENTS      100

/* The command line options. */
#define OPTIONS          "?p:v:al:c:k:A:n:N:R:W:B:"

/* The default server certificate. */
#define SVR_CERT "../certs/server-cert.pem"
/* The default server private key. */
#define SVR_KEY  "../certs/server-key.pem"
/* The default certificate/CA file for the client. */
#define CLI_CERT "../certs/client-cert.pem"

/* The file descriptor of an inactive event slot. */
#define DISABLED_EVENT_SLOT_FD -1
/* The number of maximum connections, including the listener. */
#define MAX_EVENT_SLOTS_WITH_LISTENER(maxConns) (maxConns + 1)
/* The static event slot identifier of the listener,
 * which is the last slot in the POLL events list. */
#define LISTENER_EVENT_SLOT_ID(maxConns) maxConns

/* The states of the SSL connection. */
typedef enum SSLState { ACCEPT, READ, WRITE, CLOSED } SSLState;

/* Type for the SSL connection data. */
typedef struct SSLConn SSLConn;

/* Data for each active connection. */
struct SSLConn {
    /* The socket listening on, reading from and writing to. */
    int sockfd;
    /* The wolfSSL object to perform TLS communications. */
    WOLFSSL* ssl;
    /* The current state of the SSL/TLS connection. */
    SSLState state;
    /* Previous SSL connection data object. */
    SSLConn* prev;
    /* Next SSL connection data object. */
    SSLConn* next;
};

/* The information about SSL/TLS connections. */
typedef struct SSLConn_CTX {
    /* An array of active connections. */
    SSLConn* sslConn;
    /* Free list. */
    SSLConn* freeSSLConn;
    /* Maximum number of active connections. */
    int numConns;
    /* Count of currently active connections. */
    int cnt;
    /* Accepting new connections. */
    int accepting;

    /* Size of the client data buffer. */
    int bufferLen;
    /* Number of bytes to write. */
    int replyLen;

    /* Number of connections handled. */
    int numConnections;
    /* Number of resumed connections handled. */
    int numResumed;
    /* Maximum number of connections to perform. */
    int maxConnections;

    /* Total number of bytes read. */
    int totalReadBytes;
    /* Total number of bytes written. */
    int totalWriteBytes;
    /* Maximum number of bytes to read/write. */
    int maxBytes;

    /* Total time handling accept. */
    double acceptTime;
    /* Total time handling accept - resumed connections. */
    double resumeTime;
    /* Total time handling reading. */
    double readTime;
    /* Total time handling writing. */
    double writeTime;
    /* Total time handling connections. */
    double totalTime;
} SSLConn_CTX;


static void SSLConn_Free(SSLConn_CTX* ctx);
static void SSLConn_Close(SSLConn_CTX* ctx, SSLConn* sslConn);
static void SSLConn_FreeSSLConn(SSLConn_CTX* ctx);


/* The index of the command line option. */
int   myoptind = 0;
/* The current command line option. */
char* myoptarg = NULL;
/* The data to reply with. */
static char reply[NUM_WRITE_BYTES];


/* Get the wolfSSL server method function for the specified version.
 *
 * version  Protocol version to use.
 * returns The server method function or NULL when version not supported.
 */
static wolfSSL_method_func SSL_GetMethod(int version, int allowDowngrade)
{
    wolfSSL_method_func method = NULL;

    switch (version) {
#ifndef NO_OLD_TLS
    #ifdef WOLFSSL_ALLOW_SSLV3
        case 0:
            method = wolfSSLv3_server_method_ex;
            break;
    #endif

    #ifndef NO_TLS
        #ifdef WOLFSSL_ALLOW_TLSV10
        case 1:
            method = wolfTLSv1_server_method_ex;
            break;
        #endif

        #ifndef NO_OLD_TLS
        case 2:
            method = wolfTLSv1_1_server_method_ex;
            break;
        #endif
    #endif
#endif

#ifndef NO_TLS
        case 3:
            method = allowDowngrade ? wolfSSLv23_server_method_ex : wolfTLSv1_2_server_method_ex;
            break;
#endif
    }

    return method;
}


/* Write data to a client.
 *
 * ssl         The wolfSSL object.
 * reply       The data to send to the client.
 * replyLen    The length of the data to send to the client.
 * totalBytes  The total number of bytes sent to clients.
 * writeTime   The amount of time spent writing data to client.
 * returns 0 on failure, 1 on success, 2 on want read and 3 on want write.
 */
static int SSL_Write(WOLFSSL* ssl, char* reply, int replyLen, int* totalBytes,
                     double* writeTime)
{
    int  rwret = 0;
    int  error;
    double start;

    start = current_time(1);
    rwret = wolfSSL_write(ssl, reply, replyLen);
    *writeTime += current_time(0) - start;
    if (rwret == 0) {
        fprintf(stderr, "The client has closed the connection - write!\n");
        return 0;
    }

    if (rwret > 0)
        *totalBytes += rwret;
    if (rwret == replyLen)
        return 1;

    error = wolfSSL_get_error(ssl, 0);
    if (error == SSL_ERROR_WANT_READ)
        return 2;
    if (error == SSL_ERROR_WANT_WRITE)
        return 3;
    if (error == WC_PENDING_E)
        return 4;
    if (error == 0)
        return 1;

    /* Cannot do anything about other errors. */
    fprintf(stderr, "wolfSSL_write error = %d\n", error);
    return 0;
}

/* Reads data from a client.
 *
 * ssl         The wolfSSL object.
 * buffer      The buffer to place client data into.
 * len         The length of the buffer.
 * totalBytes  The total number of bytes read from clients.
 * readTime    The amount of time spent reading data from client.
 * returns 0 on failure, 1 on success, 2 on want read and 3 on want write.
 */
static int SSL_Read(WOLFSSL* ssl, char* buffer, int len, int* totalBytes,
                    double* readTime)
{
    int  rwret = 0;
    int  error;
    double start;

    start = current_time(1);
    rwret = wolfSSL_read(ssl, buffer, len);
    *readTime += current_time(0) - start;
    if (rwret == 0) {
        return 0;
    }

    if (rwret > 0)
        *totalBytes += rwret;

    error = wolfSSL_get_error(ssl, 0);
    if (error == SSL_ERROR_WANT_READ)
        return 2;
    if (error == SSL_ERROR_WANT_WRITE)
        return 3;
    if (error == WC_PENDING_E)
        return 4;
    if (error == 0)
        return 1;

    /* Cannot do anything about other errors. */
    fprintf(stderr, "wolfSSL_read error = %d\n", error);
    return 0;
}

/* Accept/negotiate a secure connection.
 *
 * ssl         The wolfSSL object.
 * acceptTime  The amount of time spent accepting a client.
 * resumeTime  The amount of time spent resuming a connection with a client.
 * returns 0 on failure, 1 on success, 2 on want read and 3 on want write.
 */
static int SSL_Accept(WOLFSSL* ssl, double* acceptTime, double* resumeTime)
{
    int ret;
    int error;
    double start;

    /* Accept the connection. */
    start = current_time(1);
    ret = wolfSSL_accept(ssl);
    if (!wolfSSL_session_reused(ssl))
        *acceptTime += current_time(0) - start;
    else
        *resumeTime += current_time(0) - start;
    if (ret == 0) {
        fprintf(stderr, "The client has closed the connection - accept!\n");
        return 0;
    }

    if (ret == SSL_SUCCESS)
        return 1;

    error = wolfSSL_get_error(ssl, 0);
    if (error == SSL_ERROR_WANT_READ)
        return 2;
    if (error == SSL_ERROR_WANT_WRITE)
        return 3;
    if (error == WC_PENDING_E)
        return 4;

    /* Cannot do anything about other errors. */
    fprintf(stderr, "wolfSSL_accept error = %d (%p)\n", error, ssl);
    return 0;
}

/* Create a new SSL/TLS connection data object.
 *
 * max        The maximum number of concurrent connections.
 * bufferLen  The number of data bytes to read from client.
 * replyLen   The number of data bytes to write to client.
 * maxConns   The number of connections to process this run.
 *            -1 indicates no maximum.
 * maxBytes   The number of bytes to send this run.
 *            -1 indicates no maximum.
 * returns an allocated and initialized connection data object or NULL on error.
 */
static SSLConn_CTX* SSLConn_New(int numConns, int bufferLen, int replyLen,
                                int maxConns, int maxBytes)
{
    SSLConn_CTX* ctx;

    ctx = (SSLConn_CTX*)malloc(sizeof(*ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(*ctx));

    ctx->numConns = numConns;
    ctx->bufferLen = bufferLen;
    ctx->replyLen = replyLen;
    ctx->maxConnections = maxConns;
    ctx->maxBytes = maxBytes;
    ctx->sslConn = NULL;



    return ctx;
}

/* Free the SSL/TLS connection data.
 *
 * ctx  The connection data.
 */
static void SSLConn_Free(SSLConn_CTX* ctx)
{
    if (ctx == NULL)
        return;

    while (ctx->sslConn != NULL)
        SSLConn_Close(ctx, ctx->sslConn);
    SSLConn_FreeSSLConn(ctx);

    free(ctx);
}

/* Close an active connection.
 *
 * ctx      The SSL/TLS connection data.
 * sslConn  The SSL connection data object.
 */
static void SSLConn_Close(SSLConn_CTX* ctx, SSLConn* sslConn)
{
    if (sslConn->state == CLOSED)
        return;

    /* Display cipher suite all connection will use. */
    if (ctx->numConnections == 0) {
        WOLFSSL_CIPHER* cipher;
        cipher = wolfSSL_get_current_cipher(sslConn->ssl);
        printf("SSL cipher suite is %s\n",
               wolfSSL_CIPHER_get_name(cipher));
    }

    if (wolfSSL_session_reused(sslConn->ssl))
        ctx->numResumed++;
    ctx->numConnections++;

    sslConn->state = CLOSED;

    /* Take it out of the double-linked list. */
    if (ctx->sslConn == sslConn)
        ctx->sslConn = sslConn->next;
    if (sslConn->next != NULL)
        sslConn->next->prev = sslConn->prev;
    if (sslConn->prev != NULL)
        sslConn->prev->next = sslConn->next;

    /* Put object at head of free list */
    sslConn->next = ctx->freeSSLConn;
    sslConn->prev = NULL;
    ctx->freeSSLConn = sslConn;

    ctx->cnt--;
}

/* Free the SSL/TLS connections that are closed.
 *
 * ctx  The connection data.
 */
static void SSLConn_FreeSSLConn(SSLConn_CTX* ctx)
{
    SSLConn* sslConn = ctx->freeSSLConn;

    ctx->freeSSLConn = NULL;

    while (sslConn != NULL) {
        SSLConn* next = sslConn->next;

        wolfSSL_free(sslConn->ssl);
        close(sslConn->sockfd);
        free(sslConn);

        sslConn = next;
    }
}

/* Checks whether this run is done i.e. maximum number of connections or bytes
 * have been server.
 *
 * ctx  The SSL/TLS connection data.
 * returns 1 if the run is done or 0 otherwise.
 */
static int SSLConn_Done(SSLConn_CTX* ctx) {
    if (ctx->maxConnections > 0)
        return (ctx->numConnections >= ctx->maxConnections);
    return (ctx->totalWriteBytes >= ctx->maxBytes) &&
           (ctx->totalReadBytes >= ctx->maxBytes);
}

/* Accepts a new connection.
 *
 * ctx      The SSL/TLS connection data.
 * sslCtx   The SSL/TLS context.
 * sockfd   The socket file descriptor to accept on.
 * sslConn  The newly create SSL connection data object.
 * returns EXIT_SUCCESS if a new connection was accepted or EXIT_FAILURE
 * otherwise.
 */
static int SSLConn_Accept(SSLConn_CTX* ctx, WOLFSSL_CTX* sslCtx,
                          socklen_t sockfd, SSLConn** sslConn)
{
    struct sockaddr_in clientAddr = {0};
    socklen_t          size = sizeof(clientAddr);
    SSLConn*           conn;

    if (ctx->cnt == ctx->numConns) {
        fprintf(stderr, "ERROR: Too many connections!\n");
        return EXIT_FAILURE;
    }

    conn = malloc(sizeof(*conn));
    if (conn == NULL)
        return EXIT_FAILURE;

    /* Accept the client connection. */
    conn->sockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &size);
    if (conn->sockfd == -1) {
        free(conn);
        fprintf(stderr, "ERROR: failed to accept\n");
        return EXIT_FAILURE;
    }
    /* Set the new socket to be non-blocking. */
    fcntl(conn->sockfd, F_SETFL, O_NONBLOCK);

    /* Setup SSL/TLS connection. */
    if ((conn->ssl = wolfSSL_new(sslCtx)) == NULL) {
        free(conn);
        fprintf(stderr, "wolfSSL_new error.\n");
        return EXIT_FAILURE;
    }
    /* Set the socket to communicate over into the wolfSSL object. */
    wolfSSL_set_fd(conn->ssl, conn->sockfd);

    conn->state = ACCEPT;
    conn->next = ctx->sslConn;
    conn->prev = NULL;
    if (ctx->sslConn != NULL)
        ctx->sslConn->prev = conn;

    ctx->sslConn = conn;
    ctx->cnt++;

    *sslConn = conn;

    return EXIT_SUCCESS;
}

/* Read/write from/to client at the specified socket.
 *
 * ctx      The SSL/TLS connection data.
 * sslConn  The SSL connection data object.
 * returns EXIT_FAILURE on failure and EXIT_SUCCESS otherwise.
 */
static int SSLConn_ReadWrite(SSLConn_CTX* ctx, SSLConn* sslConn)
{
    int ret;
    int len;

    switch (sslConn->state) {
        case ACCEPT:
            /* Perform TLS handshake. */
            ret = SSL_Accept(sslConn->ssl, &ctx->acceptTime, &ctx->resumeTime);
            if (ret == 0) {
                printf("ERROR: Accept failed\n");
                SSLConn_Close(ctx, sslConn);
                return EXIT_FAILURE;
            }

            if (ret == 1) {
                sslConn->state = READ;
            }
            break;

        case READ:
            {
                char buffer[NUM_READ_BYTES];

                len = ctx->bufferLen;
                if (ctx->maxBytes > 0) {
                    len = min(len, ctx->maxBytes - ctx->totalReadBytes);
                }
                if (len == 0)
                    break;

                /* Read application data. */
                ret = SSL_Read(sslConn->ssl, buffer, len, &ctx->totalReadBytes,
                               &ctx->readTime);
                if (ret == 0) {
                    SSLConn_Close(ctx, sslConn);
                    return EXIT_FAILURE;
                }

                if (ret != 1)
                    break;
                sslConn->state = WRITE;
            }

        case WRITE:
            len = ctx->replyLen;
            if (ctx->maxBytes > 0) {
                len = min(len, ctx->maxBytes - ctx->totalWriteBytes);
            }
            if (len == 0)
                break;

            /* Write application data. */
            ret = SSL_Write(sslConn->ssl, reply, len, &ctx->totalWriteBytes,
                            &ctx->writeTime);
            if (ret == 0) {
                printf("ERROR: Write failed\n");
                SSLConn_Close(ctx, sslConn);
                return EXIT_FAILURE;
            }

            if (ret == 1)
                sslConn->state = READ;
            break;

        case CLOSED:
            break;
    }

    return EXIT_SUCCESS;
}

/* Print the connection statistics.
 *
 * ctx  The SSL/TLS connection data.
 */
static void SSLConn_PrintStats(SSLConn_CTX* ctx)
{
    fprintf(stderr, "wolfSSL Server Benchmark %d bytes\n"
            "\tNum Conns         : %9d\n"
            "\tTotal             : %9.3f ms\n"
            "\tTotal Avg         : %9.3f ms\n"
            "\tt/s               : %9.3f\n"
            "\tAccept            : %9.3f ms\n"
            "\tAccept Avg        : %9.3f ms\n",
            ctx->replyLen,
            ctx->numConnections - ctx->numResumed,
            ctx->totalTime * 1000,
            ctx->totalTime * 1000 / ctx->numConnections,
            ctx->numConnections / ctx->totalTime,
            ctx->acceptTime * 1000,
            ctx->acceptTime * 1000 / (ctx->numConnections - ctx->numResumed));
    if (ctx->numResumed > 0) {
        fprintf(stderr,
                "\tResumed Conns     : %9d\n"
                "\tResume            : %9.3f ms\n"
                "\tResume Avg        : %9.3f ms\n",
                ctx->numResumed,
                ctx->resumeTime * 1000,
                ctx->resumeTime * 1000 / ctx->numResumed);
    }
    fprintf(stderr,
            "\tTotal Read bytes  : %9d bytes\n"
            "\tTotal Write bytes : %9d bytes\n"
            "\tRead              : %9.3f ms (%9.3f MBps)\n"
            "\tWrite             : %9.3f ms (%9.3f MBps)\n",
            ctx->totalReadBytes,
            ctx->totalWriteBytes,
            ctx->readTime * 1000,
            ctx->totalReadBytes / ctx->readTime / 1024 / 1024,
            ctx->writeTime * 1000,
            ctx->totalWriteBytes / ctx->writeTime / 1024 / 1024 );
}


/* Initialize the wolfSSL library and create a wolfSSL context.
 *
 * version      The protocol version.
 * cert         The server's certificate.
 * key          The server's private key matching the certificate.
 * verifyCert   The certificate for client authentication.
 * cipherList   The list of negotiable ciphers.
 * wolfsslCtx  The new wolfSSL context object.
 * returns EXIT_SUCCESS when a wolfSSL context object is created and
 * EXIT_FAILURE otherwise.
 */
static int WolfSSLCtx_Init(int version, int allowDowngrade, char* cert,
    char* key, char* verifyCert, char* cipherList, WOLFSSL_CTX** wolfsslCtx)
{
    WOLFSSL_CTX* ctx;
    wolfSSL_method_func method = NULL;

    method = SSL_GetMethod(version, allowDowngrade);
    if (method == NULL)
        return(EXIT_FAILURE);

    /* Create and initialize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(method(NULL))) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return(EXIT_FAILURE);
    }

    /* Load server certificate into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM)
            != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", cert);
        wolfSSL_CTX_free(ctx);
        return(EXIT_FAILURE);
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM)
            != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", key);
        wolfSSL_CTX_free(ctx);
        return(EXIT_FAILURE);
    }

    /* Setup client authentication. */
    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, 0);
    if (wolfSSL_CTX_load_verify_locations(ctx, verifyCert, 0) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n",
                verifyCert);
        wolfSSL_CTX_free(ctx);
        return(EXIT_FAILURE);
    }

    if (cipherList != NULL) {
        if (wolfSSL_CTX_set_cipher_list(ctx, cipherList) != SSL_SUCCESS) {
            fprintf(stderr, "Server can't set cipher list.\n");
            wolfSSL_CTX_free(ctx);
            return(EXIT_FAILURE);
        }
    }

#ifndef NO_DH
    SetDHCtx(ctx);
#endif

    *wolfsslCtx = ctx;
    return EXIT_SUCCESS;
}

/* Cleanup the wolfSSL context and wolfSSL library.
 *
 * ctx  The wolfSSL context object.
 */
static void WolfSSLCtx_Final(WOLFSSL_CTX* ctx)
{
    wolfSSL_CTX_free(ctx);
}

/* Create a random reply.
 *
 * reply     The buffer to put the random data into.
 * replyLen  The amount of data to generate.
 */
static void RandomReply(char* reply, int replyLen)
{
    int ret;
    WC_RNG rng;

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        fprintf(stderr, "Error: initialize random\n");
        exit(EXIT_FAILURE);
    }

    ret = wc_RNG_GenerateBlock(&rng, (byte*)reply, replyLen);
    wc_FreeRng(&rng);
    if (ret != 0) {
        fprintf(stderr, "Error: initialize random\n");
        exit(EXIT_FAILURE);
    }
}


/* Create a socket to listen on and wait for first client.
 *
 * port        The port to listen on.
 * numClients  The number of clients for listen to support.
 * socketfd    The socket file descriptor to accept on.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE otherwise.
 */
static int CreateSocketListen(int port, int numClients, socklen_t* socketfd) {
    int                 ret;
    socklen_t           sockfd;
    struct sockaddr_in  serverAddr = {0};
    int                 on = 1;
    socklen_t           len = sizeof(on);

    /* Set the server's address. */
    memset((char *)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port        = htons(port);

    /* Create a non-blocking socket to listen on for new connections. */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == (socklen_t)-1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return(EXIT_FAILURE);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, len) < 0)
        fprintf(stderr, "setsockopt SO_REUSEADDR failed\n");
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &on, len) < 0)
        fprintf(stderr, "setsockopt TCP_NODELAY failed\n");

    if (bind(sockfd, (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0) {
        fprintf(stderr, "ERROR: failed to bind\n");
        return(EXIT_FAILURE);
    }

    printf("Waiting for a connection...\n");

    /* Listen for a client to connect. */
    ret = listen(sockfd, numClients);
    if (ret == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        return(EXIT_FAILURE);
    }

    *socketfd = sockfd;
    return EXIT_SUCCESS;
}

/* Display the usage for the program.
 */
static void Usage(void)
{
    printf("perf " LIBWOLFSSL_VERSION_STRING
           " (NOTE: All files relative to wolfSSL home dir)\n");
    printf("-?          Help, print this usage\n");
    printf("-p <num>    Port to listen on, not 0, default %d\n", wolfSSLPort);
    printf("-v <num>    SSL version [0-3], SSLv3(0) - TLS1.2(3)), default %d\n",
                                 SERVER_DEFAULT_VERSION);
    printf("-a          Allow TLS version downgrade\n");
    printf("-l <str>    Cipher suite list (: delimited)\n");
    printf("-c <file>   Certificate file,           default %s\n", SVR_CERT);
    printf("-k <file>   Key file,                   default %s\n", SVR_KEY);
    printf("-A <file>   Certificate Authority file, default %s\n", CLI_CERT);
    printf("-n <num>    Benchmark <num> connections\n");
    printf("-N <num>    <num> concurrent connections\n");
    printf("-R <num>    <num> bytes read from client\n");
    printf("-W <num>    <num> bytes written to client\n");
    printf("-B <num>    Benchmark <num> written bytes\n");
}

/*
 * Allocates an event slot in a given POLL events list.
 * The complexity of this operation is O(events). For a reduced complexity,
 * prefer the example leveraging EPOLL.
 * 
 * events           The POLL event list.
 * number_of_events The number of events in the list.
 * returns the event slot id (>= 0) on success, DISABLED_EVENT_SLOT_FD otherwise.
 * 
 */
static int allocate_event_slot(struct pollfd* events, int number_of_events)
{
    int i;

    /* Walk through the POLL events list to find the first free slot. */
    for (i = 0; i < number_of_events; i++) {
        if (events[i].fd == DISABLED_EVENT_SLOT_FD) {
            return i;
        }
    }

    return -1;
}

/*
 * Releases an event slot previously allocated in the list of POLL events.
 * The complexity of this operation is O(1).
 * 
 * events           The POLL event list.
 * event_slot_id    The event slot identifier to release.
 */
static void release_event_slot(struct pollfd* events, int event_slot_id)
{
    events[event_slot_id].fd = DISABLED_EVENT_SLOT_FD;
    events[event_slot_id].events = 0;
    events[event_slot_id].revents = 0;
}

/* Main entry point for the program.
 *
 * argc  The count of command line arguments.
 * argv  The command line arguments.
 * returns 0 on success and 1 otherwise.
 */
int main(int argc, char* argv[])
{
    int                 ret = 0;
    socklen_t           socketfd = -1;
    struct pollfd*      events = NULL;
    SSLConn**           events_data = NULL;
    int                 ch;
    int                 i;
    WOLFSSL_CTX*        ctx = NULL;
    SSLConn_CTX*        sslConnCtx;
    word16              port          = wolfSSLPort;
    char*               cipherList    = NULL;
    char*               ourCert       = SVR_CERT;
    char*               ourKey        = SVR_KEY;
    char*               verifyCert    = CLI_CERT;
    int                 version       = SERVER_DEFAULT_VERSION;
    int                 allowDowngrade= 0;
    int                 numConns      = SSL_NUM_CONN;
    int                 numBytesRead  = NUM_READ_BYTES;
    int                 numBytesWrite = NUM_WRITE_BYTES;
    int                 maxBytes      = MAX_BYTES;
    int                 maxConns      = MAX_CONNECTIONS;
    int                 numClients    = NUM_CLIENTS;

    /* Parse the command line arguments. */
    while ((ch = mygetopt(argc, argv, OPTIONS)) != -1) {
        switch (ch) {
            /* Help with command line options. */
            case '?':
                Usage();
                exit(EXIT_SUCCESS);

            /* Port number to listen on. */
            case 'p':
                port = (word16)atoi(myoptarg);
                break;

            /* Version of SSL/TLS to use. */
            case 'v':
                version = atoi(myoptarg);
                if (version < 0 || version > 3) {
                    Usage();
                    exit(MY_EX_USAGE);
                }
                break;
            case 'a':
                allowDowngrade = 1;
                break;

            /* List of cipher suites to use. */
            case 'l':
                cipherList = myoptarg;
                break;

            /* File name of server certificate for authentication. */
            case 'c':
                ourCert = myoptarg;
                break;

            /* File name of server private key for authentication. */
            case 'k':
                ourKey = myoptarg;
                break;

            /* File name of client certificate/CA for peer verification. */
            case 'A':
                verifyCert = myoptarg;
                break;

            /* Number of connections to make. */
            case 'n':
                maxConns  = atoi(myoptarg);
                if (maxConns < 0 || maxConns > 1000000) {
                    Usage();
                    exit(MY_EX_USAGE);
                }
                maxBytes = 0;
                break;

            /* Number of conncurrent connections to use. */
            case 'N':
                numConns  = atoi(myoptarg);
                if (numConns < 0 || numConns > 1000000) {
                    Usage();
                    exit(MY_EX_USAGE);
                }
                break;

            /* Number of bytes to read each call. */
            case 'R':
                numBytesRead = atoi(myoptarg);
                if (numBytesRead <= 0) {
                    Usage();
                    exit(MY_EX_USAGE);
                }
                break;

            /* Number of bytes to write each call. */
            case 'W':
                numBytesWrite = atoi(myoptarg);
                if (numBytesWrite <= 0) {
                    Usage();
                    exit(MY_EX_USAGE);
                }
                break;

            /* Maximum number of read and write bytes (separate counts). */
            case 'B':
                maxBytes = atoi(myoptarg);
                if (maxBytes <= 0) {
                    Usage();
                    exit(MY_EX_USAGE);
                }
                maxConns = 0;
                break;

            /* Unrecognized command line argument. */
            default:
                Usage();
                exit(MY_EX_USAGE);
        }
    }

    /* Allocate space for POLL events to be stored. */
    events = (struct pollfd*)malloc(MAX_EVENT_SLOTS_WITH_LISTENER(maxConns) * sizeof(struct pollfd));
    if (events == NULL)
        exit(EXIT_FAILURE);

    /* Allocate space for the POLL events data to be stored. */
    events_data = (SSLConn**)malloc(MAX_EVENT_SLOTS_WITH_LISTENER(maxConns) * sizeof(SSLConn*));
    if (events_data == NULL)
        exit(EXIT_FAILURE);

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Initialize wolfSSL and create a context object. */
    if (WolfSSLCtx_Init(version, allowDowngrade, ourCert, ourKey, verifyCert, cipherList, &ctx)
            == -1)
        exit(EXIT_FAILURE);

    RandomReply(reply, sizeof(reply));

    /* Create SSL/TLS connection data object. */
    sslConnCtx = SSLConn_New(numConns, numBytesRead, numBytesWrite,
                              maxConns, maxBytes);
    if (sslConnCtx == NULL)
        exit(EXIT_FAILURE);

    /* Create a socket and listen for a client. */
    if (CreateSocketListen(port, numClients, &socketfd) == EXIT_FAILURE)
        exit(EXIT_FAILURE);

    /* Initialize the POLL events data as empty */
    memset(events_data, 0, MAX_EVENT_SLOTS_WITH_LISTENER(maxConns) * sizeof(SSLConn*));

    /* Initialize the POLL events as disabled */
    memset(events, 0, MAX_EVENT_SLOTS_WITH_LISTENER(maxConns) * sizeof(struct pollfd));
    for (i = 0; i < MAX_EVENT_SLOTS_WITH_LISTENER(maxConns); i++) {
        events[i].fd = DISABLED_EVENT_SLOT_FD;
    }

    /* Add the event for communications on listening socket. */
    events[LISTENER_EVENT_SLOT_ID(maxConns)].events = POLLIN;
    events[LISTENER_EVENT_SLOT_ID(maxConns)].fd = socketfd;
    sslConnCtx->accepting = 1;

    /* Keep handling clients until done. */
    while (!SSLConn_Done(sslConnCtx)) {

        /* Wait for events. */
        poll(events, MAX_EVENT_SLOTS_WITH_LISTENER(maxConns), -1);

        /* Process all returned events. */
        for (i = 0; i < MAX_EVENT_SLOTS_WITH_LISTENER(maxConns); i++) {
	    /* If no data is ready to be processed. */
            if (events[i].revents == 0) continue;

            /* Error event on socket. */
            if (!(events[i].revents & POLLIN)) {
                if (events_data[i] == NULL) {
                    /* Not a client, therefore the listening connection. */
                    close(socketfd);
                    socketfd = -1;
                    events[LISTENER_EVENT_SLOT_ID(maxConns)].fd = DISABLED_EVENT_SLOT_FD;
                }
                else {
                    /* Client connection. */
                    events_data[i] = NULL;
                    release_event_slot(events, i);
                    events[LISTENER_EVENT_SLOT_ID(maxConns)].fd = socketfd;
                }
            }
            else if (events_data[i] == NULL) {
                SSLConn* sslConn;

                /* Accept a new client on the listener. */
                ret = SSLConn_Accept(sslConnCtx, ctx, socketfd, &sslConn);
                if (ret  == EXIT_SUCCESS) {
                    int event_slot_id;

                    /* Set POLL to check for events on the new socket. */
                    event_slot_id = allocate_event_slot(events, maxConns);
                    if (event_slot_id == -1) {
                        fprintf(stderr, "ERROR: failed add event to poll because the list is full.\n");
                        exit(EXIT_FAILURE);
                    }
                    events[event_slot_id].events = POLLIN;
                    events[event_slot_id].fd = sslConn->sockfd;
                    events_data[event_slot_id] = sslConn;
                }

                if (sslConnCtx->cnt == sslConnCtx->numConns) {
                    /* Don't accept any more TCP connections. */
                    events[LISTENER_EVENT_SLOT_ID(maxConns)].fd = DISABLED_EVENT_SLOT_FD;
                    sslConnCtx->accepting = 0;
                }
            }
            else {
                if (sslConnCtx->totalTime == 0)
                    sslConnCtx->totalTime = current_time(1);
                SSLConn_ReadWrite(sslConnCtx, events_data[i]);
            }
        }

        SSLConn_FreeSSLConn(sslConnCtx);

        /* Accept more connections again up to the maximum concurrent. */
        if (!sslConnCtx->accepting &&
            sslConnCtx->cnt < sslConnCtx->numConns) {
            events[LISTENER_EVENT_SLOT_ID(maxConns)].fd = socketfd;
            sslConnCtx->accepting = 1;
        }
    }

    sslConnCtx->totalTime = current_time(0) - sslConnCtx->totalTime;

    if (socketfd != -1)
        close(socketfd);
    free(events);
    free(events_data);

    SSLConn_PrintStats(sslConnCtx);
    SSLConn_Free(sslConnCtx);

    WolfSSLCtx_Final(ctx);

    wolfSSL_Cleanup();

    exit(EXIT_SUCCESS);
}

