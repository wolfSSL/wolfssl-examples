/* client-tls.c
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

#define SERVER_IPV4_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 11111

#define CA_CERT_FILE "../certs/ca-cert.pem"
#define CERT_FILE    "../certs/client-cert.pem"
#define KEY_FILE     "../certs/client-key.pem"

#define CERT_BUFFER_SZ 2048

enum {
    VERIFY_OVERRIDE_ERROR,
    VERIFY_FORCE_FAIL,
    VERIFY_USE_PREVERIFY,
    VERIFY_DEFAULT,
    VERIFY_OVERRIDE_DATE_ERR
};

static THREAD_LS_T int myVerifyAction = VERIFY_DEFAULT;

static WC_INLINE int myVerify(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    char buffer[WOLFSSL_MAX_ERROR_SZ];
    WOLFSSL_X509* peer;
    char issuerBuffer[CERT_BUFFER_SZ];
    char subjectBuffer[CERT_BUFFER_SZ];

    fprintf(stderr, "In verification callback, error = %d, %s\n", store->error,
                                wolfSSL_ERR_error_string(store->error, buffer));
    peer = store->current_cert;
    if (peer) {
        wolfSSL_X509_NAME_oneline(
            wolfSSL_X509_get_issuer_name(peer), issuerBuffer, CERT_BUFFER_SZ);
        wolfSSL_X509_NAME_oneline(
            wolfSSL_X509_get_subject_name(peer), subjectBuffer, CERT_BUFFER_SZ);
        printf("\tPeer's cert info:\n issuer : %s\n subject: %s\n",
                                                issuerBuffer, subjectBuffer);
    }
    else
        fprintf(stderr, "\tPeer has no cert!\n");

    printf("\tSubject's domain name at %d is %s\n",
            store->error_depth, store->domain);

    switch (myVerifyAction) {
        case VERIFY_OVERRIDE_ERROR:
            return 1;

        case VERIFY_FORCE_FAIL:
            return 0;

        case VERIFY_DEFAULT:
        case VERIFY_USE_PREVERIFY:
            return preverify;
        
        case VERIFY_OVERRIDE_DATE_ERR:
            if (store->error == ASN_BEFORE_DATE_E
                    || store->error == ASN_AFTER_DATE_E)
                return 1;
            return 0;
    }
    fprintf(stderr, "Invalid verify action.\n");
    exit(1);
}

int switchPeerAuthMode(char *mode)
{
    if (XSTRCMP("NONE", mode) == 0) {
        return SSL_VERIFY_NONE;
    }
    else if (XSTRCMP("PEER", mode) == 0) {
        return SSL_VERIFY_PEER;
    }
    else if (XSTRCMP("FAIL_IN_NO_PEER_CERT", mode) == 0) {
        return SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }
    else if (XSTRCMP("FAIL_EXCEPT_PSK", mode) == 0) {
        return SSL_VERIFY_FAIL_EXCEPT_PSK;
    }
    fprintf(stderr, "Invalid peer auth mode.\n");
    exit(1);
}

int switchMyVerifyAction(char *mode)
{
    if (XSTRCMP("OVERRIDE_ERROR", mode) == 0) {
        return VERIFY_OVERRIDE_ERROR;
    }
    else if (XSTRCMP("FORCE_FAIL", mode) == 0) {
        return VERIFY_FORCE_FAIL;
    }
    else if (XSTRCMP("USE_PREVERIFY", mode) == 0) {
        return VERIFY_USE_PREVERIFY;
    }
    else if (XSTRCMP("OVERRIDE_DATE_ERR", mode) == 0) {
        return VERIFY_OVERRIDE_DATE_ERR;
    }
    fprintf(stderr, "Invalid verify action.\n");
    exit(1);
}

int main(int argc, char** argv)
{
    int                sockfd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;
    int                ret;
    int                diff = 0;
    int                peerAuthMode = WOLFSSL_VERIFY_DEFAULT;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    /* Check options */
    if (argc >= 3) {
        if (XSTRCMP("-a", argv[1]) == 0) {
            peerAuthMode = switchPeerAuthMode(argv[2]);
            diff += 2;
        }
        if (XSTRCMP("-m", argv[1]) == 0) {
            myVerifyAction = switchMyVerifyAction(argv[2]);
            diff += 2;
        }
    }
    if (argc >= 5) {
        if (XSTRCMP("-a", argv[3]) == 0) {
            peerAuthMode = switchPeerAuthMode(argv[4]);
            diff += 2;
        }
        if (XSTRCMP("-m", argv[3]) == 0) {
            myVerifyAction = switchMyVerifyAction(argv[4]);
            diff += 2;
        }
    }

    /* Set specify cert/key files */
    if (argc-diff != 1) {
        printf("usage:\n"
                    "\t./client-tls-peerauth -a <Peer auth mode>\n"
                    "\t./client-tls-peerauth -m <Verify mode>\n"
                    "\t./client-tls-peerauth \\\n"
                        "\t\t-a <Peer auth mode> -m <Verify mode>\n"
                "\n"
                "Peer auth mode:\n"
                    "\tNONE, PEER,\n"
                    "\tFAIL_IF_NO_PEER_CERT,\n"
                    "\tFAIL_EXCEPT_PSK\n"
                "\n"
                "Verify mode:\n"
                    "\tOVERRIDE_ERROR,\n"
                    "\tFORCE_FAIL,\n"
                    "\tUSE_PREVERIFY,\n"
                    "\tOVERRIDE_DATE_ERR\n");
        return 0;
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */

    /* Get the server IPv4 address from define */
    if (inet_pton(AF_INET, SERVER_IPV4_ADDRESS, &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        ret = -1;
        goto exit;
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
    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto exit;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* If peer auth is enabled, Load CA certificates into WOLFSSL_CTX */
    if (peerAuthMode != WOLFSSL_VERIFY_NONE) {
        if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CA_CERT_FILE, NULL))
            != SSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load %s, "
                            "please check the file.\n", CA_CERT_FILE);
            goto exit;
        }
    }

    /* Set peer auth mode */
    if (peerAuthMode != WOLFSSL_VERIFY_DEFAULT) {
        wolfSSL_CTX_set_verify(ctx, peerAuthMode, myVerify);
    }

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE,
        SSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto exit;
    }

    /* Load client key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        goto exit;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto exit;
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto exit;
    }

    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(ssl)) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        goto exit;
    }

    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto exit;
    }
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to write entire message\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto exit;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto exit;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);

    /* Bidirectional shutdown */
    while (wolfSSL_shutdown(ssl) == SSL_SHUTDOWN_NOT_DONE) {
        printf("Shutdown not complete\n");
    }

    printf("Shutdown complete\n");

    ret = 0;

exit:
    /* Cleanup and return */
    if (ssl)
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);          /* Close the connection to the server   */
    if (ctx)
        wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object      */
    wolfSSL_Cleanup();          /* Cleanup the wolfSSL environment      */

    return ret;                 /* Return reporting a success           */
}
