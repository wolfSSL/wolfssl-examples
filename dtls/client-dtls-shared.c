/*
 * client-dtls-shared.c
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
 *
 *=============================================================================
 *
 * DTLS client example using multi-thread shared wrapper for
 * instructional/learning purposes.
 */

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/wc_port.h>

#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAXBUF 1024
#define MAXMSGS 100
/* number of writer threads */
#define THREADS 3
#define SERV_PORT 11111

typedef struct SharedDtls {
    wolfSSL_Mutex      shared_mutex;  /* mutex for using */
    WOLFSSL*           ssl;           /* WOLFSSL object being shared */
    int                sd;            /* socket fd */
    struct sockaddr_in servAddr;      /* server sockaddr */
    socklen_t          servSz;        /* length of servAddr */
    char*              recvBuf;       /* I/O recv cb buffer */
    int                recvSz;          /* bytes in recvBuf */
    int                handShakeDone;   /* is the handshake done? */
} SharedDtls;

static int min (int a, int b)
{
    return a > b ? b : a;
}

int dtls_sendto_cb(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    SharedDtls* shared = (SharedDtls*)ctx;

    return sendto(shared->sd, buf, sz, 0,
                  (const struct sockaddr*)&shared->servAddr, shared->servSz);
}

int dtls_recvfrom_cb(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    SharedDtls* shared = (SharedDtls*)ctx;
    int copied;

    if (!shared->handShakeDone) {
        /* get directly from socket */
        return recvfrom(shared->sd, buf, sz, 0, NULL, NULL);
    }
    else {
        /* get the "pushed" datagram from our cb buffer instead */
        copied = min(sz, shared->recvSz);
        memcpy(buf, shared->recvBuf, copied);
        shared->recvSz -= copied;

        return copied;
    }
}

/* DTLS Send function in its own thread */
void* DatagramSend(void* arg)
{
    int         i;
    int         sendSz;
    char        sendBuf[MAXBUF];
    SharedDtls* shared = (SharedDtls*)arg;
    WOLFSSL*    ssl = shared->ssl;
    char        id; /* each thread should have a simple ID at startup */
    char        ids[] = "abcdefgh";
    static int  index = 0;

    wc_LockMutex(&shared->shared_mutex);
    id = ids[index++];
    wc_UnLockMutex(&shared->shared_mutex);

    for (i= 0; i < MAXMSGS; i++) {

        snprintf(sendBuf, MAXBUF, "thread %c sending msg %d\n", id, i);
        sendSz = strlen(sendBuf) + 1;

        wc_LockMutex(&shared->shared_mutex);

        if ( (wolfSSL_write(ssl, sendBuf, sendSz)) != sendSz) {
            printf("wolfSSL_write failed");
        }

        wc_UnLockMutex(&shared->shared_mutex);

    }

    return NULL;
}

int main (int argc, char** argv)
{
    int     	 sockfd = 0, i;
    WOLFSSL* 	 ssl = 0;
    WOLFSSL_CTX* ctx = 0;
    char*        ca = "../certs/ca-cert.pem";
    char*        ecc_ca = "../certs/server-ecc.pem";
    SharedDtls   shared;
    SharedDtls*  recvShared = &shared; /* DTLS Recv var */
    int          sz = 0; /* DTLS Recv var */
    char         recvBuf[MAXBUF]; /* DTLS Recv var */
    char         plainBuf[MAXBUF]; /* DTLS Recv var */
    int          err1;

    if (argc != 2) {
        printf("usage: udpcli <IP address>\n");
        return 1;
    }

    wolfSSL_Init();

    if ( (ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return 1;
    }

    if (wolfSSL_CTX_load_verify_locations(ctx, ca, 0) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", ca);
        return 1;
    }
    if (wolfSSL_CTX_load_verify_locations(ctx, ecc_ca, 0) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", ecc_ca);
        return 1;
    }

    wolfSSL_SetIOSend(ctx, dtls_sendto_cb);
    wolfSSL_SetIORecv(ctx, dtls_recvfrom_cb);

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
    	printf("unable to get ssl object");
        return 1;
    }

    memset(&shared.servAddr, 0, sizeof(shared.servAddr));
    shared.servAddr.sin_family = AF_INET;
    shared.servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, argv[1], &shared.servAddr.sin_addr) < 1) {
        printf("Error and/or invalid IP address");
        return 1;
    }

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       printf("cannot create a socket.");
       return 1;
    }

    shared.sd = sockfd;
    shared.servSz = sizeof(shared.servAddr);
    shared.ssl = ssl;
    shared.handShakeDone = 0;

    if (wc_InitMutex(&shared.shared_mutex) != 0) {
	    printf("wc_InitMutex failed");
        return 1;
    }

    wolfSSL_SetIOWriteCtx(ssl, &shared);
    wolfSSL_SetIOReadCtx(ssl, &shared);

    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
	    err1 = wolfSSL_get_error(ssl, 0);
	    printf("err = %d, %s\n", err1, wolfSSL_ERR_reason_error_string(err1));
	    printf("SSL_connect failed");
        return 1;
    }

    shared.handShakeDone = 1;

    pthread_t tid[THREADS];
    for (i = 0; i < THREADS; i++) {
        pthread_create(&tid[i], NULL, DatagramSend, &shared);
    }

    /* DTLS Recv */
    for (i = 0; i < THREADS*MAXMSGS; i++) {
        /* first get datagram, works in blocking mode too */
        sz = recvfrom(recvShared->sd, recvBuf, MAXBUF, 0, NULL, NULL);

        wc_LockMutex(&recvShared->shared_mutex);

        /* push datagram to our cb, no copy needed! */
        recvShared->recvBuf = recvBuf;
        recvShared->recvSz = sz;

        /* get plaintext */
        if ( (sz = (wolfSSL_read(ssl, plainBuf, MAXBUF-1))) < 0) {
            printf("wolfSSL_write failed");
        }

        wc_UnLockMutex(&recvShared->shared_mutex);

        plainBuf[MAXBUF-1] = '\0';
        printf("got msg %s\n", plainBuf);
    }

    for (i = 0; i < THREADS; i++) {
        pthread_join(tid[i], NULL);
    }

    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wc_FreeMutex(&shared.shared_mutex);
    wolfSSL_Cleanup();

    return 0;
}

