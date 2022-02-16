/* ecc-server.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
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

#include "btle-sim.h"

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/ecc.h>

int main(int argc, char** argv)
{
    int ret = 0;
#ifdef HAVE_ECC_ENCRYPT
    WC_RNG rng;
    ecEncCtx* srvCtx = NULL;
    void* devCtx = NULL;
    const byte* mySalt;
    byte peerSalt[EXCHANGE_SALT_SZ];
    byte buffer[BTLE_MSG_MAX_SIZE];
    word32 bufferSz;
    byte plain[BTLE_MSG_MAX_SIZE];
    word32 plainSz;
    ecc_key myKey, peerKey;
    int type;

    wolfCrypt_Init();

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    /* make my session key */
    ret =  wc_ecc_init(&myKey);
    ret |= wc_ecc_init(&peerKey);
    if (ret != 0) {
        printf("wc_ecc_init failed!\n");
        goto cleanup;
    }

    /* open BTLE */
    printf("Waiting for client\n");
    ret = btle_open(&devCtx, BTLE_ROLE_SERVER);
    if (ret != 0) {
        printf("btle_open failed %d!\n", ret);
        goto cleanup;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("wc_InitRng failed! %d\n", ret);
        goto cleanup;
    }

    ret = wc_ecc_make_key(&rng, 32, &myKey);
    if (ret != 0) {
        printf("wc_ecc_make_key failed %d\n", ret);
        goto cleanup;
    }

    srvCtx = wc_ecc_ctx_new(REQ_RESP_SERVER, &rng);
    if (srvCtx == NULL) {
        printf("wc_ecc_ctx_new failed!\n");
        ret = -1; goto cleanup;
    }

    /* exchange public keys */
    /* Get peer key */
    ret = btle_recv(buffer, sizeof(buffer), &type, devCtx);
    if (ret < 0) {
        printf("btle_recv key failed %d!\n", ret);
        goto cleanup;
    }
    if (type != BTLE_PKT_TYPE_KEY) {
        printf("btle_recv expected key!\n");
        ret = -1; goto cleanup;
    }
    bufferSz = ret;
    ret = wc_ecc_import_x963(buffer, bufferSz, &peerKey);
    if (ret != 0) {
        printf("wc_ecc_import_x963 failed %d!\n", ret);
        goto cleanup;
    }

    /* send my public key */
    /* export my public key */
    bufferSz = sizeof(buffer);
    ret = wc_ecc_export_x963(&myKey, buffer, &bufferSz);
    if (ret != 0) {
        printf("wc_ecc_export_x963 failed %d\n", ret);
        goto cleanup;
    }
    /* TODO: Server should hash and sign this public key with a trust certificate (already exchanged) */
    /* ECC signature is about 65 bytes */


    ret = btle_send(buffer, bufferSz, BTLE_PKT_TYPE_KEY, devCtx);
    if (ret != bufferSz) {
        printf("btle_send key failed %d!\n", ret);
        goto cleanup;
    }

    while (1) {
        mySalt = wc_ecc_ctx_get_own_salt(srvCtx);
        if (mySalt == NULL) {
            printf("wc_ecc_ctx_get_own_salt failed!\n");
            ret = -1; goto cleanup;
        }

        /* Get peer salt */
        ret = btle_recv(peerSalt, EXCHANGE_SALT_SZ, &type, devCtx);
        if (ret <= 0) {
            printf("btle_recv salt failed %d!\n", ret);
            goto cleanup;
        }
        if (type != BTLE_PKT_TYPE_SALT) {
            printf("btle_recv expected salt!\n");
            ret = -1; goto cleanup;
        }

        /* Send my salt */
        /* You must send mySalt before set_peer_salt, because buffer changes */
        ret = btle_send(mySalt, EXCHANGE_SALT_SZ, BTLE_PKT_TYPE_SALT, devCtx);
        if (ret != EXCHANGE_SALT_SZ) {
            printf("btle_send salt failed %d!\n", ret);
            goto cleanup;
        }

        ret = wc_ecc_ctx_set_peer_salt(srvCtx, peerSalt);
        if (ret != 0) {
            printf("wc_ecc_ctx_set_peer_salt failed %d\n", ret);
            goto cleanup;
        }

        /* Get message */
        bufferSz = sizeof(buffer);
        ret = btle_recv(buffer, bufferSz, &type, devCtx);
        if (ret <= 0) {
            printf("btle_recv msg failed %d!\n", ret);
            goto cleanup;
        }
        if (type != BTLE_PKT_TYPE_MSG) {
            printf("btle_recv expected msg!\n");
            ret = -1; goto cleanup;
        }

        /* Decrypt message */
        bufferSz = ret;
        plainSz = sizeof(plain);
        ret = wc_ecc_decrypt(&myKey, &peerKey, buffer, bufferSz, plain, &plainSz, srvCtx);
        if (ret != 0) {
            printf("wc_ecc_decrypt failed %d!\n", ret);
            goto cleanup;
        }

        printf("Recv %d: %s\n", plainSz, plain);

        /* Encrypt message */
        bufferSz = sizeof(buffer);
        ret = wc_ecc_encrypt(&myKey, &peerKey, plain, plainSz, buffer, &bufferSz, srvCtx);
        if (ret != 0) {
            printf("wc_ecc_encrypt failed %d!\n", ret);
            goto cleanup;
        }

        /* Send message */
        ret = btle_send(buffer, bufferSz, BTLE_PKT_TYPE_MSG, devCtx);
        if (ret != bufferSz) {
            printf("btle_send failed %d!\n", ret);
            goto cleanup;
        }

        /* check for exit flag */
        if (strncmp((char*)plain, EXIT_STRING, strlen(EXIT_STRING)) == 0) {
            printf("Exit, closing connection\n");
            break;
        }

        /* reset context (reset my salt) */
        ret = wc_ecc_ctx_reset(srvCtx, &rng);
        if (ret != 0) {
            printf("wc_ecc_ctx_reset failed %d\n", ret);
            goto cleanup;
        }
    }

cleanup:

    if (devCtx != NULL)
        btle_close(devCtx);

    wolfCrypt_Cleanup();
#else
    printf("Please compile wolfSSL with --enable-eccencrypt or HAVE_ECC_ENCRYPT\n");
#endif
    return ret;
}
