
/* client-psk-tls13-multi-id.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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
 */

/* A client example using a TCP connection with PSK security showing
 * PSK with identity.
 */

#include <wolfssl/options.h> /* included for options sync */
#include <wolfssl/ssl.h>     /* must include this to use wolfSSL security */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>

#define     MAXLINE 256      /* max text line length */
#define     SERV_PORT 11111  /* default port*/
#define     PSK_KEY_LEN 4

#ifndef NO_PSK
/*
 * psk client set up.
 */
static inline unsigned int My_Tls13_Psk_Client_Cs_Cb(WOLFSSL* ssl,
    const char* hint, char* identity, unsigned int id_max_len,
    unsigned char* key, unsigned int key_max_len, const char* ciphersuite)
{
    (void)ssl;
    (void)hint;
    (void)key_max_len;

    if (strncmp(ciphersuite, "TLS13-AES128-GCM-SHA256",
                XSTRLEN(ciphersuite)) == 0) {

        /* identity is OpenSSL testing default for openssl s_client, keep same*/
        strncpy(identity, "Client_Id_AES", id_max_len);

        /* test key n hex is 0x1a2b3c4d */
        key[0] = 0x1a;
        key[1] = 0x2b;
        key[2] = 0x3c;
        key[3] = 0x4d;
    }
    else if (strncmp(ciphersuite, "TLS13-CHACHA20-POLY1305-SHA256",
                     XSTRLEN(ciphersuite)) == 0) {

        /* identity is OpenSSL testing default for openssl s_client, keep same*/
        strncpy(identity, "Client_Id_ChaCha", id_max_len);

        /* test key n hex is 0xa1b2c3d4 */
        key[0] = 0xa1;
        key[1] = 0xb2;
        key[2] = 0xc3;
        key[3] = 0xd4;
    }
    else {
        return 0;
    }

    return PSK_KEY_LEN;
}
#endif

int main(int argc, char **argv)
{
    int ret;
    int sockfd = SOCKET_INVALID;
    char sendline[MAXLINE]="Hello Server"; /* string to send to the server */
    char recvline[MAXLINE]; /* string received from the server */
    struct sockaddr_in servaddr;;

    WOLFSSL* ssl = NULL;
    WOLFSSL_CTX* ctx = NULL;

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
        ret = -1;
        goto exit;
    }

    /* attempts to make a connection on a socket */
    ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (ret != 0) {
        printf("Connection Error\n");
        ret = -1;
        goto exit;
    }


    wolfSSL_Init();  /* initialize wolfSSL */

    /* create and initialize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        ret = -1;
        goto exit;
    }

#ifndef NO_PSK
    /* set up pre shared keys */
    wolfSSL_CTX_set_psk_client_cs_callback(ctx, My_Tls13_Psk_Client_Cs_Cb);
#else
    fprintf(stderr, "Warning: wolfSSL not built with PSK (--enable-psk)\n");
#endif

    /* creat wolfssl object after each tcp connect */
    if ( (ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "wolfSSL_new error.\n");
        ret = -1;
        goto exit;
    }

    /* associate the file descriptor with the session */
    ret = wolfSSL_set_fd(ssl, sockfd);
    if (ret != WOLFSSL_SUCCESS) {
        goto exit;
    }

    /* write string to the server */
    if (wolfSSL_write(ssl, sendline, MAXLINE) != sizeof(sendline)) {
        printf("Write Error to Server\n");
        ret = -1;
        goto exit;
    }

    /* check if server ended before client could read a response  */
    if (wolfSSL_read(ssl, recvline, MAXLINE) < 0 ) {
        printf("Client: Server Terminated Prematurely!\n");
        ret = -1;
        goto exit;
    }

    WOLFSSL_CIPHER* cipher = wolfSSL_get_current_cipher(ssl);
    printf("Cipher suite: %s\n", wolfSSL_CIPHER_get_name(cipher));

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
