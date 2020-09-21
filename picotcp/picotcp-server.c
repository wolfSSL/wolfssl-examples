/* picotcp-server.c
 *
 * Copyright (C) 2020 wolfSSL Inc.
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
 */

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

/* PicoTCP */
#include <pico_stack.h>
#include <pico_socket.h>
#include <pico_dev_tap.h>

/* Pthreads */
#include <pthread.h>

/* Semaphore */
#include <semaphore.h>

#define DEFAULT_PORT 11111

#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"

/* Global picotcp client socket object */
static volatile struct pico_socket *conn = NULL;
sem_t picotcp_rx_data;

/* PicoTCP send/receive callbacks */
int pico_send(struct WOLFSSL *ssl, char *buf, int len, void *ctx)
{
    struct pico_socket *s = (struct pico_socket *)ctx;
    int r;
    r = pico_socket_write(s, buf, len);
    if (r > 0)
        return r;
    else
        return WOLFSSL_CBIO_ERR_WANT_WRITE;
}

int pico_recv(struct WOLFSSL *ssl, char *buf, int len, void *ctx)
{
    struct pico_socket *s = (struct pico_socket *)ctx;
    int r;
    r = pico_socket_read(s, buf, len);
    if (r > 0)
        return r;
    else
        return WOLFSSL_CBIO_ERR_WANT_READ;
}

/* PicoTCP: socket callback */
static void socket_cb(uint16_t ev, struct pico_socket *s)
{
    struct pico_ip4 client_addr;
    uint16_t client_port;
    if (ev & PICO_SOCK_EV_CONN) {
        conn = pico_socket_accept(s, &client_addr, &client_port);
        return;
    }
    if (ev & PICO_SOCK_EV_RD) {
        sem_post(&picotcp_rx_data);
    }
}

/* PicoTCP: tick thread */
static volatile int picotick_terminated = 0;
static void *picotick_thread (void *arg) {
    printf("PicoTCP thread: started.\n");
    while(1) {
        pico_stack_tick();
        if ((usleep(2000) > 0) || picotick_terminated) {
            fprintf(stderr, "picotick_thread: Goodbye!\n");
            pthread_exit(NULL);
        }
    }
}


int main()
{
    char               buff[256];
    size_t             len;
    int                shutdown = 0;
    int                ret;
    const char*        reply = "I hear ya fa shizzle!\n";
    const char*        tap_ipaddr = "10.0.0.1";
    const char*        tap_netmask = "255.255.255.0";
    struct pico_device *dev;
    struct pico_ip4 dev_tap_ipaddr, dev_tap_netmask, in_addr_any, client_addr;
    uint16_t listen_port = short_be(DEFAULT_PORT), client_port;
    struct pico_socket *pico_s;
    pthread_t pico_tick_thread;
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;

    /* Initialize picoTCP */
    pico_stack_init();

    dev = pico_tap_create("tap0");
    if (!dev) {
        fprintf(stderr, "ERROR: Failed to create picoTCP tap device: %s\n", strerror(errno));
        return -1;
    }

    /* Convert IP address + netmask from text to ip4 */
    pico_string_to_ipv4(tap_ipaddr, &dev_tap_ipaddr.addr);
    pico_string_to_ipv4(tap_netmask, &dev_tap_netmask.addr);
    memset(&in_addr_any, 0, sizeof(struct pico_ip4));

    /* Add ipv4 link to tap interface */
    pico_ipv4_link_add(dev, dev_tap_ipaddr, dev_tap_netmask);

    /* Initialize semaphore */
    sem_init(&picotcp_rx_data, 1, 0);

    /* Start picoTCP ticking thread */
    pthread_create(&pico_tick_thread, NULL, picotick_thread, NULL);

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Create a socket that uses an internet IPv4 address,
     * Set the socket to be stream based (TCP),
     * associate to picoTCP socket callback */
    pico_s = pico_socket_open(PICO_PROTO_IPV4, PICO_PROTO_TCP, &socket_cb);
    if (pico_s == NULL) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    /* Load server certificates into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        return -1;
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        return -1;
    }

    /* Associate picoTCP callbacks to wolfSSL context */
    wolfSSL_CTX_SetIORecv(ctx, pico_recv);
    wolfSSL_CTX_SetIOSend(ctx, pico_send);

    /* Bind the server socket to our port */
    if (pico_socket_bind(pico_s, &in_addr_any, &listen_port) != 0) {
        fprintf(stderr, "ERROR: failed to bind\n");
        return -1;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (pico_socket_listen(pico_s, 5) != 0) {
        fprintf(stderr, "ERROR: failed to listen\n");
        return -1;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        conn = pico_socket_accept(pico_s, &client_addr, &client_port);
        printf("Waiting for a connection...\n");
        while(!conn) {
            usleep(10000);
        }
        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            return -1;
        }
        /* Attach wolfSSL to the socket */

        /* Establish TLS connection */
        do {
            wolfSSL_SetIOReadCtx(ssl, conn);
            wolfSSL_SetIOWriteCtx(ssl, conn);
            ret = wolfSSL_accept(ssl);
            sem_wait(&picotcp_rx_data);
        } while ((ret != SSL_SUCCESS) && (wolfSSL_get_error(ssl, ret) == SSL_ERROR_WANT_READ));

        printf("Client connected successfully\n");
        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        sem_wait(&picotcp_rx_data);
        if (wolfSSL_read(ssl, buff, sizeof(buff)-1) == -1) {
            fprintf(stderr, "ERROR: failed to read\n");
            return -1;
        }
        /* Print to stdout any data the client sends */
        printf("Client: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }
        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        /* Reply back to the client */
        if (wolfSSL_write(ssl, buff, len) != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            return -1;
        }
        /* Cleanup after this connection */
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        pico_socket_close(conn);           /* Close the connection to the client   */
        conn = NULL;
    }
    printf("Shutdown complete\n");

    /* Cleanup and return */
    wolfSSL_CTX_free(ctx);     /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();         /* Cleanup the wolfSSL environment          */
    pico_socket_close(pico_s); /* Close the socket listening for clients   */
    picotick_terminated = 1;   /* Send termination to picotick thread      */
    pthread_join(pico_tick_thread, NULL); /* Join tick thread              */
    return 0;                  /* Return reporting a success               */
}
