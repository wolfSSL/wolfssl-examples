/* server-dtls-threaded.c 
 *
 * Copyright (C) 2006-2014 wolfSSL Inc.
 *
 * This file is part of CyaSSL.
 *
 * CyaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CyaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  
 * USA
 *=============================================================================
 *
 * Bare-bones example of a DTLS server for instructional/learning purposes.
 * Utilizes DTLS 1.2. and multi-threading
 */


#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <cyassl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#define SERV_PORT   11111           /* define our server port number */
#define MSGLEN      4096

CYASSL_CTX*        ctx;             /* must be global for ThreadControl */
static int         cleanup;         /* To handle shutdown */
struct sockaddr_in cliAddr;         /* the client's address */
struct sockaddr_in servAddr;        /* our server's address */

int AwaitDGram(CYASSL_CTX* ctx);
void* ThreadControl(void*);

int AwaitDGram(CYASSL_CTX* ctx)
{

    while (cleanup != 1) {
        int            on = 1;
        int            res = 1; 
        int            connfd = 0;     
        int            listenfd = 0;    /* Initialize our socket */
        socklen_t      cliLen = 0;      /* length of address' */
        socklen_t      len = sizeof(on);
        unsigned char  b[MSGLEN];
        void*          dummy = NULL;    

        if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            printf("Cannot create socket.\n");
            cleanup = 1;
            return 1;
        }

        printf("Socket allocated\n");

        /* INADDR_ANY=IPaddr, socket =  11111, modify SERV_PORT to change */
        memset((char *)&servAddr, 0, sizeof(servAddr));
        memset((char *)&cliAddr, 0, sizeof(cliAddr));
        memset(&b, 0, MSGLEN);

        /* host-to-network-long conversion (htonl) */
        /* host-to-network-short conversion (htons) */
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servAddr.sin_port = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (res < 0) {
            printf("Setsockopt SO_REUSEADDR failed.\n");
            cleanup = 1;
            return 1;
        }

        /*Bind Socket*/
        if (bind(listenfd, (struct sockaddr *)&servAddr, 
                    sizeof(servAddr)) < 0) {
            printf("Bind failed.\n");
            cleanup = 1;
            return 1;
        }
        printf("Socket bind complete\n");
        printf("Awaiting client connection on port %d\n", SERV_PORT);

        /* set cliLen to |cliAddr| */
        cliLen = sizeof(cliAddr);   
        connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
                (struct sockaddr*)&cliAddr, &cliLen);

        if (connfd < 0) {
            printf("No clients in que, enter idle state\n");
            continue;
        }
        else if (connfd > 0) {
            /* create a new thread ID */
            pthread_t threadID;
            /* use ID to spawn a unique thread */
            if (pthread_create(&threadID, NULL, 
                        ThreadControl, (void *)&connfd) < 0) {
                printf("pthread_create failed.\n");
            }
            printf("Connection being re-routed to Thread Control.\n");
        }
        else {
            printf("Recvfrom failed.\n");
            cleanup = 1;
        }
        pthread_join(pthread_self(), dummy);
    }
    return 0;
}

void* ThreadControl(void* openSock)
{
    pthread_detach(pthread_self());

    int           on = 1;
    int           res = 1;
    int           connfd = 0;
    int           recvlen = 0;
    int           listenfd = 0;
    socklen_t     cliLen;
    socklen_t     len =  sizeof(on);
    CYASSL*       ssl = NULL;
    unsigned char b[MSGLEN];
    char          buff[MSGLEN];
    char          ack[] = "I hear you fashizzle!\n";
    void*         dummy;

    if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        printf("Cannot create socket.\n");
        cleanup = 1;
    }

    /* INADDR_ANY=IPaddr, socket =  11111, modify SERV_PORT to change */
    memset((char *)&servAddr, 0, sizeof(servAddr));

    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(SERV_PORT);

    /* Eliminate socket already in use error */
    res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
    if (res < 0) {
        printf("Setsockopt SO_REUSEADDR failed.\n");
        cleanup = 1;
    }

    /*Bind Socket*/
    if (bind(listenfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        printf("Bind failed.\n");
        cleanup = 1;
    }

    cliLen = sizeof(cliAddr);
    connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
            (struct sockaddr*)&cliAddr, &cliLen);

    if (connfd < 0) {
        printf("No clients in que, enter idle state\n");
    }
    else if (connfd > 0) {
        if (connect(listenfd, (const struct sockaddr *)&cliAddr, 
                    sizeof(cliAddr)) != 0) {
            printf("Udp connect failed.\n");
            cleanup = 1;
        }
    }
    else {
        printf("Recvfrom failed.\n");
        cleanup = 1;
    }

    printf("Connected!\n");

    /* Create the CYASSL Object */
    if ((ssl = CyaSSL_new(ctx)) == NULL) {
        printf("CyaSSL_new error.\n");
        cleanup = 1;
    }

    /* set the session ssl to client connection port */
    CyaSSL_set_fd(ssl, listenfd);

    if (CyaSSL_accept(ssl) != SSL_SUCCESS) {
        int e = CyaSSL_get_error(ssl, 0);
        printf("error = %d, %s\n", e, CyaSSL_ERR_reason_error_string(e));
        printf("SSL_accept failed.\n");
        cleanup = 1;
    }
    if ((recvlen = CyaSSL_read(ssl, buff, sizeof(buff)-1)) > 0){

        printf("heard %d bytes\n", recvlen);

        buff[recvlen] = 0;
        printf("I heard this: \"%s\"\n", buff);
    }
    if (recvlen < 0) {
        int readErr = CyaSSL_get_error(ssl, 0);
        if (readErr != SSL_ERROR_WANT_READ) {
            printf("SSL_read failed.\n"); 
            cleanup = 1;
        }
    }
    if (CyaSSL_write(ssl, ack, sizeof(ack)) < 0) {
        printf("CyaSSL_write fail.\n"); 
        cleanup = 1;
    }
    else {
        printf("Sending Reply...\n");
    }

    printf("reply sent \"%s\"\n", ack);
    printf("No further packets, severing connection to client.\n");

    CyaSSL_set_fd(ssl, 0); 
    CyaSSL_shutdown(ssl);        
    CyaSSL_free(ssl);

    printf("Returning to idle state\n");
    pthread_exit(&dummy);
}

int main(int argc, char** argv)
{
    /* cont short for "continue?", Loc short for "location" */    
    int         cont = 0;
    char        caCertLoc[] = "../certs/ca-cert.pem";
    char        servCertLoc[] = "../certs/server-cert.pem";
    char        servKeyLoc[] = "../certs/server-key.pem";

    /* "./config --enable-debug" and uncomment next line for debugging */
    /* CyaSSL_Debugging_ON(); */

    /* Initialize CyaSSL */
    CyaSSL_Init();

    /* Set ctx to DTLS 1.2 */
    if ((ctx = CyaSSL_CTX_new(CyaDTLSv1_2_server_method())) == NULL) {
        printf("CyaSSL_CTX_new error.\n");
        return 1;
    }
    /* Load CA certificates */
    if (CyaSSL_CTX_load_verify_locations(ctx,caCertLoc,0) != 
            SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        return 1;
    }
    /* Load server certificates */
    if (CyaSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != 
            SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        return 1;
    }
    /* Load server Keys */
    if (CyaSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return 1;
    }

    cont = AwaitDGram(ctx);

    if (cont == 1) {
        CyaSSL_CTX_free(ctx);
        CyaSSL_Cleanup();
    }

    return 0;
}
