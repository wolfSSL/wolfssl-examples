/* client-tls-session.c
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
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

#define CERT_FILE           "../certs/client-cert.pem"
#define KEY_FILE            "../certs/client-key.pem"
#define CA_CERT_FILE        "../certs/ca-cert.pem"
#define LOCALHOST           "127.0.0.1"
#define DEFAULT_PORT        11111
#define SAVED_SESS          "session.bin"

#define MSG_SIZE            256

/* Print SSL error message */
static void print_SSL_error(const char* msg, SSL* ssl)
{
    int err;
    
    if (ssl != NULL) {
        err = wolfSSL_get_error(ssl, 0);
        fprintf(stderr, "ERROR: %s (err %d, %s)\n", msg, err,
                        ERR_error_string(err, NULL));
    }
    else {
        fprintf(stderr, "ERROR: %s \n", msg);
    }
}

/* write a session to the file */
static int write_SESS(WOLFSSL_SESSION* sess, const char* file)
{
    FILE*              fp = NULL;
    unsigned char*     buff = NULL;
    size_t             sz;
    int                ret = WOLFSSL_FAILURE;

    if ((fp = fopen(file, "wb")) == NULL) {
        fprintf(stderr, "ERROR : file %s does't exists\n", file);
        goto cleanup;
    }

    if ((sz = wolfSSL_i2d_SSL_SESSION(sess, &buff)) <= 0){
        print_SSL_error("wolfSSL_i2d_SSL_SESSION", NULL);
        goto cleanup;
    }
    
    if ((fwrite(buff, 1, sz, fp)) != sz) {
        fprintf(stderr, "ERROR : failed fwrite\n");
        goto cleanup;
    }

cleanup:
    if (fp)
        fclose(fp);
    if (buff)
        free(buff);
    
    return ret;
}

int main(int argc, char **argv)
{
    struct sockaddr_in servAddr;
    int                sockfd = -1;
    char               *ipadd = LOCALHOST;
    char               *ca_cert = CA_CERT_FILE;
    int                port = DEFAULT_PORT;
    static const char  kHttpGetMsg[] = "GET /index.html HTTP/1.0\r\n\r\n";
    struct hostent     *host;

    char               msg[MSG_SIZE];
    int                ret = WOLFSSL_FAILURE;
    
    (void)ipadd;

    /* SSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    memset(&servAddr, 0, sizeof(servAddr));

    /* SSL SESSION object */
    WOLFSSL_SESSION* session= NULL;
    
    /* Check for proper calling convention */
    if (argc == 1) 
        fprintf(stderr, "Send to localhost(%s)\n", LOCALHOST);
    if (argc >=2) {
        host = gethostbyname(argv[1]);
        memcpy(&servAddr.sin_addr, host->h_addr_list[0], host->h_length);
    }
    if (argc >= 3)  
        ca_cert = argv[2];
    if (argc == 4) 
        port = atoi(argv[3]);
    if (argc >= 5) {
        fprintf(stderr, "ERROR: Too many arguments.\n");
        goto cleanup;
    }

    /* Initialize library */
    if (wolfSSL_library_init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to initialize the library\n");
        goto cleanup;
    }
   
    /* Create and initialize an SSL context object*/
    if ((ctx = wolfSSL_CTX_new(SSLv23_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create an SSL context object\n");
        goto cleanup;
    }

    /* Load client certificate into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, 
        WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto cleanup;
    }

    /* Load client key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, 
        WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        goto cleanup;
    }

    /* Load CA certificate to the context */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, ca_cert, NULL))
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s \n", ca_cert);
        goto cleanup;
    }

   /* 
    * Set up a TCP Socket and connect to the server 
    */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create a socket. errno %d\n", errno);
        goto cleanup;
    }
    
    servAddr.sin_family = AF_INET;           /* using IPv4      */
    servAddr.sin_port = htons(port);         /* on DEFAULT_PORT */
    
    if ((ret = connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)))
            == -1) {
        fprintf(stderr, "ERROR: failed to connect. errno %d\n", errno);
        goto cleanup;
    }

    /* Create an SSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create an SSL object\n");
        goto cleanup;
    }

    /* Attach the socket to the SSL */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto cleanup;
    }
    /* SSL connect to the server */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        print_SSL_error("failed SSL connect", ssl);
        goto cleanup;
    }

   /* 
    * Application messaging
    */
    while (1) {
        printf("Message to send: ");
        if(fgets(msg, sizeof(msg), stdin) <= 0)
            break;
        if (strcmp(msg, "\n") == 0){ /* if empty send HTTP request */
            strncpy(msg, kHttpGetMsg, sizeof(msg));
        } else
            msg[strnlen(msg, sizeof(msg)) - 1] = '\0';
        /* send a message to the server */
        if ((ret = wolfSSL_write(ssl, msg, strnlen(msg, sizeof(msg)))) < 0)
        {
        print_SSL_error("failed SSL write", ssl);
        break;
        }

        /* 
         * closing the session, and write session information into a file
         * before writing session information
         */  
        if (strcmp(msg, "break") == 0) {
            session = wolfSSL_get_session(ssl);
            ret = write_SESS(session, SAVED_SESS);
            break;
        }

        /* receive a message from the server */
        if ((ret = wolfSSL_read(ssl, msg, sizeof(msg) - 1)) < 0) {
            print_SSL_error("failed SSL read", ssl);
            break;
        }
        msg[ret] = '\0';
        printf("Received: %s\n", msg);
    }

/*  Cleanup and return */
cleanup:
    if (session != NULL) {
        wolfSSL_SESSION_free(session);
    }
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (sockfd != -1)
        close(sockfd);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);
    if (ret != WOLFSSL_SUCCESS)
        ret = WOLFSSL_FAILURE;
    printf("End of TLS Client\n");
    return ret;
}

