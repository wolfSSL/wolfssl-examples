/* client-tls13-pinning.c
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

/* TLS v1.3 client that pins the server's public key.
 *
 * Normal certificate validation accepts any server certificate that chains to
 * a CA in the trust store. Certificate pinning adds a second check: the
 * application ships with the SHA-256 hash of the SubjectPublicKeyInfo (SPKI)
 * of the server it expects, and refuses the handshake if the server presents
 * any other key, even one signed by a trusted CA.
 *
 * The pin check runs inside the wolfSSL verify callback, so a mismatch aborts
 * the handshake before any application data is sent.
 *
 * wolfSSL must be built with WOLFSSL_ALWAYS_VERIFY_CB so that the verify
 * callback is invoked when chain validation succeeds, not only on failure.
 * --enable-opensslextra sets it; otherwise add CFLAGS="-DWOLFSSL_ALWAYS_VERIFY_CB".
 */

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
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define DEFAULT_PORT 11111

#define CERT_FILE "../certs/client-cert.pem"
#define KEY_FILE  "../certs/client-key.pem"
#define CA_FILE   "../certs/ca-cert.pem"

/* SHA-256 of the DER-encoded SubjectPublicKeyInfo in ../certs/server-cert.pem.
 * In your application this pin would be pre-computed from the cert or certs
 * you are wanting to pin
 */
#define DEFAULT_PIN_HEX \
    "5b3efef086a7b4637b2843d0a8fb5c5e7bf1e40b49e182184077f4b1405eddcd"

#if defined(WOLFSSL_TLS13) && defined(WOLFSSL_ALWAYS_VERIFY_CB) && \
    !defined(NO_SHA256) && LIBWOLFSSL_VERSION_HEX >= 0x05008002

/* Passed to the verify callback through wolfSSL_CTX_SetCertCbCtx(). */
typedef struct PinCtx {
    unsigned char pinSPKIhash[WC_SHA256_DIGEST_SIZE];  /* expected SPKI hash */
    unsigned char recvSPKIhash[WC_SHA256_DIGEST_SIZE];/* sent by server to us */
    int           nonMatchError; /* did error occur */
    int           matched; /* pinSPKIhash == recvSPKIhash */
} PinCtx;

static void PrintHex(FILE* fp, const unsigned char* buf, size_t sz)
{
    size_t i;
    for (i = 0; i < sz; i++) {
        fprintf(fp, "%02x", buf[i]);
    }
}

/* Decode exactly outSz bytes of hex. Returns 0 on success, -1 on bad input. */
static int HexToBytes(const char* hex, unsigned char* out, size_t outSz)
{
    size_t i;

    if (strlen(hex) != outSz * 2) {
        return -1;
    }
    for (i = 0; i < outSz; i++) {
        unsigned int v;
        if (sscanf(hex + (i * 2), "%2x", &v) != 1) {
            return -1;
        }
        out[i] = (unsigned char)v;
    }
    return 0;
}

/* Verify callback. wolfSSL calls it for the server's leaf certificate after
 * running its own chain validation (and, with WOLFSSL_VERIFY_CB_ALL_CERTS, for
 * each intermediate as well).
 *
 * preverify: 1 if wolfSSL accepted the certificate, 0 if it found an error
 *            (store->error holds the code).
 * Returns 1 to accept the certificate, 0 to reject it and abort the handshake.
 */
static int PinVerifyCallback(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    PinCtx* pinCtx = (PinCtx*)store->userCtx;
    int     ret = 0;

    /* Never override an error wolfSSL found. A pinned key on a certificate
     * that failed chain validation (expired, untrusted CA, ...) is still a
     * failure. */
    if (!preverify) {
        fprintf(stderr, "Certificate verification failed at depth %d: %s\n",
                store->error_depth,
                wolfSSL_ERR_reason_error_string(
                    (unsigned long)store->error));
        return 0;
    }

    /* Only the server's own certificate (depth 0) carries the pinned key.
     * Leave wolfSSL's result for intermediate CA certificates as it is. */
    if (store->error_depth != 0) {
        return preverify;
    }

    if (pinCtx == NULL || store->certs == NULL || store->totalCerts < 1) {
        fprintf(stderr, "Pin check: no peer certificate available\n");
        ret = BAD_FUNC_ARG;
    }

    if (ret == 0) {
        unsigned char* spki   = NULL;
        word32         spkiSz = 0;
        WOLFSSL_BUFFER_INFO certificateData = store->certs[0];

        /* First call sizes the SPKI. */
        ret = wc_GetSubjectPubKeyInfoDerFromCert(certificateData.buffer,
                certificateData.length, NULL, &spkiSz);
                                    /* ^^^^ NULL here means don't read anything
                                     * just size it into spkiSz */
        if (ret == 0) {
            spki = (unsigned char*)malloc(spkiSz);
            if (spki == NULL) {
                ret = MEMORY_E;
            }
        }
        if (ret == 0) {
            /* read SPKI in to spki */
            ret = wc_GetSubjectPubKeyInfoDerFromCert(certificateData.buffer,
                certificateData.length, spki, &spkiSz);
        }
        if (ret == 0) {
            /* hash the key */
            ret = wc_Sha256Hash(spki, spkiSz, pinCtx->recvSPKIhash);
        }

        free(spki);
    }

    if (ret != 0) {
        fprintf(stderr, "Pin check: failed to hash server public key: %s\n",
                wc_GetErrorString(ret));
        return 0;
    }
    /* made it here so any error after this is a Match Error */
    pinCtx->nonMatchError = 0;
    pinCtx->matched =
        (memcmp(pinCtx->recvSPKIhash, pinCtx->pinSPKIhash,
                WC_SHA256_DIGEST_SIZE) == 0);

    if (!pinCtx->matched) {
        fprintf(stderr, "Pin check: server public key does not match pin\n");
        return 0;
    }

    printf("Pin check: server public key matches pin\n");
    return 1;
}

int main(int argc, char** argv)
{
    int                ret = 0;
    int                sockfd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;
    const char*        pinHex = DEFAULT_PIN_HEX;
    PinCtx             pinCtx;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    /* Check for proper calling convention */
    if (argc < 2 || argc > 3) {
        printf("usage: %s <IPv4 address> [pin-sha256-hex]\n", argv[0]);
        printf("  pin-sha256-hex: 64 hex digits, the SHA-256 of the server's\n"
               "  DER SubjectPublicKeyInfo. Defaults to the pin for\n"
               "  ../certs/server-cert.pem.\n");
        return 0;
    }
    if (argc == 3) {
        pinHex = argv[2];
    }

    memset(&pinCtx, 0, sizeof(pinCtx));
    /* Stays set unless the callback gets as far as comparing the pin */
    pinCtx.nonMatchError = 1;
    if (HexToBytes(pinHex, pinCtx.pinSPKIhash,
                sizeof(pinCtx.pinSPKIhash)) != 0) {
        fprintf(stderr, "ERROR: pin must be %d hex digits\n",
                WC_SHA256_DIGEST_SIZE * 2);
        return -1;
    }

    /* ---- Make Connection To Server (internals arbitrary) ---- */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1; goto exit;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */

    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        ret = -1; goto exit;
    }

    if ((ret = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)))
         == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto exit;
    }
    /* ---- Make Connection To Server (internals arbitrary) ---- */

    /*---------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------*/

    /* uncomment to enable debug info
     * wolfSSL_Debugging_ON();
     */

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto exit;
    }

    /* Create and initialize WOLFSSL_CTX, TLS v1.3 only */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1; goto exit;
    }

    /* Load client certificate into WOLFSSL_CTX (server-tls13 requires it) */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE,
                                    WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto exit;
    }

    /* Load client key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE,
                                    WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        goto exit;
    }

    /* Load CA certificate into WOLFSSL_CTX. Chain validation still happens;
     * the pin is checked on top of it, not instead of it. */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL))
         != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CA_FILE);
        goto exit;
    }

    /* Register the pin check as the verify callback and hand it the pin.
     * wolfSSL_CTX_SetCertCbCtx() makes pinCtx available to the callback as
     * store->userCtx. */
    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, PinVerifyCallback);
    wolfSSL_CTX_SetCertCbCtx(ctx, &pinCtx);

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1; goto exit;
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
        if (!pinCtx.nonMatchError && !pinCtx.matched) {
            fprintf(stderr, "  expected pin: ");
            PrintHex(stderr, pinCtx.pinSPKIhash, sizeof(pinCtx.pinSPKIhash));
            fprintf(stderr, "\n  server sent:  ");
            PrintHex(stderr, pinCtx.recvSPKIhash, sizeof(pinCtx.recvSPKIhash));
            fprintf(stderr, "\n");
        }
        goto exit;
    }

    printf("Connected with %s, server public key SHA-256: ",
           wolfSSL_get_version(ssl));
    PrintHex(stdout, pinCtx.recvSPKIhash, sizeof(pinCtx.recvSPKIhash));
    printf("\n");

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
    printf("Example requires wolfSSL 5.8.2 or later with TLS v1.3, SHA-256 and\n"
           "WOLFSSL_ALWAYS_VERIFY_CB\n"
           "(build wolfSSL with --enable-opensslextra or\n"
           " CFLAGS=\"-DWOLFSSL_ALWAYS_VERIFY_CB\")\n");
    return 0;
}

#endif /* WOLFSSL_TLS13 && WOLFSSL_ALWAYS_VERIFY_CB && !NO_SHA256 &&
        * LIBWOLFSSL_VERSION_HEX >= 0x05008002 */
