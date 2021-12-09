/* common.c
 *
 * Copyright (C) 2021 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "common.h"

static int sock = -1;
volatile int keep_running = 1;

static uint8_t *copy_buf = NULL;
static uint8_t *copy_buf_ptr;
static size_t copy_buf_len;
static IsoTpLink g_link;

/* Alloc send and receive buffer statically in RAM */
static uint8_t g_isotpRecvBuf[ISOTP_BUFSIZE];
static uint8_t g_isotpSendBuf[ISOTP_BUFSIZE];

void sig_handle(int dummy);

/* Debug function required by ISO-TP to compile, can be blank */
void isotp_user_debug(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\r\n");
    va_end(args);
}

/* Timing function required by ISO-TP to compile */
uint32_t isotp_user_get_ms(void) {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}

/* CAN bus send function require by ISO-TP to compile */
int isotp_user_send_can(const uint32_t arbitration_id, const uint8_t* data,
        const uint8_t size) {
    /* Copy the message into a CAN bus frame, the message will always be
     * 8 bytes or less */
    struct can_frame frame;
    frame.can_id = arbitration_id;
    frame.can_dlc = size;
    memcpy(frame.data, data, size);

    /* Write the frame to the CAN bus */
    if (write(sock, &frame, sizeof(struct can_frame))
            != sizeof(struct can_frame)) {
       perror("Write error\n");
       return -1;
    }

    return 0;
}

/* Our CAN bus receive function */
int can_receive(uint8_t data[CAN_MSG_LEN], int *length) {
    int nbytes;
    int ret;
    struct can_frame frame;
    struct pollfd p[1];

    p[0].fd = sock;
    p[0].events = POLLIN;

    /* Poll for new data */
    ret = poll(p, 1, 10);

    if (ret < 0) {
        perror("Poll error\n");
        return 1;
    }
    else if (ret == 0) {
        /* No data */
        *length = 0;
        return EAGAIN;
    }

    /* Read in the frame data */
    nbytes = read(sock, &frame, sizeof(struct can_frame));
    if (nbytes < 0) {
       perror("Read error\n");
       return 1;
    }
    memcpy(data, frame.data, frame.can_dlc);
    *length = frame.can_dlc;
    return 0;
}

/* Connect to the CAN bus */
int can_connect(const char *address, uint16_t filter)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    /* Setup a CAN bus ID filter so that the kernel space filters out messages
     * we don't need. This is faster than userspace doing it */
    struct can_filter rfilter[1];
    rfilter[0].can_id   = filter;
    rfilter[0].can_mask = 0xFFF;

    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
       perror("Socket open error\n");
       return 1;
    }
    /* Set the filter */
    setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    strcpy(ifr.ifr_name, address);
    ioctl(sock, SIOCGIFINDEX, &ifr);

    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
       perror("Bind error\n");
       return 1;
    }

    /* Setup the receive copy buffer */
    copy_buf = malloc(ISOTP_BUFSIZE);
    copy_buf_ptr = copy_buf;
    copy_buf_len = 0;
    if (!copy_buf) {
        fprintf(stderr, "Copy buf malloc fail\n");
        return MEMORY_E;
    }

    return 0;
}

void can_close()
{
    free(copy_buf);
    close(sock);
}

/* The wolfSSL send packet callback. This function sends the data via ISO-TP
 * and then listens and processes the ISO-TP control packets until the message
 * is completely sent */
int send_ssl(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    uint8_t data[CAN_MSG_LEN];
    int length;
    IsoTpLink *g_link = (struct IsoTpLink*)ctx;
    int ret = isotp_send(g_link, (uint8_t*)buf, sz);
    (void) ssl;
    printf("Sending %d bytes\n", sz);

    if (ret) {
        fprintf(stderr, "Error from isotp: %d\n", ret);
        return -64;
    }

    while(g_link->send_status != ISOTP_SEND_STATUS_IDLE) {
        ret = can_receive(data, &length);
        if (!ret && (length > 0)) {
            /* Got a control frame */
            isotp_on_can_message(g_link, data, length);
        } else if (ret != EAGAIN) {
            fprintf(stderr, "Receive error\n");
        }
        isotp_poll(g_link);
    }

    return sz;
}

/* Receive callback for wolfSSL. ISO-TP will only return a buffer once and it
 * can be more data than wolfSSL wants, so we copy this into an intermediate
 * buffer and just return what wolfSSL is asking for */
int recv_ssl(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    uint8_t data[CAN_MSG_LEN];
    int data_len;
    uint16_t msg_len = 0;
    IsoTpLink *g_link = (struct IsoTpLink*)ctx;
    (void) ssl;

    if (!copy_buf_len) {
        while (isotp_receive(g_link, copy_buf, ISOTP_BUFSIZE, &msg_len)
                != ISOTP_RET_OK) {
            int ret = can_receive(data, &data_len);
            if (ret == EAGAIN) {
                return 0;
            }
            if (!ret && (data_len > 0)) {
                isotp_on_can_message(g_link, data, data_len);
            } else if (ret) {
                fprintf(stderr, "Recieve error\n");
                return -99;
            }
        }
        if (msg_len) {
            copy_buf_ptr = copy_buf;
            printf("Receiving %d bytes\n", msg_len);
            copy_buf_len = msg_len;
        }
    }

    if (copy_buf_len >= (size_t)sz) {
        memcpy(buf, copy_buf_ptr, sz);
        copy_buf_ptr+= sz;
        copy_buf_len-= sz;
        return sz;
    } else {
        memcpy(buf, copy_buf_ptr, copy_buf_len);
        sz = copy_buf_len;
        copy_buf_len = 0;
        return sz;
    }

    return msg_len;
}

void close_ssl(WOLFSSL_CTX *ctx, WOLFSSL *ssl)
{
    if (ssl) {
        int ret = WOLFSSL_SHUTDOWN_NOT_DONE;
        while (ret == WOLFSSL_SHUTDOWN_NOT_DONE) {
            ret = wolfSSL_shutdown(ssl);
        }
        if (ret != WOLFSSL_SUCCESS) {
            char buffer[ERR_MSG_LEN];
            int err = wolfSSL_get_error(ssl, ret);
            fprintf(stderr, "Error shutting down TLS connection: %d, %s",
                    err, wolfSSL_ERR_error_string(err, buffer));
            return;
        }
    }
    can_close();

    wolfSSL_free(ssl);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
}

void sig_handle(int dummy)
{
    (void) dummy;
    keep_running = 0;
}

int setup_connection(const char *interface, int local_id, int remote_id)
{
    struct sigaction sa = { .sa_handler = sig_handle, /* .sa_flags = 0 */ };
    sigaction(SIGINT, &sa, 0);

    wolfSSL_Init();

    /* Connect to CAN bus provided on command line, filter out everything
     * except for the local CAN ID */
    if (can_connect(interface, local_id)) {
        return -1;
    }

    /* Setup ISO-TP and set the buffers */
    isotp_init_link(&g_link, remote_id, g_isotpSendBuf,
            sizeof(g_isotpSendBuf), g_isotpRecvBuf, sizeof(g_isotpRecvBuf));

    return 0;
}

int setup_ssl(enum service_type type, WOLFSSL_CTX **new_ctx,
        WOLFSSL_METHOD **new_method, WOLFSSL **new_ssl)
{
    int ret;
    WOLFSSL_CTX *ctx = NULL;
    WOLFSSL_METHOD* method = NULL;
    WOLFSSL* ssl = NULL;

    if (type == SERVICE_TYPE_CLIENT) {
        method = wolfTLSv1_3_client_method();
    } else {
        method = wolfTLSv1_3_server_method();
    }

    if (!method) {
        fprintf(stderr, "Could not init wolfSSL method\n");
        return -1;
    }

    ctx = wolfSSL_CTX_new(method);
    if (!ctx) {
        fprintf(stderr, "Could not init wolfSSL context\n");
        close_ssl(NULL, NULL);
        return -1;
    }

    /* Set the send and receive callback functions which will use the
     * CAN bus */
    wolfSSL_CTX_SetIOSend(ctx, send_ssl);
    wolfSSL_CTX_SetIORecv(ctx, recv_ssl);

    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_NONE, NULL);

    if (type == SERVICE_TYPE_CLIENT) {
        ret = wolfSSL_CTX_load_verify_locations(ctx, "client.pem", NULL);
    } else {
        ret = wolfSSL_CTX_use_certificate_file(ctx, "server.pem",
                WOLFSSL_FILETYPE_PEM);
    }

    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load cert, "
                "please check the file.\n");
        close_ssl(ctx, NULL);
        return -1;
    }

    if (type == SERVICE_TYPE_SERVER) {
        if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, "server.key",
                        WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load key file, "
                    "please check the file.\n");
            close_ssl(ctx, NULL);
            return -1;
        }
    }

    ssl = wolfSSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "Could not init wolfSSL\n");
        close_ssl(ctx, NULL);
        return -1;
    }

    /* Set the read and write context to both use the ISO-TP link */
    wolfSSL_SetIOWriteCtx(ssl, &g_link);
    wolfSSL_SetIOReadCtx(ssl, &g_link);

    if (type == SERVICE_TYPE_CLIENT) {
        ret = wolfSSL_connect(ssl);
    } else {
        ret = wolfSSL_accept(ssl);
    }

    wolfSSL_set_using_nonblock(ssl, 1);

    if (ret != WOLFSSL_SUCCESS) {
        char buffer[ERR_MSG_LEN];
        int err = wolfSSL_get_error(ssl, ret);
        fprintf(stderr, "ERROR: failed to connect using wolfSSL: %d, %s\n",
                err, wolfSSL_ERR_error_string(err, buffer));
        close_ssl(ctx, ssl);
        return -1;
    }
    *new_ctx = ctx;
    *new_method = method;
    *new_ssl = ssl;

    printf("SSL handshake done!\n");

    return 0;
}
