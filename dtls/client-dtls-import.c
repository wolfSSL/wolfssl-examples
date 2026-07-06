/* client-dtls-import.c
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
 * DTLS client that imports a previously exported session.
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

#define MAXLINE   4096
#define SERV_PORT 11111

static void Usage(const char* progName)
{
    XPRINTF("Usage: %s <server IP> [session_file]\n", progName);
    XPRINTF("  server IP    - IP address of the DTLS server\n");
    XPRINTF("  session_file - Optional: file to load session from (default: %s)\n",
           DEFAULT_CLIENT_SESSION_FILE);
}

int main(int argc, char** argv)
{
    int             sockfd = 0;
    int             ret;
    struct          sockaddr_in servAddr;
    WOLFSSL*        ssl = NULL;
    WOLFSSL_CTX*    ctx = NULL;
    char            sendLine[MAXLINE];
    char            recvLine[MAXLINE - 1];
    const char*     sessionFile = DEFAULT_CLIENT_SESSION_FILE;
    unsigned char*  sessionBuf = NULL;
    unsigned int    sessionSz = 0;
    unsigned int    sessionBufSz = 0;
    int             n;
    int             err_occurred = 0;

    /* Program argument checking */
    if (argc < 2 || argc > 3) {
        Usage(argv[0]);
        return 1;
    }

    if (argc == 3) {
        sessionFile = argv[2];
    }

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Create DTLS 1.2 context */
    ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method());
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
        ret = 1;
        goto cleanup;
    }
    XPRINTF("Session imported successfully!\n");

    /* Setup server address */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        XFPRINTF(stderr, "Error: Invalid IP address: %s\n", argv[1]);
        ret = 1;
        goto cleanup;
    }

    /* Set DTLS peer */
    wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));

    /* Create UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        XFPRINTF(stderr, "Error: Cannot create socket\n");
        ret = 1;
        goto cleanup;
    }

    /* Set the socket file descriptor */
    wolfSSL_set_fd(ssl, sockfd);

    XPRINTF("Session restored - no handshake needed!\n");
    XPRINTF("Ready to communicate with server %s:%d\n\n", argv[1], SERV_PORT);

    /* Send messages using the imported session */
    XPRINTF("Enter message to send (or 'quit' to exit):\n");
    while (fgets(sendLine, MAXLINE, stdin) != NULL) {
        if (strncmp(sendLine, "quit", 4) == 0) {
            break;
        }

        /* Send message */
        ret = wolfSSL_write(ssl, sendLine, (int)strlen(sendLine));
        if (ret != (int)strlen(sendLine)) {
            int err = wolfSSL_get_error(ssl, ret);
            XFPRINTF(stderr, "Error: wolfSSL_write failed: %d (%s)\n",
                    err, wolfSSL_ERR_reason_error_string(err));
            ret = (ret != 0) ? ret : -1;
            err_occurred = 1;
            break;
        }
        XPRINTF("Sent: %s", sendLine);

        /* Read response */
        n = wolfSSL_read(ssl, recvLine, sizeof(recvLine) - 1);
        if (n > 0) {
            recvLine[n] = '\0';
            XPRINTF("Received: %s", recvLine);
        }
        else {
            int err = wolfSSL_get_error(ssl, n);
            if (err != SSL_ERROR_WANT_READ) {
                XFPRINTF(stderr, "Error: wolfSSL_read failed: %d (%s)\n",
                        err, wolfSSL_ERR_reason_error_string(err));
                ret = (n != 0) ? n : -1;
                err_occurred = 1;
                break;
            }
        }

        XPRINTF("\nEnter message (or 'quit' to exit):\n");
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
    if (sockfd > 0) close(sockfd);
    if (ctx != NULL) wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return ret;
}
