/* client-tls-alpn.c
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

/* TLS v1.3 client demonstrating Application-Layer Protocol Negotiation
 * (ALPN, RFC 7301) through wolfSSL's OpenSSL-compatible API.
 *
 * The client lists the application protocols it speaks in its ClientHello.
 * The server picks one, and the client reads the result after the handshake.
 *
 * APIs shown:
 *   wolfSSL_CTX_set_alpn_protos()  default protocol list for every connection
 *                                  made from the context
 *   wolfSSL_set_alpn_protos()      per-connection override
 *   wolfSSL_get0_alpn_selected()   the protocol the server chose
 *
 * The two setters take the list in wire format: each name is preceded by its
 * length, e.g. { 2,'h','2', 8,'h','t','t','p','/','1','.','1' } for
 * "h2,http/1.1".
 * The native wolfSSL_UseALPN() takes a comma-separated string instead.
 *
 * Pair with server-tls-alpn. Requires wolfSSL built with
 * --enable-opensslall --enable-alpn.
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

#define CA_FILE "../certs/ca-cert.pem"

/* Protocols offered when none are given on the command line, in the
 * client's order of preference. server-tls-alpn prefers h2, so it picks h2
 * from this list even though the client lists http/1.1 first. */
#define DEFAULT_PROTOCOLS "http/1.1,h2"

/* wolfSSL_get0_alpn_selected() and the server-side select callback are part
 * of the OpenSSL compatibility layer that --enable-opensslall provides. */
#if defined(WOLFSSL_TLS13) && defined(HAVE_ALPN) && !defined(NO_BIO) && \
    (defined(OPENSSL_ALL) || defined(WOLFSSL_NGINX) || defined(WOLFSSL_HAPROXY))

/* wolfSSL_CTX_set_alpn_protos() follows the OpenSSL convention of returning
 * 0 on success when WOLFSSL_ERROR_CODE_OPENSSL is defined (--enable-opensslall
 * defines it), and WOLFSSL_SUCCESS otherwise. */
#ifdef WOLFSSL_ERROR_CODE_OPENSSL
    #define ALPN_PROTOS_OK 0
#else
    #define ALPN_PROTOS_OK WOLFSSL_SUCCESS
#endif

/* Convert a comma-separated protocol list into ALPN wire format.
 * Returns the number of bytes written, or -1 on an empty name, a name longer
 * than 255 bytes or a list too large for the buffer. */
static int AlpnListToWire(const char* list, unsigned char* out, size_t outSz)
{
    size_t outLen = 0;

    while (*list != '\0') {
        const char* end     = strchr(list, ',');
        size_t      nameLen = (end != NULL) ? (size_t)(end - list)
                                            : strlen(list);

        if (nameLen == 0 || nameLen > 255 || outLen + 1 + nameLen > outSz) {
            return -1;
        }
        out[outLen++] = (unsigned char)nameLen;
        memcpy(out + outLen, list, nameLen);
        outLen += nameLen;

        list += nameLen;
        if (*list == ',') {
            list++;
            if (*list == '\0') {
                return -1; /* trailing comma leaves an empty last name */
            }
        }
    }
    return (int)outLen;
}

int main(int argc, char** argv)
{
    int                ret = 0;
    int                sockfd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;
    unsigned char      wire[512];
    int                wireLen;
    const unsigned char* selected = NULL;
    unsigned int       selectedLen = 0;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    /* Check for proper calling convention */
    if (argc < 2 || argc > 3) {
        printf("usage: %s <IPv4 address> [protocol,list]\n", argv[0]);
        printf("  protocol,list: comma-separated ALPN protocols to offer in\n"
               "  preference order. Defaults to \"%s\".\n", DEFAULT_PROTOCOLS);
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

    /* Load CA certificate into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL))
         != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CA_FILE);
        goto exit;
    }

    /* Default ALPN list for every connection created from this context */
    wireLen = AlpnListToWire(DEFAULT_PROTOCOLS, wire, sizeof(wire));
    if (wireLen < 0) {
        fprintf(stderr, "ERROR: bad default protocol list\n");
        ret = -1; goto exit;
    }
    if (wolfSSL_CTX_set_alpn_protos(ctx, wire, (unsigned int)wireLen)
            != ALPN_PROTOS_OK) {
        fprintf(stderr, "ERROR: wolfSSL_CTX_set_alpn_protos failed\n");
        ret = -1; goto exit;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1; goto exit;
    }

    /* Per-connection override of the protocol list, if one was given */
    if (argc == 3) {
        wireLen = AlpnListToWire(argv[2], wire, sizeof(wire));
        if (wireLen < 0) {
            fprintf(stderr, "ERROR: bad protocol list \"%s\"\n", argv[2]);
            ret = -1; goto exit;
        }
        if (wolfSSL_set_alpn_protos(ssl, wire, (unsigned int)wireLen)
                != ALPN_PROTOS_OK) {
            fprintf(stderr, "ERROR: wolfSSL_set_alpn_protos failed\n");
            ret = -1; goto exit;
        }
        printf("Offering ALPN protocols: %s\n", argv[2]);
    }
    else {
        printf("Offering ALPN protocols: %s\n", DEFAULT_PROTOCOLS);
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto exit;
    }

    /* Connect to wolfSSL on the server side. If server-tls-alpn finds no
     * common protocol it sends a fatal no_application_protocol alert and
     * this fails. */
    if ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, ret);
        fprintf(stderr, "ERROR: failed to connect to wolfSSL: %d (%s)\n",
                err, wolfSSL_ERR_reason_error_string((unsigned long)err));
        goto exit;
    }

    /* Read back what the server selected. An empty result means the server
     * did not select any protocol. */
    wolfSSL_get0_alpn_selected(ssl, &selected, &selectedLen);
    if (selectedLen > 0) {
        printf("Server selected ALPN protocol: %.*s\n", (int)selectedLen,
               (const char*)selected);
    }
    else {
        printf("Server selected no ALPN protocol\n");
    }

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
    printf("Example requires TLS v1.3, ALPN and the OpenSSL compatibility\n"
           "layer (build wolfSSL with --enable-opensslall --enable-alpn)\n");
    return 0;
}

#endif
