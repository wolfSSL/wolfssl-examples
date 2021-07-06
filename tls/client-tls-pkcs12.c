/* client-tls-pkcs12.c
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

/*
 * To test this example simply run against the default wolfSSL server:
 * : cd ~/path-to-wolfssl-root/
 * : ./examples/server/server
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
#include <wolfssl/wolfcrypt/pkcs12.h>

#define DEFAULT_PORT 11111

#define APP_ERR -999
#define ERR_NO_CA_IN_P12 -1000

int main(int argc, char** argv)
{
    int ret = 0;

    #if defined(OPENSSL_EXTRA) && !defined(NO_PWDBASED) && !defined(NO_ASN) \
        && !defined(NO_DES3)
    int                sockfd;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl = NULL; /* init to NULL, wolfSSL_free can account for it */

/*----------------------------------------------------------------------------*/
/* updates from the original example client-tls.c */
/*----------------------------------------------------------------------------*/
    byte buffer[5300];
    char file[] = "../certs/test-clientbundle.p12";
    char pass[] = "wolfSSL test";
    FILE *f;
    int firstLoop = 1;
    int root_ca_buf_size = -1, bytes;
    byte* root_ca_buf;

    WOLFSSL_BIO*         bio;
    WOLFSSL_EVP_PKEY*    pkey;
    WC_PKCS12*           pkcs12;
    WOLFSSL_X509*        cert;
    WOLFSSL_X509*        tmp;

    WOLF_STACK_OF(WOLFSSL_X509)* ca;
/*----------------------------------------------------------------------------*/
    (void) ret; /* ignore unused  */

    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        return 0;
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        return -1;
    }

    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        return -1;
    }

/*----------------------------------------------------------------------------*/
/* Extract cert, key, CA from PKCS12 file for use in mutual auth */
/*----------------------------------------------------------------------------*/
    f = fopen(file, "rb");
    if (!f) {
        printf("Error opening %s check the file\n", file);
        return APP_ERR;
    }

    bytes = (int)fread(buffer, 1, sizeof(buffer), f);
    printf("Read %d bytes from file\n", bytes);
    fclose(f);

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    bio = wolfSSL_BIO_new_mem_buf((void*)buffer, bytes);
    if (!bio) {
        printf("Failed to create bio\n");
        wolfSSL_CTX_free(ctx);
        wolfSSL_Cleanup();
        close(sockfd);
        return APP_ERR;
    }

    wolfSSL_d2i_PKCS12_bio(bio, &pkcs12);
    if (!pkcs12) {
        printf("Failed the d2i_PKCS12_bio call\n");
        wolfSSL_CTX_free(ctx);
        wolfSSL_Cleanup();
        wolfSSL_BIO_free(bio);
        close(sockfd);
        return APP_ERR;
    }

    /* check parse with extra certs kept */
    if ((ret = wolfSSL_PKCS12_parse(pkcs12, pass, &pkey, &cert, &ca))
                                                           != WOLFSSL_SUCCESS) {
        printf("Failed to parse the PKCS12 data. ret = %d\n", ret);
        ret = APP_ERR;
        goto client_example_end;
    }

    /* NOTE: these two calls are optional, only required for mutual auth */
    /*optional, load key that cooresponds to client cert */
    if ((ret = wolfSSL_CTX_use_PrivateKey(ctx, pkey)) != SSL_SUCCESS) {
        printf("Failed to load the privateKey. ret = %d\n", ret);
        ret = APP_ERR;
        goto client_example_end;
    }

    /* optional load a client cert */
    if ((ret = wolfSSL_CTX_use_certificate(ctx, cert)) != SSL_SUCCESS) {
        printf("Failed to load certificate. ret = %d\n", ret);
        ret = APP_ERR;
        goto client_example_end;
    }

    /* This step is REQUIRED to validate the peer being connected to */
    if (ca != NULL) {
        do {
            tmp = wolfSSL_sk_X509_pop(ca);
            if (tmp == NULL && firstLoop) {
                return ERR_NO_CA_IN_P12;
            } else if (tmp != NULL) {
                /* do something interesting with tmp, maybe it's an intermediate
                 * CA, maybe it's the root CA?
                 */
                ret = wolfSSL_X509_get_isCA(tmp);
                if (ret == 1) {
                    root_ca_buf = (byte*) wolfSSL_X509_get_der(tmp,
                                                             &root_ca_buf_size);

                    if ((ret = wolfSSL_CTX_load_verify_buffer(ctx, root_ca_buf,
                                                       root_ca_buf_size,
                                                       SSL_FILETYPE_ASN1)
                                                       ) != SSL_SUCCESS) {
                        printf("Failed to load CA. ret = %d\n", ret);
                        ret = APP_ERR;
                        goto client_example_end;
                    }

                    wolfSSL_X509_free(tmp);
                } else {
                    wolfSSL_X509_free(tmp);
                }
            }
            firstLoop = 0;
            /* Continue popping until no certs left or target cert is found */
        } while (tmp != NULL);
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto client_example_end;
    }

    /* Attach wolfSSL to the socket */
    wolfSSL_set_fd(ssl, sockfd);

    /* Connect to wolfSSL on the server side */
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        ret =  -1;
        goto client_example_end;
    }

    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto client_example_end;
    }
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    if (wolfSSL_write(ssl, buff, (int) len) != len) {
        fprintf(stderr, "ERROR: failed to write\n");
        ret = -1;
        goto client_example_end;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(ssl, buff, sizeof(buff)-1) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        ret = -1;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);

    ret = 0;

client_example_end:
    /* Cleanup and return */
    wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
    wolfSSL_EVP_PKEY_free(pkey);
    wolfSSL_X509_free(cert);
    wc_PKCS12_free(pkcs12);
    wolfSSL_BIO_free(bio);
    wolfSSL_sk_X509_free(ca);
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
    close(sockfd);          /* Close the connection to the server       */
    #else
      #ifndef OPENSSL_EXTRA
        printf("wolfSSL not configured with --enable-opensslextra\n");
      #endif

      #ifdef NO_PWDBASED
        printf("wolfSSL not configured with --enable-pwdbased\n");
      #endif

      #ifdef NO_ASN
        printf("wolfSSL not configured with --enable-asn support\n");
      #endif

      #ifdef NO_DES3
        printf("wolfSSL not configured with --enable-des3\n");
      #endif

        printf("Please re-configure and re-install wolfSSL before using\n"
               "this example\n\n");
   #endif
    return ret;               /* Return reporting a success               */
}

