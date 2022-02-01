 /****************************************************************************
 File Name        : wolfSSLClientTask.c
 Author           : wolfSSL Inc.
 Date Created     : March 25, 2016
 Current Revision : 1.0
 Notes            : This file contains a simple TLS client task with the goal
                    of demonstrating how the wolfSSL embedded SSL/TLS library
                    can be used with the uTasker stack. This client connects
                    to a server over TLS (using raw TCP sockets), sends an
                    HTTP GET message, reads the response, and closes
                    the socket.

Copyright (C) wolfSSL, Inc. 2020
 *****************************************************************************/

#include "config.h"
#include "wolfSSLClientTask.h"
#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/wolfcrypt/aes.h"
#include "wolfssl/wolfcrypt/rsa.h"
#include "wolfssl/ssl.h"

#define TEST_TCP_PORT       0x80
#define TEST_BUFFER_LENGTH  100
#define MAX_TCP_LENGTH      1460
#define RECV_BUFFER_LENGTH  1500

/* ---------------------------- STRUCTS / ENUMS ---------------------------- */

typedef enum
{
    clientInit       = 0,
    clientCryptoTest = 1,
    clientIdle       = 2,
    clientConnecting = 3,
    clientTLSInit    = 4,
    clientTLSConnect = 5,
    clientTLSSend    = 6,
    clientTLSRecv    = 7,
    clientShutdown   = 8,
} clientStates;


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

static USOCKET client_socket = 0;           /* client socket */
static TCP_MESSAGE stMessage;               /* structure to hold TCP frame   */
static UTASKER_SENDCTX sendCtx;             /* wolfSSL send callback context */
static UTASKER_RECVCTX recvCtx;             /* wolfSSL recv callback context */
static unsigned char ucRecvBuffer[RECV_BUFFER_LENGTH];    /* TCP recv buffer */
static clientStates clientState = clientCryptoTest;       /* TLS task state  */

static WOLFSSL_CTX* sslCtx;                 /* wolfSSL context context */
static WOLFSSL*     ssl;                    /* wolfSSL session object */

/* server IP address and port */
static unsigned char ucRemoteIP[IPV4_LENGTH] = { 93,184,216,34 };
static unsigned int  uiRemotePort = 443;

/* ------------------------------- PROTOTYPES ------------------------------ */

int CacheRecvBuffer(UTASKER_RECVCTX* ctx, unsigned char* data,
                    unsigned short length);
int ResetRecvBuffer(UTASKER_RECVCTX* ctx);

/* ---------------------------- SOCKET LISTENERS --------------------------- */

static int fnClientListener(USOCKET Socket, unsigned char ucEvent,
                          unsigned char *ucIp_Data, unsigned short usPortLen)
{
    int ret = 0;

    switch (ucEvent)
    {
        case TCP_EVENT_CONREQ:
        case TCP_EVENT_CONNECTED:
            fnDebugMsg("status: TCP_EVENT_CONNECTED\r\n");
            clientState = clientTLSInit;
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
            /* data received from server */
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
            break;
    }
    return APP_ACCEPT;
}

/* ---------------------------- HELPER FUNCTIONS --------------------------- */

/*
 * Copies received data into context buffer to be processed by app task.
 * Returns copied length on success, negative value on error.
 */
int CacheRecvBuffer(UTASKER_RECVCTX* ctx, unsigned char* data,
                    unsigned short length)
{
    if (ctx == NULL || ctx->buffer == NULL || data == NULL)
        return -1;

    /* error if already in use, or too small */
    if (ctx->used > 0 || ctx->bufLen < length)
        return -1;

    uMemcpy(ctx->buffer, data, length);
    ctx->used = length;

    return length;
}

/* Resets the context buffer and sizes
 * Returns 0 on success, -1 on error */
int ResetRecvBuffer(UTASKER_RECVCTX* ctx)
{
    if (ctx == NULL)
        return -1;

    uMemset(ctx->buffer, 0, ctx->bufLen);
    ctx->offset = 0;
    ctx->used = 0;

    return 0;
}

/* -------------------- wolfSSL SEND/RECV/VERIFY CALLBACKS ----------------- */


/*
 * wolfSSL receive callback
 *
 * This function is called by wolfSSL whenever it needs to read data.
 * Returns number of bytes received, or negative error.
 * WOLFSSL_CBIO_ERR_WANT_READ should be returned if more data is
 * needed to be read and wolfSSL should call this function again.
 */
int UTasker_Receive(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    unsigned int copied = 0;
    UTASKER_RECVCTX* stCtx = (UTASKER_RECVCTX*)ctx;

    if (stCtx == NULL || stCtx->buffer == NULL) {
        fnDebugMsg("UTasker_Receive invalid parameters\r\n");
        return WOLFSSL_CBIO_ERR_GENERAL;
    }

    /* check for socket closed, if no data left return error */
    if ((fnGetTCP_state(*stCtx->socket) & TCP_STATE_CLOSED) &&
        (stCtx->used == 0)) {
        fnDebugMsg("uTasker_Receive socket closed\r\n");
        return WOLFSSL_CBIO_ERR_GENERAL;
    }

    if (stCtx->used == 0) {
        return WOLFSSL_CBIO_ERR_WANT_READ;
    }

    /* copy either desired sz or number of bytes free in buffer */
    copied = min((unsigned int)sz, stCtx->used - stCtx->offset);

    uMemcpy(buf, stCtx->buffer + stCtx->offset, copied);
    stCtx->offset += copied;

    if (stCtx->offset == stCtx->used) {
        /* packet has been drained, reset */
        stCtx->offset = 0;
        stCtx->used = 0;
        uMemset(stCtx->buffer, 0, stCtx->bufLen);
    }

    return copied;
}

/*
 * wolfSSL send callback
 *
 * This function is called by wolfSSL whenever it needs to send data.
 * Returns number of bytes sent, or negative error.
 * WOLFSSL_CBIO_ERR_WANT_WRITE should be returned if more data is
 * needed to be sent and wolfSSL should call this function again.
 */
int UTasker_Send(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    int ret, send;
    UTASKER_SENDCTX* stCtx = (UTASKER_SENDCTX*)ctx;

    if (stCtx == NULL || stCtx->socket == NULL || stCtx->message == NULL) {
        fnDebugMsg("UTasker_Send, invalid parameters\r\n");
        return WOLFSSL_CBIO_ERR_GENERAL;
    }

    /* return error if socket closed */
    if (fnGetTCP_state(*stCtx->socket) & TCP_STATE_CLOSED) {
        fnDebugMsg("uTasker_Receive socket closed\r\n");
        return WOLFSSL_CBIO_ERR_GENERAL;
    }

    /* return WANT_WRITE if ACK has not been received for last frame */
    if (stCtx->dataLen != 0 && stCtx->ackd == 0) {
        return WOLFSSL_CBIO_ERR_WANT_WRITE;
    }

    send = min(sz, MAX_TCP_LENGTH);
    uMemcpy(stCtx->message->ucTCP_Message, buf, send);
    stCtx->dataLen = send;

    ret = fnSendTCP(*stCtx->socket,
                    (unsigned char*)&stCtx->message->tTCP_Header,
                    (unsigned short)send, stCtx->flags);

    if (ret == NO_ARP_ENTRY) {
        /* dest address must be resolved, try again */
        return WOLFSSL_CBIO_ERR_WANT_WRITE;
    }
    else if (ret <= 0) {
        /* no data sent or socket error */
        return WOLFSSL_CBIO_ERR_GENERAL;
    }

    stCtx->ackd = 0;

    return (int)send;
}

/*
 * wolfSSL verification callback
 *
 * This function is called when peer certificate verification
 * fails. The exact error code can be retrieved through
 * store->error. Returning "1" from this function will
 * allow the SSL/TLS handshake to continue as if verification
 * succeeded. Returning "1" here is not recommended.
 */
int myVerify(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    (void)preverify;
    char buffer[80];

    fnDebugMsg("In verification callback, error = \r\n");
    fnDebugDec(store->error, 0);
    fnDebugMsg(", ");
    fnDebugMsg(wolfSSL_ERR_error_string(store->error, buffer));
    fnDebugMsg("\r\n");

    return 0;
}

/* ------------------------------- APP TASK -------------------------------- */


/*
 * wolfSSL client app task
 */
extern void fnTLSClientTask(TTASKTABLE *ptrTaskTable)
{
    int ret, errorCode, msgSz = 0;
    char msg[64];
    char reply[1024];

    /* run crypto tests */
    if (clientState == clientCryptoTest)
    {
        fnDebugMsg("Starting wolfSSL Client Task\r\n");

        ret = WolfCryptTest();
        if (ret == 0) {
            fnDebugMsg("status: wolfCrypt Tests Passed!\r\n\r\n");
            clientState = clientInit;
        }
        else {
            fnDebugMsg("ERROR: wolfCrypt Tests Failed!\r\n\r\n");
            clientState = clientIdle;
        }
    }

    /* init socket and app state */
    if (clientState == clientInit)
    {
        /* create socket */
        client_socket = fnGetTCP_Socket(TOS_MINIMISE_DELAY,
                                        TCP_DEFAULT_TIMEOUT,
                                        fnClientListener);
        if (client_socket >= 0) {
            fnDebugMsg("status: Created socket\r\n");
            clientState = clientConnecting;
        }
    }

    /* connect socket */
    if (clientState == clientConnecting)
    {
        ret = fnTCP_Connect(client_socket, ucRemoteIP,
                            uiRemotePort, 0, 0);

        if (ret != client_socket) {
            fnDebugMsg("ERROR: fnTCP_Connect() failed\r\n");
            clientState = clientIdle;
        }
        else {
            fnDebugMsg("status: Socket connected\r\n");
            clientState = clientTLSInit;
        }
    }

    /* set up SSL/TLS context */
    if (clientState == clientTLSInit)
    {
        /* for debug, compile wolfSSL with DEBUG_WOLFSSL defined */
        /* wolfSSL_Debugging_ON(); */

        wolfSSL_Init();

        /* create wolfSSL context */
        sslCtx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
        if (sslCtx == NULL) {
            fnDebugMsg("ERROR: wolfSSL_CTX_new() failed\r\n");
            clientState = clientShutdown;
        }
        else {
            fnDebugMsg("status: Created WOLFSSL_CTX\r\n");
        }

        /* turn on peer verification, register callback */
        wolfSSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER, myVerify);
        fnDebugMsg("status: Enabled peer verification\r\n");

        /* load root CA certificate to verify peer */
        ret = wolfSSL_CTX_load_verify_buffer(sslCtx, digicert_ca_2048,
                                             sizeof(digicert_ca_2048),
                                             SSL_FILETYPE_ASN1);
        if (ret != SSL_SUCCESS) {
            fnDebugMsg("ERROR: wolfSSL_CTX_load_verify_buffer\r\n");
            clientState = clientShutdown;
        }
        else {
            fnDebugMsg("status: Loaded trusted CA certificates\r\n");
        }

        /* set up wolfSSL send/recv context details */
        sendCtx.socket = &client_socket;
        sendCtx.message = &stMessage;
        sendCtx.ackd = 0;
        sendCtx.dataLen = 0;
        sendCtx.flags = 0;
        recvCtx.socket = &client_socket;
        recvCtx.buffer = ucRecvBuffer;
        recvCtx.bufLen = sizeof(ucRecvBuffer);

        /* register wolfSSL send/recv callbacks */
        wolfSSL_SetIOSend(sslCtx, UTasker_Send);
        wolfSSL_SetIORecv(sslCtx, UTasker_Receive);

        /* create wolfSSL session */
        ssl = wolfSSL_new(sslCtx);
        if (ssl == NULL) {
            fnDebugMsg("ERROR: wolfSSL_new\r\n");
            clientState = clientShutdown;
        }
        else {
            fnDebugMsg("status: Created WOLFSSL session object\r\n");
        }

        /* register wolfSSL read/write callback contexts */
        wolfSSL_SetIOReadCtx(ssl, &recvCtx);
        wolfSSL_SetIOWriteCtx(ssl, &sendCtx);

        clientState = clientTLSConnect;
    }

    /* perform SSL/TLS handshake with peer */
    if (clientState == clientTLSConnect)
    {
        ret = wolfSSL_connect(ssl);
        errorCode = wolfSSL_get_error(ssl, ret);
        if (ret != SSL_SUCCESS && (errorCode != SSL_ERROR_WANT_READ &&
                                   errorCode != SSL_ERROR_WANT_WRITE)) {
            fnDebugMsg("ERROR: wolfSSL_connect: ");
            errorCode = wolfSSL_get_error(ssl, ret);
            fnDebugDec(errorCode, DISPLAY_NEGATIVE);
            fnDebugMsg("\r\n");
            clientState = clientShutdown;
        }
        else if (ret == SSL_SUCCESS) {
            fnDebugMsg("wolfSSL_connect() ok, sending GET...\r\n");
            clientState = clientTLSSend;
        }
    }

    /* send HTTP GET over SSL/TLS */
    if (clientState == clientTLSSend)
    {
        msgSz = 28;
        strncpy(msg, "GET /index.html HTTP/1.0\r\n\r\n", msgSz);
        ret = wolfSSL_write(ssl, msg, msgSz);
        errorCode = wolfSSL_get_error(ssl, ret);
        if (errorCode != SSL_ERROR_WANT_WRITE) {
            if (ret != msgSz) {
                fnDebugMsg("ERROR: wolfSSL_write() failed: ");
                errorCode = wolfSSL_get_error(ssl, ret);
                fnDebugDec(errorCode, DISPLAY_NEGATIVE);
                fnDebugMsg("\r\n");
                clientState = clientShutdown;
            }
            else {
                clientState = clientTLSRecv;
            }
        }
    }

    /* read server response */
    if (clientState == clientTLSRecv)
    {
        ret = wolfSSL_read(ssl, reply, sizeof(reply) - 1);
        errorCode = wolfSSL_get_error(ssl, ret);
        if (errorCode != SSL_ERROR_WANT_READ) {
            if (ret > 0) {
                fnDebugMsg("Server response: ");
                reply[ret] = 0;
                fnDebugMsg(reply);
                fnDebugMsg("\r\n");
            }
            else if (ret < 0) {
                fnDebugMsg("ERROR: wolfSSL_read() failed: ");
                errorCode = wolfSSL_get_error(ssl, ret);
                fnDebugDec(errorCode, DISPLAY_NEGATIVE);
                fnDebugMsg("\r\n");
            }
            clientState = clientShutdown;
        }
    }

    /* free resources and shutdown */
    if (clientState == clientShutdown)
    {
        fnReleaseTCP_Socket(client_socket);
        wolfSSL_free(ssl);
        wolfSSL_CTX_free(sslCtx);
        wolfSSL_Cleanup();

        fnDebugMsg("status: Released Resources\r\n");

        clientState = clientIdle;
    }
}

/* ------------------------- wolfCrypt CRYPTO TESTS ------------------------ */


int WolfCryptTest(void)
{
    int ret;

#if !defined(NO_BIG_INT)
    if (CheckCtcSettings() != 1) {
        fnDebugMsg("ERROR: Build vs runtime math mismatch\r\n");
    }

#ifdef USE_FAST_MATH
    if (CheckFastMathSettings() != 1) {
        fnDebugMsg("ERROR: Build vs runtime fastmath FP_MAX_BITS \
                   mismatch\r\n");
        clientState = clientIdle;
    }
#endif /* USE_FAST_MATH */
#endif /* NO_BIG_INT */

#ifndef NO_MD5
    if ((ret = md5_test()) != 0)
        fnDebugMsg("MD5      test failed!\r\n");
    else
        fnDebugMsg("MD5      test passed!\r\n");
#endif

#ifdef WOLFSSL_MD2
    if ((ret = md2_test()) != 0)
        fnDebugMsg("MD2      test failed!\r\n");
    else
        fnDebugMsg("MD2      test passed!\r\n");
#endif

#ifndef NO_MD4
    if ((ret = md4_test()) != 0)
        fnDebugMsg("MD4      test failed!\r\n");
    else
        fnDebugMsg("MD4      test passed!\r\n");
#endif

#ifndef NO_SHA
    if ((ret = sha_test()) != 0)
        fnDebugMsg("SHA      test failed!\r\n");
    else
        fnDebugMsg("SHA      test passed!\r\n");
#endif

#ifndef NO_SHA256
    if ((ret = sha256_test()) != 0)
        fnDebugMsg("SHA-256  test failed!\r\n");
    else
        fnDebugMsg("SHA-256  test passed!\r\n");
#endif

#ifdef WOLFSSL_SHA384
    if ((ret = sha384_test()) != 0)
        fnDebugMsg("SHA-384  test failed!\r\n");
    else
        fnDebugMsg("SHA-384  test passed!\r\n");
#endif

#ifdef WOLFSSL_SHA512
    if ((ret = sha512_test()) != 0)
        fnDebugMsg("SHA-512  test failed!\r\n");
    else
        fnDebugMsg("SHA-512  test passed!\r\n");
#endif

#ifdef WOLFSSL_RIPEMD
    if ((ret = ripemd_test()) != 0)
        fnDebugMsg("RIPEMD   test failed!\r\n");
    else
        fnDebugMsg("RIPEMD   test passed!\r\n");
#endif

#ifdef HAVE_BLAKE2
    if ((ret = blake2b_test()) != 0)
        fnDebugMsg("BLAKE2b  test failed!\r\n");
    else
        fnDebugMsg("BLAKE2b  test passed!\r\n");
#endif

#ifndef NO_HMAC
#ifndef NO_MD5
    if ((ret = hmac_md5_test()) != 0)
        fnDebugMsg("HMAC-MD5 test failed!\r\n");
    else
        fnDebugMsg("HMAC-MD5 test passed!\r\n");
#endif

#ifndef NO_SHA
    if ((ret = hmac_sha_test()) != 0)
        fnDebugMsg("HMAC-SHA test failed!\r\n");
    else
        fnDebugMsg("HMAC-SHA test passed!\r\n");
#endif

#ifndef NO_SHA256
    if ((ret = hmac_sha256_test()) != 0)
        fnDebugMsg("HMAC-SHA256 test failed!\r\n");
    else
        fnDebugMsg("HMAC-SHA256 test passed!\r\n");
#endif

#ifdef WOLFSSL_SHA384
    if ((ret = hmac_sha384_test()) != 0)
        fnDebugMsg("HMAC-SHA384 test failed!\r\n");
    else
        fnDebugMsg("HMAC-SHA384 test passed!\r\n");
#endif

#ifdef WOLFSSL_SHA512
    if ((ret = hmac_sha512_test()) != 0)
        fnDebugMsg("HMAC-SHA512 test failed!\r\n");
    else
        fnDebugMsg("HMAC-SHA512 test passed!\r\n");
#endif

#ifdef HAVE_BLAKE2
    if ((ret = hmac_blake2b_test()) != 0)
        fnDebugMsg("HMAC-BLAKE2 test failed!\r\n");
    else
        fnDebugMsg("HMAC-BLAKE2 test passed!\r\n");
#endif

#ifdef HAVE_HKDF
    if ((ret = hkdf_test()) != 0)
        fnDebugMsg("HMAC-KDF    test failed!\r\n");
    else
        fnDebugMsg("HMAC-KDF    test passed!\r\n");
#endif

#endif

#ifdef HAVE_AESGCM
    if ((ret = gmac_test()) != 0)
        fnDebugMsg("GMAC     test failed!\r\n");
    else
        fnDebugMsg("GMAC     test passed!\r\n");
#endif

#ifndef NO_RC4
    if ((ret = arc4_test()) != 0)
        fnDebugMsg("ARC4     test failed!\r\n");
    else
        fnDebugMsg("ARC4     test passed!\r\n");
#endif

#ifdef HAVE_CHACHA
    if ((ret = chacha_test()) != 0)
        fnDebugMsg("Chacha   test failed!\r\n");
    else
        fnDebugMsg("Chacha   test passed!\r\n");
#endif

#ifdef HAVE_POLY1305
    if ((ret = poly1305_test()) != 0)
        fnDebugMsg("POLY1305 test failed!\r\n");
    else
        fnDebugMsg("POLY1305 test passed!\r\n");
#endif

#if defined(HAVE_CHACHA) && defined(HAVE_POLY1305)
    if ((ret = chacha20_poly1305_aead_test()) != 0)
        fnDebugMsg("ChaCha20-Poly1305 AEAD test failed!\r\n");
    else
        fnDebugMsg("ChaCha20-Poly1305 AEAD test passed!\r\n");
#endif

#ifndef NO_DES3
    if ((ret = des_test()) != 0)
        fnDebugMsg("DES      test failed!\r\n");
    else
        fnDebugMsg("DES      test passed!\r\n");
#endif

#ifndef NO_DES3
    if ((ret = des3_test()) != 0)
        fnDebugMsg("DES3     test failed!\r\n");
    else
        fnDebugMsg("DES3     test passed!\r\n");
#endif

#ifndef NO_AES
    if ((ret = aes_test()) != 0)
        fnDebugMsg("AES      test failed!\r\n");
    else
        fnDebugMsg("AES      test passed!\r\n");

#ifdef HAVE_AESGCM
    if ((ret = aesgcm_test()) != 0)
        fnDebugMsg("AES-GCM  test failed!\r\n");
    else
        fnDebugMsg("AES-GCM  test passed!\r\n");
#endif

#ifdef HAVE_AESCCM
    if ((ret = aesccm_test()) != 0)
        fnDebugMsg("AES-CCM  test failed!\r\n");
    else
        fnDebugMsg("AES-CCM  test passed!\r\n");
#endif
#endif

#ifdef HAVE_CAMELLIA
    if ((ret = camellia_test()) != 0)
        fnDebugMsg("CAMELLIA test failed!\r\n");
    else
        fnDebugMsg("CAMELLIA test passed!\r\n");
#endif

    if ((ret = random_test()) != 0)
        fnDebugMsg("RANDOM   test failed!\r\n");
    else
        fnDebugMsg("RANDOM   test passed!\r\n");

#ifndef NO_RSA
    if ((ret = rsa_test()) != 0)
        fnDebugMsg("RSA      test failed!\r\n");
    else
        fnDebugMsg("RSA      test passed!\r\n");
#endif

#if defined(WOLFSSL_CERT_EXT) && defined(WOLFSSL_TEST_CERT)
    if ((ret = certext_test()) != 0)
        fnDebugMsg("CERT EXT test failed!\r\n");
    else
        fnDebugMsg("CERT EXT test passed!\r\n");
#endif

#ifndef NO_DH
    if ((ret = dh_test()) != 0)
        fnDebugMsg("DH       test failed!\r\n");
    else
        fnDebugMsg("DH       test passed!\r\n");
#endif

#ifndef NO_DSA
    if ((ret = dsa_test()) != 0)
        fnDebugMsg("DSA      test failed!\r\n");
    else
        fnDebugMsg("DSA      test passed!\r\n");
#endif

#ifdef WOLFCRYPT_HAVE_SRP
    if ((ret = srp_test()) != 0)
        fnDebugMsg("SRP      test failed!\r\n");
    else
        fnDebugMsg("SRP      test passed!\r\n");
#endif

#ifndef NO_PWDBASED
    if ((ret = pwdbased_test()) != 0)
        fnDebugMsg("PWDBASED test failed!\r\n");
    else
        fnDebugMsg("PWDBASED test passed!\r\n");
#endif

#ifdef OPENSSL_EXTRA
    if ((ret = openssl_test()) != 0)
        fnDebugMsg("OPENSSL  test failed!\r\n");
    else
        fnDebugMsg("OPENSSL  test passed!\r\n");
#endif

#ifdef HAVE_ECC
    if ((ret = ecc_test()) != 0)
        fnDebugMsg("ECC      test failed!\r\n");
    else
        fnDebugMsg("ECC      test passed!\r\n");
#ifdef HAVE_ECC_ENCRYPT
    if ((ret = ecc_encrypt_test()) != 0)
        fnDebugMsg("ECC Enc  test failed!\r\n");
    else
        fnDebugMsg("ECC Enc  test passed!\r\n");
#endif
#endif

#ifdef HAVE_CURVE25519
    if ((ret = curve25519_test()) != 0)
        fnDebugMsg("CURVE25519 test failed!\r\n");
    else
        fnDebugMsg("CURVE25519 test passed!\r\n");
#endif

#ifdef HAVE_ED25519
    if ((ret = ed25519_test()) != 0)
        fnDebugMsg("ED25519  test failed!\r\n");
    else
        fnDebugMsg("ED25519  test passed!\r\n");
#endif

#ifdef HAVE_LIBZ
    if ((ret = compress_test()) != 0)
        fnDebugMsg("COMPRESS test failed!\r\n");
    else
        fnDebugMsg("COMPRESS test passed!\r\n");
#endif

#ifdef HAVE_PKCS7
    if ((ret = pkcs7enveloped_test()) != 0)
        fnDebugMsg("PKCS7enveloped test failed!\r\n");
    else
        fnDebugMsg("PKCS7enveloped test passed!\r\n");

    if ((ret = pkcs7signed_test()) != 0)
        fnDebugMsg("PKCS7signed    test failed!\r\n");
    else
        fnDebugMsg("PKCS7signed    test passed!\r\n");
#endif

    return 0;
}

