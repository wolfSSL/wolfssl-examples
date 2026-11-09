/* server-tls-sni.c
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

/* TLS v1.3 server demonstrating Server Name Indication (SNI, RFC 6066).
 *
 * The server hosts two names on one port, each with its own certificate:
 *   example.com      RSA certificate  (../certs/server-cert.pem)
 *   www.wolfssl.com  ECC certificate  (../certs/server-ecc.pem)
 *
 * Before handing a connection to wolfSSL it peeks at the raw ClientHello,
 * pulls the requested host name out with wolfSSL_SNI_GetFromBuffer(), and
 * picks the WOLFSSL_CTX that holds the matching certificate. wolfSSL then
 * validates the name against the one registered on that context and applies
 * the mismatch policy chosen on the command line.
 *
 * APIs shown:
 *   wolfSSL_CTX_UseSNI()         name a context answers for
 *   wolfSSL_CTX_SNI_SetOptions() what to do when the name does not match or
 *                                is absent (wolfSSL_SNI_SetOptions() does the
 *                                same per connection after wolfSSL_UseSNI())
 *   wolfSSL_SNI_GetFromBuffer()  read the SNI out of a raw ClientHello
 *   wolfSSL_SNI_Status()         how the request matched after the handshake
 *   wolfSSL_SNI_GetRequest()     the name the client asked for
 *
 * Pair with client-tls-sni. Requires wolfSSL built with SNI, which is on by
 * default.
 */

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

/* wolfSSL */
#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/error-ssl.h>

#define DEFAULT_PORT 11111

#define TLS_HEADER_SZ   5
#define TLS_MAX_RECORD  (TLS_HEADER_SZ + 16384 + 2048) /* header + max ciphertext */
#define PEEK_TIMEOUT_S  2

#if defined(WOLFSSL_TLS13) && defined(HAVE_SNI) && !defined(NO_WOLFSSL_SERVER)

/* One virtual host: the name it answers for and its credentials */
typedef struct VHost {
    const char*  name;
    const char*  certFile;
    const char*  keyFile;
    WOLFSSL_CTX* ctx;
} VHost;

static VHost vhosts[] = {
    { "example.com",     "../certs/server-cert.pem", "../certs/server-key.pem",
      NULL },
    { "www.wolfssl.com", "../certs/server-ecc.pem",  "../certs/ecc-key.pem",
      NULL },
};
#define NUM_VHOSTS (sizeof(vhosts) / sizeof(vhosts[0]))
#define DEFAULT_VHOST 0   /* used when the client sends no SNI or an unknown one */

/* Map a policy name from the command line to SNI option bits */
static int ParsePolicy(const char* name, unsigned char* options)
{
    if (strcmp(name, "strict") == 0) {
        /* No options: an unknown name gets a fatal unrecognized_name alert,
         * a missing SNI is served by the default host. */
        *options = 0;
    }
    else if (strcmp(name, "continue") == 0) {
        /* Serve an unknown name from the default host without acknowledging
         * the SNI. wolfSSL_SNI_Status() reports WOLFSSL_SNI_NO_MATCH. */
        *options = WOLFSSL_SNI_CONTINUE_ON_MISMATCH;
    }
    else if (strcmp(name, "answer") == 0) {
        /* Serve an unknown name and acknowledge the SNI as if it matched.
         * wolfSSL_SNI_Status() reports WOLFSSL_SNI_FAKE_MATCH. */
        *options = WOLFSSL_SNI_ANSWER_ON_MISMATCH;
    }
    else if (strcmp(name, "abort") == 0) {
        /* Refuse clients that send no SNI at all (fatal missing_extension
         * alert in TLS v1.3); unknown names are still served. */
        *options = WOLFSSL_SNI_ABORT_ON_ABSENCE |
                   WOLFSSL_SNI_CONTINUE_ON_MISMATCH;
    }
    else {
        return -1;
    }
    return 0;
}

static const char* StatusName(unsigned char status)
{
    switch (status) {
        case WOLFSSL_SNI_NO_MATCH:   return "WOLFSSL_SNI_NO_MATCH";
        case WOLFSSL_SNI_FAKE_MATCH: return "WOLFSSL_SNI_FAKE_MATCH";
        case WOLFSSL_SNI_REAL_MATCH: return "WOLFSSL_SNI_REAL_MATCH";
        case WOLFSSL_SNI_FORCE_KEEP: return "WOLFSSL_SNI_FORCE_KEEP";
        default:                     return "unknown";
    }
}

/* Peek one complete TLS record without consuming it. The 5 byte header gives
 * the record length; MSG_WAITALL then blocks until that many bytes are queued
 * (honored with MSG_PEEK on Linux). On success *buf holds a malloc'd copy of
 * the record that the caller must free.
 *
 * Returns the record size, or -1 on timeout, a closed connection, a record
 * that is not a handshake, or an allocation failure. */
static int PeekRecord(int fd, unsigned char** buf)
{
    unsigned char hdr[TLS_HEADER_SZ];
    unsigned char* rec;
    int recSz;

    *buf = NULL;
    if (recv(fd, hdr, sizeof(hdr), MSG_PEEK | MSG_WAITALL) != TLS_HEADER_SZ) {
        return -1;
    }
    if (hdr[0] != 0x16) {
        return -1; /* content type is not handshake */
    }

    /* read wire order len */
    recSz = TLS_HEADER_SZ + ((hdr[3] << 8) | hdr[4]);
    if (recSz > TLS_MAX_RECORD) {
        return -1;
    }
    rec = (unsigned char*)malloc((size_t)recSz);
    if (rec == NULL) {
        return -1;
    }

    if (recv(fd, rec, (size_t)recSz, MSG_PEEK | MSG_WAITALL) != recSz) {
        free(rec);
        return -1;
    }
    *buf = rec;
    return recSz;
}

/* Peek at the ClientHello waiting on the socket and extract the SNI host
 * name without consuming any bytes, so wolfSSL_accept() still sees it.
 *
 * Returns 1 with the name in sni, 0 if the ClientHello carries no SNI, and
 * -1 on timeout, a closed connection, or a malformed record. */
static int PeekSni(int fd, char* sni, unsigned int sniSz)
{
    unsigned char* hello = NULL;
    unsigned int   outSz = sniSz - 1;
    struct timeval timeout = { PEEK_TIMEOUT_S, 0 };
    struct timeval noTimeout = { 0, 0 };
    int            n;
    int            ret;

    /* Bound the blocking peeks so a stalled client cannot hang the server,
     * then clear the timeout so wolfSSL's I/O on this socket is unaffected */
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    n = PeekRecord(fd, &hello);
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &noTimeout, sizeof(noTimeout));
    if (n < 0) {
        return -1;
    }

    ret = wolfSSL_SNI_GetFromBuffer(hello, (unsigned int)n,
                                    WOLFSSL_SNI_HOST_NAME,
                                    (unsigned char*)sni, &outSz);
    free(hello);
    if (ret == WOLFSSL_SUCCESS) {
        sni[outSz] = '\0';
        return 1;
    }
    if (ret == 0) {
        return 0; /* complete ClientHello, no SNI extension */
    }
    /* Not a ClientHello we can parse, or one fragmented across records */
    return -1;
}

/* Case-insensitive host name compare. Returns 1 on match, 0 otherwise. */
static int checkDnsHostNames(const char* n1, const char* n2)
{
    while (*n1 != '\0' && *n2 != '\0') {
        if (tolower((unsigned char)*n1++) != tolower((unsigned char)*n2++)) {
            return 0;
        }
    }
    if (tolower(*n1) == tolower(*n2)) {
        return 1;
    }
    return 0;
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
    unsigned char      options = WOLFSSL_SNI_CONTINUE_ON_MISMATCH;
    size_t             i;

    /* declare wolfSSL objects */
    WOLFSSL* ssl = NULL;

    if (argc > 2 || (argc == 2 && ParsePolicy(argv[1], &options) != 0)) {
        printf("usage: %s [strict|continue|answer|abort]\n", argv[0]);
        printf("  strict:   reject unknown names (unrecognized_name alert)\n"
               "  continue: serve unknown names from the default host "
               "(default)\n"
               "  answer:   like continue, but acknowledge the SNI anyway\n"
               "  abort:    reject clients that send no SNI\n");
        return 0;
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

    /* One WOLFSSL_CTX per virtual host, each answering for its own name */
    for (i = 0; i < NUM_VHOSTS; i++) {
        VHost* vh = &vhosts[i];

        if ((vh->ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method())) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
            ret = -1; goto exit;
        }
        if ((ret = wolfSSL_CTX_use_certificate_file(vh->ctx, vh->certFile,
                                    WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load %s, please check the "
                    "file.\n", vh->certFile);
            goto exit;
        }
        if ((ret = wolfSSL_CTX_use_PrivateKey_file(vh->ctx, vh->keyFile,
                                    WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load %s, please check the "
                    "file.\n", vh->keyFile);
            goto exit;
        }

        /* Register the name this context serves. During the handshake
         * wolfSSL compares the client's SNI against it. */
        if ((ret = wolfSSL_CTX_UseSNI(vh->ctx, WOLFSSL_SNI_HOST_NAME, vh->name,
                                      (unsigned short)strlen(vh->name)))
                != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: wolfSSL_CTX_UseSNI failed\n");
            goto exit;
        }

        /* Mismatch/absence policy for that name */
        wolfSSL_CTX_SNI_SetOptions(vh->ctx, WOLFSSL_SNI_HOST_NAME, options);

        printf("Serving %-16s with %s\n", vh->name, vh->certFile);
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
        ret = -1; goto exit;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1; goto exit;
    }

    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        char   sni[256];
        VHost* vh = &vhosts[DEFAULT_VHOST];
        int    peek;

        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1; goto exit;
        }

        /* Look at the ClientHello before wolfSSL does and pick the context
         * whose certificate matches the requested name. */
        peek = PeekSni(connd, sni, sizeof(sni));
        if (peek < 0) {
            fprintf(stderr, "Could not read a ClientHello from the client\n\n");
            goto next;
        }
        if (peek == 0) {
            printf("ClientHello has no SNI, using default host %s\n",
                   vh->name);
        }
        else {
            printf("ClientHello requests \"%s\"", sni);
            for (i = 0; i < NUM_VHOSTS; i++) {
                if (checkDnsHostNames(vhosts[i].name, sni) == 1) {
                    vh = &vhosts[i];
                    break;
                }
            }
            printf(", using host %s\n", vh->name);
        }

        /* Create a WOLFSSL object from the chosen context */
        if ((ssl = wolfSSL_new(vh->ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1; goto exit;
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);

        /* Establish TLS connection. wolfSSL reads the same ClientHello we
         * peeked at, checks its SNI against the context's name and applies
         * the policy. A rejected client does not stop the server. */
        if ((ret = wolfSSL_accept(ssl)) != WOLFSSL_SUCCESS) {
            int err = wolfSSL_get_error(ssl, ret);
            fprintf(stderr, "wolfSSL_accept error = %d (%s)\n\n", err,
                    wolfSSL_ERR_reason_error_string((unsigned long)err));
            goto next;
        }

        printf("Client connected successfully\n");

        /* Report how the SNI was handled */
        {
            void*          request   = NULL;
            unsigned short requestSz = wolfSSL_SNI_GetRequest(ssl,
                                           WOLFSSL_SNI_HOST_NAME, &request);
            unsigned char  status    = wolfSSL_SNI_Status(ssl,
                                           WOLFSSL_SNI_HOST_NAME);

            if (requestSz > 0 && request != NULL) {
                printf("SNI request: %.*s\n", (int)requestSz,
                       (const char*)request);
            }
            else {
                printf("SNI request: none recorded\n");
            }
            printf("SNI status:  %s\n", StatusName(status));
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
    for (i = 0; i < NUM_VHOSTS; i++) {
        if (vhosts[i].ctx)
            wolfSSL_CTX_free(vhosts[i].ctx);
    }
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
