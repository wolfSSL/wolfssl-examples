/* client-psk-bio-custom.c
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
#include <wolfssl/openssl/bio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>

#if defined(OPENSSL_EXTRA) && !defined(NO_PSK)

#define     MAXLINE 256      /* max text line length */
#define     SERV_PORT 11111  /* default port*/
#define     PSK_KEY_LEN 4

static int sockfd;


static int bioWriteCb(WOLFSSL_BIO* bio, const char* in, int inSz)
{
    int ret;

    ret = write(sockfd, in, inSz);
    printf("Custom WOLFSSL_BIO wrote %d bytes\n", ret);
    return ret;
}


static int bioReadCb(WOLFSSL_BIO* bio, char* out, int outSz)
{
    int ret, idx = 0;

    printf("Custom WOLFSSL_BIO trying to read %d bytes\n", outSz);
    do {
        ret = read(sockfd, out + idx, outSz - idx);
        if (ret < 0) {
            printf("error %d with read call\n", ret);
            break;
        }
        idx += ret;
    } while (idx < outSz);
    printf("Custom WOLFSSL_BIO read %d bytes\n", idx);
    return idx;
}


static int bioCreateCb(WOLFSSL_BIO* bio)
{
    printf("Creating custom WOLFSSL_BIO\n");
    return WOLFSSL_SUCCESS;
}


static int bioDestroyCb(WOLFSSL_BIO* bio)
{
    printf("Tearing down custom WOLFSSL_BIO\n");
    return WOLFSSL_SUCCESS;
}


static long bioCTRLCb(WOLFSSL_BIO* bio, int cmd, long larg, void* data)
{
    int ret = 0;

    switch (cmd) {
        case BIO_CTRL_PENDING:
        case BIO_CTRL_WPENDING:
            printf("getting pending custom BIO bytes\n");
            break;

        default:
            printf("unsupported command with custom BIO\n");
            ret = WOLFSSL_FAILURE;
    }
    return ret;
}

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

static int createSocket(char* ip)
{
    struct sockaddr_in servaddr;;
    int ret;

    /* create a stream socket using tcp,internet protocal IPv4,
     * full-duplex stream */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    /* places n zero-valued bytes in the address servaddr */
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);

    /* converts IPv4 addresses from text to binary form */
    ret = inet_pton(AF_INET, ip, &servaddr.sin_addr);
    if (ret != 1) {
        printf("inet_pton error\n");
        return 1;
    }

    /* attempts to make a connection on a socket */
    ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (ret != 0) {
        printf("Connection Error\n");
        return 1;
    }
    return 0;
}


int main(int argc, char **argv)
{
    WOLFSSL_BIO_METHOD* myMethod = NULL;

    WOLFSSL_BIO *custom = NULL, *bioSSL = NULL, *bioBuffer = NULL;

    int ret = 0;
    char sendline[MAXLINE]="Hello Server"; /* string to send to the server */
    char recvline[MAXLINE]; /* string received from the server */

    WOLFSSL* ssl = NULL;
    WOLFSSL_CTX* ctx = NULL;

    if (createSocket("127.0.0.1") != 0) {
        fprintf(stderr, "unable to create TCP socket\n");
        return -1;
    }

    wolfSSL_Init();  /* initialize wolfSSL */

    /* create a custom BIO from a custom BIO_METHOD */
    myMethod = wolfSSL_BIO_meth_new(WOLFSSL_BIO_BUFFER, "custom_bio");
    if (myMethod == NULL) {
        fprintf(stderr, "unable to create new custom BIO method\n");
        goto exit;
    }
    if (wolfSSL_BIO_meth_set_write(myMethod, bioWriteCb) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "unable to set write method\n");
        goto exit;
    }
    if (wolfSSL_BIO_meth_set_read(myMethod, bioReadCb) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "unable to set read method\n");
        goto exit;
    }
    if (wolfSSL_BIO_meth_set_ctrl(myMethod, bioCTRLCb) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "unable to set ctrl method\n");
        goto exit;
    }
    if (wolfSSL_BIO_meth_set_create(myMethod, bioCreateCb) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "unable to set create method\n");
        goto exit;
    }
    if (wolfSSL_BIO_meth_set_destroy(myMethod, bioDestroyCb) !=
            WOLFSSL_SUCCESS) {
        fprintf(stderr, "unable to set destroy method\n");
        goto exit;
    }
    custom = wolfSSL_BIO_new(myMethod);
    bioSSL = wolfSSL_BIO_new(wolfSSL_BIO_f_ssl());
    bioBuffer = wolfSSL_BIO_new(wolfSSL_BIO_f_buffer());

    if (custom == NULL || bioSSL == NULL || bioBuffer == NULL) {
        fprintf(stderr, "error creating bio's\n");
        goto exit;
    }

    /* create BIO chain bioBuffer -> bioSSL */
    bioBuffer = wolfSSL_BIO_push(bioBuffer, bioSSL);

    /* create and initialize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        goto exit;
    }

#ifndef NO_PSK
    /* set up pre shared keys */
    wolfSSL_CTX_set_psk_client_callback(ctx, My_Psk_Client_Cb);
#else
    fprintf(stderr, "Warning: wolfSSL not built with PSK (--enable-psk)\n");
#endif

    /* creat wolfssl object after each tcp connect */
    if ( (ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "wolfSSL_new error.\n");
        goto exit;
    }
    wolfSSL_set_bio(ssl, custom, custom);
    if (wolfSSL_BIO_set_ssl(bioSSL, ssl, BIO_CLOSE) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_BIO_set_ssl error.\n");
        goto exit;
    }

    /* write string to the server */
    if (wolfSSL_BIO_write(bioBuffer, sendline, MAXLINE) != sizeof(sendline)) {
        printf("Write Error to Server\n");
        goto exit;
    }

    /* check if server ended before client could read a response  */
    if (wolfSSL_BIO_read(bioBuffer, recvline, MAXLINE) < 0 ) {
        printf("Client: Server Terminated Prematurely!\n");
        goto exit;
    }

    /* show message from the server */
    printf("Server Message: %s\n", recvline);

exit:
    wolfSSL_BIO_free_all(bioBuffer);

    /* when completely done using SSL/TLS, free the wolfssl_ctx object */
    wolfSSL_CTX_free(ctx);
    wolfSSL_BIO_meth_free(myMethod);
    wolfSSL_Cleanup();
    close(sockfd);

    /* exit client */
    return ret;
}

#else

int main()
{
    printf("To use this example please recompile wolfssl with\n --enable-psk"
           " --enable-opensslextra\n");
    return 0;
}
#endif
