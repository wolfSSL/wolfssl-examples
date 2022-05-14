/* mqttsimple.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
 *
 * This file is part of wolfMQTT.
 *
 * wolfMQTT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfMQTT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* Standalone Example */

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define CLOSE_SOCKET(sock) closesocket(sock)

#else
/* Requires BSD Style Socket */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define SOCKET int
#define CLOSE_SOCKET(sock) close(sock)
#endif

#include <sys/types.h>

#include <wolfssl/options.h>

#include "wolfmqtt/mqtt_client.h"
#ifndef WOLFTPM_USER_SETTINGS
#include "wolftpm/options.h"
#endif
#include "wolftpm/tpm2.h"
#include "wolftpm/tpm2_wrap.h"


/* Configuration */
#define MQTT_HOST            "127.0.0.1"
#define MQTT_QOS             MQTT_QOS_0
#define MQTT_KEEP_ALIVE_SEC  60
#define MQTT_CMD_TIMEOUT_MS  30000
#define MQTT_CON_TIMEOUT_MS  5000
#define MQTT_CLIENT_ID       "WolfMQTTClientSimple"
#define MQTT_TOPIC_NAME      "wolfMQTT/example/testTopic"
#define MQTT_PUBLISH_MSG     "Test Publish"
#define MQTT_USERNAME        NULL
#define MQTT_PASSWORD        NULL

#define MQTT_USE_TLS     1
#define MQTT_PORT        18883

#define MQTT_MAX_PACKET_SZ   1024
#define INVALID_SOCKET_FD    -1
#define PRINT_BUFFER_SIZE    80

/* Local Variables */
static MqttClient mClient;
static MqttNet mNetwork;
static SOCKET mSockFd = INVALID_SOCKET_FD;
static byte mSendBuf[MQTT_MAX_PACKET_SZ];
static byte mReadBuf[MQTT_MAX_PACKET_SZ];
static volatile word16 mPacketIdLast;

static int gContinue = 1;

/* Local Functions */

/* msg_new on first data callback */
/* msg_done on last data callback */
/* msg->total_len: Payload total length */
/* msg->buffer: Payload buffer */
/* msg->buffer_len: Payload buffer length */
/* msg->buffer_pos: Payload buffer position */
static int mqtt_message_cb(MqttClient *client, MqttMessage *msg,
    byte msg_new, byte msg_done)
{
    byte buf[PRINT_BUFFER_SIZE+1];
    word32 len;

    (void)client;

    if (msg_new) {
        /* Determine min size to dump */
        len = msg->topic_name_len;
        if (len > PRINT_BUFFER_SIZE) {
            len = PRINT_BUFFER_SIZE;
        }
        XMEMCPY(buf, msg->topic_name, len);
        buf[len] = '\0'; /* Make sure its null terminated */

        /* Print incoming message */
        PRINTF("MQTT Message: Topic %s, Qos %d, Len %u",
            buf, msg->qos, msg->total_len);
    }

    /* Print message payload */
    len = msg->buffer_len;
    if (len > PRINT_BUFFER_SIZE) {
        len = PRINT_BUFFER_SIZE;
    }
    XMEMCPY(buf, msg->buffer, len);
    buf[len] = '\0'; /* Make sure its null terminated */
    PRINTF("Payload (%d - %d): %s",
        msg->buffer_pos, msg->buffer_pos + len, buf);

    if (msg_done) {
        PRINTF("MQTT Message: Done");
    }

    if(XMEMCMP(buf, "exit", 4)==0) {
        PRINTF("MQTT EXITING");
        gContinue=0;
    }

    /* Return negative to terminate publish processing */
    return MQTT_CODE_SUCCESS;
}

static void setup_timeout(struct timeval* tv, int timeout_ms)
{
    tv->tv_sec = timeout_ms / 1000;
    tv->tv_usec = (timeout_ms % 1000) * 1000;

    /* Make sure there is a minimum value specified */
    if (tv->tv_sec < 0 || (tv->tv_sec == 0 && tv->tv_usec <= 0)) {
        tv->tv_sec = 0;
        tv->tv_usec = 100;
    }
}

static int socket_get_error(SOCKET sockFd)
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    int so_error = 0;
    socklen_t len = sizeof(so_error);
    getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    return so_error;
#endif
}

static int mqtt_net_connect(void *context, const char* host, word16 port,
    int timeout_ms)
{
    int rc;
    SOCKET sockFd, *pSockFd = (SOCKET*)context;
    struct sockaddr_in addr;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

#ifdef _WIN32
    {
      WORD wVersionRequested;
      WSADATA wsaData;
      int err;

      /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
      wVersionRequested = MAKEWORD(2, 2);

      err = WSAStartup(wVersionRequested, &wsaData); 
    }
#endif

    if (pSockFd == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    (void)timeout_ms;

    /* get address */
    XMEMSET(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    XMEMSET(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    rc = getaddrinfo(host, NULL, &hints, &result);
    if (rc >= 0 && result != NULL) {
        struct addrinfo* res = result;

        /* prefer ip4 addresses */
        while (res) {
            if (res->ai_family == AF_INET) {
                result = res;
                break;
            }
            res = res->ai_next;
        }
        if (result->ai_family == AF_INET) {
            addr.sin_port = htons(port);
            addr.sin_family = AF_INET;
            addr.sin_addr =
                ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
        }
        else {
            rc = -1;
        }
        freeaddrinfo(result);
    }
    if (rc < 0) {
        return MQTT_CODE_ERROR_NETWORK;
    }

    sockFd = socket(addr.sin_family, SOCK_STREAM, 0);
    if (sockFd < 0) {
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* Start connect */
    rc = connect(sockFd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc < 0) {
        PRINTF("NetConnect: Error %d (Sock Err %d)",
            rc, socket_get_error(*pSockFd));
        CLOSE_SOCKET(sockFd);
        return MQTT_CODE_ERROR_NETWORK;
    }

    /* save socket number to context */
    *pSockFd = sockFd;

    return MQTT_CODE_SUCCESS;
}

static int mqtt_net_read(void *context, byte* buf, int buf_len, int timeout_ms)
{
    int rc;
    int *pSockFd = (int*)context;
    int bytes = 0;
    
    struct timeval tv;

    if (pSockFd == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    /* Setup timeout */
    setup_timeout(&tv, timeout_ms);
    setsockopt(*pSockFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

    /* Loop until buf_len has been read, error or timeout */
    while (bytes < buf_len) {
        rc = (int)recv(*pSockFd, &buf[bytes], buf_len - bytes, 0);
        if (rc < 0) {
            rc = socket_get_error(*pSockFd);
            if (rc == 0)
                break; /* timeout */
            PRINTF("NetRead: Error %d", rc);
            return MQTT_CODE_ERROR_NETWORK;
        }
        bytes += rc; /* Data */
    }

    if (bytes == 0) {
        return MQTT_CODE_ERROR_TIMEOUT;
    }

    return bytes;
}

static int mqtt_net_write(void *context, const byte* buf, int buf_len,
    int timeout_ms)
{
    int rc;
    int *pSockFd = (int*)context;
    struct timeval tv;

    if (pSockFd == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    /* Setup timeout */
    setup_timeout(&tv, timeout_ms);
    setsockopt(*pSockFd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));

    rc = (int)send(*pSockFd, buf, buf_len, 0);
    if (rc < 0) {
        PRINTF("NetWrite: Error %d (Sock Err %d)",
            rc, socket_get_error(*pSockFd));
        return MQTT_CODE_ERROR_NETWORK;
    }

    return rc;
}

static int mqtt_net_disconnect(void *context)
{
    int *pSockFd = (int*)context;

    if (pSockFd == NULL) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    CLOSE_SOCKET(*pSockFd);
    *pSockFd = INVALID_SOCKET_FD;

    return MQTT_CODE_SUCCESS;
}

static int mqtt_tls_verify_cb(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    char buffer[WOLFSSL_MAX_ERROR_SZ];
    PRINTF("MQTT TLS Verify Callback: PreVerify %d, Error %d (%s)",
        preverify, store->error, store->error != 0 ?
            wolfSSL_ERR_error_string(store->error, buffer) : "none");
    PRINTF("  Subject's domain name is %s", store->domain);

    if (store->error != 0) {
        /* Allowing to continue */
        /* Should check certificate and return 0 if not okay */
        PRINTF("  Allowing cert anyways");
    }

    return 1;
}


/* from certs/dummy-ecc.pem (as DER) */
static const unsigned char DUMMY_ECC_KEY[] = {
    0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0x05, 0x0F, 0xEA, 0xB6, 0x2C, 0x7C,
    0xD3, 0x3C, 0x66, 0x3D, 0x6B, 0x44, 0xD5, 0x8A, 0xD4, 0x1C, 0xF6, 0x2A, 0x35,
    0x49, 0xB2, 0x36, 0x7D, 0xEC, 0xD4, 0xB3, 0x9A, 0x2B, 0x4F, 0x71, 0xC8, 0xD3,
    0xA0, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0xA1,
    0x44, 0x03, 0x42, 0x00, 0x04, 0x43, 0x98, 0xF7, 0x33, 0x77, 0xB4, 0x55, 0x02,
    0xF1, 0xF3, 0x79, 0x97, 0x67, 0xED, 0xB5, 0x3A, 0x7A, 0xE1, 0x7C, 0xC6, 0xA8,
    0x23, 0x8B, 0x3A, 0x68, 0x42, 0xDD, 0x68, 0x4F, 0x48, 0x6F, 0x2D, 0x9A, 0x7C,
    0x47, 0x20, 0x1F, 0x13, 0x69, 0x71, 0x05, 0x42, 0x5B, 0x9F, 0x23, 0x7D, 0xE0,
    0xA6, 0x5D, 0xD4, 0x11, 0x44, 0xB1, 0x91, 0x66, 0x50, 0xC0, 0x2C, 0x8C, 0x71,
    0x35, 0x0E, 0x28, 0xB4
};

/* from certs/dummy-rsa.pem (as DER) */
static const unsigned char DUMMY_RSA_KEY[] = {
    0x30, 0x82, 0x04, 0xA3, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01, 0x01, 0x00, 0xCF,
    0xDD, 0xB2, 0x17, 0x49, 0xEB, 0xBF, 0xFB, 0xC5, 0x19, 0x13, 0x63, 0x86, 0x49,
    0xBC, 0xFE, 0x8E, 0xED, 0x21, 0x6E, 0x53, 0x18, 0x9C, 0x41, 0xD5, 0xEC, 0x12,
    0x31, 0xF0, 0xF9, 0x90, 0x08, 0x15, 0x68, 0x2F, 0x00, 0x9C, 0xAC, 0x36, 0x28,
    0xF6, 0xD8, 0x50, 0xA0, 0xD4, 0x7C, 0xDF, 0xE7, 0x0F, 0xE1, 0x36, 0xD1, 0xDD,
    0xC0, 0x2B, 0xF0, 0x3D, 0xC9, 0xF0, 0x5B, 0xE4, 0x76, 0x48, 0x91, 0xF0, 0x92,
    0x29, 0x82, 0x75, 0x7F, 0x0B, 0x41, 0x39, 0x77, 0x52, 0xCD, 0x1F, 0x30, 0xA3,
    0xC3, 0x79, 0x92, 0xBD, 0x0A, 0x7F, 0x16, 0xB2, 0x06, 0xFD, 0x49, 0xC5, 0x4D,
    0x34, 0x26, 0xDB, 0x49, 0x06, 0xDB, 0x49, 0x63, 0xB6, 0xE5, 0xA4, 0xEC, 0xC0,
    0x6D, 0x24, 0xF1, 0x82, 0x0F, 0x83, 0x1B, 0xB1, 0x0D, 0xA3, 0x8B, 0x6A, 0x39,
    0x39, 0xB6, 0xB3, 0xA3, 0xE1, 0x77, 0x69, 0x8C, 0xC7, 0x83, 0xE1, 0xBE, 0x9E,
    0xF9, 0xB7, 0xDB, 0xDF, 0xF8, 0x98, 0x7C, 0x9D, 0xC8, 0x72, 0x78, 0xBF, 0x13,
    0x62, 0x27, 0xA1, 0xBF, 0x4B, 0x2B, 0x04, 0x18, 0xCD, 0x2C, 0x10, 0x7E, 0xA5,
    0x33, 0x08, 0xD4, 0x49, 0xF1, 0xEC, 0x99, 0x6F, 0x2E, 0x0B, 0xB4, 0xD3, 0xB3,
    0xC2, 0x20, 0x02, 0xE9, 0x3A, 0xA1, 0xB3, 0x81, 0x9B, 0x0C, 0x02, 0xB0, 0xDE,
    0x9E, 0xEF, 0x0A, 0x47, 0x6E, 0xFA, 0xDB, 0x4D, 0x13, 0x1E, 0x1F, 0xD2, 0x7B,
    0xC6, 0x48, 0xE8, 0x27, 0xDE, 0xBC, 0x8D, 0x4C, 0x60, 0x5A, 0x71, 0xB5, 0xC3,
    0x7F, 0xFC, 0x7C, 0x28, 0xC1, 0x99, 0xF2, 0x7A, 0x3B, 0xCD, 0x6A, 0x76, 0xFE,
    0xA8, 0x9B, 0xDD, 0x03, 0x1E, 0xEB, 0xB4, 0x9D, 0x70, 0x5C, 0x7A, 0x1F, 0xB6,
    0x12, 0xEC, 0xCD, 0xAC, 0x6A, 0xCD, 0x2C, 0x25, 0x53, 0xEF, 0x34, 0xD5, 0xC5,
    0x97, 0x14, 0xD4, 0xB2, 0x86, 0x03, 0x5F, 0x89, 0x02, 0x03, 0x01, 0x00, 0x01,
    0x02, 0x82, 0x01, 0x00, 0x5D, 0x9E, 0xBF, 0x10, 0x48, 0x25, 0xDB, 0x00, 0xFD,
    0x43, 0x8E, 0xFC, 0xFB, 0x45, 0x88, 0xCE, 0xA9, 0xF6, 0xD9, 0x60, 0xC4, 0x22,
    0x48, 0x76, 0x4A, 0x70, 0x19, 0xBD, 0xCE, 0x87, 0xC8, 0x3C, 0x2B, 0xD0, 0x11,
    0xA3, 0x57, 0xED, 0x24, 0x33, 0x8D, 0x01, 0xDE, 0x46, 0xA1, 0x8D, 0x60, 0x96,
    0xC4, 0x0B, 0x2E, 0x52, 0x95, 0x6A, 0x71, 0x1F, 0xB1, 0xE4, 0x9A, 0xD1, 0xF8,
    0x72, 0xE1, 0xBA, 0x81, 0x3C, 0x83, 0x5F, 0x93, 0xA5, 0xD5, 0x9E, 0xD9, 0xD0,
    0x09, 0x46, 0x03, 0x6F, 0x37, 0xC2, 0xD9, 0xA5, 0xA2, 0x68, 0xF0, 0xD6, 0x7A,
    0xF6, 0x34, 0xEC, 0x1D, 0xE5, 0xE8, 0xC0, 0x3B, 0x71, 0x87, 0x9A, 0x0A, 0x52,
    0xD3, 0xD4, 0x58, 0x54, 0x9D, 0x52, 0x4B, 0x1A, 0x4E, 0xF6, 0xC7, 0x99, 0x18,
    0x44, 0x49, 0x4D, 0x88, 0x59, 0x1F, 0xCA, 0x4E, 0xDC, 0x57, 0xB7, 0x1D, 0x9D,
    0xDF, 0x59, 0x91, 0xD9, 0x2E, 0xE0, 0x54, 0xAA, 0x4E, 0x8F, 0x92, 0x82, 0x85,
    0x70, 0xF9, 0x93, 0x90, 0x3A, 0x30, 0xCD, 0xB3, 0x73, 0x81, 0x93, 0xE7, 0xF9,
    0x1F, 0xF6, 0xA9, 0xA9, 0xD4, 0xAE, 0x89, 0x0E, 0x38, 0x11, 0x61, 0xF7, 0xF7,
    0xDC, 0x9B, 0x99, 0x4B, 0xFE, 0xC0, 0x71, 0x78, 0x53, 0x18, 0x0F, 0x23, 0xA9,
    0x11, 0xA0, 0xAA, 0x57, 0xEE, 0x39, 0xAA, 0xEA, 0x2A, 0x7A, 0x8D, 0x12, 0x69,
    0x2C, 0x82, 0x4D, 0xA0, 0xE5, 0x1C, 0xB3, 0x69, 0x9D, 0xA1, 0x30, 0xA3, 0x40,
    0xFA, 0x86, 0x40, 0xD3, 0x8B, 0xF9, 0xAF, 0x98, 0x7D, 0x17, 0x07, 0xA3, 0x29,
    0xE2, 0x57, 0xEF, 0x47, 0xCF, 0x81, 0x22, 0x4D, 0x47, 0x63, 0xA4, 0x2F, 0x1A,
    0x8F, 0xC3, 0x26, 0x1F, 0xF6, 0xC5, 0x81, 0xFF, 0x14, 0xA9, 0x87, 0x56, 0x18,
    0x8A, 0x18, 0xFD, 0x37, 0xC3, 0x4B, 0x8E, 0xE0, 0x6B, 0x2C, 0x07, 0x4B, 0x05,
    0x02, 0x81, 0x81, 0x00, 0xED, 0x51, 0x06, 0x91, 0xB8, 0x94, 0x5E, 0x17, 0x9B,
    0x22, 0x25, 0xEF, 0x23, 0x76, 0x61, 0x26, 0xFA, 0xAC, 0xEE, 0xC1, 0x99, 0x8E,
    0x55, 0x38, 0x85, 0xD2, 0x15, 0x06, 0x6E, 0xBB, 0x45, 0xBB, 0xFE, 0x5F, 0xF7,
    0xD2, 0xA4, 0x41, 0x15, 0x24, 0x67, 0x8E, 0xA2, 0x6B, 0xBA, 0xAA, 0x28, 0x84,
    0x22, 0x63, 0xEE, 0xA8, 0xA0, 0xD0, 0xEA, 0x47, 0x8C, 0xAC, 0x4E, 0x98, 0x18,
    0x8A, 0xF2, 0x19, 0x76, 0x50, 0x9D, 0xFE, 0xD1, 0x59, 0xC3, 0xC1, 0x23, 0x3B,
    0x31, 0x73, 0xC7, 0x71, 0x3E, 0x94, 0xC8, 0x6D, 0x7F, 0xBA, 0x30, 0xF2, 0x4C,
    0x1A, 0x7E, 0x74, 0x52, 0x78, 0xA0, 0xAB, 0x69, 0x0C, 0x44, 0x59, 0xD0, 0xB0,
    0xFE, 0x2F, 0xE8, 0xC2, 0x18, 0xE7, 0x24, 0x8B, 0x73, 0xDF, 0x4F, 0x40, 0x92,
    0x0C, 0x8C, 0x6C, 0x92, 0x27, 0xBC, 0x3F, 0x5B, 0x79, 0x44, 0xC1, 0x32, 0xCC,
    0xA2, 0xB7, 0x02, 0x81, 0x81, 0x00, 0xE0, 0x3B, 0x1C, 0xC4, 0xC4, 0x69, 0x0D,
    0x6B, 0x57, 0x19, 0xB7, 0x59, 0x18, 0x92, 0xB2, 0x09, 0x39, 0x66, 0x97, 0xD1,
    0x18, 0xDE, 0x6B, 0x5F, 0xC5, 0x9B, 0x11, 0x47, 0x1E, 0xEA, 0xE3, 0xAC, 0x36,
    0x2B, 0x30, 0x99, 0x81, 0x00, 0x3D, 0x39, 0x41, 0x03, 0x90, 0x77, 0xDE, 0x4A,
    0xE1, 0x48, 0xDF, 0x98, 0x86, 0x03, 0x3B, 0xEA, 0xAF, 0xC8, 0xF6, 0xD7, 0x4F,
    0xE6, 0xAE, 0x70, 0xF2, 0x3D, 0xBB, 0xF2, 0x63, 0xB9, 0x2D, 0x3C, 0x08, 0xB3,
    0x10, 0x9E, 0x97, 0x6C, 0x8D, 0x28, 0x34, 0xAE, 0xDA, 0xD9, 0xA1, 0x8E, 0x3A,
    0x51, 0x7A, 0xA1, 0x14, 0x3F, 0xFB, 0xEA, 0x3B, 0xB4, 0x93, 0xAA, 0x14, 0x7A,
    0xB4, 0xD7, 0xCA, 0x7B, 0x61, 0xAF, 0xF5, 0x87, 0x1A, 0x64, 0xA9, 0x3E, 0x3C,
    0x7A, 0xDD, 0x11, 0x7F, 0x01, 0x2D, 0xA6, 0x91, 0xED, 0x3D, 0x28, 0x9C, 0x67,
    0xC2, 0x5C, 0xCF, 0xBF, 0x02, 0x81, 0x81, 0x00, 0xCE, 0x0C, 0x59, 0xCD, 0xD0,
    0x1B, 0x52, 0x0E, 0xE0, 0xED, 0x27, 0x4E, 0x98, 0xD5, 0xC1, 0xC8, 0x9C, 0x41,
    0xE6, 0x13, 0x46, 0x06, 0x24, 0xCC, 0x2C, 0xB4, 0x98, 0xF8, 0xBA, 0xCF, 0xF2,
    0xDE, 0x25, 0x20, 0xA2, 0x05, 0xCC, 0x03, 0x8E, 0x1D, 0xCB, 0xA4, 0x36, 0x35,
    0x9F, 0x1E, 0xFA, 0x8A, 0xAF, 0x69, 0x60, 0xE0, 0x1C, 0xB1, 0x07, 0x99, 0x13,
    0xF4, 0xCF, 0x50, 0x93, 0x8E, 0xA0, 0x61, 0xA7, 0x2E, 0x9B, 0xDF, 0x91, 0x59,
    0x84, 0xF3, 0x7E, 0x69, 0x78, 0xA8, 0x73, 0xF4, 0x49, 0x47, 0xD9, 0x35, 0xE9,
    0x7E, 0x79, 0xDD, 0x06, 0x62, 0xC2, 0x84, 0xB0, 0xCE, 0x77, 0x82, 0x1C, 0x75,
    0x40, 0x2B, 0x53, 0x5D, 0x39, 0x75, 0xD3, 0x7C, 0x23, 0x2F, 0x1D, 0xB5, 0xCE,
    0xE7, 0x86, 0xE2, 0x23, 0x6C, 0xAD, 0xC7, 0xDE, 0xA6, 0x8D, 0x75, 0xDD, 0x30,
    0x4F, 0x98, 0x07, 0x49, 0x51, 0xC5, 0x02, 0x81, 0x80, 0x46, 0x19, 0x34, 0xBD,
    0x2E, 0xC9, 0xC8, 0xB0, 0x2D, 0xE2, 0x94, 0x36, 0xFE, 0x3F, 0x9D, 0xF8, 0xD4,
    0x41, 0x06, 0x65, 0x0F, 0xE9, 0x38, 0x98, 0x10, 0x26, 0x92, 0x18, 0x31, 0xCA,
    0x2C, 0xB2, 0xC1, 0x9C, 0x6E, 0xED, 0x0E, 0x2F, 0x0C, 0xF4, 0xC1, 0x26, 0x64,
    0x1B, 0x95, 0x1A, 0xC3, 0xA3, 0x0C, 0x83, 0x9A, 0x21, 0x98, 0xB1, 0x9D, 0x92,
    0xAD, 0xD8, 0x51, 0xDA, 0x43, 0xDE, 0x7B, 0x5C, 0x61, 0x4D, 0x3D, 0x6F, 0xBE,
    0x7C, 0x6E, 0x1B, 0xCC, 0xAE, 0x47, 0x98, 0x5F, 0xE8, 0x99, 0xCF, 0xB0, 0x0B,
    0x29, 0x3E, 0x55, 0x6C, 0xF3, 0x71, 0x37, 0xEB, 0x68, 0xCD, 0xA9, 0x2C, 0xA2,
    0x9D, 0x21, 0x19, 0xDB, 0x3F, 0x3A, 0xC5, 0xA7, 0x9C, 0x62, 0x9D, 0x81, 0xDA,
    0xC6, 0x2D, 0xF6, 0xAA, 0x52, 0x42, 0x0D, 0xFA, 0x48, 0x53, 0x32, 0x7B, 0x80,
    0x0B, 0x1A, 0x1A, 0x35, 0xE0, 0xDD, 0xF1, 0x02, 0x81, 0x80, 0x76, 0x46, 0xB9,
    0x57, 0x91, 0x3F, 0x64, 0x5D, 0x42, 0x37, 0x70, 0x9D, 0x44, 0x38, 0x09, 0x09,
    0x42, 0x3E, 0x2E, 0x8A, 0x7A, 0xA4, 0x57, 0x4B, 0x81, 0x95, 0x65, 0x47, 0x3C,
    0xF3, 0x77, 0x54, 0xE3, 0x7D, 0xEC, 0x06, 0xC9, 0x26, 0xAB, 0xDD, 0x66, 0x73,
    0x54, 0x86, 0x31, 0x26, 0x75, 0x5B, 0x84, 0xAB, 0xD2, 0xA2, 0x6A, 0x9B, 0x6E,
    0xDD, 0x45, 0xAE, 0x81, 0x49, 0x12, 0x8D, 0x03, 0x1C, 0x1B, 0x6B, 0x5B, 0x37,
    0xFA, 0xE7, 0x05, 0x9F, 0xBD, 0x66, 0xDD, 0x6C, 0xD7, 0x16, 0x0D, 0xCC, 0x64,
    0x19, 0xC2, 0xCD, 0xC3, 0xA9, 0xED, 0x70, 0xFA, 0x75, 0xD8, 0x41, 0xF7, 0xC6,
    0x84, 0xE8, 0x40, 0xF0, 0xE5, 0x93, 0x88, 0xE2, 0x4E, 0x4F, 0xE4, 0x5F, 0xDF,
    0x53, 0xAB, 0xA7, 0x06, 0xDC, 0x64, 0x7E, 0x51, 0xE8, 0x7E, 0x1C, 0x33, 0x9F,
    0xBF, 0x5E, 0x58, 0xBC, 0x7D, 0xA3, 0x80, 0x84
};


/* Function checks key to see if its the "dummy" key */
static inline int myTpmCheckKey(wc_CryptoInfo* info, TpmCryptoDevCtx* ctx)
{
    int ret = 0;

#ifndef NO_RSA
    if (info && info->pk.type == WC_PK_TYPE_RSA) {
        byte    e[sizeof(word32)], e2[sizeof(word32)];
        byte    n[WOLFTPM2_WRAP_RSA_KEY_BITS/8], n2[WOLFTPM2_WRAP_RSA_KEY_BITS/8];
        word32  eSz = sizeof(e), e2Sz = sizeof(e);
        word32  nSz = sizeof(n), n2Sz = sizeof(n);
        RsaKey  rsakey;
        word32  idx = 0;

        /* export the raw public RSA portion */
        ret = wc_RsaFlattenPublicKey(info->pk.rsa.key, e, &eSz, n, &nSz);
        if (ret == 0) {
            /* load the modulus for the dummy key */
            ret = wc_InitRsaKey(&rsakey, NULL);
            if (ret == 0) {
                ret = wc_RsaPrivateKeyDecode(DUMMY_RSA_KEY, &idx, &rsakey,
                    (word32)sizeof(DUMMY_RSA_KEY));
                if (ret == 0) {
                    ret = wc_RsaFlattenPublicKey(&rsakey, e2, &e2Sz, n2, &n2Sz);
                }
                wc_FreeRsaKey(&rsakey);
            }
        }

        if (ret == 0 && XMEMCMP(n, n2, nSz) == 0) {
        #ifdef DEBUG_WOLFTPM
            printf("Detected dummy key, so using TPM RSA key handle\n");
        #endif
            ret = 1;
        }
    }
#endif
#if defined(HAVE_ECC)
    if (info && info->pk.type == WC_PK_TYPE_ECDSA_SIGN) {
        byte    qx[WOLFTPM2_WRAP_ECC_KEY_BITS/8], qx2[WOLFTPM2_WRAP_ECC_KEY_BITS/8];
        byte    qy[WOLFTPM2_WRAP_ECC_KEY_BITS/8], qy2[WOLFTPM2_WRAP_ECC_KEY_BITS/8];
        word32  qxSz = sizeof(qx), qx2Sz = sizeof(qx2);
        word32  qySz = sizeof(qy), qy2Sz = sizeof(qy2);
        ecc_key eccKey;
        word32  idx = 0;

        /* export the raw public ECC portion */
        ret = wc_ecc_export_public_raw(info->pk.eccsign.key, qx, &qxSz, qy, &qySz);
        if (ret == 0) {
            /* load the ECC public x/y for the dummy key */
            ret = wc_ecc_init(&eccKey);
            if (ret == 0) {
                ret = wc_EccPrivateKeyDecode(DUMMY_ECC_KEY, &idx, &eccKey,
                    (word32)sizeof(DUMMY_ECC_KEY));
                if (ret == 0) {
                    ret = wc_ecc_export_public_raw(&eccKey, qx2, &qx2Sz, qy2, &qy2Sz);
                }
                wc_ecc_free(&eccKey);
            }
        }

        if (ret == 0 && XMEMCMP(qx, qx2, qxSz) == 0 &&
                        XMEMCMP(qy, qy2, qySz) == 0) {
        #ifdef DEBUG_WOLFTPM
            printf("Detected dummy key, so using TPM ECC key handle\n");
        #endif
            ret = 1;
        }
    }
#endif
    (void)info;
    (void)ctx;

    /* non-zero return code means its a "dummy" key (not valid) and the
        provided TPM handle will be used, not the wolf public key info */
    return ret;
}


static int readKeyBlob(const char* filename, WOLFTPM2_KEYBLOB* key)
{
    int rc = 0;
#if !defined(NO_FILESYSTEM) && !defined(NO_WRITE_TEMP_FILES)
    XFILE  fp = NULL;
    size_t fileSz = 0;
    size_t bytes_read = 0;
    byte pubAreaBuffer[sizeof(TPM2B_PUBLIC)];
    int pubAreaSize;

    XMEMSET(key, 0, sizeof(WOLFTPM2_KEYBLOB));

    fp = XFOPEN(filename, "rb");
    if (fp != XBADFILE) {
        XFSEEK(fp, 0, XSEEK_END);
        fileSz = XFTELL(fp);
        XREWIND(fp);
        if (fileSz > sizeof(key->priv) + sizeof(key->pub)) {
            printf("File size check failed\n");
            rc = BUFFER_E; goto exit;
        }
        printf("Reading %d bytes from %s\n", (int)fileSz, filename);

        bytes_read = XFREAD(&key->pub.size, 1, sizeof(key->pub.size), fp);
        if (bytes_read != sizeof(key->pub.size)) {
            printf("Read %zu, expected size marker of %zu bytes\n",
                bytes_read, sizeof(key->pub.size));
            goto exit;
        }
        fileSz -= bytes_read;

        bytes_read = XFREAD(pubAreaBuffer, 1, sizeof(UINT16) + key->pub.size, fp);
        if (bytes_read != sizeof(UINT16) + key->pub.size) {
            printf("Read %zu, expected public blob %zu bytes\n",
                bytes_read, sizeof(UINT16) + key->pub.size);
            goto exit;
        }
        fileSz -= bytes_read; /* Reminder bytes for private key part */

        /* Decode the byte stream into a publicArea structure ready for use */
        rc = TPM2_ParsePublic(&key->pub, pubAreaBuffer,
            (word32)sizeof(pubAreaBuffer), &pubAreaSize);
        if (rc != TPM_RC_SUCCESS) return rc;

        if (fileSz > 0) {
            printf("Reading the private part of the key\n");
            bytes_read = XFREAD(&key->priv, 1, fileSz, fp);
            if (bytes_read != fileSz) {
                printf("Read %zu, expected private blob %zu bytes\n",
                    bytes_read, fileSz);
                goto exit;
            }
            rc = 0; /* success */
        }

        /* sanity check the sizes */
        if (pubAreaSize != (key->pub.size + (int)sizeof(key->pub.size)) ||
             key->priv.size > sizeof(key->priv.buffer)) {
            printf("Struct size check failed (pub %d, priv %d)\n",
                   key->pub.size, key->priv.size);
            rc = BUFFER_E;
        }
    }
    else {
        rc = BUFFER_E;
        printf("File %s not found!\n", filename);
        printf("Keys can be generated by running:\n"
               "  ./examples/keygen/keygen rsa_test_blob.raw -rsa -t\n"
               "  ./examples/keygen/keygen ecc_test_blob.raw -ecc -t\n");
    }

exit:
    if (fp)
      XFCLOSE(fp);
#else
    (void)filename;
    (void)key;
#endif /* !NO_FILESYSTEM && !NO_WRITE_TEMP_FILES */
    return rc;
}

TpmCryptoDevCtx tpmCtx;
WOLFTPM2_DEV dev;
WOLFTPM2_KEY storageKey;
WOLFTPM2_KEY key;
WOLFTPM2_KEYBLOB keyblob;
static const char gStorageKeyAuth[] = "ThisIsMyStorageKeyAuth";
static const char gKeyAuth[] =        "ThisIsMyKeyAuth";

/* Use this callback to setup TLS certificates and verify callbacks */
static int mqtt_tls_cb(MqttClient* client)
{
    int rc = WOLFSSL_FAILURE;
    TPM_ALG_ID alg = TPM_ALG_RSA;

    int tpmDevId;

#ifndef NO_RSA
    rc = readKeyBlob(CERT_PATH "rsa_test_blob.raw", &keyblob);
#else
    rc = readKeyBlob(CERT_PATH "ecc_test_blob.raw", &keyblob);
#endif
    
    /* Use highest available and allow downgrade. If wolfSSL is built with
     * old TLS support, it is possible for a server to force a downgrade to
     * an insecure version. */
    client->tls.ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
    if (client->tls.ctx) {
        wolfSSL_CTX_set_verify(client->tls.ctx, WOLFSSL_VERIFY_PEER,
                               mqtt_tls_verify_cb);

#if 0 /* Example how to enable CRL and OCSP */
        wolfSSL_CTX_EnableCRL(client->tls.ctx, WOLFSSL_CRL_CHECKALL);
        wolfSSL_CTX_EnableOCSP(client->tls.ctx, WOLFSSL_OCSP_CHECKALL);
        wolfSSL_CTX_EnableOCSPStapling(client->tls.ctx);
#endif

        /* default to success */
        rc = WOLFSSL_SUCCESS;

        /* Initialize TPM */
        rc = wolfTPM2_Init(&dev, NULL, NULL);
        if (rc != 0) {
          PRINTF("TPM init failed\n");
          return rc;
        }

        tpmCtx.rsaKey = &key;
        tpmCtx.checkKeyCb = myTpmCheckKey; /* detects if using "dummy" key */
        tpmCtx.storageKey = &storageKey;

        rc = wolfTPM2_SetCryptoDevCb(&dev, wolfTPM2_CryptoDevCb, &tpmCtx, &tpmDevId);
        if (rc != 0)  {
            PRINTF("wolfTPM2_SetCryptoDevCb failed\n");
            return rc;
        }

        /* load SRK and tpm key */
        rc = wolfTPM2_CreateSRK(&dev, &storageKey, alg,
                                (byte*)gStorageKeyAuth, sizeof(gStorageKeyAuth)-1);
        if (rc != 0) goto exit;

        rc = wolfTPM2_LoadKey(&dev, &keyblob, &storageKey.handle);
        if (rc != TPM_RC_SUCCESS) {
            printf("wolfTPM2_LoadKey failed\n");
            goto exit;
        }
        printf("Loaded key to 0x%x\n", (word32)keyblob.handle.hndl);

        /* set authentication */
        key.handle = keyblob.handle;
        key.pub    = keyblob.pub;
        key.handle.auth.size = sizeof(gKeyAuth)-1;
        XMEMCPY(key.handle.auth.buffer, gKeyAuth, key.handle.auth.size);

        /* Setup DevID */
        wolfSSL_CTX_SetDevId(client->tls.ctx, tpmDevId);


        printf("Loading RSA dummy key\n");

        /* Private key is on TPM and crypto dev callbacks are used */
        /* TLS client (mutual auth) requires a dummy key loaded (workaround) */
        if (wolfSSL_CTX_use_PrivateKey_buffer(client->tls.ctx, DUMMY_RSA_KEY,
            sizeof(DUMMY_RSA_KEY), WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
            printf("Failed to set key!\r\n");
            goto exit;
        }

        /* load certificate */
        rc = wolfSSL_CTX_use_certificate_file(client->tls.ctx,
                                              CERT_PATH "client-rsa-cert.pem",
                                              WOLFSSL_FILETYPE_PEM);

    }

 exit:
    PRINTF("MQTT TLS Setup (%d)", rc);

    return rc;
}

static word16 mqtt_get_packetid(void)
{
    /* Check rollover */
    if (mPacketIdLast >= MAX_PACKET_ID) {
        mPacketIdLast = 0;
    }

    return ++mPacketIdLast;
}

/* Public Function */
int mqttsimple_test(void)
{
    int rc = 0;
    MqttObject mqttObj;
    MqttTopic topics[1];

    /* Initialize MQTT client */
    XMEMSET(&mNetwork, 0, sizeof(mNetwork));
    mNetwork.connect = mqtt_net_connect;
    mNetwork.read = mqtt_net_read;
    mNetwork.write = mqtt_net_write;
    mNetwork.disconnect = mqtt_net_disconnect;
    mNetwork.context = &mSockFd;
    rc = MqttClient_Init(&mClient, &mNetwork, mqtt_message_cb,
        mSendBuf, sizeof(mSendBuf), mReadBuf, sizeof(mReadBuf),
        MQTT_CON_TIMEOUT_MS);
    if (rc != MQTT_CODE_SUCCESS) {
        goto exit;
    }
    PRINTF("MQTT Init Success");

    /* Connect to broker */
    rc = MqttClient_NetConnect(&mClient, MQTT_HOST, MQTT_PORT,
        MQTT_CON_TIMEOUT_MS, MQTT_USE_TLS, mqtt_tls_cb);
    if (rc != MQTT_CODE_SUCCESS) {
        goto exit;
    }
    PRINTF("MQTT Network Connect Success: Host %s, Port %d, UseTLS %d",
        MQTT_HOST, MQTT_PORT, MQTT_USE_TLS);

    /* Send Connect and wait for Ack */
    XMEMSET(&mqttObj, 0, sizeof(mqttObj));
    mqttObj.connect.keep_alive_sec = MQTT_KEEP_ALIVE_SEC;
    mqttObj.connect.client_id = MQTT_CLIENT_ID;
    mqttObj.connect.username = MQTT_USERNAME;
    mqttObj.connect.password = MQTT_PASSWORD;
    rc = MqttClient_Connect(&mClient, &mqttObj.connect);
    if (rc != MQTT_CODE_SUCCESS) {
        goto exit;
    }
    PRINTF("MQTT Broker Connect Success: ClientID %s, Username %s, Password %s",
        MQTT_CLIENT_ID,
        (MQTT_USERNAME == NULL) ? "Null" : MQTT_USERNAME,
        (MQTT_PASSWORD == NULL) ? "Null" : MQTT_PASSWORD);

    /* Subscribe and wait for Ack */
    XMEMSET(&mqttObj, 0, sizeof(mqttObj));
    topics[0].topic_filter = MQTT_TOPIC_NAME;
    topics[0].qos = MQTT_QOS;
    mqttObj.subscribe.packet_id = mqtt_get_packetid();
    mqttObj.subscribe.topic_count = sizeof(topics) / sizeof(MqttTopic);
    mqttObj.subscribe.topics = topics;
    rc = MqttClient_Subscribe(&mClient, &mqttObj.subscribe);
    if (rc != MQTT_CODE_SUCCESS) {
        goto exit;
    }
    PRINTF("MQTT Subscribe Success: Topic %s, QoS %d",
        MQTT_TOPIC_NAME, MQTT_QOS);

    /* Publish */
    XMEMSET(&mqttObj, 0, sizeof(mqttObj));
    mqttObj.publish.qos = MQTT_QOS;
    mqttObj.publish.topic_name = MQTT_TOPIC_NAME;
    mqttObj.publish.packet_id = mqtt_get_packetid();
    mqttObj.publish.buffer = (byte*)MQTT_PUBLISH_MSG;
    mqttObj.publish.total_len = (word32)XSTRLEN(MQTT_PUBLISH_MSG);
    rc = MqttClient_Publish(&mClient, &mqttObj.publish);
    if (rc != MQTT_CODE_SUCCESS) {
        goto exit;
    }
    PRINTF("MQTT Publish: Topic %s, Qos %d, Message %s",
        mqttObj.publish.topic_name, mqttObj.publish.qos, mqttObj.publish.buffer);

    /* Wait for messages */
    while (gContinue) {
        rc = MqttClient_WaitMessage_ex(&mClient, &mqttObj, MQTT_CMD_TIMEOUT_MS);

        if (rc == MQTT_CODE_ERROR_TIMEOUT) {
            /* send keep-alive ping */
            rc = MqttClient_Ping_ex(&mClient, &mqttObj.ping);
            if (rc != MQTT_CODE_SUCCESS) {
                break;
            }
            PRINTF("MQTT Keep-Alive Ping");
        }
        else if (rc != MQTT_CODE_SUCCESS) {
            break;
        }
    }

exit:
    if (rc != MQTT_CODE_SUCCESS) {
        PRINTF("MQTT Error %d: %s", rc, MqttClient_ReturnCodeToString(rc));
    }
    return rc;
}

int main(int argc, char** argv)
{
    int rc = -1;
    (void)argc;
    (void)argv;

    wolfSSL_Debugging_ON();
    rc = mqttsimple_test();


    if(storageKey.handle.hndl) {
        wolfTPM2_UnloadHandle(&dev, &storageKey.handle);
    }
    if (key.handle.hndl) {
        wolfTPM2_UnloadHandle(&dev, &key.handle);
    }

    return (rc == 0) ? 0 : EXIT_FAILURE;
}
