/* tcp.c
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
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

#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "wolfssl/wolfcrypt/settings.h"
#include "wolfssl/ssl.h"
#include "wolf/tcp.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

#define TEST_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define BLINK_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)

static void dump_bytes(const uint8_t *p, uint32_t len)
{
    for (; len; len--, p++) {
        if (((unsigned long)p & 0x07) == 0) {
            DEBUG_printf("\n");
        }
        DEBUG_printf("%02x ", *p);
    }
    DEBUG_printf("\n");
}


err_t wolf_TCPclose(WOLF_SOCKET_T *sock)
{
    err_t err = ERR_OK;
    if (sock->tcp_pcb != NULL)
    {
        tcp_arg(sock->tcp_pcb, NULL);
        tcp_poll(sock->tcp_pcb, NULL, 0);
        tcp_sent(sock->tcp_pcb, NULL);
        tcp_recv(sock->tcp_pcb, NULL);
        tcp_err(sock->tcp_pcb, NULL);
        err = tcp_close(sock->tcp_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(sock->tcp_pcb);
            err = ERR_ABRT;
        }
        sock->tcp_pcb = NULL;
    }
    free(sock);
    return err;
}

static err_t tcp_result(WOLF_SOCKET_T *sock, int status)
{
    if (status == 0) {
        DEBUG_printf("test success\n");
    } else {
        DEBUG_printf("test failed %d\n", status);
    }
    sock->complete = true;
    return 0;
}

static err_t lwip_cb_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    WOLF_SOCKET_T *sock = (WOLF_SOCKET_T *)arg;
    DEBUG_printf("tcp_client_sent %u\n", len);
    sock->sent_len += len;

    if (sock->sent_len >= BUF_SIZE)
    {

        sock->run_count++;
        if (sock->run_count >= TEST_ITERATIONS)
        {
            tcp_result(arg, 0);
            return ERR_OK;
        }

        // We should receive a new buffer from the server
        sock->buffer_len = 0;
        sock->sent_len = 0;
        DEBUG_printf("Waiting for buffer from server\n");
    }

    return ERR_OK;
}

static err_t lwip_cb_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    WOLF_SOCKET_T *sock = (WOLF_SOCKET_T *)arg;
    if (err != ERR_OK)
    {
        DEBUG_printf("connect failed %d\n", err);
        return tcp_result(arg, err);
    }
    sock->connected = true;
    return ERR_OK;
}

static err_t lwip_cb_client_poll(void *arg, struct tcp_pcb *tpcb)
{
    return tcp_result(arg, -1); // no response is an error?
}

static void lwip_cb_client_err(void *arg, err_t err)
{
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err %d\n", err);
        tcp_result(arg, err);
    }
}

static err_t lwip_cb_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    WOLF_SOCKET_T *sock = (WOLF_SOCKET_T *)arg;
    if (!p) {
        return tcp_result(arg, -1);
    }

    cyw43_arch_lwip_check();
    if (p->tot_len > 0)
    {
        const uint16_t buffer_left = BUF_SIZE - sock->buffer_len;
        sock->buffer_len += pbuf_copy_partial(p, sock->buffer + sock->buffer_len,
                                               p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
        DEBUG_printf("sock->buffer_len = %d\n", sock->buffer_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

bool wolf_TCPconnect(WOLF_SOCKET_T *sock, const char *ip, uint32_t port)
{
    ip4addr_aton(ip, &sock->remote_addr);

    DEBUG_printf("wolf_TCPconnect: Connecting to %s port %u\n", ip4addr_ntoa(&sock->remote_addr), port);
    sock->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&sock->remote_addr));
    if (!sock->tcp_pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    tcp_arg(sock->tcp_pcb, sock);
    tcp_poll(sock->tcp_pcb, lwip_cb_client_poll, POLL_TIME_S * 2);
    tcp_sent(sock->tcp_pcb, lwip_cb_client_sent);
    tcp_recv(sock->tcp_pcb, lwip_cb_client_recv);
    tcp_err(sock->tcp_pcb,  lwip_cb_client_err);

    sock->buffer_len = 0;

    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(sock->tcp_pcb, &sock->remote_addr, port, lwip_cb_client_connected);
    cyw43_arch_lwip_end();
    if (err == ERR_OK)
        DEBUG_printf("wolf_TCPconnect: Connecting");
    else {
        DEBUG_printf("wolf_TCPconnect: Failed");
        return WOLF_FAIL;
    }
    while (sock->connected != true) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
    return WOLF_SUCCESS;
}

// get a new TCP client
WOLF_SOCKET_T *wolf_TCPsocket()
{
    WOLF_SOCKET_T *sock = calloc(1, sizeof(WOLF_SOCKET_T));
    if (!sock) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }

    return sock;
}

int wolf_TCPwrite(WOLF_SOCKET_T *sock, const unsigned char *buff, long unsigned int len)
{
    int ret;
    int i;

    DEBUG_printf("wolf_TCPread(%lx, %lx, %d)\n", sock, buff, len);
    sock->sent_len = 0;
    ret = tcp_write(sock->tcp_pcb, buff, len, TCP_WRITE_FLAG_COPY);
 
    if (ret == ERR_OK) {
        tcp_output(sock->tcp_pcb);
    }
    while(sock->sent_len < len) {
        putchar('>');
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
    putchar('\n');
    return (int)len;
}

int wolf_TCPread(WOLF_SOCKET_T *sock, unsigned char *buff, long unsigned int len)
{
    int recv_len;
    int remained;
    int i;
    #define POLLING 200
    
    DEBUG_printf("wolf_TCPread(%lx, %lx, %d)\n", sock, buff, len);

    for(i=0; i<POLLING; i++) {
        if(sock->buffer_len > 0) {
            recv_len = len < sock->buffer_len ? len : sock->buffer_len;
            memcpy(buff, sock->buffer, recv_len);
            if(recv_len >= len) {
                remained = sock->buffer_len - recv_len;
                sock->buffer_len = remained;
                for(i=0; i<remained; i++)
                    sock->buffer[i] = sock->buffer[i+recv_len];
            } else
                sock->buffer_len = 0;

            DEBUG_printf("\n");
            return recv_len;
        } 
        DEBUG_printf(">");
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
}