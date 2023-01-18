/* maxq10xx-wolfssl-client.c
 *
 * Based on client-tls-pkcallback.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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

/* This example shows how to write a simple TLS client that uses the features
 * of the Analog Devices MAXQ 1065 and 1080. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* wolfSSL */
#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#ifdef USE_WINDOWS_API
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#if (defined(WOLFSSL_MAXQ1065) || defined(WOLFSSL_MAXQ108X)) && \
    defined(HAVE_PK_CALLBACKS) && defined(WOLF_CRYPTO_CB) && \
    !defined(NO_PSK) && !defined(HAVE_EXTENDED_MASTER) && \
    defined(NO_WOLFSSL_SERVER)

/* -------------- */
/* Configurations */
/* -------------- */

/* Please define this if you want wolfSSL's debug output */
#define WANT_DEBUG

/* Please set the server's address and the port it listens on */
#define DEFAULT_SERVER "127.0.0.1"
#define DEFAULT_PORT   11111
#define MESSAGE        "Hello from MAXQ10xx!"
#define MESSAGE_LEN    20

/* ------------------------------------ */
/* No modifications required below here */
/* ------------------------------------ */

/* identity is OpenSSL testing default for openssl s_client, keep same */
static const char* kIdentityStr = "Client_identity";

static WC_INLINE unsigned int my_psk_client_cb(WOLFSSL* ssl, const char* hint,
        char* identity, unsigned int id_max_len, unsigned char* key,
        unsigned int key_max_len)
{
    (void)ssl;
    (void)hint;
    (void)key;
    (void)key_max_len;

    /* see internal.h MAX_PSK_ID_LEN for PSK identity limit */
    XSTRNCPY(identity, kIdentityStr, id_max_len);

    /* For TLS 1.2, we indicate that MAXQ has the PSK. */
    return USE_HW_PSK;
}

#ifdef WOLFSSL_TLS13
static WC_INLINE unsigned int my_psk_client_cs_cb(WOLFSSL* ssl,
        const char* hint, char* identity, unsigned int id_max_len,
        unsigned char* key, unsigned int key_max_len, const char* ciphersuite)
{
    (void)ssl;
    (void)hint;
    (void)key;
    (void)key_max_len;

#ifdef WOLFSSL_PSK_MULTI_ID_PER_CS
    /* Multiple calls for each cipher suite. First identity byte indicates the
     * number of identites seen so far for cipher suite. */
    if (identity[0] != 0) {
        return 0;
    }
#endif

    /* see internal.h MAX_PSK_ID_LEN for PSK identity limit */
    XSTRNCPY(identity, kIdentityStr, id_max_len);
    XSTRNCAT(identity, ciphersuite + XSTRLEN(ciphersuite) - 6, id_max_len);

    /* For TLS 1.3, we just return an unmodified key. */
    return 32;
}
#endif /* WOLFSSL_TLS13 */

static void print_usage() {
    fprintf(stderr, "usage: ./maxq10xx-wolfssl-client [-tls12|-tls13] "
                    "[-ecc|-rsa|-psk] <cipher suite>\n");
    fprintf(stderr, "Use `example/client/client -e` to get a list of cipher "
                    "suites.\n");
    fprintf(stderr, "Specifying a cipher suite is Optional.\n");
}

#define TLS12 1
#define TLS13 2

#define ECC 1
#define RSA 2
#define PSK 3

static int tls_version = TLS12;
static int alg = ECC;
static char *key_file = NULL;
static char *ciphersuite = NULL;

static int cmd_line_parse(int argc, char** argv) {
    const size_t tls_vers_len = 6;
    const size_t alg_len = 4;

    if ((argc != 3) && (argc != 4)) {
        print_usage();
        return -1;
    }

    if (strncmp("-tls12", argv[1], tls_vers_len) == 0) {
        tls_version = TLS12;
    }
    else if (strncmp("-tls13", argv[1], tls_vers_len) == 0) {
#ifdef WOLFSSL_TLS13
        tls_version = TLS13;
#else
        fprintf(stderr, "TLS 1.3 is not supported on MAXQ1065.\n");
        return -1;
#endif
    }
    else {
        print_usage();
        return -1;
    }

    if (strncmp("-ecc", argv[2], alg_len) == 0) {
        alg = ECC;
        key_file = "ecc-p256-pub.pem";
    }
    else if (strncmp("-rsa", argv[2], alg_len) == 0) {
        alg = RSA;
        key_file = "rsa-2048-pub.pem";
    }
    else if (strncmp("-psk", argv[2], alg_len) == 0) {
        alg = PSK;
    }
    else {
        print_usage();
        return -1;
    }

    if (argc == 4) {
        ciphersuite = argv[3];
    }

    return 0;
}

int main(int argc, char** argv)
{
    int                ret, err;
    #ifdef USE_WINDOWS_API
    SOCKET             sockfd = SOCKET_INVALID;
    #else
    int                sockfd = SOCKET_INVALID;
    #endif
    struct sockaddr_in servAddr;
    char               buff[256];

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    /* Command-line parameter check */
    ret = cmd_line_parse(argc, argv);
    if (ret < 0) {
        goto exit;
    }

#ifdef USE_WINDOWS_API
    StartTCP();
#endif

    /* Create a socket that uses an internet IPv4 address.
     * Sets the socket to be stream based (TCP).
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

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, DEFAULT_SERVER, &servAddr.sin_addr) != 1) {
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

    /*---------------------------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------------------------*/
#ifdef WANT_DEBUG
    wolfSSL_Debugging_ON();
#endif

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto exit;
    }

    /* Create and initialize WOLFSSL_CTX */
#ifdef WOLFSSL_TLS13
    if (tls_version == TLS13)
        ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
    else
#endif /* WOLFSSL_TLS13 */
        ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());

    if (ctx == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* At this point you would normally register a CA certificate, however, it
     * resides in MAXQ10xx and has already been registered. */

    /* Load the dummy private key; actually a public key. The actual private
     * key resides in MAXQ 10xx. */
    if (key_file != NULL) {
        if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, key_file,
                                    WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load %s, please check the "
                    "file.\n", key_file);
            goto exit;
        }
    }

    /* If specified, set the ciphersuite. */
    if (ciphersuite != NULL) {
        if (wolfSSL_CTX_set_cipher_list(ctx, ciphersuite) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "Invalid cipher suite.\n");
            print_usage();
            goto exit;
        }
    }

    if (alg == PSK) {
        wolfSSL_CTX_set_psk_client_callback(ctx, my_psk_client_cb);
#ifdef WOLFSSL_TLS13
        if (tls_version == TLS13) {
            wolfSSL_CTX_set_psk_client_cs_callback(ctx, my_psk_client_cs_cb);
        }
#endif /* WOLFSSL_TLS13 */

        if (ciphersuite != NULL) {
            wolfSSL_CTX_set_psk_callback_ctx(ctx, (void*)ciphersuite);
        }
    }

    /* Validate peer certificate */
    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, NULL);

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto exit;
    }

#ifdef WOLFSSL_TLS13
    if (alg == RSA) {
        if (wolfSSL_UseKeyShare(ssl, WOLFSSL_FFDHE_2048) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1;
            goto exit;
        }
    }
#endif /* WOLFSSL_TLS13 */

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto exit;
    }

    /* Connect to wolfSSL on the server side */
    do {
        ret = wolfSSL_connect(ssl);
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WC_PENDING_E);
    if (ret != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        goto exit;
    }

    /* Send the message to the server */
    if ((ret = wolfSSL_write(ssl, MESSAGE, MESSAGE_LEN)) != MESSAGE_LEN) {
        fprintf(stderr, "ERROR: failed to write entire message\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, MESSAGE_LEN);
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

    ret = 0; /* success */

exit:
    /* Cleanup and return */
    if (sockfd != SOCKET_INVALID)
        close(sockfd);
    if (ssl != NULL)
        wolfSSL_free(ssl);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);

    wolfSSL_Cleanup();

    return ret;
}

#else

int main(int argc, char** argv)
{
    printf("Warning: Required flags have not been used!\n"
           "Please configure with the following flags:\n"
           "    --enable-pkcallbacks\n"
           "    --enable-cryptocb\n"
           "    --disable-extended-master\n"
           "    --enable-psk\n"
           "    --enable-aesccm\n"
           "    --with-maxq10xx=MAXQ108x|MAXQ1065\n"
           "    CFLAGS=-DNO_WOLFSSL_SERVER\n"
);
    return -1;
}
#endif /* (WOLFSSL_MAXQ1065 || WOLFSSL_MAXQ108X) && HAVE_PK_CALLBACKS &&
        * WOLF_CRYPTO_CB && !NO_PSK && !HAVE_EXTENDED_MASTER &&
        * NO_WOLFSSL_SERVER */
