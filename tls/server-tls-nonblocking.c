/* server-tls-nonblocking.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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
#include <errno.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>

#define DEFAULT_PORT 11111
#define SELECT_WAIT_mSEC 1000

#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"

typedef enum {
    SV_IDLE,     /* Free to use */
    SV_BEGIN,    /* ready to begin */
    SV_ACCEPT,
    SV_SSL_ACCEPT,
    SV_SSL_READ,
    SV_SSL_WRITE,
    SV_SSL_SHUTDOWN
} SV_STATUS;

typedef enum {
    NB_CONTINUE,
    NB_COMPLETE,
    NB_FAITAL,
} NV_RETURN;

typedef struct {
    /* core context */
    SV_STATUS stat;
    int sock;

    /* application context */
    int sockfd;
    int connd;
    WOLFSSL_CTX *ctx;
    WOLFSSL     *ssl;
} SV_CTX;

#define MAX_CONTEXT 10
static SV_CTX sv_tbl[MAX_CONTEXT];

static void sv_init()
{
    int i;
    for(i = 0; i < MAX_CONTEXT; i++) {
            sv_tbl[i].stat = SV_IDLE;
    }
}

static void sv_sock(SV_CTX *current, int sock)
{
    current->sock = sock;
}

static SV_CTX * sv_fork(SV_CTX *current, int sock)
{
    int i;
    for (i = 0; i < MAX_CONTEXT; i++) {
        if (sv_tbl[i].stat == SV_IDLE) {
            sv_tbl[i] = *current;
            sv_tbl[i].sock = sock;
            #ifdef SV_VERBOUSE
            printf("sv_fork: %lx, sock = %d\n", (unsigned long)&sv_tbl[i], sock);
            #endif
            return &sv_tbl[i];
        }
    }
    #ifdef SV_VERBOUSE
    printf("sv_fork: NONE\n");
    #endif
    return NULL;
}

static void sv_new()
{
    int i;
    for (i = 0; i < MAX_CONTEXT; i++) {
        if (sv_tbl[i].stat == SV_IDLE) {
            sv_tbl[i].stat = SV_BEGIN;
            sv_tbl[i].sock = -1;
            #ifdef SV_VERBOUSE
            printf("sv_new: %d, %lx\n", i, (unsigned long)&sv_tbl[i]);
            #endif
            return;
        }
    }
    #ifdef SV_VERBOUSE
    printf("sv_new: NONE\n");
    #endif
    return;
}

static void sv_free(SV_CTX *sv_ctx)
{
    #ifdef SV_VERBOUSE
    printf("sv_free: %lx\n", (unsigned long)sv_ctx);
    #endif
    sv_ctx->stat = SV_IDLE;
}

#define WAIT_mSEC 1000

static int scan = 0;

static SV_CTX *sv_poll() {
    int i;
    int nfd = 0;
    struct pollfd fds[MAX_CONTEXT];
    SV_CTX *next = NULL;

    /* Scan context table */
    for (i = 0; i < MAX_CONTEXT; i++) {
        if (sv_tbl[i].stat >= SV_BEGIN && sv_tbl[i].sock != SOCKET_INVALID) {
            fds[nfd].events = POLLIN | POLLOUT;
            fds[nfd].fd = sv_tbl[i].sock;
            nfd++;
        }
    }

    poll(fds, nfd, WAIT_mSEC);

    for (i = 0; i < MAX_CONTEXT; i++) {
        if(sv_tbl[scan].stat != SV_IDLE) {
            next = &sv_tbl[scan];
            scan = (scan + 1) % MAX_CONTEXT;
            break;
        }
        scan = (scan + 1) % MAX_CONTEXT;
    }
    return next;
}


int sv_main(SV_CTX *sv_ctx)
{
    int ret = NB_FAITAL; 
    int err;

    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    int                shutdown = 0;
    const char*        reply = "I hear ya fa shizzle!\n";

    if(sv_ctx == NULL)
        return NB_CONTINUE;

switch(sv_ctx->stat) {
case SV_BEGIN:

    /* Initialize application context. Note they are with sv_ctx */
    #define sockfd sv_ctx->sockfd
    #define connd sv_ctx->connd
    #define ctx sv_ctx->ctx
    #define ssl sv_ctx->ssl

    sockfd = SOCKET_INVALID;
    connd  = SOCKET_INVALID;
    ctx = NULL;
    ssl = NULL;
    
    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }

    sv_sock(sv_ctx, sockfd); /* attach socket to the server context */

    /* Set the socket options to use nonblocking I/O */
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
        fprintf(stderr, "ERROR: failed to set socket options\n");
        ret = -1;
        goto exit; 
    }



    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* Load server certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto exit;
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        ret = -1;
        goto exit;        
    }



    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */



    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto exit;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        printf("Waiting for a connection...\n");
        sv_ctx->stat = SV_ACCEPT;
        FALL_THROUGH;

case SV_ACCEPT:
        /* Accept client connections */
        if((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
               == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || 
                errno == EINPROGRESS || errno == EALREADY) {
                return NB_CONTINUE;
            }
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1;
            goto exit;
        }

        if((sv_ctx = sv_fork(sv_ctx, connd)) == NULL) {
            fprintf(stderr, "ERROR: failed to folk the context\n");
            ret = -1;
            goto exit;
        }

        /* Set the socket options to use nonblocking I/O */
        if (fcntl(connd, F_SETFL, O_NONBLOCK) == -1) {
            fprintf(stderr, "ERROR: failed to set socket options\n");
            ret = -1;
            goto exit;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1;
            goto exit;
        }

        /* Attach wolfSSL to the socket */
        if ((ret = wolfSSL_set_fd(ssl, connd)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
            goto exit;
        }
        
        /* Establish TLS connection */
        printf("wolfSSL_accepting\n");

        sv_ctx->stat = SV_SSL_ACCEPT;
        FALL_THROUGH;
        
case SV_SSL_ACCEPT:
        ret = wolfSSL_accept(ssl);
        err = wolfSSL_get_error(ssl, ret);
        if (ret != WOLFSSL_SUCCESS && 
            (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE))
            return NB_CONTINUE;
        else if (ret != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_accept error %d (%d)\n", err, ret);
            goto exit;
        }
        printf("client connected successfully\n");

        sv_ctx->stat = SV_SSL_READ;
        FALL_THROUGH;

case SV_SSL_READ:

        /* read the client data into our buff array */
        memset(buff, 0, sizeof(buff));

        ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
        err = wolfSSL_get_error(ssl, ret);
        if (ret < 0 && err == WOLFSSL_ERROR_WANT_READ)
            return NB_CONTINUE;
        else if (ret < 0) {
            fprintf(stderr, "ERROR %d: failed to read\n", ret);
            goto exit;
        }

        /* Print to stdout any data the client sends */
        printf("Client: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }
        sv_ctx->stat = SV_SSL_WRITE;
        FALL_THROUGH;



case SV_SSL_WRITE:
        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        /* Reply back to the client */
        ret = wolfSSL_write(ssl, reply, len);
        err = wolfSSL_get_error(ssl, ret);
        if (ret < 0 && err == WOLFSSL_ERROR_WANT_WRITE)
            return NB_CONTINUE;
        else if (ret < 0) {
            fprintf(stderr, "ERROR %d: failed to write\n", ret);
            goto exit;
        }

        sv_ctx->stat = SV_SSL_SHUTDOWN;
        FALL_THROUGH;

case SV_SSL_SHUTDOWN:
        /* send close notify */
        ret = wolfSSL_shutdown(ssl);
        err = wolfSSL_get_error(ssl, 0);
        if (ret != WOLFSSL_SUCCESS && err == WOLFSSL_ERROR_WANT_READ)
            return NB_CONTINUE;

        /* Cleanup after this connection */
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        close(connd);           /* Close the connection to the client   */
                                /* Parent context keeps sockfd          */
        connd = SOCKET_INVALID;

        sv_ctx->stat = SV_IDLE;
        FALL_THROUGH;

case SV_IDLE:
        return NB_CONTINUE;

    }

    printf("Shutdown complete\n");
} /* End of switch(sv_ctx->stat) */

ret = NB_COMPLETE;

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (connd != SOCKET_INVALID)
        close(connd);           /* Close the connection to the client   */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket listening for clients   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */

    return ret;
}


int main(int argc, char ** argv)
{
    SV_CTX *sv_ctx;

    /* Initialize wolfSSL */
    wolfSSL_Init();
    // wolfSSL_Debugging_ON();


    sv_init();
    sv_new(); /* get a new context so that sv_poll can pick it up */

    /* Super Loop */
    while(1) {
        switch (sv_main(sv_ctx = sv_poll())) {
        case NB_CONTINUE:
            break;
        case NB_COMPLETE:
            sv_free(sv_ctx);
            sv_new();
            break;
        case NB_FAITAL:
            sv_free(sv_ctx);
            break;
        }
    }
    return 0;
}
