/* 
 * client-dtls-nonblocking.c 
 *
 * Copyright (C) 2006-2015 wolfSSL Inc.
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
 * Bare-bones example of a DTLS client for instructional/learning purposes.
 */

#include <unistd.h>
#include <wolfssl/ssl.h>
#include <errno.h>
#include <fcntl.h>
#include <wolfssl/options.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE   4096
#define SERV_PORT 11111 

enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};

/* Tcp select using dtls nonblocking function */
static int dtls_select(int socketfd, int to_sec)
{

    fd_set recvfds, errfds;
    int    nfds = socketfd +1;
    struct timeval timeout = { (to_sec > 0) ? to_sec : 0, 0};
    int    result;

    FD_ZERO(&recvfds);
    FD_SET(socketfd, &recvfds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    result = select(nfds, &recvfds, NULL, &errfds, &timeout);

    if (result == 0)
	    return TEST_TIMEOUT;
    else if (result > 0) {
	    if (FD_ISSET(socketfd, &recvfds))
	        return TEST_RECV_READY;
	    else if (FD_ISSET(socketfd, &errfds))
	        return TEST_ERROR_READY;
    }
    return TEST_SELECT_FAIL;
}

/* Connect using Nonblocking - DTLS version */
static void NonBlockingDTLS_Connect(WOLFSSL* ssl)
{
    int ret = wolfSSL_connect(ssl);
    int error = wolfSSL_get_error(ssl, 0);
    int sockfd = (int)wolfSSL_get_fd(ssl); 
    int select_ret;
   
    while (ret != SSL_SUCCESS && (error == SSL_ERROR_WANT_READ || 
		error == SSL_ERROR_WANT_WRITE)) {
	    int currTimeout = 1;
	    if (error == SSL_ERROR_WANT_READ)
	        printf("... client would read block\n");
	    else
	        printf("... client would write block\n");
	    currTimeout = wolfSSL_dtls_get_current_timeout(ssl);
	    select_ret = dtls_select(sockfd, currTimeout);
	    if ( ( select_ret == TEST_RECV_READY) || 
		    (select_ret == TEST_ERROR_READY)) {	
	        ret = wolfSSL_connect(ssl);
	        error = wolfSSL_get_error(ssl, 0);
	    }
	    else if (select_ret == TEST_TIMEOUT && !wolfSSL_dtls(ssl)) {
	        error = 2;
	    }
	    else if (select_ret == TEST_TIMEOUT && wolfSSL_dtls(ssl) && 
		    wolfSSL_dtls_got_timeout(ssl) >= 0) {
	        error = 2;
	    }
	    else{
	        error = SSL_FATAL_ERROR;
	    }
    }

    if (ret != SSL_SUCCESS) {
	    printf("SSL_connect failed with");
    }
}

/* Main send and receive function */
void DatagramClient (WOLFSSL* ssl) 
{
    int  n = 0;
    char sendLine[MAXLINE], recvLine[MAXLINE - 1];

    while (fgets(sendLine, MAXLINE, stdin) != NULL) {

        while  ( ( wolfSSL_write(ssl, sendLine, strlen(sendLine))) != 
	        strlen(sendLine)) {
    	    printf("SSL_write failed");
        }

        while ( (n = wolfSSL_read(ssl, recvLine, sizeof(recvLine)-1)) <= 0) {

	        int readErr = wolfSSL_get_error(ssl, 0);
	        if(readErr != SSL_ERROR_WANT_READ)
	            printf("wolfSSL_read failed");
        }

        recvLine[n] = '\0';  
        fputs(recvLine, stdout);
    }
}


int main (int argc, char** argv) 
{
    int     		sockfd = 0;
    struct  		sockaddr_in servAddr;
    const char* 	host = argv[1];
    WOLFSSL* 		ssl = 0;
    WOLFSSL_CTX* 	ctx = 0;
    WOLFSSL* 		sslResume = 0;
    WOLFSSL_SESSION*	session = 0;
    char            cert_array[] = "../certs/ca-cert.pem";
    char*           certs = cert_array;
    char*           srTest = "testing session resume";
    
    if (argc != 2) {
	    printf("usage: udpcli <IP address>\n");
        return 1;
    }

    wolfSSL_Init();

    /* wolfSSL_Debugging_ON();*/

    if ( (ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method())) == NULL) {
	    fprintf(stderr, "wolfSSL_CTX_new error.\n");
	    return(EXIT_FAILURE);
    }

    if (wolfSSL_CTX_load_verify_locations(ctx,certs, 0) != SSL_SUCCESS) {
	    fprintf(stderr, "Error loading %s, please check the file.\n", certs);
	    return(EXIT_FAILURE);
    }

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
	    printf("unable to get ssl object");
        return 1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, host, &servAddr.sin_addr) < 1) {
        printf("Error and/or invalid IP address");
        return 1;
    }

    wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
    	printf("cannot create a socket."); 
        return 1;
    }

    wolfSSL_set_fd(ssl, sockfd);
    wolfSSL_set_using_nonblock(ssl, 1);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    NonBlockingDTLS_Connect(ssl);

    DatagramClient(ssl);
    while ( (wolfSSL_write(ssl, srTest, sizeof(srTest))) != sizeof(srTest)) { 
	    printf("failed to write");
        return 1;
    }

    session = wolfSSL_get_session(ssl);
    sslResume = wolfSSL_new(ctx);

    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(sockfd);

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, host, &servAddr.sin_addr) < 1) {
        printf("Error and/or invalid IP address");
        return 1;
    }

    wolfSSL_dtls_set_peer(sslResume, &servAddr, sizeof(servAddr));

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
	   printf("cannot create a socket.");
    return 1; 
    }

    wolfSSL_set_fd(sslResume, sockfd);
    wolfSSL_set_session(sslResume, session);
    wolfSSL_set_using_nonblock(sslResume, 1);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    NonBlockingDTLS_Connect(sslResume);

    if(wolfSSL_session_reused(sslResume))
	    printf("reused session id\n");
    else
	    printf("didn't reuse session id!!!\n");

    DatagramClient(sslResume);
    while ((wolfSSL_write(sslResume, srTest, sizeof(srTest))) != sizeof(srTest))
    { 
	    printf("failed to write");
        return 1;
    }
    sleep(1);

    wolfSSL_shutdown(sslResume);
    wolfSSL_free(sslResume);
   
    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return 0;
}

