/* server-dtls.c
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
 * DTLS server for instructional/learning purposes.
 * Utilizes DTLS 1.2.
 * Compile wolfSSL with WOLFSSL_THREADED_CRYPT to have encryption of application
 * packets done in threads.
 */

#include <wolfssl/options.h>
#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <wolfssl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#define SERV_PORT   11111           /* define our server port number */
#define MSGLEN      4096            /* length of read buffer */

struct sockaddr_in servAddr;        /* our server's address */
struct sockaddr_in cliaddr;         /* the client's address */

void sig_handler(const int sig);

/* Read/write thread arguments. */
typedef struct {
    WOLFSSL* ssl;   /* SSL object to read/write with. */
    int cleanup;    /* Cleanup thread as connection closing/closed. */
} threadArgs;

#ifdef WOLFSSL_THREADED_CRYPT
/* Encrypt thread arguments. */
typedef struct {
    int idx;                    /* Index of worker. */
    WOLFSSL* ssl;               /* SSL object to encrypt data in. */
    pthread_mutex_t mutex;      /* Mutex for conditional wait. */
    pthread_cond_t cond;        /* Conditional variable. */
    pthread_mutex_t mutex_ssl;  /* Mutex indicating using SSL object. */
} encArgs;

/* Thread to perform encryption of packets.
 *
 * If asynchronous encryption isn't read and not stopped, wait on conditional
 * variable to be signaled. When ready, encrypt the buffer at the index.
 */
void* thread_do_encrypt(void* args)
{
    encArgs* data = (encArgs*)args;
    int idx = data->idx;

    /* Thread doesn't need to be joined. */
    pthread_detach(pthread_self());

    /* Lock mutex for condtional variable. */
    pthread_mutex_lock(&data->mutex);

    /* Keep going while not in cleanup. */
    while (1) {
        /* Check whether there is data to encrypt. */
        if (!wolfSSL_AsyncEncryptReady(data->ssl, idx)) {
            /* Unlock mutex for use of SSL. */
            pthread_mutex_unlock(&data->mutex_ssl);
            /* Wait for conditional variable to be signaled. */
            pthread_cond_wait(&data->cond, &data->mutex);
        }
        /* Lock mutex for use of SSL. */
        pthread_mutex_lock(&data->mutex_ssl);
        /* Check we are ready to encrypt. */
        if (wolfSSL_AsyncEncryptReady(data->ssl, idx)) {
            wolfSSL_AsyncEncrypt(data->ssl, idx);
        }
    }

    /* Unlock mutex. */
    pthread_mutex_unlock(&data->mutex);
    pthread_mutex_unlock(&data->mutex_ssl);

    /* Destroy mutex and conditional variable. */
    pthread_mutex_destroy(&data->mutex);
    pthread_cond_destroy(&data->cond);
    pthread_mutex_destroy(&data->mutex_ssl);

    pthread_exit(NULL);
}

/* Callback for encryption thread.
 *
 * Signals to encryption thread that the buffer is ready for them
 * to work on.
 */
void thread_enc_signal(void* ctx, WOLFSSL* ssl)
{
    /* Get encryption thread arguments. */
    encArgs* data = (encArgs*)ctx;
    /* Signal conditional veriable to wake up thread. */
    pthread_cond_signal(&data->cond);

    (void)ssl;
}
#endif

/* Reader thread.
 *
 * Only started after handshake complete.
 */
void* Reader(void* openSock)
{
    threadArgs* args = (threadArgs*)openSock;
    int                recvLen = 0;                /* length of message     */
    int                msgLen = MSGLEN;            /* the size of message   */
    unsigned char      buff[MSGLEN];               /* the incoming message  */
    WOLFSSL*           ssl = args->ssl;
    int                result;
    fd_set             recvfds, errfds;
    struct timeval     timeout;
    int                currTimeout;
    int                nb_sockfd;
    int                nfds;

    /* Get the socket to read from so we can wait on data. */
    nb_sockfd = (int) wolfSSL_get_fd(ssl);

    /* No microseconds on timeout. */
    timeout.tv_usec = 0;

    /* Keep going while not in cleanup. */
    while (!args->cleanup) {
        /* Get SSL object's current timeout. */
        currTimeout = wolfSSL_dtls_get_current_timeout(ssl);
        /* Set max file descriptor to be one more than one reading from. */
        nfds = nb_sockfd + 1;
        /* Set seconds of time out. */
        timeout.tv_sec = (currTimeout > 0) ? currTimeout : 0;

        /* Setup file descriptor list to monitor. */
        FD_ZERO(&recvfds);
        FD_SET(nb_sockfd, &recvfds);
        FD_ZERO(&errfds);
        FD_SET(nb_sockfd, &errfds);

        /* Monitor file descriptors. */
        result = select(nfds, &recvfds, NULL, &errfds, &timeout);
        if ((result > 0) && FD_ISSET(nb_sockfd, &errfds)) {
            /* Tell other threads to cleanup. */
            args->cleanup = 1;
            /* Finished reading. */
            break;
        }

        /* If data waiting on receive file descriptor, read it. */
        if (result > 0 && FD_ISSET(nb_sockfd, &recvfds)) {
            /* Read application data. */
            if ((recvLen = wolfSSL_read(ssl, buff, msgLen-1)) < 0) {
                /* Handle errors. */
                int readErr = wolfSSL_get_error(ssl, 0);
                if (readErr != SSL_ERROR_WANT_READ) {
                    /* Tell other threads to cleanup. */
                    args->cleanup = 1;
                    /* Finished reading. */
                    break;
                }
            }
        }
    }

    return NULL;
}

/* Writer thread.
 *
 * Only started after handshake complete.
 */
void* Writer(void* openSock)
{
    threadArgs* args = (threadArgs*)openSock;
    char               msg[] = "I hear you fashizzle!\n";
    unsigned int       msgLen = sizeof(msg);
    WOLFSSL*           ssl = args->ssl;
    int                len;

    /* Keep writing while not in cleanup. */
    while (!args->cleanup) {
        /* Write message and check for error. */
        if ((len = wolfSSL_write(ssl, msg, msgLen)) < 0) {
            /* Tell other threads to cleanup. */
            args->cleanup = 1;
            /* Finished writing. */
            break;
        }
    }

    return NULL;
}

int main(int argc, char** argv)
{
    /* Loc short for "location" */
    char          caCertLoc[] = "certs/ca-cert.pem";
    char          servCertLoc[] = "certs/server-cert.pem";
    char          servKeyLoc[] = "certs/server-key.pem";
    WOLFSSL_CTX*  ctx;
    /* Variables for awaiting datagram */
    int           on = 1;
    int           res = 1;
    int           connfd = 0;
    int           listenfd = 0;   /* Initialize our socket */
    int           flags = fcntl(*(&listenfd), F_GETFL, 0);
    WOLFSSL*      ssl = NULL;
    socklen_t     cliLen;
    socklen_t     len = sizeof(int);
    unsigned char b[MSGLEN];      /* watch for incoming messages */
    threadArgs    args;
    pthread_t     threadidReader;
    pthread_t     threadidWriter;
#ifdef WOLFSSL_THREADED_CRYPT
    pthread_t     threadidEnc;
    int           i;
    encArgs       encArg[WOLFSSL_THREADED_CRYPT_CNT];
#endif

    /* "./config --enable-debug" and uncomment next line for debugging */
    /* wolfSSL_Debugging_ON(); */

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Set ctx to DTLS 1.2 */
    if ((ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method())) == NULL) {
        printf("wolfSSL_CTX_new error.\n");
        return 1;
    }
    /* Load CA certificates */
    if (wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0) !=
            SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        return 1;
    }
    /* Load server certificates */
    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != 
                                                                 SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        return 1;
    }
    /* Load server Keys */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc,
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return 1;
    }

#ifdef WOLFSSL_THREADED_CRYPT
    /* Setup encryption threads. */
    for (i = 0; i < WOLFSSL_THREADED_CRYPT_CNT; i++) {
        encArg[i].idx = i;
        encArg[i].ssl = NULL;
        pthread_mutex_init(&encArg[i].mutex, NULL);
        pthread_cond_init(&encArg[i].cond, NULL);
        pthread_mutex_init(&encArg[i].mutex_ssl, NULL);
        pthread_create(&threadidEnc, NULL, thread_do_encrypt, &encArg[i]);
    }
#endif

    while (1) {
        /* Create a UDP/IP socket */
        if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            printf("Cannot create socket.\n");
            break;
        }
        printf("Socket allocated\n");

        /* clear servAddr each loop */
        memset((char *)&servAddr, 0, sizeof(servAddr));

        /* host-to-network-long conversion (htonl) */
        /* host-to-network-short conversion (htons) */
        servAddr.sin_family      = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servAddr.sin_port        = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (res < 0) {
            printf("Setsockopt SO_REUSEADDR failed.\n");
            break;
        }

        /*Bind Socket*/
        if (bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
            printf("Bind failed.\n");
            break;
        }

        printf("Awaiting client connection on port %d\n", SERV_PORT);

        cliLen = sizeof(cliaddr);
        connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
                (struct sockaddr*)&cliaddr, &cliLen);

        if (connfd < 0) {
            printf("No clients in que, enter idle state\n");
            close(listenfd);
            continue;
        }
        else if (connfd > 0) {
            if (connect(listenfd, (const struct sockaddr *)&cliaddr,
                        sizeof(cliaddr)) != 0) {
                printf("Udp connect failed.\n");
                break;
            }
        }
        else {
            printf("Recvfrom failed.\n");
            break;
        }
        printf("Connected!\n");

        /* Create the WOLFSSL Object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            printf("wolfSSL_new error.\n");
            break;
        }

#ifdef WOLFSSL_DTLS_SET_PEER
        /* Alternative to UDP connect */
        wolfSSL_dtls_set_peer(ssl, &cliaddr, cliLen);
#endif

        flags = fcntl(*(&listenfd), F_SETFL, flags | O_NONBLOCK);
        /* set the session ssl to client connection port */
        wolfSSL_set_fd(ssl, listenfd);
        wolfSSL_dtls_set_using_nonblock(ssl, 1);

        /* Perform handshake. */
        while (wolfSSL_accept(ssl) != SSL_SUCCESS) {
            int e = wolfSSL_get_error(ssl, 0);
            if (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE) {
                printf("error = %d, %s\n", e,
                    wolfSSL_ERR_reason_error_string(e));
                printf("SSL_accept failed.\n");
            
                break;
            }
        }

#ifdef WOLFSSL_THREADED_CRYPT
        /* Setup encryption threads. */
        for (i = 0; i < WOLFSSL_THREADED_CRYPT_CNT; i++) {
            pthread_mutex_lock(&encArg[i].mutex_ssl);
            encArg[i].ssl = ssl;
            pthread_mutex_unlock(&encArg[i].mutex_ssl);
            wolfSSL_AsyncEncryptSetSignal(ssl, i, thread_enc_signal,
                &encArg[i]);
        }
#endif

        /* Set socke to non-blocking. */
        fcntl(wolfSSL_get_fd(ssl), F_SETFL, O_NONBLOCK);

        args.ssl = ssl;
        args.cleanup = 0;

        /* Create reader and writer threads. */
        pthread_create(&threadidReader, NULL, Reader, &args);
        pthread_create(&threadidWriter, NULL, Writer, &args);

        /* Wait for read/write threads to be done. */
        pthread_join(threadidReader, NULL);
        pthread_join(threadidWriter, NULL);

        /* Shutdown SSL connection. */
        wolfSSL_set_fd(ssl, 0);
        wolfSSL_shutdown(ssl);
#ifdef WOLFSSL_THREADED_CRYPT
        /* Setup encryption threads. */
        for (i = 0; i < WOLFSSL_THREADED_CRYPT_CNT; i++) {
            pthread_mutex_lock(&encArg[i].mutex_ssl);
            encArg[i].ssl = NULL;
            pthread_mutex_unlock(&encArg[i].mutex_ssl);
        }
#endif
        wolfSSL_free(ssl);
        ssl = NULL;
        close(listenfd);

        printf("Client left cont to idle state\n");
    }
    
    /* Dispose of SSL context object. */
    wolfSSL_CTX_free(ctx);
    /* Cleanup wolfSSL. */
    wolfSSL_Cleanup();

    return 0;
}

