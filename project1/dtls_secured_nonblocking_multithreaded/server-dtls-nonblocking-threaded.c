/* server-dtls-nonblocking-threaded.c 
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
 * Utilizes DTLS 1.2 and non-blocking sockets and multi threading to handle
 * multiple clients.
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
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>

#define SERV_PORT   11111           /* define our server port number */
#define MSGLEN      4096

CYASSL_CTX* ctx;                /* ctx pointer */
static int cleanup;             /* To handle shutdown */
struct sockaddr_in servaddr;    /* our server's address */
struct sockaddr_in cliaddr;     /* the client's address */
int listenfd;                   /* Initialize our socket */

static int dtls_select(socklen_t sockfd, int to_sec);
int Accept();
void *ThreadControl(void*);

void sig_handler(const int sig) 
{
    printf("\nSIGINT handled.\n");
    cleanup = 1;
    exit(0);
}

static void err_sys(const char* msg)
{
    printf("yassl error: %s\n", msg);
    if (msg)
        exit(0);
}

enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};

static int dtls_select(socklen_t sockfd, int to_sec)
{
    fd_set masterset, workingset;
    socklen_t maxfd = sockfd + 1;
    struct timeval timeout = { (to_sec > 0) ? to_sec : 0, 0};
    int result;

    FD_ZERO(&masterset);
    FD_SET(sockfd, &masterset);
    FD_ZERO(&workingset);
    FD_SET(sockfd, &workingset);

    result = select(maxfd, &masterset, NULL, &workingset, &timeout);

    if (result == 0)
        return TEST_TIMEOUT;
    else if (result > 0) {
        if (FD_ISSET(sockfd, &masterset))
            return TEST_RECV_READY;
        else if(FD_ISSET(sockfd, &workingset))
            return TEST_ERROR_READY;
    }

    return TEST_SELECT_FAIL;
}

int Accept()
{

    while (cleanup != 1) {
        socklen_t clilen;               /* length of address' */

        if ( (listenfd = socket(AF_INET, SOCK_DGRAM, 0) ) < 0 ) {
            err_sys("cannot create socket");
            return 0;
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
            err_sys("setsockopt SO_REUSEADDR failed\n");
        }

        /*Bind Socket*/
        if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            err_sys("bind failed");
            return 0;
        }
        printf("Socket bind complete\n");
        printf("Awaiting client connection on port %d\n", SERV_PORT);

        /* set listenfd non-blocking */
        fcntl(res, F_SETFL, O_NONBLOCK);

        clilen =    sizeof(cliaddr);    /* set clilen to |cliaddr| */
        unsigned char       b[1500];    
        int              connfd = 0;     

        connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
                (struct sockaddr*)&cliaddr, &clilen);

        /* No clients attempting to connect */
        if (connfd < 0){
            printf("No clients in que, enter idle state\n");
        }

        /* Client attempted to connect but was not using udp */
        else if (connfd > 0) {
            pthread_t threadID;

            if (pthread_create(&threadID, NULL, ThreadControl, (void *)&connfd) < 0) {
                printf("pthread_create failed.\n");
            }

            printf("Connection being re-routed to Thread Control.\n");
        }
    }
    return 1;
}

void *ThreadControl(void* openSock)
{
    char ack[] = "I hear you fashizzle!";
    struct sockaddr_in servaddr;    /* our server's address */
    struct sockaddr_in cliaddr;     /* the client's address */
    int listenfd;                   /* Initialize our socket */
    socklen_t clilen;               /* length of address' */
    int recvlen = 0;                /* length of message */
    char buff[MSGLEN];              /* the incoming message */

    listenfd = *(int*)openSock;
    if ( (listenfd = socket(AF_INET, SOCK_DGRAM, 0) ) < 0 ) {
        err_sys("cannot create socket");
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
        err_sys("setsockopt SO_REUSEADDR failed\n");
    }

    /*Bind Socket*/
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        err_sys("bind failed");
    }

    /* set listenfd non-blocking */
    fcntl(res, F_SETFL, O_NONBLOCK);

    CYASSL*                 ssl;    /* initialize arg */
    clilen =    sizeof(cliaddr);    /* set clilen to |cliaddr| */
    unsigned char       b[1500];    
    int              connfd = 0;     

    connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
            (struct sockaddr*)&cliaddr, &clilen);

    /* Client attempted to connect but was not using udp */
    if (connfd > 0) {
        if (connect(listenfd, (const struct sockaddr *)&cliaddr, 
                    sizeof(cliaddr)) != 0)
            err_sys("udp connect failed");
    }

    /* client attempted to connect, recvfrom failed */
    else err_sys("recvfrom failed");

    /* Create the CYASSL Object */
    if (( ssl = CyaSSL_new(ctx) ) == NULL) {
        fprintf(stderr, "CyaSSL_new error.\n");
        exit(EXIT_FAILURE);
    }

    /* set the session ssl to client connection port */
    CyaSSL_set_fd(ssl, listenfd);

    /* set listen port to nonblocking, accept nonblocking */
    CyaSSL_set_using_nonblock(ssl, 1);

    int ret;
    int error = CyaSSL_get_error(ssl, 0);
    socklen_t sockfd = (socklen_t)CyaSSL_get_fd(ssl);
    int select_ret;

    ret = CyaSSL_accept(ssl);

    while (ret != SSL_SUCCESS && (error == SSL_ERROR_WANT_READ ||
                error == SSL_ERROR_WANT_WRITE)) {

        int currTimeout = 1;

        /* NOTE: changes over time */
        currTimeout = CyaSSL_dtls_get_current_timeout(ssl);

        if (error == SSL_ERROR_WANT_READ)
            printf("Server wants to read...\n");
        else
            printf("Server wants to write...\n");

        printf("waiting to select()\n");

        select_ret = dtls_select(sockfd, currTimeout);

        if (select_ret == TEST_RECV_READY) {
            printf("TEST_RECV_READY!\n");
            ret = CyaSSL_accept(ssl);
        }

        if (select_ret == TEST_ERROR_READY && !CyaSSL_dtls(ssl)) {
            printf("TEST_ERROR_READY\n");
            error = CyaSSL_get_error(ssl, 0);
        }

        else if (select_ret == TEST_TIMEOUT && CyaSSL_dtls(ssl) &&
                CyaSSL_dtls_got_timeout(ssl) ) {
            printf("TEST_TIMEOUT\n");
            error = SSL_ERROR_WANT_READ;
        }
        else if (select_ret == TEST_TIMEOUT && CyaSSL_dtls(ssl) &&
                CyaSSL_dtls_got_timeout(ssl) >= 0) {
            error = SSL_ERROR_WANT_READ;
        }
        else {
            error = SSL_FATAL_ERROR;
        }
    }
    if (ret != SSL_SUCCESS){
        printf("SSL_accept failed\n");
    }

    printf("Connected with a client!\n");

    if (( recvlen = CyaSSL_read(ssl, buff, sizeof(buff)-1)) > 0){

        printf("heard %d bytes\n", recvlen);

        buff[recvlen] = 0;
        printf("I heard this: \"%s\"\n", buff);
    }

    if (recvlen < 0) {
        int readErr = CyaSSL_get_error(ssl, 0);
        if(readErr != SSL_ERROR_WANT_READ)
            err_sys("SSL_read failed");
    }

    if (CyaSSL_write(ssl, ack, sizeof(ack)) < 0) {
        err_sys("CyaSSL_write fail");
    }

    else 
        printf("lost the connection to client\n");

    printf("reply sent \"%s\"\n", ack);

    CyaSSL_set_fd(ssl, 0); 
    CyaSSL_shutdown(ssl);        
    CyaSSL_free(ssl);

    printf("Client left return to idle state\n");
}

int main(int argc, char** argv)
{
    /* CREATE THE SOCKET */

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

    int cont = Accept();

    if(cont == 0)
        CyaSSL_CTX_free(ctx);
    return(0);
}
