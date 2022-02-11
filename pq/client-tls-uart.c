/* client-tls-uart.c
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
 * Example for TLS client over UART
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
gcc -lwolfssl -o client-tls-uart client-tls-uart.c
*/

#ifndef UART_DEV
    #ifdef __MACH__
        #define UART_DEV "/dev/cu.usbmodem14502"
    #else
        #define UART_DEV "/dev/ttyUSB0"
    #endif
#endif
#ifndef B115200
#define B115200 115200
#endif

/* Max buffer for a single TLS frame */
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

#ifdef DEBUG_UART_IO
    printf("UART Read: In %d\n", sz);
#endif

    /* Is there pending data, return it */
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

        /* Read remain */
        do {
            remain = sz - recvd;
            ret = read(cbCtx->portFd, buf + recvd, remain);
            if (ret > 0) {
                recvd += ret;
            }
        } while (ret >= 0 && recvd < sz);

        /* Read until 0 or out of space */
        do {
            avail = (int)sizeof(cbCtx->buf) - cbCtx->pos;
            ret = read(cbCtx->portFd, cbCtx->buf + cbCtx->pos, avail);
            if (ret > 0)
                cbCtx->pos += ret;
        } while (ret > 0);
    }

    if (recvd == 0) {
        recvd = WOLFSSL_CBIO_ERR_WANT_READ;
    }

#ifdef DEBUG_UART_IO
    printf("UART Read: Out %d\n", recvd);
#endif

    return recvd;
}

static int uartIOTx(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int sent;
    CbCtx_t* cbCtx = (CbCtx_t*)ctx;

#ifdef DEBUG_UART_IO
    printf("UART Write: In %d\n", sz);
#endif

    sent = write(cbCtx->portFd, buf, sz);
    if (sent == 0) {
        return WOLFSSL_CBIO_ERR_WANT_WRITE;
    }

#ifdef DEBUG_UART_IO
    printf("UART Write: Out %d\n", sent);
#endif

    return sent;
}

int main(int argc, char** argv)
{
    int ret = -1, err;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    CbCtx_t cBctx;
    struct termios tty;
    const char testStr[] = "Testing 1, 2 and 3\r\n";
    byte readBuf[100];
    const char* uartDev = UART_DEV;

    if (argc >= 2) {
        uartDev = argv[1];
    }

    /* Open UART file descriptor */
    XMEMSET(&cBctx, 0, sizeof(cBctx));
    cBctx.portFd = open(uartDev, O_RDWR | O_NOCTTY);
    if (cBctx.portFd < 0) {
        printf("Error opening %s: Error %i (%s)\n",
            uartDev, errno, strerror(errno));
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

    /* Flush any data in the RX buffer */
    ret = read(cBctx.portFd, readBuf, sizeof(readBuf));
    if (ret < 0) {
        /* Ignore RX error on flush */
    }

#if 0
    wolfSSL_Debugging_ON();
#endif

    ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
    if (ctx == NULL) {
        printf("Error creating WOLFSSL_CTX\n");
        goto done;
    }

    /* Register wolfSSL send/recv callbacks */
    wolfSSL_CTX_SetIOSend(ctx, uartIOTx);
    wolfSSL_CTX_SetIORecv(ctx, uartIORx);

    /* For testing disable peer cert verification; the focus is on key
     * establishment via post-quantum KEM. */
    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("Error creating WOLFSSL\n");
        goto done;
    }

    /* Register wolfSSL read/write callback contexts */
    wolfSSL_SetIOReadCtx(ssl, &cBctx);
    wolfSSL_SetIOWriteCtx(ssl, &cBctx);

#ifdef HAVE_PQC
    printf("Using quantum-safe KYBER_LEVEL1.\n");
    if (wolfSSL_UseKeyShare(ssl, WOLFSSL_KYBER_LEVEL1) != WOLFSSL_SUCCESS) {
        printf("wolfSSL_UseKeyShare Error!!");
    }
#else
    printf("NOTE: Using default group for key exchange.\n");
#endif

    do {
        ret = wolfSSL_connect(ssl);
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);
    if (ret != WOLFSSL_SUCCESS) {
        printf("TLS connect error %d\n", err);
        goto done;
    }
    printf("TLS Connect handshake done\n");

    printf("Sending test string\n");
    do {
        ret = wolfSSL_write(ssl, testStr, XSTRLEN(testStr));
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);
    printf("Sent (%d): %s\n", err, testStr);

    XMEMSET(readBuf, 0, sizeof(readBuf));
    do {
        ret = wolfSSL_read(ssl, readBuf, sizeof(readBuf)-1);
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);
    printf("Read (%d): %s\n", err, readBuf);

    ret = 0; /* Success */

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
