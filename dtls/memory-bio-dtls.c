/* memory-bio-dtls.c
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
#include <semaphore.h>


static void err_sys(const char* msg)
{
    printf("wolfSSL error: %s\n", msg);
    exit(1);
}


#define key "../certs/server-key.pem"
#define cert "../certs/server-cert.pem"
#define cacert "../certs/ca-cert.pem"

typedef struct IO_HANDLES {
    WOLFSSL_BIO* rbio;
    WOLFSSL_BIO* wbio;
    sem_t bioSem;
} IO_HANDLES;

static void* client_thread(void* args)
{
    IO_HANDLES* io = (IO_HANDLES*)args;
    WOLFSSL_CTX* cli_ctx = NULL;
    WOLFSSL* cli_ssl     = NULL;
    int err, ret;

    /* set up client */
    cli_ctx = wolfSSL_CTX_new(
#ifdef WOLFSSL_DTLS13
            wolfDTLSv1_3_client_method()
#else
            wolfDTLSv1_2_client_method()
#endif
    );
    if (cli_ctx == NULL) {
        err_sys("bad client ctx new");
    }

    ret = wolfSSL_CTX_load_verify_locations(cli_ctx, cacert, NULL);
    if (ret != WOLFSSL_SUCCESS) {
        err_sys("bad ca load");
    }

    cli_ssl = wolfSSL_new(cli_ctx);
    if (cli_ctx == NULL) {
        err_sys("bad client new");
    }

    wolfSSL_set_bio(cli_ssl, io->wbio, io->rbio);
    err = 0;
    do {
        sem_wait(&io->bioSem);
        ret = wolfSSL_connect(cli_ssl);
        sem_post(&io->bioSem);
        err =  wolfSSL_get_error(cli_ssl, ret);
    } while (ret != WOLFSSL_SUCCESS && 
      ((err ==  WOLFSSL_ERROR_WANT_READ) || (err == WOLFSSL_ERROR_WANT_WRITE)));
    if (ret != WOLFSSL_SUCCESS) err_sys("bad client tls connect");
    printf("wolfSSL client success!\n");

    do {
        sem_wait(&io->bioSem);
        ret = wolfSSL_write(cli_ssl, "hello memory wolfSSL!", 21);
        sem_post(&io->bioSem);
        err =  wolfSSL_get_error(cli_ssl, ret);
    } while (ret <= 0 && 
      ((err ==  WOLFSSL_ERROR_WANT_READ) || (err == WOLFSSL_ERROR_WANT_WRITE)));

    /* clean up, wolfSSL_free would also free the WOLFSSL_BIO's so set as NULL
     * since they are also being used with srv_ssl and will be free'd there. */
    wolfSSL_set_bio(cli_ssl, NULL, NULL);
    wolfSSL_free(cli_ssl);
    wolfSSL_CTX_free(cli_ctx);

    return NULL;
}


int main()
{
    IO_HANDLES io;
    unsigned char buf[80];
    int ret, err;
    WOLFSSL_CTX* srv_ctx = NULL;
    WOLFSSL* srv_ssl     = NULL;
    pthread_t tid;

    io.rbio = wolfSSL_BIO_new(wolfSSL_BIO_s_mem());
    io.wbio = wolfSSL_BIO_new(wolfSSL_BIO_s_mem());
    sem_init(&io.bioSem, 0, 1);

    /* set up server */
    srv_ctx = wolfSSL_CTX_new(
    #ifdef WOLFSSL_DTLS13
        wolfDTLSv1_3_server_method()
    #else
        wolfDTLSv1_2_server_method()
    #endif
    );
    if (srv_ctx == NULL) err_sys("bad server ctx new");

    ret = wolfSSL_CTX_use_PrivateKey_file(srv_ctx, key, WOLFSSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        err_sys("bad server key file load");
    }

    ret = wolfSSL_CTX_use_certificate_file(srv_ctx, cert, WOLFSSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        err_sys("bad server cert file load");
    }

    srv_ssl = wolfSSL_new(srv_ctx);
    if (srv_ctx == NULL) {
        err_sys("bad server new");
    }

    /* set memory BIO's to use for IO */
    wolfSSL_set_bio(srv_ssl, io.rbio, io.wbio);

    /* start client thread */
    pthread_create(&tid, 0, client_thread, (void*)&io);

    /* accept tls connection without tcp sockets */
    err = 0;
    do {
        sem_wait(&io.bioSem);
        ret = wolfSSL_accept(srv_ssl);
        sem_post(&io.bioSem);
        err =  wolfSSL_get_error(srv_ssl, ret);
    } while (ret != WOLFSSL_SUCCESS && 
      ((err ==  WOLFSSL_ERROR_WANT_READ) || (err == WOLFSSL_ERROR_WANT_WRITE)));
    if (ret != WOLFSSL_SUCCESS) err_sys("bad server tls accept");
    printf("wolfSSL accept success!\n");

    /* read msg post handshake from client */
    memset(buf, 0, sizeof(buf));
    do {
        sem_wait(&io.bioSem);
        ret = wolfSSL_read(srv_ssl, buf, sizeof(buf)-1);
        sem_post(&io.bioSem);
        err =  wolfSSL_get_error(srv_ssl, ret);
    } while (ret != 0 && 
      ((err ==  WOLFSSL_ERROR_WANT_READ) || (err == WOLFSSL_ERROR_WANT_WRITE)));
    if (ret >= 0) {
        printf("client msg = %s\n", buf);
    }

    pthread_join(tid, NULL);

    /* clean up */
    sem_destroy(&io.bioSem);
    wolfSSL_free(srv_ssl); /* This also does free on rbio and wbio */
    wolfSSL_CTX_free(srv_ctx);

    return 0;
}

