/* server-dtls-export.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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
 *
 *=============================================================================
 *
 * DTLS server that exports its session after handshake.
 * The exported session is encrypted with AES and saved to a file.
 * This session can later be imported by server-dtls-import to resume
 * communication without a new handshake.
 *
 * Requires wolfSSL compiled with:
 *   ./configure --enable-dtls --enable-sessionexport
 */

#include "dtls-export-common.h"
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#define SERV_PORT   11111
#define MSGLEN      4096

#define DEFAULT_SERVER_SESSION_FILE "dtls_server_session.bin"

static int cleanup = 0;

static void sig_handler(int sig)
{
    (void)sig;
    cleanup = 1;
}

static void Usage(const char* progName)
{
    printf("Usage: %s [session_file]\n", progName);
    printf("  session_file - Optional: file to save session (default: %s)\n",
           DEFAULT_SERVER_SESSION_FILE);
}

int main(int argc, char** argv)
{
    char            caCertLoc[] = "../certs/ca-cert.pem";
    char            servCertLoc[] = "../certs/server-cert.pem";
    char            servKeyLoc[] = "../certs/server-key.pem";
    int             ret = 0;
    int             on = 1;
    int             listenfd = 0;
    int             recvLen;
    WOLFSSL_CTX*    ctx = NULL;
    WOLFSSL*        ssl = NULL;
    struct sockaddr_in servAddr;
    struct sockaddr_in cliAddr;
    socklen_t       cliLen;
    socklen_t       len = sizeof(int);
    unsigned char   buf[MSGLEN];
    char            ack[] = "Message received!\n";
    const char*     sessionFile = DEFAULT_SERVER_SESSION_FILE;
    unsigned char*  sessionBuf = NULL;
    unsigned int    sessionSz = 0;

    /* Program argument checking */
    if (argc > 2) {
        Usage(argv[0]);
        return 1;
    }

    if (argc == 2) {
        sessionFile = argv[1];
    }

    /* Set up signal handler for clean shutdown */
    signal(SIGINT, sig_handler);

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Create DTLS 1.2 context */
    ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method());
    if (ctx == NULL) {
        fprintf(stderr, "Error: wolfSSL_CTX_new failed\n");
        ret = 1;
        goto cleanup;
    }

    /* Load CA certificates */
    ret = wolfSSL_CTX_load_verify_locations(ctx, caCertLoc, NULL);
    if (ret != SSL_SUCCESS) {
        fprintf(stderr, "Error: Failed to load CA cert %s\n", caCertLoc);
        ret = 1;
        goto cleanup;
    }

    /* Load server certificate */
    ret = wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM);
    if (ret != SSL_SUCCESS) {
        fprintf(stderr, "Error: Failed to load server cert %s\n", servCertLoc);
        ret = 1;
        goto cleanup;
    }

    /* Load server private key */
    ret = wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, SSL_FILETYPE_PEM);
    if (ret != SSL_SUCCESS) {
        fprintf(stderr, "Error: Failed to load server key %s\n", servKeyLoc);
        ret = 1;
        goto cleanup;
    }

    /* Create UDP socket */
    listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listenfd < 0) {
        fprintf(stderr, "Error: Cannot create socket\n");
        ret = 1;
        goto cleanup;
    }

    /* Set socket options */
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
    if (ret < 0) {
        fprintf(stderr, "Error: setsockopt SO_REUSEADDR failed\n");
        ret = 1;
        goto cleanup;
    }

    /* Setup server address */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(SERV_PORT);

    /* Bind socket */
    ret = bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr));
    if (ret < 0) {
        fprintf(stderr, "Error: bind failed\n");
        ret = 1;
        goto cleanup;
    }

    printf("DTLS server listening on port %d\n", SERV_PORT);
    printf("Waiting for client connection...\n");

    /* Wait for client connection */
    cliLen = sizeof(cliAddr);
    recvLen = (int)recvfrom(listenfd, buf, sizeof(buf), MSG_PEEK,
                            (struct sockaddr*)&cliAddr, &cliLen);
    if (recvLen < 0) {
        fprintf(stderr, "Error: recvfrom failed\n");
        ret = 1;
        goto cleanup;
    }

    /* Connect UDP socket to client */
    ret = connect(listenfd, (struct sockaddr*)&cliAddr, cliLen);
    if (ret != 0) {
        fprintf(stderr, "Error: UDP connect failed\n");
        ret = 1;
        goto cleanup;
    }

    printf("Client connected from %s:%d\n",
           inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));

    /* Create SSL object */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        fprintf(stderr, "Error: wolfSSL_new failed\n");
        ret = 1;
        goto cleanup;
    }

    /* Set DTLS peer */
    wolfSSL_dtls_set_peer(ssl, &cliAddr, cliLen);

    /* Set socket file descriptor */
    wolfSSL_set_fd(ssl, listenfd);

    /* Perform DTLS handshake */
    printf("Performing DTLS handshake...\n");
    ret = wolfSSL_accept(ssl);
    if (ret != SSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, ret);
        fprintf(stderr, "Error: wolfSSL_accept failed: %d (%s)\n",
                err, wolfSSL_ERR_reason_error_string(err));
        ret = 1;
        goto cleanup;
    }
    printf("DTLS handshake successful!\n");

    /* Export the session */
    printf("Exporting DTLS session...\n");

    /* First call to get required buffer size */
    ret = wolfSSL_dtls_export(ssl, NULL, &sessionSz);
    if (ret != 0 && sessionSz == 0) {
        fprintf(stderr, "Error: wolfSSL_dtls_export (get size) failed: %d\n", ret);
        fprintf(stderr, "Make sure wolfSSL was compiled with --enable-sessionexport\n");
        ret = 1;
        goto cleanup;
    }

    /* Allocate buffer for session data */
    sessionBuf = (unsigned char*)malloc(sessionSz);
    if (sessionBuf == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        ret = 1;
        goto cleanup;
    }

    /* Export the session */
    ret = wolfSSL_dtls_export(ssl, sessionBuf, &sessionSz);
    if (ret <= 0) {
        fprintf(stderr, "Error: wolfSSL_dtls_export failed: %d\n", ret);
        ret = 1;
        goto cleanup;
    }
    printf("Session exported: %d bytes\n", ret);
    sessionSz = ret;

    /* Save encrypted session to file */
    ret = SaveEncryptedSession(sessionFile, sessionBuf, sessionSz);
    if (ret != 0) {
        fprintf(stderr, "Error: Failed to save encrypted session\n");
        ret = 1;
        goto cleanup;
    }

    /* Handle one message exchange */
    printf("\nWaiting for message from client...\n");
    recvLen = wolfSSL_read(ssl, buf, sizeof(buf) - 1);
    if (recvLen > 0) {
        buf[recvLen] = '\0';
        printf("Received: %s", (char*)buf);

        /* Send acknowledgment */
        ret = wolfSSL_write(ssl, ack, (int)strlen(ack));
        if (ret > 0) {
            printf("Sent: %s", ack);
        }
    }
    else {
        int err = wolfSSL_get_error(ssl, recvLen);
        if (err != SSL_ERROR_WANT_READ) {
            fprintf(stderr, "Warning: wolfSSL_read returned: %d (%s)\n",
                    err, wolfSSL_ERR_reason_error_string(err));
        }
    }

    printf("\nSession exported and saved to: %s\n", sessionFile);
    printf("You can now use server-dtls-import to resume this session.\n");
    ret = 0;

cleanup:
    if (sessionBuf != NULL) free(sessionBuf);
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (listenfd > 0) close(listenfd);
    if (ctx != NULL) wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return ret;
}
