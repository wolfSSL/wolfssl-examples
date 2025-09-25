/* tls-client-server.c
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

#include "certs.h"

#if !defined(NO_WOLFSSL_CLIENT) && !defined(NO_WOLFSSL_SERVER)

/* I/O buffer size - wolfSSL buffers messages internally as well. */
#define BUFFER_SIZE           2048
/* Size of static buffer for dynamic memory allocation. */
#ifdef WOLFSSL_STATIC_MEMORY_SMALL
    #define STATIC_MEM_SIZE       (24*1024)
#elif defined(HAVE_ECC)
    #define STATIC_MEM_SIZE       (144*1024)
#else
    #define STATIC_MEM_SIZE       (96*1024)
#endif


#ifdef WOLFSSL_STATIC_MEMORY
    /* Hint pointers for dynamic memory allocating from buffer. */
    static WOLFSSL_HEAP_HINT* HEAP_HINT_SERVER;
    static WOLFSSL_HEAP_HINT* HEAP_HINT_CLIENT;

    /* Static buffers to dynamically allocate from. */
    static byte gTestMemoryServer[STATIC_MEM_SIZE];
    static byte gTestMemoryClient[STATIC_MEM_SIZE];
#else
    #define HEAP_HINT_SERVER NULL
    #define HEAP_HINT_CLIENT NULL
#endif /* WOLFSSL_STATIC_MEMORY */


/* Buffer for client connection to allocate dynamic memory from. */
static unsigned char client_buffer[BUFFER_SIZE];
static int client_buffer_sz = 0;
/* Buffer for server connection to allocate dynamic memory from. */
static unsigned char server_buffer[BUFFER_SIZE];
static int server_buffer_sz = 0;


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


/* Client attempts to read data from server. */
static int recv_client(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    if (client_buffer_sz > 0) {
        if (sz > client_buffer_sz)
            sz = client_buffer_sz;
        XMEMCPY(buff, client_buffer, sz);
        if (sz < client_buffer_sz) {
            XMEMMOVE(client_buffer, client_buffer + sz, client_buffer_sz - sz);
        }
        client_buffer_sz -= sz;
    }
    else
        sz = WOLFSSL_CBIO_ERR_WANT_READ;

    return sz;
}

/* Client attempts to write data to server. */
static int send_client(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    if (server_buffer_sz < BUFFER_SIZE)
    {
        if (sz > BUFFER_SIZE - server_buffer_sz)
            sz = BUFFER_SIZE - server_buffer_sz;
        XMEMCPY(server_buffer + server_buffer_sz, buff, sz);
        server_buffer_sz += sz;
    }
    else
        sz = WOLFSSL_CBIO_ERR_WANT_WRITE;

    return sz;
}

/* Server attempts to read data from client. */
static int recv_server(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{

    if (server_buffer_sz > 0) {
        if (sz > server_buffer_sz)
            sz = server_buffer_sz;
        XMEMCPY(buff, server_buffer, sz);
        if (sz < server_buffer_sz) {
            XMEMMOVE(server_buffer, server_buffer + sz, server_buffer_sz - sz);
        }
        server_buffer_sz -= sz;
    }
    else
        sz = WOLFSSL_CBIO_ERR_WANT_READ;

    return sz;
}

/* Server attempts to write data to client. */
static int send_server(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    if (client_buffer_sz < BUFFER_SIZE)
    {
        if (sz > BUFFER_SIZE - client_buffer_sz)
            sz = BUFFER_SIZE - client_buffer_sz;
        XMEMCPY(client_buffer + client_buffer_sz, buff, sz);
        client_buffer_sz += sz;
    }
    else
        sz = WOLFSSL_CBIO_ERR_WANT_WRITE;

    return sz;
}


/* Create a new wolfSSL client with a server CA certificate. */
static int wolfssl_client_new(WOLFSSL_CTX** ctx, WOLFSSL** ssl)
{
    int          ret = 0;
    WOLFSSL_CTX* client_ctx = NULL;
    WOLFSSL*     client_ssl = NULL;

    /* Create and initialize WOLFSSL_CTX */
    if ((client_ctx = wolfSSL_CTX_new_ex(wolfTLSv1_2_client_method(),
                                                   HEAP_HINT_CLIENT)) == NULL) {
        printf("ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
    }

    if (ret == 0) {
        /* Load CA certificates into WOLFSSL_CTX */
        if (wolfSSL_CTX_load_verify_buffer(client_ctx, CA_CERTS, CA_CERTS_LEN,
                 WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load CA certificate\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Register callbacks */
        wolfSSL_SetIORecv(client_ctx, recv_client);
        wolfSSL_SetIOSend(client_ctx, send_client);
    }

    if (ret == 0) {
        /* Create a WOLFSSL object */
        if ((client_ssl = wolfSSL_new(client_ctx)) == NULL) {
            printf("ERROR: failed to create WOLFSSL object\n");
            ret = -1;
        }
    }

    if (ret == 0) {
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
static int wolfssl_client_connect(WOLFSSL* client_ssl)
{
    int ret = 0;

    if (wolfSSL_connect(client_ssl) != WOLFSSL_SUCCESS) {
        if (wolfSSL_want_read(client_ssl)) {
            printf("Client waiting for server\n");
        }
        else if (wolfSSL_want_write(client_ssl)) {
            printf("Client waiting for buffer\n");
        }
        else
            ret = -1;
    }

    return ret;
}


/* Create a new wolfSSL server with a certificate for authentication. */
static int wolfssl_server_new(WOLFSSL_CTX** ctx, WOLFSSL** ssl)
{
    int          ret = 0;
    WOLFSSL_CTX* server_ctx = NULL;
    WOLFSSL*     server_ssl = NULL;

    /* Create and initialize WOLFSSL_CTX */
    if ((server_ctx = wolfSSL_CTX_new_ex(wolfTLSv1_2_server_method(),
                                                   HEAP_HINT_SERVER)) == NULL) {
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
        if (wolfSSL_CTX_use_PrivateKey_buffer(server_ctx,
                SERVER_KEY, SERVER_KEY_LEN, WOLFSSL_FILETYPE_ASN1) !=
                WOLFSSL_SUCCESS) {
            printf("ERROR: failed to load server key\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        /* Register callbacks */
        wolfSSL_SetIORecv(server_ctx, recv_server);
        wolfSSL_SetIOSend(server_ctx, send_server);
    }

    if (ret == 0) {
        /* Create a WOLFSSL object */
        if ((server_ssl = wolfSSL_new(server_ctx)) == NULL) {
            printf("ERROR: failed to create WOLFSSL object\n");
            ret = -1;
        }
    }

    if (ret == 0) {
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
static int wolfssl_server_accept(WOLFSSL* server_ssl)
{
    int ret = 0;

    if (wolfSSL_accept(server_ssl) != WOLFSSL_SUCCESS) {
        if (wolfSSL_want_read(server_ssl)) {
            printf("Server waiting for server\n");
        }
        else if (wolfSSL_want_write(server_ssl)) {
            printf("Server waiting for buffer\n");
        }
        else
            ret = -1;
    }

    return ret;
}


/* Send application data. */
static int wolfssl_send(WOLFSSL* ssl, const char* msg)
{
    int ret;

    printf("%s", msg);
    ret = wolfSSL_write(ssl, msg, XSTRLEN(msg));
    if (ret < XSTRLEN(msg))
        ret = -1;
    else
        ret = 0;

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
        printf("%s", reply);
        ret = 0;
    }

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


/* Display dynamic memory usage statistics. */
static void wolfssl_memstats(WOLFSSL* ssl)
{
#ifdef WOLFSSL_STATIC_MEMORY
    WOLFSSL_MEM_CONN_STATS ssl_stats;

    XMEMSET(&ssl_stats, 0 , sizeof(ssl_stats));

    if (wolfSSL_is_static_memory(ssl, &ssl_stats) != 1)
        printf("static memory was not used with ssl");
    else {
        printf("*** This is memory state before wolfSSL_free is called\n");
        printf("peak connection memory = %d\n", ssl_stats.peakMem);
        printf("current memory in use  = %d\n", ssl_stats.curMem);
        printf("peak connection allocs = %d\n", ssl_stats.peakAlloc);
        printf("current connection allocs = %d\n",ssl_stats.curAlloc);
        printf("total connection allocs   = %d\n",ssl_stats.totalAlloc);
        printf("total connection frees    = %d\n\n", ssl_stats.totalFr);
    }
#endif
}

/* Display dynamic memory usage statistics of the client connection. */
static void wolfssl_client_memstats(WOLFSSL* client_ssl)
{
#ifdef WOLFSSL_STATIC_MEMORY
    printf("Client Memory Stats\n");
#endif
    wolfssl_memstats(client_ssl);
}

/* Display dynamic memory usage statistics of the server connection. */
static void wolfssl_server_memstats(WOLFSSL* server_ssl)
{
#ifdef WOLFSSL_STATIC_MEMORY
    printf("Server Memory Stats\n");
#endif
    wolfssl_memstats(server_ssl);
}

/* Main entry point. */
int main(int argc, char* argv[])
{
    int ret = 0;
    WOLFSSL_CTX* client_ctx = NULL;
    WOLFSSL*     client_ssl = NULL;
    WOLFSSL_CTX* server_ctx = NULL;
    WOLFSSL*     server_ssl = NULL;

#if defined(DEBUG_WOLFSSL)
    wolfSSL_Debugging_ON();
#endif
    /* Initialize wolfSSL library. */
    wolfSSL_Init();

#ifdef WOLFSSL_STATIC_MEMORY
    if (wc_LoadStaticMemory(&HEAP_HINT_SERVER, gTestMemoryServer,
                               sizeof(gTestMemoryServer),
                               WOLFMEM_GENERAL | WOLFMEM_TRACK_STATS, 1) != 0) {
        printf("unable to load static memory");
        ret = -1;
    }
    if (wc_LoadStaticMemory(&HEAP_HINT_CLIENT, gTestMemoryClient,
                               sizeof(gTestMemoryClient),
                               WOLFMEM_GENERAL | WOLFMEM_TRACK_STATS, 1) != 0) {
        printf("unable to load static memory");
        ret = -1;
    }
#endif

    /* Create server and client SSL objects. */
    if (ret == 0)
        ret = wolfssl_server_new(&server_ctx, &server_ssl);
    if (ret == 0)
        ret = wolfssl_client_new(&client_ctx, &client_ssl);

    /* Loop to perform SSL handshake. */
    while (ret == 0) {
        ret = wolfssl_client_connect(client_ssl);
        if (ret == 0)
            ret = wolfssl_server_accept(server_ssl);
        if (ret == 0 && wolfSSL_is_init_finished(client_ssl) &&
                        wolfSSL_is_init_finished(server_ssl)) {
            break;
        }
    }

    if (ret == 0)
        printf("Handshake complete\n");

    /* Send and receive HTTP messages. */
    if (ret == 0) {
        printf("\nClient Sending:\n");
        ret = wolfssl_send(client_ssl, msgHTTPGet);
    }
    if (ret == 0) {
        printf("\nServer Received:\n");
        ret = wolfssl_recv(server_ssl);
    }
    if (ret == 0) {
        printf("\nServer Sending:\n");
        ret = wolfssl_send(server_ssl, msgHTTPIndex);
    }
    if (ret == 0) {
        printf("\nClient Received:\n");
        ret = wolfssl_recv(client_ssl);
    }

    /* Display memory statistics. */
    wolfssl_client_memstats(client_ssl);
    wolfssl_server_memstats(server_ssl);

    /* Dispose of SSL objects. */
    wolfssl_free(client_ctx, client_ssl);
    wolfssl_free(server_ctx, server_ssl);

    /* Cleanup wolfSSL library. */
    wolfSSL_Cleanup();

    if (ret == 0)
        printf("Done\n");
    else {
        char buffer[80];
        printf("Error: %d, %s\n", ret, wolfSSL_ERR_error_string(ret, buffer));
    }

    return (ret == 0) ? 0 : 1;
}

#else

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    printf("Must build wolfSSL with client and server enabled for this example\n");
    return 0;
}

#endif
