/* server-dtls-threaded.c
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
 * Bare-bones example of a threaded DTLS server for instructional/learning
 * purposes. Utilizes DTLS 1.2 and multi-threading
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
#include <pthread.h>

#define SERV_PORT   11111           /* define our server port number */
#define MSGLEN      4096

static WOLFSSL_CTX* ctx;                    /* global for ThreadControl*/
static int          cleanup;                /* To handle shutdown */
static struct       sockaddr_in cliAddr;    /* the client's address */
static struct       sockaddr_in servAddr;   /* our server's address */

void sig_handler(const int sig);
void* ThreadControl(void*);

typedef struct {
    int activefd;
    int size;
    unsigned char b[MSGLEN];
} threadArgs;

void sig_handler(const int sig)
{
    printf("\nSIGINT %d handled\n", sig);
    cleanup = 1;
    return;
}

void* ThreadControl(void* openSock)
{
    pthread_detach(pthread_self());

    threadArgs* args = (threadArgs*)openSock;
    int                recvLen = 0;                /* length of message     */
    int                activefd = args->activefd;  /* the active descriptor */
    int                msgLen = args->size;        /* the size of message   */
    unsigned char      buff[msgLen];               /* the incoming message  */
    char               ack[] = "I hear you fashizzle!\n";
    WOLFSSL*           ssl;
    int                e;                          /* error */

    memcpy(buff, args->b, msgLen);

    /* Create the WOLFSSL Object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        cleanup = 1;
        return NULL;
    }

    /* set the session ssl to client connection port */
    wolfSSL_set_fd(ssl, activefd);

    if (wolfSSL_accept(ssl) != SSL_SUCCESS) {

        e = wolfSSL_get_error(ssl, 0);

        printf("error = %d, %s\n", e, wolfSSL_ERR_reason_error_string(e));
        printf("SSL_accept failed.\n");
        return NULL;
    }
    if ((recvLen = wolfSSL_read(ssl, buff, msgLen-1)) > 0) {
        printf("heard %d bytes\n", recvLen);

        buff[recvLen] = 0;
        printf("I heard this: \"%s\"\n", buff);
    }
    else if (recvLen < 0) {
        int readErr = wolfSSL_get_error(ssl, 0);
        if(readErr != SSL_ERROR_WANT_READ) {
            printf("SSL_read failed.\n");
            cleanup = 1;
            return NULL;
        }
    }
    if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
        printf("wolfSSL_write fail.\n");
        cleanup = 1;
        return NULL;
    }
    else {
        printf("Sending reply.\n");
    }

    printf("reply sent \"%s\"\n", ack);

    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(activefd);
    free(openSock);                 /* valgrind friendly free */

    printf("Client left return to idle state\n");
    printf("Exiting thread.\n\n");
    pthread_exit(openSock);
}

int main(int argc, char** argv)
{
    /* cont short for "continue?", Loc short for "location" */
    int         cont = 0;
    char        caCertLoc[] = "../certs/ca-cert.pem";
    char        servCertLoc[] = "../certs/server-cert.pem";
    char        servKeyLoc[] = "../certs/server-key.pem";

    int           on = 1;
    int           res = 1;
    int           bytesRcvd = 0;
    int           listenfd = 0;   /* Initialize our socket */
    socklen_t     cliLen;
    socklen_t     len = sizeof(on);
    unsigned char buf[MSGLEN];      /* watch for incoming messages */
    /* variables needed for threading */
    threadArgs* args;
    pthread_t threadid;

    /* Code for handling signals */
    struct sigaction act, oact;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);

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

    /* Create a UDP/IP socket */
    if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        printf("Cannot create socket.\n");
        cleanup = 1;
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
        cleanup = 1;
        return 1;
    }

    /*Bind Socket*/
    if (bind(listenfd,
                (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        printf("Bind failed.\n");
        cleanup = 1;
        return 1;
    }

    printf("Awaiting client connection on port %d\n", SERV_PORT);

    while (cleanup != 1) {

        memset(&threadid, 0, sizeof(threadid));

        args = (threadArgs *) malloc(sizeof(threadArgs));

        cliLen = sizeof(cliAddr);
       /* note argument 4 of recvfrom not MSG_PEEK as dtls will see
        * handshake packets and think a message is arriving. Instead
        * read any real message to struct and pass struct into thread
        * for processing.
        */

        bytesRcvd = (int)recvfrom(listenfd, (char *)buf, sizeof(buf), 0,
                (struct sockaddr*)&cliAddr, &cliLen);

        if (cleanup == 1) {
            free(args);
            return 1;
        }

        if (bytesRcvd < 0) {
            printf("No clients in que, enter idle state\n");
            continue;
        }

        else if (bytesRcvd > 0) {

            /* put all the bytes from buf into args */
            memcpy(args->b, buf, sizeof(buf));

            args->size = bytesRcvd;

            if ((args->activefd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
                printf("Cannot create socket.\n");
                cleanup = 1;
            }

            res = setsockopt(args->activefd, SOL_SOCKET, SO_REUSEADDR, &on,
                    len);

            if (res < 0) {
                printf("Setsockopt SO_REUSEADDR failed.\n");
                cleanup = 1;
                return 1;
            }

            #ifdef SO_REUSEPORT
                res = setsockopt(args->activefd, SOL_SOCKET, SO_REUSEPORT, &on,
                        len);
                if (res < 0) {
                    printf("Setsockopt SO_REUSEPORT failed.\n");
                    cleanup = 1;
                    return 1;
                }
            #endif

            if (connect(args->activefd, (const struct sockaddr *)&cliAddr,
                        sizeof(cliAddr)) != 0) {
                printf("Udp connect failed.\n");
                cleanup = 1;
                return 1;
            }
        }
        else {
            /* else bytesRcvd = 0 */
            printf("Recvfrom failed.\n");
            cleanup = 1;
            return 1;
        }
        printf("Connected!\n");

        if (cleanup != 1) {
            /* SPIN A THREAD HERE TO HANDLE "buff" and "reply/ack" */
            pthread_create(&threadid, NULL, ThreadControl, args);
            printf("control passed to ThreadControl.\n");
        }
        else if (cleanup == 1) {
            return 1;
        } else {
            printf("I don't know what to tell ya man\n");
        }

        /* clear servAddr each loop */
        memset((char *)&servAddr, 0, sizeof(servAddr));
    }

    if (cont == 1) {
        wolfSSL_CTX_free(ctx);
        wolfSSL_Cleanup();
    }

    return 0;
}
