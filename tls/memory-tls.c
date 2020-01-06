/* memory-tls.c
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


/* in memory TLS connection with I/O callbacks, no sockets
 *
 gcc -Wall memory-tls.c  -l wolfssl -lpthread

*/

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

/* client messages to server in memory */
unsigned char to_server[1024*17];
int server_bytes;
int server_write_idx;
int server_read_idx;
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  server_cond  = PTHREAD_COND_INITIALIZER;

/* server messages to client in memory */
unsigned char to_client[1024*17];
int client_bytes;
int client_write_idx;
int client_read_idx;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  client_cond  = PTHREAD_COND_INITIALIZER;


/* server send callback */
int ServerSend(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    pthread_mutex_lock(&client_mutex);

    memcpy(&to_client[client_write_idx], buf, sz);
    client_write_idx += sz;
    client_bytes += sz;

    pthread_cond_signal(&client_cond);
    pthread_mutex_unlock(&client_mutex);

    return sz;
}


/* server recv callback */
int ServerRecv(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    pthread_mutex_lock(&server_mutex);

    while (server_bytes - server_read_idx < sz)
        pthread_cond_wait(&server_cond, &server_mutex);

    memcpy(buf, &to_server[server_read_idx], sz);
    server_read_idx += sz;

    if (server_read_idx == server_write_idx) {
        server_read_idx = server_write_idx = 0;
        server_bytes = 0;
    }

    pthread_mutex_unlock(&server_mutex);

    return sz;
}


/* client send callback */
int ClientSend(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    pthread_mutex_lock(&server_mutex);

    memcpy(&to_server[server_write_idx], buf, sz);
    server_write_idx += sz;
    server_bytes += sz;

    pthread_cond_signal(&server_cond);
    pthread_mutex_unlock(&server_mutex);

    return sz;
}


/* client recv callback */
int ClientRecv(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    pthread_mutex_lock(&client_mutex);

    while (client_bytes - client_read_idx < sz)
        pthread_cond_wait(&client_cond, &client_mutex);

    memcpy(buf, &to_client[client_read_idx], sz);
    client_read_idx += sz;

    if (client_read_idx == client_write_idx) {
        client_read_idx = client_write_idx = 0;
        client_bytes = 0;
    }

    pthread_mutex_unlock(&client_mutex);

    return sz;
}


static void err_sys(const char* msg)
{
    printf("wolfSSL error: %s\n", msg);
    exit(1);
}


#define key "../certs/server-key.pem"
#define cert "../certs/server-cert.pem"
#define cacert "../certs/ca-cert.pem"


static void* client_thread(void* args)
{
    /* set up client */
    WOLFSSL_CTX* cli_ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    if (cli_ctx == NULL) err_sys("bad client ctx new");

    int ret = wolfSSL_CTX_load_verify_locations(cli_ctx, cacert, NULL);
    if (ret != SSL_SUCCESS) err_sys("bad ca load");

    wolfSSL_SetIOSend(cli_ctx, ClientSend);
    wolfSSL_SetIORecv(cli_ctx, ClientRecv);

    WOLFSSL* cli_ssl = wolfSSL_new(cli_ctx);
    if (cli_ctx == NULL) err_sys("bad client new");

    ret = wolfSSL_connect(cli_ssl);
    if (ret != SSL_SUCCESS) err_sys("bad client tls connect");
    printf("wolfSSL client success!\n");

    ret = wolfSSL_write(cli_ssl, "hello memory wolfSSL!", 21);

    /* clean up */
    wolfSSL_free(cli_ssl);
    wolfSSL_CTX_free(cli_ctx);

    return NULL;
}


int main()
{
    /* set up server */
    WOLFSSL_CTX* srv_ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    if (srv_ctx == NULL) err_sys("bad server ctx new");

    int ret = wolfSSL_CTX_use_PrivateKey_file(srv_ctx, key, SSL_FILETYPE_PEM);
    if (ret != SSL_SUCCESS) err_sys("bad server key file load");

    ret = wolfSSL_CTX_use_certificate_file(srv_ctx, cert, SSL_FILETYPE_PEM);
    if (ret != SSL_SUCCESS) err_sys("bad server cert file load");

    wolfSSL_SetIOSend(srv_ctx, ServerSend);
    wolfSSL_SetIORecv(srv_ctx, ServerRecv);

    WOLFSSL* srv_ssl = wolfSSL_new(srv_ctx);
    if (srv_ctx == NULL) err_sys("bad server new");

    /* start client thread */
    pthread_t tid;
    pthread_create(&tid, 0, client_thread, NULL);

    /* accept tls connection without tcp sockets */
    ret = wolfSSL_accept(srv_ssl);
    if (ret != SSL_SUCCESS) err_sys("bad server tls accept");
    printf("wolfSSL accept success!\n");

    /* read msg post handshake from client */
    unsigned char buf[80];
    memset(buf, 0, sizeof(buf));
    ret = wolfSSL_read(srv_ssl, buf, sizeof(buf)-1);
    printf("client msg = %s\n", buf);

    /* clean up */
    wolfSSL_free(srv_ssl);
    wolfSSL_CTX_free(srv_ctx);

    return 0;
}
