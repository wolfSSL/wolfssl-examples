/* client-tls-cryptocb.c
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
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define DEFAULT_PORT 11111

#define CA_FILE "../certs/ca-cert.pem"

#ifdef WOLF_CRYPTO_CB
/* Example custom context for crypto callback */
typedef struct {
    int exampleVar; /* example, not used */
} myCryptoCbCtx;

static void error_out(char* msg, int err)
{
    printf("Failed at %s with code %d\n", msg, err);
    exit(1);
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
#if !defined(NO_SHA) || !defined(NO_SHA256)
    #if !defined(NO_SHA)
        if (info->hash.type == WC_HASH_TYPE_SHA) {
            if (info->hash.sha1 == NULL)
                return CRYPTOCB_UNAVAILABLE;

            /* set devId to invalid, so software is used */
            info->hash.sha1->devId = INVALID_DEVID;

            if (info->hash.in != NULL) {
                ret = wc_ShaUpdate(
                    info->hash.sha1,
                    info->hash.in,
                    info->hash.inSz);
            }
            if (info->hash.digest != NULL) {
                ret = wc_ShaFinal(
                    info->hash.sha1,
                    info->hash.digest);
            }

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

            if (info->hash.in != NULL) {
                ret = wc_Sha256Update(
                    info->hash.sha256,
                    info->hash.in,
                    info->hash.inSz);
            }
            if (info->hash.digest != NULL) {
                ret = wc_Sha256Final(
                    info->hash.sha256,
                    info->hash.digest);
            }

            /* reset devId */
            info->hash.sha256->devId = devIdArg;
        }
        else
    #endif
        {
        }
#endif /* !NO_SHA || !NO_SHA256 */
    }
    else if (info->algo_type == WC_ALGO_TYPE_HMAC) {
#ifndef NO_HMAC
        if (info->hmac.hmac == NULL)
            return CRYPTOCB_UNAVAILABLE;

        /* set devId to invalid, so software is used */
        info->hmac.hmac->devId = INVALID_DEVID;

        if (info->hash.in != NULL) {
            ret = wc_HmacUpdate(
                info->hmac.hmac,
                info->hmac.in,
                info->hmac.inSz);
        }
        else if (info->hash.digest != NULL) {
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
    int                sockfd;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;

    int devId = 1; /* anything besides -2 (INVALID_DEVID) */
    myCryptoCbCtx myCtx;

    /* example data for callback */
    myCtx.exampleVar = 1;

    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        return 0;
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto end;
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
        goto end;
    }

    /* Connect to the server */
    if ((ret = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)))
         == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto end;
    }

    /*---------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------*/
    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto socket_cleanup;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto socket_cleanup;
    }

    /* register a devID for crypto callbacks */
    ret = wc_CryptoCb_RegisterDevice(devId, myCryptoCb, &myCtx);
    if (ret != 0)
        error_out("wc_CryptoCb_RegisterDevice", ret);

    /* register a devID for crypto callbacks */
    wolfSSL_CTX_SetDevId(ctx, devId);

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CA_FILE, NULL))
         != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CA_FILE);
        goto ctx_cleanup;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto ctx_cleanup;
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto cleanup;
    }

    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(ssl)) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        goto cleanup;
    }

    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto cleanup;
    }
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to write entire message\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto cleanup;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);

    ret = 0;

    /* Cleanup and return */
cleanup:
    wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
ctx_cleanup:
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
socket_cleanup:
    close(sockfd);          /* Close the connection to the server       */
end:

#else
    printf("Please configure wolfSSL with --enable-cryptocb and try again\n");
#endif /* WOLF_CRYPTO_CB */
    return ret;               /* Return reporting a success               */
}
