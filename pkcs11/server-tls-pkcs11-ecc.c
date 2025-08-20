/* server-tls-pkcs11-ecc.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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
#include <wolfssl/wolfcrypt/wc_pkcs11.h>

#define DEFAULT_PORT 11111

#define CERT_FILE    "../certs/server-ecc.pem"
#define PRIV_KEY_ID  {0x00, 0x02}

#ifndef WOLFCRYPT_ONLY
int server_tls(int devId, Pkcs11Token* token, const char *certLabel,
               const byte *certId, word32 certIdLen, const byte *privKeyId,
               word32 privKeyIdLen)
{
    int                sockfd;
    int                connd;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    int                shutdown = 0;
    int                ret;
    const char*        reply = "I hear ya fa shizzle!\n";

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;



    /* Initialize wolfSSL */
    wolfSSL_Init();



    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }



    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    if (wolfSSL_CTX_SetDevId(ctx, devId) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    /* Load server certificates into WOLFSSL_CTX */
    if (certLabel != NULL) {
        if (wolfSSL_CTX_use_certificate_label(ctx, certLabel, devId)
            != SSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load certificate by label: %s\n",
                    certLabel);
            return -1;
        }
    }
    else if (certId != NULL) {
        if (wolfSSL_CTX_use_certificate_id(ctx, certId, certIdLen, devId)
            != SSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load certificate by id: %s\n",
                    certId);
            return -1;
        }
    }
    else {
        if (wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, WOLFSSL_FILETYPE_PEM)
            != SSL_SUCCESS) {
            fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                    CERT_FILE);
            return -1;
        }
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_id(ctx, privKeyId, privKeyIdLen, devId,
            2048/8) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set id.\n");
        return -1;
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
        return -1;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        return -1;
    }



    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            return -1;
        }

        /* Create a WOLFSSL object */
        if ((ret = wc_Pkcs11Token_Open(token, 1)) != 0) {
            fprintf(stderr, "ERROR: failed to open session on token (%d)\n",
                ret);
            return -1;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            return -1;
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);

        /* Establish TLS connection */
        ret = wolfSSL_accept(ssl);
        if (ret != SSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_accept error = %d\n",
                wolfSSL_get_error(ssl, ret));
            return -1;
        }


        printf("Client connected successfully\n");



        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if (wolfSSL_read(ssl, buff, sizeof(buff)-1) == -1) {
            fprintf(stderr, "ERROR: failed to read\n");
            return -1;
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
        if (wolfSSL_write(ssl, buff, len) != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            return -1;
        }



        /* Cleanup after this connection */
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        wc_Pkcs11Token_Close(token);
        close(connd);           /* Close the connection to the client   */
    }

    printf("Shutdown complete\n");



    /* Cleanup and return */
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
    close(sockfd);          /* Close the socket listening for clients   */
    return 0;               /* Return reporting a success               */
}
#endif

static int string_matches(const char* arg, const char* str)
{
    int len = (int)XSTRLEN(str) + 1;
    return XSTRNCMP(arg, str, len) == 0;
}

static void Usage(void)
{
    printf("server-tls-pkcs11\n");
    printf("-?                  Help, print this usage\n");
    printf("-lib <file>         PKCS#11 library to test\n");
    printf("-slot <num>         Slot number to use\n");
    printf("-tokenName <string> Token name\n");
    printf("-userPin <string>   User PIN\n");
    printf("-privKeyId <string> Private key identifier\n");
    printf("-certId <string>    Certificate identifier\n");
    printf("-certLabel <string> Certificate label\n");
}

int main(int argc, char* argv[])
{
    int ret;
    const char* library = NULL;
    const char* tokenName = NULL;
    const char* userPin = NULL;
    const char* certLabel = NULL;
    const byte* certId = NULL;
    int certIdLen = 0;
    Pkcs11Dev dev;
    Pkcs11Token token;
    int slotId = -1;
    int devId = 1;
    const unsigned char defaultPrivKeyId[] = PRIV_KEY_ID;
    int privKeyIdLen = 2;
    const byte *privKeyId = (const byte *)defaultPrivKeyId;

    argc--;
    argv++;
    while (argc > 0) {
        if (string_matches(*argv, "-?")) {
            Usage();
            return 0;
        }
        else if (string_matches(*argv, "-lib")) {
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Library name not supplied\n");
                return 1;
            }
            library = *argv;
        }
        else if (string_matches(*argv, "-slot")) {
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Slot number not supplied\n");
                return 1;
            }
            slotId = atoi(*argv);
        }
        else if (string_matches(*argv, "-tokenName")) {
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Token name not supplied\n");
                return 1;
            }
            tokenName = *argv;
        }
        else if (string_matches(*argv, "-userPin")) {
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "User PIN not supplied\n");
                return 1;
            }
            userPin = *argv;
        }
        else if (string_matches(*argv, "-privKeyId")) {
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Private key identifier not supplied\n");
                return 1;
            }
            privKeyId = (byte*)*argv;
            privKeyIdLen = (int)strlen(*argv);
        }
        else if (string_matches(*argv, "-certId")) {
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Certificate identifier not supplied\n");
                return 1;
            }
            certId = (byte*)*argv;
            certIdLen = (int)strlen(*argv);
        }
        else if (string_matches(*argv, "-certLabel")) {
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Certificate label not supplied\n");
                return 1;
            }
            certLabel = (char*)*argv;
        }
        else {
            fprintf(stderr, "Unrecognized command line argument\n  %s\n",
                argv[0]);
            Usage();
            return 1;
        }

        argc--;
        argv++;
    }

    if (library == NULL || tokenName == NULL) {
        fprintf(stderr, "Error: missing arguments\n");
        Usage();
        return 1;
    }

#if defined(DEBUG_WOLFSSL)
    wolfSSL_Debugging_ON();
#endif
    wolfCrypt_Init();

    ret = wc_Pkcs11_Initialize(&dev, library, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize PKCS#11 library\n");
        ret = 2;
    }
    if (ret == 0) {
        ret = wc_Pkcs11Token_Init(&token, &dev, slotId, tokenName,
            (byte*)userPin, userPin == NULL ? 0 : strlen(userPin));
        if (ret != 0) {
            fprintf(stderr, "Failed to initialize PKCS#11 token\n");
            ret = 2;
        }
        if (ret == 0) {
            ret = wc_CryptoDev_RegisterDevice(devId, wc_Pkcs11_CryptoDevCb,
                                              &token);
            if (ret != 0) {
                fprintf(stderr, "Failed to register PKCS#11 token\n");
                ret = 2;
            }
            if (ret == 0) {
            #if !defined(WOLFCRYPT_ONLY)
                ret = server_tls(devId, &token, certLabel, certId, certIdLen,
                    privKeyId, (word32)privKeyIdLen);
                if (ret != 0)
                    ret = 1;
            #endif
            }
            wc_Pkcs11Token_Final(&token);
        }
        wc_Pkcs11_Finalize(&dev);
    }

    wolfCrypt_Cleanup();

    return ret;
}

