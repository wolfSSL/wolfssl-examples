/* server-psk-threaded.c
 * A server ecample using a multi-threaded TCP connection with PSK security. 
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <cyassl/ssl.h>     /* include CyaSSL security */
#include <cyassl/options.h> /* included for option sync */
#include <pthread.h>        /* used for concurrent threading */
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAXLINE     4096
#define LISTENQ     1024
#define SERV_PORT   11111

CYASSL_CTX* ctx; /* global so it's shared by threads */

/*
 * Identify which psk key to use.
 */
static inline unsigned int my_psk_server_cb(CYASSL* ssl, const char* identity,
                                     unsigned char* key,
                                     unsigned int key_max_len)
{
    (void)ssl;
    (void)key_max_len;

    if (strncmp(identity, "Client_identity", 15) != 0)
        return 0;

    key[0] = 26;
    key[1] = 43;
    key[2] = 60;
    key[3] = 77;

    return 4;
}

/*
 * Process handled by a thread.
 */
void* cyassl_thread(void* fd)
{
    CYASSL* ssl;
    int connfd = (int)fd;
    int  n;             
    char buf[MAXLINE];  
    char response[] = "I hear ya for shizzle";
   
    memset(buf, 0, MAXLINE);

    /* create CYASSL object */
    if ((ssl = CyaSSL_new(ctx)) == NULL) {
        printf("Fatal error : CyaSSL_new error");
        /* place signal for forced error exit here */
    }
        
    CyaSSL_set_fd(ssl, connfd);

    /* respond to client */
    n = CyaSSL_read(ssl, buf, MAXLINE);
    if (n > 0) {
        printf("%s\n", buf);
        if (CyaSSL_write(ssl, response, strlen(response)) != strlen(response)) {
            printf("Fatal error :respond: write error\n");
            /* place signal for forced error exit here */
        }
    }
    if (n < 0) {
        printf("Fatal error : respond: read error\n");
        /* place signal for forced error exit here */
    }
   
    /* closes the connections after responding */
    CyaSSL_shutdown(ssl);
    CyaSSL_free(ssl);
    if (close(connfd) == -1) {
        printf("Fatal error : close error\n"); 
        /* place signal for forced error exit here */
    }

    pthread_exit(NULL);
}

int main()
{
    int                 listenfd, connfd;
    int                 opt;
    struct sockaddr_in  cliAddr, servAddr;
    char                buff[MAXLINE];
    socklen_t           cliLen;
    pthread_t           thread;
    void*               cyassl_thread(void*);

    CyaSSL_Init();
    
    if ((ctx = CyaSSL_CTX_new(CyaSSLv23_server_method())) == NULL)
        printf("Fatal error : CyaSSL_CTX_new error\n");

    /* use psk suite for security */ 
    CyaSSL_CTX_set_psk_server_callback(ctx, my_psk_server_cb);
    CyaSSL_CTX_use_psk_identity_hint(ctx, "cyassl server");
    if (CyaSSL_CTX_set_cipher_list(ctx, "PSK-AES128-CBC-SHA256")
                                   != SSL_SUCCESS)
        printf("Fatal error : server can't set cipher list");

    /* find a socket */ 
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        printf("Fatal error : socket error");
    }

    /* set up server address and port */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port        = htons(SERV_PORT);

    /* bind to a socket */
    opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt,
               sizeof(int))) {
        return 1;                                                
    }
    
    if (bind(listenfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        printf("Fatal error : bind error");
        return 1;   
    }
    
    /* main loop for accepting and responding to clients */
    for ( ; ; ) {
        /* listen to the socket */   
        if (listen(listenfd, LISTENQ) < 0) {
            printf("Fatal error : listen error");
            return 1;
        }

        cliLen = sizeof(cliAddr);
        connfd = accept(listenfd, (struct sockaddr *) &cliAddr, &cliLen);
        if (connfd < 0) {
            printf("Fatal error : accept error");
            return 1;
        }
        else {
            printf("Connection from %s, port %d\n",
                   inet_ntop(AF_INET, &cliAddr.sin_addr, buff, sizeof(buff)),
                   ntohs(cliAddr.sin_port));
            
            if (pthread_create(&thread, NULL, &cyassl_thread, (void*) connfd) 
                               != 0) {
                return 1;   
            }
            if (pthread_detach(thread) != 0) {
                return 1;   
            }
        }
    }

    /* free up memory used by cyassl */
    CyaSSL_CTX_free(ctx);
    CyaSSL_Cleanup();

    return 0;
}

