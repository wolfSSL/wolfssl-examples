/* client-tls-perf.c
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
 * This is an example of a TCP Client that uses non blocking input and output to
 * handle a large number of connections.
*/

#include <sys/epoll.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include <wolfssl/test.h>


/* Default port to listen on. */
#define DEFAULT_PORT     11111
/* The number of concurrent connections to support. */
#define SSL_NUM_CONN     15
/* The number of bytes to read from server. */
#define NUM_READ_BYTES   16384
/* The number of bytes to write to server. */
#define NUM_WRITE_BYTES  16384
/* The maximum number of bytes to send in a run. */
#define MAX_BYTES        -1
/* The maximum number of connections to perform in this run. */
#define MAX_CONNECTIONS  100

/* The command line options. */
#define OPTIONS          "?p:v:l:c:k:A:rn:N:R:W:B:"

/* The default client certificate. */
#define CLI_CERT         "../certs/client-cert.pem"
/* The default client private key. */
#define CLI_KEY          "../certs/client-key.pem"
/* The default CA file for the server. */
#define CA_CERT          "../certs/ca-cert.pem"


/* The states of the SSL connection. */
typedef enum SSLState { INIT, CONNECT, WRITE, READ_WAIT, READ, CLOSE } SSLState;

/* Data for each active connection. */
typedef struct SSLConn {
    /* The socket listening on, reading from and writing to. */
    int sockfd;
    /* The wolfSSL object to perform TLS communications. */
    WOLFSSL* ssl;
    /* Session to resume. */
    WOLFSSL_SESSION* session;
    /* The current state of the SSL/TLS connection. */
    SSLState state;
    /* Last error from connect/read/write. */
    int err;
} SSLConn;

/* The information about SSL/TLS connections. */
typedef struct SSLConnCtx {
    /* An array of active connections. */
    SSLConn* sslConn;
    /* The number of active connections. */
    int numConns;
    /* Count of currently active connections. */
    int cnt;

    /* Resume the session on subsequent connections. */
    int resume;

    /* The buffer for the server data. */
    char* buffer;
    /* Size of the server data buffer. */
    int bufferLen;
    /* The data to reply with. */
    char* reply;
    /* Number of bytes to write. */
    int replyLen;

    /* Number of created connections. */
    int numCreated;
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

    /* Total time handling connects. */
    double connTime;
    /* Total time handling resumed connections. */
    double resumeTime;
#ifdef WOLFSSL_ASYNC_CRYPT
    /* Total time handling asynchronous operations. */
    double asyncTime;
#endif
    /* Total time handling reading. */
    double readTime;
    /* Total time handling writing. */
    double writeTime;
    /* Total time handling connections. */
    double totalTime;
} SSLConn_CTX;


static void SSLConn_Free(SSLConn_CTX* ctx);


/* The index of the command line option. */
int myoptind = 0;
/* The current command line option. */
char* myoptarg = NULL;
#ifdef WOLFSSL_ASYNC_CRYPT
/* Global device identifier. */
static int devId = INVALID_DEVID;
#endif


/* Get the wolfSSL client method function for the specified version.
 *
 * version  Protocol version to use.
 * returns The client method function or NULL when version not supported.
 */
static wolfSSL_method_func SSL_GetMethod(int version)
{
    wolfSSL_method_func method = NULL;

    switch (version) {
#ifndef NO_OLD_TLS
    #ifdef WOLFSSL_ALLOW_SSLV3
        case 0:
            method = wolfSSLv3_client_method_ex;
            break;
    #endif

    #ifndef NO_TLS
        #ifdef WOLFSSL_ALLOW_TLSV10
        case 1:
            method = wolfTLSv1_client_method_ex;
            break;
        #endif

        #ifndef NO_OLD_TLS
        case 2:
            method = wolfTLSv1_1_client_method_ex;
            break;
        #endif
    #endif
#endif

#ifndef NO_TLS
        case 3:
            method = wolfTLSv1_2_client_method_ex;
            break;
#endif
    }

    return method;
}


/* Write data to a server.
 *
 * ssl         The wolfSSL object.
 * reply       The data to send to the server.
 * replyLen    The length of the data to send to the server.
 * totalBytes  The total number of bytes sent to servers.
 * writeTime   The amount of time spent writing data to server.
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
        fprintf(stderr, "The server has closed the connection!\n");
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
    fprintf(stderr, "wolfSSL_read/write error = %d\n", error);
    return 0;
}

/* Reads data from a server.
 *
 * ssl         The wolfSSL object.
 * buffer      The buffer to place server data into.
 * len         The length of the buffer.
 * totalBytes  The total number of bytes read from servers.
 * readTime    The amount of time spent reading data from server.
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
        fprintf(stderr, "The server has closed the connection!\n");
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
    fprintf(stderr, "wolfSSL_read/write error = %d\n", error);
    return 0;
}

/* Connect/negotiate a secure connection.
 *
 * ssl         The wolfSSL object.
 * resume      Resume the session.
 * session     Session to resume.
 * connTime    The amount of time spent connecting to the server.
 * resumeTime  The amount of time spent resuming a connection with the server.
 * returns 0 on failure, 1 on success, 2 on want read and 3 on want write.
 */
static int SSL_Connect(WOLFSSL* ssl, int resume, WOLFSSL_SESSION* session,
                       double* connTime, double* resumeTime)
{
    int ret;
    int error;
    double start;

    start = current_time(1);
    if (resume && (session != NULL))
        wolfSSL_set_session(ssl, session);
    /* Connect to the server. */
    ret = wolfSSL_connect(ssl);
    if (!wolfSSL_session_reused(ssl))
        *connTime += current_time(0) - start;
    else
        *resumeTime += current_time(0) - start;
    if (ret == 0) {
        fprintf(stderr, "The server has closed the connection!\n");
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
    if (error == 0)
        return EXIT_SUCCESS;

    /* Cannot do anything about other errors. */
    fprintf(stderr, "wolfSSL_read/write error = %d\n", error);
    return 0;
}

/* Create a random reply.
 *
 * reply      The buffer to put the random data into.
 * replyLen   The amount of data to generate.
 */
static void RandomReply(char* reply, int replyLen)
{
    int    ret;
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


/* Create a new SSL/TLS connection data object.
 *
 * numConns   The number of concurrent connections.
 * bufferLen  The number of data bytes to read from server.
 * replyLen   The number of data bytes to send to servers.
 * maxConns   The number of connections to process this run.
 *            -1 indicates no maximum.
 * maxBytes   The number of bytes to send this run.
 *            -1 indicates no maximum.
 * returns an allocated and initialized connection data object or NULL on error.
 */
static SSLConn_CTX* SSLConn_New(int numConns, int bufferLen, int replyLen,
                                int maxConns, int maxBytes, int resume)
{
    SSLConn_CTX* ctx;
    int          i;

    ctx = (SSLConn_CTX*)malloc(sizeof(*ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(*ctx));

    ctx->resume = resume;
    ctx->numConns = numConns;
    ctx->bufferLen = bufferLen;
    ctx->replyLen = replyLen;
    ctx->maxConnections = maxConns;
    ctx->maxBytes = maxBytes;

    /* Create an entry for each concurrent connection. */
    ctx->sslConn = (SSLConn*)malloc(ctx->numConns * sizeof(*ctx->sslConn));
    if (ctx->sslConn == NULL) {
        SSLConn_Free(ctx);
        return NULL;
    }
    for (i = 0; i < ctx->numConns; i++) {
        ctx->sslConn[i].sockfd = -1;
        ctx->sslConn[i].ssl = NULL;
        ctx->sslConn[i].session = NULL;
        ctx->sslConn[i].state = INIT;
        ctx->sslConn[i].err = 0;
    }

    /* Create a buffer for server data. */
    ctx->buffer = (char*)malloc(bufferLen);
    if (ctx->buffer == NULL) {
        SSLConn_Free(ctx);
        return NULL;
    }

    /* Create a reply that contains rand data. */
    ctx->reply = (char*)malloc(replyLen);
    if (ctx->reply == NULL) {
        SSLConn_Free(ctx);
        return NULL;
    }
    RandomReply(ctx->reply, replyLen);

    return ctx;
}

/* Free the SSL/TLS connection data.
 *
 * ctx  The connection data.
 */
static void SSLConn_Free(SSLConn_CTX* ctx)
{
    int i;

    if (ctx == NULL)
        return;

    if (ctx->sslConn != NULL) {
        for (i = 0; i < ctx->numConns; i++)
            if (ctx->sslConn[i].ssl != NULL) {
                wolfSSL_free(ctx->sslConn[i].ssl);
                close(ctx->sslConn[i].sockfd);
            }

        if (ctx->sslConn != NULL)
            free(ctx->sslConn);
    }

    if (ctx->buffer != NULL)
        free(ctx->buffer);

    if (ctx->reply != NULL)
        free(ctx->reply);

    free(ctx);
}

/* Close an active connection.
 *
 * ctx      The SSL/TLS connection data.
 * sslConn  The SSL connection to close.
 */
static void SSLConn_Close(SSLConn_CTX* ctx, SSLConn* sslConn)
{
    if (wolfSSL_session_reused(sslConn->ssl))
        ctx->numResumed++;
    ctx->numConnections++;

    if (ctx->resume)
        sslConn->session = wolfSSL_get_session(sslConn->ssl);
    wolfSSL_free(sslConn->ssl);
    sslConn->ssl = NULL;

    close(sslConn->sockfd);
    sslConn->sockfd = -1;

    sslConn->state = INIT;

    ctx->cnt--;
}

/* Checks wether this run is done i.e. maximum number of connections or bytes
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

/* Connects to the server.
 *
 * ctx      The SSL/TLS connection data.
 * sslCtx   The SSL/TLS context.
 * sockfd   The socket for communicating with the server.
 * sslConn  The SSL connection.
 * returns EXIT_SUCCESS if a new connection connected or EXIT_FAILURE otherwise.
 */
static int SSLConn_Connect(SSLConn_CTX* ctx, WOLFSSL_CTX* sslCtx, int sockfd,
                           SSLConn* sslConn)
{
    ctx->numCreated++;

    /* Setup connection. */
    sslConn->sockfd = sockfd;
    if ((sslConn->ssl = wolfSSL_new(sslCtx)) == NULL) {
        fprintf(stderr, "wolfSSL_new error.\n");
        return EXIT_FAILURE;
    }
    sslConn->state = CONNECT;
    /* Set the socket to communicate over. */
    wolfSSL_set_fd(sslConn->ssl, sslConn->sockfd);

    return EXIT_SUCCESS;
}

/* Select on the socket and wait.
 *
 * socketfd  The socket to select on.
 * to_sec    The length of time that select waits for.
 * returns TEST_SELECT when the select times out, -1 on select error, 0 when
 * there is an error on the socket and 1 on success.
 */
static int TCP_Select(SOCKET_T socketfd, int to_sec)
{
    fd_set recvfds, errfds;
    SOCKET_T nfds = socketfd + 1;
    struct timeval timeout = { (to_sec > 0) ? to_sec : 0, 0};
    int result;

    FD_ZERO(&recvfds);
    FD_SET(socketfd, &recvfds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    result = select(nfds, &recvfds, NULL, &errfds, &timeout);

    if (result == 0)
        return -1;
    else if (result > 0) {
        if (FD_ISSET(socketfd, &recvfds))
            return 1;
        else if(FD_ISSET(socketfd, &errfds))
            return 0;
    }

    return TEST_SELECT_FAIL;
}

/* Read/write from/to server at the specified socket.
 *
 * ctx      The SSL/TLS connection data.
 * sslConn  The SSL connection.
 * returns EXIT_FAILURE on failure and EXIT_SUCCESS otherwise.
 */
static int SSLConn_ReadWrite(SSLConn_CTX* ctx, SSLConn* sslConn)
{
    int ret = 0;
    int len;

    switch (sslConn->state) {
        case INIT:
        case CLOSE:
            break;

        /* Perform TLS handshake. */
        case CONNECT:
            ret = SSL_Connect(sslConn->ssl, ctx->resume, sslConn->session,
                              &ctx->connTime, &ctx->resumeTime);
            if (ret == 0) {
                sslConn->state = CLOSE;
                return EXIT_FAILURE;
            }

            if (ret == 1) {
                sslConn->state = WRITE;
            }
            break;

        case WRITE:
            len = ctx->replyLen;
            if (ctx->maxBytes > 0) {
                len = min(len, ctx->maxBytes - ctx->totalWriteBytes);
            }
            /* Don't write if we are done. */
            if (len == 0)
                break;

            /* Write application data. */
            ret = SSL_Write(sslConn->ssl, ctx->reply, len,
                            &ctx->totalWriteBytes, &ctx->writeTime);
            if (ret == 0) {
                sslConn->state = CLOSE;
                return EXIT_FAILURE;
            }

            if (ret == 1)
                sslConn->state = READ_WAIT;
            break;

        case READ_WAIT:
            ret = TCP_Select(sslConn->sockfd, 0);
            if (ret != 1)
                break;
            sslConn->state = READ;

        case READ:
            len = ctx->bufferLen;
            if (ctx->maxBytes > 0) {
                len = min(len, ctx->maxBytes - ctx->totalReadBytes);
            }
            /* Don't read if we are done. */
            if (len == 0)
                break;

            /* Read application data. */
            ret = SSL_Read(sslConn->ssl, ctx->buffer, len, &ctx->totalReadBytes,
                           &ctx->readTime);
            if (ret == 0) {
                sslConn->state = CLOSE;
                return EXIT_FAILURE;
            }

            if (ret == 1) {
                if (ctx->maxConnections > 0)
                    sslConn->state = CLOSE;
                else
                    sslConn->state = WRITE;
            }
            break;
    }

    sslConn->err = ret;
    return EXIT_SUCCESS;
}

/* Print the connection statistics.
 *
 * ctx  The SSL/TLS connection data.
 */
static void SSLConn_PrintStats(SSLConn_CTX* ctx)
{
    fprintf(stderr, "wolfSSL Client Benchmark %d bytes\n"
            "\tNum Conns         : %9d\n"
            "\tTotal             : %9.3f ms\n"
            "\tTotal Avg         : %9.3f ms\n"
            "\tt/s               : %9.3f\n"
            "\tConnect           : %9.3f ms\n"
            "\tConnect Avg       : %9.3f ms\n",
            ctx->replyLen,
            ctx->numConnections - ctx->numResumed,
            ctx->totalTime * 1000,
            ctx->totalTime * 1000 / ctx->numConnections,
            ctx->numConnections / ctx->totalTime,
            ctx->connTime * 1000,
            ctx->connTime * 1000 / (ctx->numConnections - ctx->numResumed));
    if (ctx->resume) {
        fprintf(stderr,
                "\tResumed Conns     : %9d\n"
                "\tResume            : %9.3f ms\n"
                "\tResume Avg        : %9.3f ms\n",
                ctx->numResumed,
                ctx->resumeTime * 1000,
                ctx->resumeTime * 1000 / ctx->numResumed);
    }
#ifdef WOLFSSL_ASYNC_CRYPT
    fprintf(stderr,
            "\tAsync             : %9.3f ms\n"
            "\tAsync Avg         : %9.3f ms\n",
            ctx->asyncTime * 1000,
            ctx->asyncTime * 1000 / ctx->numConnections);
#endif
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
 * version     The protocol version.
 * cert        The client certificate for client authentication.
 * key         The client private key for client authentication.
 * verifyCert  The CA certificate for server authentication.
 * cipherList  The list of ciphers, as a string, to negotiate.
 * wolfsslCtx  The new wolfSSL context object.
 * returns EXIT_SUCCESS when a wolfSSL context object is created and
 * EXIT_FAILURE otherwise.
 */
static int WolfSSLCtx_Init(int version, char* cert, char* key, char* verifyCert,
                           char* cipherList, WOLFSSL_CTX** wolfsslCtx)
{
    WOLFSSL_CTX* ctx;
    wolfSSL_method_func method = NULL;

    method = SSL_GetMethod(version);
    if (method == NULL)
        return(EXIT_FAILURE);

    /* Create and initialize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(method(NULL))) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return(EXIT_FAILURE);
    }

#ifdef WOLFSSL_ASYNC_CRYPT
    if (wolfAsync_DevOpen(&devId) != 0) {
        fprintf(stderr, "Async device open failed\nRunning without async\n");
    }

    wolfSSL_CTX_UseAsync(ctx, devId);
#endif

    if (cipherList) {
        if (wolfSSL_CTX_set_cipher_list(ctx, cipherList) != SSL_SUCCESS)
            err_sys("client can't set cipher list 1");
    }

    /* load CA certificates into wolfSSL_CTX. which will verify the server */
    if (wolfSSL_CTX_load_verify_locations(ctx, verifyCert, 0) != SSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", verifyCert);
        return EXIT_FAILURE;
    }

    if (wolfSSL_CTX_use_certificate_chain_file(ctx, cert) != SSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", cert);
        return EXIT_FAILURE;
    }

    if (wolfSSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM)
                                     != SSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", key);
        return EXIT_FAILURE;
    }

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
#ifdef WOLFSSL_ASYNC_CRYPT
    wolfAsync_DevClose(&devId);
#endif
}

/* Create a socket to talk to server on and connect.
 *
 * port      The port the server is listening on.
 * socketfd  The connected socket.
 * returns EXIT_SUCCESS when the socket is connected and EXIT_FAILURE otherwise.
 */
static int CreateSocketConnect(int port, socklen_t* socketfd)
{
    int                on = 1;
    socklen_t          len = sizeof(on);
    struct sockaddr_in serverAddr = {0};
    socklen_t          sockfd;

    /* Create a non-blocking socket to listen on for new connections. */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == (socklen_t)-1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return EXIT_FAILURE;
    }

    /* Set the new socket to be non-blocking. */
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    /* Set the server's address. */
    memset((char *)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port        = htons(port);

    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &on, len) < 0)
        fprintf(stderr, "setsockopt TCP_NODELAY failed\n");

    /* Connect to the server. */
    while (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)))
        ;

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
    printf("-l <str>    Cipher suite list (: delimited)\n");
    printf("-c <file>   Certificate file,           default %s\n", CLI_CERT);
    printf("-k <file>   Key file,                   default %s\n", CLI_KEY);
    printf("-A <file>   Certificate Authority file, default %s\n", CA_CERT);
    printf("-r          Resume session\n");
    printf("-n <num>    Benchmark <num> connections\n");
    printf("-N <num>    <num> concurrent connections\n");
    printf("-R <num>    <num> bytes read from client\n");
    printf("-W <num>    <num> bytes written to client\n");
    printf("-B <num>    Benchmark <num> written bytes\n");
}

/* Main entry point for the program.
 *
 * argc  The count of command line arguments.
 * argv  The command line arguments.
 * returns 0 on success and 1 otherwise.
 */
int main(int argc, char* argv[])
{
    socklen_t    socketfd = -1;
    int          ch;
    WOLFSSL_CTX* ctx = NULL;
    SSLConn_CTX* sslConnCtx;
    word16       port = wolfSSLPort;
    int          resumeSession = 0;
    char*        cipherList = NULL;
    char*        ourCert       = CLI_CERT;
    char*        ourKey        = CLI_KEY;
    char*        verifyCert    = CA_CERT;
    int          version       = SERVER_DEFAULT_VERSION;
    int          numConns      = SSL_NUM_CONN;
    int          numBytesRead  = NUM_READ_BYTES;
    int          numBytesWrite = NUM_WRITE_BYTES;
    int          maxBytes      = MAX_BYTES;
    int          maxConns      = MAX_CONNECTIONS;
    int          i;

    /* Parse the command line arguments. */
    while ((ch = mygetopt(argc, argv, OPTIONS)) != -1) {
        switch (ch) {
            /* Help with command line options. */
            case '?':
                Usage();
                exit(EXIT_SUCCESS);

            /* Port number to connect to. */
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

            /* List of cipher suites to use. */
            case 'l':
                cipherList = myoptarg;
                break;

            /* File name of client certificate for client authentication. */
            case 'c':
                ourCert = myoptarg;
                break;

            /* File name of client private key for client authentication. */
            case 'k':
                ourKey = myoptarg;
                break;

            /* File name of server certificate/CA for peer verification. */
            case 'A':
                verifyCert = myoptarg;
                break;

            /* Resume sessions. */
            case 'r':
                resumeSession = 1;
                break;

            /* Number of connections to make. */
            case 'n':
                maxConns = atoi(myoptarg);
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


#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Initialize wolfSSL and create a context object. */
    if (WolfSSLCtx_Init(version, ourCert, ourKey, verifyCert, cipherList, &ctx)
            == EXIT_FAILURE)
        exit(EXIT_FAILURE);

    /* Create SSL/TLS connection data object. */
    sslConnCtx = SSLConn_New(numConns, numBytesRead, numBytesWrite,
                             maxConns, maxBytes, resumeSession);
    if (sslConnCtx == NULL)
        exit(EXIT_FAILURE);

    /* Keep handling connections until all done. */
    for (i = 0; !SSLConn_Done(sslConnCtx); i = (i + 1) % numConns) {
        SSLConn* sslConn = &sslConnCtx->sslConn[i];

        /* Perform close if in CLOSE state. */
        if (sslConn->state == CLOSE) {
            if (sslConnCtx->numConnections == 0) {
                WOLFSSL_CIPHER* cipher;
                cipher = wolfSSL_get_current_cipher(sslConn->ssl);
                printf("SSL cipher suite is %s\n",
                       wolfSSL_CIPHER_get_name(cipher));
            }
            SSLConn_Close(sslConnCtx, sslConn);
        }

        /* Create TCP connection and connect if in INIT state. */
        if ((sslConn->state == INIT) &&
            ((sslConnCtx->maxConnections <= 0) ||
             (sslConnCtx->numCreated < sslConnCtx->maxConnections))) {
            if (CreateSocketConnect(port, &socketfd) == EXIT_FAILURE) {
                printf("ERROR: failed to connect to server\n");
                exit(EXIT_FAILURE);
            }

            SSLConn_Connect(sslConnCtx, ctx, socketfd, sslConn);
        }

#ifdef WOLFSSL_ASYNC_CRYPT
        if (sslConn->err == 4) {
            int ret;
            double start;

            start = current_time(1);
            ret = wolfSSL_AsyncPoll(sslConn->ssl, WOLF_POLL_FLAG_CHECK_HW);
            sslConnCtx->asyncTime += current_time(0) - start;
            if (ret < 0) {
                printf("ERROR: failed in async polling\n");
                break;
            }
            if (ret == 0)
                continue;
        }
        sslConn->err = 0;
#endif

        /* Handle other SSL states. */
        if (sslConnCtx->totalTime == 0)
            sslConnCtx->totalTime = current_time(1);
        if (SSLConn_ReadWrite(sslConnCtx, sslConn) == EXIT_FAILURE) {
            if (sslConnCtx->maxConnections > 0)
                sslConn->state = CLOSE;
        }
    }

    sslConnCtx->totalTime = current_time(0) - sslConnCtx->totalTime;

    SSLConn_PrintStats(sslConnCtx);
    SSLConn_Free(sslConnCtx);

    WolfSSLCtx_Final(ctx);

    wolfSSL_Cleanup();

    exit(EXIT_SUCCESS);
}

