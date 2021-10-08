/* server-tls-uart.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
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
 * Example for TLS over UART
 */


#define _XOPEN_SOURCE 600
#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in6 */
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/wc_port.h>

/* build with:
gcc -lwolfssl -o server-tls-uart.c server-tls-uart.c
*/

#ifndef UART_DEV
    #ifdef __MACH__
        #define UART_DEV "/dev/cu.usbmodem14502"
    #else
        #define UART_DEV "/dev/ttyACM0"
    #endif
#endif
#ifndef B115200
#define B115200 115200
#endif

/* max buffer for a single TLS frame */
#ifndef MAX_RECORD_SIZE
#define MAX_RECORD_SIZE (16 * 1024)
#endif

typedef struct CbCtx {
    int portFd;
    byte buf[MAX_RECORD_SIZE];
    int pos;
} CbCtx_t;

static int uartIORx(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int ret, recvd = 0;
    CbCtx_t* cbCtx = (CbCtx_t*)ctx;

    printf("Read: In %d\n", sz);

    /* is there pending data, return it */
    if (cbCtx->pos > 0) {
        recvd = cbCtx->pos;
        if (recvd > sz)
            recvd = sz;
        XMEMCPY(buf, cbCtx->buf, recvd);
        XMEMCPY(cbCtx->buf, cbCtx->buf + recvd, cbCtx->pos - recvd);
        cbCtx->pos -= recvd;
    }

    if (recvd < sz && cbCtx->pos == 0) {
        int avail, remain;

        /* read remain */
        do {
            remain = sz - recvd;
            ret = read(cbCtx->portFd, buf + recvd, remain);
            if (ret > 0) {
                recvd += ret;
            }
        } while (ret >= 0 && recvd < sz);

        /* read until 0 or out of space */
        do {
            avail = (int)sizeof(cbCtx->buf) - cbCtx->pos;
            ret = read(cbCtx->portFd, cbCtx->buf + cbCtx->pos, avail);
            if (ret > 0)
                cbCtx->pos += ret;
        } while (ret > 0);
    }

#if 0
    if (logIF && recvd) {
        logIF->writeRaw(logIF, buf, recvd);
    }
#endif

    if (recvd == 0) {
        recvd = WOLFSSL_CBIO_ERR_WANT_READ;
    }

    printf("Read: Out %d\n", recvd);

    return recvd;
}

static int uartIOTx(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int sent;
    CbCtx_t* cbCtx = (CbCtx_t*)ctx;

    printf("Write: In %d\n", sz);
    
    sent = write(cbCtx->portFd, buf, sz);
#if 0
    if (logIF && sent) {
        logIF->writeRaw(logIF, buf, sent);
    }
#endif
    if (sent == 0) {
        sent = WOLFSSL_CBIO_ERR_WANT_WRITE;
    }

    printf("Write: Out %d\n", sent);
    return sent;
}


int main(void)
{
    int ret = -1, err;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    CbCtx_t cBctx;
    struct termios tty;
    const char testStr[] = "Testing 1, 2 and 3\r\n";
    byte echoBuffer[100];

    XMEMSET(&cBctx, 0, sizeof(cBctx));
    cBctx.portFd = -1;

    ctx = wolfSSL_CTX_new(wolfSSLv23_server_method()); /* highest available / allow downgrade */
    if (ctx == NULL) {
        printf("Error creating WOLFSSL_CTX\n");
        goto done;
    }

    /* register wolfSSL send/recv callbacks */
    wolfSSL_CTX_SetIOSend(ctx, uartIOTx);
    wolfSSL_CTX_SetIORecv(ctx, uartIORx);

    /* for testing disable peer cert verification */
    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("Error creating WOLFSSL\n");
        goto done;
    }

    /* register wolfSSL read/write callback contexts */
    wolfSSL_SetIOReadCtx(ssl, &cBctx);
    wolfSSL_SetIOWriteCtx(ssl, &cBctx);

    /* open UART file descriptor */
    cBctx.portFd = open(UART_DEV, O_RDWR | O_NOCTTY);
    if (cBctx.portFd < 0) {
        printf("Error opening %s: Error %i (%s)\n", UART_DEV, errno, strerror(errno));
        ret = errno;
        goto done;
    }
    tcgetattr(cBctx.portFd, &tty);
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | (CS8);
    tty.c_iflag &= ~(IGNBRK | IXON | IXOFF | IXANY| INLCR | ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~(ONLCR|OCRNL);
    tty.c_cflag &= ~(PARENB | PARODD | CSTOPB);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~ISTRIP;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;
    tcsetattr(cBctx.portFd, TCSANOW, &tty);

    /* Flush any data in the RX buffer - sure there is a better way to "flush" */
    read(cBctx.portFd, echoBuffer, sizeof(echoBuffer));


    do {
        ret = wolfSSL_accept(ssl);
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);
    if (ret != WOLFSSL_SUCCESS) {
        
    }

    do {
        ret = wolfSSL_write(ssl, testStr, XSTRLEN(testStr));
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);


    XMEMSET(echoBuffer, 0, sizeof(echoBuffer));
    do {
        ret = wolfSSL_read(ssl, echoBuffer, sizeof(echoBuffer)-1);
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);

    ret = 0; /* success */

done:
    if (ssl) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (ctx) {
        wolfSSL_CTX_free(ctx);
    }

    if (cBctx.portFd >= 0)
        close(cBctx.portFd);

    return ret;
}
