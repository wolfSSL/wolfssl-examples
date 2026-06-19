/* tls-sock-threaded.c
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
 */

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

#if !defined(SINGLE_THREADED) && !defined(NO_WOLFSSL_CLIENT) && \
    !defined(NO_WOLFSSL_SERVER)

#include "sockets.h"
#include "threading.h"
#include "certs.h"

/* Mutex to wait for server to be ready. */
wolfSSL_Mutex server_mutex;

/* Application data to send. */
static const char msgHTTPGet[] = "GET /index.html HTTP/1.0\r\n\r\n";
static const char msgHTTPIndex[] =
    "HTTP/1.1 200 OK\n"
    "Content-Type: text/html\n"
    "Connection: close\n"
    "\n"
    "<html>\n"
    "<head>\n"
    "<title>Welcome to wolfSSL!</title>\n"
    "</head>\n"
    "<body>\n"
    "<p>wolfSSL has successfully performed handshake!</p>\n"
    "</body>\n"
    "</html>\n";


/* Create a new wolfSSL client with a server CA certificate. */
static int wolfssl_client_new(WOLFSSL_CTX** ctx, WOLFSSL** ssl)
{
    int ret = 0;
    WOLFSSL_CTX* client_ctx = NULL;
    WOLFSSL*     client_ssl = NULL;

    /* Create and initialize WOLFSSL_CTX */
    if ((client_ctx = wolfSSL_CTX_new(wolfSSLv23_client_method())) == NULL) {
        printf("ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
    }

    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_load_verify_buffer(client_ctx, CA_CERTS, CA_CERTS_LEN,
                WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load CA certificate\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Create a WOLFSSL object */
        if ((client_ssl = wolfSSL_new(client_ctx)) == NULL) {
            printf("ERROR: failed to create WOLFSSL object\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Return newly created wolfSSL context and object */
        *ctx = client_ctx;
        *ssl = client_ssl;
    }
    else {
        if (client_ssl != NULL)
            wolfSSL_free(client_ssl);
        if (client_ctx != NULL)
            wolfSSL_CTX_free(client_ctx);
    }

    return ret;
}

/* Client connecting to server using TLS */
static int wolfssl_client_connect(WOLFSSL* ssl)
{
    int ret = 0;

    if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
        ret = wolfSSL_get_error(ssl, 0);
        if (ret == WOLFSSL_ERROR_WANT_READ)
            ret = 0;
        else if (ret > 0)
            ret = -1;
    }

    return ret;
}



/* Create a new wolfSSL server with a certificate for authentication. */
static int wolfssl_server_new(WOLFSSL_CTX** ctx, WOLFSSL** ssl)
{
    int ret = 0;
    WOLFSSL_CTX* server_ctx = NULL;
    WOLFSSL*     server_ssl = NULL;

    /* Create and initialize WOLFSSL_CTX */
    if ((server_ctx = wolfSSL_CTX_new(wolfSSLv23_server_method())) == NULL) {
        printf("ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
    }

    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_use_certificate_buffer(server_ctx, SERVER_CERT,
                SERVER_CERT_LEN, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load server certificate\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Load client certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_use_PrivateKey_buffer(server_ctx, SERVER_KEY,
                SERVER_KEY_LEN, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load server key\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Create a WOLFSSL object */
        if ((server_ssl = wolfSSL_new(server_ctx)) == NULL) {
            printf("ERROR: failed to create WOLFSSL object\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Return newly created wolfSSL context and object */
        *ctx = server_ctx;
        *ssl = server_ssl;
    }
    else {
        if (server_ssl != NULL)
            wolfSSL_free(server_ssl);
        if (server_ctx != NULL)
            wolfSSL_CTX_free(server_ctx);
    }

    return ret;
}

/* Server accepting a client using TLS */
static int wolfssl_server_accept(WOLFSSL* ssl)
{
    int ret = 0;

    if (wolfSSL_accept(ssl) != WOLFSSL_SUCCESS) {
        ret = wolfSSL_get_error(ssl, 0);
        if (ret == WOLFSSL_ERROR_WANT_READ)
            ret = 0;
        else if (ret > 0)
            ret = -1;
    }

    return ret;
}


/* Send application data. */
static int wolfssl_send(WOLFSSL* ssl, const char* msg)
{
    int ret = 0;
    int len;

    printf("Sending:\n%s\n", msg);
    len = wolfSSL_write(ssl, msg, XSTRLEN(msg));
    if (len < 0)
        ret = len;
    else if (len != XSTRLEN(msg))
        ret = -1;

    return ret;
}

/* Receive application data. */
static int wolfssl_recv(WOLFSSL* ssl)
{
    int ret;
    byte reply[256];

    ret = wolfSSL_read(ssl, reply, sizeof(reply)-1);
    if (ret > 0) {
        reply[ret] = '\0';
        printf("Received:\n%s\n", reply);
        ret = 1;
    }
    else if (wolfSSL_want_read(ssl) || wolfSSL_want_write(ssl))
        ret = 0;

    return ret;
}


/* Free the WOLFSSL object and context. */
static void wolfssl_free(WOLFSSL_CTX* ctx, WOLFSSL* ssl)
{
    if (ssl != NULL)
        wolfSSL_free(ssl);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);
}



/* Start the server thread. */
static void start_thread(THREAD_FUNC func, func_args* args, THREAD_TYPE* thread)
{
#if defined(_POSIX_THREADS) && !defined(__MINGW32__)
    pthread_create(thread, 0, func, args);
#elif defined(WOLFSSL_TIRTOS)
    /* Initialize the defaults and set the parameters. */
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.arg0 = (UArg)args;
    taskParams.stackSize = 24*1024;
    *thread = Task_create((Task_FuncPtr)func, &taskParams, NULL);
    if (*thread == NULL) {
        printf("Failed to create new Task\n");
    }
    Task_yield();
#elif defined(USE_WINDOWS_API)
    *thread = (THREAD_TYPE)_beginthreadex(0, 0, func, args, 0, 0);
#endif
}

static void join_thread(THREAD_TYPE thread)
{
#if defined(_POSIX_THREADS) && !defined(__MINGW32__)
    pthread_join(thread, 0);
#elif defined(WOLFSSL_TIRTOS)
    while (1) {
        if (Task_getMode(thread) == Task_Mode_TERMINATED) {
            Task_sleep(5);
            break;
        }
        Task_yield();
    }
#elif defined(USE_WINDOWS_API)
    WaitForSingleObject((HANDLE)thread, INFINITE);
    CloseHandle((HANDLE)thread);
#endif
}


/* Accept the TCP connection from the client. */
int wolfssl_server_accept_tcp(WOLFSSL* ssl, SOCKET_T* fd, SOCKET_T* acceptfd)
{
    int ret = 0;
    SOCKET_T      sockfd   = WOLFSSL_SOCKET_INVALID;
    SOCKET_T      clientfd = WOLFSSL_SOCKET_INVALID;
    word16        port = wolfSSLPort;

    ret = tcp_listen(&sockfd, &port, 1, 0, 0);

    wc_UnLockMutex(&server_mutex);

    if (ret == 0)
        ret = tcp_accept(&sockfd, &clientfd);
    if (ret == 0) {
        *acceptfd = clientfd;
        ret = tcp_set_nonblocking(&clientfd);
    }

    if (ret == 0) {
        printf("Server has accepted client\n");
        if (wolfSSL_set_fd(ssl, clientfd) != WOLFSSL_SUCCESS)
            ret = -1;
    }

    return ret;
}


/* Thread to do the server operations. */
static THREAD_RETURN WOLFSSL_THREAD server_thread(void* args)
{
    int          ret = 0;
    WOLFSSL_CTX* server_ctx = NULL;
    WOLFSSL*     server_ssl = NULL;
    SOCKET_T      sockfd   = WOLFSSL_SOCKET_INVALID;
    SOCKET_T      clientfd = WOLFSSL_SOCKET_INVALID;


    if (ret == 0)
        ret = wolfssl_server_new(&server_ctx, &server_ssl);

    if (ret == 0)
        ret = wolfssl_server_accept_tcp(server_ssl, &sockfd, &clientfd);

    while (ret == 0) {
        ret = wolfssl_server_accept(server_ssl);
        if (ret == 0 && wolfSSL_is_init_finished(server_ssl))
            break;
    }

    /* Receive HTTP request */
    while (ret == 0) {
        ret = wolfssl_recv(server_ssl);
    }
    if (ret == 1)
        ret = 0;
    /* Send HTTP response */
    if (ret == 0)
        ret = wolfssl_send(server_ssl, msgHTTPIndex);

    printf("Server Return: %d\n", ret);

    wolfssl_free(server_ctx, server_ssl);

    if (clientfd != WOLFSSL_SOCKET_INVALID)
        CloseSocket(clientfd);
    if (sockfd != WOLFSSL_SOCKET_INVALID)
        CloseSocket(sockfd);

#if defined(HAVE_ECC) && defined(FP_ECC)
    wc_ecc_fp_free();  /* free per thread cache */
#endif

#ifndef WOLFSSL_TIRTOS
    return 0;
#endif
}

/* TCP connect to the server. */
int wolfssl_client_connect_tcp(WOLFSSL* ssl, SOCKET_T* fd)
{
    int              ret = 0;

    ret = tcp_connect(fd, wolfSSLIP, wolfSSLPort, 0, 0, ssl);
    if (ret == 0)
        ret = tcp_set_nonblocking(fd);

    if (ret == 0) {
        printf("Client has connected to server\n");
        if (wolfSSL_set_fd(ssl, *fd) != WOLFSSL_SUCCESS)
            ret = -1;
    }

    return ret;
}

/* Client connection operations */
static int client()
{
    int          ret = 0;
    WOLFSSL_CTX* client_ctx = NULL;
    WOLFSSL*     client_ssl = NULL;
    SOCKET_T     sockfd = WOLFSSL_SOCKET_INVALID;

    /* Create a new client connection */
    ret = wolfssl_client_new(&client_ctx, &client_ssl);

    if (ret == 0)
        wolfssl_client_connect_tcp(client_ssl, &sockfd);

    /* Keep trying to connect until handshake finished */
    while (ret == 0) {
        ret = wolfssl_client_connect(client_ssl);
        if (ret == 0 && wolfSSL_is_init_finished(client_ssl))
            break;
    }
    if (ret == 0)
        printf("Handshake complete\n");

    /* Send HTTP request */
    if (ret == 0)
        ret = wolfssl_send(client_ssl, msgHTTPGet);
    /* Receive HTTP response */
    while (ret == 0)
        ret = wolfssl_recv(client_ssl);
    if (ret == 1)
        ret = 0;

    printf("Client Return: %d\n", ret);

    /* Free client connection data */
    wolfssl_free(client_ctx, client_ssl);

    if (sockfd != WOLFSSL_SOCKET_INVALID)
        CloseSocket(sockfd);

    return ret;
}

int main(int argc, char* argv[])
{
    int         ret = 0;
    THREAD_TYPE serverThread;

#if defined(DEBUG_WOLFSSL)
    wolfSSL_Debugging_ON();
#endif
    /* Initialise wolfSSL library */
    wolfSSL_Init();

    /* Initialise mutex that synchronises when server is ready for client */
    wc_InitMutex(&server_mutex);

    /* Unlocked by server thread */
    wc_LockMutex(&server_mutex);

    /* Start server */
    start_thread(server_thread, NULL, &serverThread);

    /* Wait for server to be ready */
    wc_LockMutex(&server_mutex);
    wc_UnLockMutex(&server_mutex);

    /* Do client */
    client();

    /* Cleanup finished thread */
    join_thread(serverThread);

    /* Cleanup wolfSSL library */
    wolfSSL_Cleanup();

    printf("Done\n");

    return (ret == 0) ? 0 : 1;
}

#else

int main(int argc, char* argv[])
{
    printf("Threading and TLS client and server required - compile wolfSSL without SINGLE_THREAED\n");
    return 0;
}

#endif
