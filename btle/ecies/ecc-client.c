/* ecc-client.c
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
    ecEncCtx* cliCtx = NULL;
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
    ret = btle_open(&devCtx, BTLE_ROLE_CLIENT);
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

    cliCtx = wc_ecc_ctx_new(REQ_RESP_CLIENT, &rng);
    if (cliCtx == NULL) {
        printf("wc_ecc_ctx_new failed!\n");
        ret = -1; goto cleanup;
    }

    /* exchange public keys */
    /* export my public key */
    bufferSz = sizeof(buffer);
    ret = wc_ecc_export_x963(&myKey, buffer, &bufferSz);
    if (ret != 0) {
        printf("wc_ecc_export_x963 failed %d\n", ret);
        goto cleanup;
    }
    /* send my public key */
    ret = btle_send(buffer, bufferSz, BTLE_PKT_TYPE_KEY, devCtx);
    if (ret != bufferSz) {
        printf("btle_send key failed %d!\n", ret);
        goto cleanup;
    }

    /* Get peer key */
    ret = btle_recv(buffer, sizeof(buffer), &type, devCtx);
    if (ret <= 0) {
        printf("btle_recv key failed %d\n", ret);
        goto cleanup;
    }
    if (type != BTLE_PKT_TYPE_KEY) {
        printf("btle_recv expected key!\n");
        ret = -1; goto cleanup;
    }

    /* TODO: Client should hash and verify this public key against trusted certificate (already exchanged) */
    /* ECC signature is about 65 bytes */

    /* import peer public key */
    bufferSz = ret;
    ret = wc_ecc_import_x963(buffer, bufferSz, &peerKey);
    if (ret != 0) {
        printf("wc_ecc_import_x963 failed %d\n", ret);
        goto cleanup;
    }

    /* Collect Message to send and get echo */
    while (1) {
        /* get my salt */
        mySalt = wc_ecc_ctx_get_own_salt(cliCtx);
        if (mySalt == NULL) {
            printf("wc_ecc_ctx_get_own_salt failed!\n");
            ret = -1; goto cleanup;
        }

        /* Send my salt */
        ret = btle_send(mySalt, EXCHANGE_SALT_SZ, BTLE_PKT_TYPE_SALT, devCtx);
        if (ret != EXCHANGE_SALT_SZ) {
            printf("btle_send salt failed %d!\n", ret);
            goto cleanup;
        }

        /* Get peer salt */
        ret = btle_recv(peerSalt, EXCHANGE_SALT_SZ, &type, devCtx);
        if (ret <= 0) {
            printf("btle_recv failed %d!\n", ret);
        }
        if (type != BTLE_PKT_TYPE_SALT) {
            printf("btle_recv expected salt!\n");
            ret = -1; goto cleanup;
        }

        ret = wc_ecc_ctx_set_peer_salt(cliCtx, peerSalt);
        if (ret != 0) {
            printf("wc_ecc_ctx_set_peer_salt failed %d\n", ret);
            goto cleanup;
        }

        /* get message to send */
        printf("Enter text to send:\n");
        plainSz = sizeof(plain)-1;
        if (fgets((char*)plain, plainSz, stdin) == NULL) {
            printf("stdin get failed\n");
            goto cleanup;
        }
        plainSz = strlen((char*)plain);
        /* pad message at 16 bytes for AES block size */
        ret = btle_msg_pad(plain, (int*)&plainSz, devCtx);
        if (ret != 0) {
            printf("btle_msg_pad failed %d\n", ret);
            goto cleanup;
        }

        /* Encrypt message */
        bufferSz = sizeof(buffer);
        ret = wc_ecc_encrypt(&myKey, &peerKey, plain, plainSz, buffer, &bufferSz, cliCtx);
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

        /* Get message */
        bufferSz = sizeof(buffer);
        ret = btle_recv(buffer, bufferSz, &type, devCtx);
        if (type != BTLE_PKT_TYPE_MSG) {
            ret = -1; goto cleanup;
        }

        /* Decrypt message */
        bufferSz = ret;
        plainSz = sizeof(plain);
        ret = wc_ecc_decrypt(&myKey, &peerKey, buffer, bufferSz, plain, &plainSz, cliCtx);
        if (ret != 0) {
            printf("wc_ecc_decrypt failed %d!\n", ret);
            goto cleanup;
        }

        printf("Recv %d: %s\n", plainSz, plain);

        /* check for exit flag */
        if (strncmp((char*)plain, EXIT_STRING, strlen(EXIT_STRING)) == 0) {
            printf("Exit, closing connection\n");
            break;
        }

        /* reset context (reset my salt) */
        ret = wc_ecc_ctx_reset(cliCtx, &rng);
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
