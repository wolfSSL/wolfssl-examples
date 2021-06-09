/* file-server.c
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
#include <sys/errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/certs_test.h>
#include <wolfssl/wolfcrypt/types.h>

#define MAXSZ      1024
/* Use the certs already available in the examples repo */
#define serverCert "../../certs/server-cert.pem"
#define serverKey  "../../certs/server-key.pem"
#define CR         "../file-client/client_read_server_write_file.txt"
#define SR         "../file-client/client_write_server_read_file.txt"
/*--------------------------------------------------------------*/
/* Function Prototypes and global variables */
/*--------------------------------------------------------------*/
unsigned int my_psk_server_cb(WOLFSSL* ssl, const char* identity,
        unsigned char* Xkey, unsigned int key_max_len);

int CbIOSend(WOLFSSL *ssl, char *buf, int sz, void *ctx);
int CbIORecv(WOLFSSL *ssl, char *buf, int sz, void *ctx);
int ConvertHexToBin(const char* h1, byte* b1, word32* b1Sz);
WOLFSSL* Server(WOLFSSL_CTX* ctx, char* suite, int setSuite);

static int fpSend;
static int fpRecv;
static int verboseFlag = 0;
/*--------------------------------------------------------------*/
/* Function implementations */
/*--------------------------------------------------------------*/
int CbIORecv(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int ret = 0;
    int i;
    while (ret <= 0)
        ret = read(fpRecv, buf, sz);
    if (verboseFlag == 1) {
        printf("SERVER WANTS TO READ: %d bytes\n", sz);
        printf("/*------------------- SERVER READING ------------------*/\n");
        for (i = 0; i < sz; i++) {
            printf("%02x ", (unsigned char) buf[i]);
            if (i > 0 && (i % 16) == 0)
                printf("\n");
        }
        printf("\n/*------------------- SERVER READING ------------------*/\n");
    }
    return ret;
}

int CbIOSend(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int ret;
    int i;
    ret = write(fpSend, buf, sz);
    if (verboseFlag == 1) {
        printf("/*------------------- SERVER SENDING ------------------*/\n");
        for (i = 0; i < sz; i++) {
            printf("%02x ", (unsigned char) buf[i]);
            if (i > 0 && (i % 16) == 0)
                printf("\n");
        }
        printf("\n/*------------------- SERVER SENDING ------------------*/\n");
    }
    return ret;
}


WOLFSSL* Server(WOLFSSL_CTX* ctx, char* suite, int setSuite)
{
    WOLFSSL* ssl;
    int ret = -1;

    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        printf("Error in setting server ctx\n");
        return NULL;
    }

#ifndef NO_PSK
    wolfSSL_CTX_SetTmpDH_buffer(ctx, dh_key_der_1024, sizeof_dh_key_der_1024,
                                                        SSL_FILETYPE_ASN1);
#endif

    if (wolfSSL_CTX_use_certificate_file(ctx, serverCert, SSL_FILETYPE_PEM)
                                                    != SSL_SUCCESS) {
        printf("trouble loading server cert file\n");
        return NULL;
    }

    if (wolfSSL_CTX_use_PrivateKey_file(ctx, serverKey, SSL_FILETYPE_PEM)
                                                    != SSL_SUCCESS) {
        printf("trouble loading server key file\n");
        return NULL;
    }

    if (setSuite == 1) {
        if (( ret = wolfSSL_CTX_set_cipher_list(ctx, suite)) != SSL_SUCCESS) {
            printf("ret = %d\n", ret);
            printf("Error :can't set cipher\n");
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
    char sMsg[] = "I hear you fashizzle\r\n";
    char reply[MAXSZ];
    int ret, msgSz;
    WOLFSSL* sslServ;
    WOLFSSL_CTX* ctxServ = NULL;

    if (argc == 2) {
        if (XSTRNCMP(argv[1], "-verbose", 8) == 0 ||
            XSTRNCMP(argv[1], "-v", 2)       == 0) {
            verboseFlag = 1;
        }
    }

    fpSend = open(CR, O_WRONLY | O_NOCTTY | O_NDELAY);
    fpRecv = open(SR, O_RDONLY | O_NOCTTY | O_NDELAY);

    if (verboseFlag == 1) {
        wolfSSL_Debugging_ON();
    }

    wolfSSL_Init();

    /* Example usage */
//    sslServ = Server(ctxServ, "ECDHE-RSA-AES128-SHA", 1);
    sslServ = Server(ctxServ, "let-wolfssl-choose", 0);

    if (sslServ == NULL) { printf("sslServ NULL\n"); return 0;}
    ret = SSL_FAILURE;
    printf("Starting server\n");
    while (ret != SSL_SUCCESS) {
        int error;
        ret = wolfSSL_accept(sslServ);
        error = wolfSSL_get_error(sslServ, 0);
        if (ret != SSL_SUCCESS) {
            if (error != SSL_ERROR_WANT_READ &&
                error != SSL_ERROR_WANT_WRITE) {
                wolfSSL_free(sslServ);
                wolfSSL_CTX_free(ctxServ);
                printf("server ssl accept failed ret = %d error = %d wr = %d\n",
                                               ret, error, SSL_ERROR_WANT_READ);
                goto cleanup;
            }
        }

    }


    /* read */
    while (1) {
        int error;

        /* server send/read */
        memset(reply, 0, sizeof(reply));
        ret  = wolfSSL_read(sslServ, reply, sizeof(reply) - 1);
        error = wolfSSL_get_error(sslServ, 0);
        if (ret < 0) {
            if (error != SSL_ERROR_WANT_READ &&
                error != SSL_ERROR_WANT_WRITE) {
                printf("server read failed\n");
                break;
            }
        }
        else {
            reply[ret] = 0;
            printf("Server Received : %s\n", reply);
            break;
        }
    }

    /* write */
    while (1) {
        int error;

        msgSz = sizeof(sMsg);
        ret = wolfSSL_write(sslServ, sMsg, msgSz);
        error = wolfSSL_get_error(sslServ, 0);
        if (ret != msgSz) {
            if (error != SSL_ERROR_WANT_READ &&
                error != SSL_ERROR_WANT_WRITE) {
                printf("server write failed\n");
                break;
            }
        } else if (ret == msgSz) {
            printf("Server send successful\n");
            break;
        } else {
            printf("Unkown error occurred, shutting down\n");
            break;
        }
    }


cleanup:
    /* close the streams so client can reset file contents */
    close(fpRecv);
    close(fpSend);
    close(open(CR, O_RDWR | O_NOCTTY | O_NDELAY));
    close(open(SR, O_RDWR | O_NOCTTY | O_NDELAY));

    wolfSSL_shutdown(sslServ);
    wolfSSL_free(sslServ);
    wolfSSL_CTX_free(ctxServ);
    wolfSSL_Cleanup();
    /* Reset the contents of the receive and send files for next run */
    fclose(fopen(SR, "wb"));
    fclose(fopen(CR, "wb"));

    return -1;
}
