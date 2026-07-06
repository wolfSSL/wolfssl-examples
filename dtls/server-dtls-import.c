/* server-dtls-import.c
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
 * DTLS server that imports a previously exported session.
 * The session is loaded from an encrypted file and decrypted with AES.
 * This allows resuming communication without a new handshake.
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

static int cleanup = 0;

static void sig_handler(int sig)
{
    (void)sig;
    cleanup = 1;
}

static void Usage(const char* progName)
{
    XPRINTF("Usage: %s [session_file]\n", progName);
    XPRINTF("  session_file - Optional: file to load session from (default: %s)\n",
           DEFAULT_SERVER_SESSION_FILE);
}

int main(int argc, char** argv)
{
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
    char            ack[] = "Message received (resumed session)!\n";
    const char*     sessionFile = DEFAULT_SERVER_SESSION_FILE;
    unsigned char*  sessionBuf = NULL;
    unsigned int    sessionSz = 0;
    unsigned int    sessionBufSz = 0;
    int             err_occurred = 0;

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
        XFPRINTF(stderr, "Error: wolfSSL_CTX_new failed\n");
        ret = 1;
        goto cleanup;
    }

    /* Create SSL object */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        XFPRINTF(stderr, "Error: wolfSSL_new failed\n");
        ret = 1;
        goto cleanup;
    }

    /* Load encrypted session from file */
    XPRINTF("Loading session from %s...\n", sessionFile);
    sessionBuf = LoadEncryptedSession(sessionFile, &sessionSz, &sessionBufSz);
    if (sessionBuf == NULL) {
        XFPRINTF(stderr, "Error: Failed to load session from file\n");
        ret = 1;
        goto cleanup;
    }

    /* Import the session */
    XPRINTF("Importing DTLS session (%u bytes)...\n", sessionSz);
    ret = wolfSSL_dtls_import(ssl, sessionBuf, sessionSz);
    if (ret < 0) {
        XFPRINTF(stderr, "Error: wolfSSL_dtls_import failed: %d\n", ret);
        XFPRINTF(stderr, "Make sure wolfSSL was compiled with --enable-sessionexport\n");
        ret = 1;
        goto cleanup;
    }
    XPRINTF("Session imported successfully!\n");

    /* Create UDP socket */
    listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listenfd < 0) {
        XFPRINTF(stderr, "Error: Cannot create socket\n");
        ret = 1;
        goto cleanup;
    }

    /* Set socket options */
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
    if (ret < 0) {
        XFPRINTF(stderr, "Error: setsockopt SO_REUSEADDR failed\n");
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
        XFPRINTF(stderr, "Error: bind failed\n");
        ret = 1;
        goto cleanup;
    }

    XPRINTF("Session restored - no handshake needed!\n");
    XPRINTF("DTLS server listening on port %d\n", SERV_PORT);
    XPRINTF("Waiting for client messages...\n\n");

    /* Wait for client connection */
    cliLen = sizeof(cliAddr);
    recvLen = (int)recvfrom(listenfd, buf, sizeof(buf), MSG_PEEK,
                            (struct sockaddr*)&cliAddr, &cliLen);
    if (recvLen < 0) {
        XFPRINTF(stderr, "Error: recvfrom failed\n");
        ret = 1;
        goto cleanup;
    }

    /* Connect UDP socket to client */
    ret = connect(listenfd, (struct sockaddr*)&cliAddr, cliLen);
    if (ret != 0) {
        XFPRINTF(stderr, "Error: UDP connect failed\n");
        ret = 1;
        goto cleanup;
    }

    XPRINTF("Client connected from %s:%d\n",
           inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));

    /* Set DTLS peer */
    wolfSSL_dtls_set_peer(ssl, &cliAddr, cliLen);

    /* Set socket file descriptor */
    wolfSSL_set_fd(ssl, listenfd);

    /* Handle messages using the imported session */
    while (!cleanup) {
        recvLen = wolfSSL_read(ssl, buf, sizeof(buf) - 1);
        if (recvLen > 0) {
            buf[recvLen] = '\0';
            XPRINTF("Received: %s", (char*)buf);

            /* Send acknowledgment */
            ret = wolfSSL_write(ssl, ack, (int)strlen(ack));
            if (ret > 0) {
                XPRINTF("Sent: %s", ack);
            }
            else {
                int err = wolfSSL_get_error(ssl, ret);
                XFPRINTF(stderr, "Error: wolfSSL_write failed: %d (%s)\n",
                        err, wolfSSL_ERR_reason_error_string(err));
                ret = (ret != 0) ? ret : -1;
                err_occurred = 1;
                break;
            }
        }
        else {
            int err = wolfSSL_get_error(ssl, recvLen);
            if (err != SSL_ERROR_WANT_READ) {
                XFPRINTF(stderr, "Error: wolfSSL_read failed: %d (%s)\n",
                        err, wolfSSL_ERR_reason_error_string(err));
                ret = (recvLen != 0) ? recvLen : -1;
                err_occurred = 1;
                break;
            }
        }
    }

    if (!err_occurred) {
        ret = 0;
    }

cleanup:
    if (sessionBuf != NULL) {
        wc_ForceZero(sessionBuf, sessionBufSz);
        XFREE(sessionBuf, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
    if (ssl != NULL) {
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (listenfd > 0) close(listenfd);
    if (ctx != NULL) wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return ret;
}
