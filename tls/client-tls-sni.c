/* client-tls-sni.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

/* TLS v1.3 client demonstrating Server Name Indication (SNI, RFC 6066).
 *
 * The client tells the server which host name it wants in the ClientHello,
 * so a server hosting several names on one address can present the matching
 * certificate. The client then checks that the certificate it received is
 * valid for that same name.
 *
 * APIs shown:
 *   wolfSSL_UseSNI()            send a host name for this connection
 *                               (wolfSSL_CTX_UseSNI() does it for every
 *                               connection made from a context)
 *   wolfSSL_check_domain_name() require the server certificate to match
 *
 * Pair with server-tls-sni, which serves two names:
 *   example.com      RSA certificate  (../certs/server-cert.pem)
 *   www.wolfssl.com  ECC certificate  (../certs/server-ecc.pem)
 * Run without a host name to see how the server handles an absent SNI.
 *
 * Requires wolfSSL built with SNI, which is on by default.
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
#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define DEFAULT_PORT 11111

/* Both server certificates chain to different CAs, so load both */
#define CA_FILE_RSA "../certs/ca-cert.pem"
#define CA_FILE_ECC "../certs/ca-ecc-cert.pem"

#if defined(WOLFSSL_TLS13) && defined(HAVE_SNI)

int main(int argc, char** argv)
{
    int                ret = 0;
    int                sockfd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;
    const char*        host = (argc == 3) ? argv[2] : NULL;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    /* Check for proper calling convention */
    if (argc < 2 || argc > 3) {
        printf("usage: %s <IPv4 address> [host name]\n", argv[0]);
        printf("  host name: sent as SNI and required to match the server\n"
               "  certificate. Omit it to connect without SNI.\n");
        return 0;
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1; goto exit;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        ret = -1; goto exit;
    }

    /* Connect to the server */
    if ((ret = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)))
         == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto exit;
    }

    /*---------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------*/
#if 0
    wolfSSL_Debugging_ON();
#endif

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto exit;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1; goto exit;
    }

    /* Load the CA certificates for both server certificates */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE_RSA, NULL))
         != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CA_FILE_RSA);
        goto exit;
    }
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE_ECC, NULL))
         != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CA_FILE_ECC);
        goto exit;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1; goto exit;
    }

    if (host != NULL) {
        /* Ask for this host name in the ClientHello */
        if ((ret = wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, host,
                                  (unsigned short)strlen(host)))
                != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: wolfSSL_UseSNI failed\n");
            goto exit;
        }

        /* And insist that the certificate we get back is for that name.
         * Without this, any certificate the CA signed would be accepted. */
        if ((ret = wolfSSL_check_domain_name(ssl, host)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: wolfSSL_check_domain_name failed\n");
            goto exit;
        }
        printf("Connecting with SNI \"%s\"\n", host);
    }
    else {
        printf("Connecting without SNI\n");
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto exit;
    }

    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, ret);
        fprintf(stderr, "ERROR: failed to connect to wolfSSL: %d (%s)\n",
                err, wolfSSL_ERR_reason_error_string((unsigned long)err));
        goto exit;
    }

    printf("Connected with %s, cipher %s\n", wolfSSL_get_version(ssl),
           wolfSSL_get_cipher_name(ssl));

#ifdef KEEP_PEER_CERT
    /* Show which certificate the server chose for our SNI */
    {
        WOLFSSL_X509* peer = wolfSSL_get_peer_certificate(ssl);
        if (peer != NULL) {
            char subject[256];
            if (wolfSSL_X509_NAME_oneline(wolfSSL_X509_get_subject_name(peer),
                                          subject, sizeof(subject)) != NULL) {
                printf("Server certificate subject: %s\n", subject);
            }
            wolfSSL_X509_free(peer);
        }
    }
#endif

    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1; goto exit;
    }
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    if ((ret = wolfSSL_write(ssl, buff, (int)len)) != (int)len) {
        fprintf(stderr, "ERROR: failed to write entire message\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto exit;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) < 0) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto exit;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);

    /* Return reporting a success */
    ret = 0;

exit:
    /* Cleanup and return */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the connection to the server       */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment          */

    return ret;
}

#else

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    printf("Example requires TLS v1.3 and SNI (build wolfSSL with "
           "--enable-sni)\n");
    return 0;
}

#endif
