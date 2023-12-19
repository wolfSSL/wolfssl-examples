/* server-tls-cryptocb.c
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

#define DEFAULT_PORT 11111

#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"

#ifdef WOLF_CRYPTO_CB
/* Example custom context for crypto callback */
typedef struct {
    int exampleVar; /* example, not used */
} myCryptoCbCtx;

typedef struct {
    word32 bufSz;
} hash_ctx_t;

/* type: WC_HASH_TYPE_SHA, WC_HASH_TYPE_SHA256, WC_HASH_TYPE_SHA384, etc */
/* in: Update (when not NULL) / Final (when NULL) */
static int cb_hash(int type, const byte* in, word32 inSz, byte* digest,
    void* shactx, void** devCtx, word32 flags)
{
    int ret = 0;
    enum wc_HashType hash_type = (enum wc_HashType)type;
    hash_ctx_t* ctx = (hash_ctx_t*)*devCtx;
    byte*  hashBuf = NULL;
    word32 hashBufSz = 0;

    /* for updates alloc/realloc and copy */
    if (in != NULL) {
        if (ctx == NULL) {
            ctx = (hash_ctx_t*)malloc(sizeof(hash_ctx_t) + hashBufSz + inSz);
        }
        else {
            hashBufSz = ctx->bufSz;
            ctx = (hash_ctx_t*)realloc(ctx, sizeof(hash_ctx_t) + hashBufSz + inSz);
        }
        if (ctx == NULL) {
            return MEMORY_E;
        }
        hashBuf = (byte*)ctx + sizeof(hash_ctx_t);
        memcpy(&hashBuf[hashBufSz], in, inSz);
        ctx->bufSz = hashBufSz + inSz;
        *devCtx = ctx;
    }
    /* final */
    else if (digest != NULL) {
        if (ctx == NULL) {
            /* valid case of empty hash (0 len hash) */
        }
        else {
            hashBuf = (byte*)ctx + sizeof(hash_ctx_t);
            hashBufSz = ctx->bufSz;
        }
        ret = wc_Hash_ex(hash_type,
            hashBuf, hashBufSz,
            digest, wc_HashGetDigestSize(hash_type),
            NULL, INVALID_DEVID);

        if (!(flags & WC_HASH_FLAG_ISCOPY)) {
            free(ctx);
            *devCtx = NULL;
        }
    }
    return ret;
}

/* Example crypto dev callback function that calls software version */
/* This is where you would plug-in calls to your own hardware crypto */
static int myCryptoCb(int devIdArg, wc_CryptoInfo* info, void* ctx)
{
    int ret = CRYPTOCB_UNAVAILABLE; /* return this to bypass HW and use SW */
    myCryptoCbCtx* myCtx = (myCryptoCbCtx*)ctx;

    if (info == NULL)
        return BAD_FUNC_ARG;

#ifdef DEBUG_CRYPTOCB
    wc_CryptoCb_InfoString(info);
#endif

    if (info->algo_type == WC_ALGO_TYPE_RNG) {
    #ifndef WC_NO_RNG
        /* set devId to invalid, so software is used */
        info->rng.rng->devId = INVALID_DEVID;

        ret = wc_RNG_GenerateBlock(info->rng.rng,
            info->rng.out, info->rng.sz);

        /* reset devId */
        info->rng.rng->devId = devIdArg;
    #endif
    }
    else if (info->algo_type == WC_ALGO_TYPE_SEED) {
    #ifndef WC_NO_RNG
        static byte seed[sizeof(word32)] = { 0x00, 0x00, 0x00, 0x01 };
        word32* seedWord32 = (word32*)seed;
        word32 len;

        /* wc_GenerateSeed is a local symbol so we need to fake the entropy. */
        while (info->seed.sz > 0) {
            len = (word32)sizeof(seed);
            if (info->seed.sz < len)
                len = info->seed.sz;
            XMEMCPY(info->seed.seed, seed, sizeof(seed));
            info->seed.seed += len;
            info->seed.sz -= len;
            (*seedWord32)++;
        }

        ret = 0;
    #endif
    }
    else if (info->algo_type == WC_ALGO_TYPE_PK) {
    #ifndef NO_RSA
        if (info->pk.type == WC_PK_TYPE_RSA) {
            /* set devId to invalid, so software is used */
            info->pk.rsa.key->devId = INVALID_DEVID;

            switch (info->pk.rsa.type) {
                case RSA_PUBLIC_ENCRYPT:
                case RSA_PUBLIC_DECRYPT:
                    /* perform software based RSA public op */
                    ret = wc_RsaFunction(
                        info->pk.rsa.in, info->pk.rsa.inLen,
                        info->pk.rsa.out, info->pk.rsa.outLen,
                        info->pk.rsa.type, info->pk.rsa.key, info->pk.rsa.rng);
                    break;
                case RSA_PRIVATE_ENCRYPT:
                case RSA_PRIVATE_DECRYPT:
                    /* perform software based RSA private op */
                    ret = wc_RsaFunction(
                        info->pk.rsa.in, info->pk.rsa.inLen,
                        info->pk.rsa.out, info->pk.rsa.outLen,
                        info->pk.rsa.type, info->pk.rsa.key, info->pk.rsa.rng);
                    break;
            }

            /* reset devId */
            info->pk.rsa.key->devId = devIdArg;
        }
    #ifdef WOLFSSL_KEY_GEN
        else if (info->pk.type == WC_PK_TYPE_RSA_KEYGEN) {
            info->pk.rsakg.key->devId = INVALID_DEVID;

            ret = wc_MakeRsaKey(info->pk.rsakg.key, info->pk.rsakg.size,
                info->pk.rsakg.e, info->pk.rsakg.rng);

            /* reset devId */
            info->pk.rsakg.key->devId = devIdArg;
        }
    #endif
    #endif /* !NO_RSA */
    #ifdef HAVE_ECC
        if (info->pk.type == WC_PK_TYPE_EC_KEYGEN) {
            /* set devId to invalid, so software is used */
            info->pk.eckg.key->devId = INVALID_DEVID;

            ret = wc_ecc_make_key_ex(info->pk.eckg.rng, info->pk.eckg.size,
                info->pk.eckg.key, info->pk.eckg.curveId);

            /* reset devId */
            info->pk.eckg.key->devId = devIdArg;
        }
        else if (info->pk.type == WC_PK_TYPE_ECDSA_SIGN) {
            /* set devId to invalid, so software is used */
            info->pk.eccsign.key->devId = INVALID_DEVID;

            ret = wc_ecc_sign_hash(
                info->pk.eccsign.in, info->pk.eccsign.inlen,
                info->pk.eccsign.out, info->pk.eccsign.outlen,
                info->pk.eccsign.rng, info->pk.eccsign.key);

            /* reset devId */
            info->pk.eccsign.key->devId = devIdArg;
        }
        else if (info->pk.type == WC_PK_TYPE_ECDSA_VERIFY) {
            /* set devId to invalid, so software is used */
            info->pk.eccverify.key->devId = INVALID_DEVID;

            ret = wc_ecc_verify_hash(
                info->pk.eccverify.sig, info->pk.eccverify.siglen,
                info->pk.eccverify.hash, info->pk.eccverify.hashlen,
                info->pk.eccverify.res, info->pk.eccverify.key);

            /* reset devId */
            info->pk.eccverify.key->devId = devIdArg;
        }
        else if (info->pk.type == WC_PK_TYPE_ECDH) {
            /* set devId to invalid, so software is used */
            info->pk.ecdh.private_key->devId = INVALID_DEVID;

            ret = wc_ecc_shared_secret(
                info->pk.ecdh.private_key, info->pk.ecdh.public_key,
                info->pk.ecdh.out, info->pk.ecdh.outlen);

            /* reset devId */
            info->pk.ecdh.private_key->devId = devIdArg;
        }
    #endif /* HAVE_ECC */
    }
    else if (info->algo_type == WC_ALGO_TYPE_CIPHER) {
#if !defined(NO_AES) || !defined(NO_DES3)
    #ifdef HAVE_AESGCM
        if (info->cipher.type == WC_CIPHER_AES_GCM) {
            if (info->cipher.enc) {
                /* set devId to invalid, so software is used */
                info->cipher.aesgcm_enc.aes->devId = INVALID_DEVID;

                ret = wc_AesGcmEncrypt(
                    info->cipher.aesgcm_enc.aes,
                    info->cipher.aesgcm_enc.out,
                    info->cipher.aesgcm_enc.in,
                    info->cipher.aesgcm_enc.sz,
                    info->cipher.aesgcm_enc.iv,
                    info->cipher.aesgcm_enc.ivSz,
                    info->cipher.aesgcm_enc.authTag,
                    info->cipher.aesgcm_enc.authTagSz,
                    info->cipher.aesgcm_enc.authIn,
                    info->cipher.aesgcm_enc.authInSz);

                /* reset devId */
                info->cipher.aesgcm_enc.aes->devId = devIdArg;
            }
            else {
                /* set devId to invalid, so software is used */
                info->cipher.aesgcm_dec.aes->devId = INVALID_DEVID;

                ret = wc_AesGcmDecrypt(
                    info->cipher.aesgcm_dec.aes,
                    info->cipher.aesgcm_dec.out,
                    info->cipher.aesgcm_dec.in,
                    info->cipher.aesgcm_dec.sz,
                    info->cipher.aesgcm_dec.iv,
                    info->cipher.aesgcm_dec.ivSz,
                    info->cipher.aesgcm_dec.authTag,
                    info->cipher.aesgcm_dec.authTagSz,
                    info->cipher.aesgcm_dec.authIn,
                    info->cipher.aesgcm_dec.authInSz);

                /* reset devId */
                info->cipher.aesgcm_dec.aes->devId = devIdArg;
            }
        }
    #endif /* HAVE_AESGCM */
    #ifdef HAVE_AES_CBC
        if (info->cipher.type == WC_CIPHER_AES_CBC) {
            if (info->cipher.enc) {
                /* set devId to invalid, so software is used */
                info->cipher.aescbc.aes->devId = INVALID_DEVID;

                ret = wc_AesCbcEncrypt(
                    info->cipher.aescbc.aes,
                    info->cipher.aescbc.out,
                    info->cipher.aescbc.in,
                    info->cipher.aescbc.sz);

                /* reset devId */
                info->cipher.aescbc.aes->devId = devIdArg;
            }
            else {
                /* set devId to invalid, so software is used */
                info->cipher.aescbc.aes->devId = INVALID_DEVID;

                ret = wc_AesCbcDecrypt(
                    info->cipher.aescbc.aes,
                    info->cipher.aescbc.out,
                    info->cipher.aescbc.in,
                    info->cipher.aescbc.sz);

                /* reset devId */
                info->cipher.aescbc.aes->devId = devIdArg;
            }
        }
    #endif /* HAVE_AES_CBC */
    #ifndef NO_DES3
        if (info->cipher.type == WC_CIPHER_DES3) {
            if (info->cipher.enc) {
                /* set devId to invalid, so software is used */
                info->cipher.des3.des->devId = INVALID_DEVID;

                ret = wc_Des3_CbcEncrypt(
                    info->cipher.des3.des,
                    info->cipher.des3.out,
                    info->cipher.des3.in,
                    info->cipher.des3.sz);

                /* reset devId */
                info->cipher.des3.des->devId = devIdArg;
            }
            else {
                /* set devId to invalid, so software is used */
                info->cipher.des3.des->devId = INVALID_DEVID;

                ret = wc_Des3_CbcDecrypt(
                    info->cipher.des3.des,
                    info->cipher.des3.out,
                    info->cipher.des3.in,
                    info->cipher.des3.sz);

                /* reset devId */
                info->cipher.des3.des->devId = devIdArg;
            }
        }
    #endif /* !NO_DES3 */
#endif /* !NO_AES || !NO_DES3 */
    }
    else if (info->algo_type == WC_ALGO_TYPE_HASH) {
#if !defined(NO_SHA) || !defined(NO_SHA256) || defined(WOLFSSL_SHA384) || \
     defined(WOLFSSL_SHA512)
    #if !defined(NO_SHA)
        if (info->hash.type == WC_HASH_TYPE_SHA) {
            if (info->hash.sha1 == NULL)
                return CRYPTOCB_UNAVAILABLE;

            /* set devId to invalid, so software is used */
            info->hash.sha1->devId = INVALID_DEVID;

            ret = cb_hash(info->hash.type, info->hash.in, info->hash.inSz,
                info->hash.digest, info->hash.sha1, &info->hash.sha1->devCtx,
                info->hash.sha1->flags);

            /* reset devId */
            info->hash.sha1->devId = devIdArg;
        }
        else
    #endif
    #if !defined(NO_SHA256)
        if (info->hash.type == WC_HASH_TYPE_SHA256) {
            if (info->hash.sha256 == NULL)
                return CRYPTOCB_UNAVAILABLE;

            /* set devId to invalid, so software is used */
            info->hash.sha256->devId = INVALID_DEVID;

            ret = cb_hash(info->hash.type, info->hash.in, info->hash.inSz,
                info->hash.digest, info->hash.sha256, &info->hash.sha256->devCtx,
                info->hash.sha256->flags);

            /* reset devId */
            info->hash.sha256->devId = devIdArg;
        }
        else
    #endif
    #ifdef WOLFSSL_SHA384
        if (info->hash.type == WC_HASH_TYPE_SHA384) {
            if (info->hash.sha384 == NULL)
                return CRYPTOCB_UNAVAILABLE;

            /* set devId to invalid, so software is used */
            info->hash.sha384->devId = INVALID_DEVID;

            ret = cb_hash(info->hash.type, info->hash.in, info->hash.inSz,
                info->hash.digest, info->hash.sha384, &info->hash.sha384->devCtx,
                info->hash.sha384->flags);

            /* reset devId */
            info->hash.sha384->devId = devIdArg;
        }
        else
    #endif
    #ifdef WOLFSSL_SHA512
        if (info->hash.type == WC_HASH_TYPE_SHA512) {
            if (info->hash.sha512 == NULL)
                return CRYPTOCB_UNAVAILABLE;

            /* set devId to invalid, so software is used */
            info->hash.sha512->devId = INVALID_DEVID;

            ret = cb_hash(info->hash.type, info->hash.in,  info->hash.inSz,
                info->hash.digest, info->hash.sha512, &info->hash.sha512->devCtx,
                info->hash.sha512->flags);

            /* reset devId */
            info->hash.sha512->devId = devIdArg;
        }
        else
    #endif
        {
        }
#endif /* !NO_SHA || !NO_SHA256 || WOLFSSL_SHA384 || WOLFSSL_SHA512 */
    }
    else if (info->algo_type == WC_ALGO_TYPE_HMAC) {
#ifndef NO_HMAC
        if (info->hmac.hmac == NULL)
            return CRYPTOCB_UNAVAILABLE;

        /* set devId to invalid, so software is used */
        info->hmac.hmac->devId = INVALID_DEVID;

        if (info->hmac.in != NULL) {
            ret = wc_HmacUpdate(
                info->hmac.hmac,
                info->hmac.in,
                info->hmac.inSz);
        }
        else if (info->hmac.digest != NULL) {
            ret = wc_HmacFinal(
                info->hmac.hmac,
                info->hmac.digest);
        }

        /* reset devId */
        info->hmac.hmac->devId = devIdArg;
#endif
    }

    (void)devIdArg;
    (void)myCtx;

    return ret;
}
#endif /* WOLF_CRYPTO_CB */


int main(int argc, char** argv)
{
    int                ret = 0;
#ifdef WOLF_CRYPTO_CB
    int                sockfd = SOCKET_INVALID;
    int                connd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    int                shutdown = 0;
    const char*        reply = "I hear ya fa shizzle!\n";
    int                on;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;
    WOLFSSL_CIPHER* cipher;

    int devId = 1; /* anything besides -2 (INVALID_DEVID) */
    myCryptoCbCtx myCtx;

#if 0
    wolfSSL_Debugging_ON();
#endif

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* example data for callback */
    memset(&myCtx, 0, sizeof(myCtx));
    myCtx.exampleVar = 1;


    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto exit;
    }

    /* Load server certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto exit;
    }

    /* Load server key into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM))
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        goto exit;
    }

#if 0
    wolfSSL_CTX_set_cipher_list(ctx, "TLS13-AES256-GCM-SHA384");
#endif

    /* register a devID for crypto callbacks */
    ret = wc_CryptoCb_RegisterDevice(devId, myCryptoCb, &myCtx);
    if (ret != 0) {
        fprintf(stderr, "ERROR: wc_CryptoCb_RegisterDevice failed %d\n", ret);
        goto exit;
    }

    /* register a devID for crypto callbacks */
    wolfSSL_CTX_SetDevId(ctx, devId);


    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto exit;
    }

    on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
        (char*)&on, (socklen_t)sizeof(on));


    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */


    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto exit;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1;
        goto exit;
    }


    /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            ret = -1;
            goto exit;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            ret = -1;
            goto exit;
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);

        /* Establish TLS connection */
        ret = wolfSSL_accept(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_accept error = %d\n",
                wolfSSL_get_error(ssl, ret));
            goto exit;
        }


        printf("Client connected successfully\n");

        cipher = wolfSSL_get_current_cipher(ssl);
        printf("SSL cipher suite is %s\n", wolfSSL_CIPHER_get_name(cipher));


        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
            fprintf(stderr, "ERROR: failed to read\n");
            goto exit;
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
        if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            goto exit;
        }

        /* Notify the client that the connection is ending */
        wolfSSL_shutdown(ssl);
        printf("Shutdown complete\n");

        /* Cleanup after this connection */
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        ssl = NULL;
        close(connd);           /* Close the connection to the client   */
    }

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
#else
    printf("Please configure wolfSSL with --enable-cryptocb and try again\n");
#endif /* WOLF_CRYPTO_CB */

    (void)argc;
    (void)argv;

    return ret;               /* Return reporting a success               */
}
