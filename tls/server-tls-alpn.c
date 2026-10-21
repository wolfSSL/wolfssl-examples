/* server-tls-alpn.c
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

/* TLS v1.3 server demonstrating Application-Layer Protocol Negotiation
 * (ALPN, RFC 7301) with a selection callback.
 *
 * The client offers a list of protocols. A callback registered with
 * wolfSSL_CTX_set_alpn_select_cb() uses wolfSSL_select_next_proto() to pick
 * the first protocol in the server's own preference list that the client also
 * offered. If there is none it fails the handshake, as RFC 7301 requires.
 *
 * APIs shown:
 *   wolfSSL_CTX_set_alpn_select_cb()  selection callback for every connection
 *   wolfSSL_set_alpn_select_cb()      per-connection override
 *   wolfSSL_select_next_proto()       server-preference selection
 *   wolfSSL_ALPN_GetProtocol()        the protocol that was negotiated
 *   wolfSSL_ALPN_GetPeerProtocol()    the full list the client offered
 *   wolfSSL_ALPN_FreePeerProtocol()   free that list
 *
 * Pair with client-tls-alpn. Requires wolfSSL built with
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
#include <signal.h>

/* wolfSSL */
#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

/* The select callback is only invoked by wolfSSL when built with OPENSSL_ALL
 * (or the nginx/haproxy ports). --enable-opensslall provides that. */
#if defined(WOLFSSL_TLS13) && defined(HAVE_ALPN) && !defined(NO_BIO) && \
    (defined(OPENSSL_ALL) || defined(WOLFSSL_NGINX) || defined(WOLFSSL_HAPROXY))

/* SSL_TLSEXT_ERR_OK, SSL_TLSEXT_ERR_NOACK, SSL_TLSEXT_ERR_ALERT_FATAL,
 * WOLFSSL_NPN_NEGOTIATED */
#include <wolfssl/openssl/ssl.h>

#define DEFAULT_PORT 11111

#define DEFAULT_PREF "h2,http/1.1"

#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"

/* A server preference list in wire format, most preferred first */
struct AlpnPrefs {
    unsigned char wire[256];
    unsigned int  len;
};

/* Convert a comma-separated list such as "h2,http/1.1" into wire format: each
 * name preceded by a one-byte length. Returns 0, or -1 on an empty name, a
 * name longer than 255 bytes or a list too large for the buffer. */
static int AlpnListToWire(const char* list, struct AlpnPrefs* prefs)
{
    prefs->len = 0;

    while (*list != '\0') {
        const char* end     = strchr(list, ',');
        size_t      nameLen = (end != NULL) ? (size_t)(end - list)
                                            : strlen(list);

        if (nameLen == 0 || nameLen > 255 ||
                prefs->len + 1 + nameLen > sizeof(prefs->wire)) {
            return -1;
        }
        prefs->wire[prefs->len++] = (unsigned char)nameLen;
        memcpy(prefs->wire + prefs->len, list, nameLen);
        prefs->len += (unsigned int)nameLen;

        list += nameLen;
        if (*list == ',') {
            list++;
            if (*list == '\0') {
                return -1; /* trailing comma leaves an empty last name */
            }
        }
    }
    return (prefs->len > 0) ? 0 : -1;
}

/* Print a wire-format list as "a,b,c", stopping at a malformed entry */
static void PrintWire(const unsigned char* in, unsigned int inLen)
{
    unsigned int i = 0;

    while (i < inLen) {
        unsigned int len = in[i];

        if (len == 0 || len > inLen - i - 1) {
            break;
        }
        printf("%s%.*s", (i > 0) ? "," : "", (int)len, in + i + 1);
        i += 1 + len;
    }
}

/* ALPN selection callback.
 *
 * in/inLen  the client's offered list in wire format (length byte + name,
 *           repeated), exactly as it appeared in the ClientHello.
 * out/outLen must be set to point at the chosen name. It only has to stay
 *           valid until the callback returns; wolfSSL copies the selection.
 * arg       whatever was passed to wolfSSL_CTX_set_alpn_select_cb(), here
 *           the server's preference list as a struct AlpnPrefs.
 *
 * wolfSSL_select_next_proto() walks the server's list in order and picks the
 * first protocol the client also offered. On no overlap it still sets out to
 * the client's first protocol (the NPN fallback), so check its return value.
 *
 * Return SSL_TLSEXT_ERR_OK to use the selection, SSL_TLSEXT_ERR_ALERT_FATAL
 * to abort the handshake with a no_application_protocol alert, or
 * SSL_TLSEXT_ERR_NOACK to ignore the client's ALPN extension and continue
 * with no protocol agreed. NOACK is what many OpenSSL applications return on
 * no overlap, but RFC 7301 says the server must send the fatal alert.
 */
static int AlpnSelectCb(WOLFSSL* ssl, const unsigned char** out,
                        unsigned char* outLen, const unsigned char* in,
                        unsigned int inLen, void* arg)
{
    const struct AlpnPrefs* prefs = (const struct AlpnPrefs*)arg;
    unsigned char*          selected = NULL;
    unsigned char           selectedLen = 0;

    (void)ssl;

    printf("ALPN callback: client offered ");
    PrintWire(in, inLen);

    if (wolfSSL_select_next_proto(&selected, &selectedLen, prefs->wire,
                                  prefs->len, in, inLen)
            != WOLFSSL_NPN_NEGOTIATED) {
        printf("; no common protocol, rejecting the handshake\n");
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    }

    *out    = selected;
    *outLen = selectedLen;
    printf("; selected %.*s\n", (int)selectedLen, selected);
    return SSL_TLSEXT_ERR_OK;
}

int main(int argc, char** argv)
{
    int                ret = 0;
    int                sockfd = SOCKET_INVALID;
    int                connd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    int                shutdown = 0;
    int                on = 1;
    const char*        reply = "I hear ya fa shizzle!\n";
    struct AlpnPrefs   defaultPrefs;
    struct AlpnPrefs   override = {{0}, 0};

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    if (argc > 2) {
        printf("usage: %s [protocol,list]\n", argv[0]);
        printf("  protocol,list: comma-separated server preference order,\n"
               "  applied per connection with wolfSSL_set_alpn_select_cb().\n"
               "  Defaults to \"%s\" set on the context.\n", DEFAULT_PREF);
        return 0;
    }

    /* Build the preference lists once; the callback reads them for every
     * connection. */
    if (AlpnListToWire(DEFAULT_PREF, &defaultPrefs) != 0) {
        fprintf(stderr, "ERROR: bad default protocol list\n");
        return -1;
    }
    if (argc == 2) {
        if (AlpnListToWire(argv[1], &override) != 0) {
            fprintf(stderr, "ERROR: bad protocol list \"%s\"\n", argv[1]);
            return -1;
        }
        printf("Server ALPN preference: ");
        PrintWire(override.wire, override.len);
        printf("\n");
    }

    /* A client that goes away while we write must not kill the server */
    signal(SIGPIPE, SIG_IGN);

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto exit;
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1; goto exit;
    }

    /* make sure server is setup for reuse addr/port */
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on,
               (socklen_t)sizeof(on));

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1; goto exit;
    }

    /* Load server certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE,
                                    WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto exit;
    }

    /* Load server key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE,
                                    WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        goto exit;
    }

    /* Register the ALPN selection callback for every connection made from
     * this context, with the default preference list as its argument. */
    wolfSSL_CTX_set_alpn_select_cb(ctx, AlpnSelectCb, (void*)&defaultPrefs);

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */

    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1; goto exit;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1; goto exit;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1; goto exit;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1; goto exit;
        }

        /* A preference list from the command line overrides the context's
         * callback argument for this connection only. */
        if (override.len > 0) {
            wolfSSL_set_alpn_select_cb(ssl, AlpnSelectCb, (void*)&override);
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);

        /* Establish TLS connection. A client with no protocol in common is
         * rejected here; keep serving other clients. */
        if ((ret = wolfSSL_accept(ssl)) != WOLFSSL_SUCCESS) {
            int err = wolfSSL_get_error(ssl, ret);
            fprintf(stderr, "wolfSSL_accept error = %d (%s)\n\n", err,
                    wolfSSL_ERR_reason_error_string((unsigned long)err));
            goto next;
        }

        printf("Client connected successfully\n");

        /* Show the outcome with the native ALPN getters */
        {
            char*          proto   = NULL;
            unsigned short protoSz = 0;
            char*          list    = NULL;
            unsigned short listSz  = 0;

            if (wolfSSL_ALPN_GetProtocol(ssl, &proto, &protoSz)
                    == WOLFSSL_SUCCESS && protoSz > 0) {
                printf("Negotiated ALPN protocol: %.*s\n", (int)protoSz, proto);
            }
            else {
                printf("No ALPN protocol negotiated\n");
            }

            /* The client's whole offered list, as a comma-separated string
             * that must be released with wolfSSL_ALPN_FreePeerProtocol(). */
            if (wolfSSL_ALPN_GetPeerProtocol(ssl, &list, &listSz)
                    == WOLFSSL_SUCCESS) {
                printf("Client offered: %.*s\n", (int)listSz, list);
                wolfSSL_ALPN_FreePeerProtocol(ssl, &list);
            }
        }

        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) < 0) {
            fprintf(stderr, "ERROR: failed to read\n");
            goto next;
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
        if ((ret = wolfSSL_write(ssl, buff, (int)len)) != (int)len) {
            fprintf(stderr, "ERROR: failed to write\n");
            goto next;
        }

        /* Notify the client that the connection is ending */
        wolfSSL_shutdown(ssl);

next:
        /* Cleanup after this connection */
        if (ssl) {
            wolfSSL_free(ssl);      /* Free the wolfSSL object              */
            ssl = NULL;
        }
        if (connd != SOCKET_INVALID) {
            close(connd);           /* Close the connection to the client   */
            connd = SOCKET_INVALID;
        }
    }

    printf("Shutdown complete\n");
    ret = 0;

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
