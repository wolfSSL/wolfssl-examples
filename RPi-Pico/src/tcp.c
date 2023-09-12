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

#include "wolf/tcp.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

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


int wolf_TCPclose(WOLF_SOCKET_T sock)
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

static err_t tcp_result(WOLF_SOCKET_T sock, int status)
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
    WOLF_SOCKET_T sock = (WOLF_SOCKET_T)arg;

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
    }

    return ERR_OK;
}

static err_t lwip_cb_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    WOLF_SOCKET_T sock = (WOLF_SOCKET_T)arg;

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
    return tcp_result(arg, 0);
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
    WOLF_SOCKET_T sock = (WOLF_SOCKET_T)arg;
    if (!p) {
        return ERR_OK;
    }

    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        const uint16_t buffer_left = BUF_SIZE - sock->buffer_len;
        sock->buffer_len += pbuf_copy_partial(p, sock->buffer + sock->buffer_len,
                                               p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

int wolf_inet_pton(int af, const char *ip_str, void *ip_dst)
{
    (void)af;
    struct sockaddr_in *addr = (struct sockaddr_in *)ip_dst;

    return ip4addr_aton(ip_str, ip_dst);
}

static u32_t swapBytes(u16_t a)
{
    u8_t *p = (u8_t *)&a;
    return p[0] << 8 | p[1];
}

int wolf_TCPconnect(WOLF_SOCKET_T sock, const struct sockaddr *addr, socklen_t addrlen)
{
    int err;
    struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;

    sock->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&addr_in->sin_addr));
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
    err = tcp_connect(sock->tcp_pcb, (const ip_addr_t *)&addr_in->sin_addr,
                      swapBytes(addr_in->sin_port), lwip_cb_client_connected);
    cyw43_arch_lwip_end();
    if (err != ERR_OK) {
        DEBUG_printf("wolf_TCPconnect: Failed");
        return WOLF_FAIL;
    }
    sock->connected = false;
    
    while (sock->connected != true)
    {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
    return WOLF_SUCCESS;
}

// get a new TCP client
WOLF_SOCKET_T wolf_TCPsocket()
{
    WOLF_SOCKET_T sock = calloc(1, sizeof(*sock));
    if (!sock) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return sock;
}

int wolf_TCPwrite(WOLF_SOCKET_T sock, const unsigned char *buff, long unsigned int len)
{
    int ret;
    int i;

    sock->sent_len = 0;
    ret = tcp_write(sock->tcp_pcb, buff, len, TCP_WRITE_FLAG_COPY);
 
    if (ret == ERR_OK) {
        tcp_output(sock->tcp_pcb);
    }
    sock->sent_len = 0;
    return (int)len;
}

int wolf_TCPread(WOLF_SOCKET_T sock, unsigned char *buff, long unsigned int len)
{
    int recv_len;
    int remained;
    int i;

    while (1) { /* no timeout for now */
        if(sock->buffer_len > 0) {
            recv_len = len <= sock->buffer_len ? len : sock->buffer_len;
            memcpy(buff, sock->buffer, recv_len);
            if(recv_len >= len) {
                remained = sock->buffer_len - recv_len;
                sock->buffer_len = remained;
                for(i=0; i<remained; i++)
                    sock->buffer[i] = sock->buffer[i+recv_len];
            } else
                sock->buffer_len = 0;
            return recv_len;
        } 
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(10));
    }
}