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

volatile int keep_running = 1;

static isotp_wolfssl_ctx isotp_ctx;


struct can_info {
    int sock;
    canid_t arbitration;
    canid_t remote_arbitration;
};

struct can_info can_con_info;

/* Function callback for wolfSSL to add delays to messages when a receiver
 * requests it*/
void can_delay(int microseconds)
{
    usleep(microseconds);
}

void sig_handle(int dummy);

/* Function callback for wolfSSL to send a CAN bus frame of up to 8 bytes */
int can_receive(struct isotp_can_data *data, void *arg, int timeout) {
    int nbytes;
    int ret;
    struct can_info *info = ((struct can_info*)arg);
    struct can_frame frame;
    struct pollfd p[1];

    p[0].fd = info->sock;
    p[0].events = POLLIN;

    /* Poll for new data */
    ret = poll(p, 1, timeout);

    if (ret <= 0) {
        return ret;
    }

    /* Read in the frame data */
    nbytes = read(info->sock, &frame, sizeof(struct can_frame));
    if (nbytes <= 0) {
       return nbytes;
    }
    memcpy(data->data, frame.data, frame.can_dlc);
    data->length = frame.can_dlc;
    return ret;
}

/* Function callback for wolfSSL to send a CAN bus frame of up to 8 bytes */
int can_send(struct isotp_can_data *data, void *arg)
{
    struct can_info *info = ((struct can_info*)arg);
    struct can_frame frame;
    memcpy(frame.data, data->data, data->length);
    frame.can_dlc = data->length;
    frame.can_id = info->arbitration;
    return write(info->sock, &frame, sizeof(struct can_frame));
}


/* Connect to the CAN bus */
int can_connect(const char *address, uint16_t filter)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    /* Setup a CAN bus ID filter so that the kernel space filters out messages
     * we don't need. This is faster than userspace doing it */
    struct can_filter rfilter[1];
    int sock = -1;

    rfilter[0].can_id   = filter;
    rfilter[0].can_mask = 0xFFF;

    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
       perror("Socket open error\n");
       return -1;
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
       return -1;
    }

    return sock;
}

void can_close()
{
    close(can_con_info.sock);
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
    int sock;
    struct sigaction sa = { .sa_handler = sig_handle, /* .sa_flags = 0 */ };
    sigaction(SIGINT, &sa, 0);

    wolfSSL_Init();

    /* Connect to CAN bus provided on command line, filter out everything
     * except for the remote CAN ID */
    sock = can_connect(interface, remote_id);
    if (sock < 1) {
        return -1;
    }
    can_con_info.sock = sock;
    can_con_info.arbitration = local_id;
    return 0;
}

int setup_ssl(enum service_type type, WOLFSSL_CTX **new_ctx,
        WOLFSSL_METHOD **new_method, WOLFSSL **new_ssl)
{
    int ret;
    WOLFSSL_CTX *ctx = NULL;
    WOLFSSL_METHOD* method = NULL;
    WOLFSSL* ssl = NULL;
    char *receive_buffer = malloc(ISOTP_DEFAULT_BUFFER_SIZE);

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

    wolfSSL_SetIO_ISOTP(ssl, &isotp_ctx, can_receive, can_send, can_delay, 0,
            receive_buffer, ISOTP_DEFAULT_BUFFER_SIZE, &can_con_info);

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
