/* server-tls-threaded.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

/* threads */
#include <pthread.h>

#define DEFAULT_PORT 11111

#define MAX_CONCURRENT_THREADS 10

#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"



/* Thread argument package */
struct targ_pkg {
    int          open;
    pthread_t    tid;
    int          num;
    int          connd;
    WOLFSSL_CTX* ctx;
    int*         shutdown;
};



void* ClientHandler(void* args)
{
    struct targ_pkg* pkg = args;
    WOLFSSL*         ssl;
    char             buff[256];
    size_t           len;
    int              ret;
    const char*        reply = "I hear ya fa shizzle!\n";


    printf("thread %d open for business\n", pkg->num);

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(pkg->ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        pkg->open = 1;
        pthread_exit(NULL);
    }

    /* Attach wolfSSL to the socket */
    printf("Handling thread %d on port %d\n", pkg->num, pkg->connd);
    wolfSSL_set_fd(ssl, pkg->connd);

    /* Establish TLS connection */
    do {
        ret = wolfSSL_accept(ssl);
    } while(wolfSSL_want_read(ssl));

    if (ret != SSL_SUCCESS) {
        printf("ret = %d\n", ret);
        fprintf(stderr, "wolfSSL_accept error = %d\n",
            wolfSSL_get_error(ssl, ret));
        pkg->open = 1;
        pthread_exit(NULL);
    }

    printf("Client %d connected successfully\n", pkg->num);

    /* Read the client data into our buff array */
    XMEMSET(buff, 0, sizeof(buff));
    do {
        ret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
        /* TODO: Currently this thread can get stuck infinitely if client
         *       disconnects, add timer to abort on a timeout eventually,
         *       just an example for now so allow for possible stuck condition
         */
    } while(wolfSSL_want_read(ssl));

    if (ret > 0) {
        /* Print to stdout any data the client sends */
        printf("Client %d said: %s\n", pkg->num, buff);
    } else {
        printf("wolfSSL_read encountered an error with code %d and msg %s\n",
               ret, wolfSSL_ERR_error_string(ret, buff));
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        ssl = NULL;
        close(pkg->connd);      /* Close the connection to the server   */
        pkg->open = 1;          /* Indicate that execution is over      */
        pthread_exit(NULL);     /* End thread execution                */
    }

    /* Check for server shutdown command */
    if (XSTRNCMP(buff, "shutdown", 8) == 0) {
        printf("Shutdown command issued!\n");
        *pkg->shutdown = 1;
    }

    /* Write our reply into buff */
    XMEMSET(buff, 0, sizeof(buff));
    len = XSTRLEN(reply);
    XMEMCPY(buff, reply, len);

    /* Reply back to the client */
    do {
        ret = wolfSSL_write(ssl, buff, len);
        /* TODO: Currently this thread can get stuck infinitely if client
         *       disconnects, add timer to abort on a timeout eventually,
         *       just an example for now so allow for possible stuck condition
         */
    } while (wolfSSL_want_write(ssl));

    if (ret != len) {
        printf("wolfSSL_write encountered an error with code %d and msg %s\n",
               ret, wolfSSL_ERR_error_string(ret, buff));
    }

    /* Cleanup after this connection */
    wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    ssl = NULL;
    close(pkg->connd);      /* Close the connection to the server   */
    pkg->open = 1;          /* Indicate that execution is over      */
#if defined(HAVE_ECC) && defined(FP_ECC)
    wc_ecc_fp_free();  /* free per thread cache */
#endif
    pthread_exit(NULL);     /* End thread execution                */
}



int main()
{
    int                ret; 
    int                sockfd = SOCKET_INVALID;
    int                connd;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    int                shutdown = 0;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;

    /* declare thread variable */
    struct targ_pkg thread[MAX_CONCURRENT_THREADS];
    int             i;



    /* Initialize wolfSSL */
    wolfSSL_Init();
    wolfSSL_Debugging_ON();


    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1; 
        goto exit;
    }

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
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
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



    /* Initialize thread array */
    for (i = 0; i < MAX_CONCURRENT_THREADS; ++i) {
        printf("Creating %d thread\n", i);
        thread[i].open = 1;
        thread[i].num = i;
        thread[i].ctx = ctx;
        thread[i].shutdown = &shutdown;
    }



    printf("Now open for connections\n");

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        /* find an open thread or continue if there is none */
        for (i = 0; i < MAX_CONCURRENT_THREADS && !thread[i].open; ++i);
        if (i == MAX_CONCURRENT_THREADS) {
            continue;
        }

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            continue;
        }



        /* Fill out the relevant thread argument package information */
        thread[i].open = 0;
        thread[i].connd = connd;

        /* Launch a thread to deal with the new client */
        pthread_create(&thread[i].tid, NULL, ClientHandler, &thread[i]);

        /* State that we won't be joining this thread */
        pthread_detach(thread[i].tid);
    }



    /* Suspend shutdown until all threads are closed */
    do {
        shutdown = 1;

        for (i = 0; i < MAX_CONCURRENT_THREADS; ++i) {
            if (!thread[i].open) {
                shutdown = 0;
            }
        }
    } while (!shutdown);

    printf("Shutdown complete\n");

    ret = 0;

exit:
    /* Cleanup and return */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the socket listening for clients   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */

    return ret;               /* Return reporting a success               */

}
