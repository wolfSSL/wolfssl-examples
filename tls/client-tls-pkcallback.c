/* client-tls-pkcallback.c
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

/* This example shows how to register a private key callback and optionally
 * use the asynchronous version of the code to return a WC_PENDING_E while the
 * hardware is processing */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
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

#define DEFAULT_PORT 11111

#define USE_ECDHE_ECDSA
#define USE_TLSV13

#ifdef USE_ECDHE_ECDSA
#define CERT_FILE   "../certs/client-ecc-cert.pem"
#define KEY_FILE    "../certs/ecc-client-key.pem"
#define KEYPUB_FILE "../certs/ecc-client-keyPub.pem"
#define CA_FILE     "../certs/ca-ecc-cert.pem"
#else
#define CERT_FILE   "../certs/client-cert.pem"
#define KEY_FILE    "../certs/client-key.pem"
#define KEYPUB_FILE "../certs/client-keyPub.pem"
#define CA_FILE     "../certs/ca-cert.pem"
#endif

typedef struct {
    const char* keyFile;
    #ifdef HAVE_ECC
    ecc_key     keyEcc;
    #endif
    #ifndef NO_RSA
    RsaKey      keyRsa;
    #endif
    int         state;
} PkCbInfo;

#ifdef HAVE_PK_CALLBACKS
/* reads file size, allocates buffer, reads into buffer, returns buffer */
static int load_file(const char* fname, byte** buf, size_t* bufLen)
{
    int ret;
    long int fileSz;
    XFILE lFile;

    if (fname == NULL || buf == NULL || bufLen == NULL)
        return BAD_FUNC_ARG;

    /* set defaults */
    *buf = NULL;
    *bufLen = 0;

    /* open file (read-only binary) */
    lFile = XFOPEN(fname, "rb");
    if (!lFile) {
        printf("Error loading %s\n", fname);
        return BAD_PATH_ERROR;
    }

    fseek(lFile, 0, SEEK_END);
    fileSz = (int)ftell(lFile);
    rewind(lFile);
    if (fileSz > 0) {
        *bufLen = (size_t)fileSz;
        *buf = (byte*)malloc(*bufLen);
        if (*buf == NULL) {
            ret = MEMORY_E;
            printf("Error allocating %lu bytes\n", (unsigned long)*bufLen);
        }
        else {
            size_t readLen = fread(*buf, *bufLen, 1, lFile);

            /* check response code */
            ret = (readLen > 0) ? 0 : -1;
        }
    }
    else {
        ret = BUFFER_E;
    }
    fclose(lFile);

    return ret;
}

static int load_key_file(const char* fname, byte** derBuf, word32* derLen)
{
    int ret;
    byte* buf = NULL;
    size_t bufLen;

    ret = load_file(fname, &buf, &bufLen);
    if (ret != 0)
        return ret;

    *derBuf = (byte*)malloc(bufLen);
    if (*derBuf == NULL) {
        free(buf);
        return MEMORY_E;
    }

    ret = wc_KeyPemToDer(buf, (word32)bufLen, *derBuf, (word32)bufLen, NULL);
    if (ret < 0) {
        free(buf);
        free(*derBuf);
        return ret;
    }
    *derLen = ret;
    free(buf);

    return 0;
}

#ifdef HAVE_ECC
/* This function is performing a sign using a private key for testing. In a
 * real-world use case this would be sent to HSM / TPM hardware for processing
 * and return WC_PENDING_E to give this thread time to do other work */
static int myEccSign(WOLFSSL* ssl, const byte* in, word32 inSz,
        byte* out, word32* outSz, const byte* key, word32 keySz, void* ctx)
{
    int ret;
    byte* keyBuf = (byte*)key;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    printf("PK ECC Sign: inSz %u, keySz %u\n", inSz, keySz);

#ifdef WOLFSSL_ASYNC_CRYPT
    if (cbInfo->state == 0) {
        cbInfo->state++;
        printf("PK ECC Sign: Async Simulate\n");
        return WC_PENDING_E;
    }
#endif

    ret = load_key_file(cbInfo->keyFile, &keyBuf, &keySz);
    if (ret == 0) {
        ret = wc_ecc_init(&cbInfo->keyEcc);
        if (ret == 0) {
            word32 idx = 0;
            ret = wc_EccPrivateKeyDecode(keyBuf, &idx, &cbInfo->keyEcc, keySz);
            if (ret == 0) {
                WC_RNG *rng = wolfSSL_GetRNG(ssl);

                printf("PK ECC Sign: Curve ID %d\n", cbInfo->keyEcc.dp->id);
                ret = wc_ecc_sign_hash(in, inSz, out, outSz, rng,
                    &cbInfo->keyEcc);
            }
            wc_ecc_free(&cbInfo->keyEcc);
        }
    }
    free(keyBuf);

#ifdef WOLFSSL_ASYNC_CRYPT
    cbInfo->state = 0;
#endif

    printf("PK ECC Sign: ret %d outSz %u\n", ret, *outSz);

    return ret;
}
#endif

#ifndef NO_RSA
static int myRsaSign(WOLFSSL* ssl, const byte* in, word32 inSz,
        byte* out, word32* outSz, const byte* key, word32 keySz, void* ctx)
{
    int     ret;
    word32  idx = 0;
    byte*   keyBuf = (byte*)key;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    printf("PK RSA Sign: inSz %u, keySz %u\n", inSz, keySz);

#ifdef WOLFSSL_ASYNC_CRYPT
    if (cbInfo->state == 0) {
        cbInfo->state++;
        printf("PK ECC Sign: Async Simulate\n");
        return WC_PENDING_E;
    }
#endif

    ret = load_key_file(cbInfo->keyFile, &keyBuf, &keySz);
    if (ret != 0)
        return ret;

    ret = wc_InitRsaKey(&cbInfo->keyRsa, NULL);
    if (ret == 0) {
        ret = wc_RsaPrivateKeyDecode(keyBuf, &idx, &cbInfo->keyRsa, keySz);
        if (ret == 0) {
            WC_RNG *rng = wolfSSL_GetRNG(ssl);
            ret = wc_RsaSSL_Sign(in, inSz, out, *outSz, &cbInfo->keyRsa, rng);
        }
        if (ret > 0) {  /* save and convert to 0 success */
            *outSz = ret;
            ret = 0;
        }
        wc_FreeRsaKey(&cbInfo->keyRsa);
    }
    free(keyBuf);
#ifdef WOLFSSL_ASYNC_CRYPT
    cbInfo->state = 0;
#endif

    printf("PK RSA Sign: ret %d, outSz %u\n", ret, *outSz);

    return ret;
}

#ifdef WC_RSA_PSS
static int myRsaPssSign(WOLFSSL* ssl, const byte* in, word32 inSz,
        byte* out, word32* outSz, int hash, int mgf, const byte* key,
        word32 keySz, void* ctx)
{
    enum wc_HashType hashType = WC_HASH_TYPE_NONE;
    int              ret;
    word32           idx = 0;
    byte*            keyBuf = (byte*)key;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    printf("PK RSA PSS Sign: inSz %u, hash %d, mgf %d, keySz %u\n",
        inSz, hash, mgf, keySz);

    ret = load_key_file(cbInfo->keyFile, &keyBuf, &keySz);
    if (ret != 0)
        return ret;

    switch (hash) {
#ifndef NO_SHA256
        case SHA256h:
            hashType = WC_HASH_TYPE_SHA256;
            break;
#endif
#ifdef WOLFSSL_SHA384
        case SHA384h:
            hashType = WC_HASH_TYPE_SHA384;
            break;
#endif
#ifdef WOLFSSL_SHA512
        case SHA512h:
            hashType = WC_HASH_TYPE_SHA512;
            break;
#endif
        default:
            hashType = WC_HASH_TYPE_NONE;
            break;
    }

    ret = wc_InitRsaKey(&cbInfo->keyRsa, NULL);
    if (ret == 0) {
        ret = wc_RsaPrivateKeyDecode(keyBuf, &idx, &cbInfo->keyRsa, keySz);
        if (ret == 0) {
            WC_RNG *rng = wolfSSL_GetRNG(ssl);
            ret = wc_RsaPSS_Sign(in, inSz, out, *outSz, hashType, mgf,
                &cbInfo->keyRsa, rng);
        }
        if (ret > 0) {  /* save and convert to 0 success */
            *outSz = ret;
            ret = 0;
        }
        wc_FreeRsaKey(&cbInfo->keyRsa);
    }
    free(keyBuf);

    printf("PK RSA PSS Sign: ret %d, outSz %u\n", ret, *outSz);

    return ret;
}
#endif
#endif

#endif /* HAVE_PK_CALLBACKS */

int main(int argc, char** argv)
{
    int                ret, err;
    int                sockfd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;

    /* PK callback context */
    PkCbInfo myCtx;
    memset(&myCtx, 0, sizeof(myCtx));
    myCtx.keyFile = KEY_FILE;

    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        return 0;
    }

#ifndef HAVE_PK_CALLBACKS
    printf("Warning: PK not compiled in! Please configure wolfSSL with "
           " --enable-pkcallbacks and try again\n");
    ret = -1;
    goto exit;
#endif

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

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
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
#if 0
    wolfSSL_Debugging_ON();
#endif

    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto exit;
    }

    /* Create and initialize WOLFSSL_CTX */
#ifdef USE_TLSV13
    ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
#else
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
#endif
    if (ctx == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

#ifdef HAVE_PK_CALLBACKS
    /* register sign callbacks for the long term key */
    #ifdef HAVE_ECC
    wolfSSL_CTX_SetEccSignCb(ctx, myEccSign);
    #endif
    #ifndef NO_RSA
    wolfSSL_CTX_SetRsaSignCb(ctx, myRsaSign);
    #ifdef WC_RSA_PSS
    wolfSSL_CTX_SetRsaPssSignCb(ctx, myRsaPssSign);
    #endif
    #endif
#endif

    /* Mutual Authentication */
    /* Load client certificate into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE,
                                    WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto exit;
    }

    /* Load client key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEYPUB_FILE,
                                    WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEYPUB_FILE);
        goto exit;
    }

    /* Load CA certificate into WOLFSSL_CTX for validating peer */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL))
         != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CA_FILE);
        goto exit;
    }

    /* validate peer certificate */
    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, NULL);

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

#ifdef HAVE_PK_CALLBACKS
    /* setup the PK context */
    #ifdef HAVE_ECC
    wolfSSL_SetEccSignCtx(ssl, &myCtx);
    #endif
    #ifndef NO_RSA
    wolfSSL_SetRsaSignCtx(ssl, &myCtx);
    #ifdef WC_RSA_PSS
    wolfSSL_SetRsaPssSignCtx(ssl, &myCtx);
    #endif
    #endif
#else
    (void)myCtx; /* not used */
#endif

    /* Connect to wolfSSL on the server side */
    do {
        ret = wolfSSL_connect(ssl);
        err = wolfSSL_get_error(ssl, ret);
    } while (err == WC_PENDING_E);
    if (ret != WOLFSSL_SUCCESS) {
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
