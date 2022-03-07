/* client-psk-nonblocking.c
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
 **/

#include <wolfssl/options.h> /* included for options sync */
#include <wolfssl/ssl.h>     /* must include this to use wolfSSL security */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define     MAXLINE 256      /* max text line length */
#define     SERV_PORT 11111  /* default port*/
#define     PSK_KEY_LEN 4

/*
 * enum used for tcp_select function
 */
enum {
    TEST_SELECT_FAIL = 0,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};

#ifndef NO_PSK
/*
 *psk client set up.
 */
static inline unsigned int My_Psk_Client_Cb(WOLFSSL* ssl, const char* hint,
        char* identity, unsigned int id_max_len, unsigned char* key,
        unsigned int key_max_len)
{
    (void)ssl;
    (void)hint;
    (void)key_max_len;

    /* identity is OpenSSL testing default for openssl s_client, keep same*/
    strncpy(identity, "Client_identity", id_max_len);

    /* test key n hex is 0x1a2b3c4d , in decimal 439,041,101, we're using
     * unsigned binary */
    key[0] = 26;
    key[1] = 43;
    key[2] = 60;
    key[3] = 77;

    return PSK_KEY_LEN;
}
#endif

int main(int argc, char **argv)
{
    int sockfd = SOCKET_INVALID;
    int ret, error, currTimeout;
    int select_ret = TEST_SELECT_FAIL;
    int nfds;
    int result;
    char sendline[MAXLINE]="Hello Server"; /* string to send to the server */
    char recvline[MAXLINE]; /* string received from the server */
    fd_set recvfds, errfds;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;
    struct timeval timeout;
    struct sockaddr_in servaddr;

    /* must include an ip address of this will flag */
    if (argc != 2) {
        printf("Usage: tcpClient <IPaddress>\n");
        return -1;
    }

    /* create a stream socket using tcp,internet protocal IPv4,
     * full-duplex stream */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    /* places n zero-valued bytes in the address servaddr */
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);

    /* converts IPv4 addresses from text to binary form */
    ret = inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    if (ret != 1) {
        printf("inet_pton error\n");
        return -1;
    }

    /* attempts to make a connection on a socket */
    ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (ret != 0) {
        printf("Connection Error\n");
        goto exit;
    }

    /* invokes the fcntl callable service to get the file status
     * flags for a file. checks if it returns an error, if it does
     * stop program */
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        printf("fcntl get failed\n");
        ret = -1;
        goto exit;
    }

    /* invokes the fcntl callable service to set file status flags.
     * Do not block an open, a read, or a write on the file
     * (do not wait for terminal input. If an error occurs,
     * stop program */
    flags = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    if (flags < 0) {
        printf("fcntl set failed\n");
        ret = -1;
        goto exit;
    }

    wolfSSL_Init();  /* initialize wolfSSL */

    /* create and initialize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        ret = -1;
        goto exit;
    }

#ifndef NO_PSK
    /* set up pre shared keys */
    wolfSSL_CTX_set_psk_client_callback(ctx,My_Psk_Client_Cb);
#else
    fprintf(stderr, "Warning: wolfSSL not built with PSK (--enable-psk)\n");
#endif

    /* create wolfSSL object after each tcp connect */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "wolfSSL_new error.\n");
        ret = -1;
        goto exit;
    }

    /* associate the file descriptor with the session */
    wolfSSL_set_fd(ssl, sockfd);

    /* setting up and running nonblocking socket */
    ret    = wolfSSL_connect(ssl);
    error  = wolfSSL_get_error(ssl, 0);

    while (ret != WOLFSSL_SUCCESS && (error == WOLFSSL_ERROR_WANT_READ ||
                                  error == WOLFSSL_ERROR_WANT_WRITE)) {
        currTimeout = 1;

        if (error == WOLFSSL_ERROR_WANT_READ) {
            printf("... client would read block\n");
        }
        else {
            printf("... client would write block\n");
        }

        timeout.tv_sec = currTimeout;
        timeout.tv_usec = 0;            /* setting to 0 microseconds */
        sockfd = (int)wolfSSL_get_fd(ssl);

        FD_ZERO(&recvfds);
        FD_SET(sockfd, &recvfds);
        FD_ZERO(&errfds);
        FD_SET(sockfd, &errfds);

        nfds = sockfd + 1;

        result = select(nfds, &recvfds, NULL, &errfds, &timeout);

        if (result == 0) {
            select_ret =  TEST_TIMEOUT;
        }
        else if (result > 0) {
            if (FD_ISSET(sockfd, &recvfds)) {
                select_ret =  TEST_RECV_READY;
            }
            else if(FD_ISSET(sockfd, &errfds)) {
                select_ret =  TEST_ERROR_READY;
            }
        }
        else {
            select_ret = TEST_SELECT_FAIL;
        }

        if ((select_ret == TEST_RECV_READY) ||
            (select_ret == TEST_ERROR_READY)) {
            ret   = wolfSSL_connect(ssl);
            error = wolfSSL_get_error(ssl, 0);
        }
        else if (select_ret == TEST_TIMEOUT) {
            error = WOLFSSL_ERROR_WANT_READ;
        }
        else {
            error = WOLFSSL_FATAL_ERROR;
        }
    }
    if (ret != WOLFSSL_SUCCESS){
        printf("wolfSSL_connect failed");
        goto exit;
    }

    /* takes inputting string and outputs it to the server */
    /* write string to the server */
    if (wolfSSL_write(ssl, sendline, MAXLINE) != sizeof(sendline)) {
        printf("Write Error to Server\n");
        ret = -1;
        goto exit;
    }

    /* flags if the Server stopped before the client could end */
    while (wolfSSL_read(ssl, recvline, MAXLINE) == -1 ) {
        if (wolfSSL_want_read(ssl)) {
            continue;
        }
        printf("Client: Server Terminated Prematurely!\n");
        ret = -1;
        goto exit;
    }

    /* show message from the server */
    printf("Server Message: %s\n", recvline);

    ret = 0;

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */

    return ret;                 /* Return reporting a success               */

}
