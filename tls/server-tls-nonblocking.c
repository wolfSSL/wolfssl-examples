/* server-tls-nonblocking.c
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  * USA
 * =============================================================================
 *
 * This is a super basic example of what a TCP Server secured with TLS 1.2
 * that uses non blocking input and output.
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <errno.h>

/* Include the CyaSSL library for our TLS 1.2 security */
#include <cyassl/ssl.h>

#define DEFAULT_PORT 11111

/* Create an enum that we will use to tell our 
 * NonBlocking_ReadWriteAccept() method what to do
 */
enum read_write_t {WRITE, READ, ACCEPT};

int AcceptAndRead(CYASSL_CTX* ctx, int socketfd, 
    struct sockaddr_in clientAddr);
int TCPSelect(int socketfd);
int NonBlocking_ReadWriteAccept(CYASSL* ssl, int socketfd, 
    enum read_write_t rw);

/*  Check if any sockets are ready for reading and writing and set it */
int TCPSelect(int socketfd)
{
    fd_set recvfds, errfds;
    int nfds = socketfd + 1;
    int result;

    FD_ZERO(&recvfds);
    FD_SET(socketfd, &recvfds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    result = select(nfds, &recvfds, NULL, &errfds, NULL);

    if (result > 0) {
        if (FD_ISSET(socketfd, &recvfds))
            return 1; /* RECV READY */
        else if (FD_ISSET(socketfd, &errfds))
            return 2; /* ERROR READY */
    }

    return -1; /* TEST FAILED */

}
/* Checks if NonBlocking I/O is wanted, if it is wanted it will
 * wait until it's available on the socket before reading or writing */
int NonBlocking_ReadWriteAccept(CYASSL* ssl, int socketfd, 
    enum read_write_t rw)
{
    const char reply[] = "I hear ya fa shizzle!\n";
    char       buff[256];
    int        rwret = 0;
    int        selectRet;
    int 	   ret;


    /* Clear the buffer memory for anything  possibly left 
       over */
    memset(&buff, 0, sizeof(buff));

    if (rw == READ)
        rwret = CyaSSL_read(ssl, buff, sizeof(buff)-1);
    else if (rw == WRITE)
        rwret = CyaSSL_write(ssl, reply, sizeof(reply)-1);
    else if (rw == ACCEPT)
        rwret = CyaSSL_accept(ssl);

    if (rwret == 0) {
        printf("The client has closed the connection!\n");
        return 0;
    }
    else if (rwret != SSL_SUCCESS) {
        int error = CyaSSL_get_error(ssl, 0);

        /* while I/O is not ready, keep waiting */
        while ((error == SSL_ERROR_WANT_READ || 
            error == SSL_ERROR_WANT_WRITE)) {

            if (error == SSL_ERROR_WANT_READ)
                printf("... server would read block\n");
            else
                printf("... server would write block\n");

            selectRet = TCPSelect(socketfd);

            if ((selectRet == 1) || (selectRet == 2)) {
                if (rw == READ)
                    rwret = CyaSSL_read(ssl, buff, sizeof(buff)-1);
                else if (rw == WRITE)
                    rwret = CyaSSL_write(ssl, reply, sizeof(reply)-1);
                else if (rw == ACCEPT)
                    rwret = CyaSSL_accept(ssl);
                
                error = CyaSSL_get_error(ssl, 0);
            }
            else {
                error = SSL_FATAL_ERROR;
                return -1;
            }
        }
        /* Print any data the client sends to the console */
        if (rw == READ)
            printf("Client: %s\n", buff);
        /* Reply back to the client */
        else if (rw == WRITE) {
            if ((ret = CyaSSL_write(ssl, reply, sizeof(reply)-1)) < 0) {
                printf("CyaSSL_write error = %d\n", 
                    CyaSSL_get_error(ssl, ret));
            }
        }
    }

    return 1;
}

int AcceptAndRead(CYASSL_CTX* ctx, int socketfd, struct sockaddr_in clientAddr)
{
    int     size = sizeof(clientAddr);
    int     ret = 0;
    int     err = 0;
    CYASSL* ssl;

    /* Wait until a client connects */
    int connd = accept(socketfd, (struct sockaddr *)&clientAddr, &size);

    /* If fails to connect, loop back up and wait for a new connection */
    if (connd == -1) {
        printf("failed to accept the connection..\n");
    }
    /* If it connects, read in and reply to the client */
    else {
        printf("Client connected successfully!\n");

        if ( (ssl = CyaSSL_new(ctx)) == NULL) {
            fprintf(stderr, "CyaSSL_new error.\n");
            exit(EXIT_FAILURE);
        }

        /* Direct our ssl to our clients connection */
        CyaSSL_set_fd(ssl, connd);
        
        /* Sets CyaSSL_accept(ssl) */
        if(NonBlocking_ReadWriteAccept(ssl, socketfd, ACCEPT) < 0)
            return 0;

        /* 
         * loop until the connected client disconnects
         * and read in any messages the client sends
         */
        for ( ; ; ) {   
            /* Read data in when I/O is available */
            if (NonBlocking_ReadWriteAccept(ssl, socketfd, READ) == 0)
                break;
            /* Write data out when I/O is available */
            if (NonBlocking_ReadWriteAccept(ssl, socketfd, WRITE) == 0)
                break;
        }
    } 
    CyaSSL_free(ssl);           /* Free the CYASSL object */
    close(connd);               /* close the connected socket */

    return 0;
}


int main()
{
    /* 
     * Creates a socket that uses an internet IP address,
     * Sets the type to be Stream based (TCP),
     * 0 means choose the default protocol.
     */
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    int loopExit = 0; /* 0 = False, 1 = True */
    int ret      = 0; /* Return variable */
    int on       = 1;

    /* Set nonblocking */
    //fcntl(socketfd, F_SETFL, fcntl(socketfd, F_GETFL, 0) | O_NONBLOCK);

    /* Create a ctx pointer for our ssl */
    CYASSL_CTX* ctx;

    /* Server and Client socket address structures */
    struct sockaddr_in serverAddr, clientAddr;

    /* Initialize the server address struct to zero */
    memset((char *)&serverAddr, 0, sizeof(serverAddr)); 

    /* Fill the server's address family */
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port        = htons(DEFAULT_PORT);
       
    socklen_t len = sizeof(on);

    /* If positive value, the socket is valid */
    if (socketfd < 0) {
        printf("ERROR: failed to create the socket\n");
        exit(EXIT_FAILURE);        /* Kill the server with exit status 1 */
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &on, len) 
        < 0)                    
        printf("setsockopt SO_REUSEADDR failed\n");

    /* Initialize CyaSSL */
    CyaSSL_Init();

    /* Create and initialize CYASSL_CTX structure */
    if ((ctx = CyaSSL_CTX_new(CyaTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "CyaSSL_CTX_new error.\n");
        exit(EXIT_FAILURE);
    }

    /* Load server certificate into CYASSL_CTX */
    if (CyaSSL_CTX_use_certificate_file(ctx, "certs/server-cert.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-cert.pem, please check"
                "the file.\n");
        exit(EXIT_FAILURE);
    }

    /* Load server key into CYASSL_CTX */
    if (CyaSSL_CTX_use_PrivateKey_file(ctx, "certs/server-key.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-key.pem, please check"
                "the file.\n");
        exit(EXIT_FAILURE);
    }

    /* Attach the server socket to our port */
    if (bind(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr))
        < 0) {
        printf("ERROR: failed to bind\n");
        exit(1);
    }

    printf("Waiting for a connection...\n");

    /* Continuously accept connects while not currently in an active connection
       or told to quit */
    while (loopExit == 0) {
        /* listen for a new connection, allow 5 pending connections */
        ret = listen(socketfd, 5);
        if (ret == 0) {
            /* Accept client connections and read from them */
            loopExit = AcceptAndRead(ctx, socketfd, clientAddr);
        }
    }

    CyaSSL_CTX_free(ctx);   /* Free CYASSL_CTX */
    CyaSSL_Cleanup();       /* Free CyaSSL */
    exit(EXIT_SUCCESS);
}
