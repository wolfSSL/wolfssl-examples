 /****************************************************************************
 File Name        : wolfSSLServerTask.c
 Author           : wolfSSL Inc.
 Date Created     : March 25, 2016
 Current Revision : 1.0
 Notes            : This file contains a simple TLS server task with the goal
                    of demonstrating how the wolfSSL embedded SSL/TLS library
                    can be used with the uTasker stack. This server creates
                    a TCP socket, sets the socket to listening mode, and
                    when a connection is received negotiates an SSL/TLS
                    session with the peer. It expects the peer to send a
                    simple message (ex: 'hello wolfssl!'), sends a
                    simple message back in return ('I hear you fa shizzle!'),
                    closes the session and socket, then loops back around to
                    listen for another connection.

Copyright (C) wolfSSL, Inc. 2020
 *****************************************************************************/

#include "config.h"
#include "wolfSSLServerTask.h"
#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/wolfcrypt/aes.h"
#include "wolfssl/wolfcrypt/rsa.h"
#include "wolfssl/ssl.h"

#define TEST_BUFFER_LENGTH  100
#define MAX_TCP_LENGTH      1460
#define RECV_BUFFER_LENGTH  1500

/* ---------------------------- STRUCTS / ENUMS ---------------------------- */

typedef enum
{
    serverInit        = 0,
    serverSocketSetup = 1,
    serverIdle        = 2,
    serverListening   = 3,
    serverTLSInit     = 4,
    serverTLSNew      = 5,
    serverTLSAccept   = 6,
    serverTLSSend     = 7,
    serverTLSRecv     = 8,
    serverShutdown    = 9,
} serverStates;


/* func_args from wolfCrypt test.h, so don't have to pull in other stuff */
typedef struct func_args {
    int    argc;
    char** argv;
    int    return_code;
} func_args;


typedef struct stTCP_MESSAGE
{
    TCP_HEADER    tTCP_Header;
    unsigned char ucTCP_Message[MAX_TCP_LENGTH];
} TCP_MESSAGE;


/* wolfSSL send callback context struct */
typedef struct stUTASKER_SENDCTX {
    TCP_MESSAGE*  message;      /* TCP frame to send */
    USOCKET*      socket;       /* socket pointer */
    unsigned int  dataLen;      /* length of data in message frame */
    unsigned char ackd;         /* has ACK been received for data (0:1) */
    unsigned char flags;        /* socket flags, ie: TCP_FLAG_PUSH */
} UTASKER_SENDCTX;


/* wolfSSL recv callback context struct */
typedef struct stUTASKER_RECVCTX {
    USOCKET*       socket;      /* socket pointer */
    unsigned int   used;        /* bytes used in buffer */
    unsigned int   offset;      /* current offset in buffer, for processing */
    unsigned int   bufLen;      /* total size of buffer in bytes */
    unsigned char* buffer;      /* recv data buffer */
} UTASKER_RECVCTX;

/* ------------------------------- VARIABLES ------------------------------- */

static USOCKET server_socket = 0;           /* server socket */
static TCP_MESSAGE stMessage;               /* structure to hold TCP frame   */
static UTASKER_SENDCTX sendCtx;             /* wolfSSL send callback context */
static UTASKER_RECVCTX recvCtx;             /* wolfSSL recv callback context */
static unsigned char ucRecvBuffer[RECV_BUFFER_LENGTH];    /* TCP recv buffer */
static serverStates serverState = serverInit;             /* TLS task state  */

static WOLFSSL_CTX* sslCtx;                 /* wolfSSL context context */
static WOLFSSL*     ssl;                    /* wolfSSL session object */

/* server port number */
static unsigned int uiServerPort = 443;
char input[80];
int wantReadTimeout = 2; /* time out after 5 consecutive WANT_READ errors */
int wantReadCounter = 0;

/* ------------------------------- PROTOTYPES ------------------------------ */

int CacheRecvBuffer(UTASKER_RECVCTX* ctx, unsigned char* data,
                    unsigned short length);
int ResetRecvBuffer(UTASKER_RECVCTX* ctx);
int UTasker_Receive(WOLFSSL* ssl, char* buf, int sz, void* ctx);
int UTasker_Send(WOLFSSL* ssl, char* buf, int sz, void* ctx);

/* ---------------------------- SOCKET LISTENERS --------------------------- */

static int fnServerListener(USOCKET Socket, unsigned char ucEvent,
                          unsigned char *ucIp_Data, unsigned short usPortLen)
{
    int ret = 0;

    switch (ucEvent)
    {
        case TCP_EVENT_CONREQ:
        case TCP_EVENT_CONNECTED:
            fnDebugMsg("status: TCP_EVENT_CONNECTED\r\n");
            serverState = serverTLSNew;
            break;

        case TCP_EVENT_ACK:
            fnDebugMsg("status: TCP_EVENT_ACK\r\n");
            /* ACK received, set ackd variable in CTX msg */
            sendCtx.ackd = 1;
            break;

        case TCP_EVENT_ARP_RESOLUTION_FAILED:
            fnDebugMsg("status: TCP_EVENT_ARP_RESOLUTION_FAILED\r\n");
            break;

        case TCP_EVENT_PARTIAL_ACK:
            fnDebugMsg("status: TCP_EVENT_PARTIAL_ACK\r\n");
            break;

        case TCP_EVENT_REGENERATE:
            /* frame lost, need to resend last frame, use cached */
            fnDebugMsg("status: TCP_EVENT_REGENERATE\r\n");
            if (sendCtx.ackd == 0) {
                ret = fnSendTCP(*sendCtx.socket,
                                (unsigned char*)&sendCtx.message->tTCP_Header,
                                sendCtx.dataLen, sendCtx.flags);
                if (ret > 0) {
                    return APP_SENT_DATA;
                }
            }
            break;

        case TCP_EVENT_DATA:
            /* data received from client */
            fnDebugMsg("status: TCP_EVENT_DATA\r\n");

            /* copy data into our temp context buffer */
            if (CacheRecvBuffer(&recvCtx, ucIp_Data, usPortLen) < 0) {
                return APP_REJECT_DATA;
            }

            break;

        case TCP_EVENT_ABORT:
        case TCP_EVENT_CLOSE:
        case TCP_EVENT_CLOSED:
            /* server closed connection */
            fnDebugMsg("status: TCP_EVENT_CLOSE || TCP_EVENT_CLOSED\r\n");
            serverState = serverShutdown;
            break;
    }
    return APP_ACCEPT;
}

/* ------------------------------- APP TASK -------------------------------- */


/*
 * wolfSSL server app task
 */
extern void fnTLSServerTask(TTASKTABLE *ptrTaskTable)
{
    int ret, errorCode, msgSz = 0;
    char msg[64];

    /* init socket and app state */
    if (serverState == serverInit)
    {
        fnDebugMsg("Starting wolfSSL Server Task\r\n");

        /* run wolfCrypt tests */
        ret = WolfCryptTest();
        if (ret == 0) {
            fnDebugMsg("status: wolfCrypt Tests Passed!\r\n\r\n");
            serverState = serverTLSInit;
        }
        else {
            fnDebugMsg("ERROR: wolfCrypt Tests Failed!\r\n\r\n");
            serverState = serverIdle;
        }
    }

    if (serverState == serverTLSInit)
    {
        /* for debug, compile wolfSSL with DEBUG_WOLFSSL defined */
        /* wolfSSL_Debugging_ON(); */

        wolfSSL_Init();

        /* create wolfSSL context */
        sslCtx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
        if (sslCtx == NULL) {
            fnDebugMsg("ERROR: wolfSSL_CTX_new() failed\r\n");
            serverState = serverShutdown;
        }
        else {
            fnDebugMsg("status: Created WOLFSSL_CTX\r\n");
        }

        /* load server certificate */
        ret = wolfSSL_CTX_use_certificate_buffer(sslCtx, server_cert_der_2048,
            sizeof(server_cert_der_2048),
            SSL_FILETYPE_ASN1);
        if (ret != SSL_SUCCESS) {
            fnDebugMsg("ERROR: wolfSSL_CTX_use_certificate_chain_buffer\r\n");
            serverState = serverShutdown;
        }

        /* load server private key */
        ret = wolfSSL_CTX_use_PrivateKey_buffer(sslCtx, server_key_der_2048,
            sizeof(server_key_der_2048),
            SSL_FILETYPE_ASN1);
        if (ret != SSL_SUCCESS) {
            fnDebugMsg("ERROR: wolfSSL_CTX_use_PrivateKey_buffer\r\n");
            serverState = serverShutdown;
        }

        /* register wolfSSL send/recv callbacks */
        wolfSSL_SetIOSend(sslCtx, UTasker_Send);
        wolfSSL_SetIORecv(sslCtx, UTasker_Receive);

        serverState = serverSocketSetup;
    }

    /* crypto tests */
    if (serverState == serverSocketSetup)
    {
        /* create socket */
        server_socket = fnGetTCP_Socket(TOS_MINIMISE_DELAY,
            TCP_DEFAULT_TIMEOUT, fnServerListener);

        if (server_socket >= 0) {

            /* set socket listening */
            ret = fnTCP_Listen(server_socket, uiServerPort, 0);
            if (ret != server_socket) {
                fnDebugMsg("ERROR: fnTCP_Listen() failed\r\n");
                serverState = serverIdle;
            }
            else {
                fnDebugMsg("status: Socket listening for connection ...\r\n");
                serverState = serverListening;
            }
        }
    }

    if (serverState == serverTLSNew)
    {
        /* create wolfSSL session */
        ssl = wolfSSL_new(sslCtx);
        if (ssl == NULL) {
            fnDebugMsg("ERROR: wolfSSL_new\r\n");
            serverState = serverShutdown;
        }
        else {
            fnDebugMsg("status: Created WOLFSSL session object\r\n");
        }

        /* set up wolfSSL send/recv context details */
        sendCtx.socket = &server_socket;
        sendCtx.message = &stMessage;
        sendCtx.ackd = 0;
        sendCtx.dataLen = 0;
        sendCtx.flags = 0;
        recvCtx.socket = &server_socket;
        recvCtx.buffer = ucRecvBuffer;
        recvCtx.bufLen = sizeof(ucRecvBuffer);

        /* register wolfSSL read/write callback contexts */
        wolfSSL_SetIOReadCtx(ssl, &recvCtx);
        wolfSSL_SetIOWriteCtx(ssl, &sendCtx);

        serverState = serverTLSAccept;
    }

    /* perform SSL/TLS handshake with peer */
    if (serverState == serverTLSAccept)
    {
        ret = wolfSSL_accept(ssl);
        errorCode = wolfSSL_get_error(ssl, ret);
        if (ret != SSL_SUCCESS && (errorCode != SSL_ERROR_WANT_READ &&
                                   errorCode != SSL_ERROR_WANT_WRITE)) {
            fnDebugMsg("ERROR: wolfSSL_accept: ");
            errorCode = wolfSSL_get_error(ssl, ret);
            fnDebugDec(errorCode, DISPLAY_NEGATIVE);
            fnDebugMsg("\r\n");
            serverState = serverShutdown;
        }
        else if (ret == SSL_SUCCESS) {
            fnDebugMsg("wolfSSL_accept() finished\r\n");
            fnDebugMsg("Client message: \r\n");
            serverState = serverTLSRecv;
        }
    }

    /* read client message over SSL/TLS */
    if (serverState == serverTLSRecv)
    {
        ret = wolfSSL_read(ssl, input, sizeof(input)-1);
        if (ret > 0) {
            input[ret] = 0;
            fnDebugMsg(input);
            fnDebugMsg("\r\n");
            wantReadCounter = 0;
        }
        else if (ret < 0) {
            errorCode = wolfSSL_get_error(ssl, ret);
            if (errorCode == SSL_ERROR_WANT_READ && recvCtx.used == 0)
            {
                /* for simplicity, just time out after 5 consecutive
                   empty read calls (adjusted using wantReadTimeout variable */
                if (wantReadCounter == wantReadTimeout)
                    serverState = serverTLSSend;
                else
                    wantReadCounter++;
            }
            else if (errorCode != SSL_ERROR_WANT_READ)
            {
                fnDebugMsg("ERROR: wolfSSL_read() failed: ");
                errorCode = wolfSSL_get_error(ssl, ret);
                fnDebugDec(errorCode, DISPLAY_NEGATIVE);
                fnDebugMsg("\r\n");
                serverState = serverShutdown;
            }
        }
        else {
            serverState = serverTLSSend;
        }
    }

    /* send server response */
    if (serverState == serverTLSSend)
    {
        msgSz = 22;
        strncpy(msg, "I hear you fa shizzle!", msgSz);
        ret = wolfSSL_write(ssl, msg, msgSz);
        errorCode = wolfSSL_get_error(ssl, ret);
        if (errorCode != SSL_ERROR_WANT_WRITE) {
            if (ret != msgSz) {
                fnDebugMsg("ERROR: wolfSSL_write() failed: ");
                errorCode = wolfSSL_get_error(ssl, ret);
                fnDebugDec(errorCode, DISPLAY_NEGATIVE);
                fnDebugMsg("\r\n");
            }
            serverState = serverShutdown;
        }
    }

    /* free resources and shutdown */
    if (serverState == serverShutdown)
    {
        /* release socket, SSL session */
        fnReleaseTCP_Socket(server_socket);
        wolfSSL_free(ssl);

        /* reset wolfSSL receive context/buffer */
        ResetRecvBuffer(&recvCtx);

        /* reset variables and buffers */
        uMemset(input, 0, sizeof(input));
        wantReadCounter = 0;

        fnDebugMsg("status: Released Resources\r\n");

        serverState = serverSocketSetup;
    }
}

