/* file-client.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 *
 * EXAMPLE DEFINITION:
 *
 * Instead of using sockets we will show how it is possible
 * to deliver TLS handshake and packet exchange through the use of
 * the file system! We will accomplish this using the wolfSSL
 * IO callbacks to read/write to files instead of using sockets
 */

/*--------------------------------------------------------------*/
/* Header files and definitions */
/*--------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/certs_test.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/types.h>

#define MAXSZ         1024
/* use the certs already available in the examples repo */
#define clientCert    "../../certs/client-cert.pem"
#define peerAuthority "../../certs/server-cert.pem"
#define CR            "client_read_server_write_file.txt"
#define SR            "client_write_server_read_file.txt"

/*--------------------------------------------------------------*/
/* Function Prototypes and global variables */
/*--------------------------------------------------------------*/
unsigned int my_psk_client_cb(WOLFSSL* ssl, const char* hint,
                              char* identity, unsigned int id_max_len,
                              unsigned char* Xkey, unsigned int key_max_len);

int CbIOSend(WOLFSSL *ssl, char *buf, int sz, void *ctx);
int CbIORecv(WOLFSSL *ssl, char *buf, int sz, void *ctx);
WOLFSSL* Client(WOLFSSL_CTX* ctx, char* suite, int setSuite, int doVerify);
WOLFSSL_METHOD* SetMethodClient(int i);


struct WOLFSSL_SOCKADDR {
    unsigned int sz;
    void*        sa;
};

static int fpSend;
static int fpRecv;
static int verboseFlag = 0;

/*--------------------------------------------------------------*/
/* Function implementations */
/*--------------------------------------------------------------*/
int CbIORecv(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    (void) ssl; /* will not need ssl context, just using the file system */
    (void) ctx; /* will not need ctx, we're just using the file system */
    int ret = 0;
    int i;

    while (ret <= 0)
        ret = (int) read(fpRecv, buf, (size_t) sz);
    if (verboseFlag == 1) {
        printf("/*-------------------- CLIENT READING -----------------*/\n");
        for (i = 0; i < ret; i++) {
            printf("%02x ", (unsigned char)buf[i]);
            if (i > 0 && (i % 16) == 0)
                printf("\n");
        }
        printf("\n/*-------------------- CLIENT READING -----------------*/\n");
    }

    return ret;
}

int CbIOSend(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    (void) ssl; /* will not need ssl context, just using the file system */
    (void) ctx; /* will not need ctx, we're just using the file system */
    int ret;
    int i;

    ret = (int) write(fpSend, buf, (size_t) sz);
    if (verboseFlag == 1) {
        printf("/*-------------------- CLIENT SENDING -----------------*/\n");
        for (i = 0; i < sz; i++) {
            printf("%02x ", (unsigned char) buf[i]);
            if (i > 0 && (i % 16) == 0)
                printf("\n");
        }
        printf("\n/*-------------------- CLIENT SENDING -----------------*/\n");
    } else {
        (void) i;
    }

    return ret;
}

WOLFSSL* Client(WOLFSSL_CTX* ctx, char* suite, int setSuite, int doVerify)
{
    WOLFSSL*     ssl = NULL;
    int ret;

    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        printf("Error in setting client ctx\n");
        return NULL;
    }

    if (doVerify == 1) {
        if ((wolfSSL_CTX_load_verify_locations(ctx, peerAuthority, 0))
                                                              != SSL_SUCCESS) {
            printf("Failed to load CA (peer Authority) file\n");
            return NULL;
        }
    } else {
        wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
    }

    if (setSuite == 1) {
        if ((ret = wolfSSL_CTX_set_cipher_list(ctx, suite)) != SSL_SUCCESS) {
            printf("ret = %d\n", ret);
            printf("can't set cipher\n");
            wolfSSL_CTX_free(ctx);
            return NULL;
        }
    } else {
        (void) suite;
    }

    wolfSSL_SetIORecv(ctx, CbIORecv);
    wolfSSL_SetIOSend(ctx, CbIOSend);

    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("issue when creating ssl\n");
        wolfSSL_CTX_free(ctx);
        return NULL;
    }

    wolfSSL_set_fd(ssl, fpRecv);

    return ssl;
}


int main(int argc, char** argv)
{
    char msg[] = "Hello wolfSSL\r\n";
    char reply[MAXSZ];
    int    ret, msgSz;
    WOLFSSL* sslCli;
    WOLFSSL_CTX* ctxCli = NULL;

    if (argc == 2) {
        if (XSTRNCMP(argv[1], "-verbose", 8) == 0 ||
            XSTRNCMP(argv[1], "-v", 2)       == 0) {
            verboseFlag = 1;
        }
    }

    fpSend = open(SR, O_WRONLY | O_NOCTTY | O_NDELAY);
    fpRecv = open(CR, O_RDONLY | O_NOCTTY | O_NDELAY);

    wolfSSL_Init();

    /* Example usage */
//    sslServ = Server(ctxServ, "ECDHE-RSA-AES128-SHA", 1);
    sslCli  = Client(ctxCli, "let-wolfssl-decide", 0, 1);

    if (sslCli == NULL) {
        printf("Failed to start client\n");
        goto cleanup;
    }

    ret = SSL_FAILURE;

    printf("Starting client\n");
    while (ret != SSL_SUCCESS) {
        int error;

        /* client connect */
        ret |= wolfSSL_connect(sslCli);
        error = wolfSSL_get_error(sslCli, 0);
        if (ret != SSL_SUCCESS) {
            if (error != SSL_ERROR_WANT_READ &&
                error != SSL_ERROR_WANT_WRITE) {
                wolfSSL_free(sslCli);
                wolfSSL_CTX_free(ctxCli);
                printf("client ssl connect failed\n");
                goto cleanup;
            }
        }
        printf("Client connected successfully...\n");
    }


    /* read and write */
    while (1) {
        int error;

        /* client send/read */
        msgSz = (int) strlen(msg);
        ret   = wolfSSL_write(sslCli, msg, msgSz);
        error = wolfSSL_get_error(sslCli, 0);
        if (ret != msgSz) {
            if (error != SSL_ERROR_WANT_READ &&
                error != SSL_ERROR_WANT_WRITE) {
                printf("client write failed\n");
                break;
            }
        }

        ret = wolfSSL_read(sslCli, reply, sizeof(reply) - 1);
        error = wolfSSL_get_error(sslCli, 0);
        if (ret < 0) {
            if (error != SSL_ERROR_WANT_READ &&
                error != SSL_ERROR_WANT_WRITE) {
                printf("client read failed\n");
                break;
            }
        }
        else {
            reply[ret] = '\0';
            printf("Client Received Reply: %s\n", reply);
            break;
        }

    }

cleanup:

    wolfSSL_shutdown(sslCli);
    wolfSSL_free(sslCli);
    wolfSSL_CTX_free(ctxCli);
    wolfSSL_Cleanup();
    /* close the streams so client can reset file contents */
    close(fpSend);
    close(fpRecv);
    close(open(CR, O_RDWR | O_NOCTTY | O_NDELAY));
    close(open(SR, O_RDWR | O_NOCTTY | O_NDELAY));

    return -1;
}
