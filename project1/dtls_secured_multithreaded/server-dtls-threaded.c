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

struct sockaddr_in servaddr;        /* our server's address */
struct sockaddr_in cliaddr;         /* the client's address */
static int cleanup;                 /* To handle shutdown */
CYASSL_CTX* ctx;

void AwaitDGram();
void* ThreadControl(void*);

void sig_handler(const int sig) 
{
    printf("\nSIGINT handled.\n");
    CyaSSL_CTX_free(ctx);
    CyaSSL_Cleanup();
    cleanup = 1;
    exit(0);
}

void AwaitDGram()
{
    int listenfd = 0;               /* Initialize our socket */
    socklen_t clilen;               /* length of address' */

    while (cleanup != 1) {
        if ( (listenfd = socket(AF_INET, SOCK_DGRAM, 0) ) < 0 ) {
            printf("Cannot create socket.\n");
            cleanup = 1;
            break;
        }
        printf("Socket allocated\n");

        /* INADDR_ANY=IPaddr, socket =  11111, modify SERV_PORT to change */
        memset((char *)&servaddr, 0, sizeof(servaddr));

        /* host-to-network-long conversion (htonl) */
        /* host-to-network-short conversion (htons) */
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        int res = 1; 
        int on = 1;
        socklen_t len = sizeof(on);
        res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (res < 0) {
            printf("Setsockopt SO_REUSEADDR failed.\n");
            cleanup = 1;
            break;
        }

        /*Bind Socket*/
        if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            printf("Bind failed.\n");
            cleanup = 1;
            break;
        }
        printf("Socket bind complete\n");


        printf("Awaiting client connection on port %d\n", SERV_PORT);

        clilen =    sizeof(cliaddr);    /* set clilen to |cliaddr| */
        unsigned char       b[1500];    
        int              connfd = 0;     

        connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
                (struct sockaddr*)&cliaddr, &clilen);

        if (connfd < 0){
            printf("No clients in que, enter idle state\n");
            continue;
        }

        else if (connfd > 0) {

            pthread_t threadID;

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

//        sleep(1);
        continue;
    }
}

void* ThreadControl(void* openSock)
{

    pthread_detach(pthread_self());
    char ack[] = "I hear you fashizzle!\n";
    int listenfd = 0;
    socklen_t clilen;               /* length of address' */
    int recvlen = 0;                /* length of message */
    char buff[MSGLEN];              /* the incoming message */

    if ( (listenfd = socket(AF_INET, SOCK_DGRAM, 0) ) < 0 ) {
        printf("Cannot create socket.\n");
        cleanup = 1;
    }

    /* INADDR_ANY=IPaddr, socket =  11111, modify SERV_PORT to change */
    memset((char *)&servaddr, 0, sizeof(servaddr));

    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    /* Eliminate socket already in use error */
    int res = 1; 
    int on = 1;
    socklen_t len = sizeof(on);
    res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
    if (res < 0) {
        printf("Setsockopt SO_REUSEADDR failed.\n");
        cleanup = 1;
    }

    /*Bind Socket*/
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("Bind failed.\n");
        cleanup = 1;
    }

    CYASSL*                 ssl;    /* initialize arg */
    clilen =    sizeof(cliaddr);    /* set clilen to |cliaddr| */
    unsigned char       b[1500];    
    int              connfd = 0;     

    connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
            (struct sockaddr*)&cliaddr, &clilen);

    if (connfd < 0){
        printf("No clients in que, enter idle state\n");
    }

    else if (connfd > 0) {
        if (connect(listenfd, (const struct sockaddr *)&cliaddr, 
                    sizeof(cliaddr)) != 0) {
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
    if (( ssl = CyaSSL_new(ctx) ) == NULL) {
        printf("CyaSSL_new error.\n");
        cleanup = 1;
    }

    /* set the session ssl to client connection port */
    CyaSSL_set_fd(ssl, listenfd);

    if (CyaSSL_accept(ssl) != SSL_SUCCESS) {
        int err = CyaSSL_get_error(ssl, 0);
        char buffer[80];
        printf("error = %d, %s\n", err, CyaSSL_ERR_error_string(err, buffer));
        buffer[sizeof(buffer)-1] = 0;
        printf("SSL_accept failed.\n");
        cleanup = 1;
    }

    if (( recvlen = CyaSSL_read(ssl, buff, sizeof(buff)-1)) > 0){

        printf("heard %d bytes\n", recvlen);

        buff[recvlen] = 0;
        printf("I heard this: \"%s\"\n", buff);
    }

    if (recvlen < 0) {
        int readErr = CyaSSL_get_error(ssl, 0);
        if(readErr != SSL_ERROR_WANT_READ) {
            printf("SSL_read failed.\n"); 
            cleanup = 1;
        }
    }

    if (CyaSSL_write(ssl, ack, sizeof(ack)) < 0) {
        printf("CyaSSL_write fail.\n"); 
        cleanup = 1;
    }

    else 
        printf("Sending Reply...\n");

    printf("reply sent \"%s\"\n", ack);
    printf("No further packets, severing connection to client.\n");

    CyaSSL_set_fd(ssl, 0); 
    CyaSSL_shutdown(ssl);        
    CyaSSL_free(ssl);

    printf("Returning to idle state\n");
    int dummy;
    pthread_exit((void*)&dummy);
}

int main(int argc, char** argv)
{
    struct sigaction    act, oact;  /* structures for signal handling */

    /* 
     * Define a signal handler for when the user closes the program
     * with Ctrl-C. Also, turn off SA_RESTART so that the OS doesn't 
     * restart the call to accept() after the signal is handled. 
     */
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);

    //    CyaSSL_Debugging_ON();
    CyaSSL_Init();                      /* Initialize CyaSSL */

    if ( (ctx = CyaSSL_CTX_new(CyaDTLSv1_2_server_method())) == NULL){
        fprintf(stderr, "CyaSSL_CTX_new error.\n");
        exit(EXIT_FAILURE);
    }
    printf("CTX set to DTLS 1.2\n");

    if (CyaSSL_CTX_load_verify_locations(ctx,"../certs/ca-cert.pem",0) != 
            SSL_SUCCESS) {
        fprintf(stderr, "Error loading ../certs/ca-cert.pem, "
                "please check the file.\n");
        exit(EXIT_FAILURE);
    }
    printf("Loaded CA certs\n");

    if (CyaSSL_CTX_use_certificate_file(ctx,"../certs/server-cert.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading ../certs/server-cert.pem, "
                "please check the file.\n");
        exit(EXIT_FAILURE);
    }
    printf("Loaded server certs\n");

    if (CyaSSL_CTX_use_PrivateKey_file(ctx,"../certs/server-key.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading ../certs/server-key.pem, "
                "please check the file.\n");
        exit(EXIT_FAILURE);
    }
    printf("Loaded server keys\n");

    AwaitDGram();
    if (cleanup == 1) {
        CyaSSL_Cleanup();
        CyaSSL_CTX_free(ctx);
    }

    return(0);
}
