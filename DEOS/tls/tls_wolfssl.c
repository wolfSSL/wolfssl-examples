/* tls_wolfssl.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
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
 */

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfcrypt/test/test.h>
#include <wolfcrypt/benchmark/benchmark.h>
#include <wolfssl/wolfcrypt/logging.h> /* to use WOLFSSL_MSG */
#include <tls_wolfssl.h>

#include <code/printx.h>
#define printf printx


int setupTransport(clientConnectionHandleType* connectionHandle,
                   char* connectionId) {
    int ret, error;
    void * sendBuffer;
    uint32_t bufferSizeInBytes;

    if ((ret = socketTransportInitialize("mailbox-transport.config",
                                         "transportConfigurationId",
                                         waitIndefinitely, &error)) != transportSuccess)
        printf("Initialize 0x%x, error=%d\n", ret, error);

    else if ((ret = socketTransportClientInitialize(waitIndefinitely,
                                                    &error)) != transportSuccess)
        printf("ClientInitialize 0x%x, error=%d\n", ret, error);

    else if ((ret = socketTransportCreateConnection(connectionId,
                                                    waitIndefinitely,
                                                    COMPATIBILITY_ID_2,
                                                    connectionHandle,
                                                    &sendBuffer,
                                                    &bufferSizeInBytes,
                                                    &error)) != transportSuccess)
        printf("CreateConnection 0x%x, error=%d\n", ret, error);

    else if ((ret = socketTransportSetConnectionForThread(currentThreadHandle(),
                                                          *connectionHandle,
                                                          waitIndefinitely,
                                                          &error)) != transportSuccess)
        printf("SetConnectionForThread 0x%x, error=%d\n", ret, error);

    return ret;
}

#if !defined(NO_WOLFSSL_CLIENT )


#define TX_BUF_SIZE 64
#define RX_BUF_SIZE 1024

#define TX_MSG "GET /index.html HTTP/1.0\n\n"
#define TX_MSG_SIZE sizeof(TX_MSG)

#include "ca_cert.h"

/* 172.217.3.174 is the IP address of https://www.google.com */
#if 1
#  define TCP_SERVER_IP_ADDR "192.168.19.1"
#  define TCP_SERVER_PORT 11111
#else
#  define TCP_SERVER_IP_ADDR "172.217.3.174"
#  define TCP_SERVER_PORT 443
#endif

void wolfssl_client_test(uintData_t statusPtr) {
    int sock;
    char rx_buf[RX_BUF_SIZE];
    char tx_buf[TX_BUF_SIZE];
    int ret = 0, error = 0;

    sockaddr_in server_addr;
    clientConnectionHandleType TCPclientHandle;

    WOLFSSL* ssl;
    WOLFSSL_CTX* ctx;

    /* set up the mailbox transport */

    if (setupTransport(&TCPclientHandle, (char*)"connectionId1") != transportSuccess){
        printf("TCP transport set up failed \n");
        return;
      }

    printf("Creating a network socket...\n");

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == SOCKET_ERROR) {
        printf("ERROR: Failed to create socket, err = %d\n", errno);
        return;
    }

    printf("Clearing memory for server_addr struct\n");

    XMEMSET((char *) &server_addr, 0u, sizeof(server_addr));

    printf("Connecting to server IP address: %s, port: %d\n",
                    TCP_SERVER_IP_ADDR, TCP_SERVER_PORT);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(TCP_SERVER_IP_ADDR);
    server_addr.sin_port = htons(TCP_SERVER_PORT);

    printf("Calling connect on socket\n");
    if (connect(sock, (sockaddr *) &server_addr, sizeof(server_addr)) < 0 ) {
        printf("ERROR: connect, err = %d\n", errno);
        closesocket(sock);
        return;
    }

    /* chooses the highest possible TLS version */

    ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());

    /* SET UP NETWORK SOCKET */
    if (ctx == 0) {
        printf("ERROR: wolfSSL_CTX_new failed\n");
        closesocket(sock);
        return;
    }

    WOLFSSL_MSG("wolfSSL_CTX_new done");

    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    ret = wolfSSL_CTX_load_verify_buffer_ex(ctx,
                                            ca_certs,
                                            sizeof(ca_certs),
                                            SSL_FILETYPE_PEM,
                                            0,
                                            WOLFSSL_LOAD_FLAG_DATE_ERR_OKAY);

    if (ret != SSL_SUCCESS) {
        printf("ERROR: wolfSSL_CTX_load_verify_buffer() failed\n");
        closesocket(sock);
        wolfSSL_CTX_free(ctx);
        return;
    }
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        printf("ERROR: wolfSSL_new() failed\n");
        closesocket(sock);
        wolfSSL_CTX_free(ctx);
        return;
    }

    WOLFSSL_MSG("wolfSSL_new done");
    ret = wolfSSL_set_fd(ssl, sock);
    if (ret != SSL_SUCCESS) {
        printf("ERROR: wolfSSL_set_fd() failed\n");
        closesocket(sock);
        wolfSSL_free(ssl);
        wolfSSL_CTX_free(ctx);
        return;
    }
    WOLFSSL_MSG("wolfSSL_set_fd done");
    do {
        error = 0; /* reset error */
        ret = wolfSSL_connect(ssl);
        if (ret != SSL_SUCCESS) {
            error = wolfSSL_get_error(ssl, 0);
            printf("ERROR: wolfSSL_connect() failed, err = %d\n", error);
            if (error != SSL_ERROR_WANT_READ) {
                closesocket(sock);
                wolfSSL_free(ssl);
                wolfSSL_CTX_free(ctx);
                return;
            }
            /* goToSleep() for 1 sec*/
        }
    } while ((ret != SSL_SUCCESS) && (error == SSL_ERROR_WANT_READ));

    printf("wolfSSL_connect() ok... sending GET\n");
    XSTRNCPY(tx_buf, TX_MSG, TX_MSG_SIZE);
    if (wolfSSL_write(ssl, tx_buf, TX_MSG_SIZE) != TX_MSG_SIZE) {
        error = wolfSSL_get_error(ssl, 0);
        printf("ERROR: wolfSSL_write() failed, err = %d\n", error);
        closesocket(sock);
        wolfSSL_free(ssl);
        wolfSSL_CTX_free(ctx);
        return;
    }
    do {
        error = 0; /* reset error */
        ret = wolfSSL_read(ssl, rx_buf, RX_BUF_SIZE - 1);
        if (ret < 0) {
            error = wolfSSL_get_error(ssl, 0);
            if (error != SSL_ERROR_WANT_READ) {
                printf("wolfSSL_read failed, error = %d\n", error);
                closesocket(sock);
                wolfSSL_free(ssl);
                wolfSSL_CTX_free(ctx);
                return;
            }
            /* goToSleep() for 1 second*/
        } else if (ret > 0) {
            rx_buf[ret] = 0;
            printf("%s\n", rx_buf);
        }
    } while (error == SSL_ERROR_WANT_READ);
    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    closesocket(sock);
    return;
}

#endif /* NO_WOLFSSL_CLIENT */

#if !defined(NO_WOLFSSL_SERVER)

#define TLS_SERVER_PORT 11111
#define TX_BUF_SIZE 64
#define RX_BUF_SIZE 1024
#define TCP_SERVER_CONN_Q_SIZE 1

/* derived from wolfSSL/certs/server-ecc.der
 *  od -tx1 -An server-ecc.der  | sed  -r 's/ ([0-9a-f]{2})/ 0x\1,/g'
 */

static const unsigned char server_ecc_der_256[] = {
		 0x30, 0x82, 0x02, 0xa1, 0x30, 0x82, 0x02, 0x47, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x03,
		 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x30, 0x81, 0x97, 0x31,
		 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x13, 0x30, 0x11,
		 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x0a, 0x57, 0x61, 0x73, 0x68, 0x69, 0x6e, 0x67, 0x74, 0x6f,
		 0x6e, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0c, 0x07, 0x53, 0x65, 0x61, 0x74,
		 0x74, 0x6c, 0x65, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x07, 0x77, 0x6f,
		 0x6c, 0x66, 0x53, 0x53, 0x4c, 0x31, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x0c, 0x0b,
		 0x44, 0x65, 0x76, 0x65, 0x6c, 0x6f, 0x70, 0x6d, 0x65, 0x6e, 0x74, 0x31, 0x18, 0x30, 0x16, 0x06,
		 0x03, 0x55, 0x04, 0x03, 0x0c, 0x0f, 0x77, 0x77, 0x77, 0x2e, 0x77, 0x6f, 0x6c, 0x66, 0x73, 0x73,
		 0x6c, 0x2e, 0x63, 0x6f, 0x6d, 0x31, 0x1f, 0x30, 0x1d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
		 0x0d, 0x01, 0x09, 0x01, 0x16, 0x10, 0x69, 0x6e, 0x66, 0x6f, 0x40, 0x77, 0x6f, 0x6c, 0x66, 0x73,
		 0x73, 0x6c, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x1e, 0x17, 0x0d, 0x32, 0x31, 0x30, 0x32, 0x31, 0x30,
		 0x31, 0x39, 0x34, 0x39, 0x35, 0x33, 0x5a, 0x17, 0x0d, 0x32, 0x33, 0x31, 0x31, 0x30, 0x37, 0x31,
		 0x39, 0x34, 0x39, 0x35, 0x33, 0x5a, 0x30, 0x81, 0x8f, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55,
		 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c,
		 0x0a, 0x57, 0x61, 0x73, 0x68, 0x69, 0x6e, 0x67, 0x74, 0x6f, 0x6e, 0x31, 0x10, 0x30, 0x0e, 0x06,
		 0x03, 0x55, 0x04, 0x07, 0x0c, 0x07, 0x53, 0x65, 0x61, 0x74, 0x74, 0x6c, 0x65, 0x31, 0x10, 0x30,
		 0x0e, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x07, 0x45, 0x6c, 0x69, 0x70, 0x74, 0x69, 0x63, 0x31,
		 0x0c, 0x30, 0x0a, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x0c, 0x03, 0x45, 0x43, 0x43, 0x31, 0x18, 0x30,
		 0x16, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x0f, 0x77, 0x77, 0x77, 0x2e, 0x77, 0x6f, 0x6c, 0x66,
		 0x73, 0x73, 0x6c, 0x2e, 0x63, 0x6f, 0x6d, 0x31, 0x1f, 0x30, 0x1d, 0x06, 0x09, 0x2a, 0x86, 0x48,
		 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x10, 0x69, 0x6e, 0x66, 0x6f, 0x40, 0x77, 0x6f, 0x6c,
		 0x66, 0x73, 0x73, 0x6c, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86,
		 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
		 0x42, 0x00, 0x04, 0xbb, 0x33, 0xac, 0x4c, 0x27, 0x50, 0x4a, 0xc6, 0x4a, 0xa5, 0x04, 0xc3, 0x3c,
		 0xde, 0x9f, 0x36, 0xdb, 0x72, 0x2d, 0xce, 0x94, 0xea, 0x2b, 0xfa, 0xcb, 0x20, 0x09, 0x39, 0x2c,
		 0x16, 0xe8, 0x61, 0x02, 0xe9, 0xaf, 0x4d, 0xd3, 0x02, 0x93, 0x9a, 0x31, 0x5b, 0x97, 0x92, 0x21,
		 0x7f, 0xf0, 0xcf, 0x18, 0xda, 0x91, 0x11, 0x02, 0x34, 0x86, 0xe8, 0x20, 0x58, 0x33, 0x0b, 0x80,
		 0x34, 0x89, 0xd8, 0xa3, 0x81, 0x89, 0x30, 0x81, 0x86, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e,
		 0x04, 0x16, 0x04, 0x14, 0x5d, 0x5d, 0x26, 0xef, 0xac, 0x7e, 0x36, 0xf9, 0x9b, 0x76, 0x15, 0x2b,
		 0x4a, 0x25, 0x02, 0x23, 0xef, 0xb2, 0x89, 0x30, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04,
		 0x18, 0x30, 0x16, 0x80, 0x14, 0x56, 0x8e, 0x9a, 0xc3, 0xf0, 0x42, 0xde, 0x18, 0xb9, 0x45, 0x55,
		 0x6e, 0xf9, 0x93, 0xcf, 0xea, 0xc3, 0xf3, 0xa5, 0x21, 0x30, 0x0c, 0x06, 0x03, 0x55, 0x1d, 0x13,
		 0x01, 0x01, 0xff, 0x04, 0x02, 0x30, 0x00, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01,
		 0xff, 0x04, 0x04, 0x03, 0x02, 0x03, 0xa8, 0x30, 0x13, 0x06, 0x03, 0x55, 0x1d, 0x25, 0x04, 0x0c,
		 0x30, 0x0a, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01, 0x30, 0x11, 0x06, 0x09,
		 0x60, 0x86, 0x48, 0x01, 0x86, 0xf8, 0x42, 0x01, 0x01, 0x04, 0x04, 0x03, 0x02, 0x06, 0x40, 0x30,
		 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x03, 0x48, 0x00, 0x30, 0x45,
		 0x02, 0x20, 0x61, 0x6f, 0xe8, 0xb9, 0xad, 0xcc, 0xc9, 0x1a, 0x81, 0x17, 0x02, 0x64, 0x07, 0xc3,
		 0x18, 0x44, 0x01, 0x81, 0x76, 0x18, 0x9d, 0x6d, 0x3d, 0x7d, 0xcb, 0xc1, 0x5a, 0x76, 0x4a, 0xad,
		 0x71, 0x55, 0x02, 0x21, 0x00, 0xcd, 0x22, 0x35, 0x04, 0x19, 0xc2, 0x23, 0x21, 0x02, 0x88, 0x4b,
		 0x51, 0xda, 0xdb, 0x51, 0xab, 0x54, 0x8c, 0xcb, 0x38, 0xac, 0x8e, 0xbb, 0xee, 0x18, 0x07, 0xbf,
		 0x88, 0x36, 0x88, 0xff, 0xd5,

};

/* derived from wolfSSL/certs/ecc-key.der
 *  od -tx1 -An ecc-key.der  | sed  -r 's/ ([0-9a-f]{2})/ 0x\1,/g'
 */

static const unsigned char ecc_key_der_256[] = {
		 0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0x45, 0xb6, 0x69, 0x02, 0x73, 0x9c, 0x6c, 0x85, 0xa1,
		 0x38, 0x5b, 0x72, 0xe8, 0xe8, 0xc7, 0xac, 0xc4, 0x03, 0x8d, 0x53, 0x35, 0x04, 0xfa, 0x6c, 0x28,
		 0xdc, 0x34, 0x8d, 0xe1, 0xa8, 0x09, 0x8c, 0xa0, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d,
		 0x03, 0x01, 0x07, 0xa1, 0x44, 0x03, 0x42, 0x00, 0x04, 0xbb, 0x33, 0xac, 0x4c, 0x27, 0x50, 0x4a,
		 0xc6, 0x4a, 0xa5, 0x04, 0xc3, 0x3c, 0xde, 0x9f, 0x36, 0xdb, 0x72, 0x2d, 0xce, 0x94, 0xea, 0x2b,
		 0xfa, 0xcb, 0x20, 0x09, 0x39, 0x2c, 0x16, 0xe8, 0x61, 0x02, 0xe9, 0xaf, 0x4d, 0xd3, 0x02, 0x93,
		 0x9a, 0x31, 0x5b, 0x97, 0x92, 0x21, 0x7f, 0xf0, 0xcf, 0x18, 0xda, 0x91, 0x11, 0x02, 0x34, 0x86,
		 0xe8, 0x20, 0x58, 0x33, 0x0b, 0x80, 0x34, 0x89, 0xd8,
};


void wolfssl_server_test(uintData_t statusPtr)
{
    int sock_listen;
    int bindStatus;
    int sock_req;
    sockaddr_in socketAddr;
    sockaddr_in server_addr;
    int  socketAddrLen=sizeof(sockaddr);
    char rx_buf[RX_BUF_SIZE];
    char tx_buf[TX_BUF_SIZE];
    clientConnectionHandleType TCPserverHandle;

    WOLFSSL * ssl;
    WOLFSSL_CTX * ctx;
    int tx_buf_sz = 0, ret = 0, error = 0;

    /* set up the mailbox transport */
    /* connectionId2 is defined in the mailbox-transport.config*/
    if (setupTransport(&TCPserverHandle, (char*)"connectionId2") != transportSuccess){
        printf("TCP transport set up failed \n");
        return;
      }

    /* SET UP NETWORK SOCKET */

    printf("Opening network socket...\n");
    sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_listen == SOCKET_ERROR) {
        printf("ERROR: socket, err = %d\n", errno);
        return;
    }

    printf("Clearing memory for server_addr struct\n");
    XMEMSET((char *) &server_addr, 0u, sizeof(server_addr));

    printf("Setting up server_addr struct\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(TLS_SERVER_PORT);

    bindStatus = bind(sock_listen, (sockaddr *) &server_addr, sizeof(server_addr));
    if (bindStatus == SOCKET_ERROR) {
       printf("ERROR: bind, err = %d\n", errno);
       closesocket(sock_listen);
       return;
    }


    /* chooses the highest possible TLS version */

    ctx = wolfSSL_CTX_new(wolfSSLv23_server_method());

    if (ctx == 0) {
        printf("ERROR: wolfSSL_CTX_new failed\n");
        closesocket(sock_listen);
        return;
    }
    WOLFSSL_MSG("wolfSSL_CTX_new done");

    ret = wolfSSL_CTX_use_certificate_buffer(ctx,
                                             server_ecc_der_256,
                                             sizeof(server_ecc_der_256),
                                             SSL_FILETYPE_ASN1);
    if (ret != SSL_SUCCESS) {
        printf("ERROR: wolfSSL_CTX_use_certificate_buffer() failed, \
                err = %d\n", ret);
        closesocket(sock_listen);
        wolfSSL_CTX_free(ctx);
        return;
    }
    ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx,
                                            ecc_key_der_256,
                                            sizeof(ecc_key_der_256),
                                            SSL_FILETYPE_ASN1);
    if (ret != SSL_SUCCESS) {
        printf("ERROR: wolfSSL_CTX_use_PrivateKey_buffer() failed\n");
        closesocket(sock_listen);
        wolfSSL_CTX_free(ctx);
        return;
    }
    /* accept client socket connections */
    printf("Listening for client connection\n");
    printf("E.g, you can use ./examples/client/client.exe -d -h <ipaddr>\n");
    printf("    \n");

    listen(sock_listen, TCP_SERVER_CONN_Q_SIZE);

    while (1) {
        sock_req = accept(sock_listen,
                          (sockaddr *) &socketAddr,
                          &socketAddrLen);

        if (sock_req == -1) {
            printf("ERROR: accept, err = %d\n", errno);
            continue;
        }

        printf("Got client connection! Starting TLS negotiation\n");

        /* set up wolfSSL session */
        ssl = wolfSSL_new(ctx);
        if (ssl == NULL) {
            printf("ERROR: wolfSSL_new() failed\n");
            goto server_exit;
        }

        WOLFSSL_MSG("wolfSSL_new done");
        ret = wolfSSL_set_fd(ssl, sock_req);
        if (ret != SSL_SUCCESS) {
            printf("ERROR: wolfSSL_set_fd() failed\n");
            goto server_exit;
        }

        WOLFSSL_MSG("wolfSSL_set_fd done");
        do {
            error = 0; /* reset error */
            if (ret != SSL_SUCCESS) {
                error = wolfSSL_get_error(ssl, 0);
                printf("ERROR: wolfSSL_accept() failed, err = %d\n", error);
                if (error != SSL_ERROR_WANT_READ) {
                    goto server_exit;
                }
                /* goToSleep() for 500 milli sec*/
            }
        } while ((ret != SSL_SUCCESS) && (error == SSL_ERROR_WANT_READ));

        printf("wolfSSL_accept() ok...\n");

        /* read client data */

        error = 0;
        XMEMSET(rx_buf, 0u, RX_BUF_SIZE);
        ret = wolfSSL_read(ssl, rx_buf, RX_BUF_SIZE - 1);
        if (ret < 0) {
            error = wolfSSL_get_error(ssl, 0);
            if (error != SSL_ERROR_WANT_READ) {
                printf("wolfSSL_read failed, error = %d\n", error);
                continue;
            }
        }

        printf("AFTER wolfSSL_read() call, ret = %d\n", ret);
        if (ret > 0) {
            rx_buf[ret] = 0;
            printf("Client sent: %s\n", rx_buf);
        }

        /* write response to client */
        XMEMSET(tx_buf, 0u, TX_BUF_SIZE);
        tx_buf_sz = 22;
        XSTRNCPY(tx_buf, "I hear ya fa shizzle!\n", tx_buf_sz);
        if (wolfSSL_write(ssl, tx_buf, tx_buf_sz) != tx_buf_sz) {
            error = wolfSSL_get_error(ssl, 0);
            printf("ERROR:  wolfSSL_write() failed, err = %d\n", error);
            continue;
        }
    }

server_exit:
    ret = wolfSSL_shutdown(ssl);
    if (ret == SSL_SHUTDOWN_NOT_DONE)
        wolfSSL_shutdown(ssl);

    wolfSSL_free(ssl);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    closesocket(sock_req);
    closesocket(sock_listen);
    return;
}

#endif /* NO_WOLFSSL_SERVER */

int main (void)
{
    thread_handle_t TLSclient;
    thread_handle_t TLSserver;
    threadStatus ts;

    initPrintx("");
    printx("wolfSSL TLS tests\n");

    // taken from hello-world-timer.cpp
    struct tm starttime = { 0, 30, 12, 1, 12, 2021-1900, 0, 0, 0 };
    // startdate: Dec 1 2021, 12:30:00
    struct timespec ts_date;
    ts_date.tv_sec  = mktime(&starttime);
    ts_date.tv_nsec = 0LL;
    int res1 = clock_settime(CLOCK_REALTIME, &ts_date);
    // this will only take effect, if time-control is set in the xml-file
    // if not, Jan 1 1970, 00:00:00 will be the date

    #if 1//def DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
    #endif

    /* wolfSSL INIT */
    wolfSSL_Init();

#if !defined(NO_WOLFSSL_SERVER)
    ts = createThread("TLSserver", "TLSThreadTemplate", wolfssl_server_test,
                      0, &TLSserver );
    if (ts != threadSuccess) {
      printf("Unable to create TLS server thread, %i ", ts);
    }
#endif

#if !defined(NO_WOLFSSL_CLIENT)
    ts = createThread("TLSclient", "TLSThreadTemplate", wolfssl_client_test,
                      0, &TLSclient );
    if (ts != threadSuccess) {
      printf("Unable to create TLS client thread, %i ", ts);
    }
#endif

    while (1) {
      waitUntilNextPeriod();
    }

    return 0;
}
